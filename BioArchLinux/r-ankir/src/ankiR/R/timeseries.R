#' Analyze interval progression over time
#'
#' Tracks how card intervals change over time periods.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Aggregation period: "day", "week", or "month"
#' @return A tibble with interval statistics over time
#' @export
#' @examples
#' \dontrun{
#' ts <- anki_ts_intervals()
#' plot(ts$date, ts$median_ivl, type = "l")
#' }
anki_ts_intervals <- function(path = NULL, profile = NULL, by = "week") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble())
  }
 
  # Aggregate by period
  revlog$period <- switch(by,
    "day" = revlog$review_date,
    "week" = as.Date(cut(revlog$review_date, "week")),
    "month" = as.Date(cut(revlog$review_date, "month")),
    revlog$review_date
  )
 
  # Only include reviews with positive intervals (review cards)
  revlog_ivl <- revlog[revlog$ivl > 0, ]
 
  stats <- lapply(split(revlog_ivl, revlog_ivl$period), function(d) {
    tibble::tibble(
      reviews = nrow(d),
      mean_ivl = mean(d$ivl, na.rm = TRUE),
      median_ivl = median(d$ivl, na.rm = TRUE),
      sd_ivl = sd(d$ivl, na.rm = TRUE),
      max_ivl = max(d$ivl, na.rm = TRUE),
      pct_mature = mean(d$ivl >= 21, na.rm = TRUE) * 100
    )
  })
 
  result <- do.call(rbind, stats)
  result$date <- as.Date(names(stats))
  result <- result[order(result$date), ]
 
  # Add rolling averages
  if (nrow(result) >= 4) {
    result$mean_ivl_ma <- zoo_rollmean(result$mean_ivl, 4)
    result$median_ivl_ma <- zoo_rollmean(result$median_ivl, 4)
  }
 
  tibble::as_tibble(result[, c("date", "reviews", "mean_ivl", "median_ivl",
                               "sd_ivl", "max_ivl", "pct_mature",
                               if ("mean_ivl_ma" %in% names(result)) c("mean_ivl_ma", "median_ivl_ma"))])
}

#' Analyze retention over time
#'
#' Tracks retention rate changes over time periods.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Aggregation period: "day", "week", or "month"
#' @return A tibble with retention statistics over time
#' @export
#' @examples
#' \dontrun{
#' ts <- anki_ts_retention()
#' }
anki_ts_retention <- function(path = NULL, profile = NULL, by = "week") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble())
  }
 
  revlog$period <- switch(by,
    "day" = revlog$review_date,
    "week" = as.Date(cut(revlog$review_date, "week")),
    "month" = as.Date(cut(revlog$review_date, "month")),
    revlog$review_date
  )
 
  stats <- lapply(split(revlog, revlog$period), function(d) {
    tibble::tibble(
      reviews = nrow(d),
      retention = (1 - sum(d$ease == 1) / nrow(d)) * 100,
      again_count = sum(d$ease == 1),
      hard_count = sum(d$ease == 2),
      good_count = sum(d$ease == 3),
      easy_count = sum(d$ease == 4),
      avg_ease = mean(d$ease, na.rm = TRUE)
    )
  })
 
  result <- do.call(rbind, stats)
  result$date <- as.Date(names(stats))
  result <- result[order(result$date), ]
 
  # Add rolling average
  if (nrow(result) >= 4) {
    result$retention_ma <- zoo_rollmean(result$retention, 4)
  }
 
  tibble::as_tibble(result)
}

