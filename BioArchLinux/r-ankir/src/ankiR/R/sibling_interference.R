#' Analyze sibling card effects
#'
#' Analyzes how sibling cards (cards from the same note) affect each other's
#' retention. For example, does seeing the forward card help with the reverse?
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param max_gap_days Maximum days between sibling reviews to consider related (default 7)
#' @return A list with sibling analysis
#' @export
#' @examples
#' \dontrun{
#' siblings <- anki_sibling_analysis()
#' }
anki_sibling_analysis <- function(path = NULL, profile = NULL, max_gap_days = 7) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()

  # Find notes with multiple cards (siblings)
  card_counts <- table(cards$nid)
  sibling_nids <- as.numeric(names(card_counts)[card_counts > 1])

  if (length(sibling_nids) == 0) {
    message("No sibling cards found (all notes have only one card)")
    return(NULL)
  }

  # Get sibling cards
  sibling_cards <- cards[cards$nid %in% sibling_nids, ]

  # Add card ordinal (which card template)
  sibling_cards <- sibling_cards[order(sibling_cards$nid, sibling_cards$cid), ]
  sibling_cards$card_ord <- ave(seq_len(nrow(sibling_cards)),
                                 sibling_cards$nid, FUN = seq_along)

  # Get reviews for sibling cards
  sibling_revlog <- revlog[revlog$cid %in% sibling_cards$cid, ]
  sibling_revlog <- merge(sibling_revlog,
                          sibling_cards[, c("cid", "nid", "card_ord")],
                          by = "cid", all.x = TRUE)

  if (nrow(sibling_revlog) < 100) {
    message("Not enough sibling review data")
    return(NULL)
  }

  # Analyze: when card 1 is reviewed, how does card 2 perform shortly after?
  # Group by note
  sibling_split <- split(sibling_revlog, sibling_revlog$nid)

  effects <- lapply(sibling_split, function(note_revs) {
    if (length(unique(note_revs$card_ord)) < 2) return(NULL)

    # Separate by card ordinal
    card1_revs <- note_revs[note_revs$card_ord == 1, ]
    card2_revs <- note_revs[note_revs$card_ord == 2, ]

    if (nrow(card1_revs) < 2 || nrow(card2_revs) < 2) return(NULL)

    # For each card2 review, check if card1 was reviewed recently
    card2_revs$card1_seen_recently <- vapply(card2_revs$review_date, function(d) {
      any(card1_revs$review_date >= (d - max_gap_days) &
          card1_revs$review_date < d)
    }, FUN.VALUE = logical(1))

    # Compare retention
    with_recent <- card2_revs[card2_revs$card1_seen_recently, ]
    without_recent <- card2_revs[!card2_revs$card1_seen_recently, ]

    if (nrow(with_recent) < 3 || nrow(without_recent) < 3) return(NULL)

    data.frame(
      nid = note_revs$nid[1],
      card2_reviews_with_recent_card1 = nrow(with_recent),
      card2_reviews_without_recent_card1 = nrow(without_recent),
      retention_with = 1 - sum(with_recent$ease == 1) / nrow(with_recent),
      retention_without = 1 - sum(without_recent$ease == 1) / nrow(without_recent)
    )
  })

  effects_df <- do.call(rbind, Filter(Negate(is.null), effects))

  if (is.null(effects_df) || nrow(effects_df) < 10) {
    message("Not enough data for sibling effect analysis")
    return(NULL)
  }

  effects_df$benefit <- effects_df$retention_with - effects_df$retention_without

  # Summary statistics
  avg_benefit <- mean(effects_df$benefit, na.rm = TRUE)
  notes_with_benefit <- sum(effects_df$benefit > 0, na.rm = TRUE)
  notes_with_harm <- sum(effects_df$benefit < 0, na.rm = TRUE)

  # Statistical test
  t_test <- tryCatch({
    stats::t.test(effects_df$retention_with, effects_df$retention_without, paired = TRUE)
  }, error = function(e) NULL)

  summary_stats <- tibble::tibble(
    metric = c("Notes with Siblings Analyzed", "Avg Retention Benefit",
               "Notes Where Sibling Helps", "Notes Where Sibling Hurts",
               "Statistical Significance"),
    value = c(
      nrow(effects_df),
      round(avg_benefit * 100, 2),
      notes_with_benefit,
      notes_with_harm,
      if (!is.null(t_test)) round(t_test$p.value, 4) else NA
    )
  )

  interpretation <- if (avg_benefit > 0.02) {
    paste0("Seeing sibling cards within ", max_gap_days, " days helps retention by ~",
           round(avg_benefit * 100, 1), " percentage points on average.")
  } else if (avg_benefit < -0.02) {
    paste0("Sibling cards may be causing interference. Consider spacing them more.")
  } else {
    "Sibling cards have minimal effect on each other's retention."
  }

  list(
    summary = summary_stats,
    details = tibble::as_tibble(effects_df),
    interpretation = interpretation,
    recommendation = if (avg_benefit > 0.02) {
      "Consider reviewing sibling cards in the same session for reinforcement."
    } else if (avg_benefit < -0.02) {
      "Consider using the 'bury siblings' option to prevent interference."
    } else {
      "Current sibling handling is working well."
    }
  )
}

