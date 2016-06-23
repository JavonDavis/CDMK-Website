<?php

/**
 * @file
 * The PoolParty Thesaurus (PPT) API class. PoolParty >= 4.6 is supported.
 */

/**
 * Abstract Class SemanticConnectorPPTApi
 *
 * API Class for the PoolParty Thesaurus.
 */
abstract class SemanticConnectorPPTApi {

  protected $connection;

  /**
   * The constructor of the PoolParty Thesaurus class.
   *
   * @param string $endpoint
   *   URL of the endpoint of the PoolParty-server.
   * @param string $credentials
   *   Username and password if required (format: "username:password").
   */
  public function __construct($endpoint, $credentials = '') {
    $this->connection = new SemanticConnectorCurlConnection($endpoint, $credentials);
  }

  /**
   * Get the configured cURL-connection.
   *
   * @return SemanticConnectorCurlConnection
   *   The connection object.
   */
  public function getConnection() {
    return $this->connection;
  }

  /**
   * Get the version of the installed PoolParty web service.
   *
   * @return string
   *   The PoolParty version formatted like '4.1.6'
   */
  public function getVersion() {
    return '';
  }

  /**
   * Get a list of available projects of a PoolParty server.
   *
   * @return array
   *   A list of projects.
   */
  public function getProjects() {
    return array();
  }

  /**
   * Export data of a project as a file and store it on the server.
   *
   * @param string $project_id
   *   The ID of the PoolParty project to export and store.
   * @param string $format
   *   The returned RDF format.
   *   Possible values are: TriG, N3, Turtle, N-Triples, RDF/XML, TriX
   * @param string $export_modules
   *   A list of the export modules for the data that should be exported.
   *   Possible values are:
   *   - concepts - includes concept schemes, concepts, collections and notes
   *   - workflow - workflow status for all concepts
   *   - history - all history events
   *   - freeConcepts - all free concepts
   *   - void - the project VoiD graph
   *   - adms - the project ADMS graph
   *
   * @return string
   *   The URL of the stored file or an empty string if an error occurred.
   */
  public function storeProject($project_id, $format = 'RDF/XML', $export_modules = 'concepts') {
    return '';
  }

  /**
   * Get information about concepts.
   *
   * @param string $project_id
   *   The ID of the PoolParty project of the concepts.
   * @param array $concept_uris
   *   An array of concept URIs to get information for.
   * @param array $properties
   *   Array of additional concept properties that will be fetched (only
   *   properties uri and prefLabel are included by default). Possible values
   *   are:
   *   - skos:altLabel
   *   - skos:hiddenLabel
   *   - skos:definition
   *   - skos:broader
   *   - skos:narrower
   *   - skos:related
   *   - skos:ConceptScheme
   * @param string $language
   *   Only concepts with labels in this language will be displayed. If no
   *   language is given, the default language of the project will be used.
   *
   * @return array
   *   Array of concept objects within the respective PoolParty project with
   *   following properties:
   *   - uri --> URI of the concept
   *   - prefLabel --> Preferred label
   *   - altLabels --> Alternative labels
   *   - hiddenLabels --> Hidden labels
   *   - definitions --> Definitions
   *   - broaders --> Broader concepts
   *   - narrowers --> Narrower concepts
   *   - relateds --> Related concepts
   *   - conceptSchemes --> Concept schemes
   */
  public function getConcepts($project_id, array $concept_uris, array $properties = array(), $language = NULL) {
    return array();
  }

  /**
   * Get information about a concept.
   *
   * @param string $project_id
   *   The ID of the PoolParty project of the concepts.
   * @param string $concept_uri
   *   The concept URI, from which the data should be retrieved.
   * @param string array $properties
   *   Array of additional concept properties that will be fetched (only
   *   properties uri and prefLabel are included by default). Possible values
   *   are:
   *   - skos:altLabel
   *   - skos:hiddenLabel
   *   - skos:definition
   *   - skos:broader
   *   - skos:narrower
   *   - skos:related
   *   - skos:ConceptScheme
   * @param string $language
   *   Only concepts with labels in this language will be displayed. If no
   *   language is given, the default language of the project will be used.
   *
   * @return stdClass
   *   A concept object within the respective PoolParty project with
   *   following properties:
   *   - uri --> URI of the concept
   *   - prefLabel --> Preferred label
   *   - altLabels --> Alternative labels
   *   - hiddenLabels --> Hidden labels
   *   - definitions --> Definitions
   *   - broaders --> Broader concepts
   *   - narrowers --> Narrower concepts
   *   - relateds --> Related concepts
   *   - conceptSchemes --> Concept schemes
   */
  public function getConcept($project_id, $concept_uri, array $properties = array(), $language = '') {
    return new stdClass();
  }
}

/**
 * Class SemanticConnectorPPTApi_4_6
 *
 * API Class for the version 4.6
 */
class SemanticConnectorPPTApi_4_6 extends SemanticConnectorPPTApi {

  /**
   * Get the version of the installed PoolParty web service.
   *
   * @return string
   *   The PoolParty version formatted like '4.6'
   */
  public function getVersion() {
    $resource_path = '/PoolParty/api/version';
    return $this->connection->get($resource_path, array('headers' => array('Accept' => 'text/plain')));
  }

