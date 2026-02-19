test_that("anki_collection opens database and provides methods", {
  skip_if_not_installed("withr")

  withr::with_tempdir({
    db_path <- file.path(getwd(), "collection.anki2")
    create_test_db(db_path)

    col <- anki_collection(db_path)
    expect_s3_class(col, "anki_collection")

    # Check all methods exist
    expect_type(col$notes, "closure")
    expect_type(col$cards, "closure")
    expect_type(col$revlog, "closure")
    expect_type(col$decks, "closure")
    expect_type(col$models, "closure")
    expect_type(col$tables, "closure")
    expect_type(col$close, "closure")

    col$close()
  })
})

test_that("anki_collection validates input", {
  expect_error(
    anki_collection("/nonexistent/path/collection.anki2"),
    "not found"
  )

  withr::with_tempdir({
    # Create a file with wrong extension
    writeLines("test", "test.txt")
    expect_warning(
      anki_collection("test.txt"),
      "extension"
    )
  })
})

test_that("read functions return tibbles with expected columns", {
  skip_if_not_installed("withr")

  withr::with_tempdir({
    db_path <- file.path(getwd(), "collection.anki2")
    create_test_db(db_path)

    col <- anki_collection(db_path)
    on.exit(col$close())

    # Notes
    notes <- col$notes()
    expect_s3_class(notes, "tbl_df")
    expect_true(all(c("nid", "mid", "tags", "flds", "sfld") %in% names(notes)))
    expect_equal(nrow(notes), 1)

    # Cards
    cards <- col$cards()
    expect_s3_class(cards, "tbl_df")
    expect_true(all(c("cid", "nid", "did", "type", "queue", "due", "ivl", "reps", "lapses") %in% names(cards)))
    expect_equal(nrow(cards), 1)

    # Revlog
    revlog <- col$revlog()
    expect_s3_class(revlog, "tbl_df")
    expect_true(all(c("rid", "cid", "ease", "ivl", "time", "review_date") %in% names(revlog)))

    # Decks
    decks <- col$decks()
    expect_s3_class(decks, "tbl_df")
    expect_true(all(c("did", "name") %in% names(decks)))
    expect_equal(decks$name[[1]], "Default")

    # Models
    models <- col$models()
    expect_s3_class(models, "tbl_df")
    expect_true(all(c("mid", "name") %in% names(models)))
    expect_equal(models$name[[1]], "Basic")
  })
})

test_that("standalone functions work", {
  skip_if_not_installed("withr")

  withr::with_tempdir({
    db_path <- file.path(getwd(), "collection.anki2")
    create_test_db(db_path)

    expect_s3_class(anki_notes(db_path), "tbl_df")
    expect_s3_class(anki_cards(db_path), "tbl_df")
    expect_s3_class(anki_decks(db_path), "tbl_df")
    expect_s3_class(anki_models(db_path), "tbl_df")
    expect_s3_class(anki_revlog(db_path), "tbl_df")
  })
})

test_that("anki_cards_fsrs extracts FSRS parameters", {
  skip_if_not_installed("withr")

  withr::with_tempdir({
    db_path <- file.path(getwd(), "collection.anki2")
    create_test_db(db_path)

    cards_fsrs <- anki_cards_fsrs(db_path)

    expect_s3_class(cards_fsrs, "tbl_df")
    expect_true(all(c("stability", "difficulty", "desired_retention", "decay") %in% names(cards_fsrs)))

    # Check FSRS values were extracted
    expect_equal(cards_fsrs$stability[1], 30.5)
    expect_equal(cards_fsrs$difficulty[1], 5.2)
    expect_equal(cards_fsrs$desired_retention[1], 0.9)
    expect_equal(cards_fsrs$decay[1], 0.3)
  })
})
