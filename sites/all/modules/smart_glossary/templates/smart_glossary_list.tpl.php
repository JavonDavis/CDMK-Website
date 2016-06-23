<?php

/**
 * @file
 * The template for the list view of the Smart Glossary
 *
 * available variables:
 * - $list array
 *     An array of filtered terms
 */

?>
<div id="smart-glossary-list">
  <?php if (empty($list)): ?>
    <p><?php print t('No terms found'); ?></p>
  <?php else: ?>
    <ul>
	<?php foreach($list as $term): ?>
      <li>
        <a href="<?php print $term->url; ?>"><?php print $term->prefLabel; ?></a>
		<?php if ($term->multiple && !empty($term->broader)): ?>
          <span class="term_text">(<?php foreach ($term->broader as $broader): ?><?php print $broader; ?><?php endforeach; ?>)</span>
        <?php endif; ?>
      </li>
    <?php endforeach; ?>
    </ul>
  <?php endif; ?>
</div>
