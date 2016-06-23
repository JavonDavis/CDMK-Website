<?php

/**
 * @file
 * Administration functionality for Smart Glossary
 */

/**
 * List all saved Smart Glossary configurations.
 *
 * @return string
 *   The rendered HTML of the list of configurations.
 */
function smart_glossary_admin_list() {
  $output = array();

  $output['smart_glossary_title'] = array(
    '#type' => 'markup',
    '#markup' => '<h3 class="semantic-connector-table-title">' . t('Smart Glossary configurations') . '</h3>',
  );

  $rows = array();
  $configs = smart_glossary_config_load();
  foreach ($configs as $config) {
    $actions = array(
      l(t('Edit'), 'admin/config/semantic-drupal/smart-glossary/' . $config->sg_id . '/edit'),
      l(t('Clone'), 'admin/config/semantic-drupal/smart-glossary/' . $config->sg_id . '/clone'),
      l(t('Delete'), 'admin/config/semantic-drupal/smart-glossary/' . $config->sg_id . '/delete'),
    );
    $title = '<div class="semantic-connector-led" data-server-id="' . $config->connection->getId() . '" data-server-type="sparql-endpoint" title="' . t('Checking service') . '"></div>';
    $title .= $config->title;
    $rows[] = array(
      $title,
      l($config->connection->getTitle(), $config->connection->getUrl(), array('attributes' => array('target' => '_blank'))),
      l('/' . $config->base_path, $config->base_path),
      implode(' | ', $actions),
    );
  }
  $output['smart_glossary'] = array(
    '#theme' => 'table',
    '#header' => array(
      t('Name'),
      t('SPARQL Endpoint'),
      t('Available on page'),
      t('Operations'),
    ),
    '#rows' => $rows,
    '#attributes' => array(
      'id' => 'smart-glossary-configurations-table',
      'class' => array('semantic-connector-tablesorter'),
    ),
    '#attached' => array(
      'js' => array(drupal_get_path('module', 'smart_glossary') . '/js/smart_glossary.admin.js'),
      'css' => array(drupal_get_path('module', 'smart_glossary') . '/css/smart_glossary.css'),
    ),
  );

  // Add the tablesorter library if available.
  $tablesorter_path = 'sites/all/libraries/tablesorter';
  if (module_exists('libraries')) {
    $tablesorter_path = libraries_get_path('tablesorter');
  }
  if (file_exists($tablesorter_path . '/jquery.tablesorter.min.js')) {
    $output['#attached']['js'][] = libraries_get_path('tablesorter') . '/jquery.tablesorter.min.js';
  }

  return $output;
}

/**
 * The Administration Settings form.
 *
 * TODO: Make position of definition, broader, narrower, Visual Mapper,
 *        autocomplete field, ... customisable.
 */
