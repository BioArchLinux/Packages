#' Plot review heatmap calendar
#'
#' Creates a calendar heatmap showing review activity.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param year Year to display (default: current year)
#' @param colors Vector of colors for the gradient (low to high)
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_heatmap()
#' anki_plot_heatmap(year = 2024)
#' }
anki_plot_heatmap <- function(path = NULL, profile = NULL, year = NULL,
                              colors = c("#ebedf0", "#9be9a8", "#40c463", "#30a14e", "#216e39")) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required. Install with: install.packages('ggplot2')")
  }
 
  if (is.null(year)) year <- as.integer(format(Sys.Date(), "%Y"))
 
  data <- anki_heatmap_data(path, profile, year)
 
  if (nrow(data) == 0) {
    message("No review data for year ", year)
    return(invisible(NULL))
  }
 
  # Create full year grid
  start_date <- as.Date(paste0(year, "-01-01"))
  end_date <- as.Date(paste0(year, "-12-31"))
  all_dates <- seq(start_date, end_date, by = "day")
 
  full_grid <- data.frame(
    date = all_dates,
    weekday = factor(weekdays(all_dates, abbreviate = TRUE),
                     levels = c("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")),
    week = as.integer(format(all_dates, "%W")),
    month = format(all_dates, "%b")
  )
 
  # Merge with actual data
  full_grid <- merge(full_grid, data[, c("date", "reviews")], by = "date", all.x = TRUE)
  full_grid$reviews[is.na(full_grid$reviews)] <- 0
 
  # Plot
  ggplot2::ggplot(full_grid, ggplot2::aes(x = week, y = weekday, fill = reviews)) +
    ggplot2::geom_tile(color = "white", linewidth = 0.4) +
    ggplot2::scale_fill_gradientn(colors = colors, name = "Reviews") +
    ggplot2::scale_y_discrete(limits = rev(c("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"))) +
    ggplot2::labs(title = paste("Review Activity", year), x = "Week", y = NULL) +
    ggplot2::theme_minimal() +
    ggplot2::theme(
      panel.grid = ggplot2::element_blank(),
      axis.text.x = ggplot2::element_blank()
    )
}

#' Plot retention over time
#'
#' Shows how retention rate has changed over time.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to include (default 90)
#' @param window Rolling window size in days for smoothing (default 7)
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_retention()
#' anki_plot_retention(days = 365, window = 14)
#' }
anki_plot_retention <- function(path = NULL, profile = NULL, days = 90, window = 7) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required.")
  }
 
  daily <- anki_stats_daily(path, profile, from = Sys.Date() - days)
 
  if (nrow(daily) < window) {
    message("Not enough data for plotting")
    return(invisible(NULL))
  }
 
  # Calculate rolling retention
  daily <- daily[order(daily$date), ]
  daily$rolling_retention <- zoo_rollmean(daily$retention, window)
 
  ggplot2::ggplot(daily, ggplot2::aes(x = date)) +
    ggplot2::geom_line(ggplot2::aes(y = retention), alpha = 0.3, color = "blue") +
    ggplot2::geom_line(ggplot2::aes(y = rolling_retention), color = "blue", linewidth = 1) +
    ggplot2::geom_hline(yintercept = 0.9, linetype = "dashed", color = "green", alpha = 0.7) +
    ggplot2::scale_y_continuous(labels = scales_percent, limits = c(0.5, 1)) +
    ggplot2::labs(
      title = "Retention Rate Over Time",
      subtitle = paste0(window, "-day rolling average"),
      x = NULL, y = "Retention"
    ) +
    ggplot2::theme_minimal()
}

#' Plot review forecast
#'
#' Shows predicted upcoming review workload.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to forecast (default 30)
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_forecast()
#' }
anki_plot_forecast <- function(path = NULL, profile = NULL, days = 30) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required.")
  }
 
  forecast <- anki_forecast(path, profile, days)
 
  ggplot2::ggplot(forecast, ggplot2::aes(x = date, y = new_due)) +
    ggplot2::geom_col(fill = "#4a90d9", alpha = 0.8) +
    ggplot2::geom_smooth(method = "loess", se = FALSE, color = "darkblue", formula = y ~ x) +
    ggplot2::labs(
      title = "Review Forecast",
      subtitle = paste("Next", days, "days"),
      x = NULL, y = "Cards Due"
    ) +
    ggplot2::theme_minimal()
}

#' Plot difficulty distribution
#'
#' Histogram of card difficulties (FSRS).
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by_deck If TRUE, facet by deck
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_difficulty()
#' }
anki_plot_difficulty <- function(path = NULL, profile = NULL, by_deck = FALSE) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required.")
  }
 
  cards <- anki_cards_fsrs(path, profile)
  cards <- cards[!is.na(cards$difficulty), ]
 
  if (nrow(cards) == 0) {
    message("No FSRS data found")
    return(invisible(NULL))
  }
 
  p <- ggplot2::ggplot(cards, ggplot2::aes(x = difficulty)) +
    ggplot2::geom_histogram(bins = 30, fill = "#e74c3c", alpha = 0.7, color = "white") +
    ggplot2::geom_vline(xintercept = mean(cards$difficulty, na.rm = TRUE),
                        linetype = "dashed", color = "darkred") +
    ggplot2::labs(
      title = "Card Difficulty Distribution",
      subtitle = paste("Mean:", round(mean(cards$difficulty, na.rm = TRUE), 2)),
      x = "Difficulty", y = "Count"
    ) +
    ggplot2::theme_minimal()
 
  if (by_deck) {
    decks <- anki_decks(path, profile)
    cards <- merge(cards, decks, by = "did", all.x = TRUE)
    p <- p + ggplot2::facet_wrap(~name, scales = "free_y")
  }
 
  p
}

