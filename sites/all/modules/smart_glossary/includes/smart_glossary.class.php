<?php

/**
 * @file
 * The Smart Glossary class file
 */

/**
 * The Smart Glossary class
 */
class SmartGlossary {

  protected static $instance;
  protected $config;
  protected $timeout;
  protected $defaultLanguage;
  protected $languages;
  protected $glossaryStore;

  /**
   * Constructor of the Smart Glossary class.
   */
  protected function __construct($config) {
    $this->config = $config;
    $this->glossaryStore = $config->connection->getApi();
    $this->languages = _smart_glossary_get_available_languages($config);
    $this->defaultLanguage = $this->languages[0];
  }

  /**
   * Get a smart-glossary-instance (Singleton).
   */
  public static function getInstance($config) {
    if (!isset(self::$instance)) {
      $object_name = __CLASS__;
      self::$instance = new $object_name($config);
    }
    return self::$instance;
  }

  public function available() {
    return $this->config->connection->available();
  }

  /**
   * Returns data for the autocomplete field.
   *
   * @param string $string
   *   The search string
   * @param int $limit
   *   Optional; the maximum number of concepts to be found, default 15
   * @param string $language
   *   The language in which you want to search
   *
   * @return array
   *   A list of concepts with following parameters:
   *   - label: prefLabel of the concept
   *   - id: the concept ID
   *   - encoded: the slaged prefLabel for creating a friendly URL
   */
  public function autoComplete($string, $limit = 15, $language = '') {
    if (empty($string) || $this->glossaryStore === FALSE) {
      return array();
    }

    if (empty($language)) {
      $language = $this->defaultLanguage;
    }

    $query = "
      PREFIX skos:<http://www.w3.org/2004/02/skos/core#>

      SELECT DISTINCT ?concept ?label ?prefLabel
      WHERE {
        ?concept a skos:Concept.
        {
          ?concept skos:prefLabel ?label FILTER(regex(str(?label),'$string','i') && lang(?label) = '$language').
          ?concept skos:prefLabel ?prefLabel FILTER(lang(?prefLabel) = '$language').
        } UNION {
          ?concept skos:altLabel ?label FILTER(regex(str(?label),'$string','i') && lang(?label) = '$language').
          ?concept skos:prefLabel ?prefLabel FILTER(lang(?prefLabel) = '$language').
        }
      }
      ORDER BY ASC(?label)
      LIMIT $limit";

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      $this->error($e->getMessage());
      return array();
    }

    // Sort the labels.
    $list_start = array();
    $list_middle = array();
    foreach ($rows as $data) {
      $label = $this->rebuiltLabel($data->label->getValue());
      $prefLabel = $this->rebuiltLabel($data->prefLabel->getValue());
      $label .= ($label == $prefLabel) ? '' : ' (' . $prefLabel . ')';
      if (stripos($label, $string) > 0) {
        $list_middle[] = array(
          'label' => $label,
          'url' => $this->createUrl($data->concept->getUri(), $prefLabel, $language),
        );
      }
      else {
        $list_start[] = array(
          'label' => $label,
          'url' => $this->createUrl($data->concept->getUri(), $prefLabel, $language),
        );
      }
    }

    usort($list_start, array($this, 'sortAlpha'));
    usort($list_middle, array($this, 'sortAlpha'));
    $list = array_merge($list_start, $list_middle);

