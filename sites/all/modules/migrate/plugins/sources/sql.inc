<?php

/**
 * @file
 * Define a MigrateSource for importing from Drupal connections
 */

/**
 * Implementation of MigrateSource, to handle imports from Drupal connections.
 */
class MigrateSourceSQL extends MigrateSource {
  /**
   * The SQL query objects from which to obtain data, and counts of data
   *
   * @var SelectQuery
   */
  protected $originalQuery, $query, $countQuery;

  /**
   * The result object from executing the query - traversed to process the
   * incoming data.
   *
   * @var DatabaseStatementInterface
   */
  protected $result;

  /**
   * Number of eligible rows processed so far (used for itemlimit checking)
   *
   * @var int
   */
  protected $numProcessed = 0;

  /**
   * List of available source fields.
   *
   * @var array
   */
  protected $fields = array();

  /**
   * If the map is a MigrateSQLMap, and the table is compatible with the
   * source query, we can join directly to the map and make things much faster
   * and simpler.
   *
   * @var boolean
   */
  protected $mapJoinable = FALSE;
  // Dynamically set whether the map is joinable - not really for production use,
  // this is primarily to support simpletests
  public function setMapJoinable($map_joinable) {
    $this->mapJoinable = $map_joinable;
  }

  /**
   * Whether this source is configured to use a highwater mark, and there is
   * a highwater mark present to use.
   *
   * @var boolean
   */
  protected $usingHighwater = FALSE;

  /**
   * Whether, in the current iteration, we have reached the highwater mark.
   *
   * @var boolen
   */
  protected $highwaterSeen = FALSE;

  /**
   * Return an options array for PDO sources.
   *
   * @param boolean $map_joinable
   *  Indicates whether the map table can be joined directly to the source query.
   */
  static public function options($map_joinable) {
    return compact('map_joinable');
  }

  /**
   * Simple initialization.
   *
   * @param SelectQuery $query
   *  The query we are iterating over.
   * @param array $fields
   *  Optional - keys are field names, values are descriptions. Use to override
   *  the default descriptions, or to add additional source fields which the
   *  migration will add via other means (e.g., prepareRow()).
   * @param SelectQuery $count_query
   *  Optional - an explicit count query, primarily used when counting the
   *  primary query is slow.
   * @param boolean $options
   *  Options applied to this source.
   */
  public function __construct(SelectQuery $query, array $fields = array(),
        SelectQuery $count_query = NULL, array $options = array()) {
    parent::__construct();
    $this->originalQuery = $query;
    $this->query = clone $query;
    $this->fields = $fields;
    if (is_null($count_query)) {
      $this->countQuery = clone $query->countQuery();
    }
    else {
      $this->countQuery = $count_query;
    }

    if (isset($options['map_joinable'])) {
      $this->mapJoinable = $options['map_joinable'];
    }
    else {
      // TODO: We want to automatically determine if the map table can be joined
      // directly to the query, but this won't work unless/until
      // http://drupal.org/node/802514 is committed, assume joinable for now
      $this->mapJoinable = TRUE;
/*      // To be able to join the map directly, it must be a PDO map on the same
      // connection, or a compatible connection
      $map = $migration->getMap();
      if (is_a($map, 'MigrateSQLMap')) {
        $map_options = $map->getConnection()->getConnectionOptions();
        $query_options = $this->query->connection()->getConnectionOptions();

        // Identical options means it will work
        if ($map_options == $query_options) {
          $this->mapJoinable = TRUE;
        }
        else {
          // Otherwise, the one scenario we know will work is if it's MySQL and
          // the credentials match (SQLite too?)
          if ($map_options['driver'] == 'mysql' && $query_options['driver'] == 'mysql') {
            if ($map_options['host'] == $query_options['host'] &&
                $map_options['port'] == $query_options['port'] &&
                $map_options['username'] == $query_options['username'] &&
                $map_options['password'] == $query_options['password']) {
              $this->mapJoinable = TRUE;
            }
          }
        }
      }*/
    }
  }


  /**
   * Return a string representing the source query.
   *
   * @return string
   */
  public function __toString() {
    return (string) $this->query;
  }

