#' Cohort Analysis (Vintage Analysis)
#'
#' Compare card performance by when they were added. Cards added in the same
#' period form a "cohort" and their learning outcomes are compared.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param cohort_size Size of each cohort: "week", "month", "quarter" (default "month")
#' @param min_cards Minimum cards per cohort to include (default 20)
#' @return A tibble with cohort statistics
#' @export
#'
#' @examples
#' \dontrun{
#' cohorts <- anki_cohort_analysis()
#' cohorts
#' }
anki_cohort_analysis <- function(path = NULL, profile = NULL,
                                 cohort_size = "month",
                                 min_cards = 20) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  revlog <- col$revlog()
  
  if (nrow(cards) < 50) {
    message("Insufficient cards for cohort analysis")
    return(tibble::tibble())
  }
  
  # Convert card ID to creation date
  cards$created_date <- as.Date(as.POSIXct(cards$cid / 1000, origin = "1970-01-01"))
  
  # Assign cohorts
  cards$cohort <- switch(cohort_size,
    "week" = format(cards$created_date, "%Y-W%V"),
    "month" = format(cards$created_date, "%Y-%m"),
    "quarter" = paste0(format(cards$created_date, "%Y"), "-Q", ceiling(as.numeric(format(cards$created_date, "%m")) / 3)),
    format(cards$created_date, "%Y-%m")
  )
  
  # Get review stats per card
  review_stats <- aggregate(
    cbind(total_reviews = ease, pass_rate = ease > 1) ~ cid,
    data = revlog,
    FUN = function(x) if (length(x) > 0) c(length(x), mean(x > 1))[1] else 0
  )
  review_counts <- aggregate(ease ~ cid, data = revlog, FUN = length)
  names(review_counts) <- c("cid", "total_reviews")
  
  pass_rates <- aggregate(ease > 1 ~ cid, data = revlog, FUN = mean)
  names(pass_rates) <- c("cid", "retention")
  
  # Merge with cards
  card_stats <- merge(cards, review_counts, by = "cid", all.x = TRUE)
  card_stats <- merge(card_stats, pass_rates, by = "cid", all.x = TRUE)
  card_stats$total_reviews[is.na(card_stats$total_reviews)] <- 0
  card_stats$retention[is.na(card_stats$retention)] <- NA
  
  # Aggregate by cohort
  cohort_summary <- aggregate(
    cbind(n_cards = 1, mature = type == 2 & ivl >= 21, 
          avg_interval = ivl, avg_reps = reps, avg_lapses = lapses) ~ cohort,
    data = card_stats,
    FUN = function(x) if (is.logical(x)) sum(x, na.rm = TRUE) else mean(x, na.rm = TRUE)
  )
  
  # Get card counts
  card_counts <- aggregate(cid ~ cohort, data = card_stats, FUN = length)
  names(card_counts) <- c("cohort", "n_cards")
  
  # Get mature counts
  mature_counts <- aggregate((type == 2 & ivl >= 21) ~ cohort, data = card_stats, FUN = sum)
  names(mature_counts) <- c("cohort", "mature_cards")
  
  # Get retention by cohort
  retention_agg <- aggregate(retention ~ cohort, data = card_stats, 
                             FUN = function(x) mean(x, na.rm = TRUE))
  
  # Get averages
  avg_stats <- aggregate(
    cbind(avg_interval = ivl, avg_reps = reps, avg_lapses = lapses) ~ cohort,
    data = card_stats, FUN = function(x) mean(x, na.rm = TRUE)
  )
  
  # Merge all
  result <- merge(card_counts, mature_counts, by = "cohort", all.x = TRUE)
  result <- merge(result, retention_agg, by = "cohort", all.x = TRUE)
  result <- merge(result, avg_stats, by = "cohort", all.x = TRUE)
  
  # Filter by minimum cards
  result <- result[result$n_cards >= min_cards, ]
  
  # Calculate derived metrics
  result$mature_pct <- round(result$mature_cards / result$n_cards * 100, 1)
  result$retention <- round(result$retention * 100, 1)
  result$avg_interval <- round(result$avg_interval, 1)
  result$avg_reps <- round(result$avg_reps, 1)
  result$avg_lapses <- round(result$avg_lapses, 2)
  result$lapse_rate <- round(result$avg_lapses / pmax(1, result$avg_reps) * 100, 1)
  
  # Sort by cohort
  result <- result[order(result$cohort), ]
  
  tibble::as_tibble(result[, c("cohort", "n_cards", "mature_cards", "mature_pct", 
                                "retention", "avg_interval", "avg_reps", 
                                "avg_lapses", "lapse_rate")])
}

