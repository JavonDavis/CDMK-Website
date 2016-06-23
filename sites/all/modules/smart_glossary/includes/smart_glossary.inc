<?php

/**
 * @file
 * Callbacks and theme-functions used by Smart Glossary
 */

/**
 * Menu callback for the Smart Glossary start page.
 *
 * @param string $glossary_language
 *   The iso-code of the current language
 *
 * @return string
 *   The generated html-code
 */
function _smart_glossary_start($glossary_language = NULL) {
  _smart_glossary_set_language($glossary_language);

  $html = _smart_glossary_header_theme();
  $html = $html . _smart_glossary_start_theme();
  drupal_add_css(drupal_get_path('module', 'smart_glossary') . '/css/smart_glossary.css');

  return $html;
}

/**
 * Menu callback for showing either a detail view or a list view.
 *
 * @param string $character
 *   Character (a-z), "all" or the label of a concept
 * @param string $glossary_language
 *   The iso-code of the current language
 *
 * @return string
 *   The generated html-code
 */
function _smart_glossary_view($character, $glossary_language = NULL) {
  $query = drupal_get_query_parameters();
  $config = smart_glossary_current_config_load();
  $variables['sg_id'] = $config->sg_id;

  _smart_glossary_set_language($glossary_language);
  $glossary_language = _smart_glossary_get_current_language($variables);
  $chars = range('a', 'z');

  if (isset($query['uri'])) {
    $output = _smart_glossary_detail_view($query['uri'], $glossary_language);
  }
  elseif (in_array($character, $chars)) {
    $output = _smart_glossary_list($character, $glossary_language);
  }
  else {
    drupal_goto($config->base_path);
    exit();
  }
  drupal_add_css(drupal_get_path('module', 'smart_glossary') . '/css/smart_glossary.css');

  return (($output !== FALSE) ? array(
    '#type' => 'markup',
    '#markup' => $output,
    '#contextual_links' => array(
      'smart_glossary' => array(
        'admin/config/semantic-drupal/smart-glossary',
        array(str_replace('smart_glossary_', '', $config->sg_id)),
      ),
    ),
  ) : '');
}

/**
 * Menu callback to search for concepts starting with a specific character.
 *
 * @param string $character
 *   Character (a-z), "all" or the label of a concept
 * @param string $glossary_language
 *   The iso-code of the current language
 *
 * @return string
 *   The generated html-code or FALSE in case of an error
 */
function _smart_glossary_list($character, $glossary_language) {
  _smart_glossary_set_language($glossary_language);

  $config = smart_glossary_current_config_load();
  $sg = SmartGlossary::getInstance($config);

  // Get all concepts.
  if ($character == 'all') {
    $list = $sg->getList('', $glossary_language);
    $html = _smart_glossary_header_theme();
    $html = $html . _smart_glossary_list_theme(array('list' => $list));
    drupal_set_title(t('All Terms'));
    _smart_glossary_set_breadcrumb();
    return $html;
  }
  // Get concepts of a single character.
  elseif (preg_match("/^[a-z]$/", $character)) {
    $list = $sg->getList($character, $glossary_language);
    $html = _smart_glossary_header_theme(array('character' => $character));
    $html = $html . _smart_glossary_list_theme(array('list' => $list));
    drupal_set_title(t('Terms with %character', array('%character' => strtoupper($character))), PASS_THROUGH);
    _smart_glossary_set_breadcrumb();
    return $html;
  }

  drupal_goto($config->base_path);
  exit();
}

/**
 * Menu callback for the glossary detail view (with the relation browser).
 *
 * @param string $concept_uri
 *   The uri of the concept
 * @param string $glossary_language
 *   The iso-code of the current language
 *
 * @return string
 *   The generated html-code or FALSE in case of an error
 */
function _smart_glossary_detail_view($concept_uri, $glossary_language) {
  _smart_glossary_set_language($glossary_language);

  $config = smart_glossary_current_config_load();
  $sg = SmartGlossary::getInstance($config);
  $concept = $sg->getConcept($concept_uri, $glossary_language);

  if (is_null($concept)) {
    drupal_set_message(t('Term with URI "@uri" not found.', array('@uri' => $concept_uri)), 'error');
    drupal_goto($config->base_path);
  }

  $label = isset($concept->prefLabel) ? $concept->prefLabel : $concept->prefLabelDefault;
  drupal_set_title($label);
  _smart_glossary_set_breadcrumb();
  $html = _smart_glossary_header_theme() . _smart_glossary_detail_view_theme(array('term' => $concept));

  return $html;
}

