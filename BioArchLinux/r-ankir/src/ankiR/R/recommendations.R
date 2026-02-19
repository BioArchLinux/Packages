#' Generate card recommendations
#'
#' Generates actionable recommendations for improving your collection:
#' cards to unsuspend, leeches to rewrite, near-duplicates to merge, etc.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with categorized recommendations
#' @export
#' @examples
#' \dontrun{
#' recs <- anki_card_recommendations()
#' recs$leeches_to_rewrite
#' recs$cards_to_unsuspend
#' }
anki_card_recommendations <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()
  decks <- col$decks()
  cards_fsrs <- anki_cards_fsrs(path = col$path)

  # Join data
  cards_full <- merge(cards, notes[, c("nid", "tags", "sfld", "flds")], by = "nid", all.x = TRUE)
  cards_full <- merge(cards_full, decks[, c("did", "name")], by = "did", all.x = TRUE)
  cards_full <- merge(cards_full, cards_fsrs[, c("cid", "stability", "difficulty")],
                      by = "cid", all.x = TRUE)

  # 1. Leeches to rewrite (high lapses, but still being reviewed)
  leeches <- cards_full[cards_full$lapses >= 8 & cards_full$queue != -1, ]
  leeches <- leeches[order(-leeches$lapses), ]

  if (nrow(leeches) > 0) {
    leeches_rec <- head(leeches[, c("cid", "sfld", "name", "lapses", "difficulty")], 20)
    leeches_rec$recommendation <- "Rewrite or add mnemonics"
  } else {
    leeches_rec <- tibble::tibble()
  }

  # 2. Cards to potentially unsuspend (suspended but might be ready)
  suspended <- cards_full[cards_full$queue == -1, ]

  if (nrow(suspended) > 0) {
    # Check their history - did they improve before being suspended?
    susp_revlog <- revlog[revlog$cid %in% suspended$cid, ]

    # Get recent reviews (last 5 per card)
    susp_analysis <- lapply(split(susp_revlog, susp_revlog$cid), function(r) {
      if (nrow(r) < 3) return(NULL)
      r <- tail(r[order(r$rid), ], 5)
      recent_retention <- 1 - sum(r$ease == 1) / nrow(r)

      if (recent_retention >= 0.6) {  # Was doing okay
        cid <- r$cid[1]
        card_data <- suspended[suspended$cid == cid, ]
        data.frame(
          cid = cid,
          sfld = card_data$sfld[1],
          deck = card_data$name[1],
          last_retention = recent_retention,
          reviews = nrow(r)
        )
      } else {
        NULL
      }
    })

    unsuspend_rec <- do.call(rbind, Filter(Negate(is.null), susp_analysis))
    if (!is.null(unsuspend_rec)) {
      unsuspend_rec <- head(unsuspend_rec[order(-unsuspend_rec$last_retention), ], 20)
      unsuspend_rec$recommendation <- "Consider unsuspending - was doing well"
    } else {
      unsuspend_rec <- tibble::tibble()
    }
  } else {
    unsuspend_rec <- tibble::tibble()
  }

  # 3. Cards that might need more context (very short, high lapses)
  short_problem <- cards_full[nchar(cards_full$sfld) < 50 & cards_full$lapses >= 3, ]
  short_problem <- short_problem[order(-short_problem$lapses), ]

  if (nrow(short_problem) > 0) {
    short_rec <- head(short_problem[, c("cid", "sfld", "name", "lapses")], 20)
    short_rec$recommendation <- "Add context, image, or mnemonic"
  } else {
    short_rec <- tibble::tibble()
  }

  # 4. Easy cards that could be retired (very high stability, perfect retention)
  if (any(!is.na(cards_full$stability))) {
    easy_cards <- cards_full[!is.na(cards_full$stability) &
                              cards_full$stability > 365 &
                              cards_full$lapses == 0 &
                              cards_full$reps >= 10, ]
    easy_cards <- easy_cards[order(-easy_cards$stability), ]

    if (nrow(easy_cards) > 0) {
      easy_rec <- head(easy_cards[, c("cid", "sfld", "name", "stability", "reps")], 20)
      easy_rec$recommendation <- "Consider suspending - well learned"
    } else {
      easy_rec <- tibble::tibble()
    }
  } else {
    easy_rec <- tibble::tibble()
  }

  # 5. Tags to consolidate (similar tags)
  all_tags <- unique(unlist(strsplit(notes$tags, "\\s+")))
  all_tags <- all_tags[all_tags != ""]

  if (length(all_tags) > 1) {
    tag_pairs <- list()
    k <- 1

    for (i in 1:(length(all_tags) - 1)) {
      for (j in (i + 1):min(length(all_tags), i + 100)) {  # Limit comparisons
        # Check for similar tags
        t1 <- tolower(all_tags[i])
        t2 <- tolower(all_tags[j])

        # Levenshtein-like similarity
        if (nchar(t1) > 3 && nchar(t2) > 3) {
          # Check if one is substring of other
          if (grepl(t1, t2, fixed = TRUE) || grepl(t2, t1, fixed = TRUE)) {
            tag_pairs[[k]] <- data.frame(
              tag1 = all_tags[i],
              tag2 = all_tags[j],
              reason = "One is substring of other"
            )
            k <- k + 1
          }
        }
      }
    }

    if (length(tag_pairs) > 0) {
      tag_rec <- do.call(rbind, tag_pairs)
      tag_rec <- head(tag_rec, 20)
      tag_rec$recommendation <- "Consider merging tags"
    } else {
      tag_rec <- tibble::tibble()
    }
  } else {
    tag_rec <- tibble::tibble()
  }

  # 6. Cards with missing media (if references exist but might be broken)
  media_missing_cards <- cards_full[grepl("\\[sound:|<img", cards_full$flds) &
                                     cards_full$lapses >= 5, ]

  if (nrow(media_missing_cards) > 0) {
    media_rec <- head(media_missing_cards[, c("cid", "sfld", "name", "lapses")], 20)
    media_rec$recommendation <- "Check if media files exist"
  } else {
    media_rec <- tibble::tibble()
  }

  # Summary
  summary <- tibble::tibble(
    category = c("Leeches to Rewrite", "Cards to Unsuspend",
                 "Short Cards Needing Context", "Easy Cards to Retire",
                 "Tags to Consolidate", "Cards with Media Issues"),
    count = c(
      nrow(leeches_rec),
      nrow(unsuspend_rec),
      nrow(short_rec),
      nrow(easy_rec),
      nrow(tag_rec),
      nrow(media_rec)
    ),
    action = c(
      "Add mnemonics, images, or rewrite",
      "Review and unsuspend if ready",
      "Add more information to card",
      "Consider suspending to reduce workload",
      "Merge similar tags in Anki",
      "Check media folder for missing files"
    )
  )

  list(
    summary = summary,
    leeches_to_rewrite = if (nrow(leeches_rec) > 0) tibble::as_tibble(leeches_rec) else tibble::tibble(),
    cards_to_unsuspend = if (nrow(unsuspend_rec) > 0) tibble::as_tibble(unsuspend_rec) else tibble::tibble(),
    short_cards = if (nrow(short_rec) > 0) tibble::as_tibble(short_rec) else tibble::tibble(),
    easy_to_retire = if (nrow(easy_rec) > 0) tibble::as_tibble(easy_rec) else tibble::tibble(),
    tags_to_consolidate = if (nrow(tag_rec) > 0) tibble::as_tibble(tag_rec) else tibble::tibble(),
    media_issues = if (nrow(media_rec) > 0) tibble::as_tibble(media_rec) else tibble::tibble()
  )
}

