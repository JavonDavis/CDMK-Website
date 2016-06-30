<?php

/**
 * @file
 * Administration functionality for the PowerTagging.
 */

/**
 * List all saved PowerTagging configurations.
 *
 * @return string
 *   The rendered HTML of the list of configurations.
 */
function powertagging_list() {
  $output = array();

  $output['powertagging_title'] = array(
    '#type' => 'markup',
    '#markup' => '<h3 class="semantic-connector-table-title">' . t('PowerTagging configurations') . '</h3>',
  );

  $config_sets = powertagging_config_load();
  $rows = array();
  foreach ($config_sets as $config) {
    $field_instances = powertagging_get_tagging_field_instances($config->powertagging_id);
    $fields = powertagging_get_tagging_field_options_list($field_instances);
    $fields = empty($fields) ? '<div class="semantic-connector-italic">' . t('not yet set') . '</div>' : theme('item_list', array('items' => $fields));
    $actions = array(
      l(t('Edit'), 'admin/config/semantic-drupal/powertagging/' . $config->powertagging_id),
      l(t('Clone'), 'admin/config/semantic-drupal/powertagging/' . $config->powertagging_id . '/clone'),
      l(t('Delete'), 'admin/config/semantic-drupal/powertagging/' . $config->powertagging_id . '/delete'),
    );
    $title = '<div class="semantic-connector-led" data-server-id="' . $config->connection->getId() . '" data-server-type="pp-server" title="' . t('Checking service') . '"></div>';
    $title .= $config->title;
    $rows[] = array(
      $title,
      l($config->connection->getUrl(), $config->connection->getUrl() . '/PoolParty', array('attributes' => array('target' => '_blank'))),
      (isset($config->config['projects'][$config->project_id]) ? $config->config['projects'][$config->project_id]['label'] : '<i>' . t('project label not found') . '</i>'),
      $fields,
      implode(' | ', $actions),
    );
  }

  $output['powertagging'] = array(
    '#theme' => 'table',
    '#header' => array(
      t('Name'),
      t('URL to PoolParty server'),
      t('Selected project'),
      t('Available in entity type'),
      t('Operations'),
    ),
    '#rows' => $rows,
    '#attributes' => array(
      'id' => 'powertagging-configurations-table',
      'class' => array('semantic-connector-tablesorter'),
    ),
    '#attached' => array(
      'js' => array(drupal_get_path('module', 'powertagging') . '/js/powertagging.admin.js'),
      'css' => array(drupal_get_path('module', 'powertagging') . '/css/powertagging.admin.css'),
    ),
  );

  // Add the tablesorter library if available.
  $tablesorter_path = 'sites/all/libraries/tablesorter';
  if (module_exists('libraries')) {
    $tablesorter_path = libraries_get_path('tablesorter');
  }
  if (file_exists($tablesorter_path . '/jquery.tablesorter.min.js')) {
    $output['#attached']['js'] = array(libraries_get_path('tablesorter') . '/jquery.tablesorter.min.js');
  }

  return $output;
}

/**
 * Ajax callback function for checking if a new PoolParty server is available.
 */
function powertagging_new_available($form, $form_state) {
  $available = '<div id="health_info" class="available">' . t('The server is available.') . '</div>';
  $not_available = '<div id="health_info" class="not-available">' . t('The server is not available or the credentials are incorrect.') . '</div>';

  if (isset($form_state['values']['url']) && valid_url($form_state['values']['url'], TRUE)) {
    // Create a new connection (without saving) with the current form data.
    $connection = SemanticConnector::getConnection('pp_server');
    $connection->setUrl($form_state['values']['url']);
    $connection->setCredentials(array(
      'username' => $form_state['values']['name'],
      'password' => $form_state['values']['pass'],
    ));

    $availability = $connection->getApi('PPX')->available();
    if (isset($availability['message']) && !empty($availability['message'])) {
      return '<div id="health_info" class="not-available">' . $availability['message'] . '</div>';
    }
    else {
      return $availability['success'] ? $available : $not_available;
    }
  }

  return $not_available;
}

/**
 * The form for the global PowerTagging module configuration.
 */
