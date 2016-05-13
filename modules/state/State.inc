<?php
/**
 * @file
 *  Drupal State module
 */

class State {

  /**
   * Form expiration in seconds.
   *
   * The value cannot be retrieved from sources like $GLOBALS['conf'],
   * since it's just a hardcoded local value of form_set_cache().
   * Btw; form_set_cache()'s value is the same as DRUPAL_MAXIMUM_TEMP_FILE_AGE.
   *
   * ~ 6 hours.
   *
   * @type integer
   */
  const FORM_EXPIRE = 21600;

  /**
   * Default server life, for overriding session.gc_maxlifetime.
   *
   * ~ 30 minutes.
   *
   * @type integer
   */
  const SERVER_LIFE_DEFAULT = 1800;

  /**
   * Default session check interval.
   *
   * ~ 5 minutes.
   *
   * @type integer
   */
  const CHECK_INTERVAL_DEFAULT = 300;

  /**
   *  List of name-spaced context types, for setting/getting session variables:
   *  - system: name of a system context
   *  - module: name of a module
   *  - application: name of concept that transcends themes and modules
   *
   * @var array $_contextTypes
   */
  protected static $_contextTypes = array(
    'module' => 'module',
    'application' => 'application',
    'system' => 'system',
  );

  /**
   *  Values:
   *  - 0: none
   *  - 1: checks session/form expiration
   *  - 2: prolongs session, when visitor is active (mousemove etc.)
   *  - 3: prolongs session unconditionally
   *
   * @var integer|boolean $_prolongation
   */
  protected static $_prolongation = FALSE;

  /**
   * @var integer|boolean $_prolongSuggestion
   */
  protected static $_prolongSuggestion = -1;

  /**
   * @var integer $_checkSession
   */
  protected static $_checkInterval = 0;

  /**
   * Auto-redirect - dont wait till user closes dialog -
   * upon session/form expiration.
   *
   * @var boolean $_autoRedirect
   */
  protected static $_autoRedirect = FALSE;

  /**
   * @var string $_redirectTarget
   */
  protected static $_redirectTarget = '';

  /**
   * @var boolean $_noFormExpire
   */
  protected static $_noFormExpire = FALSE;

  /**
   * @var boolean|null $_countRequests
   */
  protected static $_countRequests;

  /**
   * @var array $_nonPagePaths
   */
  protected static $_nonPagePaths = array(
    'system/ajax/',
    'file/ajax/',
    'file/progress/',
  );

  /**
   * @var boolean|null $_isPageRequest
   */
  protected static $_isPageRequest;

  /**
   * @var integer $_pageRequests
   */
  protected static $_pageRequests = -1;

  /**
   * @var integer $_nonPageRequests
   */
  protected static $_nonPageRequests = -1;

  /**
   * @var integer $_loggedInPageRequests
   */
  protected static $_loggedInPageRequests = -1;

  /**
   * @var integer $_pageResponses
   */
  protected static $_pageResponses = -1;

  /**
   * Nobody but the user module should destroy a session
   * (and that only at request processing start),
   * so we assume that a session will never be destroyed later.
   *
   * @var boolean $_started
   */
  protected static $_started = FALSE;

  /**
   * @see State::cookieDefaults()
   * @var array|null $_cookieDefaults
   */
  protected static $_cookieDefaults;

  /**
   * @var array $_cookiesSetInRequest
   */
  protected static $_cookiesSetInRequest = array();

  /**
   * @var array $_cookiesRemovedInRequest
   */
  protected static $_cookiesRemovedInRequest = array();

  /**
   * Set session variable, with low risk of collision.
   *
   * Starts drupal session, if no $_SESSION.
   *
   * Cannot set null as value.
   * Last non-null argument is considered value.
   *
   *  Breaks if something else has set a non-array session variable
   *  for one of the name spaces, like:
   *  - $_SESSION['system'] = non-array
   *  - $_SESSION['module'] = non-array
   *  - $_SESSION['application'] = non-array
   *
   * @throws Exception
   *   Always caught and logged.
   *   If bad argument.
   *   If too few non-null arguments (requires 3 for name-spaced context types,
   *   2 for other contexts).
   *   If session started and $_SESSION isnt array.
   *   If $context_type is falsy.
   *   If $context_type is name-spaced and $_SESSION[$context_type]
   *   exists and isnt array.
   *   If $context_type is name-spaced and arg $context_name used
   *   and is numeric.
   * @param string|array $context_type
   *   String, name-spaced value: system|module|application.
   *   String, non-namespaced value: anything else, like 'theme'.
   *   Array: all arguments.
   * @param mixed $context_name
   *   Default: empty string.
   *   Empty or numeric not allowed if name-spaced $context_type
   *   (system|module|application)
   * @param mixed $key,...
   *   Any number of arguments.
   *   Default: no argument.
   *   Last argument is considered value of the session array being set.
   * @return boolean|null
   *   Null: session not started, and could not be started.
   *   False on error.
   */
  public static function sessionSet($context_type, $context_name = '', $key = NULL/*,...*/) {
    return self::_access(10, is_array($context_type) ? $context_type : func_get_args());
  }

