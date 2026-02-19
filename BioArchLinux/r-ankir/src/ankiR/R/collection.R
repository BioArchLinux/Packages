#' Open an Anki collection
#'
#' Opens an Anki collection database and returns an object with methods to
#' access notes, cards, decks, note types, and review logs.
#'
#' @param path Path to collection.anki2 file. If NULL, auto-detected from
#'   default Anki location.
#' @param profile Profile name. If NULL, uses first available profile.
#' @return An anki_collection object with methods:
#'   \itemize{
#'     \item \code{notes()} - Get all notes
#'     \item \code{cards()} - Get all cards
#'     \item \code{revlog()} - Get review log
#'     \item \code{decks()} - Get deck information
#'     \item \code{models()} - Get note types (models)
#'     \item \code{tables()} - List all tables
#'     \item \code{close()} - Close database connection
#'   }
#' @export
#' @examples
#' \dontrun{
#' col <- anki_collection()
#' col$notes()
#' col$decks()
#' col$close()
#' }
anki_collection <- function(path = NULL, profile = NULL) {
  if (is.null(path)) {
    path <- anki_db_path(profile)
  }


  # Input validation

  if (!file.exists(path)) {
    stop("Database file not found: ", path, call. = FALSE)
  }


  if (!grepl("\\.anki2[1]?$", path, ignore.case = TRUE)) {
    warning("File doesn't have expected .anki2/.anki21 extension: ", path,
            call. = FALSE)
  }


  con <- DBI::dbConnect(RSQLite::SQLite(), path, flags = RSQLite::SQLITE_RO)


  structure(list(
    path = path,
    con = con,
    notes = function() read_notes(con),
    cards = function() read_cards(con),
    revlog = function() read_revlog(con),
    decks = function() read_decks(con),
    models = function() read_models(con),
    tables = function() DBI::dbListTables(con),
    close = function() DBI::dbDisconnect(con)
  ), class = "anki_collection")
}

#' @export
print.anki_collection <- function(x, ...) {
  cat("Anki Collection:", x$path, "\n")
  cat("Tables:", paste(x$tables(), collapse = ", "), "\n")
  invisible(x)
}

#' @keywords internal
read_notes <- function(con) {
  n <- DBI::dbReadTable(con, "notes")
  tibble::tibble(
    nid = n$id,
    mid = n$mid,
    tags = n$tags,
    flds = n$flds,
    sfld = n$sfld
  )
}

#' @keywords internal
read_cards <- function(con) {
  c <- DBI::dbReadTable(con, "cards")
  tibble::tibble(
    cid = c$id,
    nid = c$nid,
    did = c$did,
    type = c$type,
    queue = c$queue,
    due = c$due,
    ivl = c$ivl,
    reps = c$reps,
    lapses = c$lapses
  )
}

#' @keywords internal
read_revlog <- function(con) {
  r <- DBI::dbReadTable(con, "revlog")
  result <- tibble::tibble(
    rid = r$id,
    cid = r$cid,
    ease = r$ease,
    ivl = r$ivl,
    time = r$time,
    review_date = anki_timestamp_to_date(r$id)
  )
  # Add type column if it exists (0=learning, 1=review, 2=relearning, 3=filtered)
  if ("type" %in% names(r)) {
    result$review_type <- r$type
  }
  result
}

#' @keywords internal
read_decks <- function(con) {
  # Try new schema first (Anki 2.1.28+), fall back to legacy
  tables <- DBI::dbListTables(con)


  if ("decks" %in% tables) {
    # New schema: decks table exists
    d <- DBI::dbReadTable(con, "decks")
    return(tibble::tibble(
      did = d$id,
      name = d$name,
      mtime = d$mtime_secs
    ))
  }


  # Legacy schema: decks stored as JSON in col table
  col_data <- DBI::dbReadTable(con, "col")
  decks_json <- col_data$decks


  if (is.na(decks_json) || decks_json == "") {
    return(tibble::tibble(did = integer(), name = character()))
  }


  decks <- jsonlite::fromJSON(decks_json, simplifyVector = FALSE)


  tibble::tibble(
    did = as.numeric(names(decks)),
    name = vapply(decks, function(x) x$name %||% NA_character_,
                  FUN.VALUE = character(1))
  )
}

