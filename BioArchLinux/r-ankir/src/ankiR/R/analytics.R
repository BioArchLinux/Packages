#' Get cards with full joined data
#'
#' Returns cards joined with notes, decks, and models for complete analysis.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with card data joined with note content, deck names, and model info
#' @export
#' @examples
#' \dontrun{
#' cards_full <- anki_cards_full()
#' }
anki_cards_full <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
  models <- col$models()
 
  # Join everything
 result <- merge(cards, notes, by = "nid", all.x = TRUE)
  result <- merge(result, decks, by = "did", all.x = TRUE, suffixes = c("", ".deck"))
  result <- merge(result, models[, c("mid", "name")], by = "mid", all.x = TRUE,
                  suffixes = c("", ".model"))
 
  # Rename for clarity
  names(result)[names(result) == "name"] <- "deck_name"
  names(result)[names(result) == "name.model"] <- "model_name"
 
  tibble::as_tibble(result)
}

#' Extract unique tags with counts
#'
#' Parses tags from all notes and returns unique tags with their frequency.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with columns: tag, count
#' @export
#' @examples
#' \dontrun{
#' tags <- anki_tags()
#' }
anki_tags <- function(path = NULL, profile = NULL) {
  notes <- anki_notes(path, profile)
 
  # Parse space-separated tags
 all_tags <- unlist(strsplit(notes$tags, "\\s+"))
  all_tags <- all_tags[all_tags != ""]
 
  if (length(all_tags) == 0) {
    return(tibble::tibble(tag = character(), count = integer()))
  }
 
  tag_counts <- table(all_tags)
  tibble::tibble(
    tag = names(tag_counts),
    count = as.integer(tag_counts)
  ) |> 
    (\(x) x[order(-x$count), ])()
}

#' Parse note fields into columns
#'
#' Splits the field content (separated by \\x1f) into separate columns.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param model_id Optional model ID to filter notes. If NULL, uses first model.
#' @return A tibble with nid and separate columns for each field
#' @export
#' @examples
#' \dontrun{
#' fields <- anki_field_contents()
#' }
anki_field_contents <- function(path = NULL, profile = NULL, model_id = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  notes <- col$notes()
  models <- col$models()
 
  if (is.null(model_id)) {
    model_id <- models$mid[1]
    message("Using model: ", models$name[models$mid == model_id])
  }
 
  # Filter notes by model
  notes <- notes[notes$mid == model_id, ]
 
  if (nrow(notes) == 0) {
    return(tibble::tibble(nid = integer()))
  }
 
  # Get field names for this model
  field_names <- models$flds[models$mid == model_id][[1]]
  if (length(field_names) == 0) {
    field_names <- paste0("field_", seq_len(max(lengths(strsplit(notes$flds, "\x1f")))))
  }
 
  # Split fields
  field_list <- strsplit(notes$flds, "\x1f")
 
  # Pad to same length
  max_fields <- max(lengths(field_list))
  field_list <- lapply(field_list, function(x) {
    length(x) <- max_fields
    x
  })
 
  # Create data frame
  field_df <- as.data.frame(do.call(rbind, field_list), stringsAsFactors = FALSE)
 
  # Name columns
  if (length(field_names) >= ncol(field_df)) {
    names(field_df) <- field_names[seq_len(ncol(field_df))]
  } else {
    names(field_df) <- c(field_names, paste0("field_", seq(length(field_names) + 1, ncol(field_df))))
  }
 
  result <- cbind(nid = notes$nid, field_df)
  tibble::as_tibble(result)
}