  /**
   * Increase session variable, with low risk of collision.
   *
   * Sets the session var using the increment value, if the var doesnt exist.
   *
   * Starts drupal session, if no $_SESSION.
   *
   * Last non-null argument is considered the increment value.
   *
   *  Breaks if something else has set a non-array session variable
   *  for one of the name spaces, like:
   *  - $_SESSION['system'] = non-array
   *  - $_SESSION['module'] = non-array
   *  - $_SESSION['application'] = non-array
   *
   * @throws Exception
   *   Always caught and logged.
   *   If bad argument.
   *   If too few non-null arguments (requires 3 for name-spaced context types,
   *   2 for other contexts).
   *   If session started and $_SESSION isnt array.
   *   If $context_type is falsy.
   *   If $context_type is name-spaced and $_SESSION[$context_type] exists
   *   and isnt array.
   *   If $context_type is name-spaced and arg $context_name used
   *   and is numeric.
   * @param string|array $context_type
   *   String, name-spaced value: system|module|application.
   *   String, non-namespaced value: anything else, like 'theme'.
   *   Array: all arguments.
   * @param mixed $context_name
   *   Default: empty string.
   *   Empty or numeric not allowed if name-spaced $context_type
   *   (system|module|application).
   * @param mixed $key,...
   *   Any number of arguments.
   *   Default: no argument.
   *   Last argument is considered the increment value
   *   of the session array being set.
   * @return number|null|boolean
   *   Number: the value after increment.
   *   Null: session not started, and could not be started.
   *   False on error.
   */
  public static function sessionIncrease($context_type, $context_name = '', $key = NULL/*,...*/) {
    return self::_access(11, is_array($context_type) ? $context_type : func_get_args());
  }

  /**
   * Get session variable, with low risk of collision.
   *
   *  Breaks if something else has set a non-array session variable
   *  for one of the name spaces, like:
   *  - $_SESSION['system'] = non-array
   *  - $_SESSION['module'] = non-array
   *  - $_SESSION['application'] = non-array
   *
   * Asking for key in a key path that isnt array is not considered an error;
   * example:
   * @code
   *   State::sessionGet('theme') ~ 'current theme name', swell
   *   State::sessionGet('theme', 'whatever') ~ null, no error
   * @endcode
   * @throws Exception
   *   Always caught and logged.
   *   If bad argument.
   *   If session started and $_SESSION isnt array.
   *   If $context_type is falsy.
   *   If $context_type is name-spaced and $_SESSION[$context_type] exists
   *   and isnt array.
   *   If $context_type is name-spaced and arg $context_name used
   *   and is numeric.
   * @param string|array $context_type
   *   String, name-spaced value: system|module|application.
   *   String, non-namespaced value: anything else, like 'theme'.
   *   Array: all arguments.
   * @param string|integer|null $context_name
   *   Default: no argument.
   *   Numeric not allowed if name-spaced $context_type
   *   (system|module|application).
   * @param string|integer|null $key,...
   *   Any number of arguments.
   *   Default: no argument.
   * @return mixed
   *   Null: session not started, or that session array key doesnt exist,
   *   or it's value is null.
   *   False: error, or that session array value is false.
   *   Otherwise the value of that array.
   */
  public static function sessionGet($context_type, $context_name = NULL, $key = NULL/*,...*/) {
    return self::_access(0, is_array($context_type) ? $context_type : func_get_args());
  }

  /**
   * Get and unset session variable, with low risk of collision.
   *
   * Acts precisely like State::get(), except removes that session variable.
   *
   * Uses array_splice() - re-indexing the array - if the last argument
   * is an integer (not 'integer'), and that index is placed correctly
   * in the (presumably numerically indexed) array.
   *
   * @see State::sessionGet()
   * @param string|array $context_type
   *   String, name-spaced value: system|module|application.
   *   String, non-namespaced value: anything else, like 'theme'.
   *   Array: all arguments.
   * @param string|integer|null $context_name
   *   Default: no argument.
   *   Numeric not allowed if name-spaced $context_type
   *   (system|module|application).
   * @param string|integer|null $key,...
   *   Any number of arguments.
   *   Default: no argument.
   * @return mixed
   *   Null: session not started, or that session array key doesnt exist,
   *   or it's value is null.
   *   False: error, or that session array value is false.
   *   Otherwise the value of that array.
   */
  public static function sessionRemove($context_type, $context_name = NULL, $key = NULL/*,...*/) {
    return self::_access(1, is_array($context_type) ? $context_type : func_get_args());
  }

