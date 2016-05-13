--------------------------------------------------------------------------------
  ajax_facets module Readme
  https://www.drupal.org/project/ajax_facets
--------------------------------------------------------------------------------

Contents:
=========
1. ABOUT
2. INSTALLATION
3. REQUIREMENTS
4. CONFIGURATION
5. CREDITS

1. ABOUT
========

This module allows you to create facet filters which working by AJAX.
Filters and search results will be updated by AJAX.

Widgets:

Ajax range slider
Ajax multiple checkboxes
Ajax selectbox
Ajax links

2. INSTALLATION
===============

Install as usual, see http://drupal.org/node/70151 for further information.
Contact me with any questions.

3. REQUIREMENTS
===============

This module requires:
Module views - http://drupal.org/project/views
Module facetapi - http://drupal.org/project/facetapi
Module search_api - http://drupal.org/project/search_api

This module was well tested with modules search_api_solr and search_api_db

4. CONFIGURATION
================

Can be a lot of cases, how to configure and use this module.
But the simplest way is the next:
1. Install modules search_api, facetapi, search_api_solr/search_api_db and search_api_views
2. Create and configure Search server in Drupal.
3. Create Search Index, configure it. Add some fields into index and select few of them as facets.
  Select one of four ajax widgets for your facets. Enable option "Update results by ajax" for your facet.
4. Create view based on your search index. Fastest way is select type of display "Page".
  Enable Ajax for your views display.
5. Put block of your facet on the page where will be placed your view with search results.
  Important - view should be processed earlier than facet block will be rendered. Otherwise facets will not work.
6. Put "Ajax facets block" block on the page if you want to have "Reset all facets" link.

Module has integration with HTML5 browser history API. If you want to get this feature work for HTML4 browsers
you should install libraries module (https://www.drupal.org/project/libraries)
and download history.js (https://github.com/browserstate/history.js) library.

5. CREDITS
==========

Project page: http://drupal.org/project/ajax_facets

Maintainers:
Eugene Ilyin - https://www.drupal.org/user/1767626
