#' Compare statistics between decks
#'
#' Side-by-side comparison of deck statistics.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param decks Optional vector of deck names to compare (NULL for all)
#' @return A tibble with comparative statistics
#' @export
#' @examples
#' \dontrun{
#' anki_compare_decks()
#' anki_compare_decks(decks = c("Medical", "Anatomy"))
#' }
anki_compare_decks <- function(path = NULL, profile = NULL, decks = NULL) {
  stats <- anki_stats_deck(path, profile)
 
  if (!is.null(decks)) {
    stats <- stats[grepl(paste(decks, collapse = "|"), stats$name, ignore.case = TRUE), ]
  }
 
  if (nrow(stats) == 0) {
    message("No matching decks found")
    return(tibble::tibble())
  }
 
  # Add derived metrics
  stats$mature_pct <- ifelse(stats$total > 0,
                             round(stats$mature / stats$total * 100, 1), 0)
  stats$lapse_rate <- ifelse(stats$total_reps > 0,
                             round(stats$total_lapses / stats$total_reps * 100, 2), 0)
 
  # Get retention for each deck
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  retention <- anki_retention_rate(path = col$path, days = 30, by_deck = TRUE)
 
  if (nrow(retention) > 0) {
    stats <- merge(stats, retention[, c("did", "retention")], by = "did", all.x = TRUE)
    stats$retention <- round(stats$retention * 100, 1)
  }
 
  # Order by total cards
  stats <- stats[order(-stats$total), ]
 
  tibble::as_tibble(stats)
}

#' Compare two time periods
#'
#' Compare study statistics between two time periods.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param period1 First period as c(start_date, end_date)
#' @param period2 Second period as c(start_date, end_date)
#' @param period_names Names for the periods (default: "Period 1", "Period 2")
#' @return A tibble comparing the two periods
#' @export
#' @examples
#' \dontrun{
#' # Compare this month vs last month
#' anki_compare_periods(
#'   period1 = c("2024-01-01", "2024-01-31"),
#'   period2 = c("2024-02-01", "2024-02-29"),
#'   period_names = c("January", "February")
#' )
#' }
anki_compare_periods <- function(path = NULL, profile = NULL,
                                 period1 = NULL, period2 = NULL,
                                 period_names = c("Period 1", "Period 2")) {
  # Default periods: this month vs last month
  if (is.null(period1)) {
    today <- Sys.Date()
    period2 <- c(as.Date(format(today, "%Y-%m-01")), today)
    period1_end <- as.Date(format(today, "%Y-%m-01")) - 1
    period1 <- c(as.Date(format(period1_end, "%Y-%m-01")), period1_end)
    period_names <- c(format(period1[1], "%B %Y"), format(period2[1], "%B %Y"))
  }
 
  period1 <- as.Date(period1)
  period2 <- as.Date(period2)
 
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  # Calculate stats for each period
  calc_period_stats <- function(data, name) {
    if (nrow(data) == 0) {
      return(tibble::tibble(
        period = name,
        total_reviews = 0L,
        days_studied = 0L,
        avg_per_day = 0,
        retention = NA_real_,
        time_hours = 0,
        again_count = 0L,
        avg_ease = NA_real_
      ))
    }
   
    tibble::tibble(
      period = name,
      total_reviews = nrow(data),
      days_studied = length(unique(data$review_date)),
      avg_per_day = round(nrow(data) / length(unique(data$review_date)), 1),
      retention = round((1 - sum(data$ease == 1) / nrow(data)) * 100, 1),
      time_hours = round(sum(data$time, na.rm = TRUE) / 3600000, 1),
      again_count = sum(data$ease == 1),
      avg_ease = round(mean(data$ease), 2)
    )
  }
 
  p1_data <- revlog[revlog$review_date >= period1[1] & revlog$review_date <= period1[2], ]
  p2_data <- revlog[revlog$review_date >= period2[1] & revlog$review_date <= period2[2], ]
 
  result <- rbind(
    calc_period_stats(p1_data, period_names[1]),
    calc_period_stats(p2_data, period_names[2])
  )
 
  # Add change row
  if (nrow(result) == 2) {
    change <- tibble::tibble(
      period = "Change",
      total_reviews = result$total_reviews[2] - result$total_reviews[1],
      days_studied = result$days_studied[2] - result$days_studied[1],
      avg_per_day = round(result$avg_per_day[2] - result$avg_per_day[1], 1),
      retention = round(result$retention[2] - result$retention[1], 1),
      time_hours = round(result$time_hours[2] - result$time_hours[1], 1),
      again_count = result$again_count[2] - result$again_count[1],
      avg_ease = round(result$avg_ease[2] - result$avg_ease[1], 2)
    )
    result <- rbind(result, change)
  }
 
  tibble::as_tibble(result)
}

