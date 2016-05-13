CONTENTS OF THIS FILE
---------------------
 * Introduction
 * Requirements
 * Installation
 * Usage

INTRODUCTION
------------
This module helps adding a custom form field comprising chain of dependent 
select fields that can be plugged into Drupal forms pro-grammatically. 
This module is for developers only.


REQUIREMENTS
------------
This module does not require any additional module as pre-requisites


INSTALLATION
------------
Install as you would normally install a contributed Drupal module. See:
https://drupal.org/documentation/install/modules-themes/modules-7
for further information.


USAGE
-----

Fieldset Configuration
----------------------

  * #type: (String) Should be set to 'ajax_chain_select'.
  * #title: (String) The title of the fieldset. Default is empty string.
  * #config: (2-D Array) Select fields configuration (explained below).
  * #required_levels: (Integer) Till which level the selection is mandatory. 
    E.g: if #required_levels is set to 2, the first 2 levels will be mandatory
    and rest will be optional. Default is 0 (zero).
  * #show_throbber: (Boolean) Whether to show throbber or not. Default is TRUE.
  * #progress_message: (String) The message to be shown with throbber.
    Default is 'Please wait..'
  * #data_callback: (String) A valid function name that takes the previous 
    level selection and returns an array of next level elements. 
    Refer to example module for better understanding.

E.g:

$form['region'] = array(
    '#type' => 'ajax_chain_select',
    '#title' => t('Region'),
    '#config' => $config,
    '#required_levels' => 3,
    '#show_throbber' => TRUE,
    '#progress_message' => t('Please wait..'),
    '#data_callback' => 'my_data_callback',
  );

Select Fields Configuration
---------------------------
Select fields configuration can be set using a 2-D array, where key represent
the select field name and the value array represent its configuration as
follows:

  * #title:: (String) The title of the select field. Default is empty string.
  * #title_display: (String) The tile display of select field.
    Default is 'before'.
  * #empty_option: (String) The empty option to be shown when no value is
    selected. Default is '- Select -'.
  * #default_value: (Integer) The value for pre-selection. Default is NULL.

E.g:

$config = array(
    'country' => array(
      '#title' => t("Country"),
      '#empty_option' => 'Select Country',
      '#default_value' => 1,
      '#title_display' => 'before',
    ),
    'state' => array(
      '#title' => t("State"),
    ),
);
