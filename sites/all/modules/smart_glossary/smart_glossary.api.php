<?php

/**
 * @file
 * API documentation for Smart Glossary.
 */

/**
 * Use your custom header theme in a Smart Glossary.
 *
 * By default theme "smart_glossary_header" is used, using this hook will result
 * in using your returned theme instead, which gets called with the same
 * parameters as the default theme gets called with. Have a look at the original
 * theme to see what variables have to be defined and can be used in your theme.
 * The header theme gets used on every single page of the Smart Glossary.
 *
 * @return string
 *   The name of theme to use as header theme.
 *
 * @see templates/smart_glossary_header.tpl.php
 * @see _smart_glossary_header_theme()
 */
function hook_smart_glossary_header_theme() {
  drupal_add_js(drupal_get_path('module', 'custom_module') . '/js/custom_module.js');
  drupal_add_css(drupal_get_path('module', 'custom_module') . '/css/custom_module.css');

  return 'custom_module_smart_glossary_header_theme';
}

/**
 * Use your custom start theme in a Smart Glossary.
 *
 * By default theme "smart_glossary_start" is used, using this hook will result
 * in using your returned theme instead, which gets called with the same
 * parameters as the default theme gets called with. Have a look at the original
 * theme to see what variables have to be defined and can be used in your theme.
 * The start theme gets only used for the first thing you see once you enter a
 * Smart Glossary page.
 *
 * @return string
 *   The name of theme to use as start theme.
 *
 * @see templates/smart_glossary_start.tpl.php
 * @see _smart_glossary_start_theme()
 */
function hook_smart_glossary_start_theme() {
  return 'custom_module_smart_glossary_start_theme';
}

/**
 * Use your custom list theme in a Smart Glossary.
 *
 * By default theme "smart_glossary_list" is used, using this hook will result
 * in using your returned theme instead, which gets called with the same
 * parameters as the default theme gets called with. Have a look at the original
 * theme to see what variables have to be defined and can be used in your theme.
 * The list theme gets used for listings of concepts found for a selected
 * character in a Smart Glossary.
 *
 * @return string
 *   The name of theme to use as list theme.
 *
 * @see templates/smart_glossary_list.tpl.php
 * @see _smart_glossary_list_theme()
 */
function hook_smart_glossary_list_theme() {
  return 'custom_module_smart_glossary_list_theme';
}

/**
 * Use your custom detail view theme in a Smart Glossary.
 *
 * By default theme "smart_glossary_detail" is used, using this hook will result
 * in using your returned theme instead, which gets called with the same
 * parameters as the default theme gets called with. Have a look at the original
 * theme to see what variables have to be defined and can be used in your theme.
 * The detail view theme gets used for every detail page of a selected concept
 * in a Smart Glossary page.
 *
 * @return string
 *   The name of theme to use as detail view theme.
 *
 * @see templates/smart_glossary_detail.tpl.php
 * @see _smart_glossary_detail_view_theme()
 */
function hook_smart_glossary_detail_view_theme() {
  return 'custom_module_smart_glossary_detail_view_theme';
}

/**
 * Add information of a related resource of the selected concept.
 *
 * Add a block containing custom information about the currently selected
 * concept on a Smart Glossary detail page.
 *
 * @param object $sparql_client
 *   The EasyRdf SPARQL client for the configured SPARQL endpoint.
 * @param string $concept_uri
 *   The URI of the currently displayed concept.
 * @param string $selected_language
 *   The iso-code of the currently selected language of the Smart Glossary.
 *
 * @return string
 *   The themed HTML for the related resource.
 *
 * @see smart_glossary_dbpedia_smart_glossary_related_resource()
 */
function hook_smart_glossary_related_resource($sparql_client, $concept_uri, $selected_language) {
  // Get additional information from the configured SPARQL endpoint.
  $query = "
  PREFIX skos:<http://www.w3.org/2004/02/skos/core#>
  SELECT *
  WHERE {
    <$concept_uri> a skos:Concept.
    <$concept_uri> skos:exactMatch ?relatedResourceUri.
  }";

  try {
    $rows = $sparql_client->query($query);
  }
  catch (Exception $e) {
    watchdog('custom_module', 'Custom Module Query error: <pre>%errors</pre>', array('%errors' => $e->getMessage()), WATCHDOG_ERROR);
    return array();
  }

  if ($rows->numRows() == 0) {
    return array();
  }

  $uris = array();
  foreach ($rows as $data) {
    $uris[] = $data->relatedResourceUri->getUri();
  }

  // Do whatever you want with the additional information and / or add any
  // custom information for the selected concept in the selected language.
  return custom_module_theme_items($uris, $selected_language);
}

/**
 * Alter the SPARQL query for a list of concepts.
 *
 * @param string $query
 *   The query to alter.
 * @param array $alter_context
 *   An array of context variables possibly required to alter the query.
 *   Available array keys are:
 *   - string 'char' => The selected letter for concept labels to start with
 *   - string 'language' => The iso-code of the currently selected language
 *   - int 'limit' => The maximum number of items to receive. 0 means no limit.
 *
 * @see SmartGlossary::getList()
 */
function hook_smart_glossary_list_query_alter(&$query, $alter_context) {
}
