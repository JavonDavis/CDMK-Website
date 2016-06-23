<?php

/**
 * @file
 * All hooks required to export and import Smart Glossary configurations with
 * the Features module.
 */

/**
 * Implementation of hook_features_export_options()
 */
function smart_glossary_config_features_export_options() {
  $options = array();
  $smart_glossary_configs = smart_glossary_config_load();
  foreach ($smart_glossary_configs as $smart_glossary_config) {
    $options[$smart_glossary_config->sg_id] = $smart_glossary_config->title;
  }
  return $options;
}

/**
 * Implementation of hook_features_export()
 */
function smart_glossary_config_features_export($data, &$export, $module_name = '') {
  $export['dependencies'][] = 'smart_glossary';

  foreach ((array) $data as $sg_id) {
    $export['features']['smart_glossary_config'][$sg_id] = $sg_id;
  }

  return array();
}

/**
 * Implementation of hook_features_export_render()
 */
function smart_glossary_config_features_export_render($module_name, $data) {
  $smart_glossary_conf_array = array();
  foreach ($data as $sg_id) {
    $smart_glossary_config = smart_glossary_config_load($sg_id);
    if (!empty($smart_glossary_config)) {
      $smart_glossary_config->connection_details = array(
        'title' => $smart_glossary_config->connection->getTitle(),
        'url' => $smart_glossary_config->connection->getUrl(),
        'credentials' => $smart_glossary_config->connection->getCredentials(),
      );
      unset($smart_glossary_config->connection);
      $smart_glossary_conf_array[$sg_id] = $smart_glossary_config;
    }
  }
  $code = '  $data = ' . features_var_export($smart_glossary_conf_array, '  ') . ';' . PHP_EOL;
  $code .= '  return $data;';

  return array('smart_glossary_config_features_settings' => $code);
}

/**
 * Implementation of hook_features_rebuild()
 */
function smart_glossary_config_features_rebuild($module) {
  smart_glossary_config_features_revert($module);
}

/**
 * Implementation of hook_features_revert()
 */
function smart_glossary_config_features_revert($module) {
  if ($data = features_get_default('smart_glossary_config', $module)) {
    foreach ($data as $sg_id => $smart_glossary_config) {
      // Check if the connection already exists, otherwise create it.
      $potential_connections = SemanticConnector::searchConnections(array(
        'type' => 'sparql_endpoint',
        'url' => $smart_glossary_config['connection_details']['url'],
      ));

      // Connection already exists, use the first one found.
      if (!empty($potential_connections)) {
        $connection = reset($potential_connections);
      }
      // Connection doesn't exist yet, create it.
      else {
        $connection = SemanticConnector::createConnection('sparql_endpoint', $smart_glossary_config['connection_details']['url'], $smart_glossary_config['connection_details']['title'], $smart_glossary_config['connection_details']['credentials']);
      }

      // Restore the Smart Glossary configurations.
      db_query("DELETE FROM {smart_glossary} WHERE sg_id = :sg_id", array(':sg_id' => $sg_id));
      db_insert('smart_glossary')
        ->fields(array(
          'sg_id' => $sg_id,
          'connection_id' => $connection->getId(),
          'title' => $smart_glossary_config['title'],
          'base_path' => $smart_glossary_config['base_path'],
          'language_mapping' => serialize($smart_glossary_config['language_mapping']),
          'visual_mapper_settings' => serialize($smart_glossary_config['visual_mapper_settings']),
          'advanced_settings' => serialize($smart_glossary_config['advanced_settings']),
        ))
        ->execute();
    }
  }
}
