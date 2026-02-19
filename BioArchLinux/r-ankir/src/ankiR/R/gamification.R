#' Gamification Stats
#'
#' Calculate XP, level, achievements, and progress toward goals based on
#' your Anki review history. Makes studying more engaging!
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param xp_per_review XP awarded per review (default 10)
#' @param xp_per_correct Bonus XP for correct answer (default 5)
#' @param xp_per_streak Bonus XP per day of streak (default 25)
#' @return A list with XP, level, achievements, and stats
#' @export
#'
#' @examples
#' \dontrun{
#' stats <- anki_gamification()
#' stats$level
#' stats$achievements
#' }
anki_gamification <- function(path = NULL, profile = NULL,
                              xp_per_review = 10,
                              xp_per_correct = 5,
                              xp_per_streak = 25) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  revlog <- col$revlog()
  
  if (nrow(revlog) < 10) {
    message("Need more review history for gamification stats")
    return(list())
  }
  
  # Calculate total XP
  total_reviews <- nrow(revlog)
  correct_reviews <- sum(revlog$ease > 1, na.rm = TRUE)
  
  # Get streak info
  review_dates <- sort(unique(revlog$review_date))
  today <- Sys.Date()
  
  # Current streak
  current_streak <- 0
  check_date <- today
  while (check_date %in% review_dates) {
    current_streak <- current_streak + 1
    check_date <- check_date - 1
  }
  
  # Longest streak
  if (length(review_dates) > 1) {
    date_diffs <- diff(review_dates)
    streak_breaks <- which(date_diffs > 1)
    if (length(streak_breaks) > 0) {
      streak_starts <- c(1, streak_breaks + 1)
      streak_ends <- c(streak_breaks, length(review_dates))
      streak_lengths <- streak_ends - streak_starts + 1
      longest_streak <- max(streak_lengths)
    } else {
      longest_streak <- length(review_dates)
    }
  } else {
    longest_streak <- length(review_dates)
  }
  
  # Calculate XP
  base_xp <- total_reviews * xp_per_review
  correct_bonus <- correct_reviews * xp_per_correct
  streak_bonus <- current_streak * xp_per_streak
  total_xp <- base_xp + correct_bonus + streak_bonus
  
  # Calculate level (exponential scaling)
  # Level formula: xp_needed = 500 * level^1.5
  level <- 1
  xp_for_next <- 500
  cumulative_xp <- 0
  while (cumulative_xp + xp_for_next <= total_xp) {
    cumulative_xp <- cumulative_xp + xp_for_next
    level <- level + 1
    xp_for_next <- round(500 * level^1.5)
  }
  xp_into_level <- total_xp - cumulative_xp
  xp_for_current_level <- round(500 * level^1.5)
  level_progress <- xp_into_level / xp_for_current_level * 100
  
  # Level titles
  level_title <- if (level >= 50) "Grandmaster" 
                 else if (level >= 40) "Master"
                 else if (level >= 30) "Expert"
                 else if (level >= 20) "Veteran"
                 else if (level >= 15) "Adept"
                 else if (level >= 10) "Scholar"
                 else if (level >= 5) "Student"
                 else "Novice"
  
  # Check achievements
  achievements <- list()
  
  # Review count achievements
  if (total_reviews >= 1) achievements$first_review <- list(name = "First Steps", desc = "Complete your first review", earned = TRUE)
  if (total_reviews >= 100) achievements$century <- list(name = "Century", desc = "Complete 100 reviews", earned = TRUE)
  if (total_reviews >= 1000) achievements$millennium <- list(name = "Millennium", desc = "Complete 1,000 reviews", earned = TRUE)
  if (total_reviews >= 10000) achievements$ten_k <- list(name = "Dedicated", desc = "Complete 10,000 reviews", earned = TRUE)
  if (total_reviews >= 50000) achievements$fifty_k <- list(name = "Committed", desc = "Complete 50,000 reviews", earned = TRUE)
  if (total_reviews >= 100000) achievements$hundred_k <- list(name = "Legendary", desc = "Complete 100,000 reviews", earned = TRUE)
  
  # Streak achievements
  if (longest_streak >= 7) achievements$week_streak <- list(name = "Week Warrior", desc = "7-day streak", earned = TRUE)
  if (longest_streak >= 30) achievements$month_streak <- list(name = "Monthly Master", desc = "30-day streak", earned = TRUE)
  if (longest_streak >= 100) achievements$hundred_days <- list(name = "Centurion", desc = "100-day streak", earned = TRUE)
  if (longest_streak >= 365) achievements$year_streak <- list(name = "Yearly Legend", desc = "365-day streak", earned = TRUE)
  
  # Retention achievements
  overall_retention <- mean(revlog$ease > 1, na.rm = TRUE)
  if (overall_retention >= 0.90) achievements$high_retention <- list(name = "Sharp Mind", desc = "90%+ retention", earned = TRUE)
  if (overall_retention >= 0.95) achievements$elite_retention <- list(name = "Elite Memory", desc = "95%+ retention", earned = TRUE)
  
  # Card count achievements
  mature_cards <- sum(cards$type == 2 & cards$ivl >= 21, na.rm = TRUE)
  if (mature_cards >= 100) achievements$mature_100 <- list(name = "Foundation", desc = "100 mature cards", earned = TRUE)
  if (mature_cards >= 500) achievements$mature_500 <- list(name = "Growing", desc = "500 mature cards", earned = TRUE)
  if (mature_cards >= 1000) achievements$mature_1k <- list(name = "Established", desc = "1,000 mature cards", earned = TRUE)
  if (mature_cards >= 5000) achievements$mature_5k <- list(name = "Extensive", desc = "5,000 mature cards", earned = TRUE)
  if (mature_cards >= 10000) achievements$mature_10k <- list(name = "Encyclopedic", desc = "10,000 mature cards", earned = TRUE)
  
  # Time spent achievements
  total_time_hours <- sum(revlog$time, na.rm = TRUE) / 3600000
  if (total_time_hours >= 10) achievements$time_10h <- list(name = "Time Invested", desc = "10 hours studied", earned = TRUE)
  if (total_time_hours >= 100) achievements$time_100h <- list(name = "Serious Student", desc = "100 hours studied", earned = TRUE)
  if (total_time_hours >= 500) achievements$time_500h <- list(name = "Marathon Learner", desc = "500 hours studied", earned = TRUE)
  
  # Perfect day achievements
  daily_retention <- aggregate(ease > 1 ~ review_date, data = revlog, FUN = mean)
  perfect_days <- sum(daily_retention[,2] == 1 & 
                      aggregate(ease ~ review_date, data = revlog, FUN = length)[,2] >= 20)
  if (perfect_days >= 1) achievements$perfect_day <- list(name = "Perfect Day", desc = "100% retention (20+ reviews)", earned = TRUE)
  if (perfect_days >= 10) achievements$perfect_ten <- list(name = "Flawless", desc = "10 perfect days", earned = TRUE)
  
  # Today's stats
  today_reviews <- revlog[revlog$review_date == today, ]
  today_xp <- nrow(today_reviews) * xp_per_review + 
              sum(today_reviews$ease > 1) * xp_per_correct
  
  # Weekly goal progress (assume 700 reviews/week goal)
  week_start <- today - as.numeric(format(today, "%u")) + 1
  week_reviews <- sum(revlog$review_date >= week_start & revlog$review_date <= today)
  weekly_goal <- 700
  weekly_progress <- min(100, week_reviews / weekly_goal * 100)
  
  # Create achievements summary
  earned_achievements <- Filter(function(x) x$earned, achievements)
  achievement_df <- tibble::tibble(
    name = sapply(earned_achievements, function(x) x$name),
    description = sapply(earned_achievements, function(x) x$desc)
  )
  
  list(
    xp = list(
      total = total_xp,
      base = base_xp,
      correct_bonus = correct_bonus,
      streak_bonus = streak_bonus,
      today = today_xp
    ),
    level = list(
      current = level,
      title = level_title,
      progress_pct = round(level_progress, 1),
      xp_into_level = xp_into_level,
      xp_for_next = xp_for_current_level
    ),
    streak = list(
      current = current_streak,
      longest = longest_streak
    ),
    stats = list(
      total_reviews = total_reviews,
      correct_reviews = correct_reviews,
      retention_pct = round(overall_retention * 100, 1),
      mature_cards = mature_cards,
      total_hours = round(total_time_hours, 1)
    ),
    achievements = list(
      earned_count = length(earned_achievements),
      total_possible = 22,
      earned = achievement_df
    ),
    goals = list(
      weekly_reviews = week_reviews,
      weekly_goal = weekly_goal,
      weekly_progress_pct = round(weekly_progress, 1)
    ),
    summary = tibble::tibble(
      metric = c("Level", "Total XP", "Current Streak", "Longest Streak", 
                 "Total Reviews", "Retention", "Achievements"),
      value = c(paste0(level, " (", level_title, ")"), 
                format(total_xp, big.mark = ","),
                paste0(current_streak, " days"),
                paste0(longest_streak, " days"),
                format(total_reviews, big.mark = ","),
                paste0(round(overall_retention * 100, 1), "%"),
                paste0(length(earned_achievements), "/22"))
    )
  )
}

#' Print Gamification Stats
#'
#' @param x Gamification stats object
#' @param ... Additional arguments
#' @export
print.anki_gamification <- function(x, ...) {
  cat("\n=== ANKI STATS ===\n\n")
  cat("Level:", x$level$current, "-", x$level$title, "\n")
  cat("XP:", format(x$xp$total, big.mark = ","), "\n")
  cat("Progress to next level:", x$level$progress_pct, "%\n\n")
  cat("Current Streak:", x$streak$current, "days\n")
  cat("Longest Streak:", x$streak$longest, "days\n\n")
  cat("Achievements:", x$achievements$earned_count, "/", x$achievements$total_possible, "\n")
  invisible(x)
}
