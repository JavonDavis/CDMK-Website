<?php

/**
 * @file
 * The template for the detail view of the Smart Glossary
 *
 * available variables:
 * - $base_path
 *     The base path for the glossary page
 * - $visual_mapper_available bool
 *     TRUE if the Visual Mapper exists, FALSE if not
 * - $term object
 *     The term-object
 * - $visual_mapper_settings string
 *     The settings for the Visual Mapper in json format
 * - $current_language string
 *     The currently chosen language of the concepts
 * - $rdf_url
 *     The url to the RDF data
 * - $endpoint_url
 *     The url to the SPARQL endpoint
 */

$uri = isset($term->uri) ? $term->uri : '';
$visual_mapper_args = array(
  'base_path' => $base_path,
  'visual_mapper_available' => $visual_mapper_available,
  'concept_uri' => $uri,
  'visual_mapper_settings' => $visual_mapper_settings,
  'current_language' => $current_language,
);

?>
<div id="smart-glossary-detail" vocab="http://www.w3.org/2004/02/skos/core#" typeof="Concept" about="<?php print $uri; ?>">
  <?php if ($rdf_url || $endpoint_url): ?>
    <div id="block-semantic-data" class="block block-semantic-data">
    <?php if ($endpoint_url): ?>
      <div class="get-endpoint"><a href="<?php print $endpoint_url; ?>" target="_blank"><?php print t('Go to SPARQL endpoint'); ?></a></div>
    <?php endif; ?>
    <?php if ($rdf_url): ?>
      <div class="get-rdf"><a href="<?php print $rdf_url; ?>" target="_blank"><?php print t('Get RDF'); ?></a></div>
    <?php endif; ?>
    </div>
  <?php endif; ?>
  <?php if (empty($term->prefLabel)): ?>
    <h2 property="prefLabel" lang="<?php print $term->language; ?>"><?php print $term->prefLabelDefault; ?></h2>
    <p><?php print t('No translation of this glossary term available in selected language'); ?></p>
  <?php else: ?>
    <h2 class="element-invisible" property="prefLabel" lang="<?php print $current_language; ?>"><?php print $term->prefLabel; ?></h2>
    <?php if (!empty($term->prefLabels)): ?>
      <div class="element-invisible">
      <?php foreach ($term->prefLabels as $lang => $label): ?>
        <span property="prefLabel" lang="<?php print $lang; ?>"><?php print $label; ?></span>
      <?php endforeach; ?>
      </div>
    <?php endif; ?>

    <!-- term synonyms -->
    <?php if (!empty($term->altLabels)): ?>
      <?php $count = count($term->altLabels); ?>
      <h3><?php print t('Synonyms'); ?></h3>
      <p class="synonyms">
        <?php for ($i = 0; $i < $count; $i++): ?>
          <span property="altLabel" lang="<?php print $current_language; ?>"><?php print $term->altLabels[$i]; ?></span><?php if ($i < ($count - 1)): ?>, <?php endif; ?>
        <?php endfor; ?>
      </p>
    <?php endif; ?>

    <!-- term definitions -->
    <div class="definitions">
      <?php if (empty($term->definitions)): ?>
        <?php print t('No definition available'); ?>
      <?php else: ?>
        <?php if (!empty($term->definitions['internal'])): ?>
          <div class="internal">
            <h3><?php print t('Definition'); ?></h3>
            <?php foreach ($term->definitions['internal'] as $definition): ?>
              <p class="definition" property="definition" lang="<?php print $current_language; ?>"><?php print $definition; ?></p>
            <?php endforeach; ?>
          </div>
        <?php endif; ?>
        <?php if (!empty($term->definitions['external'])): ?>
          <div class="external">
          <?php foreach ($term->definitions['external'] as $match_type => $content): ?>
            <div class="<?php print $match_type . ' ' . strtolower($content['source']); ?>">
            <h3><?php print $content['title']; ?></h3>
            <?php foreach ($content['resources'] as $resource): ?>
              <span class="source" property="http://www.w3.org/2004/02/skos/core#<?php print $match_type; ?>" resource="<?php print $resource['uri']; ?>">
                <p class="definition" about="<?php print $resource['uri']; ?>" property="http://dbpedia.org/ontology/abstract" lang="<?php print $current_language; ?>"><?php print $resource['definition']; ?></p>
                <?php print t('Source:'); ?>
                <a about="<?php print $resource['uri']; ?>" property="http://xmlns.com/foaf/0.1/isPrimaryTopicOf" href="<?php print $resource['url']; ?>" target="_blank">
                  <span about="<?php print $resource['uri']; ?>" property="http://www.w3.org/2000/01/rdf-schema#label" lang="<?php print $current_language; ?>"><?php print $resource['label']; ?></span>
                </a>
              </span>
            <?php endforeach; ?>
            </div>
          <?php endforeach; ?>
          </div>
        <?php endif; ?>
      <?php endif; ?>
    </div>

    <?php print theme('smart_glossary_visual_mapper', $visual_mapper_args); ?>

    <!-- semantic relations -->
    <?php if (!empty($term->related) || !empty($term->broader) || !empty($term->narrower)): ?>
      <div class="semantic-relations">
        <?php if (!empty($term->related)): ?>
          <div class="related">
            <h3><?php print t('Related terms'); ?></h3>
            <?php $count = count($term->related); ?>
            <?php for ($i = 0; $i < $count; $i++): ?>
              <a property="related" resource="<?php print $term->related[$i]['uri']; ?>" typeof="Concept" href="<?php print $term->related[$i]['url']; ?>">
                <span about="<?php print $term->related[$i]['uri']; ?>" property="prefLabel" lang="<?php print $current_language; ?>"><?php print $term->related[$i]['prefLabel']; ?></span></a><?php if ($i < ($count - 1)): ?>, <?php endif; ?>
            <?php endfor; ?>
          </div>
        <?php endif; ?>
		<?php if (!empty($term->broader)): ?>
          <div class="broader">
            <h3><?php print t('Broader terms'); ?></h3>
            <?php $count = count($term->broader); ?>
            <?php for ($i = 0; $i < $count; $i++): ?>
              <a property="broader" resource="<?php print $term->broader[$i]['uri']; ?>" typeof="Concept" href="<?php print $term->broader[$i]['url']; ?>">
                <span about="<?php print $term->broader[$i]['uri']; ?>" property="prefLabel" lang="<?php print $current_language; ?>"><?php print $term->broader[$i]['prefLabel']; ?></span></a><?php if ($i < ($count - 1)): ?>, <?php endif; ?>
            <?php endfor; ?>
          </div>
        <?php endif; ?>
		<?php if (!empty($term->narrower)): ?>
          <div class="narrower">
            <h3><?php print t('Narrower terms'); ?></h3>
            <?php $count = count($term->narrower); ?>
            <?php for ($i = 0; $i < $count; $i++): ?>
              <a property="narrower" resource="<?php print $term->narrower[$i]['uri']; ?>" typeof="Concept" href="<?php print $term->narrower[$i]['url']; ?>">
                <span about="<?php print $term->narrower[$i]['uri']; ?>" property="prefLabel" lang="<?php print $current_language; ?>"><?php print $term->narrower[$i]['prefLabel']; ?></span></a><?php if ($i < ($count - 1)): ?>, <?php endif; ?>
            <?php endfor; ?>
          </div>
        <?php endif; ?>
      </div>
    <?php endif; ?>
  <?php endif; ?>
</div>