  /**
   * Returns a list of fields available to be mapped from the source query.
   *
   * @return array
   *  Keys: machine names of the fields (to be passed to addFieldMapping)
   *  Values: Human-friendly descriptions of the fields.
   */
  public function fields() {
    $fields = array();
    $queryFields = $this->query->getFields();

    if ($queryFields) {
      // Not much we can do in terms of describing the fields without manual intervention
      foreach ($queryFields as $field_name => $field_info) {
        // Lower case, because Drupal forces lowercase on fetch
        $fields[drupal_strtolower($field_name)] = drupal_strtolower(
          $field_info['table'] . '.' . $field_info['field']);
      }
    }

/*
 * Handle queries without explicit field lists
 * TODO: Waiting on http://drupal.org/node/814312
    $info = Database::getConnectionInfo($query->getConnection());
    $database = $info['default']['database'];
    foreach ($this->query->getTables() as $table) {
      if (isset($table['all_fields']) && $table['all_fields']) {

        $database = 'plants';
        $table = $table['table'];
        $sql = 'SELECT column_name
                FROM information_schema.columns
                WHERE table_schema=:database AND table_name = :table
                ORDER BY ordinal_position';
        $result = dbtng_query($sql, array(':database' => $database, ':table' => $table));
        foreach ($result as $row) {
          $fields[drupal_strtolower($row->column_name)] = drupal_strtolower(
            $table . '.' . $row->column_name);
        }
      }
    }*/
    $expressionFields = $this->query->getExpressions();
    foreach ($expressionFields as $field_name => $field_info) {
      // Lower case, because Drupal forces lowercase on fetch
      $fields[drupal_strtolower($field_name)] = drupal_strtolower($field_info['alias']);
    }

    // Any caller-specified fields with the same names as extracted fields will
    // override them; any others will be added
    if ($this->fields) {
      $fields = $this->fields + $fields;
    }

    return $fields;
  }

  /**
   * Return a count of all available source records.
   *
   * @param boolean $refresh
   *  If TRUE, or if there is no cached count, perform a SQL COUNT query to
   *  retrieve and cache the number of rows in the query. Otherwise, return
   *  the last cached value.
   *
   *  TODO: Implement caching
   */
  public function count($refresh = FALSE) {
    $count = $this->countQuery->execute()->fetchField();
    return $count;
  }

  /**
   * Implementation of Iterator::rewind() - called before beginning a foreach loop.
   */
  public function rewind() {
    $migration = Migration::currentMigration();
    $this->result = NULL;
    $this->currentRow = NULL;
    $this->numProcessed = 0;
    $this->highwaterSeen = FALSE;
    $this->query = clone $this->originalQuery;

    // Get the keys, for potential use in joining map/message table, or enforcing
    // idlist
    $map = $migration->getMap();
    $keys = array();
    foreach ($map->getSourceKey() as $field_name => $field_schema) {
      if (isset($field_schema['alias'])) {
        $field_name = $field_schema['alias'] . '.' . $field_name;
      }
      $keys[] = $field_name;
    }

    // The rules for determining what conditions to add to the query are as
    // follows (applying first applicable rule)
    // 1. If idlist is provided, then only process items in that list (AND key IN (idlist))
    // 2. Otherwise, If there's no highwater mark, process items which are not in the map table, or
    //    are marked needs_update=1 in the map table
    //    a. mapJoinable: AND (key not in map OR map.needs_update=1)
    //    b. !mapJoinable: Apply map logic in next()
    // 3. If there is a highwater mark (and no idlist), process items above the highwater mark, plus any
    //      items with needs_update=1
    //    For now, we set mapJoinable=FALSE in this case, until we work out exactly
    //    how to manipulate the query.
    if ($migration->getOption('idlist')) {
      // Only supp for single-field key
      $this->query->condition($keys[0], explode(',', $migration->getOption('idlist')), 'IN');
      $this->usingHighwater = FALSE;
    }
    else {
      $highwaterField = $migration->getHighwaterField();
      $this->usingHighwater = isset($highwaterField['name']);
      if ($this->usingHighwater) {
        $this->mapJoinable = FALSE;
      }
      if ($this->mapJoinable) {
        // Build the joins to the map and message tables. Because the source key
        // could have multiple fields, we need to build things up.
        // The logic is that we want to include all source rows which have no
        // existing map or message table entries, or which have map table entry
        // marked with needs_update=1. Except with highwater marks, where we don't care
        // about the existence of map/message table entries, only needs_update.
        $first = TRUE;
        $map_join = $msg_join = $map_condition = $msg_condition = '';
        $count = 1;

        foreach ($map->getSourceKey() as $field_name => $field_schema) {
          if (isset($field_schema['alias'])) {
            $field_name = $field_schema['alias'] . '.' . $field_name;
          }
          $map_key = 'sourceid' . $count++;
          if ($first) {
            $first = FALSE;
          }
          else {
            $map_join .= ' AND ';
            if (!$this->usingHighwater) {
              $msg_join .= ' AND ';
              $map_condition .= ' OR ';
              $msg_condition .= ' OR ';
            }
          }
          $map_join .= "$field_name = map.$map_key";
          if (!$this->usingHighwater) {
            $msg_join .= "$field_name = msg.$map_key";
            $map_condition .= "map.$map_key IS NULL";
            $msg_condition .= "msg.$map_key IS NULL";
          }
        }
        $this->query->leftJoin($map->getMapTable(), 'map', $map_join);
        $map_condition .= ' OR map.needs_update = 1';
        $this->query->leftJoin($map->getMessageTable(), 'msg', $msg_join);
        $this->query->where($map_condition);
        $this->query->where($msg_condition);

        // And as long as we have the map table, get the destination ID, the
        // import hook will need it to identify the existing destination object.
        // Alias to reduce possible collisions.
        $count = 1;
        foreach ($map->getDestinationKey() as $field_name => $field_schema) {
          $map_key = 'destid' . $count++;
          $this->query->addField('map', $map_key, "migrate_map_$map_key");
        }
        $this->query->addField('map', 'needs_update');
        if ($migration->getOption('itemlimit')) {
          $this->query->range(0, $migration->getOption('itemlimit'));
        }
      }
    }

    migrate_instrument_start('MigrateSourceSQL execute');
    $migration->showMessage(dpq($this->query), 'debug');
    $this->result = $this->query->execute();
    migrate_instrument_stop('MigrateSourceSQL execute');

    // Load up the first row
    $this->next();
  }

