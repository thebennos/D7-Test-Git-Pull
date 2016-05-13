<?php
/**
 * @file
 * Code for the supercookie module.
 */

/**
 * The SuperCookie class.
 */
class SuperCookie {

  public $dnt = FALSE;
  public $scid, $uid, $created, $modified, $expires = 0;
  public $data = NULL;
  public $tid, $nid, $custom = array();
  private $mongodb;

  /**
   * Constructor method.
   */
  public function __construct() {

    $this->mongodb = _supercookie_variable_get('supercookie_mongodb');

    return $this;
  }

  /**
   * Gets an instance of the MongoDB collection per config.
   */
  private function __getMongoCollection() {

    $client = NULL;

    if ($this->mongodb) {
      $conf = variable_get('mongodb_connections');
      $client = new MongoClient($conf['default']['host']);
      $client = $client
        ->selectCollection($conf['default']['db'], 'supercookie');
    }

    return $client;
  }
  /**
   * TODO.
   */
  private function __init($matcher = NULL, $data = NULL) {

    // Get client's specified hash.
    $hash_client = 0;
    $header = $this->header();
    $headers = $this->header(TRUE);

    // Flag to honor user's (and site owners's acknowledgemnt of) DNT requests.
    $this->dnt = _supercookie_variable_get('supercookie_honor_dnt') && !empty($headers['DNT']) && $headers['DNT'] == 1;

    // Check custom HTTP header for cookie value.
    if (empty($hash_client) && !empty($header) && $header !== '""') {
      $hash_client = $header;
    }
    // Check Cookie HTTP header for cookie value.
    if (empty($hash_client) && !empty($headers['Cookie'])) {
      $cookies = explode(';', $headers['Cookie']);
      foreach ($cookies as $pair) {
        $parts = explode('=', $pair);
        if ($parts[0] == _supercookie_variable_get('supercookie_name_server')) {
          $hash_client = $parts[1];
        }
      }
    }
    // Check HTTP cookie for cookie value.
    if (empty($hash_client) && !empty($_COOKIE[_supercookie_variable_get('supercookie_name_server')])) {
      $hash_client = $_COOKIE[_supercookie_variable_get('supercookie_name_server')];
    }

    // Expire user's db record and cookies if client hash does not match hash
    // lookup result.
    if (!empty($data) && !empty($hash_client)) {

      if ($data->data !== $hash_client) {
//        if ($this->mongodb) {
//          $this->__getMongoCollection()
//            ->remove(array(
//              'data' => $hash,
//            ));
//        }
//        else {
//          db_delete('supercookie')
//            ->condition('data', $hash_client)
//            ->execute();
//        }
      }
    }

    $this->scid = (!empty($data->scid) ? $data->scid : 0);
    $this->data = (!empty($data->data) ? $data->data : $matcher);
    if (empty($this->data)) {
      $this->data = $hash_client;
    }

    return $this->__read();
  }
  /**
   * TODO.
   */
  private function __read() {

    $or = db_or()
      ->condition('scid', $this->scid)
      ->condition('data', $this->data);

    // Honor user's DNT header and delete any previously collected data.
    if ($this->dnt === TRUE) {
      if ($this->mongodb) {
        $this->__getMongoCollection()
          ->remove(array('data' => $this->data));
      }
      else {
        db_delete('supercookie')
          ->condition($or)
          ->execute();
      }

      $this->scid = 0;
      $this->uid = 0;
      $this->created = 0;
      $this->modified = 0;
      $this->expires = 0;
      $this->data = NULL;
      $this->tid = array();
      $this->nid = array();
      $this->custom = array();

      return $this;
    }

    $row = null;
    // Check for mongodb storage.
    if ($this->mongodb) {
      if (!empty($this->scid)) {
        $row = (object) $this->__getMongoCollection()
          ->findOne(array('_id' => new MongoId($this->scid)));
      }
      else {
        $row = (object) $this->__getMongoCollection()
          ->findOne(array('$or' => array(
            array('data' => $this->data),
            array('scid' => $this->scid),
          )));
      }
    }

    // Default to standard rdbms.
    if (empty($row)) {
      $row = db_select('supercookie', 'sc')
        ->fields('sc', array(
          'scid',
          'uid',
          'created',
          'modified',
          'expires',
          'data',
          'tid',
          'nid',
          'custom',
        ))
        ->condition($or)
        ->range(0, 1)
        ->orderBy('sc.created', 'DESC')
        ->execute()
        ->fetchObject();
    }

    if (!empty($row) && !empty($row->scid)) {
      $this->scid = (!$this->mongodb ? intval($row->scid) : $row->scid);
      $this->uid = intval($row->uid);
      $this->created = intval($row->created);
      $this->modified = intval($row->modified);
      $this->expires = intval($row->expires);
      $this->data = $row->data;
      $this->tid = (!$this->mongodb ? unserialize($row->tid) : $row->tid);
      $this->nid = (!$this->mongodb ? unserialize($row->nid) : $row->nid);
      $this->custom = (!$this->mongodb ? unserialize($row->custom) : $row->custom);
    }
    else {
      $this->scid = 0;
      $this->uid = 0;
      $this->created = 0;
      $this->modified = 0;
      $this->expires = 0;
      // Don't even think about uncommenting "data"; doing so will cause
      // infinite XHR requests from JS when calls to $this->init() are made
      // with no args.
      // $this->data = NULL;
      $this->tid = array();
      $this->nid = array();
      $this->custom = array();
    }

    return $this;
  }
  /**
   * TODO.
   */
  private function __write($timestamp) {

    if ($this->dnt === TRUE) {
      return;
    }

    global $user;

    $row = (object) array(
      'uid' => $user->uid,
      'modified' => $timestamp,
      'data' => $this->data,
      'tid' => (is_array($this->tid) ? $this->tid : array()),
      'nid' => (is_array($this->nid) ? $this->nid : array()),
      'custom' => (is_array($this->custom) ? $this->custom : array()),
    );

    if (!empty($this->scid)) {
      $row->scid = $this->scid;
    }
    else {
      $row->created = $timestamp;
      $row->expires = $this->expires;
    }

    if ($this->mongodb) {
      // Check for mongodb storage.
      if (empty($row->scid)) {
        $this->__getMongoCollection()
          ->insert($row);

        $this->scid = strval($row->_id);
        $row->scid = $this->scid;
      }

      $this->__getMongoCollection()
        ->update(array('_id' => new MongoId($row->scid)), $row);
      $row = (object) $this->__getMongoCollection()
        ->findOne(array('_id' => new MongoId($this->scid)));
    }
    elseif (!empty($row->data) && $row->data !== '""') {
      // Default to standard rdbms.
      if (!empty($this->scid)) {
        drupal_write_record('supercookie', $row, 'scid');
      }
      else {
        drupal_write_record('supercookie', $row);
      }
    }

    $this->scid = $row->scid;
    $this->data = $row->data;
  }
  /**
   * TODO.
   */
  private function __delete($timestamp) {

    if ($this->mongodb) {
      $result = $this->__getMongoCollection()
        ->remove(array(
          'expired' => $timestamp,
          'scid' => $this->scid,
        ));
    }
    else {
      $result = db_delete('supercookie')
        ->condition('expires', $timestamp, '<')
        ->condition('scid', $this->scid)
        ->execute();
    }

    return $result;
  }