#' Analyze FSRS stability over time
#'
#' Tracks how memory stability evolves.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Aggregation period: "day", "week", or "month"
#' @return A tibble with stability statistics over time
#' @export
#' @examples
#' \dontrun{
#' ts <- anki_ts_stability()
#' }
anki_ts_stability <- function(path = NULL, profile = NULL, by = "week") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  cards_fsrs <- anki_cards_fsrs(path = col$path)
  revlog <- col$revlog()
 
  # Join FSRS data
  cards <- merge(cards, cards_fsrs[, c("cid", "stability", "difficulty")],
                 by = "cid", all.x = TRUE)
 
  # Get the last review date for each card to establish when stability was achieved
  last_review <- aggregate(review_date ~ cid, data = revlog, FUN = max)
  cards <- merge(cards, last_review, by = "cid", all.x = TRUE)
 
  cards <- cards[!is.na(cards$stability) & !is.na(cards$review_date), ]
 
  if (nrow(cards) == 0) {
    message("No FSRS stability data")
    return(tibble::tibble())
  }
 
  cards$period <- switch(by,
    "day" = cards$review_date,
    "week" = as.Date(cut(cards$review_date, "week")),
    "month" = as.Date(cut(cards$review_date, "month")),
    cards$review_date
  )
 
  stats <- lapply(split(cards, cards$period), function(d) {
    tibble::tibble(
      cards = nrow(d),
      mean_stability = mean(d$stability, na.rm = TRUE),
      median_stability = median(d$stability, na.rm = TRUE),
      mean_difficulty = mean(d$difficulty, na.rm = TRUE),
      pct_high_stability = mean(d$stability >= 30, na.rm = TRUE) * 100
    )
  })
 
  result <- do.call(rbind, stats)
  result$date <- as.Date(names(stats))
  result <- result[order(result$date), ]
 
  tibble::as_tibble(result)
}

#' Analyze workload trends
#'
#' Tracks review workload over time.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Aggregation period: "day", "week", or "month"
#' @return A tibble with workload statistics over time
#' @export
#' @examples
#' \dontrun{
#' ts <- anki_ts_workload()
#' }
anki_ts_workload <- function(path = NULL, profile = NULL, by = "week") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble())
  }
 
  revlog$period <- switch(by,
    "day" = revlog$review_date,
    "week" = as.Date(cut(revlog$review_date, "week")),
    "month" = as.Date(cut(revlog$review_date, "month")),
    revlog$review_date
  )
 
  stats <- lapply(split(revlog, revlog$period), function(d) {
    tibble::tibble(
      reviews = nrow(d),
      unique_cards = length(unique(d$cid)),
      time_minutes = sum(d$time, na.rm = TRUE) / 60000,
      avg_time_per_card = mean(d$time, na.rm = TRUE) / 1000,
      days_studied = length(unique(d$review_date))
    )
  })
 
  result <- do.call(rbind, stats)
  result$date <- as.Date(names(stats))
  result <- result[order(result$date), ]
 
  # Calculate reviews per day in period
  result$reviews_per_day <- result$reviews / result$days_studied
 
  # Add rolling average
  if (nrow(result) >= 4) {
    result$reviews_ma <- zoo_rollmean(result$reviews, 4)
  }
 
  tibble::as_tibble(result)
}

#' Analyze learning rate (new cards learned)
#'
#' Tracks how many new cards are introduced and learned over time.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Aggregation period: "day", "week", or "month"
#' @return A tibble with learning statistics over time
#' @export
#' @examples
#' \dontrun{
#' ts <- anki_ts_learning()
#' }
anki_ts_learning <- function(path = NULL, profile = NULL, by = "week") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
  cards <- col$cards()
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble())
  }
 
  # Find first review (introduction) date for each card
  first_review <- aggregate(review_date ~ cid, data = revlog, FUN = min)
  names(first_review) <- c("cid", "introduced_date")
 
  first_review$period <- switch(by,
    "day" = first_review$introduced_date,
    "week" = as.Date(cut(first_review$introduced_date, "week")),
    "month" = as.Date(cut(first_review$introduced_date, "month")),
    first_review$introduced_date
  )
 
  # Count new cards per period
  new_per_period <- as.data.frame(table(first_review$period), stringsAsFactors = FALSE)
  names(new_per_period) <- c("date", "new_cards")
  new_per_period$date <- as.Date(new_per_period$date)
 
  # Also count cards that graduated (became review cards) per period
  # A card graduates when it gets a positive interval
  revlog_graduated <- revlog[revlog$ivl > 0 & revlog$lastIvl <= 0, ]
 
  if (nrow(revlog_graduated) > 0) {
    revlog_graduated$period <- switch(by,
      "day" = revlog_graduated$review_date,
      "week" = as.Date(cut(revlog_graduated$review_date, "week")),
      "month" = as.Date(cut(revlog_graduated$review_date, "month")),
      revlog_graduated$review_date
    )
   
    graduated_per_period <- as.data.frame(table(revlog_graduated$period), stringsAsFactors = FALSE)
    names(graduated_per_period) <- c("date", "graduated_cards")
    graduated_per_period$date <- as.Date(graduated_per_period$date)
   
    result <- merge(new_per_period, graduated_per_period, by = "date", all = TRUE)
  } else {
    result <- new_per_period
    result$graduated_cards <- 0
  }
 
  result[is.na(result)] <- 0
  result <- result[order(result$date), ]
 
  # Cumulative totals
  result$cumulative_new <- cumsum(result$new_cards)
  result$cumulative_graduated <- cumsum(result$graduated_cards)
 
  tibble::as_tibble(result)
}

