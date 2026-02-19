#' Fit forgetting curve from review data
#'
#' Fits a forgetting curve to actual review data and compares it to the
#' theoretical FSRS curve. Can analyze individual cards or aggregate data.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param cid Optional card ID for individual card analysis
#' @param min_reviews Minimum reviews required for analysis (default 5)
#' @param max_cards Maximum cards to analyze for aggregate (default 1000)
#' @return A list with fitted curve parameters and comparison data
#' @export
#' @examples
#' \dontrun{
#' curve <- anki_fit_forgetting_curve()
#' plot(curve$data$days_elapsed, curve$data$observed_retention, type = "p")
#' lines(curve$data$days_elapsed, curve$data$fitted_retention, col = "blue")
#' }
anki_fit_forgetting_curve <- function(path = NULL, profile = NULL, cid = NULL,
                                       min_reviews = 5, max_cards = 1000) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  revlog <- col$revlog()

  if (nrow(revlog) < min_reviews) {
    message("Not enough review data")
    return(NULL)
  }

  # Order by card and time
  revlog <- revlog[order(revlog$cid, revlog$rid), ]

  if (!is.null(cid)) {
    # Single card analysis
    revlog <- revlog[revlog$cid == cid, ]
    if (nrow(revlog) < min_reviews) {
      message("Card has fewer than ", min_reviews, " reviews")
      return(NULL)
    }
    return(fit_single_card_curve(revlog))
  }

  # Aggregate analysis
  # Calculate retention at different intervals
  # Group reviews by days since last review

  # Calculate delta_t for each review
  revlog_split <- split(revlog, revlog$cid)

  # Sample if too many cards
  if (length(revlog_split) > max_cards) {
    revlog_split <- revlog_split[sample(length(revlog_split), max_cards)]
  }

  all_data <- lapply(revlog_split, function(card_revs) {
    if (nrow(card_revs) < 2) return(NULL)

    card_revs <- card_revs[order(card_revs$rid), ]
    dates <- card_revs$review_date

    # Calculate days between reviews
    delta_t <- c(NA, as.numeric(diff(dates)))

    # Success = ease > 1
    success <- card_revs$ease > 1

    # Return non-first reviews (we need delta_t)
    data.frame(
      delta_t = delta_t[-1],
      success = success[-1],
      ease = card_revs$ease[-1],
      ivl = card_revs$ivl[-1]
    )
  })

  combined <- do.call(rbind, Filter(Negate(is.null), all_data))

  if (nrow(combined) < 100) {
    message("Not enough data points for reliable curve fitting")
    return(NULL)
  }

  # Bin by days elapsed
  combined$bin <- cut(combined$delta_t,
                      breaks = c(0, 1, 2, 3, 5, 7, 14, 21, 30, 60, 90, 180, 365, Inf),
                      labels = c("1", "2", "3", "4-5", "6-7", "8-14", "15-21",
                                 "22-30", "31-60", "61-90", "91-180", "181-365", "365+"))

  # Calculate observed retention per bin
  retention_by_bin <- lapply(split(combined, combined$bin), function(d) {
    if (nrow(d) < 10) return(NULL)
    tibble::tibble(
      bin = as.character(d$bin[1]),
      mid_days = mean(d$delta_t, na.rm = TRUE),
      n = nrow(d),
      retention = mean(d$success),
      se = sqrt(mean(d$success) * (1 - mean(d$success)) / nrow(d))
    )
  })

  observed <- do.call(rbind, Filter(Negate(is.null), retention_by_bin))
  observed <- observed[order(observed$mid_days), ]

  # Fit power law: R = (1 + factor * t / S)^(-decay)
  # Use nonlinear least squares
  fit_result <- tryCatch({
    # Initial estimates
    start_S <- 30
    start_decay <- 0.5

    nls_fit <- stats::nls(
      retention ~ (1 + (0.9^(-1/decay) - 1) * mid_days / S)^(-decay),
      data = observed,
      start = list(S = start_S, decay = start_decay),
      lower = c(S = 1, decay = 0.01),
      upper = c(S = 365, decay = 2),
      algorithm = "port",
      control = list(maxiter = 100)
    )

    coefs <- stats::coef(nls_fit)
    list(
      stability = coefs["S"],
      decay = coefs["decay"],
      converged = TRUE
    )
  }, error = function(e) {
    # Fall back to simpler estimation
    # Linear regression on log-transformed data
    observed$log_ret <- log(observed$retention + 0.01)
    observed$log_t <- log(observed$mid_days + 1)

    lm_fit <- stats::lm(log_ret ~ log_t, data = observed)
    decay_est <- -stats::coef(lm_fit)[2]

    # Estimate S from median
    med_ret <- median(observed$retention)
    med_t <- median(observed$mid_days)
    factor_est <- 0.9^(-1/decay_est) - 1
    S_est <- med_t * factor_est / ((med_ret^(-1/decay_est)) - 1)

    list(
      stability = max(1, min(365, S_est)),
      decay = max(0.1, min(1, decay_est)),
      converged = FALSE
    )
  })

  # Calculate fitted values
  factor <- 0.9^(-1/fit_result$decay) - 1
  observed$fitted_retention <- (1 + factor * observed$mid_days / fit_result$stability)^(-fit_result$decay)

  # FSRS default comparison (S=30, decay=0.5)
  fsrs_factor <- 0.9^(-1/0.5) - 1
  observed$fsrs_default <- (1 + fsrs_factor * observed$mid_days / 30)^(-0.5)

  # Goodness of fit
  ss_res <- sum((observed$retention - observed$fitted_retention)^2)
  ss_tot <- sum((observed$retention - mean(observed$retention))^2)
  r_squared <- 1 - ss_res / ss_tot

  list(
    parameters = tibble::tibble(
      parameter = c("Estimated Stability (S)", "Estimated Decay", "R-squared",
                    "FSRS Default Stability", "FSRS Default Decay"),
      value = c(round(fit_result$stability, 1), round(fit_result$decay, 3),
                round(r_squared, 3), 30, 0.5),
      notes = c(
        "Days for retention to drop to 90%",
        "Rate of forgetting (lower = slower)",
        "Model fit quality (1 = perfect)",
        "FSRS-4.5 default",
        "FSRS-4.5 default"
      )
    ),
    data = tibble::as_tibble(observed),
    comparison = tibble::tibble(
      aspect = c("Your Stability vs Default", "Your Decay vs Default"),
      your_value = c(round(fit_result$stability, 1), round(fit_result$decay, 3)),
      default_value = c(30, 0.5),
      interpretation = c(
        if (fit_result$stability > 30) "Better than average retention" else "Below average retention",
        if (fit_result$decay < 0.5) "Slower forgetting rate" else "Faster forgetting rate"
      )
    ),
    converged = fit_result$converged
  )
}

