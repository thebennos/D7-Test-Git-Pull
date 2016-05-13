/**
 * @file
 * Ajax Facets Extra javascript file.
 */

(function ($) {
  'use strict';
  Drupal.behaviors.ajax_facets_extra = {
    attach: function (context, settings) {
      if (Drupal.settings.facetapi.isUrlJsExists) {
        // Sort links.
        $('.search-api-sorts a.sort-item:not(.processed)').bind('click', function (e) {
          e.preventDefault();

          if (e.target.href) {
            var parsed = url('?', e.target.href);
            Drupal.ajax_facets.queryState.sort = parsed.sort;
            Drupal.ajax_facets.queryState.order = parsed.order;
          }

          // Send ajax query with first found facet.
          Drupal.ajax_facets.sendAjaxQuery($('[data-facet-name]').first(), true);
        }).addClass('processed');

        if (Drupal.settings.facetapi['current_search']) {
          // Set current search block name.
          Drupal.ajax_facets.beforeAjaxCallbacks.current_search = function (context, settings, element) {
            Drupal.ajax_facets.queryState['current_search'] = Drupal.settings.facetapi.current_search;
          };

          // Current search links.
          $('.current-search-blocks-ajax-extra a:not(.processed)').bind('click', function (e) {
            e.preventDefault();

            // Clear all facets from queryState facets array.
            if (e.target.href) {
              var parsed = url('?', e.target.href);
              if (parsed['f']) {
                Drupal.ajax_facets.queryState.f = parsed.f;
              }
              else {
                // Clear all facets from queryState facets array.
                Drupal.ajax_facets.queryState.f = [];
              }
            }

            // Send ajax query with first found facet.
            Drupal.ajax_facets.sendAjaxQuery($('[data-facet-name]').first(), true);
          }).addClass('processed');
        }
      }
    }
  };
})(jQuery);
