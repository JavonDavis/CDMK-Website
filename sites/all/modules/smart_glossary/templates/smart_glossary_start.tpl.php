<?php

/**
 * @file
 * The template for the start page of the Smart Glossary
 *
 * available variables:
 * - $base_path string
 *     The base path for the glossary page
 * - $visual_mapper_available bool
 *     TRUE if the Visual Mapper exists, FALSE if not
 * - $visual_mapper_settings string
 *     The settings for the Visual Mapper in json format
 * - $current_language string
 *     The currently chosen language of the concepts
 */

$visual_mapper_args = array(
  'base_path' => $base_path,
  'visual_mapper_available' => $visual_mapper_available,
  'concept_uri' => '',
  'visual_mapper_settings' => $visual_mapper_settings,
  'current_language' => $current_language,
);
?>
<div id="smart-glossary-detail">
<?php print theme('smart_glossary_visual_mapper', $visual_mapper_args); ?>
</div>
