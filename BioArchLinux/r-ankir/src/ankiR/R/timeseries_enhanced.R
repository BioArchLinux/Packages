#' Enhanced Time Series Forecasting
#'
#' Improved forecasting with ARIMA, seasonal patterns, and workload ceilings.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param metric Metric to forecast: "reviews", "time", "retention", "cards_learned"
#' @param days_ahead Number of days to forecast (default 30)
#' @param method Forecasting method: "auto", "arima", "ets", "holt", "seasonal"
#' @param confidence Confidence level for prediction intervals (default 0.95)
#' @param workload_ceiling Maximum daily workload (NULL for none)
#' @return A tibble with forecast results
#' @export
#'
#' @examples
#' \dontrun{
#' fc <- anki_forecast_enhanced("reviews", days_ahead = 30)
#' plot(fc$date, fc$forecast, type = "l")
#' }
anki_forecast_enhanced <- function(path = NULL, profile = NULL,
                                   metric = "reviews",
                                   days_ahead = 30,
                                   method = "auto",
                                   confidence = 0.95,
                                   workload_ceiling = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  # Get historical data
  daily <- anki_stats_daily(path = col$path)
  
  if (nrow(daily) < 14) {
    stop("Need at least 14 days of data for forecasting")
  }
  
  # Select metric
  y <- switch(metric,
    "reviews" = daily$reviews,
    "time" = daily$time_minutes,
    "retention" = daily$retention,
    "cards_learned" = daily$learned,
    daily$reviews
  )
  
  dates <- daily$date
  
  # Remove missing values
  valid <- !is.na(y)
  y <- y[valid]
  dates <- dates[valid]
  
  if (length(y) < 14) {
    stop("Insufficient non-NA data points")
  }
  
  # Detect weekly seasonality
  has_weekly_pattern <- detect_weekly_pattern(y)
  
  # Choose method
  if (method == "auto") {
    method <- if (has_weekly_pattern && length(y) >= 21) "seasonal" else "holt"
  }
  
  # Fit model and forecast
  forecast_result <- switch(method,
    "arima" = forecast_arima(y, days_ahead, confidence),
    "ets" = forecast_ets(y, days_ahead, confidence),
    "holt" = forecast_holt(y, days_ahead, confidence),
    "seasonal" = forecast_seasonal(y, days_ahead, confidence),
    forecast_holt(y, days_ahead, confidence)
  )
  
  # Create future dates
  last_date <- max(dates)
  future_dates <- seq(last_date + 1, by = "day", length.out = days_ahead)
  
  # Apply workload ceiling
  if (!is.null(workload_ceiling)) {
    forecast_result$forecast <- pmin(forecast_result$forecast, workload_ceiling)
    forecast_result$upper <- pmin(forecast_result$upper, workload_ceiling)
  }
  
  # Combine historical and forecast
  historical <- tibble::tibble(
    date = dates,
    actual = y,
    forecast = NA_real_,
    lower = NA_real_,
    upper = NA_real_,
    type = "historical"
  )
  
  forecast_df <- tibble::tibble(
    date = future_dates,
    actual = NA_real_,
    forecast = forecast_result$forecast,
    lower = forecast_result$lower,
    upper = forecast_result$upper,
    type = "forecast"
  )
  
  result <- rbind(historical, forecast_df)
  
  # Add metadata
  attr(result, "method") <- method
  attr(result, "metric") <- metric
  attr(result, "confidence") <- confidence
  attr(result, "has_weekly_pattern") <- has_weekly_pattern
  
  # Add summary statistics
  attr(result, "summary") <- list(
    historical_mean = mean(y, na.rm = TRUE),
    historical_sd = sd(y, na.rm = TRUE),
    forecast_mean = mean(forecast_result$forecast),
    forecast_trend = forecast_result$forecast[days_ahead] - forecast_result$forecast[1],
    total_forecast = sum(forecast_result$forecast)
  )
  
  result
}

#' Detect weekly pattern in data
#' @noRd
detect_weekly_pattern <- function(y) {
  if (length(y) < 21) return(FALSE)
  
  # Simple test: check if variance by weekday differs
  n <- length(y)
  weekdays <- rep(1:7, length.out = n)
  
  by_weekday <- split(y, weekdays)
  means <- sapply(by_weekday, mean, na.rm = TRUE)
  
  # If weekday means vary by more than 20% from overall mean, there's a pattern
  overall_mean <- mean(y, na.rm = TRUE)
  max_deviation <- max(abs(means - overall_mean)) / overall_mean
  
  max_deviation > 0.2
}