#' Detect card interference
#'
#' Find cards that are often confused with each other, based on similar failure
#' patterns or content similarity combined with poor retention.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param min_lapses Minimum lapses for a card to be considered (default 3)
#' @param time_window_days Window to look for related failures (default 7)
#' @param max_pairs Maximum pairs to return (default 50)
#' @return A tibble of potentially interfering card pairs
#' @export
#' @examples
#' \dontrun{
#' interference <- anki_interference_analysis()
#' }
anki_interference_analysis <- function(path = NULL, profile = NULL,
                                        min_lapses = 3, time_window_days = 7,
                                        max_pairs = 50) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()

  # Focus on cards with lapses
  problem_cards <- cards[cards$lapses >= min_lapses, ]

  if (nrow(problem_cards) < 2) {
    message("Not enough problem cards (cards with >= ", min_lapses, " lapses)")
    return(tibble::tibble())
  }

  # Get failure reviews
  failure_revlog <- revlog[revlog$ease == 1 & revlog$cid %in% problem_cards$cid, ]

  if (nrow(failure_revlog) < 10) {
    message("Not enough failure data")
    return(tibble::tibble())
  }

  # Find cards that fail close together in time
  # This suggests they might be confused
  failure_revlog <- failure_revlog[order(failure_revlog$rid), ]

  # For efficiency, sample if too many
  if (nrow(failure_revlog) > 5000) {
    failure_revlog <- failure_revlog[sample(nrow(failure_revlog), 5000), ]
    failure_revlog <- failure_revlog[order(failure_revlog$rid), ]
  }

  # Find co-failures within time window
  co_failures <- list()
  k <- 1

  unique_dates <- unique(failure_revlog$review_date)

  for (date in unique_dates) {
    window_start <- date - time_window_days
    window_failures <- failure_revlog[
      failure_revlog$review_date >= window_start &
      failure_revlog$review_date <= date,
    ]

    if (nrow(window_failures) < 2) next

    cards_in_window <- unique(window_failures$cid)
    if (length(cards_in_window) < 2) next

    # Create pairs
    for (i in 1:(length(cards_in_window) - 1)) {
      for (j in (i + 1):length(cards_in_window)) {
        pair_key <- paste(sort(c(cards_in_window[i], cards_in_window[j])), collapse = "-")

        if (is.null(co_failures[[pair_key]])) {
          co_failures[[pair_key]] <- list(
            cid1 = cards_in_window[i],
            cid2 = cards_in_window[j],
            count = 0
          )
        }
        co_failures[[pair_key]]$count <- co_failures[[pair_key]]$count + 1
      }
    }
  }

  if (length(co_failures) == 0) {
    message("No co-failure patterns detected")
    return(tibble::tibble())
  }

  # Convert to data frame
  pairs_df <- do.call(rbind, lapply(co_failures, function(x) {
    data.frame(cid1 = x$cid1, cid2 = x$cid2, co_failure_count = x$count)
  }))

  # Filter to significant co-failures
  pairs_df <- pairs_df[pairs_df$co_failure_count >= 2, ]

  if (nrow(pairs_df) == 0) {
    message("No significant co-failure patterns")
    return(tibble::tibble())
  }

  # Add content similarity
  notes_content <- notes[, c("nid", "sfld", "flds")]
  cards_notes <- merge(cards[, c("cid", "nid")], notes_content, by = "nid")

  # Clean text
  cards_notes$clean_text <- gsub("<[^>]+>", "", cards_notes$sfld)
  cards_notes$clean_text <- gsub("\\[sound:[^\\]]+\\]", "", cards_notes$clean_text)
  cards_notes$clean_text <- tolower(trimws(cards_notes$clean_text))

  # Add content to pairs
  pairs_df <- merge(pairs_df,
                    cards_notes[, c("cid", "clean_text")],
                    by.x = "cid1", by.y = "cid", all.x = TRUE)
  names(pairs_df)[names(pairs_df) == "clean_text"] <- "text1"

  pairs_df <- merge(pairs_df,
                    cards_notes[, c("cid", "clean_text")],
                    by.x = "cid2", by.y = "cid", all.x = TRUE)
  names(pairs_df)[names(pairs_df) == "clean_text"] <- "text2"

  # Calculate text similarity
  pairs_df$text_similarity <- mapply(function(t1, t2) {
    if (is.na(t1) || is.na(t2)) return(0)
    words1 <- unique(unlist(strsplit(t1, "\\s+")))
    words2 <- unique(unlist(strsplit(t2, "\\s+")))
    if (length(words1) == 0 || length(words2) == 0) return(0)
    length(intersect(words1, words2)) / length(union(words1, words2))
  }, pairs_df$text1, pairs_df$text2)

  # Calculate interference score
  pairs_df$interference_score <- pairs_df$co_failure_count * (1 + pairs_df$text_similarity)

  # Sort and limit
  pairs_df <- pairs_df[order(-pairs_df$interference_score), ]
  pairs_df <- head(pairs_df, max_pairs)

  # Truncate text for display
  pairs_df$text1 <- substr(pairs_df$text1, 1, 50)
  pairs_df$text2 <- substr(pairs_df$text2, 1, 50)

  tibble::as_tibble(pairs_df[, c("cid1", "cid2", "text1", "text2",
                                  "co_failure_count", "text_similarity",
                                  "interference_score")])
}

