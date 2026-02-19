#' Detect Burnout Warning Signs
#'
#' Analyzes study patterns to detect potential burnout indicators:
#' declining retention, increasing response time, shorter sessions,
#' more "Again" presses, decreased consistency.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze (default 30)
#' @param baseline_days Days to use as baseline for comparison (default 90)
#' @return A list with burnout indicators and recommendations
#' @export
#'
#' @examples
#' \dontrun{
#' burnout <- anki_burnout_detection()
#' burnout$risk_level
#' burnout$indicators
#' }
anki_burnout_detection <- function(path = NULL, profile = NULL, 
                                   days = 30, baseline_days = 90) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  revlog <- col$revlog()
  
  if (nrow(revlog) < 100) {
    message("Insufficient review data for burnout detection")
    return(list(risk_level = "unknown", message = "Need more data"))
  }
  
  today <- Sys.Date()
  recent_cutoff <- today - days
  baseline_cutoff <- today - baseline_days
  
  recent <- revlog[revlog$review_date >= recent_cutoff, ]
  baseline <- revlog[revlog$review_date >= baseline_cutoff & 
                     revlog$review_date < recent_cutoff, ]
  
  if (nrow(baseline) < 50 || nrow(recent) < 20) {
    message("Insufficient data in baseline or recent period")
    return(list(risk_level = "unknown", message = "Need more data"))
  }
  
  indicators <- list()
  risk_scores <- numeric()
  
  # 1. Retention decline
  recent_retention <- mean(recent$ease > 1, na.rm = TRUE)
  baseline_retention <- mean(baseline$ease > 1, na.rm = TRUE)
  retention_change <- recent_retention - baseline_retention
  indicators$retention <- list(
    recent = round(recent_retention * 100, 1),
    baseline = round(baseline_retention * 100, 1),
    change = round(retention_change * 100, 1),
    status = if (retention_change < -0.05) "declining" else if (retention_change > 0.02) "improving" else "stable"
  )
  risk_scores <- c(risk_scores, if (retention_change < -0.1) 3 else if (retention_change < -0.05) 2 else if (retention_change < -0.02) 1 else 0)
  
  # 2. Response time increase
  recent_time <- median(recent$time, na.rm = TRUE)
  baseline_time <- median(baseline$time, na.rm = TRUE)
  time_change_pct <- (recent_time - baseline_time) / baseline_time * 100
  indicators$response_time <- list(
    recent_ms = round(recent_time),
    baseline_ms = round(baseline_time),
    change_pct = round(time_change_pct, 1),
    status = if (time_change_pct > 30) "much_slower" else if (time_change_pct > 15) "slower" else if (time_change_pct < -20) "rushed" else "normal"
  )
  risk_scores <- c(risk_scores, if (abs(time_change_pct) > 30) 2 else if (abs(time_change_pct) > 15) 1 else 0)
  
  # 3. "Again" rate increase
  recent_again <- mean(recent$ease == 1, na.rm = TRUE)
  baseline_again <- mean(baseline$ease == 1, na.rm = TRUE)
  again_change <- recent_again - baseline_again
  indicators$again_rate <- list(
    recent = round(recent_again * 100, 1),
    baseline = round(baseline_again * 100, 1),
    change = round(again_change * 100, 1),
    status = if (again_change > 0.05) "increasing" else if (again_change < -0.02) "decreasing" else "stable"
  )
  risk_scores <- c(risk_scores, if (again_change > 0.1) 3 else if (again_change > 0.05) 2 else if (again_change > 0.02) 1 else 0)
  
  # 4. Session length decline
  recent_daily <- aggregate(time ~ review_date, recent, sum)
  baseline_daily <- aggregate(time ~ review_date, baseline, sum)
  recent_session <- mean(recent_daily$time, na.rm = TRUE) / 60000
  baseline_session <- mean(baseline_daily$time, na.rm = TRUE) / 60000
  session_change_pct <- (recent_session - baseline_session) / baseline_session * 100
  indicators$session_length <- list(
    recent_min = round(recent_session, 1),
    baseline_min = round(baseline_session, 1),
    change_pct = round(session_change_pct, 1),
    status = if (session_change_pct < -30) "much_shorter" else if (session_change_pct < -15) "shorter" else "normal"
  )
  risk_scores <- c(risk_scores, if (session_change_pct < -30) 2 else if (session_change_pct < -15) 1 else 0)
  
  # 5. Consistency decline
  recent_study_days <- length(unique(recent$review_date))
  baseline_study_days <- length(unique(baseline$review_date))
  recent_study_rate <- recent_study_days / days
  baseline_study_rate <- baseline_study_days / (baseline_days - days)
  consistency_change <- recent_study_rate - baseline_study_rate
  indicators$consistency <- list(
    recent_rate = round(recent_study_rate * 100, 1),
    baseline_rate = round(baseline_study_rate * 100, 1),
    change = round(consistency_change * 100, 1),
    status = if (consistency_change < -0.15) "declining" else if (consistency_change > 0.05) "improving" else "stable"
  )
  risk_scores <- c(risk_scores, if (consistency_change < -0.2) 2 else if (consistency_change < -0.1) 1 else 0)
  
  # 6. "Easy" button avoidance
  recent_easy <- mean(recent$ease == 4, na.rm = TRUE)
  baseline_easy <- mean(baseline$ease == 4, na.rm = TRUE)
  easy_change <- recent_easy - baseline_easy
  indicators$easy_usage <- list(
    recent = round(recent_easy * 100, 1),
    baseline = round(baseline_easy * 100, 1),
    change = round(easy_change * 100, 1),
    status = if (easy_change < -0.05) "avoiding" else "normal"
  )
  risk_scores <- c(risk_scores, if (easy_change < -0.1) 1 else 0)
  
  # Calculate overall risk
  total_risk <- sum(risk_scores)
  risk_level <- if (total_risk >= 8) "high" else if (total_risk >= 5) "moderate" else if (total_risk >= 2) "low" else "minimal"
  
  # Generate recommendations
  recommendations <- character()
  if (indicators$retention$status == "declining") {
    recommendations <- c(recommendations, "Consider reducing new cards temporarily to focus on retention")
  }
  if (indicators$response_time$status == "rushed") {
    recommendations <- c(recommendations, "Try to slow down - rushed reviews often lead to forgetting")
  }
  if (indicators$response_time$status %in% c("slower", "much_slower")) {
    recommendations <- c(recommendations, "Review sessions may be too long - try shorter, more frequent sessions")
  }
  if (indicators$again_rate$status == "increasing") {
    recommendations <- c(recommendations, "High lapse rate - consider reviewing fewer cards or taking a break")
  }
  if (indicators$session_length$status %in% c("shorter", "much_shorter")) {
    recommendations <- c(recommendations, "Sessions getting shorter - may indicate fatigue or loss of motivation")
  }
  if (indicators$consistency$status == "declining") {
    recommendations <- c(recommendations, "Study frequency declining - try setting a specific daily study time")
  }
  if (length(recommendations) == 0) {
    recommendations <- "Your study patterns look healthy! Keep it up."
  }
  
  list(
    risk_level = risk_level,
    risk_score = total_risk,
    max_risk_score = 14,
    period = list(recent_days = days, baseline_days = baseline_days),
    indicators = indicators,
    recommendations = recommendations,
    summary = tibble::tibble(
      indicator = c("Retention", "Response Time", "Again Rate", "Session Length", "Consistency", "Easy Usage"),
      status = c(indicators$retention$status, indicators$response_time$status, 
                 indicators$again_rate$status, indicators$session_length$status,
                 indicators$consistency$status, indicators$easy_usage$status),
      recent = c(indicators$retention$recent, indicators$response_time$recent_ms,
                 indicators$again_rate$recent, indicators$session_length$recent_min,
                 indicators$consistency$recent_rate, indicators$easy_usage$recent),
      baseline = c(indicators$retention$baseline, indicators$response_time$baseline_ms,
                   indicators$again_rate$baseline, indicators$session_length$baseline_min,
                   indicators$consistency$baseline_rate, indicators$easy_usage$baseline)
    )
  )
}

