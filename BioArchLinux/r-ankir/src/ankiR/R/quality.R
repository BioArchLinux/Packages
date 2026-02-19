#' Analyze card complexity
#'
#' Measures card complexity based on field content, media usage, etc.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with card complexity metrics
#' @export
#' @examples
#' \dontrun{
#' complexity <- anki_card_complexity()
#' }
anki_card_complexity <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  notes <- col$notes()
  cards <- col$cards()
  decks <- col$decks()
 
  # Calculate note complexity
  notes$field_count <- lengths(strsplit(notes$flds, "\x1f"))
  notes$total_length <- nchar(notes$flds)
  notes$has_image <- grepl("<img", notes$flds, ignore.case = TRUE)
  notes$has_sound <- grepl("\\[sound:", notes$flds)
  notes$has_cloze <- grepl("\\{\\{c\\d+::", notes$flds)
  notes$has_mathjax <- grepl("\\\\\\(|\\\\\\[|\\$\\$", notes$flds)
  notes$html_tag_count <- lengths(regmatches(notes$flds, gregexpr("<[^>]+>", notes$flds)))
  notes$word_count <- lengths(strsplit(gsub("<[^>]+>", "", notes$flds), "\\s+"))
 
  # Calculate complexity score
  notes$complexity_score <- with(notes,
    log1p(total_length) * 0.2 +
    field_count * 0.1 +
    has_image * 2 +
    has_sound * 2 +
    has_cloze * 1 +
    has_mathjax * 1.5 +
    log1p(html_tag_count) * 0.3 +
    log1p(word_count) * 0.4
  )
 
  # Join with cards for deck info
  result <- merge(cards[, c("cid", "nid", "did")], notes, by = "nid", all.x = TRUE)
  result <- merge(result, decks, by = "did", all.x = TRUE)
 
  # Summary by deck
  summary <- lapply(split(result, result$name), function(d) {
    tibble::tibble(
      cards = nrow(d),
      avg_complexity = round(mean(d$complexity_score, na.rm = TRUE), 2),
      avg_length = round(mean(d$total_length, na.rm = TRUE)),
      avg_words = round(mean(d$word_count, na.rm = TRUE)),
      pct_with_image = round(mean(d$has_image) * 100, 1),
      pct_with_sound = round(mean(d$has_sound) * 100, 1),
      pct_cloze = round(mean(d$has_cloze) * 100, 1),
      pct_mathjax = round(mean(d$has_mathjax) * 100, 1)
    )
  })
 
  result_summary <- do.call(rbind, summary)
  result_summary$deck <- names(summary)
  result_summary <- result_summary[order(-result_summary$avg_complexity), ]
 
  tibble::as_tibble(result_summary[, c("deck", "cards", "avg_complexity", "avg_length",
                                       "avg_words", "pct_with_image", "pct_with_sound",
                                       "pct_cloze", "pct_mathjax")])
}