#' Benchmark against FSRS averages
#'
#' Compare your statistics against typical FSRS users (based on published research).
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with your stats vs benchmarks
#' @export
#' @examples
#' \dontrun{
#' anki_benchmark()
#' }
anki_benchmark <- function(path = NULL, profile = NULL) {
  # Benchmarks from FSRS-Anki-20k dataset and research papers
  benchmarks <- tibble::tibble(
    metric = c("Retention Rate", "Avg Interval (days)", "Avg Difficulty",
               "Avg Stability (days)", "Reviews per Card", "Lapse Rate"),
    benchmark = c(0.90, 45, 5.5, 60, 8, 0.10),
    description = c(
      "Target retention rate",
      "Average interval for mature cards",
      "Average FSRS difficulty (1-10)",
      "Average FSRS stability",
      "Average reviews per card",
      "Proportion of reviews rated 'Again'"
    )
  )
 
  # Calculate user stats
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  revlog <- col$revlog()
  cards_fsrs <- anki_cards_fsrs(path = col$path)
 
  # User metrics
  retention <- 1 - sum(revlog$ease == 1) / nrow(revlog)
  avg_interval <- mean(cards$ivl[cards$type == 2 & cards$ivl > 0], na.rm = TRUE)
  avg_difficulty <- mean(cards_fsrs$difficulty, na.rm = TRUE)
  avg_stability <- mean(cards_fsrs$stability, na.rm = TRUE)
  reviews_per_card <- nrow(revlog) / nrow(cards)
  lapse_rate <- sum(revlog$ease == 1) / nrow(revlog)
 
  user_values <- c(retention, avg_interval, avg_difficulty, avg_stability,
                   reviews_per_card, lapse_rate)
 
  benchmarks$your_value <- round(user_values, 2)
  benchmarks$difference <- round(benchmarks$your_value - benchmarks$benchmark, 2)
  benchmarks$status <- ifelse(
    abs(benchmarks$difference) < benchmarks$benchmark * 0.1,
    "OK Good",
    ifelse(
      (benchmarks$metric %in% c("Lapse Rate", "Avg Difficulty") & benchmarks$difference > 0) |
      (benchmarks$metric %in% c("Retention Rate", "Avg Stability") & benchmarks$difference < 0),
      "! Below",
      "OK Above"
    )
  )
 
  tibble::as_tibble(benchmarks)
}

#' Compare retention by card age
#'
#' Analyze how retention varies by how long you've been studying cards.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with retention by card age
#' @export
#' @examples
#' \dontrun{
#' anki_compare_by_age()
#' }
anki_compare_by_age <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  revlog <- col$revlog()
 
  # Get first review date for each card
  first_review <- aggregate(review_date ~ cid, data = revlog, FUN = min)
  names(first_review) <- c("cid", "first_review")
 
  # Calculate card age in days
  first_review$age_days <- as.integer(Sys.Date() - first_review$first_review)
 
  # Bin into age groups
  first_review$age_group <- cut(
    first_review$age_days,
    breaks = c(-1, 7, 30, 90, 180, 365, Inf),
    labels = c("< 1 week", "1-4 weeks", "1-3 months", "3-6 months", "6-12 months", "> 1 year")
  )
 
  # Join with revlog
  revlog <- merge(revlog, first_review[, c("cid", "age_group")], by = "cid", all.x = TRUE)
 
  # Calculate retention by age group
  stats <- lapply(split(revlog, revlog$age_group), function(d) {
    tibble::tibble(
      total_reviews = nrow(d),
      retention = round((1 - sum(d$ease == 1) / nrow(d)) * 100, 1),
      avg_ease = round(mean(d$ease), 2)
    )
  })
 
  result <- do.call(rbind, stats)
  result$age_group <- factor(names(stats), levels = c("< 1 week", "1-4 weeks",
    "1-3 months", "3-6 months", "6-12 months", "> 1 year"))
  result <- result[order(result$age_group), ]
 
  tibble::as_tibble(result[, c("age_group", "total_reviews", "retention", "avg_ease")])
}

#' Compare performance by deck difficulty
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble ranking decks by difficulty metrics
#' @export
#' @examples
#' \dontrun{
#' anki_compare_deck_difficulty()
#' }
anki_compare_deck_difficulty <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  cards_fsrs <- anki_cards_fsrs(path = col$path)
  decks <- col$decks()
  revlog <- col$revlog()
 
  # Merge FSRS data
  cards <- merge(cards, cards_fsrs[, c("cid", "difficulty", "stability")],
                 by = "cid", all.x = TRUE)
 
  # Join revlog with cards
  revlog <- merge(revlog, cards[, c("cid", "did")], by = "cid", all.x = TRUE)
 
  # Calculate per-deck metrics
  stats <- lapply(split(cards, cards$did), function(d) {
    did <- d$did[1]
    rev_subset <- revlog[revlog$did == did, ]
   
    tibble::tibble(
      avg_difficulty = mean(d$difficulty, na.rm = TRUE),
      avg_stability = mean(d$stability, na.rm = TRUE),
      avg_lapses = mean(d$lapses, na.rm = TRUE),
      retention = if (nrow(rev_subset) > 0) 1 - sum(rev_subset$ease == 1) / nrow(rev_subset) else NA_real_
    )
  })
 
  result <- do.call(rbind, stats)
  result$did <- as.numeric(names(stats))
  result <- merge(result, decks, by = "did", all.x = TRUE)
 
  # Calculate difficulty score (composite)
  result$difficulty_score <- with(result,
    scale(avg_difficulty)[,1] * 0.4 +
    scale(-avg_stability)[,1] * 0.3 +
    scale(avg_lapses)[,1] * 0.3
  )
 
  # Rank
  result <- result[order(-result$difficulty_score), ]
  result$rank <- seq_len(nrow(result))
 
  result$avg_difficulty <- round(result$avg_difficulty, 2)
  result$avg_stability <- round(result$avg_stability, 1)
  result$avg_lapses <- round(result$avg_lapses, 2)
  result$retention <- round(result$retention * 100, 1)
  result$difficulty_score <- round(result$difficulty_score, 2)
 
  tibble::as_tibble(result[, c("rank", "name", "avg_difficulty", "avg_stability",
                               "avg_lapses", "retention", "difficulty_score")])
}
