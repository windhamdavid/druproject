<?php

/**
 * @file
 * Set up for the beer (basic) example.
 */
function migrate_example_beer_schema() {
  $schema['migrate_example_beer_account'] = migrate_example_beer_schema_account();
  $schema['migrate_example_beer_node'] = migrate_example_beer_schema_node();
  $schema['migrate_example_beer_comment'] = migrate_example_beer_schema_comment();
  $schema['migrate_example_beer_topic'] = migrate_example_beer_schema_topic();
  $schema['migrate_example_beer_topic_node'] = migrate_example_beer_schema_topic_node();

  // These two tables are primarily for testing the table_copy plugin.
  // They do provide some guidance for uri redirection per uri_map_redirect.php
  $schema['migrate_example_beer_legacy_urls'] = migrate_example_beer_schema_legacy_urls();
  $schema['migrate_example_beer_copy_urls'] = migrate_example_beer_schema_legacy_urls();

  return $schema;
}

function migrate_example_beer_install() {
  migrate_example_beer_content_type();
  migrate_example_beer_tags();
  migrate_example_beer_image();
  migrate_example_beer_country();

  // Populate our tables.
  migrate_example_beer_data_account();
  migrate_example_beer_data_node();
  migrate_example_beer_data_comment();
  migrate_example_beer_data_topic();
  migrate_example_beer_data_topic_node();
  migrate_example_beer_data_urls();
}

function migrate_example_beer_uninstall() {
  if ($vocabs = taxonomy_get_vocabularies('migrate_example_beer')) {
    // Grab key of the first returned vocabulary.
    taxonomy_del_vocabulary(key($vocabs));
  }
  migrate_example_beer_content_type_delete();
}

function migrate_example_beer_disable() {
  Migration::deregisterMigration('BeerTerm');
  Migration::deregisterMigration('BeerUser');
  Migration::deregisterMigration('BeerNode');
  Migration::deregisterMigration('BeerComment');
}

function migrate_example_beer_schema_node() {
  return array(
    'description' => 'Beers of the world.',
    'fields' => array(
      'bid'  => array(
        'type' => 'serial',
        'not null' => TRUE,
        'description' => 'Beer ID.',
      ),
      'name'  => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE,
      ),
      'body' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Full description of the beer.',
      ),
      'excerpt' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Abstract for this beer.',
      ),
      'countries' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Countries of origin. Multiple values, delimited by pipe',
      ),
      'aid' => array(
        'type' => 'int',
        'not null' => FALSE,
        'description' => 'Account Id of the author.',
      ),
      'image' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Image path',
      ),
    ),
    'primary key' => array('bid'),
  );
}

function migrate_example_beer_schema_topic() {
  return array(
    'description' => 'Categories',
    'fields' => array(
      'style'  => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE,
      ),
      'details' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
      ),
      'style_parent' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Parent topic, if any',
      ),
      'region' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Region first associated with this style',
      ),
      'hoppiness' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Relative hoppiness of the beer',
      ),
    ),
    'primary key' => array('style'),
  );
}

function migrate_example_beer_schema_topic_node() {
  return array(
    'description' => 'Beers topic pairs.',
    'fields' => array(
      'bid'  => array(
        'type' => 'int',
        'not null' => TRUE,
        'description' => 'Beer ID.',
      ),
      'style'  => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE,
        'description' => 'Topic name',
      ),
    ),
    'primary key' => array('style', 'bid'),
  );
}

function migrate_example_beer_schema_comment() {
  return array(
    'description' => 'Beers comments.',
    'fields' => array(
      'cid'  => array(
        'type' => 'serial',
        'not null' => TRUE,
        'description' => 'Comment ID.',
      ),
      'bid'  => array(
        'type' => 'int',
        'not null' => TRUE,
        'description' => 'Beer ID that is being commented upon',
      ),
      'cid_parent' => array(
        'type' => 'int',
        'not null' => FALSE,
        'description' => 'Parent comment ID in case of comment replies.',
      ),
      'subject' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Comment subject',
      ),
      'body' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Comment body',
      ),
      'name' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Comment name (if anon)',
      ),
      'mail' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Comment email (if anon)',
      ),
      'aid' => array(
        'type' => 'int',
        'not null' => FALSE,
        'description' => 'Account ID (if any).',
      ),
    ),
    'primary key' => array('cid'),
  );
}

