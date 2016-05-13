<?php

/**
 * @file
 * Provides class for memcached data handling.
 */

// Load include with Memcache API.
// We don't rely here on Drupal autoload mechanism because this include could
// be used before Drupal registry with all classes is loaded.
require_once dirname(__FILE__) . '/memcache_storage.api.inc';

// This file includes on the very early step (on the 2nd bootstrap phase).
// So if we want to execute something even before cache is initialized, here is
// the best place for it.

// Get indexes for cache bin namespaces from memcached.
$cache = MemcacheStorageAPI::get('memcache_storage_bin_indexes');
$indexes = &drupal_static('memcache_storage_bin_indexes');
$indexes = !empty($cache) ? $cache : array();

/**
 * Class handles memcached cache objects.
 */
class MemcacheStorage implements DrupalCacheInterface {

  /**
   * Name of cache bin.
   * Example: 'cache' or 'cache_page', etc.
   */
  protected $bin;

  /**
   * List of wildcards for the current cache bin.
   */
  protected $wildcards = array();

  /**
   * List of all cache ids in the current cache bin.
   */
  protected $cache_ids = array();

  /**
   * Constructs a new MemcacheStorage object.
   */
  function __construct($bin) {
    $this->bin = $bin;

    // Prefetch data with information about wildcards and stored cache ids
    // in the current cache bin.
    $data = MemcacheStorageAPI::getMultiple(array('memcache_storage_wildcards', 'memcache_storage_cids'), $this->cacheBinName());
    $this->wildcards = !empty($data['memcache_storage_wildcards']) ? $data['memcache_storage_wildcards'] : array();
    $this->cache_ids = !empty($data['memcache_storage_cids']) ? $data['memcache_storage_cids'] : array();
  }

  /**
   * Implements DrupalCacheInterface::get().
   */
  function get($cid) {
    $cids = array($cid);
    $cache = $this->getMultiple($cids);
    return isset($cache[$cid]) ? $cache[$cid] : FALSE;
  }

  /**
   * Implements DrupalCacheInterface::getMultiple().
   */
  function getMultiple(&$cids) {

    $data = MemcacheStorageAPI::getMultiple($cids, $this->cacheBinName());

    // Build a new array with cached data using normal cache id.
    $cache = array();
    if (empty($data)) {
      return $cache;
    }

    foreach ($data as $item) {
      // Check expiration for every cache item recieved from memcached pool.
      if ($this->validateItem($item)) {
        $cache[$item->cid] = $item;
      }
    }

    // Leave cache ids that was unable to get data from memcache.
    $cids = array_diff($cids, array_keys($cache));

    return $cache;
  }

  /**
   * Implements DrupalCacheInterface::set().
   */
  function set($cid, $data, $expire = CACHE_PERMANENT) {

    // Build cache object (as Drupal always does).
    $cache = new stdClass;
    $cache->cid = $cid;
    $cache->data = $data;
    $cache->created = REQUEST_TIME;
    $cache->expire = $expire;

    // We should always keep in storage temporary cache data.
    // Such cached data invalidates during cache_get() operations.
    if ($expire == CACHE_TEMPORARY) {

      // We should set expiration to 0 for temporary cache.
      // This type of cache could be expired after fetching from memcached storage.
      // @see MemcacheStorage::validateItem().
      $expire = 0;

      // Process custom cache expiration for pages.
      if ($this->bin == 'cache_page') {

        // Developers may set custom expiration for cached pages in settings.php.
        $custom_expiration = variable_get('memcache_storage_page_cache_custom_expiration', FALSE);
        if ($custom_expiration) {
          $expire = variable_get('memcache_storage_page_cache_expire', 0);
        }
      }
    }
    // Otherwise simply convert default drupal expiration timestamp to
    // memcached expiration ttl.
    elseif ($expire > REQUEST_TIME) {
      $expire -= REQUEST_TIME;
    }

    // Log info about used cache ID.
    $this->addCacheId($cid, $cache->created);

    // Save data into memcached pool.
    return MemcacheStorageAPI::set($cid, $cache, $expire, $this->cacheBinName());
  }

