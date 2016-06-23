<?php
/**
 * @file
 * The template for the Visual Mapper
 *
 * available variables:
 * - $base_path string
 *     The base path for the glossary page
 * - $visual_mapper_available bool
 *     TRUE if the Visual Mapper exists, FALSE if not
 * - $concept_uri string
 *     The URI of a concept
 * - $visual_mapper_settings string
 *     The settings for the Visual Mapper in json format
 * - $current_language string
 *     The currently chosen language of the concepts
 */

global $base_url, $language;

?>
<?php if ($visual_mapper_available): ?>
  <script type="text/javascript">
    var settings = <?php print $visual_mapper_settings; ?>;
    var glossaryUrl = "<?php print ($base_url . '/' . (!empty($language->prefix) ? $language->prefix . '/' : '') . $base_path . '/' . $current_language); ?>";

    // Event listeners.
    var listeners = {
      "conceptLoaded" : []
    };

    var visualMapper = jQuery("#smart-glossary-detail").initVisualMapper(settings, listeners);
    visualMapper.load("<?php print $base_url; ?>/smart-glossary/get-visual-mapper-data/<?php print $base_path; ?>", "<?php print $concept_uri; ?>", "<?php print $current_language; ?>");
  </script>
<?php endif; ?>