function migrate_example_beer_schema_account() {
  return array(
    'description' => 'Beers accounts.',
    'fields' => array(
      'aid'  => array(
        'type' => 'serial',
        //'not null' => TRUE,
        'description' => 'Account ID',
      ),
      'status'  => array(
        'type' => 'int',
        'not null' => TRUE,
        'description' => 'Blocked_Allowed',
      ),
      'posted' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE,
        'description' => 'Registration date',
      ),
      'name' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Account name (for login)',
      ),
      'nickname' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Account name (for display)',
      ),
      'password' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Account password (raw)',
      ),
      'mail' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Account email',
      ),
      'sex' => array(
        'type' => 'int',
        'not null' => FALSE,
        'description' => 'Gender',
      ),
      'beers' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => FALSE,
        'description' => 'Favorite Beers',
      ),
    ),
    'primary key' => array('aid'),
  );
}

function migrate_example_beer_schema_legacy_urls() {
  return array(
    'description' => 'Stores legacy paths and destination ids for redirection.',
    'fields' => array(
      'id'  => array(
        'type' => 'int',
        'not null' => TRUE,
        'description' => 'Primary Key: ID.',
      ),
      'migration_name' => array(
        'type' => 'varchar',
        'length' => 50,
        'not null' => TRUE,
        'default' => '',
      ),
      'source_id' => array(
        'type' => 'int',
        'not null' => FALSE,
      ),
      'source_uri' => array(
        'type' => 'varchar',
        'length' => 500,
        'not null' => FALSE,
      ),
      'modificationdatetime' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    ),
    'primary key' => array('ID'),
    'indexes' => array(
      'source_uri' => array(array('source_uri', 255)),
    ),
  );
}

function migrate_example_beer_content_type() {
  // This code based on from standard.profile.
  // Insert default user-defined node types into the database.
  $types = array(
    array(
      'type' => 'migrate_example_beer',
      'name' => st('Beer'),
      'module' => 'node',
      'description' => st("Beer is what we drink."),
      'custom' => 1,
      'modified' => 1,
      'locked' => 1,
    ),
  );

  foreach ($types as $type) {
    $type = node_type_set_defaults($type);
    node_type_save($type);
  }
}

function migrate_example_beer_tags() {
  // Create a vocabulary named "Migrate Example Beer Styles", enabled for the 'migrate_example_beer' content type.
  $description = st('Use tags to group beers on similar topics into categories.');
  $help = st('Enter a comma-separated list of words to describe your content.');
  $vocabulary = array(
    'name' => 'Migrate Example Beer Styles',
    'description' => $description,
    'machine_name' => 'migrate_example_beer_styles',
    'help' => $help,
    'multiple' => 1,
    'required' => 0,
    'hierarchy' => 1,
    'relations' => 0,
    'module' => 'migrate_example',
    'nodes' => array('migrate_example_beer' => 1),
  );

  taxonomy_vocabulary_save($vocabulary);
}

// Create an image field named "Migrate Example Image", enabled for the 'Beer' content type.
function migrate_example_beer_image() {
  $field = array(
    'field_name' => 'field_migrate_example_image',
    'type' => 'filefield',
    'widget_type' => 'imagefield_widget',
    'type_name' => 'migrate_example_beer',
    'label' => t('Image'),
  );
  content_field_instance_create($field);
}

// Create a text field named "Countries", enabled for the 'Beer' content type.
function migrate_example_beer_country() {
  $field = array(
    'field_name' => 'field_migrate_example_country',
    'type' => 'text',
    'widget_type' => 'text_textfield',
    'type_name' => 'migrate_example_beer',
    'label' => t('Country'),
  );
  content_field_instance_create($field);
}

function migrate_example_beer_content_type_delete() {
  $bundle = 'migrate_example_beer';
/*  $field_names = array('migrate_example_beer_styles', 'field_migrate_example_image', 'field_migrate_example_country');
  foreach ($field_names as $field_name) {
    $instance = field_info_instance('node', $field_name, $bundle);
    field_delete_instance($instance);
    field_delete_field($field_name);
  }*/
  node_type_delete($bundle);
}

