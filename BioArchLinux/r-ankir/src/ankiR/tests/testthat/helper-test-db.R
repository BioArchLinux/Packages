# Helper functions for tests

# Create a minimal test database
create_test_db <- function(path) {
  con <- DBI::dbConnect(RSQLite::SQLite(), path)
  on.exit(DBI::dbDisconnect(con))

  # Create minimal schema
  DBI::dbExecute(con, "
    CREATE TABLE col (
      id INTEGER PRIMARY KEY,
      crt INTEGER,
      mod INTEGER,
      scm INTEGER,
      ver INTEGER,
      dty INTEGER,
      usn INTEGER,
      ls INTEGER,
      conf TEXT,
      models TEXT,
      decks TEXT,
      dconf TEXT,
      tags TEXT
    )
 ")

  DBI::dbExecute(con, "
    CREATE TABLE notes (
      id INTEGER PRIMARY KEY,
      guid TEXT,
      mid INTEGER,
      mod INTEGER,
      usn INTEGER,
      tags TEXT,
      flds TEXT,
      sfld TEXT,
      csum INTEGER,
      flags INTEGER,
      data TEXT
    )
  ")

  DBI::dbExecute(con, "
    CREATE TABLE cards (
      id INTEGER PRIMARY KEY,
      nid INTEGER,
      did INTEGER,
      ord INTEGER,
      mod INTEGER,
      usn INTEGER,
      type INTEGER,
      queue INTEGER,
      due INTEGER,
      ivl INTEGER,
      factor INTEGER,
      reps INTEGER,
      lapses INTEGER,
      left INTEGER,
      odue INTEGER,
      odid INTEGER,
      flags INTEGER,
      data TEXT
    )
  ")

  DBI::dbExecute(con, "
    CREATE TABLE revlog (
      id INTEGER PRIMARY KEY,
      cid INTEGER,
      usn INTEGER,
      ease INTEGER,
      ivl INTEGER,
      lastIvl INTEGER,
      factor INTEGER,
      time INTEGER,
      type INTEGER
    )
  ")

  # Insert test data
  decks_json <- '{"1": {"name": "Default", "id": 1}}'
  models_json <- '{"1234": {"name": "Basic", "flds": [{"name": "Front"}, {"name": "Back"}], "tmpls": [{"name": "Card 1"}]}}'

  DBI::dbExecute(con, sprintf(
    "INSERT INTO col VALUES (1, 0, 0, 0, 11, 0, 0, 0, '{}', '%s', '%s', '{}', '')",
    models_json, decks_json
  ))

  DBI::dbExecute(con, "INSERT INTO notes VALUES (1, 'abc', 1234, 0, 0, '', 'Front text\x1fBack text', 'Front text', 0, 0, '')")
  DBI::dbExecute(con, "INSERT INTO cards VALUES (1, 1, 1, 0, 0, 0, 2, 2, 100, 30, 2500, 5, 1, 0, 0, 0, 0, '{\"s\": 30.5, \"d\": 5.2, \"dr\": 0.9, \"decay\": 0.3}')")
  DBI::dbExecute(con, "INSERT INTO revlog VALUES (1609459200000, 1, 0, 3, 1, 0, 2500, 5000, 0)")

  invisible(path)
}
