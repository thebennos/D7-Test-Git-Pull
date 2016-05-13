<?php

/**
 * @file
 * Settings form.
 */

/**
 * Config form shown at admin/config/system/recacher
 */
function recacher_admin($form, &$form_state) {

  $default_recacher_max_urls = 'all';
  $form['recacher_max_urls'] = array(
    '#type' => 'textfield',
    '#title' => t('How many expired URLs to re-cache'),
    '#default_value' => variable_get('recacher_max_urls', $default_recacher_max_urls),
    '#size' => 3,
    '#maxlength' => 3,
    '#description' => t('Expired URLs are stored in a database table and at cron time they can be re-cached. This setting allows you to set the maximum number of URLs to re-cache. Set it to \'all\' if you wish to re-cache all expired URLs. Default: %default_recacher_max_urls', array('%default_recacher_max_urls' => check_plain($default_recacher_max_urls))),
    '#required' => FALSE,
  );

  $times = array();
  $intervals = array('default', 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597);
  $defaultcroninterval = $intervals[0];
  foreach ($intervals as $interval) {
    $times[] = $interval;
  }
  $form['recacher_cron_interval'] = array(
    '#type' => 'select',
    '#title' => t('Minimum time between re-caching'),
    '#default_value' => variable_get('recacher_cron_interval', $defaultcroninterval),
    '#description' => t('Re-caching is triggered by a cron job. Typically you would want to have this process run as often as possible so as to quickly warm recently expired caches. High frequency (even a minute, or less) has no downside, because the process will not do anything most of the time. (Of course, your cron has to be triggered at least at the same or higher frequency.) If you use Elysia Cron for more granular settings of your cron (recommended), set this value to "default". Specify the time in <em>minutes</em>. Default: %defaultcroninterval', array('%defaultcroninterval' => check_plain($defaultcroninterval))),
    '#options' => drupal_map_assoc($times),
    '#required' => TRUE,
  );

  $options = array('only unsuccessful', 'all', 'none');
  $defaultwatchdogreporting = $options[0];
  $form['recacher_watchdog'] = array(
    '#type' => 'select',
    '#title' => t('Watchdog reporting'),
    '#default_value' => variable_get('recacher_watchdog', $defaultwatchdogreporting),
    '#description' => t('Success of re-caching can be reported via watchdog. One can choose to only show URLs that failed to recache, or all URLs (useful while debugging), or switch off reporting completely. Default: %defaultwatchdog', array('%defaultwatchdog' => check_plain($defaultwatchdogreporting))),
    '#options' => drupal_map_assoc($options),
    '#required' => TRUE,
  );

  return system_settings_form($form);
}