  /**
   * List of domain-safe cookie defaults.
   *
   *  Buckets:
   *  - (integer) expire; zero ~ session
   *  - (string) path; '/' or '/path-without-traling-slash'
   *    (checked copy of $GLOBALS['base_path'])
   *  - (string) domain; '.domain' (checked copy of $GLOBALS['cookie_domain'])
   *  - (boolean) secure ~ $GLOBALS['is_https']
   *  - (boolean) httponly; false
   *  - (string) namespace; hash of http|https + domain + path
   *    (see State::cookieSet() for explanation)
   *
   * Called by hook_init() implementation.
   *
   * @see State::cookieSet()
   * @return array
   */
  public static function cookieDefaults() {
    if (!$a = self::$_cookieDefaults) {
      // Make sure that path is either '/', or doesnt have trailing slash;
      // base_path always has trailing slash.
      if (($p = $GLOBALS['base_path']) != '/') {
        if (!($le = strlen($p))) {
          $p = '/';
        }
        elseif ($p{ $le - 1 } == '/') {
          $p = substr($p, 0, $le - 1);
        }
      }
      // Make sure that domain starts with dot.
      $d = $GLOBALS['cookie_domain'];
      if ($d{0} != '.') {
        $d = '.' . $d;
      }
      self::$_cookieDefaults = $a = array(
        'expire' => 0,
        'path' => $p,
        'domain' => $d,
        'secure' => $s = $GLOBALS['is_https'],
        'httponly' => FALSE,
        // Hash it to prevent issues with illegal chars.
        'namespace' => '_' . sha1('' . ((int)$s) . $d . $p),
      );
    }
    return $a;
  }

