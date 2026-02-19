#' Search cards like Anki's browser
#'
#' Searches cards using a simplified version of Anki's search syntax.
#' Supports: deck:name, tag:name, is:new, is:due, is:suspended, is:buried,
#' is:learn, is:review, prop:ivl>N, prop:lapses>N, prop:reps>N
#'
#' @param query Search query string
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of matching cards
#' @export
#' @examples
#' \dontrun{
#' # Find suspended cards
#' anki_search("is:suspended")
#'
#' # Find cards in a specific deck
#' anki_search("deck:Default")
#'
#' # Find cards with many lapses
#' anki_search("prop:lapses>5")
#'
#' # Combine conditions
#' anki_search("deck:Medical is:review prop:ivl>30")
#' }
anki_search <- function(query, path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
 
  # Join for searching
  cards <- merge(cards, notes[, c("nid", "tags", "flds", "sfld")], by = "nid", all.x = TRUE)
  cards <- merge(cards, decks, by = "did", all.x = TRUE)
 
  # Parse and apply each search term
  terms <- strsplit(query, "\\s+")[[1]]
  mask <- rep(TRUE, nrow(cards))
 
  for (term in terms) {
    term_mask <- parse_search_term(term, cards)
    mask <- mask & term_mask
  }
 
  tibble::as_tibble(cards[mask, ])
}

#' @keywords internal
parse_search_term <- function(term, cards) {
  # Handle negation
  negate <- FALSE
  if (startsWith(term, "-")) {
    negate <- TRUE
    term <- substring(term, 2)
  }
 
  mask <- rep(TRUE, nrow(cards))
 
  if (startsWith(term, "deck:")) {
    deck_name <- substring(term, 6)
    mask <- grepl(deck_name, cards$name, ignore.case = TRUE)
   
  } else if (startsWith(term, "tag:")) {
    tag_name <- substring(term, 5)
    mask <- grepl(tag_name, cards$tags, ignore.case = TRUE)
   
  } else if (term == "is:new") {
    mask <- cards$type == 0
   
  } else if (term == "is:learn" || term == "is:learning") {
    mask <- cards$type == 1 | cards$type == 3
   
  } else if (term == "is:review") {
    mask <- cards$type == 2
   
  } else if (term == "is:suspended") {
    mask <- cards$queue == -1
   
  } else if (term == "is:buried") {
    mask <- cards$queue == -2 | cards$queue == -3
   
  } else if (term == "is:due") {
    # Cards that are due today or overdue
    mask <- cards$queue == 2 & cards$due <= as.integer(Sys.Date())
   
  } else if (startsWith(term, "prop:")) {
    prop_expr <- substring(term, 6)
    mask <- parse_prop_search(prop_expr, cards)
   
  } else {
    # Text search in sort field
    mask <- grepl(term, cards$sfld, ignore.case = TRUE)
  }
 
  if (negate) mask <- !mask
  mask
}

#' @keywords internal
parse_prop_search <- function(expr, cards) {
  # Parse prop:field>value or prop:field<value or prop:field=value
  match <- regexpr("(\\w+)([<>=]+)(\\d+)", expr, perl = TRUE)
 
  if (match == -1) return(rep(TRUE, nrow(cards)))
 
  starts <- attr(match, "capture.start")
  lengths <- attr(match, "capture.length")
 
  field <- substring(expr, starts[1], starts[1] + lengths[1] - 1)
  op <- substring(expr, starts[2], starts[2] + lengths[2] - 1)
  value <- as.numeric(substring(expr, starts[3], starts[3] + lengths[3] - 1))
 
  col_name <- switch(field,
    "ivl" = "ivl",
    "interval" = "ivl",
    "lapses" = "lapses",
    "reps" = "reps",
    "due" = "due",
    NULL
  )
 
  if (is.null(col_name) || !col_name %in% names(cards)) {
    return(rep(TRUE, nrow(cards)))
  }
 
  col_values <- cards[[col_name]]
 
  switch(op,
    ">" = col_values > value,
    ">=" = col_values >= value,
    "<" = col_values < value,
    "<=" = col_values <= value,
    "=" = col_values == value,
    "==" = col_values == value,
    rep(TRUE, nrow(cards))
  )
}