#' ARIMA forecasting
#' @noRd
forecast_arima <- function(y, h, confidence) {
  # Simple ARIMA(1,1,1) implementation
  n <- length(y)
  
  # Difference the series
  dy <- diff(y)
  
  # Estimate AR(1) coefficient
  phi <- cor(dy[-length(dy)], dy[-1])
  if (is.na(phi)) phi <- 0
  phi <- max(-0.9, min(0.9, phi))
  
  # Estimate MA(1) coefficient (simplified)
  theta <- 0.3
  
  # Last values
  last_y <- y[n]
  last_dy <- dy[n - 1]
  
  # Forecast
  forecast <- numeric(h)
  sigma <- sd(dy, na.rm = TRUE)
  
  for (i in 1:h) {
    if (i == 1) {
      forecast[i] <- last_y + phi * last_dy
    } else {
      forecast[i] <- forecast[i - 1] + phi * (forecast[i - 1] - if (i > 2) forecast[i - 2] else last_y)
    }
  }
  
  # Prediction intervals widen with horizon
  z <- qnorm((1 + confidence) / 2)
  se <- sigma * sqrt(1:h)
  
  list(
    forecast = forecast,
    lower = forecast - z * se,
    upper = forecast + z * se
  )
}

#' Exponential smoothing forecasting
#' @noRd
forecast_ets <- function(y, h, confidence) {
  n <- length(y)
  
  # Estimate level smoothing parameter
  alpha <- 0.3
  
  # Initialize
  level <- y[1]
  
  # Fit
  for (i in 2:n) {
    level <- alpha * y[i] + (1 - alpha) * level
  }
  
  # Forecast (flat)
  forecast <- rep(level, h)
  
  # Variance estimation
  residuals <- numeric(n - 1)
  fitted_level <- y[1]
  for (i in 2:n) {
    residuals[i - 1] <- y[i] - fitted_level
    fitted_level <- alpha * y[i] + (1 - alpha) * fitted_level
  }
  
  sigma <- sd(residuals, na.rm = TRUE)
  z <- qnorm((1 + confidence) / 2)
  se <- sigma * sqrt(cumsum(rep(alpha^2, h)) + (1:h) * (1 - alpha)^2)
  
  list(
    forecast = forecast,
    lower = forecast - z * se,
    upper = forecast + z * se
  )
}

#' Holt's linear trend forecasting
#' @noRd
forecast_holt <- function(y, h, confidence) {
  n <- length(y)
  
  # Parameters
  alpha <- 0.3
  beta <- 0.1
  
  # Initialize
  level <- y[1]
  trend <- mean(diff(y[1:min(7, n)]))
  
  # Fit
  for (i in 2:n) {
    old_level <- level
    level <- alpha * y[i] + (1 - alpha) * (level + trend)
    trend <- beta * (level - old_level) + (1 - beta) * trend
  }
  
  # Forecast
  forecast <- level + (1:h) * trend
  
  # Variance
  residuals <- numeric(n - 1)
  fit_level <- y[1]
  fit_trend <- trend
  for (i in 2:n) {
    residuals[i - 1] <- y[i] - (fit_level + fit_trend)
    old_level <- fit_level
    fit_level <- alpha * y[i] + (1 - alpha) * (fit_level + fit_trend)
    fit_trend <- beta * (fit_level - old_level) + (1 - beta) * fit_trend
  }
  
  sigma <- sd(residuals, na.rm = TRUE)
  z <- qnorm((1 + confidence) / 2)
  se <- sigma * sqrt(1:h)
  
  list(
    forecast = pmax(0, forecast),
    lower = pmax(0, forecast - z * se),
    upper = forecast + z * se
  )
}

