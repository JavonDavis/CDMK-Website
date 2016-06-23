
-- SUMMARY --

This module presents a glossary (SKOS thesaurus) and its linked open data,
searchable and interactive on a relation browser.

-- REQUIREMENTS --

- To use this module you need to already have a SKOS-thesaurus via a
SPARQL-endpoint to access the data (e.g. PoolParty (http://www.poolparty.biz)).
- If you want to display additional linked open data, the linking has to be
already done in the thesaurus.
- The "Semantic Connector"-module (https://drupal.org/project/semantic_connector)
needs to be installed and enabled.

-- INSTALLATION --

1, Install the EasyRDF-Library
Download the EasyRDF-library (http://www.easyrdf.org/downloads) and add
EasyRdf.php and all the other files and folders to "sites/all/libraries/easyrdf".

2, Enable first the modules from the "Requirements"-list above and then the
PowerTagging module. See
https://drupal.org/documentation/install/modules-themes/modules-7 for further
information.

3, If you want to display a relation browser in the glossary, the VisualMapper
library has to be copied to "sites/all/libraries/visual_mapper". This library is
free to use but currently only available by contacting one of the this module's
maintainers. (a platform to download the library will be provided in the near
future)

-- USAGE --

- Configure the module at "admin/config/semantic-drupal/smart-glossary" or by
visiting Configuration -> Semantic Drupal -> Smart Glossary.
- Optional; If you have any concepts linked to data from DBpedia, feel free
to also activate the "Smart Glossary DBpedia"-sub-module.
- Optional; If you want to display any other linked open data, add it by
creating your own sub-module and using Smart Glossary's API.
