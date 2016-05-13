<?php

/**
 * @file
 * The facetapi_links and facetapi_checkbox_links widget plugin classes.
 */

/**
 * Widget that renders facets as a list of clickable links.
 *
 * Links make it easy for users to narrow down their search results by clicking
 * on them. The render arrays use theme_item_list() to generate the HTML markup.
 */
class FacetapiAjaxWidgetExtraCheckboxes extends FacetapiAjaxWidgetCheckboxes {
  /**
   * Overrides FacetapiWidget::settingsForm().
   */
  public function settingsForm(&$form, &$form_state) {
    parent::settingsForm($form, $form_state);

    $form['widget']['widget_settings']['ajax'][$this->id]['display_title'] = array(
      '#type' => 'checkbox',
      '#title' => t('Display title'),
      '#default_value' => $this->settings->settings['display_title'],
      '#description' => t('Display facet title in the block.'),
      '#states' => array(
        'visible' => array(
          'select[name="widget"]' => array('value' => $this->id),
        ),
      ),
    );
  }

  /**
   * Overrides FacetapiWidget::getDefaultSettings().
   */
  public function getDefaultSettings() {
    return array(
      'display_title' => TRUE,
      'soft_limit' => 20,
      'show_expanded' => 0,
      'update_results' => 1,
      'reset_link' => 0,
      'empty_behavior' => 'ajax_facets',
    );
  }

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
    $element = &$this->build[$this->facet['field alias']];

    // Sets each item's theme hook, builds item list.
    $this->setThemeHooks($element);
    $item_list = array(
      '#theme' => 'item_list',
      '#items' => $this->buildListItems($element),
      '#attributes' => $this->build['#attributes'],
    );

    $reset_link = '';
    if (!empty($this->settings->settings['show_reset_link']) && $this->settings->settings['show_reset_link']) {
      $reset_link = '<a class="reset-link" href="#">' . t('Reset filter') . '</a>';
    }

    // We cannot use drupal_html_id to save the same id for each facet.
    $wrapper_id = $this->build['#attributes']['id'] . '-wrapper';

    $title = '';
    if ($this->settings->settings['display_title']) {
      $title = '<h4 class="facetapi-block-title">' . $this->build['#title'] . '</h4>';
    }

    $element = array(
      '#markup' => '<div id="' . $wrapper_id . '">'
      . $title
      . $reset_link
      . render($item_list)
      . '</div>',
    );

    ajax_facets_add_ajax_js($this->facet);
    ajax_facets_extra_add_ajax_js($this->facet);
  }

}