#' Analyze card maturation over time
#'
#' Tracks cumulative mature cards over time.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Aggregation period: "day", "week", or "month"
#' @param mature_threshold Days to consider a card mature (default 21)
#' @return A tibble with maturation statistics
#' @export
#' @examples
#' \dontrun{
#' ts <- anki_ts_maturation()
#' }
anki_ts_maturation <- function(path = NULL, profile = NULL, by = "week",
                               mature_threshold = 21) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (nrow(revlog) == 0) {
    return(tibble::tibble())
  }
 
  # Find when each card first became mature
  revlog_mature <- revlog[revlog$ivl >= mature_threshold & revlog$lastIvl < mature_threshold, ]
 
  if (nrow(revlog_mature) == 0) {
    message("No cards have matured yet")
    return(tibble::tibble())
  }
 
  # First maturation date for each card
  first_mature <- aggregate(review_date ~ cid, data = revlog_mature, FUN = min)
  names(first_mature) <- c("cid", "mature_date")
 
  first_mature$period <- switch(by,
    "day" = first_mature$mature_date,
    "week" = as.Date(cut(first_mature$mature_date, "week")),
    "month" = as.Date(cut(first_mature$mature_date, "month")),
    first_mature$mature_date
  )
 
  # Count cards maturing per period
  mature_per_period <- as.data.frame(table(first_mature$period), stringsAsFactors = FALSE)
  names(mature_per_period) <- c("date", "newly_mature")
  mature_per_period$date <- as.Date(mature_per_period$date)
  mature_per_period <- mature_per_period[order(mature_per_period$date), ]
 
  # Cumulative
  mature_per_period$cumulative_mature <- cumsum(mature_per_period$newly_mature)
 
  # Get total cards for percentage
  total_cards <- length(unique(revlog$cid))
  mature_per_period$pct_mature <- mature_per_period$cumulative_mature / total_cards * 100
 
  tibble::as_tibble(mature_per_period)
}

#' Decompose time series into trend, seasonal, and residual
#'
#' Uses classical decomposition on review data.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param metric Metric to decompose: "reviews", "retention", or "time"
#' @param frequency Seasonal frequency (default 7 for weekly pattern)
#' @return A decomposition object (or list with components)
#' @export
#' @examples
#' \dontrun{
#' dec <- anki_ts_decompose()
#' plot(dec)
#' }
anki_ts_decompose <- function(path = NULL, profile = NULL,
                              metric = "reviews", frequency = 7) {
  daily <- anki_stats_daily(path, profile)
 
  if (nrow(daily) < frequency * 2) {
    message("Not enough data for decomposition (need at least ", frequency * 2, " days)")
    return(NULL)
  }
 
  # Create time series
  values <- switch(metric,
    "reviews" = daily$reviews,
    "retention" = daily$retention,
    "time" = daily$time_minutes,
    daily$reviews
  )
 
  ts_data <- stats::ts(values, frequency = frequency)
 
  # Decompose
  decomp <- stats::decompose(ts_data, type = "additive")
 
  # Return with dates for plotting
  result <- list(
    decomposition = decomp,
    dates = daily$date,
    metric = metric,
    data = tibble::tibble(
      date = daily$date,
      observed = as.numeric(decomp$x),
      trend = as.numeric(decomp$trend),
      seasonal = as.numeric(decomp$seasonal),
      residual = as.numeric(decomp$random)
    )
  )
 
  class(result) <- c("anki_decomposition", class(result))
  result
}

