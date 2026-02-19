#' Analyze learning efficiency
#'
#' Calculate how much "real learning" is happening vs time spent on failed reviews.
#' Measures the ratio of successful retention to total study time.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze (default 30, NULL for all)
#' @param by_deck If TRUE, calculate efficiency per deck
#' @return A tibble with efficiency metrics
#' @export
#' @examples
#' \dontrun{
#' eff <- anki_learning_efficiency()
#' eff <- anki_learning_efficiency(days = 90, by_deck = TRUE)
#' }
anki_learning_efficiency <- function(path = NULL, profile = NULL, days = 30,
                                     by_deck = FALSE) {

  col <- anki_collection(path, profile)
  on.exit(col$close())

  revlog <- col$revlog()
  cards <- col$cards()

  # Filter by date

if (!is.null(days)) {
    cutoff <- Sys.Date() - days
    revlog <- revlog[revlog$review_date >= cutoff, ]
  }

  if (nrow(revlog) == 0) {
    message("No reviews found in the specified period")
    return(tibble::tibble())
  }

  calc_efficiency <- function(data) {
    total_reviews <- nrow(data)
    successful_reviews <- sum(data$ease > 1)
    failed_reviews <- sum(data$ease == 1)
    total_time_ms <- sum(data$time, na.rm = TRUE)
    total_time_min <- total_time_ms / 60000

    # Time spent on failed reviews (wasted time)
    failed_time_ms <- sum(data$time[data$ease == 1], na.rm = TRUE)
    failed_time_min <- failed_time_ms / 60000

    # Efficiency metrics
    success_rate <- successful_reviews / total_reviews
    time_efficiency <- (total_time_min - failed_time_min) / total_time_min
    
    # Learning points: successful reviews weighted by interval gained
    # Higher interval = more "learning" accomplished
    data_with_ivl <- data[data$ivl > 0, ]
    if (nrow(data_with_ivl) > 0) {
      learning_points <- sum(log1p(data_with_ivl$ivl[data_with_ivl$ease > 1]), na.rm = TRUE)
    } else {
      learning_points <- 0
    }

    # Effective learning rate: learning points per minute
    effective_rate <- if (total_time_min > 0) learning_points / total_time_min else 0

    # Review burden: time spent per successful long-term retention
    mature_reviews <- sum(data$ivl >= 21 & data$ease > 1)
    burden_ratio <- if (mature_reviews > 0) total_time_min / mature_reviews else NA_real_

    tibble::tibble(
      total_reviews = total_reviews,
      successful_reviews = successful_reviews,
      failed_reviews = failed_reviews,
      success_rate = round(success_rate * 100, 1),
      total_time_min = round(total_time_min, 1),
      failed_time_min = round(failed_time_min, 1),
      time_efficiency = round(time_efficiency * 100, 1),
      learning_points = round(learning_points, 1),
      effective_rate = round(effective_rate, 2),
      minutes_per_mature_review = round(burden_ratio, 2)
    )
  }

  if (!by_deck) {
    return(calc_efficiency(revlog))
  }

  # By deck
  decks <- col$decks()
  revlog <- merge(revlog, cards[, c("cid", "did")], by = "cid", all.x = TRUE)

  stats <- lapply(split(revlog, revlog$did), calc_efficiency)
  result <- do.call(rbind, stats)
  result$did <- as.numeric(names(stats))
  result <- merge(result, decks, by = "did", all.x = TRUE)
  result <- result[order(-result$effective_rate), ]

  tibble::as_tibble(result)
}