  /**
   * Implements DrupalCacheInterface::clear().
   */
  function clear($cid = NULL, $wildcard = FALSE) {

    // Function like cache_clear_all(NULL, 'cache_bin');
    // We should invalidate all old cache.
    if (empty($cid)) {

      $cache_min_lifetime = variable_get('cache_lifetime', 0);
      if (!empty($cache_min_lifetime)) {
        // Load timestamp of last cache flush for this bin.
        $cache_last_flush = variable_get('cache_flush_' . $this->bin, 0);
        if (empty($cache_last_flush) || REQUEST_TIME > ($cache_last_flush + $cache_min_lifetime)) {
          // Set timestamp when cache bin was flushed.
          // Actually, before cache load we will check this variable.
          variable_set('cache_flush_' . $this->bin, REQUEST_TIME);
        }
      }
      else {
        // No minimum cache lifetime, flush all temporary cache entries now.
        variable_set('cache_flush_' . $this->bin, REQUEST_TIME);
      }
    }
    else {
      if ($wildcard) {
        if ($cid == '*') {
          // Simple changing index for bin storage will flushes the whole cache
          // bin, because it is used for building memcache key.
          $this->increaseBinIndex();
        }
        else {
          // In case of wildcard invalidation just store info about wildcard
          // in the memcached. Later we will use bulk invalidation to delete
          // invalid cache from memcached. See memcache_storage_wildcard_flush()
          // for detailed overview.
          $this->wildcards[$cid] = REQUEST_TIME;
          MemcacheStorageAPI::set('memcache_storage_wildcards', $this->wildcards, CACHE_PERMANENT, $this->cacheBinName());
        }
      }
      elseif (is_array($cid)) {

        // Remove the current cache ID from the list of cache IDs in use.
        $this->removeCacheIDs($cid);

        // Delete cache from memcached instance.
        MemcacheStorageAPI::deleteMultiple($cid, $this->cacheBinName());
      }
      else {
        // Remove the current cache ID from the list of cache IDs in use.
        $this->removeCacheID($cid);

        // Just delete cache from memcached instance.
        MemcacheStorageAPI::delete($cid, $this->cacheBinName());
      }
    }

    return TRUE;
  }

  /**
   * Implements DrupalCacheInterface::isEmpty().
   */
  function isEmpty() {
    // It is unable to get empty state of memcached pool,
    // so we assume it is never empty.
    return FALSE;
  }

  /**
   * Validates cache item.
   * Checks if it is still valid and not expired.
   *
   * @param $cache
   *   Cache item loaded from memcache pool.
   *
   * @return bool|object
   *   Return FALSE if object is not valid.
   *   Return cache item otherwise.
   */
  protected function validateItem($cache) {

    // Check if cache object is valid.
    if (empty($cache->cid) || empty($cache->created) || !isset($cache->expire)) {
      return FALSE;
    }

    foreach ($this->wildcards as $wildcard => $flush_timestamp) {

      // See if wildcard is actual for current cache item.
      if ($cache->created <= $flush_timestamp) {

        // See if current cache id matches wildcard.
        if (strpos($cache->cid, $wildcard) === 0) {

          // Remove the current cache ID from the list of cache IDs in use.
          $this->removeCacheID($cache->cid);

          // Remove expired item from memcache.
          MemcacheStorageAPI::delete($cache->cid, $this->cacheBinName());

          // Return no value from cache.
          return FALSE;
        }
      }
    }

    // For temporary cache we should check last bin flush.
    // If temporary cache was created earlier than last cache flush - cache is invalid.
    if ($cache->expire == CACHE_TEMPORARY) {

      // Timestamp when cache bin was flushed last time.
      $last_bin_flush = variable_get('cache_flush_' . $this->bin, 0);

      // Custom behavior for cached pages.
      if ($this->bin == 'cache_page') {

        // Get timestamp of last page cache flush only if custom expiration is disabled.
        // In other case memcached will handle expiration of this cache itself.
        $custom_expiration = variable_get('memcache_storage_page_cache_custom_expiration', FALSE);
        if ($custom_expiration == FALSE) {

          // We should load variables for page cache earlier.
          // Drupal variables are load only during 4th bootstrap phase,
          // while page cache loads during 2th bootstrap phase.
          $variables = cache_get('variables', 'cache_bootstrap');
          if (!empty($variables->data) && !empty($variables->data['cache_flush_cache_page'])) {
            $last_bin_flush = $variables->data['cache_flush_cache_page'];
          }
        }
      }

      if ($cache->created <= $last_bin_flush) {

        // Remove the current cache ID from the list of cache IDs in use.
        $this->removeCacheID($cache->cid);

        // Remove expired item from memcache.
        MemcacheStorageAPI::delete($cache->cid, $this->cacheBinName());

        // Return no value from cache.
        return FALSE;
      }
    }

    return TRUE;
  }