#' Plot interval distribution
#'
#' Histogram of card intervals.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param log_scale If TRUE, use log scale for x-axis
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_intervals()
#' }
anki_plot_intervals <- function(path = NULL, profile = NULL, log_scale = TRUE) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required.")
  }
 
  cards <- anki_cards(path, profile)
  cards <- cards[cards$type == 2 & cards$ivl > 0, ]
 
  if (nrow(cards) == 0) {
    message("No review cards found")
    return(invisible(NULL))
  }
 
  p <- ggplot2::ggplot(cards, ggplot2::aes(x = ivl)) +
    ggplot2::geom_histogram(bins = 50, fill = "#3498db", alpha = 0.7, color = "white") +
    ggplot2::geom_vline(xintercept = 21, linetype = "dashed", color = "green",
                        alpha = 0.7) +
    ggplot2::annotate("text", x = 21, y = Inf, label = "Mature (21d)",
                      vjust = 2, hjust = -0.1, size = 3) +
    ggplot2::labs(
      title = "Card Interval Distribution",
      subtitle = paste("Median:", round(median(cards$ivl)), "days"),
      x = "Interval (days)", y = "Count"
    ) +
    ggplot2::theme_minimal()
 
  if (log_scale) {
    p <- p + ggplot2::scale_x_log10()
  }
 
  p
}

#' Plot stability distribution
#'
#' Histogram of FSRS stability values.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_stability()
#' }
anki_plot_stability <- function(path = NULL, profile = NULL) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required.")
  }
 
  cards <- anki_cards_fsrs(path, profile)
  cards <- cards[!is.na(cards$stability) & cards$stability > 0, ]
 
  if (nrow(cards) == 0) {
    message("No FSRS data found")
    return(invisible(NULL))
  }
 
  ggplot2::ggplot(cards, ggplot2::aes(x = stability)) +
    ggplot2::geom_histogram(bins = 50, fill = "#2ecc71", alpha = 0.7, color = "white") +
    ggplot2::scale_x_log10() +
    ggplot2::geom_vline(xintercept = median(cards$stability, na.rm = TRUE),
                        linetype = "dashed", color = "darkgreen") +
    ggplot2::labs(
      title = "Memory Stability Distribution",
      subtitle = paste("Median:", round(median(cards$stability, na.rm = TRUE), 1), "days"),
      x = "Stability (days, log scale)", y = "Count"
    ) +
    ggplot2::theme_minimal()
}

#' Plot reviews by hour of day
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to include
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_hours()
#' }
anki_plot_hours <- function(path = NULL, profile = NULL, days = 90) {
 if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required.")
  }
 
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
 
  if (!is.null(days)) {
    cutoff <- Sys.Date() - days
    revlog <- revlog[revlog$review_date >= cutoff, ]
  }
 
  if (nrow(revlog) == 0) {
    message("No review data")
    return(invisible(NULL))
  }
 
  # Extract hour from timestamp (rid is in ms)
  revlog$hour <- as.integer(format(as.POSIXct(revlog$rid / 1000, origin = "1970-01-01"), "%H"))
 
  hourly <- as.data.frame(table(revlog$hour), stringsAsFactors = FALSE)
  names(hourly) <- c("hour", "reviews")
  hourly$hour <- as.integer(hourly$hour)
 
  ggplot2::ggplot(hourly, ggplot2::aes(x = hour, y = reviews)) +
    ggplot2::geom_col(fill = "#9b59b6", alpha = 0.8) +
    ggplot2::scale_x_continuous(breaks = seq(0, 23, 3)) +
    ggplot2::labs(
      title = "Reviews by Hour of Day",
      subtitle = paste("Last", days, "days"),
      x = "Hour", y = "Reviews"
    ) +
    ggplot2::theme_minimal()
}

#' Plot reviews by day of week
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days Number of days to include
#' @return A ggplot2 object
#' @export
#' @examples
#' \dontrun{
#' anki_plot_weekdays()
#' }
anki_plot_weekdays <- function(path = NULL, profile = NULL, days = 90) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required.")
  }
 
  daily <- anki_stats_daily(path, profile, from = Sys.Date() - days)
 
  if (nrow(daily) == 0) {
    message("No review data")
    return(invisible(NULL))
  }
 
  daily$weekday <- factor(
    weekdays(daily$date, abbreviate = TRUE),
    levels = c("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
  )
 
  by_weekday <- aggregate(reviews ~ weekday, data = daily, FUN = mean)
 
  ggplot2::ggplot(by_weekday, ggplot2::aes(x = weekday, y = reviews)) +
    ggplot2::geom_col(fill = "#f39c12", alpha = 0.8) +
    ggplot2::labs(
      title = "Average Reviews by Day of Week",
      subtitle = paste("Last", days, "days"),
      x = NULL, y = "Avg Reviews"
    ) +
    ggplot2::theme_minimal()
}

# Helper functions (not exported)
zoo_rollmean <- function(x, k) {
  n <- length(x)
  if (n < k) return(rep(NA_real_, n))
 
  result <- rep(NA_real_, n)
  for (i in k:n) {
    result[i] <- mean(x[(i - k + 1):i], na.rm = TRUE)
  }
  result
}

scales_percent <- function(x) paste0(round(x * 100), "%")
