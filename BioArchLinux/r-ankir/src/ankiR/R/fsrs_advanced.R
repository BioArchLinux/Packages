#' Analyze FSRS difficulty distribution
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by_deck If TRUE, calculate distribution per deck
#' @return A tibble with difficulty statistics
#' @export
#' @examples
#' \dontrun{
#' diff_dist <- fsrs_difficulty_distribution()
#' }
fsrs_difficulty_distribution <- function(path = NULL, profile = NULL, by_deck = FALSE) {
  cards <- anki_cards_fsrs(path, profile)
 
  # Filter to cards with FSRS data
  cards <- cards[!is.na(cards$difficulty), ]
 
  if (nrow(cards) == 0) {
    message("No FSRS data found. Is FSRS enabled in Anki?")
    return(tibble::tibble())
  }
 
  if (!by_deck) {
    return(tibble::tibble(
      mean_difficulty = mean(cards$difficulty, na.rm = TRUE),
      median_difficulty = median(cards$difficulty, na.rm = TRUE),
      sd_difficulty = sd(cards$difficulty, na.rm = TRUE),
      min_difficulty = min(cards$difficulty, na.rm = TRUE),
      max_difficulty = max(cards$difficulty, na.rm = TRUE),
      n_cards = nrow(cards),
      easy = sum(cards$difficulty < 4, na.rm = TRUE),
      medium = sum(cards$difficulty >= 4 & cards$difficulty <= 7, na.rm = TRUE),
      hard = sum(cards$difficulty > 7, na.rm = TRUE)
    ))
  }
 
  # By deck
  col <- anki_collection(path, profile)
  on.exit(col$close())
  decks <- col$decks()
 
  stats <- lapply(split(cards, cards$did), function(d) {
    tibble::tibble(
      mean_difficulty = mean(d$difficulty, na.rm = TRUE),
      median_difficulty = median(d$difficulty, na.rm = TRUE),
      n_cards = nrow(d),
      easy = sum(d$difficulty < 4, na.rm = TRUE),
      medium = sum(d$difficulty >= 4 & d$difficulty <= 7, na.rm = TRUE),
      hard = sum(d$difficulty > 7, na.rm = TRUE)
    )
  })
 
  result <- do.call(rbind, stats)
  result$did <- as.numeric(names(stats))
  result <- merge(result, decks, by = "did", all.x = TRUE)
 
  tibble::as_tibble(result)
}

#' Analyze FSRS stability distribution
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by_deck If TRUE, calculate distribution per deck
#' @return A tibble with stability statistics
#' @export
#' @examples
#' \dontrun{
#' stab_dist <- fsrs_stability_distribution()
#' }
fsrs_stability_distribution <- function(path = NULL, profile = NULL, by_deck = FALSE) {
  cards <- anki_cards_fsrs(path, profile)
 
  # Filter to cards with FSRS data
  cards <- cards[!is.na(cards$stability) & cards$stability > 0, ]
 
  if (nrow(cards) == 0) {
    message("No FSRS data found. Is FSRS enabled in Anki?")
    return(tibble::tibble())
  }
 
  if (!by_deck) {
    return(tibble::tibble(
      mean_stability = mean(cards$stability, na.rm = TRUE),
      median_stability = median(cards$stability, na.rm = TRUE),
      sd_stability = sd(cards$stability, na.rm = TRUE),
      min_stability = min(cards$stability, na.rm = TRUE),
      max_stability = max(cards$stability, na.rm = TRUE),
      n_cards = nrow(cards),
      short_term = sum(cards$stability < 7, na.rm = TRUE),
      medium_term = sum(cards$stability >= 7 & cards$stability < 30, na.rm = TRUE),
      long_term = sum(cards$stability >= 30 & cards$stability < 365, na.rm = TRUE),
      very_long_term = sum(cards$stability >= 365, na.rm = TRUE)
    ))
  }
 
  # By deck
  col <- anki_collection(path, profile)
  on.exit(col$close())
  decks <- col$decks()
 
  stats <- lapply(split(cards, cards$did), function(d) {
    tibble::tibble(
      mean_stability = mean(d$stability, na.rm = TRUE),
      median_stability = median(d$stability, na.rm = TRUE),
      n_cards = nrow(d)
    )
  })
 
  result <- do.call(rbind, stats)
  result$did <- as.numeric(names(stats))
  result <- merge(result, decks, by = "did", all.x = TRUE)
 
  tibble::as_tibble(result)
}