#' Find weak areas by tag or deck
#'
#' Identifies tags or subdecks with the lowest retention or highest lapse rates.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param n Number of weak areas to return (default 10)
#' @param by Analysis type: "tag" or "deck" (default "tag")
#' @param days Number of days to analyze (default 90)
#' @return A tibble of weak areas
#' @export
#' @examples
#' \dontrun{
#' weak <- anki_weak_areas(n = 10, by = "tag")
#' }
anki_weak_areas <- function(path = NULL, profile = NULL, n = 10,
                            by = "tag", days = 90) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()
  decks <- col$decks()

  # Filter recent reviews
  cutoff <- Sys.Date() - days
  revlog <- revlog[revlog$review_date >= cutoff, ]

  if (nrow(revlog) == 0) {
    message("No reviews in the specified period")
    return(tibble::tibble())
  }

  if (by == "tag") {
    # Join with tags
    cards_notes <- merge(cards[, c("cid", "nid")], notes[, c("nid", "tags")], by = "nid")
    revlog_tags <- merge(revlog, cards_notes[, c("cid", "tags")], by = "cid", all.x = TRUE)

    # Extract all tags
    all_tags <- unique(unlist(strsplit(notes$tags, "\\s+")))
    all_tags <- all_tags[all_tags != ""]

    if (length(all_tags) == 0) {
      message("No tags found")
      return(tibble::tibble())
    }

    # Calculate stats per tag
    tag_stats <- lapply(all_tags, function(tag) {
      matching_revs <- revlog_tags[grepl(tag, revlog_tags$tags, fixed = TRUE), ]
      if (nrow(matching_revs) < 10) return(NULL)

      tibble::tibble(
        area = tag,
        reviews = nrow(matching_revs),
        retention = round((1 - sum(matching_revs$ease == 1) / nrow(matching_revs)) * 100, 1),
        again_rate = round(sum(matching_revs$ease == 1) / nrow(matching_revs) * 100, 1),
        avg_time_sec = round(mean(matching_revs$time, na.rm = TRUE) / 1000, 1)
      )
    })

    stats <- do.call(rbind, Filter(Negate(is.null), tag_stats))

  } else {
    # By deck
    cards_decks <- cards[, c("cid", "did")]
    revlog_decks <- merge(revlog, cards_decks, by = "cid", all.x = TRUE)
    revlog_decks <- merge(revlog_decks, decks[, c("did", "name")], by = "did", all.x = TRUE)

    deck_stats <- lapply(split(revlog_decks, revlog_decks$name), function(d) {
      if (nrow(d) < 10) return(NULL)

      tibble::tibble(
        area = as.character(d$name[1]),
        reviews = nrow(d),
        retention = round((1 - sum(d$ease == 1) / nrow(d)) * 100, 1),
        again_rate = round(sum(d$ease == 1) / nrow(d) * 100, 1),
        avg_time_sec = round(mean(d$time, na.rm = TRUE) / 1000, 1)
      )
    })

    stats <- do.call(rbind, Filter(Negate(is.null), deck_stats))
  }

  if (is.null(stats) || nrow(stats) == 0) {
    message("No areas with sufficient data")
    return(tibble::tibble())
  }

  # Calculate weakness score (lower retention = higher weakness)
  stats$weakness_score <- 100 - stats$retention + stats$again_rate * 0.5

  # Sort by weakness
  stats <- stats[order(-stats$weakness_score), ]

  # Return top n
  result <- head(stats, n)

  tibble::as_tibble(result)
}
