#' Import ankiR Stats Addon Export
#'
#' Import data exported from the ankiR Stats Anki addon. This allows
#' analyzing Anki data in R without direct database access.
#'
#' @param path Path to the JSON export file
#' @return A list containing cards, revlog, daily_stats, and summary
#' @export
#'
#' @examples
#' \dontrun{
#' data <- import_addon_export("ankir_export.json")
#' data$summary
#' data$cards
#' data$daily_stats
#' }
import_addon_export <- function(path) {
  if (!file.exists(path)) {
    stop("File not found: ", path)
  }
  
  data <- jsonlite::fromJSON(path, simplifyVector = FALSE)
  
  if (is.null(data$version) || is.null(data$source)) {
    stop("Invalid export file format")
  }
  
  message("Importing ankiR Stats export from ", data$export_date)
  message("Source: ", data$source, " (v", data$version, ")")
  
  # Convert cards to tibble
  cards <- tibble::tibble(
    cid = sapply(data$cards, function(x) x$cid),
    nid = sapply(data$cards, function(x) x$nid),
    did = sapply(data$cards, function(x) x$did),
    type = sapply(data$cards, function(x) x$type),
    queue = sapply(data$cards, function(x) x$queue),
    ivl = sapply(data$cards, function(x) x$ivl),
    due = sapply(data$cards, function(x) x$due),
    reps = sapply(data$cards, function(x) x$reps),
    lapses = sapply(data$cards, function(x) x$lapses)
  )
  
  # Convert revlog to tibble
  revlog <- tibble::tibble(
    rid = sapply(data$revlog, function(x) x$rid),
    cid = sapply(data$revlog, function(x) x$cid),
    ease = sapply(data$revlog, function(x) x$ease),
    ivl = sapply(data$revlog, function(x) x$ivl),
    time = sapply(data$revlog, function(x) x$time),
    review_type = sapply(data$revlog, function(x) x$review_type)
  )
  revlog$review_date <- as.Date(as.POSIXct(revlog$rid / 1000, origin = "1970-01-01"))
  
 # Convert daily stats to tibble
  daily_stats <- tibble::tibble(
    date = as.Date(names(data$daily_stats)),
    reviews = sapply(data$daily_stats, function(x) x$reviews),
    time_minutes = sapply(data$daily_stats, function(x) x$time_minutes),
    retention = sapply(data$daily_stats, function(x) x$retention),
    again = sapply(data$daily_stats, function(x) x$again),
    easy = sapply(data$daily_stats, function(x) x$easy)
  )
  daily_stats <- daily_stats[order(daily_stats$date), ]
  
  # Summary
  summary <- data$summary
  
  message("Imported: ", nrow(cards), " cards, ", nrow(revlog), " reviews, ", 
          nrow(daily_stats), " days of stats")
  
  list(
    cards = cards,
    revlog = revlog,
    daily_stats = daily_stats,
    summary = summary,
    export_date = data$export_date,
    version = data$version
  )
}

#' Analyze Imported Addon Data
#'
#' Run common analyses on data imported from ankiR Stats addon.
#'
#' @param data Data imported via import_addon_export()
#' @return A list with analysis results
#' @export
#'
#' @examples
#' \dontrun{
#' data <- import_addon_export("ankir_export.json")
#' analysis <- analyze_addon_import(data)
#' analysis$retention_trend
#' }
analyze_addon_import <- function(data) {
  if (!all(c("cards", "revlog", "daily_stats") %in% names(data))) {
    stop("Invalid import data structure")
  }
  
  cards <- data$cards
  revlog <- data$revlog
  daily <- data$daily_stats
  
  # Card type breakdown
  card_types <- tibble::tibble(
    type = c("New", "Learning", "Review", "Relearning"),
    count = c(
      sum(cards$type == 0),
      sum(cards$type == 1),
      sum(cards$type == 2),
      sum(cards$type == 3)
    )
  )
  
  # Maturity breakdown
  mature_count <- sum(cards$type == 2 & cards$ivl >= 21)
  young_count <- sum(cards$type == 2 & cards$ivl < 21 & cards$ivl > 0)
  
  # Retention trend (weekly)
  daily$week <- format(daily$date, "%Y-W%V")
  weekly_retention <- aggregate(retention ~ week, data = daily, FUN = mean)
  weekly_retention$retention <- round(weekly_retention$retention, 1)
  
  # Review volume trend
  weekly_reviews <- aggregate(reviews ~ week, data = daily, FUN = sum)
  
  # Time spent trend
  weekly_time <- aggregate(time_minutes ~ week, data = daily, FUN = sum)
  weekly_time$time_hours <- round(weekly_time$time_minutes / 60, 1)
  
  # Overall stats
  overall <- list(
    total_cards = nrow(cards),
    mature_cards = mature_count,
    young_cards = young_count,
    total_reviews = nrow(revlog),
    total_days = nrow(daily),
    avg_daily_reviews = round(mean(daily$reviews), 1),
    overall_retention = round(mean(daily$retention), 1),
    total_time_hours = round(sum(daily$time_minutes) / 60, 1)
  )
  
  list(
    card_types = card_types,
    overall = overall,
    retention_trend = tibble::as_tibble(weekly_retention),
    review_trend = tibble::as_tibble(weekly_reviews),
    time_trend = tibble::as_tibble(weekly_time)
  )
}
