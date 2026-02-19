#' Compare FSRS parameters
#'
#' Compares your optimized FSRS parameters against defaults and community averages.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with parameter comparison
#' @export
#' @examples
#' \dontrun{
#' comp <- fsrs_compare_parameters()
#' }
fsrs_compare_parameters <- function(path = NULL, profile = NULL) {
  # Get user parameters
  params <- fsrs_get_parameters(path, profile)

  # FSRS-4.5/5 default parameters (19 weights)
  # Based on https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm
  default_weights <- c(
    0.4,    # w0: initial stability for Again
    0.6,    # w1: initial stability for Hard
    2.4,    # w2: initial stability for Good
    5.8,    # w3: initial stability for Easy
    4.93,   # w4: difficulty mean reversion
    0.94,   # w5: difficulty change for Again
    0.86,   # w6: difficulty change for Hard
    0.01,   # w7: difficulty change for Good
    1.49,   # w8: difficulty change for Easy
    0.14,   # w9: stability change for fail
    0.94,   # w10: stability decay
    2.18,   # w11: stability increase base
    0.05,   # w12: difficulty impact
    0.34,   # w13: retrievability impact
    1.26,   # w14: stability increase difficulty mod
    0.29,   # w15: stability after fail
    2.61,   # w16: hard penalty
    0.0,    # w17: (reserved)
    0.0,    # w18: (reserved)
    0.0     # w19: (reserved) - some versions have 17, some 19
  )

  # Parameter names
  param_names <- c(
    "Initial Stability (Again)", "Initial Stability (Hard)",
    "Initial Stability (Good)", "Initial Stability (Easy)",
    "Difficulty Mean Reversion", "Difficulty Change (Again)",
    "Difficulty Change (Hard)", "Difficulty Change (Good)",
    "Difficulty Change (Easy)", "Stability Fail Factor",
    "Stability Decay", "Stability Increase Base",
    "Difficulty Impact", "Retrievability Impact",
    "Stability Increase Difficulty Mod", "Stability After Fail",
    "Hard Penalty", "Reserved 1", "Reserved 2", "Reserved 3"
  )

  # Parameter interpretations
  interpretations <- c(
    "Higher = longer initial interval for 'Again' responses",
    "Higher = longer initial interval for 'Hard' responses",
    "Higher = longer initial interval for 'Good' responses",
    "Higher = longer initial interval for 'Easy' responses",
    "Controls how difficulty returns to average",
    "How much difficulty changes with 'Again'",
    "How much difficulty changes with 'Hard'",
    "How much difficulty changes with 'Good'",
    "How much difficulty changes with 'Easy'",
    "Factor applied when card is failed",
    "Rate of stability decay over time",
    "Base multiplier for stability increase",
    "How much difficulty affects stability",
    "How much retrievability affects stability",
    "Difficulty modifier for stability increase",
    "Stability restoration after fail",
    "Penalty multiplier for 'Hard' responses",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use"
  )

  if (is.null(params) || is.null(params$weights)) {
    return(list(
      message = "FSRS parameters not found. Is FSRS enabled in your Anki?",
      fsrs_enabled = FALSE,
      default_parameters = tibble::tibble(
        parameter = param_names[1:17],
        default_value = round(default_weights[1:17], 3),
        interpretation = interpretations[1:17]
      )
    ))
  }

  user_weights <- params$weights

  # Ensure same length
  len <- min(length(user_weights), length(default_weights), length(param_names))

  # Create comparison
  comparison <- tibble::tibble(
    parameter = param_names[1:len],
    your_value = round(user_weights[1:len], 3),
    default_value = round(default_weights[1:len], 3),
    difference = round(user_weights[1:len] - default_weights[1:len], 3),
    pct_change = round((user_weights[1:len] - default_weights[1:len]) / default_weights[1:len] * 100, 1)
  )

  # Handle division by zero
  comparison$pct_change[!is.finite(comparison$pct_change)] <- NA

  # Flag significant differences (>20% change)
  comparison$significant <- abs(comparison$pct_change) > 20 & !is.na(comparison$pct_change)

  # Key insights
  insights <- character()

  # Check initial stability
  avg_init_stab <- mean(user_weights[1:4])
  default_init_stab <- mean(default_weights[1:4])
  if (avg_init_stab > default_init_stab * 1.3) {
    insights <- c(insights, "Your initial stability is higher than default - you learn new cards faster")
  } else if (avg_init_stab < default_init_stab * 0.7) {
    insights <- c(insights, "Your initial stability is lower than default - new cards need more repetition")
  }

  # Check stability decay
  if (length(user_weights) >= 11 && user_weights[11] > default_weights[11] * 1.2) {
    insights <- c(insights, "Your stability decays faster than average - consider more frequent reviews")
  }

  # Check difficulty changes
  if (length(user_weights) >= 6 && user_weights[6] > default_weights[6] * 1.3) {
    insights <- c(insights, "Cards become difficult faster after failures for you")
  }

  list(
    fsrs_enabled = TRUE,
    desired_retention = params$desired_retention,
    maximum_interval = params$maximum_interval,
    comparison = comparison,
    significant_differences = comparison[comparison$significant, ],
    insights = if (length(insights) > 0) insights else "Your parameters are close to defaults",
    raw_weights = tibble::tibble(
      index = paste0("w", 0:(len-1)),
      your_value = round(user_weights[1:len], 4),
      default_value = round(default_weights[1:len], 4)
    )
  )
}

