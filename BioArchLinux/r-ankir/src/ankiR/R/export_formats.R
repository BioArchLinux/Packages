#' Export deck to Org-mode format
#'
#' Exports cards to Emacs Org-mode flashcard format (org-drill compatible).
#'
#' @param deck Deck name or ID
#' @param file Output file path (default: deck name + .org)
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param include_scheduling If TRUE, include scheduling data as properties
#' @return Invisibly returns the number of cards exported
#' @export
#' @examples
#' \dontrun{
#' anki_to_org("Medical")
#' anki_to_org("Vocabulary", file = "vocab.org")
#' }
anki_to_org <- function(deck, file = NULL, path = NULL, profile = NULL,
                        include_scheduling = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
 
  # Find matching deck
  if (is.numeric(deck)) {
    deck_ids <- deck
    deck_name <- decks$name[decks$did == deck]
  } else {
    deck_ids <- decks$did[grepl(deck, decks$name, ignore.case = TRUE)]
    deck_name <- deck
  }
 
  if (length(deck_ids) == 0) {
    stop("No deck found matching: ", deck)
  }
 
  cards <- cards[cards$did %in% deck_ids, ]
  cards <- merge(cards, notes, by = "nid", all.x = TRUE)
 
  if (nrow(cards) == 0) {
    message("No cards found in deck")
    return(invisible(0))
  }
 
  if (is.null(file)) {
    file <- paste0(gsub("[^a-zA-Z0-9]", "_", deck_name), ".org")
  }
 
  lines <- c(
    paste0("#+TITLE: ", deck_name),
    paste0("#+DATE: ", Sys.Date()),
    "#+FILETAGS: :drill:",
    ""
  )
 
  for (i in seq_len(nrow(cards))) {
    card <- cards[i, ]
    fields <- strsplit(card$flds, "\x1f")[[1]]
    front <- clean_html(fields[1])
    back <- if (length(fields) > 1) clean_html(fields[2]) else ""
   
    lines <- c(lines, paste0("* ", substr(front, 1, 60), "... :drill:"))
   
    if (include_scheduling) {
      lines <- c(lines,
        ":PROPERTIES:",
        paste0(":ANKI_ID: ", card$cid),
        paste0(":ANKI_INTERVAL: ", card$ivl),
        ":END:"
      )
    }
    lines <- c(lines, "", front, "", "** Answer", "", back, "")
  }
 
  writeLines(lines, file)
  message("Exported ", nrow(cards), " cards to ", file)
  invisible(nrow(cards))
}

#' Export deck to Markdown format
#'
#' Exports cards to Markdown flashcard format.
#'
#' @param deck Deck name or ID
#' @param file Output file path
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param format Format: "obsidian", "logseq", or "basic"
#' @return Invisibly returns the number of cards exported
#' @export
#' @examples
#' \dontrun{
#' anki_to_markdown("Medical")
#' }
anki_to_markdown <- function(deck, file = NULL, path = NULL, profile = NULL,
                             format = c("obsidian", "logseq", "basic")) {
  format <- match.arg(format)
 
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
 
  if (is.numeric(deck)) {
    deck_ids <- deck
    deck_name <- decks$name[decks$did == deck]
  } else {
    deck_ids <- decks$did[grepl(deck, decks$name, ignore.case = TRUE)]
    deck_name <- deck
  }
 
  if (length(deck_ids) == 0) stop("No deck found matching: ", deck)
 
  cards <- cards[cards$did %in% deck_ids, ]
  cards <- merge(cards, notes, by = "nid", all.x = TRUE)
 
  if (nrow(cards) == 0) {
    message("No cards found")
    return(invisible(0))
  }
 
  if (is.null(file)) {
    file <- paste0(gsub("[^a-zA-Z0-9]", "_", deck_name), ".md")
  }
 
  lines <- c(paste0("# ", deck_name), "", paste0("*Exported ", Sys.Date(), "*"), "", "---", "")
 
  for (i in seq_len(nrow(cards))) {
    fields <- strsplit(cards$flds[i], "\x1f")[[1]]
    front <- clean_html(fields[1])
    back <- if (length(fields) > 1) clean_html(fields[2]) else ""
   
    if (format == "obsidian") {
      lines <- c(lines, front, "", "?", "", back, "", "---", "")
    } else if (format == "logseq") {
      lines <- c(lines, paste0("- ", front, " #card"), paste0("  - ", back), "")
    } else {
      lines <- c(lines, paste0("**Q:** ", front), "", paste0("**A:** ", back), "", "---", "")
    }
  }
 
  writeLines(lines, file)
  message("Exported ", nrow(cards), " cards to ", file)
  invisible(nrow(cards))
}