#' Review Quality Score
#'
#' Detects low-quality reviews: pattern clicking, rushed reviews,
#' suspicious timing patterns, and other indicators of disengaged studying.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to analyze (default 30)
#' @param min_time_ms Minimum expected review time in ms (default 800)
#' @param max_time_ms Maximum expected review time in ms (default 60000)
#' @return A list with quality metrics and flagged reviews
#' @export
#'
#' @examples
#' \dontrun{
#' quality <- anki_review_quality()
#' quality$score
#' quality$issues
#' }
anki_review_quality <- function(path = NULL, profile = NULL, days = 30,
                                min_time_ms = 800, max_time_ms = 60000) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  revlog <- col$revlog()
  
  cutoff <- Sys.Date() - days
  recent <- revlog[revlog$review_date >= cutoff, ]
  
  if (nrow(recent) < 50) {
    message("Insufficient recent reviews for quality analysis")
    return(list(score = NA, message = "Need more data"))
  }
  
  issues <- list()
  quality_penalties <- 0
  
  # 1. Rushed reviews
  rushed <- recent[recent$time < min_time_ms & recent$time > 0, ]
  rushed_pct <- nrow(rushed) / nrow(recent) * 100
  issues$rushed_reviews <- list(
    count = nrow(rushed),
    percentage = round(rushed_pct, 1),
    threshold_ms = min_time_ms,
    severity = if (rushed_pct > 20) "high" else if (rushed_pct > 10) "medium" else "low"
  )
  quality_penalties <- quality_penalties + min(rushed_pct * 2, 30)
  
  # 2. Suspiciously consistent timing
  time_sd <- sd(recent$time, na.rm = TRUE)
  time_mean <- mean(recent$time, na.rm = TRUE)
  cv <- time_sd / time_mean
  issues$timing_consistency <- list(
    cv = round(cv, 3),
    status = if (cv < 0.3) "suspicious" else if (cv < 0.5) "very_consistent" else "normal"
  )
  if (cv < 0.3) quality_penalties <- quality_penalties + 15
  
  # 3. Pattern clicking
  recent_sorted <- recent[order(recent$rid), ]
  if (nrow(recent_sorted) >= 10) {
    rle_result <- rle(recent_sorted$ease)
    max_run <- max(rle_result$lengths)
    long_runs <- sum(rle_result$lengths >= 10)
    issues$pattern_clicking <- list(
      longest_same_answer_run = max_run,
      runs_of_10_plus = long_runs,
      severity = if (max_run >= 20 || long_runs >= 5) "high" else if (max_run >= 10) "medium" else "low"
    )
    if (max_run >= 20) quality_penalties <- quality_penalties + 20
    else if (max_run >= 10) quality_penalties <- quality_penalties + 10
  }
  
  # 4. "Again" followed immediately by "Easy"
  if (nrow(recent_sorted) >= 2) {
    again_then_easy <- sum(
      recent_sorted$ease[-nrow(recent_sorted)] == 1 & 
      recent_sorted$ease[-1] == 4 &
      recent_sorted$cid[-nrow(recent_sorted)] == recent_sorted$cid[-1],
      na.rm = TRUE
    )
    again_then_easy_pct <- again_then_easy / nrow(recent) * 100
    issues$again_then_easy <- list(
      count = again_then_easy,
      percentage = round(again_then_easy_pct, 2)
    )
    if (again_then_easy_pct > 5) quality_penalties <- quality_penalties + 10
  }
  
  # 5. Very long reviews
  very_long <- recent[recent$time > max_time_ms, ]
  very_long_pct <- nrow(very_long) / nrow(recent) * 100
  issues$distracted_reviews <- list(
    count = nrow(very_long),
    percentage = round(very_long_pct, 1),
    threshold_ms = max_time_ms
  )
  if (very_long_pct > 15) quality_penalties <- quality_penalties + 5
  
  # 6. Late night reviews
  recent$hour <- as.POSIXlt(recent$rid / 1000, origin = "1970-01-01")$hour
  late_night <- recent[recent$hour >= 23 | recent$hour < 5, ]
  late_night_pct <- nrow(late_night) / nrow(recent) * 100
  late_night_retention <- if (nrow(late_night) > 10) mean(late_night$ease > 1) else NA
  normal_retention <- mean(recent[recent$hour >= 5 & recent$hour < 23, ]$ease > 1, na.rm = TRUE)
  issues$late_night <- list(
    count = nrow(late_night),
    percentage = round(late_night_pct, 1),
    retention = if (!is.na(late_night_retention)) round(late_night_retention * 100, 1) else NA,
    normal_retention = round(normal_retention * 100, 1)
  )
  
  # 7. Review bunching
  daily_counts <- table(recent$review_date)
  daily_mean <- mean(daily_counts)
  daily_max <- max(daily_counts)
  bunching_ratio <- daily_max / daily_mean
  issues$review_bunching <- list(
    max_daily = as.integer(daily_max),
    mean_daily = round(daily_mean, 1),
    bunching_ratio = round(bunching_ratio, 1),
    severity = if (bunching_ratio > 5) "high" else if (bunching_ratio > 3) "medium" else "low"
  )
  if (bunching_ratio > 5) quality_penalties <- quality_penalties + 10
  
  # Calculate final score
  quality_score <- max(0, 100 - quality_penalties)
  
  grade <- if (quality_score >= 90) "A" else if (quality_score >= 80) "B" else 
           if (quality_score >= 70) "C" else if (quality_score >= 60) "D" else "F"
  
  # Recommendations
  recommendations <- character()
  if (issues$rushed_reviews$severity != "low") {
    recommendations <- c(recommendations, "Slow down - aim for at least 3-5 seconds per card")
  }
  if (issues$timing_consistency$status == "suspicious") {
    recommendations <- c(recommendations, "Review timing is very consistent - engage more with each card")
  }
  if (!is.null(issues$pattern_clicking) && issues$pattern_clicking$severity != "low") {
    recommendations <- c(recommendations, "Detected potential pattern clicking - vary your responses")
  }
  if (issues$review_bunching$severity != "low") {
    recommendations <- c(recommendations, "Try to spread reviews throughout the week")
  }
  if (length(recommendations) == 0) {
    recommendations <- "Your review quality looks good!"
  }
  
  list(
    score = quality_score,
    grade = grade,
    period_days = days,
    total_reviews = nrow(recent),
    issues = issues,
    recommendations = recommendations
  )
}