#' Seasonal forecasting (weekly pattern)
#' @noRd
forecast_seasonal <- function(y, h, confidence) {
  n <- length(y)
  
  # Calculate weekly seasonal factors
  weekdays <- rep(1:7, length.out = n)
  by_weekday <- split(y, weekdays)
  seasonal <- sapply(by_weekday, mean, na.rm = TRUE)
  overall_mean <- mean(y, na.rm = TRUE)
  seasonal_factors <- seasonal / overall_mean
  
  # Deseasonalize
  y_deseasonalized <- y / seasonal_factors[weekdays]
  
  # Fit Holt on deseasonalized
  alpha <- 0.3
  beta <- 0.05
  
  level <- y_deseasonalized[1]
  trend <- mean(diff(y_deseasonalized[1:min(7, n)]))
  
  for (i in 2:n) {
    old_level <- level
    level <- alpha * y_deseasonalized[i] + (1 - alpha) * (level + trend)
    trend <- beta * (level - old_level) + (1 - beta) * trend
  }
  
  # Forecast
  forecast_base <- level + (1:h) * trend
  
  # Apply seasonal factors
  last_weekday <- weekdays[n]
  future_weekdays <- ((last_weekday + 1:h - 1) %% 7) + 1
  forecast <- forecast_base * seasonal_factors[future_weekdays]
  
  # Variance
  sigma <- sd(y - fitted_seasonal(y, seasonal_factors, alpha, beta), na.rm = TRUE)
  z <- qnorm((1 + confidence) / 2)
  se <- sigma * sqrt(1:h)
  
  list(
    forecast = pmax(0, forecast),
    lower = pmax(0, forecast - z * se),
    upper = forecast + z * se
  )
}

#' Helper for seasonal fitting
#' @noRd
fitted_seasonal <- function(y, factors, alpha, beta) {
  n <- length(y)
  weekdays <- rep(1:7, length.out = n)
  y_ds <- y / factors[weekdays]
  
  level <- y_ds[1]
  trend <- 0
  fitted <- numeric(n)
  fitted[1] <- level * factors[1]
  
  for (i in 2:n) {
    old_level <- level
    level <- alpha * y_ds[i] + (1 - alpha) * (level + trend)
    trend <- beta * (level - old_level) + (1 - beta) * trend
    fitted[i] <- (level + trend) * factors[weekdays[i]]
  }
  
  fitted
}

#' Workload Projection (Rough Estimate)
#'
#' Project future review workload with scenario analysis. This provides rough
#' ballpark estimates for planning purposes, not accurate FSRS simulation.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to project (default 30)
#' @param scenarios List of scenario parameters
#' @return A tibble with workload projections for each scenario
#' @export
#'
#' @details
#' This function uses simplified heuristics to estimate workload:
#' - New cards reviewed ~4x during learning phase
#' - Mature card reviews based on recent average
#' - Retention affects review frequency
#'
#' For accurate FSRS simulation, use Anki's built-in simulator (Tools > FSRS >
#' Simulate) or the FSRS Helper add-on, which implement the full FSRS algorithm.
#'
#' This function is useful for:
#' - Quick "what if" scenario comparisons
#' - Rough planning estimates
#' - Programmatic analysis in R
#'
#' @examples
#' \dontrun{
#' proj <- anki_workload_projection(days = 30)
#' }
anki_workload_projection <- function(path = NULL, profile = NULL,
                                     days = 30,
                                     scenarios = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  # Default scenarios
  if (is.null(scenarios)) {
    scenarios <- list(
      current = list(new_cards = 20, retention = 0.9, name = "Current Settings"),
      reduced = list(new_cards = 10, retention = 0.9, name = "Reduced New Cards"),
      intensive = list(new_cards = 40, retention = 0.9, name = "Intensive"),
      maintenance = list(new_cards = 0, retention = 0.9, name = "Maintenance Only")
    )
  }
  
  # Get baseline data
  cards <- col$cards()
  revlog <- col$revlog()
  
  # Calculate current workload patterns
  daily <- anki_stats_daily(path = col$path)
  avg_reviews <- mean(tail(daily$reviews, 30), na.rm = TRUE)
  
  # Current due load
  today_num <- as.numeric(Sys.Date() - as.Date("1970-01-01"))
  current_due <- sum(cards$queue == 2 & cards$due <= today_num)
  
  # New cards remaining
  new_remaining <- sum(cards$queue == 0)
  
  # Project for each scenario
  results <- lapply(names(scenarios), function(scenario_name) {
    sc <- scenarios[[scenario_name]]
    
    # Simple projection model
    daily_base <- current_due / 7  # spread current backlog
    daily_new_reviews <- sc$new_cards * 4  # new cards reviewed ~4x in learning
    daily_mature_reviews <- avg_reviews * 0.7  # mature cards
    
    daily_total <- daily_base + daily_new_reviews + daily_mature_reviews
    
    # Adjust for retention (lower retention = more reviews)
    retention_factor <- (1 - sc$retention) / 0.1 + 1
    daily_total <- daily_total * retention_factor
    
    # Time estimate (avg 10 seconds per review)
    daily_time <- daily_total * 10 / 60
    
    tibble::tibble(
      day = 1:days,
      date = Sys.Date() + 1:days,
      scenario = sc$name,
      reviews = round(daily_total),
      time_minutes = round(daily_time, 1),
      cumulative_reviews = cumsum(round(daily_total)),
      new_cards_added = cumsum(rep(sc$new_cards, days))
    )
  })
  
  do.call(rbind, results)
}