  /**
   * Returns a cache bin name with namespace prefix.
   *
   * This small feature allows to manage flushing of entire cache bin.
   * To flush a cache bin we only need to change this number (bin index).
   */
  public  function cacheBinName() {
    return $this->bin . '_' . $this->getBinIndex();
  }

  /**
   * Increase cache bin index.
   * This operation changes all memcache keys in selected cache bin
   * so we simulate cache flush for it.
   */
  protected function increaseBinIndex() {

    $currentIndex = $this->getBinIndex();

    $indexes = &drupal_static('memcache_storage_bin_indexes', array());
    $indexes[$this->bin] = ++$currentIndex;

    MemcacheStorageAPI::set('memcache_storage_bin_indexes', $indexes, 0);
  }

  /**
   * Load cache bin index.
   * This index is part of memcache key and changes if cache bin should be cleared.
   */
  protected function getBinIndex() {

    $indexes = &drupal_static('memcache_storage_bin_indexes', array());

    if (empty($indexes[$this->bin])) {
      $indexes[$this->bin] = 1; // Initial index value.
      MemcacheStorageAPI::set('memcache_storage_bin_indexes', $indexes, CACHE_PERMANENT);
    }

    return $indexes[$this->bin];
  }

  /**
   * Add cache ID to the list of cache IDs
   * which are used in the cache bin.
   *
   * @param $cid
   *   Cache ID.
   *
   * @param $created
   *   Timestamp when cache item was created.
   */
  protected function addCacheID($cid, $created) {
    if (!isset($this->cache_ids[$cid])) {
      $this->cache_ids[$cid] = $created;
      MemcacheStorageAPI::set('memcache_storage_cids', $this->cache_ids, CACHE_PERMANENT, $this->cacheBinName());
    }
  }

  /**
   * Remove cache ID from the list of cache IDs
   * which are used in the cache bin.
   *
   * @param $cid
   *   Cache ID.
   */
  protected function removeCacheID($cid) {
    if (isset($this->cache_ids[$cid])) {
      unset($this->cache_ids[$cid]);
      MemcacheStorageAPI::set('memcache_storage_cids', $this->cache_ids, CACHE_PERMANENT, $this->cacheBinName());
    }
  }

  /**
   * Remove list of cache IDs from the list of cache IDs
   * which are used in the cache bin.
   *
   * @param $cids
   *   Array with cache IDs.
   */
  protected function removeCacheIDs($cids) {
    foreach ($cids as $cid) {
      unset($this->cache_ids[$cid]);
    }
    MemcacheStorageAPI::set('memcache_storage_cids', $this->cache_ids, CACHE_PERMANENT, $this->cacheBinName());
  }

  /**
   * Returns a list of cache ids which are currently in use.
   */
  public function getCacheIDs() {
    return $this->cache_ids;
  }

  /**
   * Returns a list of wildcard flushes for the current cache bin
   * which has not been invalidated yet.
   */
  public function getWildcards() {
    return $this->wildcards;
  }

}
