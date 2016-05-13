<?php

/**
 * @file
 * The facetapi_links and facetapi_checkbox_links widget plugin classes.
 */

/**
 * Widget that renders facets as a range of values.
 */
class FacetapiAjaxWidgetExtraRanges extends FacetapiAjaxWidgetRanges {
  /**
   * Implements FacetapiWidget::execute().
   *
   * Transforms the render array into something that can be themed by
   * theme_item_list().
   *
   * @see FacetapiWidgetLinks::setThemeHooks()
   * @see FacetapiWidgetLinks::buildListItems()
   */
  public function execute() {
    parent::execute();

    ajax_facets_extra_add_ajax_js($this->facet);
  }

}
