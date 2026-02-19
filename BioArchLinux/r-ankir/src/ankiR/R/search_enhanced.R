#' Enhanced Search for Anki Cards
#'
#' Advanced search functionality supporting additional search operators beyond
#' the basic `anki_search()`.
#'
#' @param pattern Search pattern with support for advanced operators
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param case_sensitive Whether search is case-sensitive (default FALSE)
#' @return A tibble with matching cards
#' @export
#'
#' @details
#' Supported search operators:
#' - `added:N` - Cards added in last N days
#' - `rated:N` - Cards rated in last N days  
#' - `rated:N:ease` - Cards rated with specific ease (1-4) in last N days
#' - `note:type` - Cards with specific note type name
#' - `card:N` - Card template number (0-indexed)
#' - `ivl:N` - Cards with interval >= N days
#' - `ivl:<N` - Cards with interval < N days
#' - `prop:ease>N` - Cards with ease factor > N (e.g., prop:ease>2.5)
#' - `prop:lapses>N` - Cards with lapses > N
#' - `prop:reps>N` - Cards with reps > N
#' - `re:pattern` - Regex search in card content
#' - `OR` - OR operator between terms
#' - Standard operators: `deck:`, `tag:`, `is:`, `flag:`
#'
#' @examples
#' \dontrun{
#' # Cards added in last 7 days
#' anki_search_enhanced("added:7")
#' 
#' # Cards rated "Again" in last 3 days
#' anki_search_enhanced("rated:3:1")
#' 
#' # Leeches with high lapses
#' anki_search_enhanced("is:leech prop:lapses>5")
#' 
#' # Regex search
#' anki_search_enhanced("re:^The\\s+")
#' 
#' # OR search
#' anki_search_enhanced("deck:German OR deck:Spanish")
#' }
anki_search_enhanced <- function(pattern, path = NULL, profile = NULL,
                                  case_sensitive = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
  models <- col$models()
  revlog <- col$revlog()
  
  # Join cards with notes and decks
  cards <- merge(cards, notes[, c("nid", "mid", "sfld", "flds", "tags")], 
                 by = "nid", all.x = TRUE)
  cards <- merge(cards, decks[, c("did", "name")], by = "did", all.x = TRUE)
  names(cards)[names(cards) == "name"] <- "deck_name"
  cards <- merge(cards, models[, c("mid", "name")], by = "mid", all.x = TRUE)
  names(cards)[names(cards) == "name"] <- "model_name"
  
  # Handle OR operator - split into sub-searches
  if (grepl("\\bOR\\b", pattern, ignore.case = TRUE)) {
    parts <- strsplit(pattern, "\\s+OR\\s+", perl = TRUE)[[1]]
    results <- lapply(parts, function(p) {
      tryCatch(
        search_single_pattern(p, cards, revlog, case_sensitive),
        error = function(e) integer(0)
      )
    })
    matching_indices <- unique(unlist(results))
    return(tibble::as_tibble(cards[matching_indices, ]))
  }
  
  # Single pattern search
  matching_indices <- search_single_pattern(pattern, cards, revlog, case_sensitive)
  tibble::as_tibble(cards[matching_indices, ])
}

#' Internal function to search a single pattern
#' @noRd
search_single_pattern <- function(pattern, cards, revlog, case_sensitive) {
  # Parse operators from pattern
  operators <- parse_search_operators(pattern)
  
  # Start with all cards
  mask <- rep(TRUE, nrow(cards))
  
  for (op in operators) {
    mask <- mask & apply_operator(op, cards, revlog, case_sensitive)
  }
  
  which(mask)
}