#' Calculate retention by content type
#'
#' Break down retention by card characteristics: cloze vs basic, with/without media,
#' short vs long content.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze (default 30)
#' @return A list with retention breakdowns by different content types
#' @export
#' @examples
#' \dontrun{
#' ret_type <- anki_retention_by_type()
#' ret_type$by_card_type
#' ret_type$by_media
#' }
anki_retention_by_type <- function(path = NULL, profile = NULL, days = 30) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  notes <- col$notes()
  cards <- col$cards()
  revlog <- col$revlog()
  models <- col$models()

  # Filter by date
  if (!is.null(days)) {
    cutoff <- Sys.Date() - days
    revlog <- revlog[revlog$review_date >= cutoff, ]
  }

  # Enrich notes with content characteristics
  notes$has_cloze <- grepl("\\{\\{c\\d+::", notes$flds)
  notes$has_image <- grepl("<img", notes$flds, ignore.case = TRUE)
  notes$has_sound <- grepl("\\[sound:", notes$flds)
  notes$has_media <- notes$has_image | notes$has_sound
  notes$has_mathjax <- grepl("\\\\\\(|\\\\\\[|\\$\\$", notes$flds)
  notes$char_count <- nchar(notes$flds)
  notes$word_count <- lengths(strsplit(gsub("<[^>]+>", "", notes$flds), "\\s+"))

  # Categorize by length
  notes$length_category <- cut(
    notes$char_count,
    breaks = c(-1, 200, 500, 1000, 2000, Inf),
    labels = c("Very Short (<200)", "Short (200-500)", "Medium (500-1000)",
               "Long (1000-2000)", "Very Long (>2000)")
  )

  # Join data
  cards <- merge(cards, notes[, c("nid", "mid", "has_cloze", "has_image", "has_sound",
                                   "has_media", "has_mathjax", "length_category")],
                 by = "nid", all.x = TRUE)
  revlog <- merge(revlog, cards[, c("cid", "has_cloze", "has_image", "has_sound",
                                     "has_media", "has_mathjax", "length_category", "mid")],
                  by = "cid", all.x = TRUE)

  # Join with model names
  revlog <- merge(revlog, models[, c("mid", "name")], by = "mid", all.x = TRUE)

  calc_retention <- function(data, group_name) {
    if (nrow(data) == 0) return(NULL)
    tibble::tibble(
      category = group_name,
      reviews = nrow(data),
      retention = round((1 - sum(data$ease == 1) / nrow(data)) * 100, 1),
      avg_time_sec = round(mean(data$time, na.rm = TRUE) / 1000, 1)
    )
  }

  # By card type (cloze vs non-cloze)
  by_cloze <- rbind(
    calc_retention(revlog[revlog$has_cloze == TRUE, ], "Cloze"),
    calc_retention(revlog[revlog$has_cloze == FALSE, ], "Non-Cloze")
  )

  # By media presence
  by_media <- rbind(
    calc_retention(revlog[revlog$has_media == TRUE, ], "With Media"),
    calc_retention(revlog[revlog$has_media == FALSE, ], "Without Media")
  )

  # By image specifically
  by_image <- rbind(
    calc_retention(revlog[revlog$has_image == TRUE, ], "With Images"),
    calc_retention(revlog[revlog$has_image == FALSE, ], "Without Images")
  )

  # By sound
  by_sound <- rbind(
    calc_retention(revlog[revlog$has_sound == TRUE, ], "With Sound"),
    calc_retention(revlog[revlog$has_sound == FALSE, ], "Without Sound")
  )

  # By MathJax
  by_mathjax <- rbind(
    calc_retention(revlog[revlog$has_mathjax == TRUE, ], "With MathJax"),
    calc_retention(revlog[revlog$has_mathjax == FALSE, ], "Without MathJax")
  )

  # By length
  by_length <- do.call(rbind, lapply(split(revlog, revlog$length_category), function(d) {
    if (nrow(d) == 0) return(NULL)
    calc_retention(d, as.character(d$length_category[1]))
  }))

  # By note type
  by_model <- do.call(rbind, lapply(split(revlog, revlog$name), function(d) {
    if (nrow(d) == 0) return(NULL)
    calc_retention(d, as.character(d$name[1]))
  }))
  if (!is.null(by_model)) {
    by_model <- by_model[order(-by_model$reviews), ]
  }

  list(
    by_card_type = tibble::as_tibble(by_cloze),
    by_media = tibble::as_tibble(by_media),
    by_image = tibble::as_tibble(by_image),
    by_sound = tibble::as_tibble(by_sound),
    by_mathjax = tibble::as_tibble(by_mathjax),
    by_length = tibble::as_tibble(by_length),
    by_note_type = if (!is.null(by_model)) tibble::as_tibble(by_model) else tibble::tibble()
  )
}

