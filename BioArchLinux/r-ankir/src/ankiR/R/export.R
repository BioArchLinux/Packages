#' Export deck to CSV
#'
#' Exports cards from a deck to a CSV file with note content.
#'
#' @param deck Deck name or ID (partial match supported)
#' @param file Output file path (default: deck name + .csv)
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param include_html If FALSE, strip HTML tags from fields
#' @return Invisibly returns the exported data
#' @export
#' @examples
#' \dontrun{
#' anki_to_csv("Default")
#' anki_to_csv("Medical", file = "medical_cards.csv")
#' }
anki_to_csv <- function(deck, file = NULL, path = NULL, profile = NULL, include_html = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
 
  # Find matching deck
  if (is.numeric(deck)) {
    deck_ids <- deck
  } else {
    deck_ids <- decks$did[grepl(deck, decks$name, ignore.case = TRUE)]
  }
 
  if (length(deck_ids) == 0) {
    stop("No deck found matching: ", deck)
  }
 
  # Filter cards
  cards <- cards[cards$did %in% deck_ids, ]
 
  if (nrow(cards) == 0) {
    message("No cards found in deck")
    return(invisible(tibble::tibble()))
  }
 
  # Join with notes
  data <- merge(cards, notes, by = "nid", all.x = TRUE)
  data <- merge(data, decks, by = "did", all.x = TRUE)
 
  # Split fields
  field_list <- strsplit(data$flds, "\x1f")
  max_fields <- max(lengths(field_list))
 
  field_df <- as.data.frame(do.call(rbind, lapply(field_list, function(x) {
    length(x) <- max_fields
    x
  })), stringsAsFactors = FALSE)
  names(field_df) <- paste0("field_", seq_len(ncol(field_df)))
 
  # Strip HTML if requested
  if (!include_html) {
    field_df <- as.data.frame(lapply(field_df, function(x) {
      gsub("<[^>]+>", "", x)
    }), stringsAsFactors = FALSE)
  }
 
  # Combine
  result <- cbind(
    data[, c("cid", "nid", "name", "tags", "type", "queue", "ivl", "reps", "lapses")],
    field_df
  )
  names(result)[names(result) == "name"] <- "deck"
 
  # Write to file
  if (is.null(file)) {
    deck_name <- decks$name[decks$did == deck_ids[1]]
    file <- paste0(gsub("[^a-zA-Z0-9]", "_", deck_name), ".csv")
  }
 
  utils::write.csv(result, file, row.names = FALSE)
  message("Exported ", nrow(result), " cards to ", file)
 
  invisible(tibble::as_tibble(result))
}

#' Generate collection summary report
#'
#' Creates a summary of your Anki collection statistics.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with collection statistics
#' @export
#' @examples
#' \dontrun{
#' report <- anki_report()
#' print(report)
#' }
anki_report <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
  revlog <- col$revlog()
 
  # Basic counts
  total_cards <- nrow(cards)
  total_notes <- nrow(notes)
  total_decks <- nrow(decks)
  total_reviews <- nrow(revlog)
 
  # Card states
  new_cards <- sum(cards$type == 0)
  learning_cards <- sum(cards$type == 1 | cards$type == 3)
  review_cards <- sum(cards$type == 2)
  suspended_cards <- sum(cards$queue == -1)
  buried_cards <- sum(cards$queue == -2 | cards$queue == -3)
 
  # Maturity
  mature_cards <- sum(cards$ivl >= 21)
  young_cards <- sum(cards$type == 2 & cards$ivl < 21)
 
  # Review stats
  if (nrow(revlog) > 0) {
    first_review <- min(revlog$review_date)
    last_review <- max(revlog$review_date)
    days_studied <- length(unique(revlog$review_date))
    avg_reviews_per_day <- total_reviews / as.numeric(last_review - first_review + 1)
    retention_rate <- 1 - sum(revlog$ease == 1) / nrow(revlog)
    total_time_hours <- sum(revlog$time, na.rm = TRUE) / 3600000
  } else {
    first_review <- NA
    last_review <- NA
    days_studied <- 0
    avg_reviews_per_day <- 0
    retention_rate <- NA
    total_time_hours <- 0
  }
 
  # Streak
  streak_info <- anki_streak(path = col$path)
 
  report <- list(
    # Overview
    total_cards = total_cards,
    total_notes = total_notes,
    total_decks = total_decks,
    total_reviews = total_reviews,
   
    # Card breakdown
    new_cards = new_cards,
    learning_cards = learning_cards,
    review_cards = review_cards,
    suspended_cards = suspended_cards,
    buried_cards = buried_cards,
   
    # Maturity
    mature_cards = mature_cards,
    young_cards = young_cards,
    mature_percent = if (review_cards > 0) mature_cards / review_cards * 100 else 0,
   
    # Review history
    first_review = first_review,
    last_review = last_review,
    days_studied = days_studied,
    avg_reviews_per_day = round(avg_reviews_per_day, 1),
    retention_rate = round(retention_rate * 100, 1),
    total_time_hours = round(total_time_hours, 1),
   
    # Streaks
    current_streak = streak_info$current_streak,
    longest_streak = streak_info$longest_streak
  )
 
  class(report) <- c("anki_report", "list")
  report
}