#' Calculate per-deck statistics
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with deck statistics
#' @export
#' @examples
#' \dontrun{
#' stats <- anki_stats_deck()
#' }
anki_stats_deck <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  decks <- col$decks()
 
  # Aggregate by deck
  stats <- lapply(split(cards, cards$did), function(d) {
    tibble::tibble(
      total = nrow(d),
      new = sum(d$type == 0),
      learning = sum(d$type == 1),
      review = sum(d$type == 2),
      relearning = sum(d$type == 3),
      suspended = sum(d$queue == -1),
      buried = sum(d$queue == -2 | d$queue == -3),
      avg_interval = if (sum(d$type == 2) > 0) mean(d$ivl[d$type == 2]) else NA_real_,
      max_interval = if (sum(d$type == 2) > 0) max(d$ivl[d$type == 2]) else NA_integer_,
      total_reps = sum(d$reps),
      total_lapses = sum(d$lapses),
      mature = sum(d$ivl >= 21),
      young = sum(d$type == 2 & d$ivl < 21)
    )
  })
 
  result <- do.call(rbind, stats)
  result$did <- as.numeric(names(stats))
 
  # Join with deck names
  result <- merge(result, decks, by = "did", all.x = TRUE)
 
  tibble::as_tibble(result[, c("did", "name", "total", "new", "learning", "review",
                               "relearning", "suspended", "buried", "avg_interval",
                               "max_interval", "total_reps", "total_lapses",
                               "mature", "young")])
}

#' Calculate daily review statistics
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param from Optional start date (Date or character "YYYY-MM-DD")
#' @param to Optional end date (Date or character "YYYY-MM-DD")
#' @return A tibble with daily statistics
#' @export
#' @examples
#' \dontrun{
#' daily <- anki_stats_daily()
#' daily <- anki_stats_daily(from = "2024-01-01")
#' }
anki_stats_daily <- function(path = NULL, profile = NULL, from = NULL, to = NULL) {
  revlog <- anki_revlog(path, profile)
 
  if (!is.null(from)) {
    from <- as.Date(from)
    revlog <- revlog[revlog$review_date >= from, ]
  }
 
  if (!is.null(to)) {
    to <- as.Date(to)
    revlog <- revlog[revlog$review_date <= to, ]
  }
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble(
      date = as.Date(character()),
      reviews = integer(),
      time_minutes = numeric(),
      again = integer(),
      hard = integer(),
      good = integer(),
      easy = integer(),
      retention = numeric()
    ))
  }
 
  # Aggregate by date
  stats <- lapply(split(revlog, revlog$review_date), function(d) {
    tibble::tibble(
      reviews = nrow(d),
      time_minutes = sum(d$time, na.rm = TRUE) / 60000,
      again = sum(d$ease == 1),
      hard = sum(d$ease == 2),
      good = sum(d$ease == 3),
      easy = sum(d$ease == 4),
      retention = 1 - sum(d$ease == 1) / nrow(d)
    )
  })
 
  result <- do.call(rbind, stats)
  result$date <- as.Date(names(stats))
 
  tibble::as_tibble(result[, c("date", "reviews", "time_minutes", "again",
                               "hard", "good", "easy", "retention")])
}

#' Calculate actual retention rate from review history
#'
#' Calculates the proportion of reviews that were successful (not "Again").
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to look back (default 30, NULL for all)
#' @param by_deck If TRUE, calculate retention per deck
#' @return A tibble with retention statistics
#' @export
#' @examples
#' \dontrun{
#' retention <- anki_retention_rate()
#' retention <- anki_retention_rate(days = 90, by_deck = TRUE)
#' }
anki_retention_rate <- function(path = NULL, profile = NULL, days = 30, by_deck = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  # Filter by date
  if (!is.null(days)) {
    cutoff <- Sys.Date() - days
    revlog <- revlog[revlog$review_date >= cutoff, ]
  }
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble(retention = NA_real_, total_reviews = 0L))
  }
 
  if (!by_deck) {
    return(tibble::tibble(
      retention = 1 - sum(revlog$ease == 1) / nrow(revlog),
      total_reviews = nrow(revlog),
      again = sum(revlog$ease == 1),
      passed = sum(revlog$ease > 1)
    ))
  }
 
  # By deck requires joining with cards
  cards <- col$cards()
  decks <- col$decks()
 
  revlog <- merge(revlog, cards[, c("cid", "did")], by = "cid", all.x = TRUE)
 
  stats <- lapply(split(revlog, revlog$did), function(d) {
    tibble::tibble(
      retention = 1 - sum(d$ease == 1) / nrow(d),
      total_reviews = nrow(d),
      again = sum(d$ease == 1),
      passed = sum(d$ease > 1)
    )
  })
 
  result <- do.call(rbind, stats)
  result$did <- as.numeric(names(stats))
  result <- merge(result, decks, by = "did", all.x = TRUE)
 
  tibble::as_tibble(result[, c("did", "name", "retention", "total_reviews", "again", "passed")])
}