#' Retention Stability Analysis
#'
#' Analyze how stable your retention is over time.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param window Rolling window size in days (default 7)
#' @return A tibble with retention stability metrics
#' @export
anki_retention_stability <- function(path = NULL, profile = NULL, window = 7) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  revlog <- col$revlog()
  
  # Filter to review type only (if column exists)
  if ("review_type" %in% names(revlog)) {
    reviews <- revlog[revlog$review_type == 1, ]
  } else {
    # Use all reviews if type not available
    reviews <- revlog
  }
  
  if (nrow(reviews) < window * 2) {
    message("Insufficient review data")
    return(tibble::tibble())
  }
  
  # Calculate daily retention
  daily <- aggregate(
    cbind(correct = ease > 1, total = rep(1, nrow(reviews))) ~ review_date,
    data = reviews,
    FUN = sum
  )
  daily$retention <- daily$correct / daily$total
  daily <- daily[order(daily$review_date), ]
  
  # Rolling statistics
  n <- nrow(daily)
  if (n < window) {
    message("Insufficient days of data")
    return(tibble::as_tibble(daily))
  }
  
  rolling_mean <- numeric(n)
  rolling_sd <- numeric(n)
  rolling_min <- numeric(n)
  rolling_max <- numeric(n)
  
  for (i in window:n) {
    window_data <- daily$retention[(i - window + 1):i]
    rolling_mean[i] <- mean(window_data, na.rm = TRUE)
    rolling_sd[i] <- sd(window_data, na.rm = TRUE)
    rolling_min[i] <- min(window_data, na.rm = TRUE)
    rolling_max[i] <- max(window_data, na.rm = TRUE)
  }
  
  daily$rolling_mean <- rolling_mean
  daily$rolling_sd <- rolling_sd
  daily$rolling_min <- rolling_min
  daily$rolling_max <- rolling_max
  daily$rolling_range <- rolling_max - rolling_min
  
  # Stability score (lower SD = more stable)
  daily$stability_score <- 1 - pmin(1, rolling_sd * 5)
  
  # Trend detection
  if (n >= 30) {
    recent <- tail(daily$retention, 14)
    earlier <- daily$retention[(n - 27):(n - 14)]
    trend_change <- mean(recent, na.rm = TRUE) - mean(earlier, na.rm = TRUE)
    attr(daily, "trend") <- list(
      recent_mean = mean(recent, na.rm = TRUE),
      earlier_mean = mean(earlier, na.rm = TRUE),
      change = trend_change,
      direction = if (trend_change > 0.02) "improving" else if (trend_change < -0.02) "declining" else "stable"
    )
  }
  
  tibble::as_tibble(daily)
}