#' Parse search operators from pattern string
#' @noRd
parse_search_operators <- function(pattern) {
  operators <- list()
  
  # Extract quoted strings first
  quoted_pattern <- '("[^"]+"|\'[^\']+\')'
  quoted_matches <- regmatches(pattern, gregexpr(quoted_pattern, pattern))[[1]]
  for (q in quoted_matches) {
    operators <- c(operators, list(list(type = "text", value = gsub('["\']', '', q))))
    pattern <- sub(q, "", pattern, fixed = TRUE)
  }
  
  # Operator patterns
  op_patterns <- list(
    added = "added:(\\d+)",
    rated = "rated:(\\d+)(?::(\\d+))?",
    note = "note:([^\\s]+)",
    card = "card:(\\d+)",
    ivl_gte = "ivl:(\\d+)",
    ivl_lt = "ivl:<(\\d+)",
    prop_ease = "prop:ease([<>]=?)(\\d+\\.?\\d*)",
    prop_lapses = "prop:lapses([<>]=?)(\\d+)",
    prop_reps = "prop:reps([<>]=?)(\\d+)",
    regex = "re:([^\\s]+)",
    deck = "deck:([^\\s]+)",
    tag = "tag:([^\\s]+)",
    is = "is:(\\w+)",
    flag = "flag:(\\d+)"
  )
  
  for (name in names(op_patterns)) {
    matches <- regmatches(pattern, regexec(op_patterns[[name]], pattern, perl = TRUE))[[1]]
    if (length(matches) > 0 && matches[1] != "") {
      operators <- c(operators, list(list(
        type = name,
        value = matches[-1]
      )))
      pattern <- sub(matches[1], "", pattern, fixed = TRUE)
    }
  }
  
  # Remaining text is free text search
  remaining <- trimws(pattern)
  if (nchar(remaining) > 0) {
    words <- strsplit(remaining, "\\s+")[[1]]
    for (word in words) {
      if (nchar(word) > 0) {
        operators <- c(operators, list(list(type = "text", value = word)))
      }
    }
  }
  
  operators
}

