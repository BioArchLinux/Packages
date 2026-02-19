#' Analyze reviews by hour of day
#'
#' Shows when you study most during the day.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze (NULL for all)
#' @return A tibble with hourly statistics
#' @export
#' @examples
#' \dontrun{
#' anki_time_by_hour()
#' }
anki_time_by_hour <- function(path = NULL, profile = NULL, days = 90) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (!is.null(days)) {
    cutoff <- Sys.Date() - days
    revlog <- revlog[revlog$review_date >= cutoff, ]
  }
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble(hour = 0:23, reviews = rep(0L, 24)))
  }
 
  # Extract hour from timestamp
  revlog$hour <- as.integer(format(as.POSIXct(revlog$rid / 1000, origin = "1970-01-01"), "%H"))
 
  # Aggregate
  stats <- lapply(split(revlog, revlog$hour), function(d) {
    tibble::tibble(
      reviews = nrow(d),
      retention = round((1 - sum(d$ease == 1) / nrow(d)) * 100, 1),
      avg_time_ms = round(mean(d$time, na.rm = TRUE)),
      total_time_min = round(sum(d$time, na.rm = TRUE) / 60000, 1)
    )
  })
 
  result <- do.call(rbind, stats)
  result$hour <- as.integer(names(stats))
 
  # Fill missing hours
  all_hours <- tibble::tibble(hour = 0:23)
  result <- merge(all_hours, result, by = "hour", all.x = TRUE)
  result[is.na(result)] <- 0
 
  result <- result[order(result$hour), ]
 
  # Add peak indicator
  result$is_peak <- result$reviews >= quantile(result$reviews, 0.75)
 
  tibble::as_tibble(result)
}

#' Analyze reviews by day of week
#'
#' Shows which days you study most.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze (NULL for all)
#' @return A tibble with daily statistics
#' @export
#' @examples
#' \dontrun{
#' anki_time_by_weekday()
#' }
anki_time_by_weekday <- function(path = NULL, profile = NULL, days = 90) {
  daily <- anki_stats_daily(path, profile, from = if (!is.null(days)) Sys.Date() - days else NULL)
 
  if (nrow(daily) == 0) {
    return(tibble::tibble())
  }
 
  daily$weekday <- weekdays(daily$date, abbreviate = FALSE)
 
  # Aggregate by weekday
  stats <- lapply(split(daily, daily$weekday), function(d) {
    tibble::tibble(
      days_studied = nrow(d),
      total_reviews = sum(d$reviews),
      avg_reviews = round(mean(d$reviews), 1),
      avg_time_min = round(mean(d$time_minutes), 1),
      avg_retention = round(mean(d$retention, na.rm = TRUE) * 100, 1)
    )
  })
 
  result <- do.call(rbind, stats)
  result$weekday <- factor(
    names(stats),
    levels = c("Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday")
  )
  result <- result[order(result$weekday), ]
 
  tibble::as_tibble(result[, c("weekday", "days_studied", "total_reviews",
                               "avg_reviews", "avg_time_min", "avg_retention")])
}