#' Export deck to SuperMemo Q&A format
#'
#' @param deck Deck name or ID
#' @param file Output file path
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return Invisibly returns the number of cards exported
#' @export
#' @examples
#' \dontrun{
#' anki_to_supermemo("Medical")
#' }
anki_to_supermemo <- function(deck, file = NULL, path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
 
  if (is.numeric(deck)) {
    deck_ids <- deck
    deck_name <- decks$name[decks$did == deck]
  } else {
    deck_ids <- decks$did[grepl(deck, decks$name, ignore.case = TRUE)]
    deck_name <- deck
  }
 
  if (length(deck_ids) == 0) stop("No deck found matching: ", deck)
 
  cards <- cards[cards$did %in% deck_ids, ]
  cards <- merge(cards, notes, by = "nid", all.x = TRUE)
 
  if (nrow(cards) == 0) {
    message("No cards found")
    return(invisible(0))
  }
 
  if (is.null(file)) {
    file <- paste0(gsub("[^a-zA-Z0-9]", "_", deck_name), "_sm.txt")
  }
 
  lines <- character()
  for (i in seq_len(nrow(cards))) {
    fields <- strsplit(cards$flds[i], "\x1f")[[1]]
    front <- gsub("\n", " ", clean_html(fields[1]))
    back <- if (length(fields) > 1) gsub("\n", " ", clean_html(fields[2])) else ""
    lines <- c(lines, paste0("Q: ", front), paste0("A: ", back), "")
  }
 
  writeLines(lines, file)
  message("Exported ", nrow(cards), " cards to ", file)
  invisible(nrow(cards))
}

#' Export reviews for external FSRS analysis
#'
#' @param file Output file path (.csv or .json)
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param format Output format: "csv" or "json"
#' @return Invisibly returns the exported data
#' @export
#' @examples
#' \dontrun{
#' fsrs_export_reviews("reviews.csv")
#' }
fsrs_export_reviews <- function(file, path = NULL, profile = NULL, format = "csv") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  revlog <- col$revlog()
  cards <- col$cards()
  decks <- col$decks()
 
  revlog <- merge(revlog, cards[, c("cid", "did", "nid")], by = "cid", all.x = TRUE)
  revlog <- merge(revlog, decks[, c("did", "name")], by = "did", all.x = TRUE)
 
  revlog$datetime <- as.POSIXct(revlog$rid / 1000, origin = "1970-01-01")
  revlog <- revlog[order(revlog$cid, revlog$rid), ]
 
  revlog$delta_t <- ave(as.numeric(revlog$review_date), revlog$cid, FUN = function(x) c(0, diff(x)))
 
  export_data <- revlog[, c("cid", "nid", "name", "datetime", "review_date", "ease", "delta_t", "ivl", "time")]
  names(export_data)[names(export_data) == "name"] <- "deck"
  names(export_data)[names(export_data) == "ease"] <- "rating"
 
  if (format == "json") {
    jsonlite::write_json(export_data, file, pretty = TRUE)
  } else {
    utils::write.csv(export_data, file, row.names = FALSE)
  }
 
  message("Exported ", nrow(export_data), " reviews to ", file)
  invisible(tibble::as_tibble(export_data))
}