/**
 * AJAX callback for the glossary concept autocomplete.
 */
function _smart_glossary_concept_autocomplete($base_path) {
  $base_path = str_replace('|', '/', $base_path);
  $query = drupal_get_query_parameters();
  if (isset($query['term'])) {
    $config = smart_glossary_current_config_load($base_path);
    $sg = SmartGlossary::getInstance($config);
    $list = $sg->autocomplete($query['term'], 15, $_SESSION['smart_glossary_language'][$config->sg_id]);
  }
  else {
    $list = array();
  }

  echo drupal_json_encode($list);
  exit;
}

/**
 * Theme function of the glossary header.
 *
 * @param array $variables
 *   Array of variables in the theme
 *
 * @return string
 *   The themed string
 */
function _smart_glossary_header_theme($variables = array()) {
  global $base_url;

  $config = smart_glossary_current_config_load();
  $variables['sg_id'] = $config->sg_id;

  // Get possible themes from other module's hooks.
  $themes = module_invoke_all('smart_glossary_header_theme');
  $language = _smart_glossary_get_current_language($variables);

  if (!empty($themes)) {
    $theme = reset($themes);
  }
  else {
    $theme = 'smart_glossary_header';

    // Add javascript libraries and javascript settings.
    drupal_add_library('system', 'ui.autocomplete');
    drupal_add_js(drupal_get_path('module', 'smart_glossary') . '/js/smart_glossary.js');
    drupal_add_js(array(
      'smart_glossary' => array(
        'module_path' => $base_url . '/' . drupal_get_path('module', 'smart_glossary') . '/',
        'base_path' => $config->base_path,
      ),
    ), 'setting');

    // Add visual mapper if available.
    if (_smart_glossary_visual_mapper_exists()) {
      drupal_add_js(libraries_get_path('d3js') . '/d3.min.js');
      drupal_add_js(libraries_get_path('visual_mapper') . '/visual_mapper.min.js');
      drupal_add_css(libraries_get_path('visual_mapper') . '/visual_mapper.css');
    }
  }

  if (!isset($config->advanced_settings['char_a_z']) || empty($config->advanced_settings['char_a_z'])) {
    _smart_glossary_update_character_list($config);
  }
  $char_a_z = $config->advanced_settings['char_a_z'];
  $char_a_z = isset($char_a_z[$language]) ? $char_a_z[$language] : array();

  if (empty($char_a_z)) {
    $chars = range('a', 'z');
    foreach ($chars as $char) {
      $char_a_z[$char] = 0;
    }
  }

  return theme($theme,
    array(
      'base_path' => $config->base_path,
      'character_list' => $char_a_z,
      'current_language' => $language,
    )
  );
}


/**
 * Theme function of the glossary start page.
 *
 * @param array $variables
 *   Array of variables in the theme
 *
 * @return string
 *   The themed string
 */
function _smart_glossary_start_theme($variables = array()) {
  global $language;

  // Get possible themes from other module's hooks.
  $themes = module_invoke_all('smart_glossary_start_theme');
  $config = smart_glossary_current_config_load();
  $variables['sg_id'] = $config->sg_id;
  $current_language = _smart_glossary_get_current_language($variables);

  return theme(
    ((!empty($themes)) ? reset($themes) : 'smart_glossary_start'),
    array(
      'base_path' => $config->base_path,
      'visual_mapper_available' => _smart_glossary_visual_mapper_exists() & (!isset($config->visual_mapper_settings['enabled']) || $config->visual_mapper_settings['enabled']),
      'visual_mapper_settings' => _smart_glossary_build_visual_mapper_config($config, $language->language),
      'current_language' => $current_language,
    )
  );
}

/**
 * Theme function of the Smart Glossary list.
 *
 * @param array $variables
 *   Array of variables in the theme
 *
 * @return string
 *   The themed string
 */
function _smart_glossary_list_theme($variables = array()) {
  // Get possible themes from other module's hooks.
  $themes = module_invoke_all('smart_glossary_list_theme');

  return theme(
    ((!empty($themes)) ? reset($themes) : 'smart_glossary_list'),
    array(
      'list' => ((isset($variables['list'])) ? $variables['list'] : NULL),
    )
  );
}