  /**
   * Set domain-safe cookie.
   *
   * Do not use this method for cookies that actually _should_ share
   * cookie domain with another site.
   * Nor usable for setting a cookie for another protocol (http|https)
   * or domain than the global site settings.
   * But can set cookie below base_path.
   *
   * Cookie namespace prevents collisions between different sites having same
   * or similar base cookie domain.
   * If the cookie domain of site B is .sub.domain.tld, and that of site A
   * is .domain.tld (a subset of B's domain),
   * then site B will read cookies set for site A.
   * Appending a string to cookie names - which reflects protocol (http|https),
   * cookie domain and base path - prevents collisions.
   *
   * @see State::cookieDefaults()
   * @param string $name
   *   Without cookie namespace appendix.
   * @param mixed $value
   *   Will be stringed.
   * @param array|boolean|null $options
   *   True: httponly.
   *   Array (integer) expire: default session (zero), non-session
   *   is unix timestamp.
   *   Array (string) path: default base_path, non-empty value
   *   is interpretated as relative to base_path.
   *   Array (boolean) httponly: default false.
   * @return boolean
   *   False: failed to set the cookie.
   */
  public static function cookieSet($name, $value, $options = NULL) {
    // PHP array copy.
    $a = self::$_cookieDefaults;
    if ($options) {
      if ($options === TRUE) {
        $a['httponly'] = TRUE;
      }
      elseif (is_array($options)) {
        foreach ($options as $k => $v) {
          switch ($k) {
            case 'expire':
              if ($v > 0) {
                $a['expire'] = $v;
              }
              break;
            case 'path':
              if ($v && ($v = trim($v)) != '/' && ($le = strlen($v))) {
                // Remove trailing slash.
                if ($v{ $le - 1 } == '/') {
                  $v = substr($v, 0, $le - 1);
                }
                if ($a['path'] == '/') {
                  if ($v{0} == '/') {
                    $v = substr($v, 1);
                  }
                }
                // Default path isnt '/'.
                elseif ($v{0} != '/') {
                  $v = '/' . $v;
                }
                $a['path'] .= $v;
              }
              break;
            case 'httponly':
              $a['httponly'] = (bool)$v;
              break;
          }
        }
      }
    }
    if (setcookie('' . $name . $a['namespace'], $value, $a['expire'], $a['path'], $a['domain'], $a['secure'], $a['httponly'])) {
      // If not deleting.
      if (!$a['expire'] || $a['expire'] > REQUEST_TIME - 3600) {
        self::$_cookiesSetInRequest[ $name ] = $value;
        unset(self::$_cookiesRemovedInRequest[ $name ]);
      }
      else {
        unset(self::$_cookiesSetInRequest[ $name ]);
        self::$_cookiesRemovedInRequest[ $name ] = TRUE;
      }
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Increase value of domain-safe cookie; see State::cookieSet().
   *
   * Is aware of cookies set/removed previously in same request (normally
   * you cannot 'see' a cookie set in same request, only in a later request).
   *
   * @see State::cookieSet()
   * @throws Exception
   *   Caught (to get trace) if cookie value or arg $increment is not empty
   *   or a number.
   * @param string $name
   *   Without cookie namespace appendix.
   * @param integer|float $increment
   *   Default: one.
   * @param array|boolean|null $options
   *   True: httponly.
   *   Array (integer) expire: default session (zero), non-session
   *   is unix timestamp.
   *   Array (string) path: default base_path, non-empty value is interpretated
   *   as relative to base_path.
   *   Array (boolean) httponly: default false.
   * @return number|boolean
   *   Number: the new value of the cookie.
   *   False: cookie value or arg $increment is not empty or a number,
   *   or failed to set the cookie.
   */
  public static function cookieIncrease($name, $increment = 1, $options = NULL) {
    $em = NULL;
    if ($increment && !is_numeric($increment)) {
      $em = 'Cookie named[' . $name . '] cannot be increased by increment[' . $increment . '] because that is not a number.';
    }
    elseif (($v = self::cookieGet($name)) && !is_numeric($v)) {
      $em = 'Cookie named[' . $name . '] cannot be increased because it\'s value[' . $increment . '] is not a number.';
    }
    if ($em) {
      try {
        throw new Exception($em);
      }
      catch (Exception $xc) {
        self::_errorHandler($xc);
      }
      return FALSE;
    }
    return self::cookieSet($name, $v += $increment, $options) ? $v : FALSE;
  }

  /**
   * Get domain-safe cookie; see State::cookieSet() for explanation.
   *
   * Is aware of cookies set/removed previously in same request (normally
   * you cannot 'see' a cookie set in same request, only in a later request).
   *
   * @see State::cookieSet()
   * @param string $name
   *   Without cookie namespace appendix.
   * @return mixed|null
   *   Null: not set.
   */
  public static function cookieGet($name) {
    if (array_key_exists($name, self::$_cookiesRemovedInRequest)) {
      return NULL;
    }
    if (array_key_exists($name, self::$_cookiesSetInRequest)) {
      return self::$_cookiesSetInRequest[ $name ];
    }
    return !empty($GLOBALS['_COOKIE']) && array_key_exists($nm = '' . $name . self::$_cookieDefaults['namespace'], $_COOKIE) ?
      $_COOKIE[$nm] : NULL;
  }

  /**
   * Remove domain-safe cookie; see State::cookieSet() for explanation.
   *
   * Is aware of cookies set/removed previously in same request (normally
   * you cannot 'see' a cookie set in same request, only in a later request).
   *
   * @see State::cookieSet()
   * @param string $name
   *   Without cookie namespace appendix.
   * @param array|boolean|null $options
   *   True: httponly.
   *   Array (string) path: default base_path, non-empty value is interpretated
   *   as relative to base_path.
   *   Array (boolean) httponly: default false.
   * @return string|null
   *   String: value of the cookie if it existed.
   *   Null: the cookie didnt exist.
   */
  public static function cookieRemove($name, $options = NULL) {
    if (array_key_exists($name, self::$_cookiesRemovedInRequest)) {
      return NULL;
    }
    if ($options && is_array($options)) {
      $a = $options;
      $a['expire'] = REQUEST_TIME - 3600;
    }
    else {
      $a = array(
        'expire' => REQUEST_TIME - 3600
      );
    }
    if (array_key_exists($name, self::$_cookiesSetInRequest)) {
      $v = self::$_cookiesSetInRequest[ $name ];
    }
    else {
      $v = !empty($GLOBALS['_COOKIE']) && array_key_exists($nm = '' . $name . self::$_cookieDefaults['namespace'], $_COOKIE) ?
        $_COOKIE[$nm] : NULL;
    }
    self::cookieSet($name, '', $a);
    return $v;
  }

  /**
   * Get list of registered non-page paths.
   *
   * @param boolean $builtInsOnly
   *   Default: false.
   * @return array
   */
  public static function nonPagePaths($builtInsOnly = FALSE) {
    return $builtInsOnly || !($v = variable_get('state__nonpages')) ? self::$_nonPagePaths :
        array_merge(self::$_nonPagePaths, explode("\n", $v));
  }

  /**
   * Not AJAX-, iframe- and other non-page requests.
   *
   * @return boolean
   */
  public static function isPageRequest() {
    // If never checked (null).
    if (!($is = self::$_isPageRequest) && $is === NULL) {
      $is = TRUE;
      // Test for AJAX request header.
      if ( (!empty($_SERVER['HTTP_X_REQUESTED_WITH']) && strtolower($_SERVER['HTTP_X_REQUESTED_WITH']) == 'xmlhttprequest')) {
        $is = FALSE;
      }
      // Test if current request url is in the list of non-page urls.
      elseif (!empty($_GET['q'])) {
        $le = count(
          $a = !($v = variable_get('state__nonpages')) ? self::$_nonPagePaths : array_merge(self::$_nonPagePaths, explode("\n", $v))
        );
        for ($i = 0; $i < $le; $i++) {
          if (strpos($_GET['q'], $a[0]) === 0) {
            $is = FALSE;
            break;
          }
        }
      }
      self::$_isPageRequest = $is;
    }
    return $is;
  }

  /**
   * Request number in session (the whole browser session, not only $_SESSION).
   *
   * Only valid (> -1) if State is configured to detect (and count by)
   * request type.
   *
   * 'logged_in' request number is not available when current request
   * is a non-page request (then returns -1).
   *
   * A page redirect counts as a page request.
   *
   * @param string $type
   *   Default: 'page'.
   *   Allowed values: page | non_page | all (~ page + non_page)
   *   | logged_in (~ page request since user logged in) | list.
   * @return integer|array|boolean
   *   False: unsupported $type value.
   *   Minus 1: module not configured to detect and count requests,
   *   or is non-page request and $type is 'logged_in'.
   *   1: first request is one, not zero.
   *   Array: when $type is 'list'; [ 'page': n0, 'non_page': n1, 'logged_in': n2 ].
   */
  public static function requestNumber($type = 'page') {
    switch ($type) {
      case 'page':
        return self::$_pageRequests;
      case 'non_page':
        return self::$_nonPageRequests;
      case 'logged_in':
        return self::$_loggedInPageRequests;
      case 'all':
        return self::$_pageRequests + self::$_nonPageRequests;
      case 'list':
        return array(
          'page' => self::$_pageRequests,
          'non_page' => self::$_nonPageRequests,
          'logged_in' => self::$_loggedInPageRequests,
        );
    }
    return FALSE;
  }

  /**
   * Number of page responses delivered in session (the whole browser session,
   * not only $_SESSION) - until now (current response not counted).
   *
   * Always fairly reliable, and doesnt depend on State being configured
   * to detect (and count by) request type
   * - because counts responses that included hook_preprocess_html() execution.
   *
   * @return integer
   *   Zero: none yet.
   */
  public static function pageResponses() {
    return (($n = self::$_pageResponses) > -1) ? $n :
      (self::$_pageResponses = (int)self::cookieGet('state__prespno'));
  }

  /**
   * Get the frontend session control mode - checking/prolongation -
   * which applys to current visitor.
   *
   * Reflects prolongation suggestion, if prolongSuggestion() has been called.
   *
   * Not called in init, because it should only be called on demand
   * (and available to everybody).
   * Will eventually be called during page generation, via
   * the hook_preprocess_html() implementation.
   *
   * @see State::suggestProlongation()
   * @return integer|boolean
   *   False: session not started yet, frontend checking/prolongation
   *   may apply if session gets started later.
   *   0: none.
   *   1: checks session/form expiration.
   *   2: prolongs session, when visitor is active (mousemove etc.).
   *   3: prolongs session unconditionally.
   */
  public static function prolongation() {
    if (!self::$_started && !(self::$_started = drupal_session_started())) {
      return FALSE;
    }
    if (($fsc = self::$_prolongation) || $fsc === 0) {
      return $fsc;
    }
    if (!variable_get('state__frntnd')) {
      return (self::$_prolongation = 0);
    }
    $fsc = 0;
    $cond = variable_get('state__prlngcnd');
    $unCond = variable_get('state__prlnguncnd');
    // Not logged in.
    if (!user_is_logged_in()) {
      // -2 ~ anonymous user.
      if ($cond && array_key_exists('-2', $cond)) {
        $fsc = 2;
      }
      elseif ($unCond && array_key_exists('-2', $unCond)) {
        $fsc = 3;
      }
    }
    // Logged in.
    else {
      $cond_all = $unCond_all = FALSE;
      if ($cond) {
        // -1 ~ authenticated user ~ wildcard.
        if (array_key_exists('-1', $cond)) {
          $cond_all = TRUE;
        }
        else {
          // Get rid of anonymous user.
          unset($cond['-2']);
        }
      }
      if ($unCond) {
        // -1 ~ authenticated user ~ wildcard.
        if (array_key_exists('-1', $unCond)) {
          $unCond_all = TRUE;
        }
        else {
          // Get rid of anonymous user.
          unset($unCond['-2']);
        }
      }
      // Check for authenticated user (~ wildcard) for one mode,
      // and possible (specific) overrider for other mode.
      // Check for admin user 1 - and that user's pseudo role -3.
      if (($uid = $GLOBALS['user']->uid) == 1) {
        $userRoles = array('-3');
      }
      else {
        $user = user_load($uid);
        $userRoles = array_keys($user->roles);
      }
      if ($cond_all) {
        $fsc = 2;
        if ($unCond) {
          // Specific role overrides.
          if (array_intersect($unCond, $userRoles)) {
            $fsc = 3;
          }
        }
      }
      elseif ($unCond_all) {
        $fsc = 3;
        if ($cond) {
          // Specific role overrides.
          if (array_intersect($cond, $userRoles)) {
            $fsc = 2;
          }
        }
      }
      // No authenticated user (~ wildcard) for any mode; conditional
      // must have priority over unconditional.
      elseif ($cond) {
        if (array_intersect($cond, $userRoles)) {
          $fsc = 2;
        }
      }
      if (!$fsc && $unCond) {
        if (array_intersect($unCond, $userRoles)) {
          $fsc = 3;
        }
      }
    }
    // Simple session/form checking, no prolongation.
    if (!$fsc && variable_get('state__chcksss')) {
      $fsc = 1;
    }
    if ($fsc) {
      if (($v = self::$_prolongSuggestion) > -1 && $v < $fsc) {
        $fsc = $v;
      }
    }
    if ($fsc) {
      self::$_checkInterval = variable_get('state__chckntrvl', self::CHECK_INTERVAL_DEFAULT);
      if (variable_get('state__autrdr')) {
        self::$_autoRedirect = TRUE;
      }
      if (($v = variable_get('state__rdrtrg')) && strlen($v)) {
        self::$_redirectTarget = $v;
      }
    }
    return (self::$_prolongation = $fsc);
  }

  /**
   * Suggest less (safer) session checking/prolongation mode, for current page.
   *
   * Has no effect if called later or at execution of hook_preprocess_html()
   * implementations.
   *
   * Is disregarded if current visitor's mode is more restrictive than
   * suggested; e.g. if current visitor (by role) is set to get no prolongation
   * and the suggestion is conditional prolongation.
   *
   * @param integer|string|null $prolong
   *   Default: null (~ no suggestion).
   *   0|none: dont check nor prolong.
   *   1|check: checks session/form expiration.
   *   2|conditional: prolongs session, when visitor is active (mousemove etc.).
   *   3|unconditional: prolongs session unconditionally.
   */
  public static function suggestProlongation($prolong = NULL) {
    if ($prolong !== NULL) {
      switch ('' . $prolong) {
        case '0':
        case 'none':
          $v = 0;
          break;
        case '1':
        case 'check':
          $v = 1;
          break;
        case '2':
        case 'conditional':
          $v = 2;
          break;
        case '3':
        case 'unconditional':
          $v = 3;
          break;
        // Unsupported.
        default:
          $v = -1;
          try {
            throw new Exception( t('Unsupported value[!value] of prolongation suggestion.', array('!value' => $prolong)) );
          }
          catch (Exception $xc) {
            self::_errorHandler($xc);
          }
      }
      self::$_prolongSuggestion = $v;
    }
  }

  /**
   * For completely AJAX-driven pages, which solely uses custom AJAX mechanisms,
   * not Drupal form API's AJAX procedures.
   *
   * Makes frontend session prolongation skip checking for form expiration.
   *
   * Has to be called per page (like in theme hook) because it's unlikely
   * that all forms in a Drupal site are completely AJAX-driven.
   *
   *  Why 'custom AJAX' and not 'Drupal form API's AJAX'?
   *  - because Drupal's conventional form AJAX procedures - probably
   *  - are constrained by Drupal form expiration (6 hours).
   */
  public static function unlimitFormExpire() {
    self::$_noFormExpire = TRUE;
  }

  /**
   * Destroy current visitor's session (if any) and log current user out
   * (if logged in).
   *
   * Usable when you want to log out the user, but don't want that redirect
   * which user_logout() performs.
   * A redirect prevents setting/changing cookies. And a redirect during
   * a non-page (AJAX, file upload etc.) response doesn't make much sense.
   *
   * Called by frontend State, when it detects that the session has expired;
   * to help session garbage collection.
   *
   * @param string $session_id
   *   Default: empty.
   *   Non-empty: destroy only if value equals current session id.
   * @param string $session_id_hash
   *   Default: empty.
   *   Non-empty: destroy only if value equals hash of current session id.
   * @param string $log_reason
   *   Default: 'session_timeout'.
   */
  public static function sessionDestroy($session_id = '', $session_id_hash = '', $log_reason = 'session_timeout') {
    if ( (self::$_started || (self::$_started = drupal_session_started()))
        && (!$session_id || $session_id === session_id())
        && (!$session_id_hash || $session_id_hash === hash('sha256', session_id() . variable_get('state__salt')))
    ) {
      // Remove State session related non-httponly cookies, which frontend uses:
      // - the hash of session id, used to check
      // if window/tab's session is current
      self::cookieRemove('state__sid');
      // - the session prolonged cookie, used to get the last time
      // any window/tab made a (prolonging) AJAX request
      self::cookieRemove('state__ssprlngd');

      // We cannot call user_logout(), because that function performs
      // a redirect, but we do all the same procedures (but drupal_goto()).
      // And we expect that no function implementing hook_user_logout()
      // performs a redirect;
      // though the consequenses probably would narrow to the cookies
      // above not being deleted.
      //
      // global $user;
      // watchdog('user', 'Session closed for %name.', array('%name' => $user->name));
      // module_invoke_all('user_logout', $user);
      // // Destroy the current session, and reset $user to the anonymous user.
      // session_destroy();
      // drupal_goto();
      //
      // user_logout()
      global $user;
      if ($user->uid) {
        watchdog(
          'user',
          'Session closed for (%uid) %name, reason: %reason.',
          array('%uid' => $user->uid, '%name' => $user->name, '%reason' => $log_reason)
        );
        // If logged in and session timeout: Tell user about it (here, but not
        // when forced logout from backend; state_init()); unless turned off.
        if ($log_reason == 'session_timeout' && !variable_get('state__sssoutmsomit', FALSE)) {
          // Use English label/message override?
          if (!empty($GLOBALS['language_content']->language) && $GLOBALS['language_content']->language == 'en'
            && ($json = variable_get('state__enovrrd')) && ($enOverride = drupal_json_decode($json)) && !empty($enOverride['expired_logout'])
          ) {
            $t = $enOverride['expired_logout'];
          }
          else {
            $t = t('You were logged out, because the page timed out.');
          }
          state_set_message($t);
        }
        // Let everybody else respond to hook_user_logout().
        module_invoke_all('user_logout', $user);
      }
      session_destroy();
      // /user_logout()

      self::$_started = FALSE;
    }
  }

  /**
   * Adds state javascript library, and commands session checking
   * and/or prolongation (if applicable for current visitor).
   *
   * Public because state_preprocess() needs access.
   *
   * Effectively a hook_preprocess_html() implementation.
   *
   * @staticvar boolean $called
   */
  public static function _frontend() {
    static $called = FALSE;
    if (!$called) {
      $called = TRUE;
      // Page response number (how many times we have delivered a page).
      self::$_pageResponses = ($pageRespNo = self::cookieIncrease('state__prespno', 1)) - 1;

      // Prepare javascripting.
      // jQuery.cookie.
      drupal_add_library('system', 'cookie');
      $cookie_defaults =& self::$_cookieDefaults;
      $js_args = '"' . $cookie_defaults['domain'] . '", "' . $cookie_defaults['path'] . '", "' . $cookie_defaults['namespace'] . '", ' . $pageRespNo;

      // Check if not checked yet (or no session).
      if (($fsc = self::$_prolongation) === FALSE) {
        $fsc = self::prolongation();
      }
      if ($fsc && ($v = self::$_prolongSuggestion) > -1 && $v < $fsc) {
        $fsc = $v;
      }

      if ($fsc) {
        // jQuery.ui.dialog, for expiration info to visitor.
        drupal_add_library('system', 'ui.dialog');
        // Hash of session id must be highly obfuscated, because we set it
        // in non-httponly cookie (session cookie is httponly).
        $sessHash = hash('sha256', session_id() . variable_get('state__salt'));
        self::cookieSet('state__sid', $sessHash);

        $js_args .= ', { sid:"' . $sessHash . '", '
          . 'formExpire:' . (!self::$_noFormExpire ? state::FORM_EXPIRE : 0) . ', '
          . 'prolong:' . $fsc . ', '
          . 'checkInterval:' . self::$_checkInterval . ', '
          . 'autoRedirect:' . (int)self::$_autoRedirect . ', '
          . 'redirectTarget:"' . self::$_redirectTarget . '", '
          . 'localEn:'
          . (empty($GLOBALS['language_content']->language) || $GLOBALS['language_content']->language != 'en' || !($v = variable_get('state__enovrrd')) ?
              '{}' : $v) . '} ';
      }

      drupal_add_js(
          '(function($) { $(document).bind("ready", function() { State.init(' . $js_args . '); }); })(jQuery);',
          array('type' => 'inline', 'scope' => 'header', 'preprocess' => FALSE, 'every_page' => FALSE)
      );
    }
  }

  /**
   * Internal method for accessing/modifying session vars.
   *
   * Public because functions state_set()/state_increase()/state_get()
   * /state_remove() must have access.
   * Using this method directly is not recommended.
   *
   * @throws Exception
   *   Always caught and logged.
   * @param integer $act
   * @param array $args
   * @return mixed
   */
  public static function _access($act, $args) {
    switch ($act) {
      // Set.
      case 10:
      // Increase.
      case 11:
        if (!self::$_started && !(self::$_started = drupal_session_started())) {
          // Why doesnt drupal_session_start return boolean?
          drupal_session_start();
          if (!(self::$_started = drupal_session_started())) {
            return NULL;
          }
        }
        break;
      // 0 ~ get | 1 ~ remove.
      default:
        if (!isset($_SESSION) || !$_SESSION) {
          return NULL;
        }
    }
    try {
      if (is_array($_SESSION)) {
        $nArgs = count($args);
        // Remove null values, backwards, which may exist if $args
        // is all arguments of calling method/function.
        for ($i = $nArgs - 1; $i >= 0; $i--) {
          if ($args[$i] === NULL) {
            unset($args[$i]);
          }
          else {
            break;
          }
        }
        $nameSpaced = isset(self::$_contextTypes[ $args[0] ]);
        switch (($nArgs = count($args))) {
          case 0:
            throw new Exception('No args');
          case 1:
            if ($act >= 10) { // set/increase must receive more than one arg
              throw new Exception('Cannot set/increase value using only one arg');
            }
            break;
          default:
            // Namespaced, special requirements.
            if ($nameSpaced) {
              if (!$args[1]) {
                throw new Exception('Cannot access namespaced[' . $args[0] . '] vars using falsy 2nd arg[' . $args[1] . ']');
              }
              if (!is_string($args[1])) {
                throw new Exception('Cannot access namespaced[' . $args[0] . '] vars using non-string[' . gettype($args[1]) . '] 2nd arg');
              }
              // Set/increase.
              if ($act >= 10) {
                switch ($nArgs) {
                  case 2:
                    // Set.
                    if ($act == 10) {
                      throw new Exception('Cannot set namespaced[' . join('][', $args) . '] using less than 3 args');
                    }
                    // Increase.
                    else {
                      throw new Exception('Cannot increase namespaced[' . join('][', $args) . '] using less than 4 args, received 2');
                    }
                  case 3:
                    // Increase.
                    if ($act == 11) {
                      throw new Exception('Cannot increase namespaced[' . join('][', $args) . '] using less than 4 args');
                    }
                    // Set.
                    elseif (!is_array($args[2])) {
                      throw new Exception('When setting namespaced[' . $args[0] . '][' . $args[1]
                        . '] using only 3 args, 3rd arg must be array, not [' . gettype($args[2]) . ']');
                    }
                    break;
                  default:
                    if (!is_string($args[2])) {
                      throw new Exception('When setting/increasing namespaced[' . $args[0] . '][' . $args[1]
                        . '] using more than 3 args, 3rd arg must be string, not [' . gettype($args[2]) . ']');
                    }
                }
                if ($act == 11 && !is_numeric($args[ $nArgs - 1 ])) {
                  $v = array_pop($args);
                  throw new Exception('Cannot increase namespaced[' . join('][', $args) . '] using non-numeric value[' . $v . ']');
                }
              }
            }
        }
        $aS =& $_SESSION;
        // Get/remove.
        if ($act < 10) {
          for ($i = 0; $i < $nArgs; $i++) {
            // Bucket exists.
            if (array_key_exists($k = $args[$i], $aS)) {
              // Not last argument.
              if ($i < $nArgs - 1) {
                if (is_array($aS[$k])) {
                  // For next iteration.
                  $aS =& $aS[$k];
                  continue;
                }
                // $_SESSION[ context type ] is not array.
                elseif (!$i && $nameSpaced) {
                  throw new Exception('$_SESSION[' . $args[0] . '], type [' . gettype($aS[ $args[0] ]) . '], is not array');
                }
                return NULL;
              }
              // $_SESSION[ context type ] is not array.
              elseif (!$i && $nameSpaced && !is_array($aS[$k])) {
                throw new Exception('$_SESSION[' . $args[0] . '], type [' . gettype($aS[ $args[0] ]) . '], is not array');
              }
              // Last argument.
              // Get.
              if (!$act) {
                return $aS[$k];
              }
              $v = $aS[$k];
              // Splice if key is integer and placed correctly in the array.
              if (is_int($k) && $k > -1 && count($aS) > $k) {
                $keys = array_keys($aS);
                if ($keys[$k] == $k) {
                  array_splice($aS, $k, 1);
                }
                else {
                  unset($aS[$k]);
                }
              }
              else {
                unset($aS[$k]);
              }
              return $v;
            }
            return NULL;
          }
        }
        // Set/increase.
        else {
          // We dont know, check at first iteration.
          $empty = FALSE;
          // Last arg is value/increment, skip that.
          // For IDE.
          $k = NULL;
          for ($i = 0; $i < $nArgs - 1; $i++) {
            $k = $args[$i];
            // If parent didnt exist already or current wasnt array.
            if ($empty
              // If current doesnt exist.
              || !array_key_exists($k, $aS)
              // If current not last key, and current not array.
              || ($i < $nArgs - 2 && !is_array($aS[$k]))
            ) {
              $empty = TRUE;
              $aS[$k] = array();
            }
            // If current not last key, refer for next iteration.
            if ($i < $nArgs - 2) {
              $aS =& $aS[$k];
            }
          }
          // Set.
          if ($act == 10) {
            $aS[$k] = $args[$nArgs - 1];
            return TRUE;
          }
          elseif (array_key_exists($k, $aS) && is_numeric($aS[$k])) {
            return ($aS[$k] += $args[$nArgs - 1]);
          }
          return ($aS[$k] = $args[$nArgs - 1]);
        }
      }
      throw new Exception('$_SESSION, type [' . gettype($_SESSION) . '], is not array');
    }
    catch (Exception $xc) {
      switch ($act) {
        case 0:
          $act = 'get';
          break;
        case 1:
          $act = 'remove';
          break;
        case 10:
          $act = 'set';
          break;
        case 11:
          $act = 'increase';
          break;
      }
      self::_errorHandler($xc, 'State::' . $act . '()');
    }
    return FALSE;
  }

  /**
   * Public because state_init() needs access.
   *
   * @staticvar boolean $called
   * @param boolean $started
   */
  public static function _initRequestCounting($started) {
    static $called = FALSE;
    if (!$called) {
      $called = TRUE;
      self::$_countRequests = TRUE;
      if ($started) {
        self::$_started = TRUE;
      }
      // Check if page request.
      $pageRequest = self::isPageRequest();

      // Find non-page request number in cookie.
      $n = !$pageRequest ? 1 : 0;
      if (strlen($v = (int)self::cookieGet('state__reqsnonpage')) < 11
        // PHP_MAX_INT.
        && $v > 0 && $v < 2147483647
      ) {
        $n = $v + 1;
      }
      self::$_nonPageRequests = $n;
      if (!$pageRequest) {
        // httponly.
        self::cookieSet('state__reqsnonpage', $n, TRUE);
      }
      // Find page request number in cookie.
      $n = !$pageRequest ? 0 : 1;
      if (strlen($v = (int)self::cookieGet('state__reqspage')) < 11 && $v > 0 && $v < 2147483647) {
        $n = $v + 1;
      }
      self::$_pageRequests = $n;
      // Non-page request gets out here.
      if (!$pageRequest) {
        return;
      }
      // httponly.
      self::cookieSet('state__reqspage', $n, TRUE);
      // Find request number since login.
      $n = 0;
      if ($started && user_is_logged_in()) {
        $n = self::sessionIncrease('module', 'state', 'reqsloggedin', 1);
      }
      self::$_loggedInPageRequests = $n;
    }
  }

  /**
   * Traces and logs via Inspect tracer, or watchdog (no trace).
   *
   * Displays drupal error message, if error level allows that.
   *
   * @param Exception $xc
   * @param string $message
   *   Default: empty.
   */
  protected static function _errorHandler($xc, $message = '') {
    if (module_exists('inspect')) {
      inspect_trace($xc, array('category' => 'state', 'message' => $message, 'severity' => WATCHDOG_ERROR));
    }
    else {
      watchdog(
        'state',
        // Will generate error when extracting strings for translation,
        // but replacing just the message doesnt make sense.
        ($message ? (check_plain($message) . ': ') : '')
          // To prevent error when extracting strings for translation.
          . check_plain($xc->getMessage()),
        NULL,
        WATCHDOG_ERROR
      );
    }
    if ($GLOBALS['conf']['error_level']) {
      drupal_set_message(check_plain($xc->getMessage()), 'error');
    }
  }
}