#' Find similar/duplicate cards
#'
#' Uses text similarity to find potential duplicate cards.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param threshold Similarity threshold (0-1, default 0.8)
#' @param max_comparisons Maximum number of comparisons (default 50000)
#' @return A tibble of similar card pairs
#' @export
#' @examples
#' \dontrun{
#' dupes <- anki_similar_cards(threshold = 0.9)
#' }
anki_similar_cards <- function(path = NULL, profile = NULL, threshold = 0.8,
                               max_comparisons = 50000) {
  notes <- anki_notes(path, profile)
 
  if (nrow(notes) < 2) {
    return(tibble::tibble())
  }
 
  # Clean text for comparison
  notes$clean_text <- gsub("<[^>]+>", "", notes$flds)  # Remove HTML
  notes$clean_text <- gsub("\\[sound:[^\\]]+\\]", "", notes$clean_text)  # Remove sound
  notes$clean_text <- tolower(trimws(notes$clean_text))
 
  # Sample if too many comparisons
  n <- nrow(notes)
  n_comparisons <- n * (n - 1) / 2
 
  if (n_comparisons > max_comparisons) {
    sample_size <- floor(sqrt(max_comparisons * 2))
    message("Sampling ", sample_size, " notes for comparison (", n, " total)")
    idx <- sample(n, sample_size)
    notes <- notes[idx, ]
    n <- nrow(notes)
  }
 
  # Simple Jaccard similarity on word sets
  word_sets <- lapply(notes$clean_text, function(x) {
    unique(unlist(strsplit(x, "\\s+")))
  })
 
  similar_pairs <- list()
  k <- 1
 
  for (i in 1:(n-1)) {
    for (j in (i+1):n) {
      set_i <- word_sets[[i]]
      set_j <- word_sets[[j]]
     
      if (length(set_i) == 0 || length(set_j) == 0) next
     
      intersection <- length(intersect(set_i, set_j))
      union <- length(union(set_i, set_j))
      similarity <- intersection / union
     
      if (similarity >= threshold) {
        similar_pairs[[k]] <- tibble::tibble(
          nid1 = notes$nid[i],
          nid2 = notes$nid[j],
          similarity = round(similarity, 3),
          text1 = substr(notes$clean_text[i], 1, 100),
          text2 = substr(notes$clean_text[j], 1, 100)
        )
        k <- k + 1
      }
    }
  }
 
  if (length(similar_pairs) == 0) {
    message("No similar cards found above threshold ", threshold)
    return(tibble::tibble())
  }
 
  result <- do.call(rbind, similar_pairs)
  result <- result[order(-result$similarity), ]
 
  tibble::as_tibble(result)
}

#' Analyze tag usage
#'
#' Provides detailed tag statistics and identifies orphan/unused tags.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with tag analysis
#' @export
#' @examples
#' \dontrun{
#' tags <- anki_tag_analysis()
#' }
anki_tag_analysis <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  notes <- col$notes()
  revlog <- col$revlog()
  cards <- col$cards()
 
  # Get all tags
  all_tags <- unlist(strsplit(notes$tags, "\\s+"))
  all_tags <- all_tags[all_tags != ""]
 
  if (length(all_tags) == 0) {
    return(list(
      total_tags = 0,
      tag_counts = tibble::tibble(),
      orphan_tags = character(),
      notes_without_tags = sum(notes$tags == "" | is.na(notes$tags))
    ))
  }
 
  # Tag counts
  tag_counts <- as.data.frame(table(all_tags), stringsAsFactors = FALSE)
  names(tag_counts) <- c("tag", "count")
  tag_counts <- tag_counts[order(-tag_counts$count), ]
 
  # Hierarchical analysis
  tag_counts$depth <- lengths(strsplit(tag_counts$tag, "::"))
  tag_counts$parent <- vapply(tag_counts$tag, function(t) {
    parts <- strsplit(t, "::")[[1]]
    if (length(parts) > 1) paste(parts[-length(parts)], collapse = "::") else ""
  }, FUN.VALUE = character(1))
 
  # Find orphan tags (parent doesn't exist)
  all_tags_unique <- unique(all_tags)
  orphan_tags <- character()
  for (tag in all_tags_unique) {
    parts <- strsplit(tag, "::")[[1]]
    if (length(parts) > 1) {
      parent <- paste(parts[-length(parts)], collapse = "::")
      # Check if any note has this parent tag
      if (!parent %in% all_tags_unique) {
        orphan_tags <- c(orphan_tags, tag)
      }
    }
  }
 
  # Join tags with performance
  notes_with_tags <- notes[notes$tags != "" & !is.na(notes$tags), ]
  cards_with_notes <- merge(cards, notes_with_tags[, c("nid", "tags")], by = "nid")
  revlog_with_tags <- merge(revlog, cards_with_notes[, c("cid", "tags")], by = "cid")
 
  # Calculate retention by tag (top 20)
  top_tags <- head(tag_counts$tag, 20)
  tag_retention <- lapply(top_tags, function(t) {
    matching_revs <- revlog_with_tags[grepl(t, revlog_with_tags$tags, fixed = TRUE), ]
    if (nrow(matching_revs) > 0) {
      tibble::tibble(
        tag = t,
        reviews = nrow(matching_revs),
        retention = round((1 - sum(matching_revs$ease == 1) / nrow(matching_revs)) * 100, 1)
      )
    } else {
      NULL
    }
  })
  tag_retention <- do.call(rbind, Filter(Negate(is.null), tag_retention))
 
  list(
    total_tags = length(unique(all_tags)),
    tag_counts = tibble::as_tibble(tag_counts),
    orphan_tags = orphan_tags,
    notes_without_tags = sum(notes$tags == "" | is.na(notes$tags)),
    notes_with_tags = sum(notes$tags != "" & !is.na(notes$tags)),
    avg_tags_per_note = round(length(all_tags) / nrow(notes), 2),
    tag_retention = if (!is.null(tag_retention)) tibble::as_tibble(tag_retention) else tibble::tibble()
  )
}

