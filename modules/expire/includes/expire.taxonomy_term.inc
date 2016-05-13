<?php

/**
 * @file
 * Provides class that expires taxonomy terms.
 */

class ExpireTaxonomyTerm implements ExpireInterface {

  /**
   * Executes expiration actions for taxonomy term.
   *
   * @param $taxonomy_term
   *   Taxonomy term object.
   *
   * @param $action
   *   Action that has been executed.
   *
   * @param $skip_action_check
   *   Shows whether should we check executed action or just
   *   expire taxonomy term.
   */
  function expire($taxonomy_term, $action, $skip_action_check = FALSE) {

    if (empty($taxonomy_term->tid) || empty($taxonomy_term->vocabulary_machine_name)) {
      return;
    }

    // See if cache settings was overridden for this
    // taxonomy_term vocabulary_machine_name.
    $settings_overridden = variable_get('expire_taxonomy_term_override_defaults_' . $taxonomy_term->vocabulary_machine_name);

    $variable_suffix = '';
    if (!empty($settings_overridden)) {
      // If page cache settings was overridden for this taxonomy_term
      // vocabulary_machine_name we should add
      // "[taxonomy_term-VOCABULARY_MACHINE_NAME]" to every variable name
      // we use here.
      $variable_suffix = '_' . $taxonomy_term->vocabulary_machine_name;
    }

    $enabled_actions = variable_get('expire_taxonomy_term_actions' . $variable_suffix, array());
    $enabled_actions = array_filter($enabled_actions);

    // Stop further expiration if executed action is not selected by admin.
    if (!in_array($action, $enabled_actions) && !$skip_action_check) {
      return;
    }

    $expire_urls = array();

    // Expire front page.
    $expire_front_page = variable_get('expire_taxonomy_term_front_page' . $variable_suffix, EXPIRE_TAXONOMY_TERM_FRONT_PAGE);
    if ($expire_front_page) {
      $expire_urls = ExpireAPI::getFrontPageUrls();
    }

    // Expire taxonomy_term page.
    $expire_taxonomy_term_page = variable_get('expire_taxonomy_term_taxonomy_term_page' . $variable_suffix, EXPIRE_TAXONOMY_TERM_TAXONOMY_TERM_PAGE);
    if ($expire_taxonomy_term_page) {
      $expire_urls['taxonomy_term-' . $taxonomy_term->tid] = 'taxonomy/term/' . $taxonomy_term->tid;
    }

    // Expire taxonomy_term reference's pages.
    $expire_taxonomy_term_references = variable_get('expire_taxonomy_term_reference_pages' . $variable_suffix, EXPIRE_TAXONOMY_TERM_REFERENCE_PAGES);
    if ($expire_taxonomy_term_references) {
      $urls = ExpireAPI::expireTermPages($taxonomy_term, 'taxonomy_term');
      $expire_urls = array_merge($expire_urls, $urls);
      if (module_exists('node_reference') || module_exists('user_reference') || module_exists('entityreference')) {
        $traverse_field_collection = module_exists('field_collection') && variable_get('expire_taxonomy_term_reference_field_collection_pages' . $variable_suffix, EXPIRE_TAXONOMY_TERM_REFERENCE_FC_PAGES);
        $urls = ExpireAPI::expireReferences($taxonomy_term, 'taxonomy_term', $traverse_field_collection);
        $expire_urls = array_merge($expire_urls, $urls);
      }
    }

    // Expire custom pages.
    $expire_custom = variable_get('expire_taxonomy_term_custom' . $variable_suffix, EXPIRE_TAXONOMY_TERM_CUSTOM);
    if ($expire_custom) {
      $pages = variable_get('expire_taxonomy_term_custom_pages' . $variable_suffix);
      $urls = ExpireAPI::expireCustomPages($pages, array('taxonomy_term' => $taxonomy_term));
      $expire_urls = array_merge($expire_urls, $urls);
    }

    // Flush page cache for expired urls.
    ExpireAPI::executeExpiration($expire_urls, 'taxonomy_term', $taxonomy_term);
  }

}