  /**
   * Implementation of Iterator::next() - called at the bottom of the loop implicitly,
   * as well as explicitly from rewind().
   */
  public function next() {
    $migration = Migration::currentMigration();
    $this->currentRow = NULL;
    $this->currentKey = NULL;
    // If we couldn't add the itemlimit to the query directly, enforce it here
    if (!$this->mapJoinable) {
      $itemlimit = $migration->getOption('itemlimit');
      if ($itemlimit && $this->numProcessed >= $itemlimit) {
        return;
      }
    }

    // get next row
    migrate_instrument_start('MigrateSourceSQL next');
    $map = $migration->getMap();
    while ($this->currentRow = $this->result->fetchObject()) {
      foreach ($map->getSourceKey() as $field_name => $field_schema) {
        $this->currentKey[$field_name] = $this->currentRow->$field_name;
      }

      if (!$this->mapJoinable) {
        $map_row = $migration->getMap()->getRowBySource($this->currentKey);
        if (!$map_row) {
          // Unmigrated row, take it
        }
        elseif ($map_row && $map_row['needs_update'] == 1) {
          // We always want to take this row if needs_update = 1
        }
        else {
          if ($this->usingHighwater) {
            if ($this->highwaterSeen) {
              // We've passed the highwater mark previously, so no need to check again
            }
            else {
              // With highwater, we want to take this row if it's above the highwater
              // mark
              $highwaterField = $migration->getHighwaterField();
              $highwaterField = $highwaterField['name'];
              if ($this->currentRow->$highwaterField <= $migration->getHighwater()) {
                continue;
              }
            }
          }
          else {
            // With no highwater, we want to take this row if it's not in the map table
            if ($map_row) {
              continue;
            }
          }
        }
        // Add map info to the row, if present
        if ($map_row) {
          foreach ($map_row as $field => $value) {
            $field = 'migrate_map_' . $field;
            $this->currentRow->$field = $value;
          }
        }
      }

      // Add some debugging, just for the first row. Requires --debug
      if (empty($this->numProcessed)) {
        $migration->showMessage(print_r($this->currentRow, TRUE), 'debug');
      }

      // Allow the Migration to prepare this row. prepareRow() can return boolean
      // FALSE to stop processing this row. To add/modify fields on the
      // result, modify $row by reference.
      $return = TRUE;
      if (method_exists($migration, 'prepareRow')) {
        $return = $migration->prepareRow($this->currentRow);
      }

      if ($return !== FALSE) {
        $this->numProcessed++;
        break;
      }
    }
    if (!is_object($this->currentRow)) {
      $this->currentRow = NULL;
    }
    migrate_instrument_stop('MigrateSourceSQL next');
  }
}