#' Apply a single operator to filter cards
#' @noRd
apply_operator <- function(op, cards, revlog, case_sensitive) {
  n <- nrow(cards)
  
  switch(op$type,
    "added" = {
      days <- as.integer(op$value[1])
      cutoff <- Sys.Date() - days
      cards$card_created_date >= cutoff
    },
    "rated" = {
      days <- as.integer(op$value[1])
      ease <- if (length(op$value) > 1 && !is.na(op$value[2])) as.integer(op$value[2]) else NULL
      cutoff <- Sys.Date() - days
      recent_revs <- revlog[revlog$review_date >= cutoff, ]
      if (!is.null(ease)) {
        recent_revs <- recent_revs[recent_revs$ease == ease, ]
      }
      cards$cid %in% recent_revs$cid
    },
    "note" = {
      pattern <- op$value[1]
      if (case_sensitive) {
        grepl(pattern, cards$model_name, fixed = TRUE)
      } else {
        grepl(tolower(pattern), tolower(cards$model_name), fixed = TRUE)
      }
    },
    "card" = {
      template <- as.integer(op$value[1])
      cards$ord == template
    },
    "ivl_gte" = {
      ivl <- as.integer(op$value[1])
      cards$ivl >= ivl
    },
    "ivl_lt" = {
      ivl <- as.integer(op$value[1])
      cards$ivl < ivl
    },
    "prop_ease" = {
      cmp <- op$value[1]
      val <- as.numeric(op$value[2])
      ease <- cards$factor / 1000
      switch(cmp,
        ">" = ease > val,
        ">=" = ease >= val,
        "<" = ease < val,
        "<=" = ease <= val,
        rep(TRUE, n)
      )
    },
    "prop_lapses" = {
      cmp <- op$value[1]
      val <- as.integer(op$value[2])
      switch(cmp,
        ">" = cards$lapses > val,
        ">=" = cards$lapses >= val,
        "<" = cards$lapses < val,
        "<=" = cards$lapses <= val,
        rep(TRUE, n)
      )
    },
    "prop_reps" = {
      cmp <- op$value[1]
      val <- as.integer(op$value[2])
      switch(cmp,
        ">" = cards$reps > val,
        ">=" = cards$reps >= val,
        "<" = cards$reps < val,
        "<=" = cards$reps <= val,
        rep(TRUE, n)
      )
    },
    "regex" = {
      pattern <- op$value[1]
      if (case_sensitive) {
        grepl(pattern, cards$flds, perl = TRUE) | grepl(pattern, cards$sfld, perl = TRUE)
      } else {
        grepl(pattern, cards$flds, perl = TRUE, ignore.case = TRUE) | 
          grepl(pattern, cards$sfld, perl = TRUE, ignore.case = TRUE)
      }
    },
    "deck" = {
      pattern <- op$value[1]
      # Handle wildcards
      pattern <- gsub("\\*", ".*", pattern)
      if (case_sensitive) {
        grepl(paste0("^", pattern), cards$deck_name, perl = TRUE)
      } else {
        grepl(paste0("^", pattern), cards$deck_name, perl = TRUE, ignore.case = TRUE)
      }
    },
    "tag" = {
      pattern <- op$value[1]
      pattern <- gsub("\\*", ".*", pattern)
      if (case_sensitive) {
        grepl(pattern, cards$tags, perl = TRUE)
      } else {
        grepl(pattern, cards$tags, perl = TRUE, ignore.case = TRUE)
      }
    },
    "is" = {
      state <- tolower(op$value[1])
      switch(state,
        "new" = cards$queue == 0,
        "learn" = cards$queue == 1 | cards$queue == 3,
        "review" = cards$queue == 2,
        "due" = cards$queue == 2 & cards$due <= as.numeric(Sys.Date() - as.Date("1970-01-01")),
        "suspended" = cards$queue == -1,
        "buried" = cards$queue == -2 | cards$queue == -3,
        "leech" = cards$lapses >= 8,
        rep(TRUE, n)
      )
    },
    "flag" = {
      flag <- as.integer(op$value[1])
      cards$flags == flag
    },
    "text" = {
      text <- op$value
      if (case_sensitive) {
        grepl(text, cards$flds, fixed = TRUE) | grepl(text, cards$sfld, fixed = TRUE)
      } else {
        grepl(tolower(text), tolower(cards$flds), fixed = TRUE) | 
          grepl(tolower(text), tolower(cards$sfld), fixed = TRUE)
      }
    },
    rep(TRUE, n)
  )
}

#' Search cards by content similarity
#'
#' Find cards with similar content to a given card or text.
#'
#' @param query Either a card ID or text to search for similar cards
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param n Number of similar cards to return (default 10)
#' @param method Similarity method: "tfidf", "jaccard", or "ngram"
#' @param within_deck Only search within same deck as query card
#' @return A tibble with similar cards and similarity scores
#' @export
#'
#' @examples
#' \dontrun{
#' # Find cards similar to card ID 1234567890
#' anki_find_similar(1234567890)
#' 
#' # Find cards similar to specific text
#' anki_find_similar("mitochondria powerhouse")
#' }
anki_find_similar <- function(query, path = NULL, profile = NULL,
                               n = 10, method = "tfidf", within_deck = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
  
  # Join data
  cards <- merge(cards, notes[, c("nid", "sfld", "flds")], by = "nid", all.x = TRUE)
  cards <- merge(cards, decks[, c("did", "name")], by = "did", all.x = TRUE)
  names(cards)[names(cards) == "name"] <- "deck_name"
  
  # Get query text
  if (is.numeric(query)) {
    query_card <- cards[cards$cid == query, ]
    if (nrow(query_card) == 0) {
      stop("Card ID not found: ", query)
    }
    query_text <- paste(query_card$sfld, query_card$flds, collapse = " ")
    query_deck <- query_card$did[1]
    exclude_cid <- query
  } else {
    query_text <- query
    query_deck <- NULL
    exclude_cid <- NULL
  }
  
  # Filter to same deck if requested
  if (within_deck && !is.null(query_deck)) {
    cards <- cards[cards$did == query_deck, ]
  }
  
  # Remove query card from candidates
  if (!is.null(exclude_cid)) {
    cards <- cards[cards$cid != exclude_cid, ]
  }
  
  if (nrow(cards) == 0) {
    message("No cards to compare")
    return(tibble::tibble())
  }
  
  # Create content column
  cards$content <- paste(cards$sfld, cards$flds)
  
  # Calculate similarity based on method
  similarities <- switch(method,
    "tfidf" = tfidf_similarity(query_text, cards$content),
    "jaccard" = jaccard_similarity(query_text, cards$content),
    "ngram" = ngram_similarity(query_text, cards$content),
    jaccard_similarity(query_text, cards$content)
  )
  
  cards$similarity <- similarities
  
  # Sort and return top n
  cards <- cards[order(-cards$similarity), ]
  cards <- head(cards, n)
  
  tibble::as_tibble(cards[, c("cid", "nid", "deck_name", "sfld", "similarity")])
}

