#' Find optimal review times
#'
#' Analyzes retention and performance by hour of day and day of week to
#' identify when you learn best.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param min_reviews Minimum reviews per time slot for analysis (default 50)
#' @return A list with optimal time analysis
#' @export
#' @examples
#' \dontrun{
#' times <- anki_best_review_times()
#' times$best_hours
#' times$best_days
#' }
anki_best_review_times <- function(path = NULL, profile = NULL, min_reviews = 50) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  revlog <- col$revlog()

  if (nrow(revlog) < 100) {
    message("Not enough review data for reliable analysis")
    return(NULL)
  }

  # Extract hour and weekday from review ID (millisecond timestamp)
  revlog$datetime <- anki_timestamp_to_datetime(revlog$rid)
  revlog$hour <- as.integer(format(revlog$datetime, "%H"))
  revlog$weekday <- weekdays(revlog$datetime)
  revlog$weekday_num <- as.integer(format(revlog$datetime, "%u"))  # 1=Monday

  # Calculate metrics by hour
  by_hour <- lapply(split(revlog, revlog$hour), function(d) {
    if (nrow(d) < min_reviews) return(NULL)
    tibble::tibble(
      hour = d$hour[1],
      reviews = nrow(d),
      retention = (1 - sum(d$ease == 1) / nrow(d)) * 100,
      avg_time_sec = mean(d$time, na.rm = TRUE) / 1000,
      easy_rate = sum(d$ease == 4) / nrow(d) * 100,
      again_rate = sum(d$ease == 1) / nrow(d) * 100
    )
  })

  hourly_stats <- do.call(rbind, Filter(Negate(is.null), by_hour))

  if (is.null(hourly_stats) || nrow(hourly_stats) == 0) {
    message("Not enough data per hour for analysis")
    return(NULL)
  }

  hourly_stats <- hourly_stats[order(hourly_stats$hour), ]

  # Calculate performance score (higher = better)
  hourly_stats$performance_score <- with(hourly_stats,
    scale(retention)[,1] * 0.4 +
    scale(-avg_time_sec)[,1] * 0.3 +
    scale(easy_rate)[,1] * 0.2 +
    scale(-again_rate)[,1] * 0.1
  )

  # Rank hours
  hourly_stats$rank <- rank(-hourly_stats$performance_score)

  # Calculate metrics by weekday
  by_weekday <- lapply(split(revlog, revlog$weekday), function(d) {
    if (nrow(d) < min_reviews) return(NULL)
    tibble::tibble(
      weekday = d$weekday[1],
      weekday_num = d$weekday_num[1],
      reviews = nrow(d),
      retention = (1 - sum(d$ease == 1) / nrow(d)) * 100,
      avg_time_sec = mean(d$time, na.rm = TRUE) / 1000,
      easy_rate = sum(d$ease == 4) / nrow(d) * 100,
      again_rate = sum(d$ease == 1) / nrow(d) * 100
    )
  })

  weekday_stats <- do.call(rbind, Filter(Negate(is.null), by_weekday))

  if (!is.null(weekday_stats) && nrow(weekday_stats) > 0) {
    weekday_stats <- weekday_stats[order(weekday_stats$weekday_num), ]
    weekday_stats$performance_score <- with(weekday_stats,
      scale(retention)[,1] * 0.4 +
      scale(-avg_time_sec)[,1] * 0.3 +
      scale(easy_rate)[,1] * 0.2 +
      scale(-again_rate)[,1] * 0.1
    )
    weekday_stats$rank <- rank(-weekday_stats$performance_score)
  }

  # Identify best times
  best_hours <- hourly_stats[order(-hourly_stats$performance_score), ]
  top_3_hours <- head(best_hours, 3)
  worst_3_hours <- tail(best_hours, 3)

  if (!is.null(weekday_stats) && nrow(weekday_stats) > 0) {
    best_days <- weekday_stats[order(-weekday_stats$performance_score), ]
    top_day <- best_days$weekday[1]
    worst_day <- best_days$weekday[nrow(best_days)]
  } else {
    best_days <- NULL
    top_day <- NA
    worst_day <- NA
  }

  # Time blocks analysis
  hourly_stats$time_block <- cut(hourly_stats$hour,
    breaks = c(-1, 6, 9, 12, 14, 17, 20, 24),
    labels = c("Night (0-6)", "Early Morning (6-9)", "Morning (9-12)",
               "Early Afternoon (12-14)", "Afternoon (14-17)",
               "Evening (17-20)", "Late Evening (20-24)")
  )

  block_stats <- lapply(split(hourly_stats, hourly_stats$time_block), function(d) {
    if (nrow(d) == 0) return(NULL)
    tibble::tibble(
      time_block = as.character(d$time_block[1]),
      avg_retention = mean(d$retention),
      avg_performance = mean(d$performance_score),
      total_reviews = sum(d$reviews)
    )
  })
  block_stats <- do.call(rbind, Filter(Negate(is.null), block_stats))
  if (!is.null(block_stats)) {
    block_stats <- block_stats[order(-block_stats$avg_performance), ]
  }

  # Generate recommendations
  recommendations <- character()

  if (nrow(top_3_hours) > 0) {
    best_hour_range <- paste(top_3_hours$hour, collapse = ", ")
    recommendations <- c(recommendations,
      paste0("Your best performing hours are: ", best_hour_range, ":00"))
  }

  if (!is.na(top_day)) {
    recommendations <- c(recommendations,
      paste0("You perform best on ", top_day, "s"))
  }

  if (nrow(worst_3_hours) > 0) {
    worst_hour <- worst_3_hours$hour[1]
    recommendations <- c(recommendations,
      paste0("Consider avoiding reviews at ", worst_hour, ":00 if possible"))
  }

  # Check for fatigue patterns (performance drop in consecutive hours)
  if (nrow(hourly_stats) > 3) {
    perf_diff <- diff(hourly_stats$performance_score[order(hourly_stats$hour)])
    if (any(perf_diff < -1)) {
      fatigue_hour <- hourly_stats$hour[order(hourly_stats$hour)][which(perf_diff < -1)[1] + 1]
      recommendations <- c(recommendations,
        paste0("Performance drops significantly around ", fatigue_hour, ":00 - consider taking breaks"))
    }
  }

  list(
    by_hour = tibble::as_tibble(hourly_stats[, c("hour", "reviews", "retention",
                                                  "avg_time_sec", "easy_rate",
                                                  "again_rate", "performance_score", "rank")]),
    by_weekday = if (!is.null(weekday_stats)) {
      tibble::as_tibble(weekday_stats[, c("weekday", "reviews", "retention",
                                           "avg_time_sec", "easy_rate",
                                           "again_rate", "performance_score", "rank")])
    } else tibble::tibble(),
    by_time_block = if (!is.null(block_stats)) tibble::as_tibble(block_stats) else tibble::tibble(),
    best_hours = tibble::as_tibble(top_3_hours[, c("hour", "retention", "performance_score")]),
    worst_hours = tibble::as_tibble(worst_3_hours[, c("hour", "retention", "performance_score")]),
    best_day = top_day,
    worst_day = worst_day,
    recommendations = recommendations
  )
}