#' Find cards with empty fields
#'
#' Identifies cards that have empty required fields.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of cards with empty fields
#' @export
#' @examples
#' \dontrun{
#' empty <- anki_empty_cards()
#' }
anki_empty_cards <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  notes <- col$notes()
  cards <- col$cards()
  models <- col$models()
  decks <- col$decks()
 
  # Check for empty fields
  notes$field_list <- strsplit(notes$flds, "\x1f")
 
  # Find notes with empty fields
  notes$has_empty <- vapply(notes$field_list, function(fields) {
    any(trimws(fields) == "" | is.na(fields))
  }, FUN.VALUE = logical(1))
 
  notes$empty_count <- vapply(notes$field_list, function(fields) {
    sum(trimws(fields) == "" | is.na(fields))
  }, FUN.VALUE = integer(1))
 
  notes$total_fields <- vapply(notes$field_list, length, FUN.VALUE = integer(1))
 
  # Filter to notes with empty fields
  empty_notes <- notes[notes$has_empty, c("nid", "mid", "sfld", "empty_count", "total_fields")]
 
  if (nrow(empty_notes) == 0) {
    message("No cards with empty fields found")
    return(tibble::tibble())
  }
 
  # Join with cards and decks
  result <- merge(cards[, c("cid", "nid", "did")], empty_notes, by = "nid")
  result <- merge(result, decks[, c("did", "name")], by = "did", all.x = TRUE)
  result <- merge(result, models[, c("mid", "name")], by = "mid", all.x = TRUE,
                  suffixes = c(".deck", ".model"))
 
  result <- result[order(-result$empty_count), ]
 
  tibble::as_tibble(result[, c("cid", "nid", "name.deck", "name.model", "sfld",
                               "empty_count", "total_fields")])
}

#' Find cards with very long content
#'
#' Identifies cards that might be too complex.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param threshold Character count threshold (default 2000)
#' @return A tibble of long cards
#' @export
#' @examples
#' \dontrun{
#' long_cards <- anki_long_cards()
#' }
anki_long_cards <- function(path = NULL, profile = NULL, threshold = 2000) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  notes <- col$notes()
  cards <- col$cards()
  decks <- col$decks()
 
  notes$char_count <- nchar(notes$flds)
  notes$word_count <- lengths(strsplit(gsub("<[^>]+>", "", notes$flds), "\\s+"))
 
  # Filter long notes
  long_notes <- notes[notes$char_count >= threshold, ]
 
  if (nrow(long_notes) == 0) {
    message("No cards found above threshold ", threshold, " characters")
    return(tibble::tibble())
  }
 
  # Join with cards and decks
  result <- merge(cards[, c("cid", "nid", "did")], long_notes[, c("nid", "sfld", "char_count", "word_count")], by = "nid")
  result <- merge(result, decks[, c("did", "name")], by = "did", all.x = TRUE)
 
  result <- result[order(-result$char_count), ]
 
  tibble::as_tibble(result[, c("cid", "nid", "name", "sfld", "char_count", "word_count")])
}