#' TF-IDF based similarity calculation
#' @noRd
tfidf_similarity <- function(query, documents) {
  # Simple TF-IDF implementation
  all_docs <- c(query, documents)
  
  # Tokenize
  tokenize <- function(text) {
    text <- tolower(text)
    text <- gsub("[^a-z0-9\\s]", " ", text)
    words <- strsplit(text, "\\s+")[[1]]
    words[nchar(words) > 1]
  }
  
  query_tokens <- tokenize(query)
  doc_tokens <- lapply(documents, tokenize)
  
  # Build vocabulary from query terms
  vocab <- unique(query_tokens)
  if (length(vocab) == 0) return(rep(0, length(documents)))
  
  # Calculate document frequency
  df <- sapply(vocab, function(term) {
    sum(sapply(doc_tokens, function(d) term %in% d))
  })
  
  # IDF
  n_docs <- length(documents)
  idf <- log(n_docs / (df + 1)) + 1
  
  # Query TF-IDF vector
  query_tf <- sapply(vocab, function(term) sum(query_tokens == term))
  query_tfidf <- query_tf * idf
  query_norm <- sqrt(sum(query_tfidf^2))
  if (query_norm == 0) return(rep(0, length(documents)))
  
  # Document similarities
  sapply(doc_tokens, function(doc) {
    doc_tf <- sapply(vocab, function(term) sum(doc == term))
    doc_tfidf <- doc_tf * idf
    doc_norm <- sqrt(sum(doc_tfidf^2))
    if (doc_norm == 0) return(0)
    sum(query_tfidf * doc_tfidf) / (query_norm * doc_norm)
  })
}

#' Jaccard similarity calculation
#' @noRd
jaccard_similarity <- function(query, documents) {
  tokenize <- function(text) {
    text <- tolower(text)
    text <- gsub("[^a-z0-9\\s]", " ", text)
    unique(strsplit(text, "\\s+")[[1]])
  }
  
  query_tokens <- tokenize(query)
  
  sapply(documents, function(doc) {
    doc_tokens <- tokenize(doc)
    intersection <- length(intersect(query_tokens, doc_tokens))
    union <- length(union(query_tokens, doc_tokens))
    if (union == 0) return(0)
    intersection / union
  })
}

#' N-gram similarity calculation
#' @noRd
ngram_similarity <- function(query, documents, n = 3) {
  get_ngrams <- function(text, n) {
    text <- tolower(gsub("[^a-z0-9]", "", text))
    if (nchar(text) < n) return(character(0))
    sapply(1:(nchar(text) - n + 1), function(i) substr(text, i, i + n - 1))
  }
  
  query_ngrams <- get_ngrams(query, n)
  
  sapply(documents, function(doc) {
    doc_ngrams <- get_ngrams(doc, n)
    intersection <- length(intersect(query_ngrams, doc_ngrams))
    union <- length(union(query_ngrams, doc_ngrams))
    if (union == 0) return(0)
    intersection / union
  })
}
