<?php

/**
 * @file
 * Ajax Facets Extra admin file.
 */

/**
 * Admin settings form.
 *
 * @param array $form
 *   Form.
 * @param array $form_state
 *   Form state.
 *
 * @return array
 *   Admin form.
 */
function ajax_facets_extra_settings_form($form, &$form_state) {
  $form['ajax_facets_extra_override_widget'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable sort support (override plugins)'),
    '#description' => t(
      'Allows Search API Sorts support by overriding default Ajax Facets widgets. However, you can still use dedicated Ajax Facets Extra widgets.'
    ),
    '#default_value' => variable_get('ajax_facets_extra_override_widget'),
  );

  $form = system_settings_form($form);

  $form['#submit'][] = 'ajax_facets_extra_settings_form_submit';

  return $form;
}

/**
 * Admin settings form.
 *
 * @param array $form
 *   Form.
 * @param array $form_state
 *   Form state.
 */
function ajax_facets_extra_settings_form_submit($form, &$form_state) {
  // Reset plugin cache to make sure new include files are picked up.
  ctools_include('plugins');
  ctools_get_plugins_reset();
}