#' Plot time series decomposition
#'
#' @param x An anki_decomposition object
#' @param ... Additional arguments (unused)
#' @return A ggplot object (if ggplot2 available) or base R plots
#' @export
plot.anki_decomposition <- function(x, ...) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    plot(x$decomposition)
    return(invisible(NULL))
  }
 
  d <- x$data
 
  # Reshape for faceted plot
  d_long <- data.frame(
    date = rep(d$date, 4),
    component = rep(c("Observed", "Trend", "Seasonal", "Residual"), each = nrow(d)),
    value = c(d$observed, d$trend, d$seasonal, d$residual)
  )
  d_long$component <- factor(d_long$component,
                             levels = c("Observed", "Trend", "Seasonal", "Residual"))
 
  ggplot2::ggplot(d_long, ggplot2::aes(x = date, y = value)) +
    ggplot2::geom_line(color = "#3498db") +
    ggplot2::facet_wrap(~component, scales = "free_y", ncol = 1) +
    ggplot2::labs(
      title = paste("Time Series Decomposition:", x$metric),
      x = NULL, y = NULL
    ) +
    ggplot2::theme_minimal()
}

#' Detect anomalies in review patterns
#'
#' Identifies unusual days (very high or low activity).
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param threshold Number of standard deviations for anomaly (default 2)
#' @return A tibble with anomalous days
#' @export
#' @examples
#' \dontrun{
#' anomalies <- anki_ts_anomalies()
#' }
anki_ts_anomalies <- function(path = NULL, profile = NULL, threshold = 2) {
  daily <- anki_stats_daily(path, profile)
 
  if (nrow(daily) < 14) {
    message("Not enough data for anomaly detection")
    return(tibble::tibble())
  }
 
  # Calculate z-scores
  daily$reviews_z <- (daily$reviews - mean(daily$reviews)) / sd(daily$reviews)
  daily$time_z <- (daily$time_minutes - mean(daily$time_minutes)) / sd(daily$time_minutes)
 
  # Identify anomalies
  daily$anomaly_type <- NA_character_
  daily$anomaly_type[daily$reviews_z > threshold] <- "high_reviews"
  daily$anomaly_type[daily$reviews_z < -threshold] <- "low_reviews"
  daily$anomaly_type[daily$time_z > threshold & is.na(daily$anomaly_type)] <- "high_time"
 
  anomalies <- daily[!is.na(daily$anomaly_type), ]
 
  if (nrow(anomalies) == 0) {
    message("No anomalies detected at threshold ", threshold)
    return(tibble::tibble())
  }
 
  tibble::as_tibble(anomalies[, c("date", "reviews", "time_minutes", "retention",
                                  "reviews_z", "anomaly_type")])
}

#' Forecast future reviews using simple methods
#'
#' Uses exponential smoothing or linear trend for forecasting.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to forecast
#' @param method Forecasting method: "ets" (exponential smoothing) or "linear"
#' @return A tibble with forecasted values
#' @export
#' @examples
#' \dontrun{
#' forecast <- anki_ts_forecast(days = 30)
#' }
anki_ts_forecast <- function(path = NULL, profile = NULL, days = 30,
                             method = "linear") {
  daily <- anki_stats_daily(path, profile, from = Sys.Date() - 90)
 
  if (nrow(daily) < 14) {
    message("Not enough data for forecasting")
    return(tibble::tibble())
  }
 
  future_dates <- seq(max(daily$date) + 1, by = "day", length.out = days)
 
  if (method == "ets") {
    # Simple exponential smoothing
    ts_data <- stats::ts(daily$reviews, frequency = 7)
    fit <- stats::HoltWinters(ts_data, gamma = FALSE)
    pred <- stats::predict(fit, n.ahead = days)
   
    result <- tibble::tibble(
      date = future_dates,
      forecast = as.numeric(pred),
      method = "exponential_smoothing"
    )
  } else {
    # Linear trend
    daily$day_num <- as.numeric(daily$date - min(daily$date))
    fit <- stats::lm(reviews ~ day_num, data = daily)
   
    future_day_num <- as.numeric(future_dates - min(daily$date))
    pred <- stats::predict(fit, newdata = data.frame(day_num = future_day_num),
                          interval = "prediction")
   
    result <- tibble::tibble(
      date = future_dates,
      forecast = pred[, "fit"],
      lower = pred[, "lwr"],
      upper = pred[, "upr"],
      method = "linear_trend"
    )
  }
 
  # Ensure non-negative
  result$forecast <- pmax(0, result$forecast)
  if ("lower" %in% names(result)) {
    result$lower <- pmax(0, result$lower)
  }
 
  tibble::as_tibble(result)
}