#' Monte Carlo Forecasting
#'
#' Forecast future reviews using Monte Carlo simulation with bootstrapping.
#' Unlike statistical methods (ARIMA, Holt-Winters), this approach:
#' - Makes no distributional assumptions
#' - Preserves day-of-week patterns naturally
#' - Handles irregular study habits (missed days, catch-up sessions)
#' - Provides empirical confidence intervals
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days_ahead Number of days to forecast (default 30)
#' @param n_sim Number of simulations (default 1000)
#' @param method Bootstrap method: "weekday" (preserves day-of-week), "block" (preserves sequences), "simple" (iid sampling)
#' @param block_size Block size for block bootstrap (default 7)
#' @param include_trend Whether to include trend component (default TRUE)
#' @param seed Random seed for reproducibility (NULL for random)
#' @return A list with forecast distribution, summary statistics, and simulation data
#' @export
#'
#' @examples
#' \dontrun{
#' mc <- anki_forecast_monte_carlo(days_ahead = 30, n_sim = 1000)
#' 
#' # Summary
#' mc$summary
#' 
#' # Probability of >100 reviews on day 7
#' mc$prob_above(day = 7, threshold = 100)
#' 
#' # Full simulation matrix
#' dim(mc$simulations)  # n_sim x days_ahead
#' }
anki_forecast_monte_carlo <- function(path = NULL, profile = NULL,
                                      days_ahead = 30,
                                      n_sim = 1000,
                                      method = "weekday",
                                      block_size = 7,
                                      include_trend = TRUE,
                                      seed = NULL) {
  if (!is.null(seed)) set.seed(seed)
  
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  # Get historical data
  daily <- anki_stats_daily(path = col$path)
  
  if (nrow(daily) < 14) {
    stop("Need at least 14 days of data for Monte Carlo forecasting")
  }
  
  y <- daily$reviews
  dates <- daily$date
  n <- length(y)
  
  # Calculate day-of-week patterns
  weekdays <- as.POSIXlt(dates)$wday  # 0=Sunday, 6=Saturday
  
  # Group historical data by weekday
  weekday_data <- lapply(0:6, function(wd) {
    y[weekdays == wd]
  })
  
  # Calculate trend if requested
  trend_slope <- 0
  if (include_trend && n >= 30) {
    # Simple linear trend on recent 30 days
    recent_y <- tail(y, 30)
    recent_x <- seq_along(recent_y)
    fit <- stats::lm(recent_y ~ recent_x)
    trend_slope <- stats::coef(fit)[2]
    # Cap extreme trends
    trend_slope <- max(min(trend_slope, 2), -2)
  }
  
  # Future dates
  future_dates <- seq(max(dates) + 1, by = "day", length.out = days_ahead)
  future_weekdays <- as.POSIXlt(future_dates)$wday
  
  # Initialize simulation matrix
  sim_matrix <- matrix(NA_real_, nrow = n_sim, ncol = days_ahead)
  
  # Run simulations
  for (sim in seq_len(n_sim)) {
    if (method == "weekday") {
      # Sample from same weekday's historical distribution
      for (d in seq_len(days_ahead)) {
        wd <- future_weekdays[d]
        pool <- weekday_data[[wd + 1]]
        if (length(pool) > 0) {
          base_value <- sample(pool, 1)
        } else {
          base_value <- sample(y, 1)
        }
        # Add trend
        sim_matrix[sim, d] <- max(0, base_value + trend_slope * d)
      }
      
    } else if (method == "block") {
      # Block bootstrap - sample contiguous blocks
      blocks_needed <- ceiling(days_ahead / block_size)
      sampled <- numeric(0)
      
      for (b in seq_len(blocks_needed)) {
        start_idx <- sample(seq_len(n - block_size + 1), 1)
        block <- y[start_idx:(start_idx + block_size - 1)]
        sampled <- c(sampled, block)
      }
      
      sim_matrix[sim, ] <- pmax(0, sampled[seq_len(days_ahead)] + trend_slope * seq_len(days_ahead))
      
    } else {
      # Simple iid bootstrap
      sampled <- sample(y, days_ahead, replace = TRUE)
      sim_matrix[sim, ] <- pmax(0, sampled + trend_slope * seq_len(days_ahead))
    }
  }
  
  # Calculate summary statistics for each day
  summary_stats <- tibble::tibble(
    day = seq_len(days_ahead),
    date = future_dates,
    weekday = weekdays(future_dates),
    mean = colMeans(sim_matrix),
    median = apply(sim_matrix, 2, stats::median),
    sd = apply(sim_matrix, 2, stats::sd),
    ci_lower_50 = apply(sim_matrix, 2, stats::quantile, probs = 0.25),
    ci_upper_50 = apply(sim_matrix, 2, stats::quantile, probs = 0.75),
    ci_lower_80 = apply(sim_matrix, 2, stats::quantile, probs = 0.10),
    ci_upper_80 = apply(sim_matrix, 2, stats::quantile, probs = 0.90),
    ci_lower_95 = apply(sim_matrix, 2, stats::quantile, probs = 0.025),
    ci_upper_95 = apply(sim_matrix, 2, stats::quantile, probs = 0.975),
    min = apply(sim_matrix, 2, min),
    max = apply(sim_matrix, 2, max),
    prob_zero = colMeans(sim_matrix == 0),
    prob_above_100 = colMeans(sim_matrix > 100),
    prob_above_150 = colMeans(sim_matrix > 150)
  )
  
  # Cumulative statistics (total reviews over period)
  cumulative_sims <- t(apply(sim_matrix, 1, cumsum))
  
  cumulative_stats <- tibble::tibble(
    day = seq_len(days_ahead),
    date = future_dates,
    cumulative_mean = colMeans(cumulative_sims),
    cumulative_median = apply(cumulative_sims, 2, stats::median),
    cumulative_ci_lower_95 = apply(cumulative_sims, 2, stats::quantile, probs = 0.025),
    cumulative_ci_upper_95 = apply(cumulative_sims, 2, stats::quantile, probs = 0.975)
  )
  
  # Historical comparison stats
  historical_stats <- list(
    mean = mean(y),
    median = stats::median(y),
    sd = stats::sd(y),
    min = min(y),
    max = max(y),
    days_analyzed = n,
    zero_days = sum(y == 0),
    zero_rate = mean(y == 0)
  )
  
  # Helper function for probability queries
  prob_above <- function(day, threshold) {
    if (day < 1 || day > days_ahead) {
      stop("Day must be between 1 and ", days_ahead)
    }
    mean(sim_matrix[, day] > threshold)
  }
  
  prob_below <- function(day, threshold) {
    if (day < 1 || day > days_ahead) {
      stop("Day must be between 1 and ", days_ahead)
    }
    mean(sim_matrix[, day] < threshold)
  }
  
  prob_between <- function(day, lower, upper) {
    if (day < 1 || day > days_ahead) {
      stop("Day must be between 1 and ", days_ahead)
    }
    mean(sim_matrix[, day] >= lower & sim_matrix[, day] <= upper)
  }
  
  # Return results
  structure(
    list(
      summary = summary_stats,
      cumulative = cumulative_stats,
      simulations = sim_matrix,
      historical = historical_stats,
      prob_above = prob_above,
      prob_below = prob_below,
      prob_between = prob_between,
      parameters = list(
        days_ahead = days_ahead,
        n_sim = n_sim,
        method = method,
        block_size = block_size,
        include_trend = include_trend,
        trend_slope = trend_slope,
        seed = seed
      )
    ),
    class = c("anki_mc_forecast", "list")
  )
}