/**
 * Theme function of the glossary detail view.
 *
 * @param array $variables
 *   Array of variables in the theme
 *
 * @return string
 *   The themed string
 */
function _smart_glossary_detail_view_theme($variables = array()) {
  global $language;

  // Get possible themes from other module's hooks.
  $themes = module_invoke_all('smart_glossary_detail_view_theme');
  $config = smart_glossary_current_config_load();
  $variables['sg_id'] = $config->sg_id;
  $current_language = _smart_glossary_get_current_language($variables);
  $rdf_url = $config->advanced_settings['add_rdf_link'] ? current_path() . '.rdf?uri=' . $_GET['uri'] : '';
  $endpoint_url = $config->advanced_settings['add_endpoint_link'] ? $config->connection->getUrl() : '';

  return theme(
    ((!empty($themes)) ? reset($themes) : 'smart_glossary_detail'),
    array(
      'base_path' => $config->base_path,
      'visual_mapper_available' => _smart_glossary_visual_mapper_exists(),
      'term' => ((isset($variables['term'])) ? $variables['term'] : NULL),
      'visual_mapper_settings' => _smart_glossary_build_visual_mapper_config($config, $language->language),
      'current_language' => $current_language,
      'rdf_url' => $rdf_url,
      'endpoint_url' => $endpoint_url,
    )
  );
}

/**
 * Get the data for the Visual Mapper inside a Smart Glossary.
 *
 * @param string $glossary_base_path
 *   The base-path to the glossary of choice.
 */
function _smart_glossary_get_visual_mapper_data($glossary_base_path) {
  global $language;

  $config = smart_glossary_current_config_load($glossary_base_path);
  if (isset($_GET['lang']) && !empty($_GET['lang'])) {
    $glossary_language = $_GET['lang'];
  }
  else {
    $language_mapping = $config->language_mapping;
    if (isset($language_mapping[$language->language]) && !empty($language_mapping[$language->language]['glossary_languages'])) {
      $glossary_languages = $language_mapping[$language->language]['glossary_languages'];
      $glossary_language = $glossary_languages[0];
    }
    else {
      $glossary_language = language_default('language');
    }
  }

  $sg = SmartGlossary::getInstance($config);
  $sg->getVisualMapperData(
    isset($_GET['uri']) && !empty($_GET['uri']) ? $_GET['uri'] : NULL,
    $glossary_language,
    isset($_GET['output']) && !empty($_GET['output']) ? $_GET['output'] : 'json'
  );
}

/**
 * Build the JSON config required in the Visual Mapper.
 *
 * @param object $smart_glossary_config
 *   The module configuration to build the Visual Mapper config for.
 * @param string $language
 *   The ISO code of the language to use.
 *
 * @return string
 *   The Visual Mapper config in a JSON format.
 */
function _smart_glossary_build_visual_mapper_config($smart_glossary_config, $language) {
  $visual_mapper_config = array();
  if (isset($smart_glossary_config->visual_mapper_settings) && is_array($smart_glossary_config->visual_mapper_settings)) {
    $visual_mapper_config = array_merge($smart_glossary_config->visual_mapper_settings, array('wording' => $smart_glossary_config->language_mapping[$language]['wording']));

    // Add the relations.
    $visual_mapper_config['relations'] = array(
      'parents' => array(
        'colors' => array(
          'bright' => $visual_mapper_config['brightColors']['parent'],
          'dark' => $visual_mapper_config['darkColors']['parent'],
        ),
        'wording' => array(
          'legend' => $visual_mapper_config['wording']['legendParent'],
        ),
      ),
      'children' => array(
        'colors' => array(
          'bright' => $visual_mapper_config['brightColors']['children'],
          'dark' => $visual_mapper_config['darkColors']['children'],
        ),
        'wording' => array(
          'legend' => $visual_mapper_config['wording']['legendChildren'],
        ),
      ),
      'related' => array(
        'colors' => array(
          'bright' => $visual_mapper_config['brightColors']['related'],
          'dark' => $visual_mapper_config['darkColors']['related'],
        ),
        'wording' => array(
          'legend' => $visual_mapper_config['wording']['legendRelated'],
        ),
      ),
    );
  }

  return json_encode($visual_mapper_config);
}