#' Collection health check
#'
#' Performs a comprehensive health check on your Anki collection,
#' identifying common issues and problems.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with health check results
#' @export
#' @examples
#' \dontrun{
#' health <- anki_health_check()
#' }
anki_health_check <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()
  decks <- col$decks()

  issues <- list()
  warnings <- list()
  info <- list()

  # Check 1: Orphan cards (cards without notes - shouldn't happen)
  orphan_cards <- cards[!cards$nid %in% notes$nid, ]
  if (nrow(orphan_cards) > 0) {
    issues$orphan_cards <- list(
      severity = "HIGH",
      count = nrow(orphan_cards),
      message = paste(nrow(orphan_cards), "cards have no associated note"),
      action = "Run Check Database in Anki (Tools > Check Database)"
    )
  }

  # Check 2: High leech rate
  leech_rate <- sum(cards$lapses >= 8) / nrow(cards) * 100
  if (leech_rate > 10) {
    issues$high_leech_rate <- list(
      severity = "HIGH",
      count = sum(cards$lapses >= 8),
      message = paste0(round(leech_rate, 1), "% of cards are leeches (8+ lapses)"),
      action = "Review and rewrite leech cards or adjust leech threshold"
    )
  } else if (leech_rate > 5) {
    warnings$moderate_leech_rate <- list(
      severity = "MEDIUM",
      count = sum(cards$lapses >= 8),
      message = paste0(round(leech_rate, 1), "% of cards are leeches"),
      action = "Consider reviewing problem cards"
    )
  }

  # Check 3: Many suspended cards
  suspended_rate <- sum(cards$queue == -1) / nrow(cards) * 100
  if (suspended_rate > 20) {
    warnings$high_suspended <- list(
      severity = "MEDIUM",
      count = sum(cards$queue == -1),
      message = paste0(round(suspended_rate, 1), "% of cards are suspended"),
      action = "Review suspended cards - consider deleting or unsuspending"
    )
  }

  # Check 4: Very long cards
  notes$char_count <- nchar(notes$flds)
  very_long <- sum(notes$char_count > 5000)
  if (very_long > 0) {
    long_rate <- very_long / nrow(notes) * 100
    warnings$very_long_cards <- list(
      severity = "LOW",
      count = very_long,
      message = paste(very_long, "notes have >5000 characters"),
      action = "Consider breaking up very long cards"
    )
  }

  # Check 5: Empty fields
  notes$field_list <- strsplit(notes$flds, "\x1f")
  notes$has_empty <- vapply(notes$field_list, function(fields) {
    any(trimws(fields) == "" | is.na(fields))
  }, FUN.VALUE = logical(1))
  empty_count <- sum(notes$has_empty)

  if (empty_count > nrow(notes) * 0.1) {
    warnings$empty_fields <- list(
      severity = "LOW",
      count = empty_count,
      message = paste(empty_count, "notes have empty fields"),
      action = "Fill in empty fields or remove unused fields from note types"
    )
  }

  # Check 6: Review consistency
  recent_dates <- unique(revlog$review_date[revlog$review_date >= Sys.Date() - 30])
  days_studied <- length(recent_dates)

  if (days_studied < 15) {
    warnings$inconsistent_study <- list(
      severity = "MEDIUM",
      count = days_studied,
      message = paste("Only", days_studied, "days of study in the last 30 days"),
      action = "Try to maintain a consistent daily review habit"
    )
  }

  # Check 7: Overdue cards
  today <- as.integer(Sys.Date())
  overdue <- sum(cards$queue == 2 & cards$due < today - 7)

  if (overdue > 100) {
    warnings$many_overdue <- list(
      severity = "MEDIUM",
      count = overdue,
      message = paste(overdue, "cards are overdue by more than a week"),
      action = "Catch up on reviews to prevent further memory decay"
    )
  }

  # Check 8: Low retention
  recent_revlog <- revlog[revlog$review_date >= Sys.Date() - 30, ]
  if (nrow(recent_revlog) > 100) {
    retention <- 1 - sum(recent_revlog$ease == 1) / nrow(recent_revlog)

    if (retention < 0.80) {
      issues$low_retention <- list(
        severity = "HIGH",
        count = round(retention * 100, 1),
        message = paste0("Retention rate is only ", round(retention * 100, 1), "%"),
        action = "Consider increasing desired retention in FSRS settings or reviewing card quality"
      )
    } else if (retention < 0.85) {
      warnings$moderate_retention <- list(
        severity = "MEDIUM",
        count = round(retention * 100, 1),
        message = paste0("Retention rate is ", round(retention * 100, 1), "%"),
        action = "May want to slightly increase desired retention"
      )
    }
  }

  # Check 9: No FSRS
  cards_fsrs <- anki_cards_fsrs(path = col$path)
  has_fsrs <- sum(!is.na(cards_fsrs$stability)) > 0

  if (!has_fsrs) {
    info$no_fsrs <- list(
      severity = "INFO",
      message = "FSRS is not enabled",
      action = "Consider enabling FSRS in Anki preferences for better scheduling"
    )
  }

  # Check 10: Tag orphans
  all_tags <- unique(unlist(strsplit(notes$tags, "\\s+")))
  all_tags <- all_tags[all_tags != ""]

  orphan_tags <- character()
  for (tag in all_tags) {
    parts <- strsplit(tag, "::")[[1]]
    if (length(parts) > 1) {
      parent <- paste(parts[-length(parts)], collapse = "::")
      if (!parent %in% all_tags) {
        orphan_tags <- c(orphan_tags, tag)
      }
    }
  }

  if (length(orphan_tags) > 5) {
    info$orphan_tags <- list(
      severity = "INFO",
      count = length(orphan_tags),
      message = paste(length(orphan_tags), "tags have missing parent tags"),
      action = "Consider organizing tag hierarchy"
    )
  }

  # Summary
  total_issues <- length(issues)
  total_warnings <- length(warnings)
  total_info <- length(info)

  health_score <- 100 - (total_issues * 20) - (total_warnings * 5) - (total_info * 1)
  health_score <- max(0, min(100, health_score))

  status <- if (health_score >= 90) {
    "EXCELLENT"
  } else if (health_score >= 75) {
    "GOOD"
  } else if (health_score >= 50) {
    "FAIR"
  } else {
    "NEEDS ATTENTION"
  }

  list(
    health_score = health_score,
    status = status,
    summary = tibble::tibble(
      level = c("Issues", "Warnings", "Info"),
      count = c(total_issues, total_warnings, total_info)
    ),
    issues = issues,
    warnings = warnings,
    info = info,
    collection_stats = tibble::tibble(
      metric = c("Total Notes", "Total Cards", "Review Cards", "New Cards",
                 "Suspended Cards", "Leech Cards", "Days Studied (30d)"),
      value = c(nrow(notes), nrow(cards), sum(cards$type == 2), sum(cards$type == 0),
                sum(cards$queue == -1), sum(cards$lapses >= 8), days_studied)
    )
  )
}