#' Analyze card quality metrics
#'
#' Comprehensive quality analysis combining multiple metrics.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with quality metrics
#' @export
#' @examples
#' \dontrun{
#' quality <- anki_quality_report()
#' }
anki_quality_report <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
 
  notes <- col$notes()
  cards <- col$cards()
 
  # Calculate metrics
  notes$char_count <- nchar(notes$flds)
  notes$word_count <- lengths(strsplit(gsub("<[^>]+>", "", notes$flds), "\\s+"))
  notes$has_image <- grepl("<img", notes$flds, ignore.case = TRUE)
  notes$has_sound <- grepl("\\[sound:", notes$flds)
 
  # Check for empty fields
  notes$field_list <- strsplit(notes$flds, "\x1f")
  notes$has_empty <- vapply(notes$field_list, function(fields) {
    any(trimws(fields) == "" | is.na(fields))
  }, FUN.VALUE = logical(1))
 
  # Leech analysis
  leeches <- cards[cards$lapses >= 8, ]
  suspended <- cards[cards$queue == -1, ]
 
  list(
    overview = tibble::tibble(
      metric = c("Total Notes", "Total Cards", "Unique Tags", "Notes with Empty Fields",
                 "Very Long Notes (>2000 chars)", "Notes with Media", "Leech Cards",
                 "Suspended Cards"),
      value = c(
        nrow(notes),
        nrow(cards),
        length(unique(unlist(strsplit(notes$tags, "\\s+")))),
        sum(notes$has_empty),
        sum(notes$char_count > 2000),
        sum(notes$has_image | notes$has_sound),
        nrow(leeches),
        nrow(suspended)
      )
    ),
    content_stats = tibble::tibble(
      metric = c("Avg Characters", "Avg Words", "% with Images", "% with Sound",
                 "% with Empty Fields"),
      value = c(
        round(mean(notes$char_count)),
        round(mean(notes$word_count)),
        round(mean(notes$has_image) * 100, 1),
        round(mean(notes$has_sound) * 100, 1),
        round(mean(notes$has_empty) * 100, 1)
      )
    ),
    recommendations = generate_quality_recommendations(notes, cards)
  )
}

#' @keywords internal
generate_quality_recommendations <- function(notes, cards) {
  recs <- character()
 
  # Check for issues
  empty_pct <- mean(vapply(strsplit(notes$flds, "\x1f"), function(fields) {
    any(trimws(fields) == "" | is.na(fields))
  }, FUN.VALUE = logical(1)))
 
  if (empty_pct > 0.1) {
    recs <- c(recs, paste0("! ", round(empty_pct * 100, 1), "% of notes have empty fields"))
  }
 
  leech_pct <- sum(cards$lapses >= 8) / nrow(cards)
  if (leech_pct > 0.05) {
    recs <- c(recs, paste0("! ", round(leech_pct * 100, 1), "% of cards are leeches (8+ lapses)"))
  }
 
  suspended_pct <- sum(cards$queue == -1) / nrow(cards)
  if (suspended_pct > 0.1) {
    recs <- c(recs, paste0("INFO: ", round(suspended_pct * 100, 1), "% of cards are suspended"))
  }
 
  very_long <- sum(nchar(notes$flds) > 2000) / nrow(notes)
  if (very_long > 0.05) {
    recs <- c(recs, paste0("! ", round(very_long * 100, 1), "% of notes are very long (>2000 chars)"))
  }
 
  media_pct <- mean(grepl("<img|\\[sound:", notes$flds))
  if (media_pct < 0.1) {
    recs <- c(recs, "TIP: Consider adding more images/audio for better retention")
  }
 
  if (length(recs) == 0) {
    recs <- "OK No major quality issues detected"
  }
 
  recs
}