#' Plot Monte Carlo Forecast
#'
#' Visualize Monte Carlo forecast with confidence bands.
#'
#' @param mc_forecast Output from anki_forecast_monte_carlo()
#' @param show_bands Confidence bands to show: "95", "80", "50", or combinations
#' @param show_simulations Number of individual simulations to overlay (0 for none)
#' @param cumulative Plot cumulative reviews instead of daily
#' @return A ggplot2 object
#' @export
#'
#' @examples
#' \dontrun{
#' mc <- anki_forecast_monte_carlo(days_ahead = 30)
#' anki_plot_monte_carlo(mc)
#' anki_plot_monte_carlo(mc, cumulative = TRUE)
#' }
anki_plot_monte_carlo <- function(mc_forecast, 
                                   show_bands = c("95", "80"),
                                   show_simulations = 20,
                                   cumulative = FALSE) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required for plotting")
  }
  
  if (cumulative) {
    d <- mc_forecast$cumulative
    y_lab <- "Cumulative Reviews"
    title <- "Monte Carlo Forecast: Cumulative Reviews"
    
    p <- ggplot2::ggplot(d, ggplot2::aes(x = .data$date))
    
    if ("95" %in% show_bands) {
      p <- p + ggplot2::geom_ribbon(
        ggplot2::aes(ymin = .data$cumulative_ci_lower_95, 
                     ymax = .data$cumulative_ci_upper_95),
        fill = "#3498db", alpha = 0.2
      )
    }
    
    p <- p + 
      ggplot2::geom_line(ggplot2::aes(y = .data$cumulative_median), 
                         color = "#2c3e50", linewidth = 1.2) +
      ggplot2::geom_line(ggplot2::aes(y = .data$cumulative_mean), 
                         color = "#e74c3c", linetype = "dashed", linewidth = 0.8)
    
  } else {
    d <- mc_forecast$summary
    y_lab <- "Daily Reviews"
    title <- "Monte Carlo Forecast: Daily Reviews"
    
    p <- ggplot2::ggplot(d, ggplot2::aes(x = .data$date))
    
    # Add simulation traces first (behind everything)
    if (show_simulations > 0) {
      sim_subset <- mc_forecast$simulations[sample(nrow(mc_forecast$simulations), 
                                                    min(show_simulations, nrow(mc_forecast$simulations))), ]
      sim_df <- data.frame(
        date = rep(d$date, each = nrow(sim_subset)),
        value = as.vector(t(sim_subset)),
        sim = rep(seq_len(nrow(sim_subset)), times = ncol(sim_subset))
      )
      p <- p + ggplot2::geom_line(
        data = sim_df,
        ggplot2::aes(x = .data$date, y = .data$value, group = .data$sim),
        color = "#bdc3c7", alpha = 0.3, linewidth = 0.3
      )
    }
    
    # Add confidence bands
    if ("95" %in% show_bands) {
      p <- p + ggplot2::geom_ribbon(
        ggplot2::aes(ymin = .data$ci_lower_95, ymax = .data$ci_upper_95),
        fill = "#3498db", alpha = 0.15
      )
    }
    if ("80" %in% show_bands) {
      p <- p + ggplot2::geom_ribbon(
        ggplot2::aes(ymin = .data$ci_lower_80, ymax = .data$ci_upper_80),
        fill = "#3498db", alpha = 0.25
      )
    }
    if ("50" %in% show_bands) {
      p <- p + ggplot2::geom_ribbon(
        ggplot2::aes(ymin = .data$ci_lower_50, ymax = .data$ci_upper_50),
        fill = "#3498db", alpha = 0.35
      )
    }
    
    p <- p + 
      ggplot2::geom_line(ggplot2::aes(y = .data$median), 
                         color = "#2c3e50", linewidth = 1.2) +
      ggplot2::geom_line(ggplot2::aes(y = .data$mean), 
                         color = "#e74c3c", linetype = "dashed", linewidth = 0.8)
  }
  
  p + 
    ggplot2::labs(
      title = title,
      subtitle = paste0(mc_forecast$parameters$n_sim, " simulations, ",
                        mc_forecast$parameters$method, " bootstrap"),
      x = "Date",
      y = y_lab,
      caption = "Solid line: median, Dashed line: mean"
    ) +
    ggplot2::theme_minimal() +
    ggplot2::theme(
      plot.title = ggplot2::element_text(face = "bold"),
      legend.position = "bottom"
    )
}