function smart_glossary_admin_form($form, &$form_state, $config = NULL) {
  global $base_url;
  // Error occurred while loading the configuration.
  if (is_null($config) || empty($config)) {
    $config = smart_glossary_default_config_load();
  }

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Name of the Smart Glossary configuration'),
    '#size' => 35,
    '#maxlength' => 60,
    '#default_value' => $config->title,
    '#required' => TRUE,
  );

  $form['base_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Base Path'),
    '#description' => 'The URL path where the glossary can be found. The default path is "glossary".',
    '#default_value' => $config->base_path,
    '#required' => TRUE,
  );

  // Define the container for the vertical tabs.
  $form['settings'] = array(
    '#type' => 'vertical_tabs',
  );

  // Tab: Connection settings.
  $form['connection_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Connection settings'),
    '#group' => 'settings',
  );

  $form['connection_settings']['connection_id'] = array(
    '#type' => 'hidden',
    '#value' => $config->connection->getId(),
  );

  $form['connection_settings']['sparql_endpoint'] = array(
    '#type' => 'fieldset',
    '#title' => t('SPARQL Endpoint'),
    '#description' => t('URL and credentials for the SPARQL-Endpoint'),
  );

  $sparql_endpoints = SemanticConnector::getConnectionsByType('sparql_endpoint');
  $servers = SemanticConnector::getConnectionsByType('pp_server');

  if (!empty($sparql_endpoints)) {
    $connection_options = array();
    $already_added_urls = array();

    // Add all SPARQL-endpoints of a configured PoolParty server first.
    foreach ($servers as $server) {
      $server_config = $server->getConfig();
      if (isset($server_config['projects'])) {
        $server_project_uris = array();
        foreach ($server_config['projects'] as $project) {
          if (property_exists($project, 'sparql_endpoint_url')) {
            $server_project_uris[] = $project->sparql_endpoint_url;
          }
        }
        if (!empty($server_project_uris)) {
          $already_added_urls = array_merge($already_added_urls, $server_project_uris);
          foreach ($sparql_endpoints as $sparql_endpoint) {
            if (in_array($sparql_endpoint->getUrl(), $server_project_uris)) {
              $credentials = $sparql_endpoint->getCredentials();
              $endpoint_key = array(
                $sparql_endpoint->getTitle(),
                $sparql_endpoint->getUrl(),
                $credentials['username'],
                $credentials['password'],
              );
              $connection_options[$server->getTitle()][implode('|', $endpoint_key)] = $sparql_endpoint->getTitle();
            }
          }
        }
      }
    }

    // Add all customized SPARQL-endpoints afterwards.
    foreach ($sparql_endpoints as $sparql_endpoint) {
      if (!in_array($sparql_endpoint->getUrl(), $already_added_urls)) {
        $credentials = $sparql_endpoint->getCredentials();
        $endpoint_key = array(
          $sparql_endpoint->getTitle(),
          $sparql_endpoint->getUrl(),
          $credentials['username'],
          $credentials['password'],
        );
        $connection_options[t('Custom SPARQL-Endpoints')][implode('|', $endpoint_key)] = $sparql_endpoint->getTitle();
      }
    }

    $form['connection_settings']['sparql_endpoint']['load_connection'] = array(
      '#type' => 'select',
      '#title' => t('Load an available SPARQL endpoint'),
      '#options' => $connection_options,
      '#empty_option' => '',
      '#default_value' => '',
    );

    $form['connection_settings']['sparql_endpoint']['load_connection_button'] = array(
      '#type' => 'button',
      '#value' => t('Load'),
      '#attributes' => array(
        'onclick' => '
          var connection_value = (jQuery(\'#edit-load-connection\').val());
          if (connection_value.length > 0) {
            var connection_details = connection_value.split(\'|\');
            jQuery(\'#edit-endpoint-title\').val(connection_details[0]);
            jQuery(\'#edit-url\').val(connection_details[1]);
            jQuery(\'#edit-username\').val(connection_details[2]);
            jQuery(\'#edit-password\').val(connection_details[3]);
          }
          return false;',
      ),
    );
  }

  $form['connection_settings']['sparql_endpoint']['endpoint_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('A short title for the SPARQL Endpoint'),
    '#required' => TRUE,
    '#default_value' => $config->connection->getTitle(),
  );
  $form['connection_settings']['sparql_endpoint']['url'] = array(
    '#type' => 'textfield',
    '#title' => t('Url'),
    '#required' => TRUE,
    '#default_value' => $config->connection->getUrl(),
  );

  $credentials = $config->connection->getCredentials();
  $form['connection_settings']['sparql_endpoint']['credentials'] = array(
    '#type' => 'fieldset',
    '#title' => t('Credentials'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['connection_settings']['sparql_endpoint']['credentials']['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#default_value' => $credentials['username'],
  );
  $form['connection_settings']['sparql_endpoint']['credentials']['password'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
    '#default_value' => $credentials['password'],
  );

  // Tab: Language and wording settings.
  $form['language_mapping'] = array(
    '#type' => 'fieldset',
    '#title' => t('Language and wording settings'),
    '#tree' => TRUE,
    '#group' => 'settings',
  );

  $language_mapping = isset($config->language_mapping) ? $config->language_mapping : array();
  $installed_languages = language_list();
  // array_unshift($installed_languages, (object) array('language' => '', 'name' => t('Language neutral')));

  foreach ($installed_languages as $installed_language) {
    $lang_code = $installed_language->language;
    $mapping = isset($language_mapping[$lang_code]) ? $language_mapping[$lang_code] : array();

    $form['language_mapping']['languages_' . $lang_code] = array(
      '#type' => 'fieldset',
      '#title' => t('Mapping for %language', array('%language' => $installed_language->name)),
      '#tree' => TRUE,
    );
    $form['language_mapping']['languages_' . $lang_code]['page_language'] = array(
      '#type' => 'value',
      '#value' => $lang_code,
    );
    $form['language_mapping']['languages_' . $lang_code]['glossary_languages'] = array(
      '#type' => 'textfield',
      '#title' => t('Glossary language codes'),
      '#default_value' => isset($mapping['glossary_languages']) ? implode(', ', $mapping['glossary_languages']) : '',
      '#description' => t('Enter language codes comma separated (e.g. en, de, fr).'),
    );

    // Wording settings for every selected language.
    $wording = isset($mapping['wording']) ? $mapping['wording'] : array();
    $form['language_mapping']['languages_' . $lang_code]['wording'] = array(
      '#type' => 'fieldset',
      '#title' => t('Wording'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $form['language_mapping']['languages_' . $lang_code]['wording']['glossaryRoot'] = array(
      '#type' => 'textfield',
      '#title' => t('Glossary Root'),
      '#default_value' => isset($wording['glossaryRoot']) ? $wording['glossaryRoot'] : 'Home',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['homeButton'] = array(
      '#type' => 'textfield',
      '#title' => t('Home Button'),
      '#default_value' => isset($wording['glossaryRoot']) ? $wording['glossaryRoot'] : 'Home',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['showDefinitionButton'] = array(
      '#type' => 'textfield',
      '#title' => t('Show definition button'),
      '#default_value' => isset($wording['showDefinitionButton']) ? $wording['showDefinitionButton'] : 'Show definition',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['noDefinition'] = array(
      '#type' => 'textfield',
      '#title' => t('No definition'),
      '#default_value' => isset($wording['noDefinition']) ? $wording['noDefinition'] : 'No definition',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['showContentButton'] = array(
      '#type' => 'textfield',
      '#title' => t('Show content button'),
      '#default_value' => isset($wording['showContentButton']) ? $wording['showContentButton'] : 'Show content',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['legendConceptScheme'] = array(
      '#type' => 'textfield',
      '#title' => t('Legend: Concept schemes'),
      '#default_value' => isset($wording['legendConceptScheme']) ? $wording['legendConceptScheme'] : 'Concept Scheme',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['legendParent'] = array(
      '#type' => 'textfield',
      '#title' => t('Legend: Broader'),
      '#default_value' => isset($wording['legendParent']) ? $wording['legendParent'] : 'Broader',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['legendChildren'] = array(
      '#type' => 'textfield',
      '#title' => t('Legend: Narrower'),
      '#default_value' => isset($wording['legendChildren']) ? $wording['legendChildren'] : 'Narrower',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['legendRelated'] = array(
      '#type' => 'textfield',
      '#title' => t('Legend: Related'),
      '#default_value' => isset($wording['legendRelated']) ? $wording['legendRelated'] : 'Related',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['currentConcept'] = array(
      '#type' => 'textfield',
      '#title' => t('Current concept'),
      '#default_value' => isset($wording['currentConcept']) ? $wording['currentConcept'] : 'Current concept',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['helpButton'] = array(
      '#type' => 'textfield',
      '#title' => t('Help button'),
      '#default_value' => isset($wording['helpButton']) ? $wording['helpButton'] : 'Help',
    );
    $form['language_mapping']['languages_' . $lang_code]['wording']['helpText'] = array(
      '#type' => 'text_format',
      '#title' => t('Help text'),
      '#default_value' => isset($wording['helpText']) ? $wording['helpText']['value'] : '
        <h2 id="helptext-about-the-concepts">About the concepts</h2>
        <p>The fundamental element of the SKOS vocabulary is the concept. Concepts are the units of thought &mdash;ideas, meanings, or (categories of) objects and events. We can add labels to our concept. There are two types of labels: Preferred Labels (<a class="external-link" href="http://www.w3.org/TR/skos-reference/#prefLabel" rel="nofollow">skos:prefLabel</a>) and Alternative Labels (<a class="external-link" href="http://www.w3.org/TR/skos-reference/#altLabel" rel="nofollow">skos:altLabel</a>). Note that a concept can only have one preferred label per language but it can have many alternative labels. Now we can start to add semantic relationships. SKOS offers: hierarchical, associative and mapping relationships. The hierarchical relationships are <a class="external-link" href="http://www.w3.org/TR/skos-reference/#broader" rel="nofollow">skos:broader</a> and <a class="external-link" href="http://www.w3.org/TR/skos-reference/#narrower" rel="nofollow">skos:narrower</a>. For example, the concept &ldquo;Computer&rdquo; is <span class="color-broader"><strong>broader</strong></span> than the concept &ldquo;Laptop.&rdquo; Likewise, the concept &ldquo;Laptop&rdquo; is <strong><span class="color-narrower">narrower</span></strong> than the concept &rdquo;Computer.&rdquo; The basic tree structure of the SKOS Glossary is build with this two relationships, forming roots and branches in several levels. SKOS has also one associative relationship, <a class="external-link" href="http://www.w3.org/TR/skos-reference/#related" rel="nofollow">skos:related</a>, which is used to assert a relationship between two concepts. For example, the concept &ldquo;Computer&rdquo; is <span class="color-related"><strong>related</strong></span> to the concept &ldquo;Software&rdquo;. Using this relationship type interconnections outside the hierarchical structure of the glossary can be realized.</p>
        <h2 id="helptext-functions-of-the-Browser">Functions of the Browser</h2>
        <p><strong>Glossary root</strong></p>
        <p>In order to provide an efficient access to the entry points of broader/narrower concept hierarchies, SKOS defines a <code><a href="http://www.w3.org/TR/skos-reference#hasTopConcept" class="external-link" rel="nofollow">skos:hasTopConcept</a></code>property. User can view all those Top concepts at once, to get an overview of all possible branches of the glossary, in pressing the top left button &quot;<strong>Glossary Root</strong>&quot;.</p>
        <p><strong>Definition</strong></p>
        <p>Next to these structured characterizations, concepts sometimes have to be further defined using human-readable (&quot;informal&quot;) documentation. The non-mandatory <code><a href="http://www.w3.org/TR/skos-reference#definition" class="external-link" rel="nofollow">skos:definition</a></code>supplies a complete explanation of the intended meaning of a concept. The visual mappers displays the definition of the current concept, as the right hand button &quot;<strong>Definition</strong>&quot; is clicked.</p>
        <p><strong>Help</strong></p>
        <p>The right hand button &quot;<strong>Help</strong>&quot; activates this help section.</p>',
      '#format' => isset($wording['helpText']) ? $wording['helpText']['format'] : 'full_html',
    );
  }

  // Tab: Visual Mapper settings
  // TODO: Set description if Visual Mapper is not installed.
  $form['visual_mapper_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Visual Mapper settings'),
    '#tree' => TRUE,
    '#group' => 'settings',
  );

  if (_smart_glossary_visual_mapper_exists()) {
    $visual_mapper_settings = $config->visual_mapper_settings;

    // Dimension selection for the elements of the Visual Mapper.
    $form['visual_mapper_settings']['enabled'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show Visual Mapper'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['enabled'])) ? $visual_mapper_settings['enabled'] : 1),
      '#parents' => array('visual_mapper_settings', 'enabled'),
    );
    // Dimension selection for the elements of the Visual Mapper.
    $form['visual_mapper_settings']['width'] = array(
      '#type' => 'textfield',
      '#title' => t('Width in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['width'])) ? $visual_mapper_settings['width'] : '800'),
      '#parents' => array('visual_mapper_settings', 'width'),
    );
    $form['visual_mapper_settings']['height'] = array(
      '#type' => 'textfield',
      '#title' => t('Height in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['height'])) ? $visual_mapper_settings['height'] : '800'),
      '#parents' => array('visual_mapper_settings', 'height'),
    );

    $form['visual_mapper_settings']['chartTypes'] = array(
      '#type' => 'value',
      '#value' => array('spider'),
    );

    /*
    $form['visual_mapper_settings']['chartTypes'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Selectable chart types'),
      '#options' => array(
        'spider' => 'Spider chart',
        'treemap' => 'Treemap',
      ),
      '#default_value' => ((!empty($visual_mapper_data) && isset($visual_mapper_data['chartTypes'])) ? $visual_mapper_data['chartTypes'] : array()),
      '#required' => TRUE,
      '#parents' => array('visual_mapper_settings', 'chartTypes'),
    );
    */

    // Color definitions for the Visual Mapper.
    $form['visual_mapper_settings']['colors'] = array(
      '#type' => 'fieldset',
      '#title' => t('Color Options'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $form['visual_mapper_settings']['colors']['backColor'] = array(
      '#type' => 'textfield',
      '#title' => t('Background-color of the Visual Mapper'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['backColor'])) ? $visual_mapper_settings['backColor'] : '#ffffff'),
      '#parents' => array('visual_mapper_settings', 'backColor'),
    );
    $form['visual_mapper_settings']['colors']['backColorItems'] = array(
      '#type' => 'textfield',
      '#title' => t('Background-color of items inside the Visual Mapper'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['backColorItems'])) ? $visual_mapper_settings['backColorItems'] : '#ffffff'),
      '#parents' => array('visual_mapper_settings', 'backColorItems'),
    );
    $form['visual_mapper_settings']['colors']['headerColor'] = array(
      '#type' => 'textfield',
      '#title' => t('Header-color of the Visual Mapper'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['headerColor'])) ? $visual_mapper_settings['headerColor'] : '#0094cc'),
      '#parents' => array('visual_mapper_settings', 'headerColor'),
    );
    $form['visual_mapper_settings']['colors']['inactiveColor'] = array(
      '#type' => 'textfield',
      '#title' => t('Inactive color'),
      '#description' => t('The color used for inactive elements like inactive pagination and smaller objects like the inner circle of the spider chart.'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['inactiveColor'])) ? $visual_mapper_settings['inactiveColor'] : '#dadada'),
      '#parents' => array('visual_mapper_settings', 'inactiveColor'),
    );

    // Brights colors.
    $form['visual_mapper_settings']['colors']['brightColors'] = array(
      '#type' => 'fieldset',
      '#title' => t('Bright Colors'),
    );

    $form['visual_mapper_settings']['colors']['brightColors']['parent'] = array(
      '#type' => 'textfield',
      '#title' => t('Parent'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['brightColors']['parent'])) ? $visual_mapper_settings['brightColors']['parent'] : '#99ebc4'),
      '#parents' => array(
        'visual_mapper_settings',
        'brightColors',
        'parent',
      ),
    );
    $form['visual_mapper_settings']['colors']['brightColors']['children'] = array(
      '#type' => 'textfield',
      '#title' => t('Children'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['brightColors']['children'])) ? $visual_mapper_settings['brightColors']['children'] : '#ffe3ae'),
      '#parents' => array(
        'visual_mapper_settings',
        'brightColors',
        'children',
      ),
    );
    $form['visual_mapper_settings']['colors']['brightColors']['related'] = array(
      '#type' => 'textfield',
      '#title' => t('Related'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['brightColors']['related'])) ? $visual_mapper_settings['brightColors']['related'] : '#ffb2ab'),
      '#parents' => array(
        'visual_mapper_settings',
        'brightColors',
        'related',
      ),
    );
    $form['visual_mapper_settings']['colors']['brightColors']['conceptScheme'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept Scheme'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['brightColors']['conceptScheme'])) ? $visual_mapper_settings['brightColors']['conceptScheme'] : '#99d4eb'),
      '#parents' => array(
        'visual_mapper_settings',
        'brightColors',
        'conceptScheme',
      ),
    );

    // Dark colors.
    $form['visual_mapper_settings']['colors']['darkColors'] = array(
      '#type' => 'fieldset',
      '#title' => t('Dark Colors'),
    );

    $form['visual_mapper_settings']['colors']['darkColors']['parent'] = array(
      '#type' => 'textfield',
      '#title' => t('Parent'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['darkColors']['parent'])) ? $visual_mapper_settings['darkColors']['parent'] : '#00cc6b'),
      '#parents' => array(
        'visual_mapper_settings',
        'darkColors',
        'parent',
      ),
    );
    $form['visual_mapper_settings']['colors']['darkColors']['children'] = array(
      '#type' => 'textfield',
      '#title' => t('Children'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['darkColors']['children'])) ? $visual_mapper_settings['darkColors']['children'] : '#ffb11a'),
      '#parents' => array(
        'visual_mapper_settings',
        'darkColors',
        'children',
      ),
    );
    $form['visual_mapper_settings']['colors']['darkColors']['related'] = array(
      '#type' => 'textfield',
      '#title' => t('Related'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['darkColors']['related'])) ? $visual_mapper_settings['darkColors']['related'] : '#d9210b'),
      '#parents' => array(
        'visual_mapper_settings',
        'darkColors',
        'related',
      ),
    );
    $form['visual_mapper_settings']['colors']['darkColors']['conceptScheme'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept Scheme'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['darkColors']['conceptScheme'])) ? $visual_mapper_settings['darkColors']['conceptScheme'] : '#0094cc'),
      '#parents' => array(
        'visual_mapper_settings',
        'darkColors',
        'conceptScheme',
      ),
    );

    // Spider chart settings.
    $form['visual_mapper_settings']['spiderChart'] = array(
      '#type' => 'fieldset',
      '#title' => t('Spider Chart Settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $form['visual_mapper_settings']['spiderChart']['rootInnerRadius'] = array(
      '#type' => 'textfield',
      '#title' => t('Root-Circle: Inner radius'),
      '#description' => t('The inner radius of the root-circle in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['rootInnerRadius'])) ? $visual_mapper_settings['spiderChart']['rootInnerRadius'] : '75'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'rootInnerRadius',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['rootOuterRadius'] = array(
      '#type' => 'textfield',
      '#title' => t('Root-circle: Outer radius'),
      '#description' => t('The outer radius of the root-circle in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['rootOuterRadius'])) ? $visual_mapper_settings['spiderChart']['rootOuterRadius'] : '360'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'rootOuterRadius',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['rootFontSize'] = array(
      '#type' => 'textfield',
      '#title' => t('Root-circle: Font-size'),
      '#description' => t('Font-size of the root-circle-text in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['rootFontSize'])) ? $visual_mapper_settings['spiderChart']['rootFontSize'] : '15'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'rootFontSize',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['rootCharactersPerLine'] = array(
      '#type' => 'textfield',
      '#title' => t('Root-circle: Characters per line'),
      '#description' => t('The maximum characters per line in the text of the root-circle'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['rootCharactersPerLine'])) ? $visual_mapper_settings['spiderChart']['rootCharactersPerLine'] : '18'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'rootCharactersPerLine',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['conceptMaxRadius'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept-circle: Maximum radius'),
      '#description' => t('The maximum radius of the concept circles in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['conceptMaxRadius'])) ? $visual_mapper_settings['spiderChart']['conceptMaxRadius'] : '30'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'conceptMaxRadius',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['conceptMinRadius'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept-circle: Minimum radius'),
      '#description' => t('The minimum radius of the concept circles in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['conceptMinRadius'])) ? $visual_mapper_settings['spiderChart']['conceptMinRadius'] : '15'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'conceptMinRadius',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['conceptFontSize'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept-circle: Font-size'),
      '#description' => t('The font-size of the concept-circle-text in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['conceptFontSize'])) ? $visual_mapper_settings['spiderChart']['conceptFontSize'] : '12'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'conceptFontSize',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['conceptCharactersPerLine'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept-circle: Characters per line'),
      '#description' => t('The maximum characters per line in the text of the concept-circles'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['conceptCharactersPerLine'])) ? $visual_mapper_settings['spiderChart']['conceptCharactersPerLine'] : '18'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'conceptCharactersPerLine',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['sectorMinArc'] = array(
      '#type' => 'textfield',
      '#title' => t('Minimum arc-sector'),
      '#description' => t('The minimum radius of the sectors in RAD'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['sectorMinArc'])) ? $visual_mapper_settings['spiderChart']['sectorMinArc'] : '0.5'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'sectorMinArc',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['paginationAngle'] = array(
      '#type' => 'textfield',
      '#title' => t('Page button radius'),
      '#description' => t('The preferred radius of one page of the pagination area in RAD'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['paginationAngle'])) ? $visual_mapper_settings['spiderChart']['paginationAngle'] : '0.5'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'paginationAngle',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['rectHeight'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept space height'),
      '#description' => t('The height of the space required for all elements of one concept circle in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['rectHeight'])) ? $visual_mapper_settings['spiderChart']['rectHeight'] : '90'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'rectHeight',
      ),
    );
    $form['visual_mapper_settings']['spiderChart']['rectWidth'] = array(
      '#type' => 'textfield',
      '#title' => t('Concept space width'),
      '#description' => t('The width of the space required for all elements of one concept circle in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['rectWidth'])) ? $visual_mapper_settings['spiderChart']['rectWidth'] : '90'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'rectWidth',
      ),
    );

    // Legend settings.
    $form['visual_mapper_settings']['legend'] = array(
      '#type' => 'fieldset',
      '#title' => t('Legend Settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $form['visual_mapper_settings']['legend']['legendSize'] = array(
      '#type' => 'textfield',
      '#title' => t('Legend size'),
      '#description' => t('The font-size and space between every line of the legend in "px"'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['legendSize'])) ? $visual_mapper_settings['spiderChart']['legendSize'] : '12'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'legendSize',
      ),
    );
    $form['visual_mapper_settings']['legend']['legendPositionX'] = array(
      '#type' => 'select',
      '#title' => t('Legend position X'),
      '#options' => array(
        'left' => t('Left'),
        'right' => t('Right'),
      ),
      '#description' => t('The horizontal position of the legend in reference to the chart itself.'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['legendPositionX'])) ? $visual_mapper_settings['spiderChart']['legendPositionX'] : 'left'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'legendPositionX',
      ),
    );
    $form['visual_mapper_settings']['legend']['legendPositionY'] = array(
      '#type' => 'select',
      '#title' => t('Legend position Y'),
      '#options' => array(
        'top' => t('Top'),
        'bottom' => t('Bottom'),
      ),
      '#description' => t('The vertical position of the legend in reference to the chart itself.'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['legendPositionY'])) ? $visual_mapper_settings['spiderChart']['legendPositionY'] : 'top'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'legendPositionY',
      ),
    );
    $form['visual_mapper_settings']['legend']['legendStyle'] = array(
      '#type' => 'select',
      '#title' => t('Legend style'),
      '#options' => array(
        'none' => t('No legend required'),
        'list' => t('As a list'),
        'circle' => t('As a labeled circle'),
      ),
      '#description' => t('The way the legend has to be presented to the users.'),
      '#default_value' => ((!empty($visual_mapper_settings) && isset($visual_mapper_settings['spiderChart']['legendStyle'])) ? $visual_mapper_settings['spiderChart']['legendStyle'] : 'top'),
      '#parents' => array(
        'visual_mapper_settings',
        'spiderChart',
        'legendStyle',
      ),
    );
  }
  else {
    // TODO: add a description (maybe with image + link to a glossary page
    // including the Visual Mapper)
    // - what is the Visual Mapper
    // - where to download it
    // - how to install it.
    $form['visual_mapper_settings']['info'] = array(
      '#markup' => '<div class="messages warning">' . t('To use the Visual Mapper the "%libraries"-module needs to be enabled and the Visual Mapper library needs to be installed at %installpath.', array('%libraries' => 'libraries', '%installpath' => 'sites/all/libraries/visual_mapper')) . '</div>',
    );
  }

  // Tab: Advanced settings.
  $advanced_settings = isset($config->advanced_settings) ? $config->advanced_settings : array();
  $form['advanced_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced settings'),
    '#tree' => TRUE,
    '#group' => 'settings',
  );

  $form['advanced_settings']['data_updates'] = array(
    '#type' => 'fieldset',
    '#title' => t('Data updates'),
  );
  $form['advanced_settings']['data_updates']['interval'] = array(
    '#type' => 'select',
    '#title' => t('Automatic character list update time period'),
    '#description' => t('Unused letters in the character list will be greyed out after the update'),
    // Time in seconds.
    '#options' => array(
      '3600' => t('Every hour'),
      '86400' => t('Every day'),
      '604800' => t('Every week'),
      '2592000' => t('Every month'),
      '0' => t('Manual regeneration only'),
    ),
    '#default_value' => isset($advanced_settings['interval']) ? $advanced_settings['interval'] : '86400',
  );
  $form['advanced_settings']['data_updates']['next_run'] = array(
    '#type' => 'value',
    '#value' => isset($advanced_settings['next_run']) ? $advanced_settings['next_run'] : 0,
  );
  $form['advanced_settings']['data_updates']['char_a_z'] = array(
    '#type' => 'value',
    '#value' => isset($advanced_settings['char_a_z']) ? $advanced_settings['char_a_z'] : array(),
  );
  $form['advanced_settings']['data_updates']['update_character_list'] = array(
    '#type' => 'submit',
    '#value' => t('Update the character list now'),
    '#submit' => array('smart_glossary_update_character_list_submit'),
  );

  $htaccess = array();
  if (isset($config->base_path)) {
    $htaccess[] = '
  #---------------------------------------------------------
  # Content Negotiation from Drupal Module "Smart Glossary"
  #---------------------------------------------------------';
    $htaccess[] = '
  # RDF/XML rewrite rule for .rdf extension
  RewriteCond %{QUERY_STRING} ^uri=(.*)$
  RewriteRule ^' . $config->base_path . '\/(.*)\.rdf$   ' . $config->connection->getUrl() . '?query=CONSTRUCT+{+\%3C%1\%3E+\%3Fp+\%3Fo.+}+WHERE+{+\%3C%1\%3E+\%3Fp+\%3Fo.+}&format=application/rdf\%2Bxml    [NE,L,P]';
    $htaccess[] = '
  # N3 rewrite rule for .n3 extension
  RewriteCond %{QUERY_STRING} ^uri=(.*)$
  RewriteRule ^' . $config->base_path . '\/(.*)\.n3$   ' . $config->connection->getUrl() . '?query=CONSTRUCT+{+\%3C%1\%3E+\%3Fp+\%3Fo.+}+WHERE+{+\%3C%1\%3E+\%3Fp+\%3Fo.+}&format=text/rdf\%2Bn3    [NE,L,P]';
    $htaccess[] = '
  # RDF/XML content negotiation for accept header = application/rdf+xml
  RewriteCond %{HTTP_ACCEPT}  application/rdf\+xml
  RewriteCond %{QUERY_STRING} ^uri=(.*)$
  RewriteRule ^(' . $config->base_path . ')\/(.*)$    $1/$2.rdf?uri=%1    [L,R=303]';
    $htaccess[] = '
  # N3 content negotiation for accept header = text/rdf+n3
  RewriteCond %{HTTP_ACCEPT}  text/n3 [OR]
  RewriteCond %{HTTP_ACCEPT}  text/rdf\+n3
  RewriteCond %{QUERY_STRING} ^uri=(.*)$
  RewriteRule ^(' . $config->base_path . ')\/(.*)$    $1/$2.n3?uri=%1   [L,R=303]';
    $htaccess[] = '
  #---------------------------------------------------------
  # End of Content Negotiation
  #---------------------------------------------------------';
  }
  $form['advanced_settings']['rdf'] = array(
    '#type' => 'fieldset',
    '#title' => t('RDF'),
  );
  $form['advanced_settings']['rdf']['add_rdf_link'] = array(
    '#type' => 'checkbox',
    '#title' => t('Add a link to RDF/XML'),
    '#description' => t('Adds a link to get the concept data in RDF/XML format.'),
    '#default_value' => (isset($config->advanced_settings['add_rdf_link']) ? $config->advanced_settings['add_rdf_link'] : FALSE),
  );
  $form['advanced_settings']['rdf']['add_endpoint_link'] = array(
    '#type' => 'checkbox',
    '#title' => t('Add a link to the SPARQL endpoint'),
    '#description' => t('Adds a link to the specified SPARQL endpoint.'),
    '#default_value' => (isset($config->advanced_settings['add_endpoint_link']) ? $config->advanced_settings['add_endpoint_link'] : FALSE),
  );
  $form['advanced_settings']['rdf']['text'] = array(
    '#type' => 'textarea',
    '#title' => t('Add content negotiation'),
    '#description' => t('Copy and paste it into the .htaccess file below "RewriteBase /"'),
    '#default_value' => implode("\n", $htaccess),
    '#rows' => 25,
    '#resizable' => FALSE,
    '#attributes' => array('readonly' => 'readonly'),
  );

  if (isset($config->sg_id)) {
    $form['sg_id'] = array(
      '#type' => 'hidden',
      '#value' => $config->sg_id,
    );
  }

  // Tab: Semantic module connection.
  $form['semantic_connection'] = array(
    '#type' => 'fieldset',
    '#title' => t('Semantic module connection'),
    '#group' => 'settings',
    '#tree' => TRUE,
  );

  if (module_exists('powertagging')) {
    $form['semantic_connection']['add_show_content_link'] = array(
      '#type' => 'checkbox',
      '#title' => t('Add a "Show Content"-link'),
      '#description' => t('Add a link to the list of content tagged with the current root concept in the visual mapper if a corresponding taxonomy term exists.'),
      '#default_value' => ((isset($config->advanced_settings['semantic_connection']) && isset($config->advanced_settings['semantic_connection']['add_show_content_link'])) ? $config->advanced_settings['semantic_connection']['add_show_content_link'] : FALSE),
    );
  }
  else {
    $form['semantic_connection']['add_show_content_link'] = array(
      '#type' => 'value',
      '#value' => FALSE,
      '#parents' => array('semantic_connection', 'add_show_content_link'),
    );
  }

  $form['semantic_connection']['show_in_destinations'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show as concept destination'),
    '#description' => t('Add this Smart Glossary to a list of destinations for a concept link if applicable.'),
    '#default_value' => ((isset($config->advanced_settings['semantic_connection']) && isset($config->advanced_settings['semantic_connection']['show_in_destinations'])) ? $config->advanced_settings['semantic_connection']['show_in_destinations'] : TRUE),
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  if (isset($config->sg_id)) {
    $form['delete'] = array(
      '#type' => 'button',
      '#value' => t('Delete'),
      '#attributes' => array('onclick' => 'window.location.href = "' . url('admin/config/semantic-drupal/smart-glossary/' . $config->sg_id . '/delete') . '"; return false;'),
    );
  }
  $form['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => (isset($_GET['destination']) ? $_GET['destination'] : 'admin/config/semantic-drupal/smart-glossary'),
  );

  drupal_add_js(drupal_get_path('module', 'smart_glossary') . '/js/smart_glossary.admin.js');
  drupal_add_js(array('smart_glossary' => array('baseUrl' => $base_url)), 'setting');

  return $form;
}

/**
 * Validation-handler for the "Administration Settings form".
 */
function smart_glossary_admin_form_validate($form, &$form_state) {
  if (!valid_url($form_state['values']['url'], TRUE)) {
    form_set_error('url', t('The field URL must be a valid URL.'));
  }
  if ($form_state['values']['base_path'] == '<none>') {
    form_set_error('base_path', t('Please select a valid base path.'));
  }

  // Check if base path is unique.
  $query = db_select('smart_glossary', 'sg')
    ->fields('sg', array('sg_id'))
    ->condition('base_path', $form_state['values']['base_path']);
  if (isset($form_state['values']['sg_id'])) {
    $query->condition('sg_id', $form_state['values']['sg_id'], '!=');
  }
  $count = $query->execute()
    ->rowCount();
  if ($count > 0) {
    form_set_error('base_path', t('The base path must be unique.'));
  }

  if (isset($form_state['values']['visual_mapper_settings'])) {
    // Fix the values from checkboxes-selection.
    $form_state['values']['visual_mapper_settings']['chartTypes'] = array_filter(
      array_unique(array_values($form_state['values']['visual_mapper_settings']['chartTypes'])),
      '_smart_glossary_checkbox_value_filter'
    );

    // Replace numeric strings with numbers.
    _smart_glossary_form_values_string_to_number($form_state['values']['visual_mapper_settings']);
  }
}

/**
 * Fix the values from checkboxes-selection by replacing all keys with value 0.
 */
function _smart_glossary_checkbox_value_filter($value) {
  return ($value !== 0);
}

/**
 * Cast numeric strings to double recursively in a given array.
 *
 * @param array $array
 *   The array to check for numeric strings.
 */
function _smart_glossary_form_values_string_to_number(&$array) {
  foreach ($array as &$value) {
    if (is_array($value)) {
      _smart_glossary_form_values_string_to_number($value);
    }
    elseif (is_numeric($value)) {
      $value = (double) $value;
    }
  }
}

/**
 * Submit-handler for the "Administration Settings form".
 *
 * TODO: Start the "update character list" function on the first time.
 */
function smart_glossary_admin_form_submit($form, &$form_state) {
  // Get the languages into one variable.
  $language_mapping = array();
  foreach ($form_state['values']['language_mapping'] as $mapping) {
    $mapping['glossary_languages'] = explode(',', preg_replace('/ +/', '', $mapping['glossary_languages']));
    if (!empty($mapping['glossary_languages'])) {
      $language_mapping[$mapping['page_language']] = array(
        'glossary_languages' => $mapping['glossary_languages'],
        'wording' => $mapping['wording'],
      );
    }
  }

  $advanced_settings = $form_state['values']['advanced_settings']['data_updates'];
  $advanced_settings['add_rdf_link'] = $form_state['values']['advanced_settings']['rdf']['add_rdf_link'];
  $advanced_settings['add_endpoint_link'] = $form_state['values']['advanced_settings']['rdf']['add_endpoint_link'];
  $advanced_settings['semantic_connection'] = $form_state['values']['semantic_connection'];

  // Always create a new connection, if URL and type are the same the old one
  // will be used anyway.
  $connection = SemanticConnector::createConnection('sparql_endpoint', $form_state['values']['url'], $form_state['values']['endpoint_title'], array(
    'username' => $form_state['values']['username'],
    'password' => $form_state['values']['password'],
  ));

  if (isset($form_state['values']['sg_id'])) {
    db_update('smart_glossary')
      ->fields(array(
        'connection_id' => $connection->getId(),
        'title' => $form_state['values']['title'],
        'base_path' => $form_state['values']['base_path'],
        'language_mapping' => serialize($language_mapping),
        'visual_mapper_settings' => serialize(isset($form_state['values']['visual_mapper_settings']) ? $form_state['values']['visual_mapper_settings'] : array()),
        'advanced_settings' => serialize($advanced_settings),
      ))
      ->condition('sg_id', $form_state['values']['sg_id'])
      ->execute();
  }
  else {
    db_insert('smart_glossary')
      ->fields(array(
        'connection_id' => $connection->getId(),
        'title' => $form_state['values']['title'],
        'base_path' => $form_state['values']['base_path'],
        'language_mapping' => serialize($language_mapping),
        'visual_mapper_settings' => serialize(isset($form_state['values']['visual_mapper_settings']) ? $form_state['values']['visual_mapper_settings'] : array()),
        'advanced_settings' => serialize($advanced_settings),
      ))
      ->execute();
  }

  drupal_set_message(t('Smart Glossary configuration %title has been saved.', array('%title' => $form_state['values']['title'])));
  cache_clear_all();
  $form_state['redirect'] = 'admin/config/semantic-drupal/smart-glossary';
}

/**
 * Add a SmartGlossary configuration for a fixed SPARQL endpoint connection.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param int $connection_id
 *   The ID of the SPARQL endpoint connection
 *
 * @return array
 *   The Drupal form array of the confirmation form.
 */
function smart_glossary_add_config_for_fixed_connection_form($form, &$form_state, $connection_id) {
  $form_state['connection_id'] = $connection_id;

  $connection = SemanticConnector::getConnection('sparql_endpoint', $connection_id);
  if (!is_null($connection)) {
    return confirm_form($form,
      t('Are you sure you want to create the SmartGlossary configuration?'),
      (isset($_GET['destination']) ? $_GET['destination'] : 'admin/config/semantic-drupal/smart_glossary'),
      'Selected SPARQL endpoint: <b>' . $connection->getTitle() . ' (' . $connection->getUrl() . ')</b><p><b>ATTENTION:</b> ' . t('Make sure to adapt the base path and do the configuration of the new SmartGlossary after its creation.') . '</p>',
      t('Create configuration'));
  }
  else {
    drupal_set_message(t('Connection-ID %connectionid is not valid.', array('%connectionid' => $connection_id)));
    drupal_goto('admin/config/semantic-drupal/smart-glossary');
  }
}

/**
 * Submit handler for smart_glossary_add_config_for_fixed_connection_form().
 */
function smart_glossary_add_config_for_fixed_connection_form_submit($form, &$form_state) {
  $connection = SemanticConnector::getConnection('sparql_endpoint', $form_state['connection_id']);
  $default_config = smart_glossary_default_config_load();

  if (!is_null($connection)) {
    $configuration_title = 'SmartGlossary configuration for ' . $connection->getTitle();
    $sgid = db_insert('smart_glossary')
      ->fields(array(
        'connection_id' => $connection->getId(),
        'title' => $configuration_title,
        'base_path' => '<none>',
        'language_mapping' => serialize($default_config->language_mapping),
        'visual_mapper_settings' => serialize($default_config->visual_mapper_settings),
        'advanced_settings' => serialize($default_config->advanced_settings),
      ))
      ->execute();

    drupal_set_message(t('Smart Glossary configuration "%title" has been created.', array('%title' => $configuration_title)));
    // Drupal Goto to forward a destination if one is available.
    if (isset($_GET['destination'])) {
      $destination = $_GET['destination'];
      unset($_GET['destination']);
      drupal_goto('admin/config/semantic-drupal/smart-glossary/' . $sgid, array('query' => array('destination' => $destination)));
    }
    $form_state['redirect'] = 'admin/config/semantic-drupal/smart-glossary/' . $sgid;
  }
  else {
    drupal_set_message(t('Connection-ID %connectionid is not valid.', array('%connectionid' => $form_state['connection_id'])));
    $form_state['redirect'] = 'admin/config/semantic-drupal/smart-glossary';
  }
}

/**
 * Additional submit-handler for the "Administration Settings form".
 */
function smart_glossary_update_character_list_submit($form, &$form_state) {
  $config = smart_glossary_config_load($form_state['values']['sg_id']);
  _smart_glossary_update_character_list($config);
}

/**
 * The confirmation-form for cloning a SmartGlossary config.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $config
 *   A SmartGlossary configuration.
 *
 * @return array
 *   The Drupal form array of the confirmation form.
 */
function smart_glossary_clone_form($form, &$form_state, $config) {
  $form_state['config'] = $config;
  return confirm_form($form,
    t('Are you sure you want to clone the SmartGlossary configuration "%title"?', array('%title' => $config->title)),
    'admin/config/semantic-drupal/smart-glossary',
    '<b>ATTENTION:</b> ' . t('Make sure to adapt the base path of the cloned SmartGlossary after its creation.'),
    t('Clone configuration'));
}

/**
 * The submit handler of the confirmation-form to clone a SmartGlossary config.
 */
function smart_glossary_clone_form_submit($form, &$form_state) {
  $config = $form_state['config'];

  $sgid = db_insert('smart_glossary')
    ->fields(array(
      'connection_id' => $config->connection_id,
      'title' => $config->title . ' (CLONE)',
      'base_path' => '<none>',
      'language_mapping' => serialize($config->language_mapping),
      'visual_mapper_settings' => serialize($config->visual_mapper_settings),
      'advanced_settings' => serialize($config->advanced_settings),
    ))
    ->execute();

  drupal_set_message(t('%title has been cloned.', array('%title' => $config->title)));
  $form_state['redirect'] = 'admin/config/semantic-drupal/smart-glossary/' . $sgid;
}

/**
 * The confirmation-form for deleting a Smart Glossary configuration set.
 */
function smart_glossary_delete_form($form, &$form_state, $config) {
  $form_state['config'] = $config;
  return confirm_form($form,
    t('Are you sure you want to delete the configuration %title?', array('%title' => $config->title)),
    'admin/config/semantic-drupal/smart-glossary',
    t('This action cannot be undone.'),
    t('Delete configuration'));
}

/**
 * Submit-handler for deleting a Smart Glossary configuration set.
 */
function smart_glossary_delete_form_submit($form, &$form_state) {
  $config = $form_state['config'];

  db_delete('smart_glossary')
    ->condition('sg_id', $config->sg_id)
    ->execute();

  drupal_set_message(t('Smart Glossary configuration %title has been deleted.', array('%title' => $config->title)));
  $form_state['redirect'] = 'admin/config/semantic-drupal/smart-glossary';
}
