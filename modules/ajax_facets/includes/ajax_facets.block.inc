<?php

/**
 * @file
 * Functions, related with block API.
 */

/**
 * Implements hook_block_info().
 */
function ajax_facets_block_info() {
  return array(
    'ajax_facets_block' => array(
      'info' => t('Ajax facets block'),
    ),
  );
}

/**
 * Implements hook_block_configure().
 */
function ajax_facets_block_configure($delta = '') {
  $form = array();

  switch ($delta) {
    case 'ajax_facets_block':
      $form['ajax_facets_reset_all_link_text'] = array(
        '#type' => 'textfield',
        '#title' => t('"Reset all facets" link text'),
        '#default_value' => ajax_facets_reset_link_text(),
      );

      break;
  }

  return $form;
}

/**
 * Implements hook_block_save().
 */
function ajax_facets_block_save($delta = '', $edit = array()) {
  switch ($delta) {
    case 'ajax_facets_block':
      variable_set('ajax_facets_reset_all_link_text', $edit['ajax_facets_reset_all_link_text']);
      // Update the i18n string if need.
      if (function_exists('i18n_string_update')) {
        $name = array(
          'ajax_facets',
          'ajax_facets_block',
          'facet_reset_all_link_text',
          'text'
        );
        i18n_string_update($name, $edit['ajax_facets_reset_all_link_text']);
      }
      break;
  }
}

/**
 * Implements hook_block_view().
 */
function ajax_facets_block_view($delta = '') {
  $block = array();

  switch ($delta) {
    case 'ajax_facets_block':
      $block['content'] = array(
        '#markup' => l(ajax_facets_reset_link_text(), current_path(), array(
          'attributes' => array(
            'class' => array(
              'ajax-facets-reset-all-link',
            ),
          ),
        )),
      );

      break;
  }

  return $block;
}

/**
 * Returns text for Ajax Facets reset link.
 */
function ajax_facets_reset_link_text() {
  $text = variable_get('ajax_facets_reset_all_link_text', t('Reset all facets'));
  if (function_exists('i18n_string_translate')) {
    $text = i18n_string_translate(
      array(
        'ajax_facets',
        'ajax_facets_block',
        'facet_reset_all_link_text',
        'text'
      ),
      $text
    );
  }
  return $text;
}