#' Analyze study sessions
#'
#' Identifies study sessions and calculates session statistics.
#' A session is defined as reviews with gaps less than session_gap minutes.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param session_gap Gap in minutes that defines session boundaries (default 30)
#' @param days Number of days to analyze
#' @return A tibble with session statistics
#' @export
#' @examples
#' \dontrun{
#' sessions <- anki_session_stats()
#' }
anki_session_stats <- function(path = NULL, profile = NULL, session_gap = 30, days = 90) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (!is.null(days)) {
    cutoff <- Sys.Date() - days
    revlog <- revlog[revlog$review_date >= cutoff, ]
  }
 
  if (nrow(revlog) < 2) {
    return(tibble::tibble())
  }
 
  # Convert timestamps and sort
  revlog$datetime <- as.POSIXct(revlog$rid / 1000, origin = "1970-01-01")
  revlog <- revlog[order(revlog$datetime), ]
 
  # Calculate gaps between reviews (in minutes)
  revlog$gap_min <- c(0, diff(as.numeric(revlog$datetime)) / 60)
 
  # Assign session IDs
  revlog$session_id <- cumsum(revlog$gap_min > session_gap)
 
  # Calculate session stats
  sessions <- lapply(split(revlog, revlog$session_id), function(s) {
    tibble::tibble(
      start_time = min(s$datetime),
      end_time = max(s$datetime),
      duration_min = round(as.numeric(difftime(max(s$datetime), min(s$datetime), units = "mins")) +
                           mean(s$time, na.rm = TRUE) / 60000, 1),
      reviews = nrow(s),
      cards_reviewed = length(unique(s$cid)),
      retention = round((1 - sum(s$ease == 1) / nrow(s)) * 100, 1),
      avg_time_per_card_sec = round(mean(s$time, na.rm = TRUE) / 1000, 1)
    )
  })
 
  result <- do.call(rbind, sessions)
  result$session_id <- as.integer(names(sessions))
 
  # Add date and hour
  result$date <- as.Date(result$start_time)
  result$start_hour <- as.integer(format(result$start_time, "%H"))
 
  # Summary statistics
  attr(result, "summary") <- list(
    total_sessions = nrow(result),
    avg_session_duration = round(mean(result$duration_min), 1),
    avg_reviews_per_session = round(mean(result$reviews), 1),
    median_session_duration = round(median(result$duration_min), 1),
    longest_session = round(max(result$duration_min), 1)
  )
 
  tibble::as_tibble(result[, c("session_id", "date", "start_hour", "duration_min",
                               "reviews", "cards_reviewed", "retention",
                               "avg_time_per_card_sec")])
}

#' Analyze response time by card properties
#'
#' Shows how response time varies by difficulty, interval, etc.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze
#' @return A tibble with response time analysis
#' @export
#' @examples
#' \dontrun{
#' anki_response_time()
#' }
anki_response_time <- function(path = NULL, profile = NULL, days = 90) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
  cards <- col$cards()
  cards_fsrs <- anki_cards_fsrs(path = col$path)
 
  if (!is.null(days)) {
    cutoff <- Sys.Date() - days
    revlog <- revlog[revlog$review_date >= cutoff, ]
  }
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble())
  }
 
  # Join with card data
  revlog <- merge(revlog, cards[, c("cid", "did")], by = "cid", all.x = TRUE)
  revlog <- merge(revlog, cards_fsrs[, c("cid", "difficulty", "stability")],
                  by = "cid", all.x = TRUE)
 
  # Overall stats
  overall <- tibble::tibble(
    category = "Overall",
    group = "All",
    avg_time_sec = round(mean(revlog$time, na.rm = TRUE) / 1000, 1),
    median_time_sec = round(median(revlog$time, na.rm = TRUE) / 1000, 1),
    reviews = nrow(revlog)
  )
 
  # By rating
  by_rating <- lapply(split(revlog, revlog$ease), function(d) {
    tibble::tibble(
      category = "Rating",
      group = paste0("Ease ", d$ease[1]),
      avg_time_sec = round(mean(d$time, na.rm = TRUE) / 1000, 1),
      median_time_sec = round(median(d$time, na.rm = TRUE) / 1000, 1),
      reviews = nrow(d)
    )
  })
  by_rating <- do.call(rbind, by_rating)
 
  # By difficulty bin
  revlog$diff_bin <- cut(revlog$difficulty,
                         breaks = c(-Inf, 3, 5, 7, Inf),
                         labels = c("Easy (<3)", "Medium (3-5)", "Hard (5-7)", "Very Hard (>7)"))
  revlog_with_diff <- revlog[!is.na(revlog$diff_bin), ]
 
  if (nrow(revlog_with_diff) > 0) {
    by_difficulty <- lapply(split(revlog_with_diff, revlog_with_diff$diff_bin), function(d) {
      tibble::tibble(
        category = "Difficulty",
        group = as.character(d$diff_bin[1]),
        avg_time_sec = round(mean(d$time, na.rm = TRUE) / 1000, 1),
        median_time_sec = round(median(d$time, na.rm = TRUE) / 1000, 1),
        reviews = nrow(d)
      )
    })
    by_difficulty <- do.call(rbind, by_difficulty)
  } else {
    by_difficulty <- tibble::tibble()
  }
 
  # By interval bin
  revlog <- merge(revlog, cards[, c("cid", "ivl")], by = "cid", all.x = TRUE, suffixes = c("", ".card"))
  revlog$ivl_bin <- cut(revlog$ivl.card,
                        breaks = c(-Inf, 7, 21, 90, 365, Inf),
                        labels = c("< 1 week", "1-3 weeks", "1-3 months", "3-12 months", "> 1 year"))
  revlog_with_ivl <- revlog[!is.na(revlog$ivl_bin), ]
 
  if (nrow(revlog_with_ivl) > 0) {
    by_interval <- lapply(split(revlog_with_ivl, revlog_with_ivl$ivl_bin), function(d) {
      tibble::tibble(
        category = "Interval",
        group = as.character(d$ivl_bin[1]),
        avg_time_sec = round(mean(d$time, na.rm = TRUE) / 1000, 1),
        median_time_sec = round(median(d$time, na.rm = TRUE) / 1000, 1),
        reviews = nrow(d)
      )
    })
    by_interval <- do.call(rbind, by_interval)
  } else {
    by_interval <- tibble::tibble()
  }
 
  result <- rbind(overall, by_rating, by_difficulty, by_interval)
  tibble::as_tibble(result)
}

