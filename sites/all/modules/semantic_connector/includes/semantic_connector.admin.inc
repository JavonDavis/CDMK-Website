<?php

/**
 * @file
 * Administration functionality for the PoolParty Semantic Connector.
 */

/**
 * Show an overview of all the available semantic Drupal modules.
 *
 * @return string
 *   The rendered HTML of the module overview.
 */
function semantic_connector_overview() {
  $output = array();

  // Create a list of all "Semantic Drupal" modules.
  $output['modules_title'] = array(
    '#type' => 'markup',
    '#markup' => '<h3 class="semantic-connector-table-title">' . t('Semantic Drupal modules') . '</h3>',
  );

  $installed_modules = module_list();
  $semantic_modules = array(
    'powertagging' => array(
      'title' => 'PowerTagging',
      'configuration_path' => 'admin/config/semantic-drupal/powertagging',
    ),
    'sonr_webmining' => array(
      'title' => 'sOnr webMining Content Aggregator',
      'configuration_path' => 'admin/config/semantic-drupal/sonr-webmining',
    ),
    'smart_glossary' => array(
      'title' => 'Smart Glossary',
      'configuration_path' => 'admin/config/semantic-drupal/smart-glossary',
    ),
    /*
    'skos_importer' => array(
      'title' => 'SKOS Importer',
      'configuration_path' => 'admin/config/semantic-drupal/skos-importer',
    ),
    */
  );

  $module_rows = array();
  $installed_semantic_modules = array();
  foreach ($semantic_modules as $module_key => $module_info) {
    $installed = in_array($module_key, $installed_modules);
    $module_rows[] = array(
      $module_info['title'],
      ($installed ? 'installed' : l(t('Download'), 'http://www.drupal.org/project/' . $module_key)),
      ($installed ? l(t('Configure'), $module_info['configuration_path']) : ''),
    );

    // Create a list of connections used by module and connection-id.
    if ($installed) {
      $installed_semantic_modules[] = $module_key;
    }
  }

  $output['modules'] = array(
    '#theme' => 'table',
    '#header' => array(t('Module'), t('Installed'), t('Configuration')),
    '#rows' => $module_rows,
    '#caption' => NULL,
    '#colgroups' => array(),
    '#sticky' => FALSE,
    '#empty' => '',
  );

  // Find out what connections are used by the installed semantic modules.
  $connections_used = semantic_connector_check_connection_usage($installed_semantic_modules);
  $pp_server_connections = SemanticConnector::getConnectionsByType('pp_server');

  // Build an array of existing connections using a SPARQL endpoint.
  $sparql_endpoint_connections = SemanticConnector::getConnectionsByType('sparql_endpoint');
  $sparql_endpoint_connections_assoc = array();
  foreach ($sparql_endpoint_connections as $sparql_endpoint_connection) {
    $sparql_endpoint_connections_assoc[$sparql_endpoint_connection->getUrl()] = $sparql_endpoint_connection;
  }
  $sparql_endpoint_connections = $sparql_endpoint_connections_assoc;
  unset($sparql_endpoint_connections_assoc);

  // List the PoolParty server connections.
  foreach ($pp_server_connections as $pp_server_connection) {
    $server_id = $pp_server_connection->getId();
    $server_config = $pp_server_connection->getConfig();
    $server_ppx_projects = $pp_server_connection->getApi('PPX')->getProjects();

    $server_title = '<h3 class="semantic-connector-table-title">';
    // Check the PoolParty server version if required.
    if (variable_get('semantic_connector_version_checking', TRUE)) {
      $api_version_info = $pp_server_connection->getVersionInfo('PPX');
      if (version_compare($api_version_info['installed_version'], $api_version_info['latest_version'], '<')) {
        $server_title .= '<div class="messages warning">' . t('The installed PoolParty server version is not up to date. You are currently running version %installedversion, upgrade to version %latestversion to enjoy the new features.', array('%installedversion' => $api_version_info['installed_version'], '%latestversion' => $api_version_info['latest_version'])) . '</div>';
      }
    }
    $server_title .= '<div class="semantic-connector-led" data-server-id="' . $server_id . '" data-server-type="pp-server" title="' . t('Checking service') . '"></div>';
    $server_title .= t('PoolParty Server "%pptitle"', array('%pptitle' => $pp_server_connection->getTitle())) . '</h3>';
    $output['server_title_' . $server_id] = array(
      '#type' => 'markup',
      '#markup' => $server_title,
    );
    $output['server_buttons_' . $server_id] =  array(
      '#type' => 'markup',
      '#markup' => _semantic_connector_theme_connection_buttons($pp_server_connection, !isset($connections_used[$server_id])),
    );

    $project_rows = array();
    if (isset($server_config['projects']) && !empty($server_config['projects'])) {
      foreach ($server_config['projects'] as $project) {
        $project_row = array($project->title);

        foreach ($installed_semantic_modules as $semantic_module_key) {
          if ($semantic_module_key == 'powertagging') {
            // PowerTagging cell content.
            $project_powertagging_content = '';
            $powertagging_project_uses = array();
            if (isset($connections_used[$server_id]) && isset($connections_used[$server_id]['powertagging'])) {
              foreach ($connections_used[$server_id]['powertagging'] as $powertagging_use) {
                // This sOnr Webmining configuration uses the sOnr server using
                // this project on the current PP server.
                if ($powertagging_use['project_id'] == $project->id) {
                  $powertagging_project_uses[] = '<li>' . l($powertagging_use['title'], $semantic_modules['powertagging']['configuration_path'] . '/' . $powertagging_use['id'], array('query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector'))) . '</li>';
                }
              }
            }
            if (!empty($powertagging_project_uses)) {
              $project_powertagging_content .= '<ul>' . implode('', $powertagging_project_uses) . '</ul><br />';
            }

            // Check if the project is valid for PPX communication (corpus was
            // already built).
            $project_is_valid = FALSE;
            foreach ($server_ppx_projects as $server_ppx_project) {
              if ($server_ppx_project->uuid == $project->id) {
                $project_is_valid = TRUE;
                break;
              }
            }

            // Valid PPX project.
            if ($project_is_valid) {
              $project_powertagging_content .= l(t('Add new PowerTagging configuration'), $semantic_modules['powertagging']['configuration_path'] . '/add/' . $server_id . '/' . $project->id, array('attributes' => array('class' => 'add-configuration'), 'query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector')));
            }
            // Project is not available for PowerTagging.
            else {
              $project_powertagging_content .= '<div class="semantic-connector-italic">' . t('not supported') . '</div>';
            }
            $project_row[] = $project_powertagging_content;
          }

          // sOnr cell content.
          elseif ($semantic_module_key == 'sonr_webmining') {
            $project_sonr_content = '';
            // A sOnr server is available for this project on the current PP server.
            if (isset($server_config['sonr_configuration']) && !empty($server_config['sonr_configuration']) && $server_config['sonr_configuration']['project'] == $project->id) {
              $sonr_webmining_project_uses = array();
              if (isset($connections_used[$server_id]) && isset($connections_used[$server_id]['sonr_webmining'])) {
                foreach ($connections_used[$server_id]['sonr_webmining'] as $sonr_webmining_use) {
                  // This sOnr Webmining configuration uses the sOnr server using
                  // this project on the current PP server.
                  if ($sonr_webmining_use['project_id'] == $project->id) {
                    $sonr_webmining_project_uses[] = '<li>' . l($sonr_webmining_use['title'], $semantic_modules['sonr_webmining']['configuration_path'] . '/' . $sonr_webmining_use['id'], array('query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector'))) . '</li>';
                  }
                }
              }
              if (!empty($sonr_webmining_project_uses)) {
                $project_sonr_content .= '<ul>' . implode('', $sonr_webmining_project_uses) . '</ul><br />';
              }
              $project_sonr_content .= l(t('Add new sOnr webMining configuration'), $semantic_modules['sonr_webmining']['configuration_path'] . '/add/' . $server_id . '/' . $project->id, array('attributes' => array('class' => 'add-configuration'), 'query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector')));
            }
            // There is no sOnr server available for this project on the PP server.
            else {
              $project_sonr_content .= '<div class="semantic-connector-italic">' . t('not supported') . '</div>';
            }
            $project_row[] = $project_sonr_content;
          }

          // Smart Glossary cell content.
          elseif ($semantic_module_key == 'smart_glossary') {
            $project_sparql_content = '';
            if (isset($project->sparql_endpoint_url) && isset($sparql_endpoint_connections[$project->sparql_endpoint_url])) {
              $sparql_endpoint_connection = $sparql_endpoint_connections[$project->sparql_endpoint_url];
              $smart_glossary_project_uses = array();
              if (isset($connections_used[$sparql_endpoint_connection->getId()]) && isset($connections_used[$sparql_endpoint_connection->getId()]['smart_glossary'])) {
                foreach ($connections_used[$sparql_endpoint_connection->getId()]['smart_glossary'] as $smart_glossary_use) {
                  $smart_glossary_project_uses[] = '<li>' . l($smart_glossary_use['title'], $semantic_modules['smart_glossary']['configuration_path'] . '/' . $smart_glossary_use['id'], array('query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector'))) . '</li>';
                }
              }
              if (!empty($smart_glossary_project_uses)) {
                $project_sparql_content .= '<ul>' . implode('', $smart_glossary_project_uses) . '</ul><br />';
              }

              $project_sparql_content .= l(t('Add new Smart Glossary configuration'), $semantic_modules['smart_glossary']['configuration_path'] . '/add/' . $sparql_endpoint_connection->getId(), array('attributes' => array('class' => 'add-configuration'), 'query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector')));
              unset($sparql_endpoint_connections[$project->sparql_endpoint_url]);
            }
            else {
              $project_sparql_content .= '<div class="semantic-connector-italic">' . t('not supported') . ' (' . t('refresh PoolParty server details') . ')</div>';
            }
            $project_row[] = $project_sparql_content;
          }
        }

        // Add the collected data for the project as a row.
        $project_rows[] = $project_row;
      }
    }

    $pp_table_headers = array(t('Projects'));
    foreach ($installed_semantic_modules as $semantic_module_key) {
      $pp_table_headers[] = $semantic_modules[$semantic_module_key]['title'];
    }
    $output['server_projects_' . $server_id] = array(
      '#theme' => 'table',
      '#header' => $pp_table_headers,
      '#rows' => $project_rows,
      '#sticky' => TRUE,
      '#empty' => t('This PoolParty server has no projects available for the configured user.'),
      '#attributes' => array('class' => array('pp-server-projects-table', 'semantic-connector-tablesorter')),
    );
  }

  if (!empty($sparql_endpoint_connections) && in_array('smart_glossary', $installed_semantic_modules)) {
    $output['sparql_endpoints_title'] = array(
      '#type' => 'markup',
      '#markup' => '<h3 class="semantic-connector-table-title">' . t('Custom SPARQL endpoints') . '</h3>',
    );
    $sparql_endpoint_rows = array();

    // Add all the custom SPARQL endpoints.
    foreach ($sparql_endpoint_connections as $sparql_endpoint_connection) {
      $sparql_connection_use_content = '';
        $smart_glossary_project_uses = array();
        if (isset($connections_used[$sparql_endpoint_connection->getId()]) && isset($connections_used[$sparql_endpoint_connection->getId()]['smart_glossary'])) {
          foreach ($connections_used[$sparql_endpoint_connection->getId()]['smart_glossary'] as $smart_glossary_use) {
            $smart_glossary_project_uses[] = '<li>' . l($smart_glossary_use['title'], $semantic_modules['smart_glossary']['configuration_path'] . '/' . $smart_glossary_use['id']) . '</li>';
          }
        }
        if (!empty($smart_glossary_project_uses)) {
          $sparql_connection_use_content .= '<ul>' . implode('', $smart_glossary_project_uses) . '</ul><br />';
        }
      $sparql_connection_use_content .= l(t('Add new Smart Glossary configuration'), $semantic_modules['smart_glossary']['configuration_path'] . '/add/' . $sparql_endpoint_connection->getId(), array('attributes' => array('class' => 'add-configuration')));

      $title = '<div class="semantic-connector-led" data-server-id="' . $sparql_endpoint_connection->getId() . '" data-server-type="sparql-endpoint" title="' . t('Checking service') . '"></div>';
      $title .= l($sparql_endpoint_connection->getTitle(), $sparql_endpoint_connection->getUrl(), array('attributes' => array('target' => array('_blank'))));
      $sparql_endpoint_rows[] = array(
        $title,
        $sparql_connection_use_content,
        _semantic_connector_theme_connection_buttons($sparql_endpoint_connection, empty($uses)),
      );
    }

    $output['sparql_endpoints'] = array(
      '#theme' => 'table',
      '#header' => array(t('URL'), t('Smart Glossary'), t('Operations')),
      '#rows' => $sparql_endpoint_rows,
      '#empty' => t('There are no custom SPARQL endpoint connections'),
      '#attributes' => array(
        'id' => 'sparql-endpoints-table',
        'class' => array('semantic-connector-tablesorter'),
      ),
    );
  }

  $output['#attached'] = array(
    'js' => array(drupal_get_path('module', 'semantic_connector') . '/js/semantic_connector.admin.js'),
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
 * Theme buttons to edit or delete a Semantic Connector connection.
 *
 * @param object $connection
 *   The Semantic Connector connection to theme the buttons for.
 * @param bool $can_be_deleted
 *   Whether a delete-button should be added or not.
 *
 * @return string
 *   The rendered HTML.
 */
function _semantic_connector_theme_connection_buttons($connection, $can_be_deleted = FALSE) {
  $type = str_replace('_', '-', $connection->getType());
  $output = '<div class="semantic-connector-connection-buttons">';

  // Edit-button.
  $output .= l(t('Edit'), 'admin/config/semantic-drupal/semantic-connector/connections/' . $type . '/' . $connection->getId(), array('attributes' => array('class' => array('semantic-connector-connection-buttons-edit'))));

  // Delete button.
  if ($can_be_deleted) {
    $output .= '|' . l(t('Delete'), 'admin/config/semantic-drupal/semantic-connector/connections/' . $type . '/' . $connection->getId() . '/delete', array('attributes' => array('class' => array('semantic-connector-connection-buttons-delete'))));
  }

  // Refresh projects button.
  if ($connection->getType() == 'pp_server') {
    $output .= '|' . l(t('Refresh server details'), 'admin/config/semantic-drupal/semantic-connector/connections/' . $type . '/' . $connection->getId() . '/refresh', array('attributes' => array('class' => array('semantic-connector-connection-buttons-refresh'))));
  }

  $output .= '</div>';
  return $output;
}

/**
 * The form to add a new Semantic Connector connection.
 */
function semantic_connector_connection_add_form($form, &$form_state, $connection_type) {
  $connection_type = str_replace('-', '_', $connection_type);

  $form['server_settings']['type'] = array(
    '#type' => 'hidden',
    '#value' => $connection_type,
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Server title'),
    '#description' => t('A short title for the connection.'),
    '#size' => 35,
    '#maxlength' => 60,
    '#required' => TRUE,
  );

  $form['url'] = array(
    '#type' => 'textfield',
    '#title' => t('URL'),
    '#description' => t('URL of the connection.'),
    '#size' => 35,
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $form['credentials'] = array(
    '#type' => 'fieldset',
    '#title' => t('Credentials'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['credentials']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#description' => t('Name of a user for the credentials.'),
    '#size' => 35,
    '#maxlength' => 60,
  );
  $form['credentials']['pass'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
    '#description' => t('Password of a user for the credentials.'),
    '#size' => 35,
    '#maxlength' => 128,
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Submit handler for the add-form of a Semantic Connector Connection.
 */
function semantic_connector_connection_add_form_submit($form, &$form_state) {
  SemanticConnector::createConnection($form_state['values']['type'], $form_state['values']['url'], $form_state['values']['title'], array(
    'username' => $form_state['values']['name'],
    'password' => $form_state['values']['pass'],
  ));

  drupal_set_message(t('Successfully added connection "%contitle".', array('%contitle' => $form_state['values']['title'])));
  drupal_goto('admin/config/semantic-drupal/semantic-connector');
}

/**
 * The form to edit an existing Semantic Connector connection.
 */
function semantic_connector_connection_edit_form($form, &$form_state, $connection_type, $connection_id) {
  $connection_type = str_replace('-', '_', $connection_type);
  $connection = SemanticConnector::getConnection($connection_type, $connection_id);

  if (!is_null($connection)) {
    $form['server_settings']['connection_id'] = array(
      '#type' => 'hidden',
      '#value' => $connection_id,
    );

    $form['server_settings']['type'] = array(
      '#type' => 'hidden',
      '#value' => $connection_type,
    );

    $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Server title'),
      '#description' => t('A short title for the connection.'),
      '#size' => 35,
      '#maxlength' => 60,
      '#default_value' => $connection->getTitle(),
      '#required' => TRUE,
    );

    $form['url'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('URL of the connection.'),
      '#size' => 35,
      '#maxlength' => 255,
      '#default_value' => $connection->getUrl(),
      '#required' => TRUE,
    );

    $credentials = $connection->getCredentials();
    $form['credentials'] = array(
      '#type' => 'fieldset',
      '#title' => t('Credentials'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['credentials']['name'] = array(
      '#type' => 'textfield',
      '#title' => t('Username'),
      '#description' => t('Name of a user for the credentials.'),
      '#size' => 35,
      '#maxlength' => 60,
      '#default_value' => $credentials['username'],
    );
    $form['credentials']['pass'] = array(
      '#type' => 'textfield',
      '#title' => t('Password'),
      '#description' => t('Password of a user for the credentials.'),
      '#size' => 35,
      '#maxlength' => 128,
      '#default_value' => $credentials['password'],
    );

    $form['save'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
    $form['cancel'] = array(
      '#type' => 'link',
      '#title' => t('Cancel'),
      '#href' => 'admin/config/semantic-drupal/semantic-connector',
    );
  }

  return $form;
}

/**
 * Validate-Handler for the edit-form of a Semantic Connector Connection.
 */
function semantic_connector_connection_edit_form_validate($form, &$form_state) {
  // Check if already exists a PoolParty Server with the entered URL.
  $connections = SemanticConnector::searchConnections(array('type' => $form_state['values']['type'], 'url' => $form_state['values']['url']));
  if (!empty($connections)) {
    foreach ($connections as $connection) {
      if ($connection->getId() != $form_state['values']['connection_id']) {
        form_set_error('url', t('There already exists a PoolParty Server (%name) with this URL. Please enter a different URL.', array('%name' => $connection->getTitle())));
        break;
      }
    }
  }
}

/**
 * Submit handler for the edit-form of a Semantic Connector Connection.
 */
function semantic_connector_connection_edit_form_submit($form, &$form_state) {
  SemanticConnector::createConnection($form_state['values']['type'], $form_state['values']['url'], $form_state['values']['title'], array(
    'username' => $form_state['values']['name'],
    'password' => $form_state['values']['pass'],
  ), array(), $form_state['values']['connection_id']);

  drupal_set_message(t('Successfully updated connection "%contitle".', array('%contitle' => $form_state['values']['title'])));
  drupal_goto('admin/config/semantic-drupal/semantic-connector');
}

/**
 * List deletion form.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param int $connection_type
 *   The type of the connection.
 * @param int $connection_id
 *   The ID of the Semantic Connector connection to delete.
 *
 * @return array
 *   The Drupal form array of the confirmation form.
 */
function semantic_connector_connection_delete_form($form, &$form_state, $connection_type, $connection_id) {
  $connection_type = str_replace('-', '_', $connection_type);
  $form_state['connection_id'] = $connection_id;
  $connection = SemanticConnector::getConnection($connection_type, $connection_id);

  return confirm_form($form,
    t('Are you sure you want to delete the PoolParty Server %title?', array('%title' => $connection->getTitle())),
    'admin/config/semantic-drupal/semantic-connector',
    t('This action cannot be undone.'),
    t('Delete configuration'));
}

/**
 * Submit handler for semantic_connector_connection_delete_form().
 */
function semantic_connector_connection_delete_form_submit($form, &$form_state) {
  $connection_id = $form_state['connection_id'];
  semantic_connector_connection_delete($connection_id, TRUE);
  $form_state['redirect'] = 'admin/config/semantic-drupal/semantic-connector';
}

/**
 * Delete a Semantic Connector connection.
 *
 * @param int $connection_id
 *   The ID of the Semantic Connector connection to delete.
 * @param bool $is_user_interaction
 *   Handle this as a user interaction? If TRUE, a Drupal messages regarding the
 *   success of this operation and a goto will be included.
 *
 * @return bool
 *   TRUE if the operation succeeded, FALSE if not.
 */
function semantic_connector_connection_delete($connection_id, $is_user_interaction = FALSE) {
  $success = FALSE;

  // Delete the connection if it is not required by any configuration.
  $connection_usage = semantic_connector_check_connection_usage();
  if (!isset($connection_usage[$connection_id]) || empty($connection_usage[$connection_id])) {
    SemanticConnector::deleteConnections(array($connection_id));
    $success = TRUE;
  }

  if ($is_user_interaction) {
    if ($success) {
      drupal_set_message(t('The connection was deleted successfully.'));
    }
    else {
      drupal_set_message(t('The connection is used by a module and therefore could not be deleted.'), 'error');
    }
    drupal_goto('admin/config/semantic-drupal/semantic-connector');
  }

  return $success;
}

/**
 * Refresh the projects of a Semantic Connector Poolparty server connection.
 *
 * All depending connections (SPARQL-endpoints) get recreated or updated by
 * resaving the connection.
 *
 * @param string $connection_type
 *   The type of connection to refresh ("pp_server" or "sparql_endpoint")
 * @param int $connection_id
 *   The ID of the Semantic Connector connection to refresh
 */
function semantic_connector_refresh_connection($connection_type, $connection_id) {
  $connection_type = str_replace('-', '_', $connection_type);
  $connection = SemanticConnector::getConnection($connection_type, $connection_id);
  if (!is_null($connection)) {
    $connection->save();
    drupal_set_message(t('Successfully refreshed connection "%contitle".', array('%contitle' => $connection->getTitle())));
  }
  else {
    drupal_set_message(t('Refreshing failed due to an invalid connection ID.'));
  }

  drupal_goto('admin/config/semantic-drupal/semantic-connector');
}

/**
 * Check what Semantic Connector connections are used by which module.
 *
 * @param array $modules_to_check
 *   An array of module keys to check for connections.
 *
 * @return array
 *   Associative array of connections usages, categorized by connection_id and
 *   then by module_key.
 */
function semantic_connector_check_connection_usage(array $modules_to_check = array(
  'powertagging',
  'smart_glossary',
  'sonr_webmining',
)) {
  $connections_used = array();

  foreach ($modules_to_check as $module_key) {
    if (module_exists($module_key)) {
      switch ($module_key) {
        case 'powertagging':
          foreach (powertagging_config_load() as $config) {
            if (!isset($connections_used[$config->connection_id])) {
              $connections_used[$config->connection_id] = array();;
            }
            if (!isset($connections_used[$config->connection_id][$module_key])) {
              $connections_used[$config->connection_id][$module_key] = array();
            }
            $connections_used[$config->connection_id][$module_key][] = array(
              'id' => $config->powertagging_id,
              'title' => $config->title,
              'project_id' => $config->project_id,
            );
          }
          break;

        case 'smart_glossary':
          foreach (smart_glossary_config_load() as $config) {
            if (!isset($connections_used[$config->connection_id])) {
              $connections_used[$config->connection_id] = array();;
            }
            if (!isset($connections_used[$config->connection_id][$module_key])) {
              $connections_used[$config->connection_id][$module_key] = array();
            }
            $connections_used[$config->connection_id][$module_key][] = array(
              'id' => $config->sg_id,
              'title' => $config->title,
            );
          }
          break;

        case 'sonr_webmining':
          foreach (sonr_webmining_config_load_multiple() as $config) {
            $connection_id = $config->getConnectionId();
            if (!isset($connections_used[$connection_id])) {
              $connections_used[$connection_id] = array();;
            }
            if (!isset($connections_used[$connection_id][$module_key])) {
              $connections_used[$connection_id][$module_key] = array();
            }
            $connections_used[$connection_id][$module_key][] = array(
              'id' => $config->getId(),
              'title' => $config->getTitle(),
              'project_id' => $config->getProjectId(),
            );
          }
          break;
      }
    }
  }

  return $connections_used;
}

/**
 * The form for the global Semantic Connector module configuration.
 */
function semantic_connector_configuration_form($form, &$form_state) {
  $form['semantic_connector_version_checking'] = array(
    '#type' => 'checkbox',
    '#title' => t('Service version checking'),
    '#default_value' => variable_get('semantic_connector_version_checking', TRUE),
    '#description' => t("Check for newer versions of PoolParty servers and sOnr servers"),
  );

  $destinations = _semantic_connector_get_destinations();
  foreach ($destinations as $destination_id => $destination) {
    $form['semantic_connector_term_click_destinations']['#tree'] = TRUE;
    // This field is invisible, but contains sort info (weights).
    $form['semantic_connector_term_click_destinations'][$destination_id]['weight'] = array(
      '#type' => 'weight',
      // Weights from -255 to +255 are supported because of this delta.
      '#delta' => 255,
      '#title_display' => 'invisible',
      '#default_value' => $destination['weight'],
    );
    $form['semantic_connector_term_click_destinations'][$destination_id]['label'] = array(
      '#type' => 'markup',
      '#markup' => $destination['label'],
    );

    // Add a list of sub-destinations if required.
    $connection_list_items = '';
    if ($destination_id == 'smart_glossary_detail_page') {
      $configs = smart_glossary_config_load();
      foreach ($configs as $config) {
        $connection_list_items .= '<li>' . l($config->title, 'admin/config/semantic-drupal/smart-glossary/' . $config->sg_id, array('query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector/config'))) . ' <b>' . ((isset($config->advanced_settings['semantic_connection']) && isset($config->advanced_settings['semantic_connection']['show_in_destinations']) && !$config->advanced_settings['semantic_connection']['show_in_destinations']) ? 'deactivated': 'activated') . '</b></li>';
      }
    }
    elseif ($destination_id == 'sonr_webmining') {
      $config_sets = sonr_webmining_config_load_multiple();
      foreach ($config_sets as $config) {
        $advanced_config = $config->getConfig();
        $connection_list_items .= '<li>' . l($config->getTitle(), 'admin/config/semantic-drupal/sonr-webmining/' . $config->getId(), array('query' => array('destination' => 'admin/config/semantic-drupal/semantic-connector/config'))) . ' <b>' . ((isset($advanced_config['semantic_connection']) && isset($advanced_config['semantic_connection']['show_in_destinations']) && !$advanced_config['semantic_connection']['show_in_destinations']) ? 'deactivated': 'activated') . '</b></li>';
      }
    }
    if (!empty($connection_list_items)) {
      $form['semantic_connector_term_click_destinations'][$destination_id]['label']['#markup'] .= '<ul>' . $connection_list_items . '</ul>';
    }

    $form['semantic_connector_term_click_destinations'][$destination_id]['list_title'] = array(
      '#type' => 'textfield',
      '#size' => 15,
      '#maxlength' => 255,
      '#default_value' => $destination['list_title'],
    );
    $form['semantic_connector_term_click_destinations'][$destination_id]['use'] = array(
      '#type' => 'checkbox',
      '#default_value' => $destination['use'],
    );
  }

  // Save and cancel buttons.
  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => 'admin/config/semantic-drupal/semantic-connector',
  );

  return $form;
}

/**
 * Submit handler for semantic_connector_configuration_form.
 */
function semantic_connector_configuration_form_submit($form, &$form_state) {
  variable_set('semantic_connector_version_checking', $form_state['values']['semantic_connector_version_checking']);
  variable_set('semantic_connector_term_click_destinations', $form_state['values']['semantic_connector_term_click_destinations']);
  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Theme function for the configuration set admin form.
 *
 * @param array $variables
 *   An array of variables supported by the theme.
 *
 * @return string
 *   The rendered HTML content of the admin form.
 */
function theme_semantic_connector_configuration_form($variables) {
  $form = $variables['form'];

  if (!isset($form['semantic_connector_term_click_destinations']['#value'])) {
    $rows = array();
    foreach (element_children($form['semantic_connector_term_click_destinations']) as $destination_id) {
      $form['semantic_connector_term_click_destinations'][$destination_id]['weight']['#attributes']['class'] = array('term-click-destinations-order-weight');
      $rows[] = array(
        'data' => array(
          drupal_render($form['semantic_connector_term_click_destinations'][$destination_id]['label']),
          drupal_render($form['semantic_connector_term_click_destinations'][$destination_id]['use']),
          drupal_render($form['semantic_connector_term_click_destinations'][$destination_id]['list_title']),
          drupal_render($form['semantic_connector_term_click_destinations'][$destination_id]['weight']),
        ),
        'class' => array('draggable'),
      );
    }

    $header = array(t('Destination name'), t('Show'), t('List title'), t('Weight'));
    $form['semantic_connector_term_click_destinations'] = array(
      '#type' => 'item',
      '#title' => t('Term Click Destinations'),
      '#description' => t('Select which items should be displayed when clicking on a term.') . '<br />' . t('A whole destination type can be hidden by deselecting the "Show"-checkbox above, single destinations can be hidden inside their module\'s configuration page.'),
      '#markup' => theme(
        'table',
        array(
          'header' => $header,
          'rows' => $rows,
          'attributes' => array('id' => 'term-click-destinations-order'),
        )
      ),
    );

    drupal_add_tabledrag('term-click-destinations-order', 'order', 'sibling', 'term-click-destinations-order-weight');
  }

  $output = drupal_render_children($form);
  return $output;
}