#' @keywords internal
read_models <- function(con) {
  # Try new schema first (Anki 2.1.28+), fall back to legacy
  tables <- DBI::dbListTables(con)


  if ("notetypes" %in% tables) {
    # New schema: notetypes table exists
    m <- DBI::dbReadTable(con, "notetypes")
    return(tibble::tibble(
      mid = m$id,
      name = m$name,
      mtime = m$mtime_secs
    ))
  }


  # Legacy schema: models stored as JSON in col table
  col_data <- DBI::dbReadTable(con, "col")
  models_json <- col_data$models


  if (is.na(models_json) || models_json == "") {
    return(tibble::tibble(mid = integer(), name = character(),
                          flds = list(), tmpls = list()))
  }


  models <- jsonlite::fromJSON(models_json, simplifyVector = FALSE)


  tibble::tibble(
    mid = as.numeric(names(models)),
    name = vapply(models, function(x) x$name %||% NA_character_,
                  FUN.VALUE = character(1)),
    flds = lapply(models, function(x) {
      if (!is.null(x$flds)) {
        vapply(x$flds, function(f) f$name %||% NA_character_,
               FUN.VALUE = character(1))
      } else {
        character(0)
      }
    }),
    tmpls = lapply(models, function(x) {
      if (!is.null(x$tmpls)) {
        vapply(x$tmpls, function(t) t$name %||% NA_character_,
               FUN.VALUE = character(1))
      } else {
        character(0)
      }
    })
  )
}

#' Read notes from Anki collection
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of notes with columns: nid, mid, tags, flds, sfld
#' @export
#' @examples
#' \dontrun{
#' anki_notes()
#' }
anki_notes <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  col$notes()
}

#' Read cards from Anki collection
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of cards with columns: cid, nid, did, type, queue, due,
#'   ivl, reps, lapses
#' @export
#' @examples
#' \dontrun{
#' anki_cards()
#' }
anki_cards <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  col$cards()
}

#' Read review log from Anki collection
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of review log entries with columns: rid, cid, ease, ivl,
#'   time, review_date
#' @export
#' @examples
#' \dontrun{
#' anki_revlog()
#' }
anki_revlog <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  col$revlog()
}

#' Read decks from Anki collection
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of decks with columns: did, name
#' @export
#' @examples
#' \dontrun{
#' anki_decks()
#' }
anki_decks <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  col$decks()
}

#' Read note types (models) from Anki collection
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of note types with columns: mid, name, flds (list of field
#'   names), tmpls (list of template names)
#' @export
#' @examples
#' \dontrun{
#' anki_models()
#' }
anki_models <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  col$models()
}

#' Read cards with FSRS-6 parameters
#'
#' Extracts cards along with their FSRS (Free Spaced Repetition Scheduler)
#' memory state parameters. FSRS-6 stores stability, difficulty, and
#' a per-card decay parameter in the card's data field.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of cards with FSRS parameters:
#'   \itemize{
#'     \item \code{cid} - Card ID
#'     \item \code{nid} - Note ID
#'     \item \code{did} - Deck ID
#'     \item \code{type} - Card type (0=new, 1=learning, 2=review, 3=relearning)
#'     \item \code{queue} - Queue (-1=suspended, 0=new, 1=learning, 2=review, 3=day learning, 4=preview)
#'     \item \code{due} - Due date/position
#'     \item \code{ivl} - Current interval in days
#'     \item \code{reps} - Number of reviews
#'     \item \code{lapses} - Number of lapses
#'     \item \code{stability} - FSRS stability (S) in days