function powertagging_configuration_form($form, &$form_state) {
  $glossary_items_max_options = range(0, 10);
  $glossary_items_max_options[0] = 'Unlimited';

  $form['powertagging_tag_glossary'] = array(
    '#type' => 'fieldset',
    '#title' => t('"PowerTagging Tag Glossary" block configuration'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['powertagging_tag_glossary']['powertagging_tag_glossary_items_max'] = array(
    '#type' => 'select',
    '#options' => $glossary_items_max_options,
    '#title' => t('Number of tag glossary items'),
    '#description' => t('The maximum number of items to show in the "PowerTagging Tag Glossary" block'),
    '#default_value' => variable_get('powertagging_tag_glossary_items_max', 3),
  );

  $form['powertagging_tag_glossary']['powertagging_tag_glossary_use_dbpedia_definition'] = array(
    '#type' => 'checkbox',
    '#title' => t('Fetch definitions from DBpedia'),
    '#description' => t('If a concept has no definition but an exactMatch-entry to DBpedia, the abstract of the DBpedia entry will be used instead'),
    '#default_value' => variable_get('powertagging_tag_glossary_use_dbpedia_definition', FALSE),
  );

  $form['powertagging_tag_glossary']['powertagging_tag_glossary_definition_max_characters'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum characters of definitions'),
    '#description' => t('The maximum number of characters of a definition that will be displayed initially for each concept. If a definition is longer it will be cut on word-basis and ellipsis will be added.!linebreakLeave this field empty if you don\'t want the number of characters to be limited.', array('!linebreak' => '<br />')),
    '#default_value' => variable_get('powertagging_tag_glossary_definition_max_characters', 300),
    '#element_validate' => array('element_validate_integer_positive'),
  );

  return system_settings_form($form);
}

/**
 * A form to update the PP server connection of a PowerTagging configuration.
 *
 * This form is used when creating a completely new PowerTagging configuration
 * or when the PoolParty server connection needs to be changed or a different
 * project shell be used for an existing PowerTagging configuration.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $config
 *   A PowerTagging configuration.
 *
 * @return array
 *   The Drupal form array.
 */
function powertagging_connection_form($form, &$form_state, $config = NULL) {
  if (!is_null($config) && !empty($config)) {
    $is_configuration_new = FALSE;
    $form['powertagging_id'] = array(
      '#type' => 'hidden',
      '#value' => $config->powertagging_id,
    );

    $form['title'] = array(
      '#type' => 'hidden',
      '#value' => $config->title,
    );
  }
  else {
    $is_configuration_new = TRUE;
    $config = powertagging_default_config_load();

    $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Name'),
      '#description' => t('Name of the PowerTagging configuration.'),
      '#size' => 35,
      '#maxlength' => 255,
      '#default_value' => $config->title,
      '#required' => TRUE,
    );
  }

  // Tab: Server settings.
  $form['server_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('PoolParty server connection settings'),
  );

  // Display any unfulfilled module requirement.
  $requirement_errors = powertagging_check_requirements();
  if (!empty($requirement_errors)) {
    $requirement_errors_markup = '';
    foreach ($requirement_errors as $requirement_error) {
      $requirement_errors_markup .= '<div class="messages error">' . $requirement_error . '</div>';
    }
    $form['server_settings']['requirements'] = array(
      '#type' => 'item',
      '#markup' => $requirement_errors_markup,
    );
  }

  $connections = SemanticConnector::getConnectionsByType('pp_server');
  if (!empty($connections)) {
    $connection_options = array();
    foreach ($connections as $connection) {
      $credentials = $connection->getCredentials();
      $connection_options[implode('|', array($connection->getTitle(), $connection->getUrl(), $credentials['username'], $credentials['password']))] = $connection->getTitle();
    }
    $form['server_settings']['load_connection'] = array(
      '#type' => 'select',
      '#title' => t('Load an available PoolParty server'),
      '#options' => $connection_options,
      '#empty_option' => '',
      '#default_value' => '',
    );

    $form['server_settings']['load_connection_button'] = array(
      '#type' => 'button',
      '#value' => t('Load'),
      '#attributes' => array(
        'onclick' => '
        var connection_value = (jQuery(\'#edit-load-connection\').val());
        if (connection_value.length > 0) {
          var connection_details = connection_value.split(\'|\');
          jQuery(\'#edit-server-title\').val(connection_details[0]);
          jQuery(\'#edit-url\').val(connection_details[1]);
          jQuery(\'#edit-name\').val(connection_details[2]);
          jQuery(\'#edit-pass\').val(connection_details[3]);
        }
        return false;',
      ),
    );
  }

  $form['server_settings']['connection_id'] = array(
    '#type' => 'hidden',
    '#value' => $config->connection->getId(),
  );

  $form['server_settings']['server_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Server title'),
    '#description' => t('A short title for the server below.'),
    '#size' => 35,
    '#maxlength' => 60,
    '#default_value' => $config->connection->getTitle(),
    '#required' => TRUE,
  );

  $form['server_settings']['url'] = array(
    '#type' => 'textfield',
    '#title' => t('URL'),
    '#description' => t('URL, where the PoolParty server runs.'),
    '#size' => 35,
    '#maxlength' => 255,
    '#default_value' => $config->connection->getUrl(),
    '#required' => TRUE,
  );

  $credentials = $config->connection->getCredentials();
  $form['server_settings']['credentials'] = array(
    '#type' => 'fieldset',
    '#title' => t('Credentials'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['server_settings']['credentials']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#description' => t('Name of a user for the credentials.'),
    '#size' => 35,
    '#maxlength' => 60,
    '#default_value' => $credentials['username'],
  );
  $form['server_settings']['credentials']['pass'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
    '#description' => t('Password of a user for the credentials.'),
    '#size' => 35,
    '#maxlength' => 128,
    '#default_value' => $credentials['password'],
  );

  $form['server_settings']['health_check'] = array(
    '#type' => 'button',
    '#value' => t('Health check'),
    '#ajax' => array(
      'callback' => 'powertagging_new_available',
      'wrapper' => 'health_info',
      'method' => 'replace',
      'effect' => 'slide',
    ),
  );

  if ($is_configuration_new) {
    $markup = '<div id="health_info">' . t('Click to check if the server is available.') . '</div>';
  }
  else {
    $available = '<div id="health_info" class="available">' . t('The server is available.') . '</div>';
    $not_available = '<div id="health_info" class="not-available">' . t('The server is not available or the credentials are incorrect.') . '</div>';
    $markup = $config->connection->available() ? $available : $not_available;
  }
  $form['server_settings']['health_info'] = array(
    '#markup' => $markup,
  );

  // Get the project options for the currently configured PoolParty server.
  $project_options = array();
  if (!$is_configuration_new) {
    $projects = $config->connection->getApi('PPX')->getProjects();
    foreach ($projects as $project) {
      $project_options[$project->uuid] = $project->label;
    }
  }
  // configuration set admin page.
  $form['project'] = array(
    '#type' => 'select',
    '#title' => 'Select a project',
    '#prefix' => '<div id="projects-replace">',
    '#suffix' => '</div>',
    '#options' => $project_options,
    '#default_value' => (!$is_configuration_new ? $config->project_id : NULL),
    '#required' => TRUE,
    '#validated' => TRUE,
  );

  $form['load_projects'] = array(
    '#type' => 'button',
    '#value' => 'Load projects',
    '#ajax' => array(
      'event' => 'click',
      'callback' => 'powertagging_connection_form_get_projects',
      'wrapper' => 'projects-replace',
    ),
  );

  // Save and cancel buttons.
  $form['save'] = array(
    '#type' => 'submit',
    '#value' => ($is_configuration_new ? t('Save and edit details') : t('Save')),
    '#prefix' => '<div class="admin-form-submit-buttons" style="margin-top: 40px;">',
  );
  $form['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => 'admin/config/semantic-drupal/powertagging',
    '#suffix' => '</div>',
  );

  return $form;
}

/**
 * Ajax callback function to get a project select list for a given PoolParty
 * server connection configuration.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 *
 * @return array
 *   The select form element containing the project options for the current
 *   PoolParty server connection.
 */
function powertagging_connection_form_get_projects($form, $form_state) {
  $projects_element = $form['project'];

  $project_options = array();
  if (isset($form_state['values']['url']) && valid_url($form_state['values']['url'], TRUE)) {
    // Create a new connection (without saving) with the current form data.
    $connection = SemanticConnector::getConnection('pp_server');
    $connection->setUrl($form_state['values']['url']);
    $connection->setCredentials(array(
      'username' => $form_state['values']['name'],
      'password' => $form_state['values']['pass'],
    ));

    $projects = $connection->getApi('PPX')->getProjects();
    foreach ($projects as $project) {
      $project_options[$project->uuid] = $project->label;
    }
  }

  $projects_element['#options'] = $project_options;
  return $projects_element;
}

/**
 * The validation handler for the PowerTagging connection form.
 */
function powertagging_connection_form_validate($form, &$form_state) {
  // Only do project validation during the save-operation, not during
  // AJAX-requests like the health check of the server.
  if ($form_state['triggering_element']['#parents'][0] == 'save') {
    // A project needs to be selected.
    if (empty($form_state['values']['project'])) {
      form_set_error('project', t('Please select a project.'));
    }
    // And it has to be a valid project (one that is available on the connected
    // PoolParty server).
    else {
      if (isset($form_state['values']['url']) && valid_url($form_state['values']['url'], TRUE)) {
        // Create a new connection (without saving) with the current form data.
        $connection = SemanticConnector::getConnection('pp_server');
        $connection->setUrl($form_state['values']['url']);
        $connection->setCredentials(array(
          'username' => $form_state['values']['name'],
          'password' => $form_state['values']['pass'],
        ));
        $projects = $connection->getApi('PPX')->getProjects();
        $project_is_valid = FALSE;
        foreach ($projects as $project) {
          if ($project->uuid == $form_state['values']['project']) {
            $project_is_valid = TRUE;
            break;
          }
        }
        if (!$project_is_valid) {
          form_set_error('project', t('The selected project is not available on the given PoolParty server configuration.'));
        }
      }
      else {
        form_set_error('url', t('The field URL must be a valid URL.'));
      }
    }
  }
}


/**
 * The submit handler for the PowerTagging connection form.
 */
function powertagging_connection_form_submit($form, &$form_state) {
  // Always create a new connection, if URL and type are the same the old one
  // will be used anyway.
  $connection = SemanticConnector::createConnection('pp_server', $form_state['values']['url'], $form_state['values']['server_title'], array(
    'username' => $form_state['values']['name'],
    'password' => $form_state['values']['pass'],
  ));

  // Update an existing configuration set.
  if (isset($form_state['values']['powertagging_id'])) {
    $ptid = $form_state['values']['powertagging_id'];
    db_update('powertagging')
      ->fields(array(
        'connection_id' => $connection->getId(),
        'project_id' => $form_state['values']['project'],
      ))
      ->condition('powertagging_id', $ptid)
      ->execute();

    // Insert/update skos importer configuration set.
    if (module_exists('skos_importer')) {
      $existing_config = powertagging_config_load($ptid);
      $form_state['values']['projects'] = $existing_config->config['projects'];
      powertagging_skos_importer_config_save($form_state['values']);
    }

    drupal_set_message(t('The connection for PowerTagging configuration %title has been updated.', array('%title' => $form_state['values']['title'])));
  }
  // Create a new configuration set.
  else {
    $default_config = powertagging_default_config_load();
    $ptid = db_insert('powertagging')
      ->fields(array(
        'title' => $form_state['values']['title'],
        'connection_id' => $connection->getId(),
        'project_id' => $form_state['values']['project'],
        'config' => serialize($default_config->config),
      ))
      ->execute();

    drupal_set_message(t('PowerTagging configuration %title has been created.', array('%title' => $form_state['values']['title'])));
  }

  $form_state['redirect'] = 'admin/config/semantic-drupal/powertagging/' . $ptid;
}

/**
 * The form for a PowerTagging configuration.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $config
 *   A PowerTagging configuration.
 *
 * @return array
 *   The Drupal form array.
 */
function powertagging_form($form, &$form_state, $config = NULL) {
  $form['powertagging_id'] = array(
    '#type' => 'hidden',
    '#value' => $config->powertagging_id,
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Name of the PowerTagging configuration.'),
    '#size' => 35,
    '#maxlength' => 255,
    '#default_value' => $config->title,
    '#required' => TRUE,
  );

  // Get the project title of the currently configured project.
  $project_title = '<invalid project selected>';
  $pp_server_projects = $config->connection->getApi('PPX')->getProjects();
  foreach ($pp_server_projects as $pp_server_project) {
    if ($pp_server_project->uuid == $config->project_id) {
      $project_title = $pp_server_project->label;
    }
  }

  // Add information about the connection.
  $connection_markup = '';
  // Check the PoolParty server version if required.
  if (variable_get('semantic_connector_version_checking', TRUE)) {
    $api_version_info = $config->connection->getVersionInfo('PPX');
    if (version_compare($api_version_info['installed_version'], $api_version_info['latest_version'], '<')) {
      $connection_markup .= '<div class="messages warning">' . t('The installed PoolParty server version is not up to date. You are currently running version %installedversion, upgrade to version %latestversion to enjoy the new features.', array('%installedversion' => $api_version_info['installed_version'], '%latestversion' => $api_version_info['latest_version'])) . '</div>';
    }
  }
  $connection_markup .= '<p id="powertagging-connection-info">Connected PoolParty server: <b>' . $config->connection->getTitle() . ' (' . $config->connection->getUrl() . ')</b><br />'
    . 'Selected project: <b>' . $project_title . '</b><br />'
    . l('Change the connected PoolParty server or project', 'admin/config/semantic-drupal/powertagging/' . $config->powertagging_id . '/edit_connection') . '</p>';
  $form['pp_connection_markup'] = array(
    '#type' => 'markup',
    '#markup' => $connection_markup,
  );

  // Define the container for the vertical tabs.
  $form['settings'] = array(
    '#type' => 'vertical_tabs',
  );

  // Tab: Available project settings.
  $form['project_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Available projects'),
    '#group' => 'settings',

  );

  $form['project_settings']['projects'] = array(
    '#type' => 'item',
    '#tree' => TRUE,
  );

  $available_languages = array(
    LANGUAGE_NONE => (object) array(
      'language' => LANGUAGE_NONE,
      'name' => 'Language neutral',
      'enabled' => TRUE,
    ),
  );
  $available_languages = array_merge($available_languages, language_list());

  $pp_api = $config->connection->getApi('PPX');
  $projects = $pp_api->getProjects();
  if (!empty($projects)) {
    foreach ($projects as $project) {
      if ($project->uuid == $config->project_id) {
        $form['project_settings']['projects'][$project->uuid] = array(
          '#type' => 'fieldset',
          '#title' => $project->label,
          '#description' => t('Please map the PoolParty project languages to the Drupal languages:'),
        );

        $form['project_settings']['projects'][$project->uuid]['label'] = array(
          '#type' => 'hidden',
          '#value' => $project->label,
        );

        // Language mapping.
        $project_language_options = array();
        foreach ($project->languages as $project_language) {
          $project_language_options[$project_language] = $project_language;
        }

        $states = array();
        foreach ($available_languages as $lang) {
          if ($lang->enabled) {
            $form['project_settings']['projects'][$project->uuid]['languages'][$lang->language] = array(
              '#type' => 'select',
              '#title' => $lang->name,
              '#options' => $project_language_options,
              '#empty_option' => '',
              '#default_value' => ((isset($config->config['projects']) && isset($config->config['projects'][$project->uuid]) && isset($config->config['projects'][$project->uuid]['languages'][$lang->language])) ? $config->config['projects'][$project->uuid]['languages'][$lang->language] : ''),
            );
            $states['#edit-projects-' . strtolower($project->uuid) . '-languages-' . $lang->language] = array('value' => '');
          }
        }

        // Vocabulary selection.
        // Hidden field for the selecting the vocabulary.
        // It checks the availability of a language.
        $form['project_settings']['projects'][$project->uuid]['no_language_selected'] = array(
          '#type' => 'checkbox',
          '#default_value' => TRUE,
          '#attributes' => array('class' => array('element-hidden')),
          '#states' => array(
            'checked' => $states,
          ),
        );
        $form['project_settings']['projects'][$project->uuid]['taxonomy_name'] = array(
          '#title' => t('Select or enter a new vocabulary'),
          '#type' => 'textfield',
          '#default_value' => ((isset($config->config['projects']) && isset($config->config['projects'][$project->uuid]) && isset($config->config['projects'][$project->uuid]['taxonomy_name'])) ? $config->config['projects'][$project->uuid]['taxonomy_name'] : ''),
          '#autocomplete_path' => 'powertagging/autocomplete-taxonomies',
          '#states' => array(
            'required' => array(
              '#edit-projects-' . strtolower($project->uuid) . '-no-language-selected' => array('checked' => FALSE),
            ),
            'disabled' => $states,
          ),
        );

        // Ask if the vocabulary should be removed also if no language is
        // selected.
        if (isset($config->config['projects'][$project->uuid]['taxonomy_name']) && !empty($config->config['projects'][$project->uuid]['taxonomy_name'])) {
          $form['project_settings']['projects'][$project->uuid]['remove_taxonomy'] = array(
            '#type' => 'checkbox',
            '#title' => t('Remove the appropriate vocabulary. All terms and relations of this vocabulary will be removed also.'),
            '#states' => array(
              'visible' => $states,
            ),
          );
        }
        break;
      }
    }
  }
  else {
    $form['project_settings']['errors'] = array(
      '#type' => 'item',
      '#markup' => '<div class="messages warning">' . t('Either no connection can be established or there are no projects available for the given credentials.') . '</div>',
    );
  }

  // Tab: Global limits.
  $form['global_limits_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Global limits'),
    '#group' => 'settings',
  );

  powertagging_global_limits_form($form['global_limits_settings'], $config->config['concepts_per_extraction'], $config->config['concepts_threshold'], $config->config['freeterms_per_extraction'], $config->config['freeterms_threshold']);

  // Search for all content types with the PowerTagging field.
  $field_instances = powertagging_get_tagging_field_instances();
  if (!empty($field_instances)) {
    $form['global_limits_settings']['overwriting'] = array(
      '#type' => 'fieldset',
      '#title' => t('List of all content types with "PowerTagging Tags" fields'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
      '#description' => t('Select those content types which ones you want to overwrite the limits with the global limits defined above.'),
      '#tree' => FALSE,
    );
    if (count($field_instances) > 1) {
      $form['global_limits_settings']['overwriting']['select_all_content_types'] = array(
        '#type' => 'checkbox',
        '#title' => t('Select all'),
        '#attributes' => array(
          'onclick' => 'jQuery("#edit-overwrite-content-types").find("input").attr("checked", jQuery(this).attr("checked"));',
        ),
      );
    }
    $form['global_limits_settings']['overwriting']['overwrite_content_types'] = array(
      '#type' => 'checkboxes',
      '#options' => powertagging_get_tagging_field_options_list($field_instances),
      '#validated' => TRUE,
    );
  }

  // Tab: Batch Jobs.
  $form['batch_jobs'] = array(
    '#type' => 'fieldset',
    '#title' => t('Batch jobs'),
    '#group' => 'settings',
    '#tree' => TRUE,
  );

  $bulk_projects = array();
  if (isset($config->config['projects'])) {
    $project_rows = array();

    foreach ($projects as $project) {
      if ($project->uuid == $config->project_id && isset($config->config['projects'][$project->uuid])) {
        $project_languages = array();
        foreach ($config->config['projects'][$project->uuid]['languages'] as $pp_language) {
          if (!empty($pp_language)) {
            $project_languages[] = $pp_language;
          }
        }

        if (!empty($project_languages)) {
          $bulk_projects[] = $project->uuid;

          $project_operations = array(
            l(t('Index'), 'admin/config/semantic-drupal/powertagging/' . $config->powertagging_id . '/index'),
            l(t('Update taxonomy'), 'admin/config/semantic-drupal/powertagging/' . $config->powertagging_id . '/update-taxonomy')
          );
          $project_rows[$project->uuid] = array(
            'label' => $project->label . ' (' . implode(', ', $project_languages) . ')',
            'operations' => implode('&emsp;', $project_operations),
          );

          if (module_exists('skos_importer')) {
            $project_rows[$project->uuid]['skos_import'] = l(t('Synchronize'), 'admin/config/semantic-drupal/powertagging/' . $config->powertagging_id . '/synchronize');
          }
        }
      }
    }
    if (!empty($project_rows)) {
      $header = array(
        'label' => t('Project'),
        'operations' => 'Operations',
      );

      if (module_exists('skos_importer')) {
        $header['skos_import'] = '';
      }

      $form['batch_jobs']['projects'] = array(
        '#type' => 'item',
        '#markup' => theme('table', array(
          'header' => $header,
          'rows' => $project_rows,
          'attributes' => array(),
          'empty' => t('No content available.'),
          'sticky' => FALSE,
          'colgroups' => array(),
          'caption' => NULL,
        )),
      );
    }
  }

  if (empty($bulk_projects)) {
    $form['batch_jobs']['errors'] = array(
      '#type' => 'item',
      '#markup' => t('Projects need to be correctly configured before bulk operations can be run for them.'),
    );
  }

  /*if (module_exists('smart_glossary')) {
    // Tab: Semantic module connection.
    $form['semantic_connection'] = array(
      '#type' => 'fieldset',
      '#title' => t('Semantic module connection'),
      '#group' => 'settings',
      '#tree' => TRUE,
    );
  }*/

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['delete'] = array(
    '#type' => 'button',
    '#value' => t('Delete'),
    '#attributes' => array('onclick' => 'window.location.href = "' . url('admin/config/semantic-drupal/powertagging/' . $config->powertagging_id . '/delete') . '"; return false;'),
  );
  $form['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => (isset($_GET['destination']) ? $_GET['destination'] : 'admin/config/semantic-drupal/powertagging'),
  );

  // Add CSS and JS files
  $form['#attached'] = array(
    'js' => array(drupal_get_path('module', 'powertagging') . '/js/powertagging.admin.js'),
    'css' => array(drupal_get_path('module', 'powertagging') . '/css/powertagging.admin.css'),
  );

  return $form;
}

/**
 * Validate handler for powertagging_form.
 */
function powertagging_form_validate($form, &$form_state) {
  $config = powertagging_config_load($form_state['values']['powertagging_id']);
  // If there are any requirement errors, do not save the configuration.
  $requirement_errors = powertagging_check_requirements();
  if (!empty($requirement_errors)) {
    $requirement_errors_markup = t('The configuration could not be saved due to unfulfilled module requirements:');
    foreach ($requirement_errors as $requirement_error) {
      $requirement_errors_markup .= '<br />&nbsp;- ' . $requirement_error;
    }

    form_set_error('', $requirement_errors_markup);
    return FALSE;
  }

  $values = $form_state['values'];
  $default_language = language_default('language');

  if (isset($values['projects']) && !empty($values['projects'])) {
    foreach ($values['projects'] as $key => $project) {
      if ($key == $config->project_id) {
        if (!$project['no_language_selected'] && empty($project['taxonomy_name'])) {
          form_set_error('projects][' . $key . '][taxonomy_name', t('"Select or enter a new vocabulary" field is required.'));
        }

        if (!powertagging_translation_possible()) {
          foreach ($project['languages'] as $language) {
            if (!empty($language) && $language != LANGUAGE_NONE && $language != $default_language) {
              $drupal_languages = language_list();
              form_set_error('projects][' . $key . '][languages][' . $language, t('Language "%language" requires localization of taxonomies as it is not your default site language.<br /> Install and enable module "!internationalizationlink" and its sub-module "Taxonomy translation" to make multilingual tagging possible.', array(
                '%language' => $drupal_languages[$language]->name,
                '!internationalizationlink' => l(t('Internationalization'), 'https://www.drupal.org/project/i18n', array('attributes' => array('target' => '_blank'))),
              )));
            }
          }
        }
        break;
      }
    }
  }
}

/**
 * Submit handler for powertagging_form.
 */
function powertagging_form_submit($form, &$form_state) {
  // Get taxonomy id if a taxonomy name is given for each project.
  if (isset($form_state['values']['projects'])) {
    // Get all existing taxonomies.
    $taxonomy_names = taxonomy_vocabulary_get_names();
    $vids = array();
    foreach ($taxonomy_names as $taxonomy_data) {
      $vids[$taxonomy_data->name] = $taxonomy_data->vid;
    }
    // Go through all projects.
    foreach ($form_state['values']['projects'] as &$project) {
      if (!empty($project['taxonomy_name'])) {
        $taxonomy_name = trim(check_plain($project['taxonomy_name']));
        if (isset($vids[$taxonomy_name])) {
          if (isset($project['remove_taxonomy']) && $project['remove_taxonomy']) {
            taxonomy_vocabulary_delete($vids[$taxonomy_name]);
            drupal_set_message(t('Vocabulary %name has been removed.', array('%name' => $project['taxonomy_name'])));
            unset($project['taxonomy_id']);
            unset($project['taxonomy_name']);
          }
          else {
            // Make sure the already existing vocabulary is able to store
            // PowerTagging tags.
            powertagging_modify_vocabulary($vids[$taxonomy_name]);
            $project['taxonomy_id'] = $vids[$taxonomy_name];
          }
        }
        else {
          // If taxonomy does not exists, it will be created.
          $taxonomy = powertagging_create_vocabulary($taxonomy_name);
          $project['taxonomy_id'] = $taxonomy->vid;
        }
      }
    }
  }

  // Serialize the configuration data.
  $serialized_config = serialize(array(
    'concepts_per_extraction' => $form_state['values']['concepts_per_extraction']['value'],
    'concepts_threshold' => $form_state['values']['concepts_threshold']['value'],
    'freeterms_per_extraction' => $form_state['values']['freeterms_per_extraction']['value'],
    'freeterms_threshold' => $form_state['values']['freeterms_threshold']['value'],
    'projects' => (isset($form_state['values']['projects']) ? $form_state['values']['projects'] : array()),
  ));

  // Save configuration into database.
  db_update('powertagging')
    ->fields(array(
        'title' => $form_state['values']['title'],
        'config' => $serialized_config,
      ))
    ->condition('powertagging_id', $form_state['values']['powertagging_id'])
    ->execute();

  // Overwrite content type limits with the global limits for
  // each selected content type.
  // TODO: Remove the same code from the climate_tagger module,
  //       when this module goes productive.
  if (!module_exists('climate_tagger') && isset($form_state['values']['overwrite_content_types'])) {
    $overwrite_content_types = array_filter($form_state['values']['overwrite_content_types']);
    if (!empty($overwrite_content_types)) {
      foreach ($overwrite_content_types as $content_type) {
        list($entity_type, $bundle_name, $field_name) = explode(' ', $content_type);
        $instance = field_info_instance($entity_type, $field_name, $bundle_name);
        $limits = array(
          'concepts_per_extraction' => array('value' => $form_state['values']['concepts_per_extraction']['value']),
          'concepts_threshold' => array('value' => $form_state['values']['concepts_threshold']['value']),
          'freeterms_per_extraction' => array('value' => $form_state['values']['freeterms_per_extraction']['value']),
          'freeterms_threshold' => array('value' => $form_state['values']['freeterms_threshold']['value']),
        );
        $instance['settings'] = array_merge($instance['settings'], $limits);
        field_update_instance(($instance));
      }
    }
  }

  // Insert/update skos importer configuration set.
  if (module_exists('skos_importer')) {
    powertagging_skos_importer_config_save($form_state['values']);
  }

  drupal_set_message(t('PowerTagging configuration %title has been saved.', array('%title' => $form_state['values']['title'])));
  $form_state['redirect'] = 'admin/config/semantic-drupal/powertagging';
}

/**
 * Add a PowerTagging configuration for a predefined PP connection + project.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param int $connection_id
 *   The ID of the PoolParty server connection
 * @param string $project_id
 *   The ID of the project on the PoolParty server
 *
 * @return array
 *   The Drupal form array of the confirmation form.
 */
function powertagging_add_config_for_fixed_connection_form($form, &$form_state, $connection_id, $project_id) {
  $form_state['connection_id'] = $connection_id;
  $form_state['project_id'] = $project_id;

  $connection = SemanticConnector::getConnection('pp_server', $connection_id);
  $pp_config = $connection->getConfig();
  foreach ($pp_config['projects'] as $project) {
    if ($project->id == $project_id) {
      return confirm_form($form,
        t('Are you sure you want to create the PowerTagging configuration?'),
        (isset($_GET['destination']) ? $_GET['destination'] : 'admin/config/semantic-drupal/powertagging'),
        'Selected PoolParty server: <b>' . $connection->getTitle() . '</b><br />Selected project: <b>' .$project->title . '</b>',
        t('Create configuration'));
      break;
    }
  }
}

/**
 * Submit handler for powertagging_add_config_for_fixed_connection_form().
 */
function powertagging_add_config_for_fixed_connection_form_submit($form, &$form_state) {
  $connection = SemanticConnector::getConnection('pp_server', $form_state['connection_id']);
  $project_id = $form_state['project_id'];

  // Get the current list of projects from the server to make sure the project
  // still exists.
  $project_is_valid = FALSE;
  $projects = $connection->getApi('PPX')->getProjects();
  foreach ($projects as $project) {
    if ($project->uuid == $project_id) {
      $project_is_valid = TRUE;

      // Create a new configuration set.
      $default_config = powertagging_default_config_load();
      $configuration_set_title = 'PowerTagging configuration for ' . $connection->getTitle() . ' (' . $project->label . ')';
      $ptid = db_insert('powertagging')
        ->fields(array(
          'title' => $configuration_set_title,
          'connection_id' => $connection->getId(),
          'project_id' => $project_id,
          'config' => serialize($default_config->config),
        ))
        ->execute();

      drupal_set_message(t('PowerTagging configuration "%title" has been created.', array('%title' => $configuration_set_title)));
      // Drupal Goto to forward a destination if one is available.
      if (isset($_GET['destination'])) {
        $destination = $_GET['destination'];
        unset($_GET['destination']);
        drupal_goto('admin/config/semantic-drupal/powertagging/' . $ptid, array('query' => array('destination' => $destination)));
      }
      $form_state['redirect'] = 'admin/config/semantic-drupal/powertagging' . $ptid;
      break;
    }
  }

  if (!$project_is_valid) {
    drupal_set_message(t('Project-ID "%projectid" was not found on PoolParty server "%ppservertitle"', array('%projectid' => $project_id,'%ppservertitle' => $connection->getTitle())));
    $form_state['redirect'] = 'admin/config/semantic-drupal/powertagging';
  }
}

/**
 * The confirmation-form for cloning a PowerTagging config.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $config
 *   A PowerTagging configuration.
 *
 * @return array
 *   The Drupal form array of the confirmation form.
 */
function powertagging_clone_form($form, &$form_state, $config) {
  $form_state['config'] = $config;
  return confirm_form($form,
    t('Are you sure you want to clone the PowerTagging configuration "%title"?', array('%title' => $config->title)),
    'admin/config/semantic-drupal/powertagging',
    '',
    t('Clone configuration'));
}

/**
 * The submit handler of the confirmation-form to clone a PowerTagging config.
 */
function powertagging_clone_form_submit($form, &$form_state) {
  $config = $form_state['config'];

  $ptid = db_insert('powertagging')
    ->fields(array(
      'title' => $config->title . ' (CLONE)',
      'connection_id' => $config->connection_id,
      'project_id' => $config->project_id,
      'config' => serialize($config->config),
    ))
    ->execute();

  drupal_set_message(t('%title has been cloned.', array('%title' => $config->title)));
  $form_state['redirect'] = 'admin/config/semantic-drupal/powertagging/' . $ptid;
}

/**
 * List deletion form.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $config
 *   A PowerTagging configuration.
 *
 * @return array
 *   The Drupal form array of the confirmation form.
 */
function powertagging_delete_form($form, &$form_state, $config) {
  $form_state['config'] = $config;
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $config->title)),
    'admin/config/semantic-drupal/powertagging',
    t('This action cannot be undone.'),
    t('Delete configuration'));
}

/**
 * Submit handler for powertagging_delete_form().
 */
function powertagging_delete_form_submit($form, &$form_state) {
  $config = $form_state['config'];
  powertagging_config_delete($config->powertagging_id);
  drupal_set_message(t('%title has been deleted.', array('%title' => $config->title)));
  $form_state['redirect'] = 'admin/config/semantic-drupal/powertagging';
}

/**
 * Create a new taxonomy for PowerTagging tags.
 *
 * @param string $name
 *   The human readable name of the new taxonomy.
 *
 * @return object
 *   The created vocabulary object.
 */
function powertagging_create_vocabulary($name) {
  // Create the new Drupal taxonomy.
  $machine_name = powertagging_machine_name($name);
  $vocabulary = (object) array(
    'name' => $name,
    'machine_name' => $machine_name,
    'description' => t('Automatically created by PowerTagging Module'),
    'hierarchy' => 2,
  );
  taxonomy_vocabulary_save($vocabulary);

  // Add required fields for PowerTagging.
  powertagging_modify_vocabulary($vocabulary->vid);

  drupal_set_message(t('Vocabulary %name has been created.', array('%name' => $vocabulary->name)));

  return $vocabulary;
}

/**
 * Modify a taxonomy in a way that it can be used to store PowerTagging tags.
 *
 * @param int $vid
 *   The vocabulary id of the taxonomy to modify.
 */
function powertagging_modify_vocabulary($vid) {
  $taxonomy = taxonomy_vocabulary_load($vid);
  $machine_name = $taxonomy->machine_name;

  // Create an URI field if it does not exist yet.
  $field_name = 'field_uri';
  $field      = field_info_field($field_name);
  if (is_null($field)) {
    $field = array(
      'field_name'  => $field_name,
      'type'        => 'text',
      'label'       => t('URI'),
    );
    field_create_field($field);
  }

  // Attach the URI field to the taxonomy if not yet attached.
  $instance = field_info_instance('taxonomy_term', $field_name, $machine_name);
  if (is_null($instance)) {
    $instance = array(
      'field_name'  => $field_name,
      'entity_type' => 'taxonomy_term',
      'bundle'      => $machine_name,
      'label'       => t('URI'),
      'description' => t('The URI of the concept'),
      'required'    => FALSE,
      'widget'      => array(
        'type'      => 'text_textfield',
        'weight'    => 3,
      ),
    );
    field_create_instance($instance);
  }

  // Create an "alt label" field if it does not exist yet.
  $field_name = 'field_alt_labels';
  $field      = field_info_field($field_name);
  if (is_null($field)) {
    $field = array(
      'field_name'  => $field_name,
      'type'        => 'text',
      'label'       => t('Alternative labels'),
      'description' => t('A comma separated list of synonyms'),
      'settings'    => array(
        'max_length' => 8191,
      ),
    );
    field_create_field($field);
  }

  // Attach the "alt label" field to the taxonomy if not yet attached.
  $instance = field_info_instance('taxonomy_term', $field_name, $machine_name);
  if (is_null($instance)) {
    $instance = array(
      'field_name'  => $field_name,
      'entity_type' => 'taxonomy_term',
      'bundle'      => $machine_name,
      'label'       => t('Alternative labels'),
      'description' => t('A comma separated list of synonyms'),
      'required'    => FALSE,
      'widget'      => array(
        'type'      => 'text_textfield',
        'weight'    => 4,
      ),
    );
    field_create_instance($instance);
  }

  // Create an "hidden label" field if it does not exist yet.
  $field_name = 'field_hidden_labels';
  $field      = field_info_field($field_name);
  if (is_null($field)) {
    $field = array(
      'field_name'  => $field_name,
      'type'        => 'text',
      'label'       => t('Hidden labels'),
      'description' => t('A comma separated list of secondary variants of this concept'),
      'settings'    => array(
        'max_length' => 8191,
      ),
    );
    field_create_field($field);
  }

  // Attach the "hidden label" field to the taxonomy if not yet attached.
  $instance = field_info_instance('taxonomy_term', $field_name, $machine_name);
  if (is_null($instance)) {
    $instance = array(
      'field_name'  => $field_name,
      'entity_type' => 'taxonomy_term',
      'bundle'      => $machine_name,
      'label'       => t('Hidden labels'),
      'description' => t('A comma separated list of secondary variants of this concept'),
      'required'    => FALSE,
      'widget'      => array(
        'type'      => 'text_textfield',
        'weight'    => 5,
      ),
    );
    field_create_instance($instance);
  }

  // Create an "exact match" field if it does not exist yet.
  $field_name = 'field_exact_match';
  $field      = field_info_field($field_name);
  if (is_null($field)) {
    $field = array(
      'field_name'  => $field_name,
      'type'        => 'text',
      'label'       => t('Exact matches'),
      'description' => t('URIs which show to the same concept at a different data source'),
      'cardinality' => -1,
      'settings'    => array(
        'max_length' => 1024,
      ),
    );
    field_create_field($field);
  }

  // Attach the "exact match" field to the taxonomy if not yet attached.
  $instance = field_info_instance('taxonomy_term', $field_name, $machine_name);
  if (is_null($instance)) {
    $instance = array(
      'field_name'  => $field_name,
      'entity_type' => 'taxonomy_term',
      'bundle'      => $machine_name,
      'label'       => t('Exact matches'),
      'description' => t('URIs which show to the same concept at a different data source'),
      'required'    => FALSE,
      'widget'      => array(
        'type'      => 'text_textfield',
        'weight'    => 6,
      ),
    );
    field_create_instance($instance);
  }

  // Make the vocabulary localized for concept translations.
  if (powertagging_translation_possible() && (!isset($taxonomy->i18n_mode) || $taxonomy->i18n_mode != I18N_MODE_LOCALIZE)) {
    db_update('taxonomy_vocabulary')
      ->fields(array('i18n_mode' => I18N_MODE_LOCALIZE))
      ->condition('vid', $taxonomy->vid)
      ->execute();
  }
}

/**
 * Start the indexing-process for a PowerTagging configuration.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $powertagging_config
 *   A PowerTagging configuration.
 *
 * @return array
 *   The Drupal form array.
 */
function powertagging_index_form($form, &$form_state, $powertagging_config) {
  $field_instances = powertagging_get_tagging_field_instances($powertagging_config->powertagging_id);

  if (!empty($field_instances)) {
    // Fixed values for the formatter.
    $form['powertagging_config'] = array(
      '#type' => 'value',
      '#value' => $powertagging_config,
    );

    $form['content_types'] = array(
      '#title' => t('Entity types to be included in the batch process.'),
      '#type' => 'checkboxes',
      '#options' => powertagging_get_tagging_field_options_list($field_instances, TRUE),
      '#required' => TRUE,
    );

    $form['skip_tagged_content'] = array(
      '#title' => t('Skip already tagged content'),
      '#type' => 'radios',
      '#options' => array(
        '1' => t('Yes'),
        '0' => t('No'),
      ),
      '#default_value' => TRUE,
    );

    $form['entities_per_request'] = array(
      '#type' => 'textfield',
      '#title' => t('Entities per request'),
      '#description' => t('The number of entities, that get processed during one HTTP request. (Allowed value range: 1 - 100)') . '<br />' . t('The higher this number is, the less HTTP requests have to be sent to the server until the batch finished tagging ALL your entities, what results in a shorter duration of the bulk tagging process.') . '<br />' . t('Numbers too high can result in a timeout, which will break the whole bulk tagging process.') . '<br />' . t('If entities are configured to get tagged with uploaded files, a value of 5 or below is recommended.'),
      '#required' => TRUE,
      '#default_value' => '10',
    );

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Index',
    );
  }
  else {
    $form['error'] = array(
      '#type' => 'item',
      '#markup' => '<div class="messages error">' . t('No taggable content types found for PowerTagging configuration "%ptconfname".', array('%ptconfname' => $powertagging_config->title)) . '</div>',
    );
  }

  return $form;
}

/**
 * Validate-Handler of the form to index a PowerTagging configuration.
 */
function powertagging_index_form_validate($form, &$form_state) {
  $entities_per_request = $form_state['values']['entities_per_request'];
  if (empty($entities_per_request) || !ctype_digit($entities_per_request) || (int) $entities_per_request == 0 || (int) $entities_per_request > 100) {
    form_set_error('entities_per_request', t('Only values in the range of 1 - 100 are allowed for field "Entities per request"'));
  }
}

/**
 * Submit-Handler of the form to index a PowerTagging configuration.
 */
function powertagging_index_form_submit($form, &$form_state) {
  $content_types = array();
  foreach ($form_state['values']['content_types'] as $content_type_id => $value) {
    if (!empty($value)) {
      $content_types[] = $content_type_id;
    }
  }

  powertagging_index($form_state['values']['powertagging_config'], array(
    'content_types' => $content_types,
    'skip_tagged_content' => $form_state['values']['skip_tagged_content'],
    'entities_per_request' => $form_state['values']['entities_per_request'],
  ));
}

/**
 * Start the taxonomy term updating for a PowerTagging configuration.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $powertagging_config
 *   A PowerTagging configuration.
 *
 * @return array
 *   The Drupal form array.
 */
function powertagging_update_taxonomy_form($form, &$form_state, $powertagging_config) {
  // Taxonomy was configured already.
  if (isset($powertagging_config->config['projects'][$powertagging_config->project_id]) && isset($powertagging_config->config['projects'][$powertagging_config->project_id]['taxonomy_id']) && $powertagging_config->config['projects'][$powertagging_config->project_id]['taxonomy_id'] != 0) {
    $vocabulary = taxonomy_vocabulary_load($powertagging_config->config['projects'][$powertagging_config->project_id]['taxonomy_id']);
    // Taxonomy exists.
    if ($vocabulary !== FALSE) {
      // Fixed values for the formatter.
      $form['powertagging_config'] = array(
        '#type' => 'value',
        '#value' => $powertagging_config,
      );

      $form['vocabulary_name'] = array(
        '#type' => 'value',
        '#value' => $vocabulary->name,
      );

      return confirm_form($form,
        t('Are you sure you want to update taxonomy "@taxonomyname"?', array('@taxonomyname' => $vocabulary->name)),
        'admin/config/semantic-drupal/powertagging/' . $powertagging_config->powertagging_id,
        t('This process updates the concept-details of all taxonomy terms of the connected taxonomy used to save the PowerTagging tags (name, altLabels, ...).'),
        t('Update taxonomy'));
    }
    else {
      $form['error'] = array(
        '#type' => 'item',
        '#markup' => '<div class="messages error">' . t('The connected taxonomy does not exist anymore for PowerTagging configuration "%ptconfname".', array('%ptconfname' => $powertagging_config->title)) . '</div>',
      );
    }
  }
  else {
    $form['error'] = array(
      '#type' => 'item',
      '#markup' => '<div class="messages error">' . t('There was no taxonomy created for PowerTagging configuration "%ptconfname" yet.', array('%ptconfname' => $powertagging_config->title)) . '</div>',
    );
  }
}

/**
 * Submit-Handler of powertagging_update_taxonomy_form().
 */
function powertagging_update_taxonomy_form_submit($form, &$form_state) {
  $powertagging_config = $form_state['values']['powertagging_config'];

  // Get the URIs of all concepts in the connected taxonomy.
  $tid_query = db_select('taxonomy_vocabulary', 'v')
    ->fields('u', array('entity_id'))
    ->condition('v.vid', $powertagging_config->config['projects'][$powertagging_config->project_id]['taxonomy_id']);
  $tid_query->join('field_data_field_uri', 'u', 'u.bundle = v.machine_name AND u.entity_type = \'taxonomy_term\'');
  $tids = $tid_query->execute()
    ->fetchCol();

  $batch_infos = array(
    'total' => count($tids),
    'start_time' => time(),
  );

  $tid_chunks = array_chunk($tids, 10);
  $batch_operations = array();
  foreach ($tid_chunks as $tid_chunk) {
    $batch_operations[] = array('powertagging_update_taxonomy_batch', array($tid_chunk, $powertagging_config, $batch_infos));
  }

  $batch = array(
    'operations' => $batch_operations,
    'progress_message' => '',
    'init_message' => t('Preparing the updating of taxonomy "@taxonomy_name".', array('@taxonomy_name' => $form_state['values']['vocabulary_name'])),
    'finished' => 'powertagging_update_taxonomy_batch_finished',
  );

  batch_set($batch);
  return TRUE;
}

/**
 * Start the indexing-process for a PoolParty project of a PowerTagging config.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form_state array.
 * @param object $powertagging_config
 *   A PowerTagging configuration.
 *
 * @return array
 *   The Drupal form array.
 */
function powertagging_synchronizeProject_form($form, &$form_state, $powertagging_config) {
  if (module_exists('skos_importer')) {
    // Save source and import.
    $source = skos_importer_source($powertagging_config->project_id);

    // Create the config if it does not exist yet.
    if (is_null($source)) {
      powertagging_skos_importer_config_save($form_state['values']);
      $source = skos_importer_source($powertagging_config->project_id);
    }

    $ppt_api = $powertagging_config->connection->getApi('PPT');
    $file_url = $ppt_api->storeProject($powertagging_config->project_id);

    if (!empty($file_url)) {
      $importer = $source->importer();
      $fetcher_config = $importer->fetcher->getConfig();

      if ($fetcher_config['source'] != $file_url) {
        // Update the config of the current fetcher instance.
        $fetcher_config['source'] = $file_url;
        $importer->fetcher->setConfig($fetcher_config);

        // Save the URL for further import-processes.
        $importer_config = $importer->getConfig();
        $importer_config['fetcher']['fetchers']['SKOSImporterHTTPFetcher']['source'] = $file_url;
        $importer->setConfig($importer_config);
        $importer->save();
      }
    }

    // Start the import.
    $source->startImport();

    batch_process('admin/config/semantic-drupal/powertagging/' . $powertagging_config->powertagging_id);
  }
  else {
    drupal_set_message(t('The SKOS Importer module is required for taxonomy synchronization.'), 'error');
  }
}

/**
 * Creates the configuration set for the skos importer for each project.
 *
 * @param array $pt_config
 *   The powertagging configuration set.
 *
 * @return bool
 *   TRUE on success, FALSE on fail.
 */
function powertagging_skos_importer_config_save(array $pt_config) {
  // @todo: make this work with an actual PowerTagging config instead of with
  // form_state['values'].
  if (empty($pt_config['projects'])) {
    return FALSE;
  }

  // Create the RDF/XML URL for the import.
  $http_source = $pt_config['url'];
  if (substr($http_source, -1) == '/') {
    $http_source = substr($http_source, 0, -1);
  }
  $http_source .= '/PoolParty/OpenConfigPathResourceProvider/backuproot?';

  // Create a skos importer configuration set for each project.
  foreach ($pt_config['projects'] as $uuid => $project) {

    // Get selected languages.
    $languages = array();
    if (!$project['no_language_selected']) {
      foreach ($project['languages'] as $d_lang => $pp_lang) {
        if (!empty($pp_lang)) {
          $languages[$d_lang] = $d_lang;
        }
      }
      $languages = array_unique($languages);
    }

    // Create a skos importer object.
    $rdf_filename = preg_replace('/\W+/', '', strtolower($project['label']));
    $rdf_filename = 'pp_project_' . $rdf_filename . '.rdf';
    $importer = skos_importer_importer($uuid);

    // Disable or remove skos importer configuration set if no language is
    // given.
    if (empty($languages)) {
      // Delete importer configuration set and the appropriate taxonomy.
      if ($project['remove_taxonomy']) {
        // $importer->delete();
      }
      // Disable the importer configuration set.
      else {
        /* if ($importer->export_type) {
          $importer->disabled = TRUE;
          $importer->save();
        } */
      }
      continue;
    }

    $si_config = array(
      'name' => $project['label'] . ' (' . $pt_config['title'] . ')',
      'description' => t('Automatically generated by the PowerTagging module'),
      'fetcher' => array(
        'plugin_key' => 'SKOSImporterHTTPFetcher',
        'fetchers' => array(
          'SKOSImporterFileFetcher' => array(
            'fid' => 0,
            'source' => '',
            'upload' => '',
          ),
          'SKOSImporterHTTPFetcher' => array(
            'source' => $http_source . 'fileName=' . $uuid . '/' . $rdf_filename,
          ),
        ),
      ),
      'parser' => array(
        'plugin_key' => 'SKOSImporterTermParser',
        'additional_data' => array(
          'altLabel' => 'altLabel',
          'hiddenLabel' => 0,
          'scopeNote' => 0,
          'related' => 0,
        ),
        // Import all ConceptSchemes and Concepts in one vocabulary.
        'import_type' => 1,
        'vocabulary_name' => $project['taxonomy_name'],
        'multi_language' => (count($languages) <= 1 ? 0 : 1),
        'languages' => $languages,
        'language_mapping' => $project['languages'],
      ),
      'processor' => array(
        'plugin_key' => 'SKOSImporterTermProcessor',
        // Update existing terms: TRUE.
        'update_existing' => 2,
      ),
      // Periodic import: Off.
      'import_period' => -1,
      // Process in background: FALSE.
      'process_in_background' => 0,
    );
    $importer->addConfig($si_config);
    $importer->save();
  }

  return TRUE;
}

/**
 * Callback function for the autocomplete field to search taxonomies.
 *
 * @param string $string
 *   The search string.
 */
function powertagging_autocomplete_taxonomies($string) {
  $taxonomy_names = array();
  $result = db_select('taxonomy_vocabulary', 'v')
    ->fields('v', array('name'))
    ->condition('name', '%' . db_like($string) . '%', 'LIKE')
    ->range(0, 10)
    ->orderBy('name')
    ->execute();

  foreach ($result as $row) {
    $taxonomy_names[$row->name] = check_plain($row->name);
  }

  drupal_json_output($taxonomy_names);
}

/**
 * Create a machine name from a given name.
 *
 * @param string $name
 *   Name of the skos importer.
 *
 * @return string
 *   The machine name.
 */
function powertagging_machine_name($name) {
  return substr(preg_replace('/[^a-z0-9]+/', '_', strtolower($name)), 0, 255);
}