    return $list;
  }

  /**
   * Get a list of concepts.
   *
   * @param string $char
   *   The starting letter of the concepts
   * @param string $language
   *   Optional; the language of the objects
   * @param int $limit
   *   Optional; The maximum number of items to receive.
   *   0 means no limit.
   *
   * @return array
   *   An array of concepts-objects
   */
  public function getList($char, $language = '', $limit = 0) {
    if ($this->glossaryStore === FALSE) {
      return array();
    }

    if (empty($language)) {
      $language = $this->defaultLanguage;
    }

    $prefLabel_filter = !empty($char) ? "regex(str(?prefLabel),'^ *$char','i') &&" : '';

    $query = "
      PREFIX skos:<http://www.w3.org/2004/02/skos/core#>

      SELECT DISTINCT ?concept ?prefLabel ?broaderLabel
      WHERE {
        ?concept a skos:Concept.
        ?concept skos:prefLabel ?prefLabel FILTER($prefLabel_filter lang(?prefLabel) = '$language').
        OPTIONAL {
          ?concept skos:broader ?broader.
          ?broader skos:prefLabel ?broaderLabel FILTER(lang(?broaderLabel) = '$language').
        }
      }";
    if ($limit > 0) {
      $query .= "LIMIT $limit";
    }

    // Offer the possibility to alter the query before its execution.
    $alter_context = array(
      'char' => $char,
      'language' => $language,
      'limit' => $limit,
    );
    drupal_alter('smart_glossary_list_query', $query, $alter_context);

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      $this->error($e->getMessage());
      return array();
    }

    $concepts     = array();
    $last_concept = array();
    foreach ($rows as $data) {
      if (!isset($data->concept) || !isset($data->prefLabel)) {
        continue;
      }
      $concept_uri = $data->concept->getUri();
      $concepts[$concept_uri] = new stdClass();
      $concepts[$concept_uri]->uri  = $data->concept->getUri();
      $concepts[$concept_uri]->prefLabel = $this->rebuiltLabel($data->prefLabel->getValue());
      $concepts[$concept_uri]->url = $this->createUrl($concept_uri, $data->prefLabel->getValue(), $language);
      if (!isset($concepts[$concept_uri]->broader)) {
        $concepts[$concept_uri]->broader = array();
      }
      if (isset($data->broaderLabel)) {
        $concepts[$concept_uri]->broader[] = $data->broaderLabel->getValue();
      }
      if (!isset($concepts[$concept_uri]->multiple)) {
        $concepts[$concept_uri]->multiple = FALSE;
      }
      if (!empty($last_concept) && $concept_uri != $last_concept['uri'] && strtolower($data->prefLabel->getValue()) == strtolower($last_concept['prefLabel'])) {
        $concepts[$last_concept['uri']]->multiple = TRUE;
        $concepts[$concept_uri]->multiple = TRUE;
      }

      $last_concept = array(
        'uri' => $concept_uri,
        'prefLabel'  => $data->prefLabel->getValue(),
      );
    }

    uasort($concepts, array($this, 'sortAlpha'));
    return $concepts;
  }

  /**
   * Get a single concept.
   *
   * @param string $uri
   *   The URI of the concept
   * @param string $language
   *   Optional; the language of the objects
   *
   * @return object
   *   The concept-object or NULL if there was a problem
   */
  public function getConcept($uri, $language = '') {
    if (!valid_url($uri) || $this->glossaryStore === FALSE) {
      return NULL;
    }

    if (empty($language)) {
      $language = $this->defaultLanguage;
    }

    // Get labels and definitions.
    $query = "
      PREFIX skos:<http://www.w3.org/2004/02/skos/core#>

      SELECT *
      WHERE {
        <$uri> a skos:Concept.
        <$uri> skos:prefLabel ?prefLabel.
        OPTIONAL {
          <$uri> skos:altLabel ?altLabel FILTER(lang(?altLabel) = '$language').
        }
        OPTIONAL {
          <$uri> skos:definition ?definition FILTER(lang(?definition) = '$language').
        }
      }";

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      $this->error($e->getMessage());
      return NULL;
    }

    // Concept not found (e.g. false concept URI).
    if (count($rows) == 0) {
      return NULL;
    }

    $pref_labels = array();
    $definitions = array('internal' => array(), 'external' => array());
    $alt_labels = array();
    foreach ($rows as $data) {
      if ($data->prefLabel->getLang() == $language) {
        $pref_label = $data->prefLabel->getValue();
      }
      else {
        $key = $data->prefLabel->getLang();
        $pref_labels[$key] = $data->prefLabel->getValue();
      }
      if ($data->prefLabel->getLang() == $this->defaultLanguage) {
        $pref_label_default = $data->prefLabel->getValue();
      }
      if (isset($data->definition)) {
        $definition = $this->clearDefinition($data->definition->getValue());
        if (!empty($definition)) {
          $definitions['internal'][] = $definition;
        }
      }
      if (isset($data->altLabel)) {
        $alt_labels[] = $data->altLabel->getValue();
      }
    }
    $definitions['internal'] = array_unique($definitions['internal']);

    // No data for given language.
    if (empty($pref_label)) {
      return (object) array(
        'uri' => $uri,
        'prefLabelDefault' => $pref_label_default,
        'language' => $this->defaultLanguage,
      );
    }

    // Get broader, narrower and related.
    $query = "
      PREFIX skos:<http://www.w3.org/2004/02/skos/core#>

      SELECT *
      WHERE {
        <$uri> a skos:Concept.
        OPTIONAL {
          <$uri> skos:broader ?broaderUri.
          ?broaderUri skos:prefLabel ?broader FILTER(lang(?broader) = '$language').
        }
        OPTIONAL {
          <$uri> skos:narrower ?narrowerUri.
          ?narrowerUri skos:prefLabel ?narrower FILTER(lang(?narrower) = '$language').
        }
        OPTIONAL {
          <$uri> skos:related ?relatedUri.
          ?relatedUri skos:prefLabel ?related FILTER(lang(?related) = '$language').
        }
      }";

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      $this->error($e->getMessage());
      return NULL;
    }

    $broader    = array();
    $narrower    = array();
    $related    = array();
    foreach ($rows as $data) {
      if (isset($data->broaderUri)) {
        $broader[$data->broaderUri->getUri()] = array(
          'uri' => $data->broaderUri->getUri(),
          'prefLabel' => $data->broader->getValue(),
          'url' => $this->createUrl($data->broaderUri->getUri(), $data->broader->getValue(), $language),
        );
      }
      if (isset($data->narrowerUri)) {
        $narrower[$data->narrowerUri->getUri()] = array(
          'uri' => $data->narrowerUri->getUri(),
          'prefLabel' => $data->narrower->getValue(),
          'url' => $this->createUrl($data->narrowerUri->getUri(), $data->narrower->getValue(), $language),
        );
      }
      if (isset($data->relatedUri)) {
        $related[$data->relatedUri->getUri()] = array(
          'uri' => $data->relatedUri->getUri(),
          'prefLabel' => $data->related->getValue(),
          'url' => $this->createUrl($data->relatedUri->getUri(), $data->related->getValue(), $language),
        );
      }
    }

    $related_resources = module_invoke_all('smart_glossary_related_resource', $this->glossaryStore, $uri, $language);
    $definitions['external'] = is_null($related_resources) ? array() : $related_resources;

    $concept = array(
      'uri' => $uri,
      'prefLabel' => $pref_label,
      'prefLabels' => $pref_labels,
      'altLabels' => array_unique($alt_labels),
      'definitions' => $definitions,
      'related' => array_values($related),
      'broader' => array_values($broader),
      'narrower' => array_values($narrower),
    );

    return (object) $concept;
  }

  /**
   * Update the a-z character list to be able to grey out unused letters.
   *
   * @return array
   *   The adapted update settings
   */
  public function updateCharacterList() {
    $chars = range('a', 'z');
    $advanced_settings = $this->config->advanced_settings;
    $char_a_z = isset($advanced_settings['char_a_z']) ? $advanced_settings['char_a_z'] : array();

    $languages = _smart_glossary_get_available_languages($this->config);
    foreach ($languages as $language) {
      $char_a_z[$language] = array();
      foreach ($chars as $char) {
        $concepts = $this->getList($char, $language, 1);
        $char_a_z[$language][$char] = count($concepts);
      }
    }
    $advanced_settings['char_a_z'] = $char_a_z;

    return $advanced_settings;
  }

  /**
   * Get the number of concepts available for a specific language.
   *
   * @param string $language
   *   Optional; The language of the concepts to get
   *
   * @return int
   *   The number of concepts or NULL in case of an error
   */
  public function getNumberOfConcepts($language = '') {
    if ($this->glossaryStore === FALSE) {
      return NULL;
    }

    if (empty($language)) {
      $language = $this->defaultLanguage;
    }

    $query = "
      PREFIX skos:<http://www.w3.org/2004/02/skos/core#>

      SELECT DISTINCT ?concept
      WHERE {
        ?concept a skos:Concept.
        ?concept skos:prefLabel ?label FILTER(lang(?label) = '$language').
      }";

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      $this->error($e->getMessage());
      return NULL;
    }

    return count($rows);
  }

  /**
   * Get the number of concept schemes available for a specific language.
   *
   * @param string $language
   *   Optional; The language of the concept schemes to get
   *
   * @return int
   *   The number of concept schemes or NULL in case of an error
   */
  public function getNumberOfConceptSchemes($language = '') {
    if ($this->glossaryStore === FALSE) {
      return NULL;
    }

    if (empty($language)) {
      $language = $this->defaultLanguage;
    }

    $query = "
      PREFIX skos:<http://www.w3.org/2004/02/skos/core#>

      SELECT DISTINCT ?conceptscheme
      WHERE {
        ?conceptscheme a skos:ConceptScheme.
      }";

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      $this->error($e->getMessage());
      return NULL;
    }

    return count($rows);
  }

  /**
   * Get tha resource data from a given URI.
   *
   * @param string $uri
   *   The URI of the resource
   *
   * @return array
   *   The resource as an array or properties
   */
  public function getResource($uri) {
    if ($this->glossaryStore === FALSE) {
      return NULL;
    }

    $query = "
      SELECT ?property ?value
      WHERE {
        <$uri> ?property ?value.
      }";

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      $this->error($e->getMessage());
      return array();
    }

    $name_properties = array(
      'foaf:name',
      'skos:prefLabel',
      'dc:title',
      'geonames:name',
    );
    $result = array();
    foreach ($rows as $data) {
      $value = array();

      $property = array(
        'uri'  => $data->property->getUri(),
        'name' => $this->getPrefixes($data->property->getUri()));
      // Resource.
      if ($data->value instanceof EasyRdf_Resource) {
        $value['url'] = $data->value->getUri();
        $value['type'] = 'uri';
      }
      // Literal.
      else {
        $value['type'] = 'string';
        $value['value'] = $data->value->getValue();
        if (in_array($property['name'], $name_properties) && $data->value->getLang() == $this->defaultLanguage) {
          $result['name'] = $data->value->getValue();
        }
      }

      $result['resource'] = 'concept';
      $result['value'][] = array(
        'property' => $property,
        'value'  => $value,
      );
    }
    return $result;
  }

  /**
   * Get all the data for a specified URI for the Visual Mapper.
   *
   * @param string $root_uri
   *   The uri, which should be used as root.
   * @param string $lang
   *   The language of the selected concept.
   * @param string $output_format
   *   The output format, currently only "json" is allowed.
   */
  public function getVisualMapperData($root_uri = NULL, $lang = 'en', $output_format = 'json') {
    global $language;

    // Create the root object
    $concept = $this->createRootUriObject($root_uri, $lang);

    switch ($concept->type) {
      case 'project':
        // Get all conceptSchemes.
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?n ?nLabel ?nn
            WHERE {
                ?n rdf:type skos:ConceptScheme .
                ?n dc:title ?nLabel . FILTER(lang(?nLabel) = '$lang') .

                OPTIONAL {
                    ?n skos:hasTopConcept ?nn .
                }
            }";
        if ($children = $this->getRelationData($concept, $query, 'n')) {
          $concept->relations->children = $children;
        }
        break;

      case 'conceptScheme':
        // Get all topConcepts.
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?n ?nLabel ?nb ?nn ?nr
            WHERE {
                <$root_uri> skos:hasTopConcept ?n .
                ?n skos:prefLabel ?nLabel . FILTER(lang(?nLabel) = '$lang') .
                ?nb skos:hasTopConcept ?n .
                OPTIONAL { ?n skos:narrower ?nn . }
                OPTIONAL { ?n skos:related ?nr . }
            }";
        if ($children = $this->getRelationData($concept, $query, 'n')) {
          $concept->relations->children = $children;
        }
        break;

      case 'topConcept':
        // Get all conceptSchemes.
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?b ?bLabel ?bn
            WHERE {
                ?b skos:hasTopConcept <$root_uri> .
                ?b dc:title ?bLabel . FILTER(lang(?bLabel) = '$lang') .
                OPTIONAL { ?b skos:hasTopConcept ?bn . }
            }";
        if ($parents = $this->getRelationData($concept, $query, 'b')) {
          $concept->relations->parents = $parents;
        }
        // Get all narrower concepts
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?n ?nLabel ?nb ?nn ?nr
            WHERE {
                <$root_uri> skos:narrower ?n .
                ?n skos:prefLabel ?nLabel . FILTER(lang(?nLabel) = '$lang') .
                ?n skos:broader ?nb.
                OPTIONAL { ?n skos:narrower ?nn . }
                OPTIONAL { ?n skos:related ?nr . }
            }";
        if ($children = $this->getRelationData($concept, $query, 'n')) {
          $concept->relations->children = $children;
        }
        // Get all related concepts
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?r ?rLabel ?rb ?rn ?rr
            WHERE {
                <$root_uri> skos:related ?r .
                ?r skos:prefLabel ?rLabel . FILTER(lang(?rLabel) = '$lang') .
                { ?r skos:broader ?rb . } UNION { ?rb skos:hasTopConcept ?r }
                OPTIONAL { ?r skos:narrower ?rn . }
                OPTIONAL { ?r skos:related ?rr . }
            }";
        if ($related = $this->getRelationData($concept, $query, 'r')) {
          $concept->relations->related = $related;
        }
        break;

      case 'concept':
        // Get all broader concepts.
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?b ?bLabel ?bb ?bn ?br
            WHERE {
                <$root_uri> skos:broader ?b .
                ?b skos:prefLabel ?bLabel . FILTER(lang(?bLabel) = '$lang') .
                { ?b skos:broader ?bb . } UNION { ?bb skos:hasTopConcept ?b }
                ?b skos:narrower ?bn .
                OPTIONAL { ?b skos:related ?br . }
            }";
        if ($parents = $this->getRelationData($concept, $query, 'b')) {
          $concept->relations->parents = $parents;
        }
        // Get all narrower concepts.
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?n ?nLabel ?nb ?nn ?nr
            WHERE {
                <$root_uri> skos:narrower ?n.
                ?n skos:prefLabel ?nLabel . FILTER(lang(?nLabel) = '$lang') .
                OPTIONAL { ?n skos:narrower ?nn . }
                ?n skos:broader ?nb.
                OPTIONAL { ?n skos:related ?nr . }
            }";
        if ($children = $this->getRelationData($concept, $query, 'n')) {
          $concept->relations->children = $children;
        }
        // Get all related concepts.
        $query = "
            PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
            PREFIX dc:<http://purl.org/dc/terms/>
            PREFIX rdf:<http://www.w3.org/1999/02/22-rdf-syntax-ns#>

            SELECT DISTINCT ?r ?rLabel ?rb ?rn ?rr
            WHERE {
                <$root_uri> skos:related ?r.
                ?r skos:prefLabel ?rLabel . FILTER(lang(?rLabel) = '$lang') .
                { ?r skos:broader ?rb . } UNION { ?rb skos:hasTopConcept ?r }
                OPTIONAL { ?r skos:narrower ?rn . }
                OPTIONAL { ?r skos:related ?rr . }
            }";
        if ($related = $this->getRelationData($concept, $query, 'r')) {
          $concept->relations->related = $related;
        }
        break;
    }

    // Add the ID of the connection taxonomy-term, if a link to related content
    // has to be added.
    if (isset($concept->id) && isset($this->config->advanced_settings['semantic_connection']) && isset($this->config->advanced_settings['semantic_connection']['add_show_content_link']) && $this->config->advanced_settings['semantic_connection']['add_show_content_link']) {
      $tid = db_select('field_data_field_uri', 'u')
        ->fields('u', array('entity_id'))
        ->condition('field_uri_value', $concept->id)
        ->execute()
        ->fetchField();

      if ($tid) {
        $concept->tid = $tid;
      }
    }

    if (!is_null($root_uri) && $concept->type != 'conceptScheme') {
      $pp_server_info = _semantic_connector_get_sparql_connection_details($this->config->connection_id);
      if ($pp_server_info !== FALSE) {
        $concept->content_button = semantic_connector_theme_concepts(array(array(
          'html' => ((isset($this->config->language_mapping[$language->language]) && isset($this->config->language_mapping[$language->language]['wording']['showContentButton'])) ? $this->config->language_mapping[$language->language]['wording']['showContentButton'] : 'Show content'),
          'uri' => $root_uri,
        )), $pp_server_info['pp_connection_id'], $pp_server_info['project_id'], '', array('smart_glossary_detail_page'));
      }
    }

    // Print out the data
    switch ($output_format) {
      case 'json':
        header("Content-Type: application/json");
        echo json_encode($concept);
        break;
    }
    exit;
  }

  /**
   * Creates a data object for the root concept with id, name, type and size.
   * Important for the Visual Mapper data.
   *
   * @param string $uri
   *  The concept URI.
   * @param string $lang
   *  The language for the concept data.
   *
   * @return object
   *  The root concept object.
   */
  protected function createRootUriObject($uri, $lang) {
    if ($this->glossaryStore === FALSE) {
      return NULL;
    }

    $object = new stdClass();
    $object->id = $uri;
    $object->size = 1;
    $object->relations = new stdClass();

    if (is_null($uri)) {
      $object->name = '';
      $object->type = 'project';
      return $object;
    }

    // Get the label and the type of the given concept
    $query = "
        PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
        PREFIX dc:<http://purl.org/dc/terms/>

        SELECT ?label ?topConcept ?concept
        WHERE {
          { <$uri> skos:prefLabel ?label . FILTER(lang(?label) = '$lang') . }
            UNION { <$uri> dc:title ?label . FILTER(lang(?label) = '$lang') . }

          OPTIONAL {
                <$uri> skos:broader ?concept .
            }
            OPTIONAL {
                ?topConcept skos:hasTopConcept <$uri> .
            }
        }";
    $rows = $this->glossaryStore->query($query);
    $object->name = $rows[0]->label->getValue();
    $object->type = isset($rows[0]->concept) ? 'concept' : (isset($rows[0]->topConcept) ? 'topConcept' : 'conceptScheme');

    return $object;
  }

  /**
   * Get the data from the SPARQL endpoint for a given relation type (broader,
   * narrower or related).
   * Important for the Visual Mapper data.
   *
   * @param object $concept
   *    The root concept object..
   * @param string $query
   *    The query to get the data from SPARQL endpoint.
   * @param string $type
   *    The relation type:
   *      b => broader (parents),
   *      n => narrower (children),
   *      r => related (related)
   *
   * @return array
   *
   */
  protected function getRelationData(&$concept, $query, $type) {
    if ($this->glossaryStore === FALSE) {
      return array();
    }

    try {
      $rows = $this->glossaryStore->query($query);
    }
    catch (Exception $e) {
      drupal_set_message(t('An error occurred calling the query %query (%error).', array(
        '%query' => $query,
        '%error' => print_r($e->getMessage(), TRUE)
      )), 'error');
      exit();
    }

    $map = array('b' => 'parents', 'n' => 'children', 'r' => 'related');
    if (!isset($map[$type])) {
      return NULL;
    }

    $relations = array();
    foreach ($rows as $row) {
      if (isset($row->{$type})) {
        $uri = $row->{$type}->getUri();
        if (!isset($relations[$uri])) {
          $concept->size++;
          $relations[$uri] = new stdClass();
          $relations[$uri]->id = $uri;
          $relations[$uri]->name = $row->{$type . 'Label'}->getValue();
          $relations[$uri]->size = 1;
        }
        if (isset($row->{$type . 'b'})) {
          $broader_uri = $row->{$type . 'b'}->getUri();
          if (!isset($relations[$uri]->relations->parents[$broader_uri])) {
            $concept->size++;
            $relations[$uri]->size++;
            $relations[$uri]->relations->parents[$broader_uri] = new stdClass();
            $relations[$uri]->relations->parents[$broader_uri]->id = $broader_uri;
            $relations[$uri]->relations->parents[$broader_uri]->size = 1;
          }
        }
        if (isset($row->{$type . 'n'})) {
          $narrower_uri = $row->{$type . 'n'}->getUri();
          if (!isset($relations[$uri]->relations->children[$narrower_uri])) {
            $concept->size++;
            $relations[$uri]->size++;
            $relations[$uri]->relations->children[$narrower_uri] = new stdClass();
            $relations[$uri]->relations->children[$narrower_uri]->id = $narrower_uri;
            $relations[$uri]->relations->children[$narrower_uri]->size = 1;
          }
        }
        if (isset($row->{$type . 'r'})) {
          $related_uri = $row->{$type . 'r'}->getUri();
          if (!isset($relations[$uri]->relations->related[$related_uri])) {
            $concept->size++;
            $relations[$uri]->size++;
            $relations[$uri]->relations->related[$related_uri] = new stdClass();
            $relations[$uri]->relations->related[$related_uri]->id = $related_uri;
            $relations[$uri]->relations->related[$related_uri]->size = 1;
          }
        }
      }
    }

    if (empty($relations)) {
      return NULL;
    }

    foreach ($relations as &$relation) {
      if (isset($relation->relations->parents)) {
        $relation->relations->parents = array_values($relation->relations->parents);
      }
      if (isset($relation->relations->children)) {
        $relation->relations->children = array_values($relation->relations->children);
      }
      if (isset($relation->relations->related)) {
        $relation->relations->related = array_values($relation->relations->related);
      }
    }

    usort($relations, array($this, 'sortRelationsBySize'));
    $relations = array_values($relations);

    return $relations;
  }

  /**
   * Create a nice URL for a concept.
   *
   * @param string $uri
   *   The URI of the concept
   * @param string $label
   *   The label(name) of the concept
   * @param string $glossary_language
   *   The language of the concept
   *
   * @return string
   *   The URL
   */
  protected function createUrl($uri, $label, $glossary_language) {
    global $base_url, $language;

    $site_language = $language->language;
    $default_language = language_default('language');
    $language_mapping = $this->config->language_mapping;

    // Check if the glossary language is ok
    $mapping_exists = isset($language_mapping[$site_language]) && !empty($language_mapping[$site_language]['glossary_languages']);
    $glossary_languages = $mapping_exists ? $language_mapping[$site_language]['glossary_languages'] : array();
    if (!in_array($glossary_language, $glossary_languages)) {
      if (isset($language_mapping[$default_language]) && !empty($language_mapping[$default_language]['glossary_languages'])) {
        $glossary_language = $language_mapping[$default_language]['glossary_languages'][0];
      } else {
        $glossary_language = 'en';
      }
    }

    $url = $base_url . '/';
    if (!empty($language->prefix)) {
      $url .= $language->prefix . '/';
    }
    $url .= $this->config->base_path . '/' . $glossary_language . '/' . $label . '?uri=' . $uri;

    return $url;
  }

  /**
   * Replace special characters in names.
   *
   * @param string $name
   *   The name to clean
   *
   * @return string
   *   The cleaned name
   */
  protected function createName($name) {
    return trim($name);
    /*
    $search[0] = '/&([A-Za-z]{1,2})(tilde|grave|acute|circ|cedil|lig);/';
    $search[1] = '/&([A-Za-z]{1,2})(uml);/';
    $replace[0] = '$1';
    $replace[1] = '$1e';
    return preg_replace($search, $replace, htmlentities(trim($name)));
    */
  }

  /**
   * Rebuild a label.
   *
   * @param string $label
   *   The label
   *
   * @return string
   *   The rebuilt label
   */
  protected function rebuiltLabel($label) {
    return preg_replace('/ +/', ' ', trim($label));
  }

  /**
   * Converts a string to a slug, for use in URLs or CSS classes.
   *
   * This function properly replaces letters with accents with their
   * non-accented counterparts.
   *
   * @param string $string
   *   The string to convert.
   *
   * @return string
   *   The slug.
   */
  protected function stringToSlug($string) {
    $search[0] = '/&([A-Za-z]{1,2})(tilde|grave|acute|circ|cedil|lig);/';
    $search[1] = '/&([A-Za-z]{1,2})(uml);/';
    $replace[0] = '$1';
    $replace[1] = '$1e';
    $string = html_entity_decode(strtolower(preg_replace($search, $replace, htmlentities(trim($string)))));
    return preg_replace(
      array('/[^a-z0-9-]/', '/-+/', '/-$/'),
      array('-', '-', ''),
      $string
    );
  }

  /**
   * Trim a definition.
   *
   * @param string $definition
   *   The definition to trim
   *
   * @return string
   *   The trimmed definition
   */
  protected function clearDefinition($definition) {
    $definition = trim($definition);
    if (is_null($definition) || empty($definition)) {
      return '';
    }
    return $definition;
  }

  /**
   * Sort two concepts.
   *
   * @param array/string $a
   *   The first concept
   * @param array/string $b
   *   The second concept
   *
   * @return int
   *   The sort-value (positive or negative integer or 0)
   */
  protected function sortAlpha($a, $b) {
    if (is_array($a)) {
      return strcasecmp($a['label'], $b['label']);
    }
    return strcasecmp($a->prefLabel, $b->prefLabel);
  }

  /**
   * Callback function to sort the concepts by size.
   *
   * @param object $a
   *    First concept to compare.
   * @param object $b
   *    Second concept to compare.
   *
   * @return boolean
   */
  protected function sortRelationsBySize($a, $b) {
    return $a->size >= $b->size ? ($a->size == $b->size ? 0 : -1) : 1;
  }

  /**
   * Replace URLs with prefixes in a property id.
   *
   * @param string $property
   *   The id of the property
   *
   * @return string
   *   The property-id with prefixes
   */
  protected function getPrefixes($property) {
    $search = array(
      'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
      'http://www.w3.org/2000/01/rdf-schema#',
      'http://purl.org/dc/elements/1.1/',
      'http://purl.org/dc/terms/',
      'http://dbpedia.org/property/',
      'http://xmlns.com/foaf/0.1/',
      'http://www.geonames.org/ontology#',
      'http://www.w3.org/2004/02/skos/core#',
      'http://www.w3.org/2002/07/owl#',
    );
    $replace = array(
      'rdf:',
      'rdfs:',
      'dc:',
      'dcterms:',
      'dbpedia:',
      'foaf:',
      'geonames:',
      'skos:',
      'owl:');
    return str_replace($search, $replace, $property);
  }

  /**
   * Log an error of "Smart Glossary" via watchdog.
   *
   * @param array $errors
   *   An array of errors
   * @param string $title
   *   The title of the error to be visible in the watchdog-log
   * @param int $severity
   *   The id of the watchdog-severity
   */
  protected function error($errors, $title = 'Glossary store', $severity = WATCHDOG_ERROR) {
    watchdog('smart_glossary', '%title: <pre>%errors</pre>', array('%title' => $title, '%errors' => $errors), $severity);
  }
}
