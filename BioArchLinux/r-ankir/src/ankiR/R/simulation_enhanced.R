#' Anki Schema Version Detection
#'
#' Detect the Anki database schema version to help debug compatibility issues.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with schema information
#' @export
#'
#' @examples
#' \dontrun{
#' schema <- anki_schema_version()
#' }
anki_schema_version <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  db <- col$db
  
  # Get schema version from col table
  schema_ver <- tryCatch({
    res <- DBI::dbGetQuery(db, "SELECT ver FROM col")
    res$ver[1]
  }, error = function(e) NA)
  
  # Check for FSRS data columns
  card_cols <- DBI::dbListFields(db, "cards")
  has_fsrs <- "data" %in% card_cols
  
  # Get table list
  tables <- DBI::dbListTables(db)
  
  # Check for revlog columns
  revlog_cols <- DBI::dbListFields(db, "revlog")
  
  # Get some stats
  col_data <- tryCatch({
    DBI::dbGetQuery(db, "SELECT crt, mod FROM col")
  }, error = function(e) NULL)
  
  created_date <- if (!is.null(col_data)) {
    as.POSIXct(col_data$crt[1], origin = "1970-01-01")
  } else NA
  
  modified_date <- if (!is.null(col_data)) {
    as.POSIXct(col_data$mod[1] / 1000, origin = "1970-01-01")
  } else NA
  
  # Detect Anki version features
  features <- list(
    fsrs = has_fsrs,
    deck_config_in_json = "dconf" %in% tables || schema_ver >= 11,
    media_db = file.exists(file.path(dirname(col$path), "media.db2")),
    new_scheduler = "originalDue" %in% revlog_cols || schema_ver >= 18
  )
  
  # Determine approximate Anki version
  anki_version <- if (schema_ver >= 18) {
    "Anki 23.10+ (v3 scheduler, FSRS support)"
  } else if (schema_ver >= 15) {
    "Anki 2.1.45+ (v3 scheduler)"
  } else if (schema_ver >= 11) {
    "Anki 2.1.x"
  } else {
    "Legacy Anki"
  }
  
  list(
    schema_version = schema_ver,
    estimated_anki_version = anki_version,
    collection_created = created_date,
    collection_modified = modified_date,
    tables = tables,
    card_columns = card_cols,
    revlog_columns = revlog_cols,
    features = features,
    path = col$path
  )
}

#' Quick Collection Summary
#'
#' Get a one-liner overview of your Anki collection.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param print Whether to print the summary (default TRUE)
#' @return Invisibly returns a list with summary stats
#' @export
#'
#' @examples
#' \dontrun{
#' anki_quick_summary()
#' }
anki_quick_summary <- function(path = NULL, profile = NULL, print = TRUE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()
  
  # Basic counts
  total_cards <- nrow(cards)
  total_notes <- nrow(notes)
  mature <- sum(cards$ivl >= 21)
  young <- sum(cards$queue == 2 & cards$ivl < 21)
  new_cards <- sum(cards$queue == 0)
  suspended <- sum(cards$queue == -1)
  
  # Due today
  today <- as.numeric(Sys.Date() - as.Date("1970-01-01"))
  due_today <- sum(cards$queue == 2 & cards$due <= today)
  
  # Reviews today
  today_revs <- revlog[revlog$review_date == Sys.Date(), ]
  reviews_today <- nrow(today_revs)
  
  # Streak
  streak <- anki_streak(path = col$path)
  
  # 7-day retention
  week_ago <- Sys.Date() - 7
  recent_revs <- revlog[revlog$review_date >= week_ago, ]
  # Filter to review type if available
  if ("review_type" %in% names(recent_revs) && nrow(recent_revs) > 0) {
    recent_revs <- recent_revs[recent_revs$review_type == 1, ]
  }
  retention_7d <- if (nrow(recent_revs) > 0) {
    round(sum(recent_revs$ease > 1) / nrow(recent_revs) * 100, 1)
  } else NA
  
  result <- list(
    total_cards = total_cards,
    total_notes = total_notes,
    mature = mature,
    young = young,
    new_cards = new_cards,
    suspended = suspended,
    due_today = due_today,
    reviews_today = reviews_today,
    current_streak = streak$current,
    retention_7d = retention_7d
  )
  
  if (print) {
    cat(sprintf(
      "Anki: %d cards (%d mature, %d young, %d new) | Due today: %d | Reviews today: %d | Streak: %d days | 7d retention: %s%%\n",
      total_cards, mature, young, new_cards, due_today, reviews_today,
      streak$current, ifelse(is.na(retention_7d), "N/A", retention_7d)
    ))
  }
  
  invisible(result)
}

#' Today's Activity Summary
#'
#' Detailed breakdown of today's Anki activity.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with today's activity breakdown
#' @export
#'
#' @examples
#' \dontrun{
#' anki_today()
#' }
anki_today <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  revlog <- col$revlog()
  cards <- col$cards()
  
  # Filter to today
  today <- Sys.Date()
  today_revs <- revlog[revlog$review_date == today, ]
  
  if (nrow(today_revs) == 0) {
    return(list(
      date = today,
      total_reviews = 0,
      message = "No reviews yet today"
    ))
  }
  
  # Breakdown by ease
  ease_breakdown <- table(factor(today_revs$ease, levels = 1:4))
  names(ease_breakdown) <- c("Again", "Hard", "Good", "Easy")
  
  # Breakdown by type
  type_breakdown <- table(factor(today_revs$type, levels = 0:3))
  names(type_breakdown) <- c("Learning", "Review", "Relearn", "Filtered")
  
  # Time spent
  time_spent_sec <- sum(today_revs$time) / 1000
  time_spent_min <- time_spent_sec / 60
  
  # Unique cards
  unique_cards <- length(unique(today_revs$cid))
  
  # Retention
  review_revs <- today_revs[today_revs$type == 1, ]
  retention <- if (nrow(review_revs) > 0) {
    round(sum(review_revs$ease > 1) / nrow(review_revs) * 100, 1)
  } else NA
  
  # Average time per card
  avg_time <- mean(today_revs$time) / 1000
  
  # Due remaining
  today_num <- as.numeric(today - as.Date("1970-01-01"))
  due_remaining <- sum(cards$queue == 2 & cards$due <= today_num) - 
    length(unique(today_revs$cid[today_revs$type == 1]))
  due_remaining <- max(0, due_remaining)
  
  list(
    date = today,
    total_reviews = nrow(today_revs),
    unique_cards = unique_cards,
    time_spent_minutes = round(time_spent_min, 1),
    avg_time_seconds = round(avg_time, 1),
    ease_breakdown = as.list(ease_breakdown),
    type_breakdown = as.list(type_breakdown),
    retention = retention,
    due_remaining = due_remaining,
    again_rate = round(ease_breakdown["Again"] / nrow(today_revs) * 100, 1),
    easy_rate = round(ease_breakdown["Easy"] / nrow(today_revs) * 100, 1)
  )
}