#' Export collection report to HTML
#'
#' @param file Output HTML file path
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return Invisibly returns the file path
#' @export
#' @examples
#' \dontrun{
#' anki_to_html("report.html")
#' }
anki_to_html <- function(file, path = NULL, profile = NULL) {
  report <- anki_report(path, profile)
  deck_stats <- anki_stats_deck(path, profile)
  streak <- anki_streak(path, profile)
 
  html <- c(
    "<!DOCTYPE html>",
    "<html><head>",
    "<meta charset='utf-8'>",
    "<title>Anki Collection Report</title>",
    "<style>",
    "body { font-family: -apple-system, sans-serif; max-width: 900px; margin: 0 auto; padding: 20px; }",
    "h1 { color: #2c3e50; } h2 { color: #34495e; border-bottom: 2px solid #3498db; }",
    "table { border-collapse: collapse; width: 100%; margin: 15px 0; }",
    "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }",
    "th { background-color: #3498db; color: white; }",
    "tr:nth-child(even) { background-color: #f2f2f2; }",
    ".stat-box { display: inline-block; background: #ecf0f1; padding: 15px; margin: 5px; border-radius: 8px; }",
    ".stat-value { font-size: 24px; font-weight: bold; color: #2c3e50; }",
    ".stat-label { color: #7f8c8d; }",
    "</style></head><body>",
    paste0("<h1>Anki Collection Report</h1><p>Generated: ", Sys.Date(), "</p>"),
    "<h2>Overview</h2>",
    paste0("<div class='stat-box'><div class='stat-value'>", report$total_cards, "</div><div class='stat-label'>Cards</div></div>"),
    paste0("<div class='stat-box'><div class='stat-value'>", report$total_notes, "</div><div class='stat-label'>Notes</div></div>"),
    paste0("<div class='stat-box'><div class='stat-value'>", format(report$total_reviews, big.mark=","), "</div><div class='stat-label'>Reviews</div></div>"),
    paste0("<div class='stat-box'><div class='stat-value'>", streak$current_streak, "</div><div class='stat-label'>Day Streak</div></div>"),
    "<h2>Card Breakdown</h2><table><tr><th>Status</th><th>Count</th><th>%</th></tr>",
    paste0("<tr><td>New</td><td>", report$new_cards, "</td><td>", round(report$new_cards/report$total_cards*100, 1), "%</td></tr>"),
    paste0("<tr><td>Learning</td><td>", report$learning_cards, "</td><td>", round(report$learning_cards/report$total_cards*100, 1), "%</td></tr>"),
    paste0("<tr><td>Review</td><td>", report$review_cards, "</td><td>", round(report$review_cards/report$total_cards*100, 1), "%</td></tr>"),
    paste0("<tr><td>Mature</td><td>", report$mature_cards, "</td><td>", round(report$mature_percent, 1), "%</td></tr>"),
    "</table>",
    "<h2>Deck Statistics</h2><table><tr><th>Deck</th><th>Total</th><th>New</th><th>Review</th><th>Mature</th></tr>"
  )
 
  for (i in seq_len(min(nrow(deck_stats), 20))) {
    html <- c(html, paste0("<tr><td>", deck_stats$name[i], "</td><td>", deck_stats$total[i],
              "</td><td>", deck_stats$new[i], "</td><td>", deck_stats$review[i],
              "</td><td>", deck_stats$mature[i], "</td></tr>"))
  }
  html <- c(html, "</table><footer><p>Generated by ankiR</p></footer></body></html>")
 
  writeLines(html, file)
  message("Report saved to ", file)
  invisible(file)
}

#' Import review data from CSV for analysis
#'
#' @param file Path to CSV file
#' @param date_col Name of date column
#' @param rating_col Name of rating column
#' @param card_col Name of card ID column
#' @param date_format Date format string
#' @return A tibble with standardized review data
#' @export
#' @examples
#' \dontrun{
#' reviews <- fsrs_from_csv("reviews.csv", date_col = "date", rating_col = "grade")
#' }
fsrs_from_csv <- function(file, date_col = "date", rating_col = "rating",
                          card_col = "card_id", date_format = "%Y-%m-%d") {
  data <- utils::read.csv(file, stringsAsFactors = FALSE)
 
  required <- c(date_col, rating_col, card_col)
  missing <- setdiff(required, names(data))
  if (length(missing) > 0) stop("Missing columns: ", paste(missing, collapse = ", "))
 
  result <- tibble::tibble(
    card_id = data[[card_col]],
    review_date = as.Date(data[[date_col]], format = date_format),
    rating = as.integer(data[[rating_col]])
  )
 
  result <- result[order(result$card_id, result$review_date), ]
  result$review_num <- ave(seq_len(nrow(result)), result$card_id, FUN = seq_along)
  result$delta_t <- ave(as.numeric(result$review_date), result$card_id,
                        FUN = function(x) c(0, diff(x)))
 
  message("Loaded ", nrow(result), " reviews for ", length(unique(result$card_id)), " cards")
  tibble::as_tibble(result)
}

#' Export to Anki-importable format
#'
#' @param data Data frame with front and back columns
#' @param file Output file path
#' @param tags Optional tags
#' @return Invisibly returns number of cards
#' @export
#' @examples
#' \dontrun{
#' cards <- data.frame(front = c("Q1", "Q2"), back = c("A1", "A2"))
#' anki_export_importable(cards, "new_cards.txt")
#' }
anki_export_importable <- function(data, file, tags = NULL) {
  if (!all(c("front", "back") %in% names(data))) {
    stop("Data must have 'front' and 'back' columns")
  }
 
  output <- data.frame(front = data$front, back = data$back, stringsAsFactors = FALSE)
  if (!is.null(tags)) output$tags <- tags
 
  utils::write.table(output, file, sep = "\t", row.names = FALSE, col.names = FALSE, quote = FALSE)
  message("Exported ", nrow(output), " cards to ", file)
  invisible(nrow(output))
}

# Helper function
clean_html <- function(text) {
  if (is.na(text) || is.null(text)) return("")
  text <- gsub("<br\\s*/?>", "\n", text, ignore.case = TRUE)
  text <- gsub("<[^>]+>", "", text)
  text <- gsub("\\[sound:[^\\]]+\\]", "", text)
  text <- gsub("&nbsp;", " ", text)
  text <- gsub("&amp;", "&", text)
  text <- gsub("&lt;", "<", text)
  text <- gsub("&gt;", ">", text)
  trimws(text)
}