  /**
   * TODO.
   * @see @see http://php.net/manual/en/function.array-walk-recursive.php#114574
   */
  private function walk_recursive_remove_nulls(array $array) {
    foreach ($array as $k => $v) {
      if (is_array($v)) {
        $array[$k] = $this->walk_recursive_remove_nulls($v);
      }
      else {
        if ($v === NULL) {
          unset($array[$k]);
        }
      }
    }

    return $array;
  }


  /**
   * TODO.
   */
  public function match(&$hash) {

    // Check db for fingerprint match on data.
    if ($this->mongodb) {
      $data = $this->__getMongoCollection()
        ->findOne(array(
          'data' => $hash,
        ));

      if (!empty($data['scid'])) {
        $data = (object) array(
          'scid' => $data['scid'],
          'data' => $data['data'],
        );
      }
      else {
        $data = NULL;
      }
    }
    else {
      $data = db_select('supercookie', 'sc')
        ->fields('sc', array(
          'scid',
          'data',
        ))
        ->condition('data', $hash)
        ->range(0, 1)
        ->orderBy('sc.created', 'DESC')
        ->execute()
        ->fetchObject();
    }

    return $this->__init($hash, $data);
  }
  /**
   * TODO.
   */
  public function save($timestamp) {

    // Ignore client time; use server time exclusively.
    $timestamp = REQUEST_TIME;
    $expires = _supercookie_variable_get('supercookie_expire');
    if ($expires == 'calendar_day') {
      $expires = strtotime(date('Y-m-d', $timestamp) . ' + 1 day');
    }
    else {
      $expires = ($timestamp + $expires);
    }

    if (empty($this->expires)) {
      $this->expires = $expires;
    }

    // Clean up expired sessions.
    if ($this->expires < $timestamp) {
      $expired = $this->__delete($timestamp);

      // Reset object and set new expiration.
      if (!empty($expired)) {
        $this->__read();
        $this->expires = $expires;
      }
    }

    // Insert or update fingerprint record.
    $this->__write($timestamp);

    // Return populated supercookie.
    return $this->__read();
  }
  /**
   * Get the custom HTTP header set by supercookie.
   */
  public function header($all = FALSE) {

    if (function_exists('apache_request_headers')) {
      $request_headers = getallheaders();
    }
    else {
      // Nginx equivalent.
      foreach($_SERVER as $key => $value) {
        if (substr($key, 0, 5) == 'HTTP_') {
          $key = str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($key, 5)))));
          $request_headers[$key] = $value;
        }
        else{
          $request_headers[$key] = $value;
        }
      }
    }

    if ($all) {
      return $request_headers;
    }

    return (!empty($request_headers[_supercookie_variable_get('supercookie_name_header')]) ? $request_headers[_supercookie_variable_get('supercookie_name_header')] : NULL);
  }

  /**
   * Update record's target field.
   */
  private function mergeField($field_name, $data) {
    // Merge $data to target field.
    if (empty($this->{$field_name})) {
      $this->{$field_name} = array();
    }

    if (empty($data)) {
      return $this;
    }

    if (in_array($field_name, array('tid', 'nid'))) {
      $data = array_fill_keys($data, 1);
    }
    if (!empty($this->{$field_name})) {
      // Increment value counters for nid and tid fields.
      if (in_array($field_name, array('tid', 'nid'))) {
        foreach ($this->{$field_name} as $key => &$count) {
          if (array_key_exists($key, $data)) {
            $this->{$field_name}[$key] = ($this->{$field_name}[$key] + 1);
            unset($data[$key]);
          }
        }

        foreach ($data as $key => &$count) {
          $this->{$field_name}[$key] = $data[$key];
        }
      }
      elseif ($field_name == 'custom') {
        // Deep merge existing value with input leaves.
        $this->{$field_name} = array_replace_recursive($this->{$field_name}, $data);
        // Now prune NULL leaves from value.
        $this->{$field_name} = $this->walk_recursive_remove_nulls($this->{$field_name});
      }

      arsort($this->{$field_name});
    }
    else {
      $this->{$field_name} = $data;
    }

    // Return populated supercookie.
    return $this->save($this->created);
  }
  /**
   * Update record's custom field. To remove a leaf from the custom array, set
   * its value to NULL.
   */
  public function mergeCustom(array $data) {
    return $this->mergeField('custom', $data);
  }
  /**
   * Update record's nid field.
   */
  public function trackNodes(array $data) {
    if (_supercookie_variable_get('supercookie_track_nid')) {
      return $this->mergeField('nid', $data);
    }

    return $this;
  }
  /**
   * Update record's tid field.
   */
  public function trackTerms(array $data) {
    if (_supercookie_variable_get('supercookie_track_tid')) {
      return $this->mergeField('tid', $data);
    }

    return $this;
  }

}