#' Compare Forecast Methods
#'
#' Run multiple forecasting methods and compare their predictions.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days_ahead Number of days to forecast (default 30)
#' @param methods Methods to compare (default: all available)
#' @return A list with comparison results
#' @export
#'
#' @examples
#' \dontrun{
#' comp <- anki_compare_forecasts(days_ahead = 14)
#' comp$summary
#' }
anki_compare_forecasts <- function(path = NULL, profile = NULL,
                                   days_ahead = 30,
                                   methods = c("holt", "arima", "seasonal", "monte_carlo")) {
  results <- list()
  
  # Statistical methods
  stat_methods <- setdiff(methods, "monte_carlo")
  if (length(stat_methods) > 0) {
    for (m in stat_methods) {
      tryCatch({
        fc <- anki_forecast_enhanced(path, profile, 
                                     days_ahead = days_ahead, 
                                     method = m)
        # Filter to forecast rows only
        fc_only <- fc[fc$type == "forecast", ]
        results[[m]] <- list(
          method = m,
          forecast = fc_only$forecast,
          ci_lower = fc_only$lower,
          ci_upper = fc_only$upper,
          dates = fc_only$date
        )
      }, error = function(e) {
        message("Method '", m, "' failed: ", e$message)
      })
    }
  }
  
  # Monte Carlo
  if ("monte_carlo" %in% methods) {
    tryCatch({
      mc <- anki_forecast_monte_carlo(path, profile, 
                                      days_ahead = days_ahead,
                                      n_sim = 500)
      results[["monte_carlo"]] <- list(
        method = "monte_carlo",
        forecast = mc$summary$median,
        ci_lower = mc$summary$ci_lower_95,
        ci_upper = mc$summary$ci_upper_95,
        dates = mc$summary$date,
        full_mc = mc
      )
    }, error = function(e) {
      message("Monte Carlo failed: ", e$message)
    })
  }
  
  if (length(results) == 0) {
    stop("All forecasting methods failed")
  }
  
  # Build comparison summary
  comparison_df <- do.call(rbind, lapply(names(results), function(m) {
    r <- results[[m]]
    ci_lo <- if (is.null(r$ci_lower)) rep(NA_real_, length(r$forecast)) else r$ci_lower
    ci_hi <- if (is.null(r$ci_upper)) rep(NA_real_, length(r$forecast)) else r$ci_upper
    tibble::tibble(
      method = m,
      day = seq_along(r$forecast),
      date = r$dates,
      forecast = r$forecast,
      ci_lower = ci_lo,
      ci_upper = ci_hi,
      ci_width = ci_hi - ci_lo
    )
  }))
  
  # Summary by method
  method_summary <- do.call(rbind, lapply(names(results), function(m) {
    r <- results[[m]]
    ci_lo <- if (is.null(r$ci_lower)) rep(NA_real_, length(r$forecast)) else r$ci_lower
    ci_hi <- if (is.null(r$ci_upper)) rep(NA_real_, length(r$forecast)) else r$ci_upper
    tibble::tibble(
      method = m,
      mean_forecast = mean(r$forecast, na.rm = TRUE),
      total_forecast = sum(r$forecast, na.rm = TRUE),
      avg_ci_width = mean(ci_hi - ci_lo, na.rm = TRUE),
      max_forecast = max(r$forecast, na.rm = TRUE),
      min_forecast = min(r$forecast, na.rm = TRUE)
    )
  }))
  
  list(
    comparison = tibble::as_tibble(comparison_df),
    method_summary = tibble::as_tibble(method_summary),
    results = results
  )
}