#' Learning Velocity Analysis
#'
#' Track learning rate over time: cards learned per day, time to graduation,
#' acceleration or deceleration of learning.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param period Analysis period: "week", "month", "quarter", "all" (default "month")
#' @return A list with velocity metrics and trends
#' @export
#'
#' @examples
#' \dontrun{
#' velocity <- anki_learning_velocity()
#' velocity$current
#' velocity$trend
#' }
anki_learning_velocity <- function(path = NULL, profile = NULL, period = "month") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  revlog <- col$revlog()
  
  if (nrow(cards) < 20 || nrow(revlog) < 50) {
    message("Insufficient data for velocity analysis")
    return(list())
  }
  
  today <- Sys.Date()
  
  # Determine period
  start_date <- switch(period,
    "week" = today - 7,
    "month" = today - 30,
    "quarter" = today - 90,
    "all" = min(revlog$review_date, na.rm = TRUE),
    today - 30
  )
  
  recent_revlog <- revlog[revlog$review_date >= start_date, ]
  
  # 1. Cards learned (first review) per day
  first_reviews <- revlog[!duplicated(revlog$cid), ]
  first_reviews_recent <- first_reviews[first_reviews$review_date >= start_date, ]
  
  if (nrow(first_reviews_recent) > 0) {
    cards_per_day <- aggregate(cid ~ review_date, data = first_reviews_recent, FUN = length)
    names(cards_per_day) <- c("date", "new_cards_learned")
    avg_new_per_day <- mean(cards_per_day$new_cards_learned, na.rm = TRUE)
  } else {
    avg_new_per_day <- 0
    cards_per_day <- data.frame(date = Sys.Date(), new_cards_learned = 0)
  }
  
  # 2. Learning rate trend
  current_total <- nrow(first_reviews_recent)
  period_days <- as.numeric(today - start_date)
  
  if (period != "all" && nrow(first_reviews) > 0) {
    comparison_start <- start_date - period_days
    previous_period <- first_reviews[first_reviews$review_date >= comparison_start & 
                                      first_reviews$review_date < start_date, ]
    previous_total <- nrow(previous_period)
    
    if (previous_total > 0) {
      velocity_change <- (current_total - previous_total) / previous_total * 100
      trend <- if (velocity_change > 15) "accelerating" 
               else if (velocity_change < -15) "decelerating" 
               else "steady"
    } else {
      velocity_change <- NA
      trend <- "unknown"
    }
  } else {
    velocity_change <- NA
    trend <- "unknown"
    previous_total <- NA
  }
  
  # 3. Review velocity
  if (nrow(recent_revlog) > 0) {
    daily_reviews <- aggregate(ease ~ review_date, data = recent_revlog, FUN = length)
    avg_reviews_per_day <- mean(daily_reviews$ease, na.rm = TRUE)
    
    # Time spent velocity
    daily_time <- aggregate(time ~ review_date, data = recent_revlog, FUN = sum)
    avg_time_per_day <- mean(daily_time$time, na.rm = TRUE) / 60000
  } else {
    avg_reviews_per_day <- 0
    avg_time_per_day <- 0
  }
  
  # 4. Weekly breakdown
  first_reviews_90d <- first_reviews[first_reviews$review_date >= (today - 90), ]
  if (nrow(first_reviews_90d) > 0) {
    weekly_velocity <- aggregate(
      cid ~ format(review_date, "%Y-W%V"),
      data = first_reviews_90d,
      FUN = length
    )
    names(weekly_velocity) <- c("week", "cards_learned")
    weekly_velocity <- weekly_velocity[order(weekly_velocity$week), ]
  } else {
    weekly_velocity <- tibble::tibble(week = character(), cards_learned = integer())
  }
  
  # 5. Retention trend
  retention_trend <- "unknown"
  if (nrow(recent_revlog) > 100) {
    recent_revlog$week <- format(recent_revlog$review_date, "%Y-W%V")
    weekly_retention <- aggregate(ease > 1 ~ week, data = recent_revlog, FUN = mean)
    names(weekly_retention) <- c("week", "retention")
    
    if (nrow(weekly_retention) >= 3) {
      ret_cor <- cor(seq_len(nrow(weekly_retention)), weekly_retention$retention)
      retention_trend <- if (ret_cor > 0.3) "improving" 
                         else if (ret_cor < -0.3) "declining" 
                         else "stable"
    }
  }
  
  list(
    current = list(
      new_cards_per_day = round(avg_new_per_day, 1),
      reviews_per_day = round(avg_reviews_per_day, 1),
      time_per_day_min = round(avg_time_per_day, 1)
    ),
    trend = list(
      velocity_change_pct = if (!is.na(velocity_change)) round(velocity_change, 1) else NA,
      direction = trend,
      current_period_cards = current_total,
      previous_period_cards = previous_total,
      retention_trend = retention_trend
    ),
    weekly_breakdown = tibble::as_tibble(weekly_velocity),
    period = list(
      name = period,
      start = as.character(start_date),
      end = as.character(today),
      days = period_days
    ),
    summary = tibble::tibble(
      metric = c("New Cards/Day", "Reviews/Day", "Time/Day (min)", "Velocity Trend", "Retention Trend"),
      value = c(as.character(round(avg_new_per_day, 1)), 
                as.character(round(avg_reviews_per_day, 1)), 
                as.character(round(avg_time_per_day, 1)), 
                trend, retention_trend)
    )
  )
}