#' @export
print.anki_report <- function(x, ...) {
  cat("=== Anki Collection Report ===\n\n")
 
  cat("OVERVIEW\n")
  cat(sprintf("  Cards: %d | Notes: %d | Decks: %d\n",
              x$total_cards, x$total_notes, x$total_decks))
  cat(sprintf("  Total reviews: %s\n", format(x$total_reviews, big.mark = ",")))
  cat("\n")
 
  cat("CARD BREAKDOWN\n")
  cat(sprintf("  New: %d | Learning: %d | Review: %d\n",
              x$new_cards, x$learning_cards, x$review_cards))
  cat(sprintf("  Suspended: %d | Buried: %d\n",
              x$suspended_cards, x$buried_cards))
  cat("\n")
 
  cat("MATURITY\n")
  cat(sprintf("  Mature (21+ days): %d (%.1f%%)\n",
              x$mature_cards, x$mature_percent))
  cat(sprintf("  Young: %d\n", x$young_cards))
  cat("\n")
 
  cat("STUDY HISTORY\n")
  if (!is.na(x$first_review)) {
    cat(sprintf("  Studying since: %s\n", x$first_review))
    cat(sprintf("  Days studied: %d\n", x$days_studied))
    cat(sprintf("  Avg reviews/day: %.1f\n", x$avg_reviews_per_day))
    cat(sprintf("  Retention rate: %.1f%%\n", x$retention_rate))
    cat(sprintf("  Total study time: %.1f hours\n", x$total_time_hours))
  } else {
    cat("  No reviews yet\n")
  }
  cat("\n")
 
  cat("STREAKS\n")
  cat(sprintf("  Current: %d days\n", x$current_streak))
  cat(sprintf("  Longest: %d days\n", x$longest_streak))
 
  invisible(x)
}

#' Export review history
#'
#' Exports the full review log to CSV.
#'
#' @param file Output file path
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param include_card_info If TRUE, join with card data
#' @return Invisibly returns the exported data
#' @export
#' @examples
#' \dontrun{
#' anki_export_revlog("my_reviews.csv")
#' }
anki_export_revlog <- function(file, path = NULL, profile = NULL, include_card_info = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (include_card_info) {
    cards <- col$cards()
    decks <- col$decks()
    revlog <- merge(revlog, cards[, c("cid", "did", "ivl")], by = "cid", all.x = TRUE,
                    suffixes = c("", ".card"))
    revlog <- merge(revlog, decks, by = "did", all.x = TRUE)
  }
 
  utils::write.csv(revlog, file, row.names = FALSE)
  message("Exported ", nrow(revlog), " reviews to ", file)
 
  invisible(tibble::as_tibble(revlog))
}

#' Get forecast of upcoming reviews
#'
#' Predicts how many reviews will be due each day.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to forecast (default 30)
#' @return A tibble with daily forecast
#' @export
#' @examples
#' \dontrun{
#' forecast <- anki_forecast()
#' }
anki_forecast <- function(path = NULL, profile = NULL, days = 30) {
  cards <- anki_cards(path, profile)
 
  # Filter to review cards
  cards <- cards[cards$type == 2 & cards$queue == 2, ]
 
  if (nrow(cards) == 0) {
    return(tibble::tibble(
      date = Sys.Date() + seq_len(days) - 1,
      due = rep(0L, days)
    ))
  }
 
  # Estimate due dates (simplified)
  # Note: This is approximate since Anki's due field interpretation varies
  today <- as.integer(Sys.Date())
 
  forecast <- vapply(seq_len(days), function(d) {
    target_day <- today + d - 1
    # Cards due on or before this day
    sum(cards$due <= target_day)
  }, FUN.VALUE = integer(1))
 
  # Convert to new cards due per day
  new_due <- c(forecast[1], diff(forecast))
 
  tibble::tibble(
    date = Sys.Date() + seq_len(days) - 1,
    cumulative_due = forecast,
    new_due = new_due
  )
}