#' @keywords internal
fit_single_card_curve <- function(revlog) {
  revlog <- revlog[order(revlog$rid), ]

  if (nrow(revlog) < 3) {
    return(NULL)
  }

  dates <- revlog$review_date
  delta_t <- c(NA, as.numeric(diff(dates)))
  success <- revlog$ease > 1

  # Create data frame for non-first reviews
  data <- data.frame(
    review_num = 2:nrow(revlog),
    delta_t = delta_t[-1],
    success = success[-1],
    ease = revlog$ease[-1],
    ivl = revlog$ivl[-1]
  )

  # Calculate cumulative retention
  data$cum_retention <- cumsum(data$success) / seq_len(nrow(data))

  # Simple analysis
  list(
    card_id = revlog$cid[1],
    total_reviews = nrow(revlog),
    overall_retention = mean(data$success),
    avg_interval = mean(data$delta_t, na.rm = TRUE),
    data = tibble::as_tibble(data),
    summary = tibble::tibble(
      metric = c("Total Reviews", "Retention Rate", "Avg Interval (days)",
                 "Longest Success Streak", "Total Lapses"),
      value = c(
        nrow(revlog),
        round(mean(data$success) * 100, 1),
        round(mean(data$delta_t, na.rm = TRUE), 1),
        max(rle(data$success)$lengths[rle(data$success)$values], 0),
        sum(!data$success)
      )
    )
  )
}

#' Plot forgetting curve comparison
#'
#' Creates a visualization comparing your fitted forgetting curve to FSRS defaults.
#'
#' @param curve_data Output from anki_fit_forgetting_curve()
#' @return A ggplot object
#' @export
#' @examples
#' \dontrun{
#' curve <- anki_fit_forgetting_curve()
#' anki_plot_forgetting_curve(curve)
#' }
anki_plot_forgetting_curve <- function(curve_data) {
  if (!requireNamespace("ggplot2", quietly = TRUE)) {
    stop("Package 'ggplot2' is required for plotting")
  }

  if (is.null(curve_data) || is.null(curve_data$data)) {
    stop("Invalid curve data")
  }

  d <- curve_data$data

  ggplot2::ggplot(d, ggplot2::aes(x = .data$mid_days)) +
    ggplot2::geom_point(ggplot2::aes(y = .data$retention), size = 3, alpha = 0.7) +
    ggplot2::geom_errorbar(
      ggplot2::aes(ymin = .data$retention - 1.96 * .data$se, 
                   ymax = .data$retention + 1.96 * .data$se),
      width = 0.1, alpha = 0.5
    ) +
    ggplot2::geom_line(ggplot2::aes(y = .data$fitted_retention, color = "Your Curve"),
                       linewidth = 1.2) +
    ggplot2::geom_line(ggplot2::aes(y = .data$fsrs_default, color = "FSRS Default"),
                       linewidth = 1, linetype = "dashed") +
    ggplot2::scale_color_manual(
      values = c("Your Curve" = "#3498db", "FSRS Default" = "#e74c3c")
    ) +
    ggplot2::scale_x_log10() +
    ggplot2::scale_y_continuous(labels = scales::percent_format(accuracy = 1),
                                limits = c(0, 1)) +
    ggplot2::labs(
      title = "Your Forgetting Curve vs FSRS Default",
      x = "Days Since Last Review (log scale)",
      y = "Retention Rate",
      color = "Curve"
    ) +
    ggplot2::theme_minimal() +
    ggplot2::theme(legend.position = "bottom")
}