#' Get suspended cards
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param include_notes If TRUE, join with note data
#' @return A tibble of suspended cards
#' @export
#' @examples
#' \dontrun{
#' suspended <- anki_suspended()
#' }
anki_suspended <- function(path = NULL, profile = NULL, include_notes = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  cards <- cards[cards$queue == -1, ]
 
  if (include_notes && nrow(cards) > 0) {
    notes <- col$notes()
    cards <- merge(cards, notes, by = "nid", all.x = TRUE)
  }
 
  tibble::as_tibble(cards)
}

#' Get buried cards
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param include_notes If TRUE, join with note data
#' @return A tibble of buried cards
#' @export
#' @examples
#' \dontrun{
#' buried <- anki_buried()
#' }
anki_buried <- function(path = NULL, profile = NULL, include_notes = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  # -2 = buried manually, -3 = buried by sibling
  cards <- cards[cards$queue == -2 | cards$queue == -3, ]
 
  if (include_notes && nrow(cards) > 0) {
    notes <- col$notes()
    cards <- merge(cards, notes, by = "nid", all.x = TRUE)
  }
 
  tibble::as_tibble(cards)
}

#' Find leech cards (high lapse count)
#'
#' Leeches are cards that you keep forgetting. By default, Anki marks cards
#' as leeches after 8 lapses.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param threshold Minimum number of lapses to consider a leech (default 8)
#' @param include_notes If TRUE, join with note data
#' @return A tibble of leech cards ordered by lapses
#' @export
#' @examples
#' \dontrun{
#' leeches <- anki_leeches()
#' leeches <- anki_leeches(threshold = 5)
#' }
anki_leeches <- function(path = NULL, profile = NULL, threshold = 8, include_notes = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  cards <- cards[cards$lapses >= threshold, ]
  cards <- cards[order(-cards$lapses), ]
 
  if (include_notes && nrow(cards) > 0) {
    notes <- col$notes()
    decks <- col$decks()
    cards <- merge(cards, notes, by = "nid", all.x = TRUE)
    cards <- merge(cards, decks, by = "did", all.x = TRUE)
  }
 
  tibble::as_tibble(cards)
}

#' Get cards due for review
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param days_ahead Number of days to look ahead (default 0 = today only)
#' @return A tibble of due cards
#' @export
#' @examples
#' \dontrun{
#' due_today <- anki_due()
#' due_week <- anki_due(days_ahead = 7)
#' }
anki_due <- function(path = NULL, profile = NULL, days_ahead = 0) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
 
  # Review cards: due is day number since collection creation
  # For simplicity, filter by queue = 2 (review queue) and approximate
  today <- as.integer(Sys.Date())
  cutoff <- today + days_ahead
 
  review_due <- cards$queue == 2 & cards$due <= cutoff
  learning_due <- cards$queue %in% c(1, 3)  # Learning cards
  new_cards <- cards$queue == 0  # New cards always "due"
 
  cards <- cards[review_due | learning_due, ]
  cards <- cards[order(cards$due), ]
 
  tibble::as_tibble(cards)
}

#' Get new cards (never reviewed)
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param deck Optional deck name to filter
#' @return A tibble of new cards
#' @export
#' @examples
#' \dontrun{
#' new_cards <- anki_new()
#' }
anki_new <- function(path = NULL, profile = NULL, deck = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  cards <- cards[cards$type == 0, ]
 
  if (!is.null(deck)) {
    decks <- col$decks()
    deck_ids <- decks$did[grepl(deck, decks$name, ignore.case = TRUE)]
    cards <- cards[cards$did %in% deck_ids, ]
  }
 
  tibble::as_tibble(cards)
}

#' Get mature cards (interval >= 21 days)
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param min_interval Minimum interval to consider mature (default 21)
#' @return A tibble of mature cards
#' @export
#' @examples
#' \dontrun{
#' mature <- anki_mature()
#' }
anki_mature <- function(path = NULL, profile = NULL, min_interval = 21) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  cards <- col$cards()
  cards <- cards[cards$ivl >= min_interval, ]
  cards <- cards[order(-cards$ivl), ]
 
  tibble::as_tibble(cards)
}