#' Calculate autocorrelation of review patterns
#'
#' Identifies cyclical patterns in studying.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param max_lag Maximum lag to compute (default 30)
#' @return A tibble with autocorrelation values
#' @export
#' @examples
#' \dontrun{
#' acf_data <- anki_ts_autocorrelation()
#' # Peaks at lag 7 indicate weekly patterns
#' }
anki_ts_autocorrelation <- function(path = NULL, profile = NULL, max_lag = 30) {
  daily <- anki_stats_daily(path, profile)
 
  if (nrow(daily) < max_lag + 10) {
    message("Not enough data for autocorrelation analysis")
    return(tibble::tibble())
  }
 
  acf_result <- stats::acf(daily$reviews, lag.max = max_lag, plot = FALSE)
 
  result <- tibble::tibble(
    lag = 0:max_lag,
    acf = as.numeric(acf_result$acf),
    significant = abs(as.numeric(acf_result$acf)) > 2 / sqrt(nrow(daily))
  )
 
  # Identify weekly pattern
  if (result$acf[8] > 0.2) {  # lag 7 (index 8 due to lag 0)
    attr(result, "weekly_pattern") <- TRUE
    message("Weekly pattern detected (autocorrelation at lag 7: ",
            round(result$acf[8], 3), ")")
  }
 
  tibble::as_tibble(result)
}

#' Plot time series with trend line
#'
#' @param ts_data A tibble from any anki_ts_* function
#' @param y_col Column name to plot on y-axis
#' @param title Plot title
#' @return A ggplot object
#' @export
#' @examples
#' \dontrun{
#' ts <- anki_ts_intervals()
#' anki_ts_plot(ts, "median_ivl", "Median Interval Over Time")
#' }
anki_ts_plot <- function(ts_data, y_col, title = NULL) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required")
  }
 
  if (!"date" %in% names(ts_data)) {
    stop("ts_data must have a 'date' column")
  }
 
  if (!y_col %in% names(ts_data)) {
    stop("Column '", y_col, "' not found in data")
  }
 
  if (is.null(title)) {
    title <- paste(y_col, "Over Time")
  }
 
  p <- ggplot2::ggplot(ts_data, ggplot2::aes(x = date, y = .data[[y_col]])) +
    ggplot2::geom_line(alpha = 0.5, color = "gray50") +
    ggplot2::geom_smooth(method = "loess", color = "#3498db", se = TRUE,
                         formula = y ~ x) +
    ggplot2::labs(title = title, x = NULL, y = y_col) +
    ggplot2::theme_minimal()
 
  # Add moving average if present
  ma_col <- paste0(y_col, "_ma")
  if (ma_col %in% names(ts_data)) {
    p <- p + ggplot2::geom_line(ggplot2::aes(y = .data[[ma_col]]),
                                 color = "#e74c3c", linewidth = 1)
  }
 
  p
}

# Helper: rolling mean (already defined in plotting.R but duplicating for safety)
if (!exists("zoo_rollmean", mode = "function")) {
  zoo_rollmean <- function(x, k) {
    n <- length(x)
    if (n < k) return(rep(NA_real_, n))
    result <- rep(NA_real_, n)
    for (i in k:n) {
      result[i] <- mean(x[(i - k + 1):i], na.rm = TRUE)
    }
    result
  }
}