#' Analyze study sessions
#'
#' Identifies and analyzes discrete study sessions based on gaps between reviews.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param gap_minutes Gap in minutes to consider a new session (default 30)
#' @param min_session_reviews Minimum reviews to count as a session (default 5)
#' @return A list with session analysis
#' @export
#' @examples
#' \dontrun{
#' sessions <- anki_session_analysis()
#' }
anki_session_analysis <- function(path = NULL, profile = NULL, gap_minutes = 30,
                                   min_session_reviews = 5) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  revlog <- col$revlog()

  if (nrow(revlog) < min_session_reviews) {
    message("Not enough reviews for session analysis")
    return(NULL)
  }

  # Order by timestamp
  revlog <- revlog[order(revlog$rid), ]
  revlog$datetime <- anki_timestamp_to_datetime(revlog$rid)

  # Calculate gaps between reviews
  time_diff <- c(NA, diff(as.numeric(revlog$datetime)))
  time_diff_min <- time_diff / 60

  # Identify session boundaries
  session_start <- c(TRUE, time_diff_min > gap_minutes)
  session_start[is.na(session_start)] <- TRUE

  # Assign session IDs
  revlog$session_id <- cumsum(session_start)

  # Analyze each session
  session_stats <- lapply(split(revlog, revlog$session_id), function(s) {
    if (nrow(s) < min_session_reviews) return(NULL)

    duration_min <- as.numeric(difftime(max(s$datetime), min(s$datetime), units = "mins"))

    tibble::tibble(
      session_id = s$session_id[1],
      date = as.Date(s$datetime[1]),
      start_time = format(min(s$datetime), "%H:%M"),
      end_time = format(max(s$datetime), "%H:%M"),
      duration_min = round(duration_min, 1),
      reviews = nrow(s),
      cards_per_min = round(nrow(s) / max(duration_min, 1), 1),
      retention = round((1 - sum(s$ease == 1) / nrow(s)) * 100, 1),
      avg_time_per_card_sec = round(mean(s$time, na.rm = TRUE) / 1000, 1),
      total_time_min = round(sum(s$time, na.rm = TRUE) / 60000, 1),
      unique_cards = length(unique(s$cid)),
      # Performance over session
      first_half_retention = round((1 - sum(s$ease[1:floor(nrow(s)/2)] == 1) / floor(nrow(s)/2)) * 100, 1),
      second_half_retention = round((1 - sum(s$ease[(floor(nrow(s)/2)+1):nrow(s)] == 1) / ceiling(nrow(s)/2)) * 100, 1)
    )
  })

  sessions <- do.call(rbind, Filter(Negate(is.null), session_stats))

  if (is.null(sessions) || nrow(sessions) == 0) {
    message("No valid sessions found")
    return(NULL)
  }

  # Calculate fatigue indicator
  sessions$fatigue_drop <- sessions$first_half_retention - sessions$second_half_retention

  # Summary statistics
  summary_stats <- tibble::tibble(
    metric = c("Total Sessions", "Avg Duration (min)", "Avg Reviews/Session",
               "Avg Retention", "Avg Cards/Min", "Sessions with Fatigue Drop"),
    value = c(
      nrow(sessions),
      round(mean(sessions$duration_min), 1),
      round(mean(sessions$reviews), 0),
      round(mean(sessions$retention), 1),
      round(mean(sessions$cards_per_min), 1),
      sum(sessions$fatigue_drop > 5, na.rm = TRUE)
    )
  )

  # Optimal session length analysis
  # Bin by duration and check retention
  sessions$duration_bin <- cut(sessions$duration_min,
    breaks = c(0, 10, 20, 30, 45, 60, 90, Inf),
    labels = c("0-10 min", "10-20 min", "20-30 min", "30-45 min",
               "45-60 min", "60-90 min", "90+ min")
  )

  by_duration <- lapply(split(sessions, sessions$duration_bin), function(d) {
    if (nrow(d) < 3) return(NULL)
    tibble::tibble(
      duration_range = as.character(d$duration_bin[1]),
      sessions = nrow(d),
      avg_retention = round(mean(d$retention), 1),
      avg_fatigue_drop = round(mean(d$fatigue_drop, na.rm = TRUE), 1)
    )
  })
  by_duration <- do.call(rbind, Filter(Negate(is.null), by_duration))

  # Find optimal session length
  if (!is.null(by_duration) && nrow(by_duration) > 0) {
    optimal_idx <- which.max(by_duration$avg_retention - by_duration$avg_fatigue_drop * 0.5)
    optimal_duration <- by_duration$duration_range[optimal_idx]
  } else {
    optimal_duration <- "Unable to determine"
  }

  # Recommendations
  recommendations <- character()

  avg_fatigue <- mean(sessions$fatigue_drop, na.rm = TRUE)
  if (avg_fatigue > 5) {
    recommendations <- c(recommendations,
      paste0("Average retention drops ", round(avg_fatigue, 1),
             "% in second half of sessions. Consider shorter sessions or breaks."))
  }

  if (!is.null(by_duration) && nrow(by_duration) > 0) {
    recommendations <- c(recommendations,
      paste0("Your optimal session length appears to be ", optimal_duration))
  }

  list(
    sessions = tibble::as_tibble(sessions),
    summary = summary_stats,
    by_duration = if (!is.null(by_duration)) tibble::as_tibble(by_duration) else tibble::tibble(),
    optimal_duration = optimal_duration,
    recommendations = recommendations
  )
}