#' Track card learning progression over time
#'
#' Shows how cards have progressed through intervals over time.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param card_ids Optional vector of card IDs to track (NULL for sample)
#' @param n_cards Number of cards to sample if card_ids is NULL
#' @return A tibble with review history per card
#' @export
#' @examples
#' \dontrun{
#' curve <- anki_learning_curve()
#' }
anki_learning_curve <- function(path = NULL, profile = NULL, card_ids = NULL, n_cards = 100) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (is.null(card_ids)) {
    unique_cards <- unique(revlog$cid)
    n_cards <- min(n_cards, length(unique_cards))
    card_ids <- sample(unique_cards, n_cards)
  }
 
  revlog <- revlog[revlog$cid %in% card_ids, ]
 
  # Order by card and time
  revlog <- revlog[order(revlog$cid, revlog$rid), ]
 
  # Add review number per card
  revlog$review_num <- ave(seq_len(nrow(revlog)), revlog$cid, FUN = seq_along)
 
  tibble::as_tibble(revlog)
}

#' Get review data formatted for calendar heatmaps
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param year Optional year to filter (NULL for all)
#' @return A tibble with date and count columns suitable for heatmap visualization
#' @export
#' @examples
#' \dontrun{
#' heatmap_data <- anki_heatmap_data()
#' # Use with ggplot2:
#' # ggplot(heatmap_data, aes(week, weekday, fill = reviews)) + geom_tile()
#' }
anki_heatmap_data <- function(path = NULL, profile = NULL, year = NULL) {
  revlog <- anki_revlog(path, profile)
 
  if (!is.null(year)) {
    revlog <- revlog[format(revlog$review_date, "%Y") == as.character(year), ]
  }
 
  # Aggregate by date
  daily <- as.data.frame(table(revlog$review_date), stringsAsFactors = FALSE)
  names(daily) <- c("date", "reviews")
  daily$date <- as.Date(daily$date)
  daily$reviews <- as.integer(daily$reviews)
 
  # Add calendar fields for heatmap
  daily$weekday <- factor(weekdays(daily$date, abbreviate = TRUE),
                          levels = c("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"))
  daily$week <- as.integer(format(daily$date, "%W"))
  daily$month <- format(daily$date, "%b")
  daily$year <- as.integer(format(daily$date, "%Y"))
 
  tibble::as_tibble(daily)
}

#' Calculate current review streak
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with current_streak, longest_streak, and streak_history
#' @export
#' @examples
#' \dontrun{
#' streak <- anki_streak()
#' streak$current_streak
#' streak$longest_streak
#' }
anki_streak <- function(path = NULL, profile = NULL) {
  revlog <- anki_revlog(path, profile)
 
  if (nrow(revlog) == 0) {
    return(list(current_streak = 0L, longest_streak = 0L, streak_history = integer()))
  }
 
  # Get unique review dates
  dates <- sort(unique(revlog$review_date))
 
  if (length(dates) == 0) {
    return(list(current_streak = 0L, longest_streak = 0L, streak_history = integer()))
  }
 
  # Calculate gaps between consecutive dates
  gaps <- diff(dates)
 
  # Find streak breaks (gaps > 1 day)
  breaks <- which(gaps > 1)
 
  # Calculate streak lengths
  if (length(breaks) == 0) {
    streaks <- length(dates)
  } else {
    streak_starts <- c(1, breaks + 1)
    streak_ends <- c(breaks, length(dates))
    streaks <- streak_ends - streak_starts + 1
  }
 
  # Current streak (only if last review was today or yesterday)
  last_review <- max(dates)
  days_since_last <- as.integer(Sys.Date() - last_review)
 
  if (days_since_last <= 1) {
    current_streak <- streaks[length(streaks)]
  } else {
    current_streak <- 0L
  }
 
  list(
    current_streak = as.integer(current_streak),
    longest_streak = as.integer(max(streaks)),
    streak_history = as.integer(streaks),
    last_review = last_review,
    total_days_studied = length(dates)
  )
}