#' Calculate current retrievability for all cards
#'
#' Computes the current probability of recall for all FSRS-enabled cards.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with cards and their current retrievability
#' @export
#' @examples
#' \dontrun{
#' r <- fsrs_current_retrievability()
#' # Cards with low retrievability need review soon
#' r[r$retrievability < 0.8, ]
#' }
fsrs_current_retrievability <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- anki_cards_fsrs(path = col$path)
  revlog <- col$revlog()
 
  # Filter to cards with FSRS data
  cards <- cards[!is.na(cards$stability) & cards$stability > 0, ]
 
  if (nrow(cards) == 0) {
    message("No FSRS data found.")
    return(tibble::tibble())
  }
 
  # Get last review date for each card
  last_review <- aggregate(review_date ~ cid, data = revlog, FUN = max)
  names(last_review) <- c("cid", "last_review")
 
  cards <- merge(cards, last_review, by = "cid", all.x = TRUE)
 
  # Calculate days since last review
  cards$days_elapsed <- as.numeric(Sys.Date() - cards$last_review)
  cards$days_elapsed[is.na(cards$days_elapsed)] <- 0
 
  # Calculate retrievability using per-card decay or default
  cards$decay[is.na(cards$decay)] <- 0.5
 
  cards$retrievability <- fsrs_retrievability(
    stability = cards$stability,
    days_elapsed = cards$days_elapsed,
    decay = cards$decay
  )
 
  # Order by retrievability (lowest first = most urgent)
  cards <- cards[order(cards$retrievability), ]
 
  tibble::as_tibble(cards)
}

#' Prepare review data for r-fsrs optimizer
#'
#' Converts Anki review history to the format expected by the r-fsrs package
#' for parameter optimization.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param min_reviews Minimum reviews per card to include (default 3)
#' @return A list of review items suitable for r-fsrs
#' @export
#' @examples
#' \dontrun{
#' # Prepare data for r-fsrs
#' items <- fsrs_prepare_for_optimizer()
#'
#' # Use with r-fsrs (if installed)
#' # library(fsrs)
#' # params <- compute_parameters(items)
#' }
fsrs_prepare_for_optimizer <- function(path = NULL, profile = NULL, min_reviews = 3) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  # Order by card and time
  revlog <- revlog[order(revlog$cid, revlog$rid), ]
 
  # Filter cards with minimum reviews
  card_counts <- table(revlog$cid)
  valid_cards <- names(card_counts)[card_counts >= min_reviews]
  revlog <- revlog[revlog$cid %in% valid_cards, ]
 
  if (nrow(revlog) == 0) {
    message("No cards with enough reviews found.")
    return(list())
  }
 
  # Convert to list of review items
  items <- lapply(split(revlog, revlog$cid), function(card_revs) {
    card_revs <- card_revs[order(card_revs$rid), ]
   
    # Calculate delta_t (days between reviews)
    dates <- card_revs$review_date
    delta_t <- c(0, as.integer(diff(dates)))
   
    # Map ease to rating (1-4)
    rating <- card_revs$ease
   
    list(
      card_id = card_revs$cid[1],
      reviews = data.frame(
        rating = rating,
        delta_t = delta_t
      )
    )
  })
 
  names(items) <- NULL
  items
}

#' Get FSRS parameters from Anki deck config
#'
#' Extracts the FSRS parameters stored in Anki's deck configuration.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with FSRS parameters or NULL if not found
#' @export
#' @examples
#' \dontrun{
#' params <- fsrs_get_parameters()
#' }
fsrs_get_parameters <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  # Try to read deck config from col table
  tryCatch({
    col_data <- DBI::dbReadTable(col$con, "col")
    dconf <- jsonlite::fromJSON(col_data$dconf, simplifyVector = FALSE)
   
    # Look for FSRS parameters in deck configs
    params <- lapply(dconf, function(dc) {
      if (!is.null(dc$fsrs) && dc$fsrs) {
        list(
          enabled = TRUE,
          weights = dc$fsrsWeights,
          desired_retention = dc$desiredRetention,
          maximum_interval = dc$maxIvl
        )
      } else {
        NULL
      }
    })
   
    # Return first FSRS config found
    params <- Filter(Negate(is.null), params)
    if (length(params) > 0) {
      return(params[[1]])
    }
   
    NULL
  }, error = function(e) {
    NULL
  })
}