#' Analyze FSRS decay parameter distribution
#'
#' In FSRS-6, each card can have its own decay parameter. This analyzes the
#' distribution across your collection.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by_deck If TRUE, analyze by deck
#' @return A tibble with decay distribution statistics
#' @export
#' @examples
#' \dontrun{
#' decay <- fsrs_decay_distribution()
#' }
fsrs_decay_distribution <- function(path = NULL, profile = NULL, by_deck = FALSE) {
  cards <- anki_cards_fsrs(path, profile)

  # Filter to cards with decay data
  cards <- cards[!is.na(cards$decay) & cards$decay > 0, ]

  if (nrow(cards) == 0) {
    message("No FSRS-6 decay data found. This feature requires FSRS-6 in Anki 24.11+")
    return(tibble::tibble())
  }

  if (!by_deck) {
    return(tibble::tibble(
      metric = c("Mean Decay", "Median Decay", "SD Decay", "Min Decay", "Max Decay",
                 "Cards with Decay Data", "Low Decay (<0.3)", "Medium Decay (0.3-0.6)",
                 "High Decay (>0.6)"),
      value = c(
        round(mean(cards$decay, na.rm = TRUE), 3),
        round(median(cards$decay, na.rm = TRUE), 3),
        round(sd(cards$decay, na.rm = TRUE), 3),
        round(min(cards$decay, na.rm = TRUE), 3),
        round(max(cards$decay, na.rm = TRUE), 3),
        nrow(cards),
        sum(cards$decay < 0.3),
        sum(cards$decay >= 0.3 & cards$decay <= 0.6),
        sum(cards$decay > 0.6)
      ),
      interpretation = c(
        "Lower = slower forgetting",
        "Typical decay for your cards",
        "Variation in forgetting rates",
        "Your slowest forgetting card",
        "Your fastest forgetting card",
        "Cards analyzed",
        "Slow forgetters - easier to remember",
        "Average forgetters",
        "Fast forgetters - harder to remember"
      )
    ))
  }

  # By deck
  col <- anki_collection(path, profile)
  on.exit(col$close())
  decks <- col$decks()

  stats <- lapply(split(cards, cards$did), function(d) {
    if (nrow(d) < 5) return(NULL)
    tibble::tibble(
      mean_decay = round(mean(d$decay, na.rm = TRUE), 3),
      median_decay = round(median(d$decay, na.rm = TRUE), 3),
      n_cards = nrow(d)
    )
  })

  result <- do.call(rbind, Filter(Negate(is.null), stats))
  result$did <- as.numeric(names(Filter(Negate(is.null), stats)))
  result <- merge(result, decks[, c("did", "name")], by = "did", all.x = TRUE)
  result <- result[order(result$mean_decay), ]

  tibble::as_tibble(result[, c("name", "mean_decay", "median_decay", "n_cards")])
}