#' Get monthly summary statistics
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param months Number of months to include (default 12)
#' @return A tibble with monthly statistics
#' @export
#' @examples
#' \dontrun{
#' anki_monthly_summary()
#' }
anki_monthly_summary <- function(path = NULL, profile = NULL, months = 12) {
  daily <- anki_stats_daily(path, profile, from = Sys.Date() - months * 31)
 
  if (nrow(daily) == 0) {
    return(tibble::tibble())
  }
 
  daily$month <- format(daily$date, "%Y-%m")
 
  # Aggregate by month
  stats <- lapply(split(daily, daily$month), function(d) {
    tibble::tibble(
      days_studied = nrow(d),
      total_reviews = sum(d$reviews),
      avg_per_day = round(mean(d$reviews), 1),
      total_time_hours = round(sum(d$time_minutes) / 60, 1),
      avg_retention = round(mean(d$retention, na.rm = TRUE) * 100, 1)
    )
  })
 
  result <- do.call(rbind, stats)
  result$month <- names(stats)
  result <- result[order(result$month), ]
 
  tibble::as_tibble(result[, c("month", "days_studied", "total_reviews",
                               "avg_per_day", "total_time_hours", "avg_retention")])
}

#' Analyze study consistency
#'
#' Measures how consistent your study habits are.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze
#' @return A list with consistency metrics
#' @export
#' @examples
#' \dontrun{
#' anki_consistency()
#' }
anki_consistency <- function(path = NULL, profile = NULL, days = 90) {
  daily <- anki_stats_daily(path, profile, from = Sys.Date() - days)
 
  if (nrow(daily) == 0) {
    return(list(
      study_rate = 0,
      consistency_score = 0,
      coefficient_of_variation = NA
    ))
  }
 
  # Calculate metrics
  total_days <- days
  days_studied <- nrow(daily)
  study_rate <- round(days_studied / total_days * 100, 1)
 
  # Coefficient of variation (lower = more consistent)
  cv <- sd(daily$reviews) / mean(daily$reviews)
 
  # Consistency score (0-100)
  # Based on: study rate, regularity, streak length
  streak_info <- anki_streak(path, profile)
 
  consistency_score <- round(
    study_rate * 0.4 +
    max(0, 100 - cv * 50) * 0.3 +
    min(100, streak_info$current_streak / days * 100) * 0.3,
    1
  )
 
  list(
    study_rate = study_rate,
    days_studied = days_studied,
    total_days = total_days,
    consistency_score = consistency_score,
    coefficient_of_variation = round(cv, 2),
    avg_reviews_per_day = round(mean(daily$reviews), 1),
    sd_reviews = round(sd(daily$reviews), 1),
    current_streak = streak_info$current_streak,
    longest_streak = streak_info$longest_streak
  )
}