#' Quick collection summary
#'
#' Returns a one-liner overview of your Anki collection.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with key metrics
#' @export
#' @examples
#' \dontrun{
#' anki_summary()
#' }
anki_summary <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  revlog <- col$revlog()

  # Current streak
  streak_info <- anki_streak(path = col$path)

  # Recent retention
  recent_revlog <- revlog[revlog$review_date >= Sys.Date() - 7, ]
  retention_7d <- if (nrow(recent_revlog) > 0) {
    round((1 - sum(recent_revlog$ease == 1) / nrow(recent_revlog)) * 100, 1)
  } else NA

  # Reviews today
  today_revlog <- revlog[revlog$review_date == Sys.Date(), ]
  reviews_today <- nrow(today_revlog)

  # Due today
  today <- as.integer(Sys.Date())
  due_today <- sum(cards$queue == 2 & cards$due <= today)

  tibble::tibble(
    metric = c("Total Cards", "Mature Cards", "Due Today", "Reviews Today",
               "Current Streak", "7-Day Retention"),
    value = c(
      nrow(cards),
      sum(cards$ivl >= 21),
      due_today,
      reviews_today,
      streak_info$current_streak,
      paste0(retention_7d, "%")
    )
  )
}

#' Today's activity summary
#'
#' Returns what happened today in your Anki studies.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with today's stats
#' @export
#' @examples
#' \dontrun{
#' anki_today()
#' }
anki_today <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  revlog <- col$revlog()

  # Today's reviews
  today_revlog <- revlog[revlog$review_date == Sys.Date(), ]

  if (nrow(today_revlog) == 0) {
    return(list(
      studied_today = FALSE,
      message = "No reviews yet today",
      due_remaining = sum(cards$queue == 2 & cards$due <= as.integer(Sys.Date()))
    ))
  }

  # Stats
  retention <- round((1 - sum(today_revlog$ease == 1) / nrow(today_revlog)) * 100, 1)
  time_min <- round(sum(today_revlog$time, na.rm = TRUE) / 60000, 1)
  unique_cards <- length(unique(today_revlog$cid))

  # Due remaining
  today <- as.integer(Sys.Date())
  due_remaining <- sum(cards$queue == 2 & cards$due <= today) -
                   sum(today_revlog$cid %in% cards$cid[cards$queue == 2])
  due_remaining <- max(0, due_remaining)

  list(
    studied_today = TRUE,
    stats = tibble::tibble(
      metric = c("Reviews", "Unique Cards", "Time (min)", "Retention (%)",
                 "Again", "Hard", "Good", "Easy"),
      value = c(
        nrow(today_revlog),
        unique_cards,
        time_min,
        retention,
        sum(today_revlog$ease == 1),
        sum(today_revlog$ease == 2),
        sum(today_revlog$ease == 3),
        sum(today_revlog$ease == 4)
      )
    ),
    due_remaining = due_remaining,
    message = if (due_remaining == 0) {
      "All reviews complete for today!"
    } else {
      paste(due_remaining, "cards still due")
    }
  )
}