  /**
   * Get a list of available projects of a PoolParty server.
   *
   * @return array
   *   A list of projects.
   */
  public function getProjects() {
    $resource_path = '/PoolParty/api/projects';
    $result = $this->connection->get($resource_path);
    $projects = json_decode($result);

    if (is_array($projects)) {
      foreach ($projects as &$project) {
        if (property_exists($project, 'uriSupplement')) {
          $project->sparql_endpoint_url = $this->connection->getEndpoint() . '/PoolParty/sparql/' . $project->uriSupplement;
        }
      }
    }
    else {
      $projects = array();
    }

    return $projects;
  }

  /**
   * Export data of a project as a file and store it on the server.
   *
   * @param string $project_id
   *   The ID of the PoolParty project to export and store.
   * @param string $format
   *   The returned RDF format.
   *   Possible values are: TriG, N3, Turtle, N-Triples, RDF/XML, TriX
   * @param string $export_modules
   *   A list of the export modules for the data that should be exported.
   *   Possible values are:
   *   - concepts - includes concept schemes, concepts, collections and notes
   *   - workflow - workflow status for all concepts
   *   - history - all history events
   *   - freeConcepts - all free concepts
   *   - void - the project VoiD graph
   *   - adms - the project ADMS graph
   *
   * @return string
   *   The URL of the stored file or an empty string if an error occurred.
   */
  public function storeProject($project_id, $format = 'RDF/XML', $export_modules = 'concepts') {
    $resource_path = '/PoolParty/api/projects/' . $project_id . '/store';
    $get_parameters = array(
      'format' => $format,
      'exportModules' => $export_modules,
    );
    $file_path = $this->connection->get($resource_path, array(
      'query' => $get_parameters,
      'headers' => array('Accept' => 'text/plain'),
    ));

    return (filter_var($file_path, FILTER_VALIDATE_URL) !== FALSE) ? $file_path : '';
  }

  /**
   * Get information about concepts.
   *
   * @param string $project_id
   *   The ID of the PoolParty project of the concepts.
   * @param array $concept_uris
   *   An array of concept URIs to get information for.
   * @param array $properties
   *   Array of additional concept properties that will be fetched (only
   *   properties uri and prefLabel are included by default). Possible values
   *   are:
   *   - skos:altLabel
   *   - skos:hiddenLabel
   *   - skos:definition
   *   - skos:broader
   *   - skos:narrower
   *   - skos:related
   *   - skos:ConceptScheme
   * @param string $language
   *   Only concepts with labels in this language will be displayed. If no
   *   language is given, the default language of the project will be used.
   *
   * @return array
   *   Array of concept objects within the respective PoolParty project with
   *   following properties:
   *   - uri --> URI of the concept
   *   - prefLabel --> Preferred label
   *   - altLabels --> Alternative labels
   *   - hiddenLabels --> Hidden labels
   *   - definitions --> Definitions
   *   - broaders --> Broader concepts
   *   - narrowers --> Narrower concepts
   *   - relateds --> Related concepts
   *   - conceptSchemes --> Concept schemes
   */
  public function getConcepts($project_id, array $concept_uris, array $properties = array(), $language = '') {
    if (empty($concept_uris)) {
      return array();
    }

    $resource_path = '/PoolParty/api/thesaurus/' . $project_id . '/concepts';
    foreach ($concept_uris as $uri_count => $concept_uri) {
      if ($uri_count == 0) {
        $resource_path .= '?';
      }
      else {
        $resource_path .= '&';
      }
      $resource_path .= 'concepts=' . urlencode($concept_uri);
    }
    foreach ($properties as $property) {
      $resource_path .= '&properties=' . urlencode($property);
    }
    if (!is_null($language)) {
      $resource_path .= '&language=' . urlencode($language);
    }

    $result = $this->connection->get($resource_path);
    $concepts = json_decode($result);

    return $concepts;
  }

  /**
   * Get information about a concept.
   *
   * @param string $project_id
   *   The ID of the PoolParty project of the concepts.
   * @param string $concept_uri
   *   The concept URI, from which the data should be retrieved.
   * @param string array $properties
   *   Array of additional concept properties that will be fetched (only
   *   properties uri and prefLabel are included by default). Possible values
   *   are:
   *   - skos:altLabel
   *   - skos:hiddenLabel
   *   - skos:definition
   *   - skos:broader
   *   - skos:narrower
   *   - skos:related
   *   - skos:ConceptScheme
   * @param string $language
   *   Only concepts with labels in this language will be displayed. If no
   *   language is given, the default language of the project will be used.
   *
   * @return stdClass
   *   A concept object within the respective PoolParty project with
   *   following properties:
   *   - uri --> URI of the concept
   *   - prefLabel --> Preferred label
   *   - altLabels --> Alternative labels
   *   - hiddenLabels --> Hidden labels
   *   - definitions --> Definitions
   *   - broaders --> Broader concepts
   *   - narrowers --> Narrower concepts
   *   - relateds --> Related concepts
   *   - conceptSchemes --> Concept schemes
   */
  public function getConcept($project_id, $concept_uri, array $properties = array(), $language = '') {
    $resource_path = '/PoolParty/api/thesaurus/' . $project_id . '/concept';
    $get_parameters = array(
      'concept' => $concept_uri,
      'properties' => $properties,
      'language' => $language,
    );
    $result = $this->connection->get($resource_path, array(
      'query' => $get_parameters,
    ));
    $concept = json_decode($result);

    return $concept;
  }
}