#' Calculate memory state for cards
#'
#' Calculates the current FSRS memory state (stability, difficulty, retrievability)
#' for all cards in the collection.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param include_projection Include projected future states
#' @return A tibble with memory states
#' @export
#' @examples
#' \dontrun{
#' states <- fsrs_memory_states()
#' }
fsrs_memory_states <- function(path = NULL, profile = NULL, include_projection = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- anki_cards_fsrs(path = col$path)
  revlog <- col$revlog()
  decks <- col$decks()

  # Filter to cards with FSRS data
  cards <- cards[!is.na(cards$stability) & cards$stability > 0, ]

  if (nrow(cards) == 0) {
    message("No FSRS data found")
    return(tibble::tibble())
  }

  # Get last review date for each card
  last_review <- aggregate(review_date ~ cid, data = revlog, FUN = max)
  cards <- merge(cards, last_review, by = "cid", all.x = TRUE)

  # Calculate days elapsed
  cards$days_elapsed <- as.numeric(Sys.Date() - cards$review_date)
  cards$days_elapsed[is.na(cards$days_elapsed)] <- 0

  # Use per-card decay or default
  cards$decay[is.na(cards$decay)] <- 0.5

  # Calculate current retrievability
  cards$retrievability <- fsrs_retrievability(
    stability = cards$stability,
    days_elapsed = cards$days_elapsed,
    decay = cards$decay
  )

  # Add deck info
  cards <- merge(cards, decks[, c("did", "name")], by = "did", all.x = TRUE)

  # Calculate memory strength category
  cards$memory_strength <- cut(
    cards$retrievability,
    breaks = c(-Inf, 0.5, 0.7, 0.85, 0.95, Inf),
    labels = c("Critical", "Weak", "Fair", "Good", "Strong")
  )

  # Stability category
  cards$stability_category <- cut(
    cards$stability,
    breaks = c(-Inf, 7, 30, 90, 365, Inf),
    labels = c("Short-term", "Medium-term", "Long-term", "Very Long-term", "Permanent")
  )

  result <- cards[, c("cid", "nid", "name", "stability", "difficulty", "decay",
                       "days_elapsed", "retrievability", "memory_strength",
                       "stability_category")]
  names(result)[names(result) == "name"] <- "deck"

  if (include_projection) {
    # Project retrievability at different future times
    result$r_in_7_days <- fsrs_retrievability(
      result$stability, result$days_elapsed + 7, result$decay
    )
    result$r_in_30_days <- fsrs_retrievability(
      result$stability, result$days_elapsed + 30, result$decay
    )
    result$r_in_90_days <- fsrs_retrievability(
      result$stability, result$days_elapsed + 90, result$decay
    )
  }

  # Order by retrievability (lowest first = most urgent)
  result <- result[order(result$retrievability), ]

  tibble::as_tibble(result)
}

#' Analyze response time outliers
#'
#' Find reviews with suspicious response times (too fast or too slow).
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param min_time_ms Minimum expected time in ms (default 500)
#' @param max_time_ms Maximum expected time in ms (default 120000 = 2 min)
#' @return A tibble with outlier reviews
#' @export
#' @examples
#' \dontrun{
#' outliers <- anki_response_time_outliers()
#' }
anki_response_time_outliers <- function(path = NULL, profile = NULL,
                                        min_time_ms = 500, max_time_ms = 120000) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  revlog <- col$revlog()
  cards <- col$cards()
  notes <- col$notes()

  # Find outliers
  too_fast <- revlog[revlog$time < min_time_ms, ]
  too_slow <- revlog[revlog$time > max_time_ms, ]

  # Summarize
  summary <- tibble::tibble(
    category = c("Total Reviews", "Too Fast (<0.5s)", "Too Slow (>2min)",
                 "Normal Range"),
    count = c(
      nrow(revlog),
      nrow(too_fast),
      nrow(too_slow),
      nrow(revlog) - nrow(too_fast) - nrow(too_slow)
    ),
    percentage = round(c(
      100,
      nrow(too_fast) / nrow(revlog) * 100,
      nrow(too_slow) / nrow(revlog) * 100,
      (nrow(revlog) - nrow(too_fast) - nrow(too_slow)) / nrow(revlog) * 100
    ), 1)
  )

  # Get card details for outliers
  get_outlier_details <- function(outlier_revlog, type) {
    if (nrow(outlier_revlog) == 0) return(tibble::tibble())

    # Aggregate by card
    by_card <- aggregate(cbind(count = rid, avg_time = time) ~ cid,
                         data = outlier_revlog,
                         FUN = function(x) if (is.numeric(x)) mean(x) else length(x))

    by_card <- merge(by_card, cards[, c("cid", "nid")], by = "cid")
    by_card <- merge(by_card, notes[, c("nid", "sfld")], by = "nid")

    by_card$type <- type
    by_card <- by_card[order(-by_card$count), ]

    head(by_card[, c("cid", "sfld", "count", "avg_time", "type")], 20)
  }

  fast_details <- get_outlier_details(too_fast, "Too Fast")
  slow_details <- get_outlier_details(too_slow, "Too Slow")

  list(
    summary = summary,
    too_fast = tibble::as_tibble(fast_details),
    too_slow = tibble::as_tibble(slow_details),
    recommendations = c(
      if (nrow(too_fast) > nrow(revlog) * 0.05) {
        "Many very fast responses - consider if you're reviewing too quickly"
      },
      if (nrow(too_slow) > nrow(revlog) * 0.05) {
        "Many very slow responses - these cards may need simplification"
      }
    )
  )
}