#' Simulate a study session
#'
#' Given a time budget, predict how many cards you'll review, expected retention,
#' and workload distribution.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param minutes Available study time in minutes (default 30)
#' @param include_new Whether to include new cards (default TRUE)
#' @return A tibble with session simulation
#' @export
#' @examples
#' \dontrun{
#' sim <- anki_simulate_session(minutes = 30)
#' }
anki_simulate_session <- function(path = NULL, profile = NULL, minutes = 30,
                                   include_new = TRUE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  revlog <- col$revlog()

  # Calculate average time per card type from history
  recent_revlog <- revlog[revlog$review_date >= Sys.Date() - 30, ]

  if (nrow(recent_revlog) == 0) {
    # Use defaults
    avg_time_new <- 15  # seconds
    avg_time_learn <- 8
    avg_time_review <- 6
    avg_retention <- 0.85
  } else {
    # Join with card types
    recent_revlog <- merge(recent_revlog, cards[, c("cid", "type", "ivl")],
                           by = "cid", all.x = TRUE)

    # New cards (first review, ivl = 0 at time of review)
    new_reviews <- recent_revlog[recent_revlog$ivl == 0 | is.na(recent_revlog$ivl), ]
    avg_time_new <- if (nrow(new_reviews) > 0) {
      mean(new_reviews$time, na.rm = TRUE) / 1000
    } else 15

    # Learning cards
    learn_reviews <- recent_revlog[recent_revlog$ivl < 0, ]
    avg_time_learn <- if (nrow(learn_reviews) > 0) {
      mean(learn_reviews$time, na.rm = TRUE) / 1000
    } else 8

    # Review cards
    review_reviews <- recent_revlog[recent_revlog$ivl > 0, ]
    avg_time_review <- if (nrow(review_reviews) > 0) {
      mean(review_reviews$time, na.rm = TRUE) / 1000
    } else 6

    avg_retention <- 1 - sum(recent_revlog$ease == 1) / nrow(recent_revlog)
  }

  # Count current due cards
  today <- as.integer(Sys.Date())
  due_review <- sum(cards$queue == 2 & cards$due <= today)
  due_learn <- sum(cards$queue %in% c(1, 3))
  new_cards <- sum(cards$type == 0)

  # Calculate how many we can do in time budget
  total_seconds <- minutes * 60

  # Priority: learning > review > new
  result <- tibble::tibble(
    card_type = character(),
    available = integer(),
    estimated_cards = integer(),
    estimated_time_min = numeric(),
    expected_retention = numeric()
  )

  remaining_time <- total_seconds

  # Learning cards first
  if (due_learn > 0 && remaining_time > 0) {
    learn_cards <- min(due_learn, floor(remaining_time / avg_time_learn))
    learn_time <- learn_cards * avg_time_learn
    remaining_time <- remaining_time - learn_time

    result <- rbind(result, tibble::tibble(
      card_type = "Learning",
      available = due_learn,
      estimated_cards = learn_cards,
      estimated_time_min = round(learn_time / 60, 1),
      expected_retention = round(avg_retention * 100, 1)
    ))
  }

  # Review cards
  if (due_review > 0 && remaining_time > 0) {
    review_cards <- min(due_review, floor(remaining_time / avg_time_review))
    review_time <- review_cards * avg_time_review
    remaining_time <- remaining_time - review_time

    result <- rbind(result, tibble::tibble(
      card_type = "Review",
      available = due_review,
      estimated_cards = review_cards,
      estimated_time_min = round(review_time / 60, 1),
      expected_retention = round(avg_retention * 100, 1)
    ))
  }

  # New cards
  if (include_new && new_cards > 0 && remaining_time > 0) {
    new_count <- min(new_cards, floor(remaining_time / avg_time_new), 20)  # Cap at 20
    new_time <- new_count * avg_time_new
    remaining_time <- remaining_time - new_time

    result <- rbind(result, tibble::tibble(
      card_type = "New",
      available = new_cards,
      estimated_cards = new_count,
      estimated_time_min = round(new_time / 60, 1),
      expected_retention = round(70, 1)  # New cards have lower initial retention
    ))
  }

  # Summary
  total_cards <- sum(result$estimated_cards)
  total_time_used <- sum(result$estimated_time_min)
  overall_retention <- if (total_cards > 0) {
    sum(result$estimated_cards * result$expected_retention) / total_cards
  } else 0

  list(
    breakdown = tibble::as_tibble(result),
    summary = tibble::tibble(
      metric = c("Time Budget (min)", "Time Used (min)", "Time Remaining (min)",
                 "Total Cards", "Expected Avg Retention (%)"),
      value = c(minutes, round(total_time_used, 1), round(remaining_time / 60, 1),
                total_cards, round(overall_retention, 1))
    ),
    timing_estimates = tibble::tibble(
      card_type = c("New", "Learning", "Review"),
      avg_seconds = round(c(avg_time_new, avg_time_learn, avg_time_review), 1)
    )
  )
}