#' Print Monte Carlo Forecast
#'
#' @param x An anki_mc_forecast object
#' @param ... Additional arguments (ignored)
#' @export
print.anki_mc_forecast <- function(x, ...) {
  cat("Monte Carlo Forecast\n")
  cat("====================\n")
  cat("Simulations:", x$parameters$n_sim, "\n")
  cat("Days ahead:", x$parameters$days_ahead, "\n")
  cat("Method:", x$parameters$method, "bootstrap\n")
  if (x$parameters$include_trend) {
    cat("Trend:", round(x$parameters$trend_slope, 2), "reviews/day\n")
  }
  cat("\nHistorical baseline:\n")
  cat("  Mean daily reviews:", round(x$historical$mean, 1), "\n")
  cat("  Days with 0 reviews:", x$historical$zero_days, 
      "(", round(x$historical$zero_rate * 100, 1), "%)\n")
  cat("\nForecast summary (day", x$parameters$days_ahead, "):\n")
  last_day <- x$summary[x$summary$day == x$parameters$days_ahead, ]
  cat("  Median:", round(last_day$median, 0), "reviews\n")
  cat("  95% CI: [", round(last_day$ci_lower_95, 0), ", ", 
      round(last_day$ci_upper_95, 0), "]\n", sep = "")
  cat("  P(>100 reviews):", round(last_day$prob_above_100 * 100, 1), "%\n")
  cat("\nTotal reviews over", x$parameters$days_ahead, "days:\n")
  last_cum <- x$cumulative[x$cumulative$day == x$parameters$days_ahead, ]
  cat("  Median:", round(last_cum$cumulative_median, 0), "\n")
  cat("  95% CI: [", round(last_cum$cumulative_ci_lower_95, 0), ", ",
      round(last_cum$cumulative_ci_upper_95, 0), "]\n", sep = "")
  invisible(x)
}