function migrate_example_beer_data_node() {
  // Use high bid numbers to avoid overwriting an existing node id.
  $data = array(
    array(99999999, 'Heineken', 'Blab Blah Blah Green', 'Green', 'Netherlands|Belgium', 0, 'heineken.jpg'), // comes with migrate_example project.
    array(99999998, 'Miller Lite', 'We love Miller Brewing', 'Tasteless', 'USA|Canada', 1, NULL),
    array(99999997, 'Boddington', 'English occassionally get something right', 'A treat', 'United Kingdom', 1, NULL),
  );
  foreach ($data as $row) {
    db_query("INSERT INTO {migrate_example_beer_node}
              (bid, name, body, excerpt, countries, aid, image)
              VALUES(%d, '%s', '%s', '%s', '%s', %d, '%s')",
             $row);
  }
}

// Note that alice has duplicate username. Tests our dedupe().
// @TODO duplicate email also.
function migrate_example_beer_data_account() {
  $data = array(
    array(1, '2010-03-30 10:31:05', 'alice', 'alice hot pants', 'alicepass', 'alice@example.com', '1', '99999999|99999998|99999997'),
    array(1, '2010-04-04 10:31:05', 'alice', 'alice duplicate pants', 'alicepass', 'alice2@example.com', '1', '99999999|99999998|99999997'),
    array(0, '2007-03-15 10:31:05', 'bob', 'rebob', 'bobpass', 'bob@example.com', '1', '99999999|99999997'),
    array(1, '2004-02-29 10:31:05', 'charlie', 'charlie chocolate', 'mykids', 'charlie@example.com', '0', '99999999|99999998'),
  );
  foreach ($data as $row) {
    db_query("INSERT INTO {migrate_example_beer_account}
              (status, posted, name, nickname, password, mail, sex, beers)
              VALUES(%d, '%s', '%s', '%s', '%s', '%s', %d, '%s')",
             $row);
  }
}

function migrate_example_beer_data_comment() {
  $data = array(
    array(99999998, NULL, 'im first', 'hot body', 'alice', 'alice@example.com', 0),
    array(99999998, NULL, 'im second', 'hot body', 'alice', 'alice@example.com', 0),
    array(99999999, NULL, 'im parent', 'hot body', 'alice', 'alice@example.com', 0),
    array(99999999, 1, 'im child', 'cold body', 'bob', NULL, 1),
    array(99999999, 2, 'im grandchild', 'bitter body', 'charlie@example.com', NULL, 1),
  );
  foreach ($data as $row) {
    db_query("INSERT INTO {migrate_example_beer_comment}
              (bid, cid_parent, subject, body, name, mail, aid)
              VALUES(%d, %d, '%s', '%s', '%s', '%s', %d)",
             $row);
  }
}

function migrate_example_beer_data_topic() {
  $data = array(
    array('ale', 'traditional', NULL, 'Medieval British Isles', 'Medium'),
    array('red ale', 'colorful', 'ale', NULL, NULL),
    array('pilsner', 'refreshing', NULL, 'Pilsen, Bohemia (now Czech Republic)', 'Low'),
  );
  foreach ($data as $row) {
    db_query("INSERT INTO {migrate_example_beer_topic}
              (style, details, style_parent, region, hoppiness)
              VALUES('%s', '%s', '%s', '%s', '%s')",
             $row);
  }
}

function migrate_example_beer_data_topic_node() {
  $data = array(
    array(99999999, 'pilsner'),
    array(99999999, 'red ale'),
    array(99999998, 'red ale'),
  );
  foreach ($data as $row) {
    db_query("INSERT INTO {migrate_example_beer_topic_node}
              (bid, style)
              VALUES(%d, '%s')",
             $row);
  }
}

function migrate_example_beer_data_urls() {
  $data = array(
    array(1, 'BeerNode', 99999997, 'the_boddington/main', strtotime('2010-04-12 08:32:06')),
    array(2, 'BeerNode', 99999998, 'Miller Lite taste', strtotime('2010-04-12 08:32:05')),
    array(3, 'BeerNode', 99999999, 'green wonder', strtotime('2010-04-12 08:32:03')),
  );
  foreach ($data as $row) {
    db_query("INSERT INTO {migrate_example_beer_legacy_urls}
              (id, migration_name, source_id, source_uri, modificationdatetime)
              VALUES(%d, '%s', %d, '%s', '%s')",
             $row);
  }
}
