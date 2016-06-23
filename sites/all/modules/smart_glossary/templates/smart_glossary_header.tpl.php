<?php

/**
 * @file
 * The template for the header of the Smart Glossary
 *
 * available variables:
 * - $base_path string
 *     The base path for the glossary page
 * - $character_list array
 *     A list of all characters
 * - $current_language string
 *     The currently chosen language of the concepts
 */

$letters = array_keys($character_list);
$last_letter = end($letters);

?>
<div id="smart-glossary-header">
  <div id="smart-glossary-autocomplete">
    <input class="concept-autocomplete" autocomplete="off" title="<?php print t('Enter term name'); ?>" placeholder="<?php print t('Enter term name'); ?>" value="" type="text">
  </div>
  <div class="clearBoth"></div>

  <ul class="alphabet">
<?php foreach ($character_list as $char => $available): ?>
    <li class="letter floatLeft<?php if (!$available): ?> disabled<?php endif; ?><?php if ($char == $last_letter): ?> last<?php endif; ?>">
      <?php print l($char, $base_path . '/' . $current_language . '/' . $char); ?>
    </li>
<?php endforeach; ?>
    <div class="clearBoth"></div>
  </ul>
</div>
