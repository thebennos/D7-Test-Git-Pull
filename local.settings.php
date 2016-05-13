<?php # local settings.php 
$databases['default']['default']['unix_socket'] = '/var/run/mysqld/mysqld.sock';
$databases['default']['default']['host'] = NULL;
$drupal_hash_salt = 'd721b6f756aa0e4840d1f7cc34f6f08f2ecd99be794db76282ebb191ef1166d2';
$conf['lock_inc'] = 'sites/testgitpull.front.om-manager.org/modules/apdqc/apdqc.lock.inc';
$conf['base_lock_inc'] = 'modules/o_contrib_seven/redis_edge/redis.lock.inc';
$databases['default']['default']['init_commands']['innodb_lock_wait_timeout'] = "SET SESSION innodb_lock_wait_timeout = 25";
$databases['default']['default']['init_commands']['isolation'] = "SET SESSION tx_isolation='READ-COMMITTED'";
$databases['default']['default']['mysql_db_type'] = 'mariadb';
$conf['session_inc'] = 'sites/testgitpull.front.om-manager.org/modules/apdqc/apdqc.session.inc';