#' Backlog Calculator
#'
#' Calculate how long it would take to clear your review backlog
#' at different review rates, and project backlog growth if you stop studying.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param new_cards_per_day Expected new cards per day (0 = maintenance mode)
#' @return A list with backlog analysis and projections
#' @export
#'
#' @examples
#' \dontrun{
#' backlog <- anki_backlog_calculator()
#' backlog$current
#' backlog$scenarios
#' }
anki_backlog_calculator <- function(path = NULL, profile = NULL,
                                    new_cards_per_day = 0) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  revlog <- col$revlog()
  
  today <- as.numeric(Sys.Date() - as.Date("1970-01-01"))
  
  # Current due counts
  due_now <- sum(cards$type == 2 & cards$due <= today, na.rm = TRUE)
  learning <- sum(cards$type %in% c(1, 3), na.rm = TRUE)
  new_cards <- sum(cards$type == 0, na.rm = TRUE)
  
  current_backlog <- due_now + learning
  
  # Historical review rate
  daily_reviews <- aggregate(ease ~ review_date, data = revlog, FUN = length)
  avg_daily_reviews <- mean(daily_reviews$ease, na.rm = TRUE)
  max_daily_reviews <- max(daily_reviews$ease, na.rm = TRUE)
  
  # Estimate daily accumulation (cards becoming due)
  mature_cards <- sum(cards$type == 2 & cards$ivl >= 21, na.rm = TRUE)
  avg_interval <- mean(cards$ivl[cards$type == 2 & cards$ivl > 0], na.rm = TRUE)
  daily_accumulation <- if (!is.na(avg_interval) && avg_interval > 0) {
    mature_cards / avg_interval
  } else {
    0
  }
  
  # Scenarios
  scenarios <- tibble::tibble(
    scenario = c("Light (50%)", "Normal (100%)", "Intensive (150%)", "Marathon (200%)"),
    reviews_per_day = round(c(avg_daily_reviews * 0.5, avg_daily_reviews, 
                              avg_daily_reviews * 1.5, avg_daily_reviews * 2)),
    net_daily_change = round(c(avg_daily_reviews * 0.5, avg_daily_reviews, 
                               avg_daily_reviews * 1.5, avg_daily_reviews * 2) - 
                             daily_accumulation - new_cards_per_day),
    days_to_clear = NA_integer_,
    clear_date = as.character(NA)
  )
  
  for (i in seq_len(nrow(scenarios))) {
    net <- scenarios$net_daily_change[i]
    if (net > 0 && current_backlog > 0) {
      days <- ceiling(current_backlog / net)
      scenarios$days_to_clear[i] <- days
      scenarios$clear_date[i] <- as.character(Sys.Date() + days)
    } else if (current_backlog == 0) {
      scenarios$days_to_clear[i] <- 0
      scenarios$clear_date[i] <- as.character(Sys.Date())
    }
  }
  
  # "What if I stop" projections
  stop_analysis <- tibble::tibble(
    days_stopped = c(1, 3, 7, 14, 30),
    accumulated = round(current_backlog + (daily_accumulation + new_cards_per_day) * c(1, 3, 7, 14, 30)),
    days_to_recover = ceiling((current_backlog + (daily_accumulation + new_cards_per_day) * c(1, 3, 7, 14, 30)) / 
                              pmax(1, avg_daily_reviews - daily_accumulation))
  )
  
  # Recommendation
  if (current_backlog > avg_daily_reviews * 7) {
    recommendation <- "Significant backlog - consider stopping new cards until caught up"
  } else if (current_backlog > avg_daily_reviews * 3) {
    recommendation <- "Moderate backlog - reduce new cards and focus on reviews"
  } else if (current_backlog > avg_daily_reviews) {
    recommendation <- "Small backlog - one good session will clear it"
  } else {
    recommendation <- "No backlog - you're on top of your reviews!"
  }
  
  list(
    current = tibble::tibble(
      metric = c("Due Now", "In Learning", "New Cards Waiting", "Total Backlog", 
                 "Your Avg Reviews/Day", "Daily Accumulation Rate"),
      value = c(due_now, learning, new_cards, current_backlog, 
                round(avg_daily_reviews, 1), round(daily_accumulation, 1))
    ),
    scenarios = scenarios,
    stop_analysis = stop_analysis,
    recommendation = recommendation
  )
}