#'     \item \code{difficulty} - FSRS difficulty (D), range 1-10
#'     \item \code{desired_retention} - Target retention rate
#'     \item \code{decay} - FSRS-6 decay parameter (w20), typically 0.1-0.8
#'   }
#' @importFrom jsonlite fromJSON
#' @export
#' @examples
#' \dontrun{
#' cards_fsrs <- anki_cards_fsrs()
#' # Calculate current retrievability
#' cards_fsrs$retrievability <- fsrs_retrievability(
#'   stability = cards_fsrs$stability,
#'   days_elapsed = as.numeric(Sys.Date() - as.Date("1970-01-01")) -
#'                  cards_fsrs$due / 86400,
#'   decay = cards_fsrs$decay
#' )
#' }
anki_cards_fsrs <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  c <- DBI::dbReadTable(col$con, "cards")

  # Parse FSRS JSON data from card's data field
  fsrs <- lapply(c$data, function(x) {
    if (is.na(x) || x == "" || x == "{}") {
      return(list(s = NA_real_, d = NA_real_, dr = NA_real_, decay = NA_real_))
    }
    j <- tryCatch(jsonlite::fromJSON(x), error = function(e) list())
    list(
      s = j$s %||% NA_real_,
      d = j$d %||% NA_real_,
      dr = j$dr %||% NA_real_,
      decay = j$decay %||% NA_real_
    )
  })

  tibble::tibble(
    cid = c$id,
    nid = c$nid,
    did = c$did,
    type = c$type,
    queue = c$queue,
    due = c$due,
    ivl = c$ivl,
    reps = c$reps,
    lapses = c$lapses,
    stability = vapply(fsrs, `[[`, FUN.VALUE = numeric(1), "s"),
    difficulty = vapply(fsrs, `[[`, FUN.VALUE = numeric(1), "d"),
    desired_retention = vapply(fsrs, `[[`, FUN.VALUE = numeric(1), "dr"),
    decay = vapply(fsrs, `[[`, FUN.VALUE = numeric(1), "decay")
  )
}

#' Calculate retrievability for FSRS cards
#'
#' Computes the current probability of recall using the FSRS-6 power forgetting
#' curve formula. This is the formula used by Anki 24.11+ with FSRS-6.
#'
#' @param stability Stability (S) in days - time for R to drop from 100% to 90%
#' @param days_elapsed Days since last review (t)
#' @param decay Decay parameter (w20). In FSRS-6, this is optimized per-user,
#'   typically between 0.1-0.8. Default 0.5 is FSRS-4.5 value. Use per-card
#'   decay from \code{anki_cards_fsrs()} for best accuracy.
#' @return Retrievability (R), probability of recall between 0 and 1
#' @details
#' The FSRS-6 forgetting curve formula is:
#' \deqn{R(t, S) = (1 + factor \cdot t / S)^{-decay}}
#' where \code{factor = 0.9^(-1/decay) - 1}.
#'
#' When \code{t = S}, retrievability equals 90% by definition.
#' @keywords internal
#' @examples
#' # Card with 30-day stability reviewed 15 days ago
#' \dontrun{
#' fsrs_retrievability(stability = 30, days_elapsed = 15)
#' }
fsrs_retrievability <- function(stability, days_elapsed, decay = 0.5) {
  # FSRS-6 formula: R = (1 + factor * t / S)^(-decay)

  # where factor = 0.9^(-1/decay) - 1
  # This ensures R = 0.9 when t = S
  factor <- 0.9^(-1 / decay) - 1
  (1 + factor * days_elapsed / stability)^(-decay)
}

#' Calculate next interval for desired retention
#'
#' Given a stability value and desired retention, calculates how many days
#' until the card should be reviewed.
#'
#' @param stability Stability (S) in days
#' @param desired_retention Target retention rate (default 0.9 = 90%)
#' @param decay Decay parameter (w20). Default 0.5 is FSRS-4.5 value.
#' @return Interval in days until next review
#' @details
#' Derived from the retrievability formula by solving for t:
#' \deqn{I(r, S) = S / factor \cdot (r^{-1/decay} - 1)}
#' @keywords internal
#' @examples
#' \dontrun{
#' # When should a card with 30-day stability be reviewed for 90% retention?
#' }
#' \dontrun{
#' \dontrun{
#' fsrs_interval(stability = 30, desired_retention = 0.9)
#' }
#' }
fsrs_interval <- function(stability, desired_retention = 0.9, decay = 0.5) {
  factor <- 0.9^(-1 / decay) - 1
  stability / factor * (desired_retention^(-1 / decay) - 1)
}