#' Calculate spaced repetition ROI
#'
#' Estimate the return on investment of your spaced repetition practice.
#' Calculates knowledge half-life extension per minute of study.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with ROI analysis
#' @export
#' @examples
#' \dontrun{
#' roi <- anki_roi_analysis()
#' }
anki_roi_analysis <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  revlog <- col$revlog()
  cards_fsrs <- anki_cards_fsrs(path = col$path)

  # Total time invested
  total_time_ms <- sum(revlog$time, na.rm = TRUE)
  total_time_hours <- total_time_ms / 3600000
  total_time_min <- total_time_ms / 60000

  # Total reviews
  total_reviews <- nrow(revlog)

  # Knowledge retained (cards at various stability levels)
  cards_fsrs <- cards_fsrs[!is.na(cards_fsrs$stability), ]

  if (nrow(cards_fsrs) == 0) {
    # Fall back to interval-based analysis
    review_cards <- cards[cards$type == 2 & cards$ivl > 0, ]
    total_stability_days <- sum(review_cards$ivl)
    avg_stability <- mean(review_cards$ivl)
    mature_cards <- sum(review_cards$ivl >= 21)
  } else {
    total_stability_days <- sum(cards_fsrs$stability)
    avg_stability <- mean(cards_fsrs$stability)
    mature_cards <- sum(cards_fsrs$stability >= 21)
  }

  # ROI Metrics
  # Stability per hour: how many days of memory stability gained per hour studied
  stability_per_hour <- total_stability_days / total_time_hours

  # Cards matured per hour
  mature_per_hour <- mature_cards / total_time_hours

  # Efficiency score: stability gained per review
  stability_per_review <- total_stability_days / total_reviews

  # Time value: if you had to re-learn everything from scratch
  # Assume 5 minutes to learn a new card
  time_to_relearn_hours <- nrow(cards) * 5 / 60
  time_savings_ratio <- time_to_relearn_hours / total_time_hours

  # Cumulative knowledge value
  # Assume each day of stability = some knowledge value
  # Weight by current retrievability if available
  if (nrow(cards_fsrs) > 0) {
    # Get last review dates
    last_review <- aggregate(review_date ~ cid, data = revlog, FUN = max)
    cards_fsrs <- merge(cards_fsrs, last_review, by = "cid", all.x = TRUE)
    cards_fsrs$days_elapsed <- as.numeric(Sys.Date() - cards_fsrs$review_date)
    cards_fsrs$days_elapsed[is.na(cards_fsrs$days_elapsed)] <- 0
    cards_fsrs$decay[is.na(cards_fsrs$decay)] <- 0.5

    cards_fsrs$retrievability <- fsrs_retrievability(
      stability = cards_fsrs$stability,
      days_elapsed = cards_fsrs$days_elapsed,
      decay = cards_fsrs$decay
    )

    current_knowledge <- sum(cards_fsrs$retrievability * cards_fsrs$stability, na.rm = TRUE)
  } else {
    current_knowledge <- total_stability_days * 0.85  # Assume 85% avg retrievability
  }

  # Project future value
  # Without reviews, knowledge decays. With maintenance, it persists.
  projected_value_1year <- current_knowledge * 0.9  # Maintained
  no_review_value_1year <- current_knowledge * 0.1  # Would decay

  list(
    investment = tibble::tibble(
      metric = c("Total Time (hours)", "Total Time (minutes)", "Total Reviews",
                 "Total Cards", "Mature Cards"),
      value = c(round(total_time_hours, 1), round(total_time_min, 0),
                total_reviews, nrow(cards), mature_cards)
    ),
    returns = tibble::tibble(
      metric = c("Total Stability (card-days)", "Avg Stability (days)",
                 "Current Knowledge Value", "Stability per Hour",
                 "Mature Cards per Hour", "Stability per Review"),
      value = round(c(total_stability_days, avg_stability, current_knowledge,
                      stability_per_hour, mature_per_hour, stability_per_review), 1)
    ),
    value_analysis = tibble::tibble(
      scenario = c("Time to Re-learn All (hours)", "Time Savings Ratio",
                   "Projected Value (1 year, maintained)",
                   "Projected Value (1 year, no reviews)"),
      value = round(c(time_to_relearn_hours, time_savings_ratio,
                      projected_value_1year, no_review_value_1year), 1)
    ),
    interpretation = c(
      paste0("You've invested ", round(total_time_hours, 1), " hours of study.
"),
      paste0("This has built ", round(total_stability_days, 0), " card-days of memory stability."),
      paste0("On average, you gain ", round(stability_per_hour, 1), " days of stability per hour studied."),
      if (time_savings_ratio > 1) {
        paste0("Your spaced repetition is ", round(time_savings_ratio, 1), "x more efficient than re-learning.")
      } else {
        "Continue building your collection for better efficiency gains."
      }
    )
  )
}
