<?php
/**
 * @file
 * Better comment settings form.
 */

/**
 * Better comments settings form.
 */
function better_comments_settings() {
  $form['better_comments_setting'] = array(
    '#type' => 'fieldset',
    '#title' => t('Better Comment Settings'),
    '#description' => t('To allow a user to delete his own comments, Set the <a href="!url">permissions</a> here.', array('!url' => url('admin/people/permissions'))),
  );
  $form['settings']['better_comments_subject'] = array(
    '#type' => 'checkbox',
    '#options' => array(
      '1' => t('Yes'),
      '0' => t('No'),
    ),
    '#title' => t('Disable Comment subject textbox'),
    '#description' => t('This disables subject box. If not selected the subject textbox is enable by default.'),
    '#default_value' => variable_get('better_comments_subject', 0),
  );
  $form['settings']['better_comments_author'] = array(
    '#type' => 'checkbox',
    '#options' => array(
      '1' => t('Yes'),
      '0' => t('No'),
    ),
    '#title' => t('Disable User name textbox'),
    '#description' => t('This disable the user name textbox. If not selected the subject textbox is enable by default.'),
    '#default_value' => variable_get('better_comments_author', 0),
  );
  $form['settings']['better_comments_body_title'] = array(
    '#title' => t('Remove the Comment body title.'),
    '#type' => 'checkbox',
    '#description' => t('Remove the title along with  start showing above the comment text box.'),
    '#default_value' => variable_get('better_comments_body_title', 0),
  );
  $form['settings']['better_comments_picture'] = array(
    '#title' => t('Show user picture beside comment box'),
    '#type' => 'checkbox',
    '#description' => t('Show login user or default picture in the comment box and comment display. Note: This field requires the Default user picture to be set in <a href="!url">Accounts Settings</a>', array('!url' => url('admin/config/people/accounts'))),
    '#default_value' => variable_get('better_comments_picture', 0),
  );
  $form['settings']['better_comments_text_placeholder'] = array(
    '#title' => t('Comment box placeholder text.'),
    '#type' => 'textfield',
    '#description' => t('Leave empty to display the default placeholder, use @none to remove the default placeholder or add your own text here.', array('@none' => '<none>')),
    '#default_value' => variable_get('better_comments_text_placeholder'),
  );
  $form['settings']['better_comments_text_filters'] = array(
    '#title' => t('Disable text filter format options below comment box.'),
    '#type' => 'checkbox',
    '#description' => t('Disables the text format box below comment form. Note: After disabling the text format box, the comment form uses the default formats assigned in each content type comment settings.'),
    '#default_value' => variable_get('better_comments_text_filters', 0),
  );
  $form['settings']['better_comments_preview'] = array(
    '#title' => t('Disable the Preview button.'),
    '#type' => 'checkbox',
    '#description' => t('Disable the preview button beside the submit comment button.'),
    '#default_value' => variable_get('better_comments_preview', 0),
  );
  $form['settings']['better_comments_picture']['#after_build'][] = 'better_comments_validate_image';
  return system_settings_form($form);
}

/**
 * Validate if default user picture is set.
 */
function better_comments_validate_image($element) {
  if (variable_get('user_picture_default') == FALSE) {
    $element['#attributes'] = array('disabled' => 'disabled');
  }
  return $element;
}
