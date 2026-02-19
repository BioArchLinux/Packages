#' Track exam readiness
#'
#' Projects whether you'll complete cards before an exam, at what retention level.
#' Useful for medical board exam preparation.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param target_date Exam date (Date or character "YYYY-MM-DD")
#' @param deck_pattern Pattern to match deck names (e.g., "Step1", "USMLE")
#' @param new_cards_per_day Expected new cards per day (default 20)
#' @param target_retention Target retention rate (default 0.90)
#' @return A list with exam readiness analysis
#' @export
#' @examples
#' \dontrun{
#' readiness <- anki_exam_readiness(target_date = "2024-06-15", deck_pattern = "Step1")
#' }
anki_exam_readiness <- function(path = NULL, profile = NULL,
                                target_date, deck_pattern = NULL,
                                new_cards_per_day = 20, target_retention = 0.90) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
  revlog <- col$revlog()
  cards_fsrs <- anki_cards_fsrs(path = col$path)

  # Convert target date
  target_date <- as.Date(target_date)
  days_until_exam <- as.integer(target_date - Sys.Date())

  if (days_until_exam <= 0) {
    message("Target date is in the past")
    return(list(message = "Exam date has passed"))
  }

  # Filter to relevant decks
  if (!is.null(deck_pattern)) {
    relevant_decks <- decks$did[grepl(deck_pattern, decks$name, ignore.case = TRUE)]
    cards <- cards[cards$did %in% relevant_decks, ]
    if (nrow(cards) == 0) {
      message("No cards found matching deck pattern: ", deck_pattern)
      return(NULL)
    }
  }

  # Current status
  total_cards <- nrow(cards)
  new_cards <- sum(cards$type == 0)
  learning_cards <- sum(cards$type == 1 | cards$type == 3)
  review_cards <- sum(cards$type == 2)
  mature_cards <- sum(cards$ivl >= 21)
  suspended_cards <- sum(cards$queue == -1)

  # Cards that need to be studied (excluding suspended)
  active_cards <- cards[cards$queue != -1, ]
  active_new <- sum(active_cards$type == 0)
  active_total <- nrow(active_cards)

  # Time to introduce all new cards
  days_to_introduce <- ceiling(active_new / new_cards_per_day)

  # Check if we have time
  can_complete <- days_to_introduce <= days_until_exam

  # Estimate when cards will mature
  # Assume average of 30 days from introduction to mature
  days_for_maturing <- 30

  # Calculate projected completion timeline
  completion_date <- Sys.Date() + days_to_introduce + days_for_maturing

  # Current retention
  recent_revlog <- revlog[revlog$review_date >= Sys.Date() - 30 &
                           revlog$cid %in% cards$cid, ]
  current_retention <- if (nrow(recent_revlog) > 100) {
    1 - sum(recent_revlog$ease == 1) / nrow(recent_revlog)
  } else {
    NA
  }

  # Projected retention at exam
  # Get FSRS data
  cards_fsrs <- cards_fsrs[cards_fsrs$cid %in% cards$cid, ]
  cards_fsrs <- cards_fsrs[!is.na(cards_fsrs$stability), ]

  if (nrow(cards_fsrs) > 0) {
    # Get last review dates
    last_review <- aggregate(review_date ~ cid, data = revlog, FUN = max)
    cards_fsrs <- merge(cards_fsrs, last_review, by = "cid", all.x = TRUE)

    # Days elapsed by exam date
    cards_fsrs$days_at_exam <- as.numeric(target_date - cards_fsrs$review_date)
    cards_fsrs$days_at_exam[is.na(cards_fsrs$days_at_exam)] <- days_until_exam

    cards_fsrs$decay[is.na(cards_fsrs$decay)] <- 0.5

    # Calculate retrievability at exam
    cards_fsrs$r_at_exam <- fsrs_retrievability(
      stability = cards_fsrs$stability,
      days_elapsed = cards_fsrs$days_at_exam,
      decay = cards_fsrs$decay
    )

    avg_r_at_exam <- mean(cards_fsrs$r_at_exam, na.rm = TRUE)
    pct_below_target <- sum(cards_fsrs$r_at_exam < target_retention) / nrow(cards_fsrs) * 100
  } else {
    avg_r_at_exam <- NA
    pct_below_target <- NA
  }

  # Calculate required daily reviews to maintain retention
  if (nrow(cards_fsrs) > 0) {
    avg_stability <- mean(cards_fsrs$stability, na.rm = TRUE)
    avg_decay <- mean(cards_fsrs$decay, na.rm = TRUE)
    avg_interval <- fsrs_interval(avg_stability, target_retention, avg_decay)
    steady_state_reviews <- review_cards / avg_interval
  } else {
    steady_state_reviews <- review_cards / 14  # Default assumption
  }

  # Daily workload estimate
  learning_reviews <- active_new * 5 / 7  # ~5 learning reviews per new card over first week
  daily_workload <- steady_state_reviews + learning_reviews + new_cards_per_day

  # Risk assessment
  risk_level <- if (!can_complete) {
    "HIGH"
  } else if (completion_date > target_date - 14) {
    "MEDIUM"
  } else if (!is.na(avg_r_at_exam) && avg_r_at_exam < target_retention) {
    "MEDIUM"
  } else {
    "LOW"
  }

  # Recommendations
  recommendations <- character()

  if (!can_complete) {
    recommendations <- c(recommendations,
      paste0("CRITICAL: At ", new_cards_per_day, " new cards/day, you need ",
             days_to_introduce, " days to see all cards (", days_until_exam, " days available)"))
    recommendations <- c(recommendations,
      paste0("Consider increasing new cards to ", ceiling(active_new / days_until_exam), " per day"))
  }

  if (!is.na(pct_below_target) && pct_below_target > 30) {
    recommendations <- c(recommendations,
      paste0(round(pct_below_target, 1), "% of cards may be below target retention by exam"))
  }

  if (suspended_cards > total_cards * 0.1) {
    recommendations <- c(recommendations,
      paste0(suspended_cards, " cards are suspended - review if needed for exam"))
  }

  if (daily_workload > 200) {
    recommendations <- c(recommendations,
      paste0("Expected daily workload of ", round(daily_workload), " reviews may be challenging"))
  }

  list(
    exam_info = tibble::tibble(
      metric = c("Exam Date", "Days Until Exam", "Target Retention"),
      value = c(as.character(target_date), days_until_exam, paste0(target_retention * 100, "%"))
    ),
    current_status = tibble::tibble(
      metric = c("Total Cards", "New Cards", "Learning Cards", "Review Cards",
                 "Mature Cards", "Suspended Cards", "Active Cards to Study"),
      value = c(total_cards, new_cards, learning_cards, review_cards,
                mature_cards, suspended_cards, active_total)
    ),
    timeline = tibble::tibble(
      milestone = c("Days to Introduce All Cards", "Days for Maturing",
                    "Projected Completion", "Exam Date"),
      date = c(
        as.character(Sys.Date() + days_to_introduce),
        paste(days_for_maturing, "days after last introduction"),
        as.character(completion_date),
        as.character(target_date)
      )
    ),
    projections = tibble::tibble(
      metric = c("Can Complete Before Exam", "Current Retention (30d)",
                 "Projected Retention at Exam", "Cards Below Target at Exam",
                 "Expected Daily Reviews"),
      value = c(
        if (can_complete) "Yes" else "No - at risk",
        if (!is.na(current_retention)) paste0(round(current_retention * 100, 1), "%") else "N/A",
        if (!is.na(avg_r_at_exam)) paste0(round(avg_r_at_exam * 100, 1), "%") else "N/A",
        if (!is.na(pct_below_target)) paste0(round(pct_below_target, 1), "%") else "N/A",
        round(daily_workload)
      )
    ),
    risk_level = risk_level,
    recommendations = recommendations
  )
}

#' Analyze topic coverage
#'
#' Shows percentage complete, mature, and retained by topic (tag or subdeck).
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Analysis type: "tag" or "deck" (default "tag")
#' @param pattern Optional pattern to filter tags/decks
#' @param min_cards Minimum cards to include in analysis (default 10)
#' @return A tibble with coverage analysis per topic
#' @export
#' @examples
#' \dontrun{
#' coverage <- anki_coverage_analysis()
#' coverage <- anki_coverage_analysis(pattern = "Anatomy")
#' }
anki_coverage_analysis <- function(path = NULL, profile = NULL,
                                   by = "tag", pattern = NULL, min_cards = 10) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  notes <- col$notes()
  decks <- col$decks()
  revlog <- col$revlog()

  # Get recent retention per card
  recent_revlog <- revlog[revlog$review_date >= Sys.Date() - 30, ]

  if (by == "tag") {
    # Extract all tags
    cards_notes <- merge(cards, notes[, c("nid", "tags")], by = "nid")

    # Get unique tags
    all_tags <- unique(unlist(strsplit(notes$tags, "\\s+")))
    all_tags <- all_tags[all_tags != ""]

    if (!is.null(pattern)) {
      all_tags <- all_tags[grepl(pattern, all_tags, ignore.case = TRUE)]
    }

    if (length(all_tags) == 0) {
      message("No matching tags found")
      return(tibble::tibble())
    }

    # Analyze each tag
    coverage <- lapply(all_tags, function(tag) {
      matching_cards <- cards_notes[grepl(tag, cards_notes$tags, fixed = TRUE), ]

      if (nrow(matching_cards) < min_cards) return(NULL)

      # Calculate metrics
      total <- nrow(matching_cards)
      new <- sum(matching_cards$type == 0)
      learning <- sum(matching_cards$type == 1 | matching_cards$type == 3)
      review <- sum(matching_cards$type == 2)
      mature <- sum(matching_cards$ivl >= 21)
      suspended <- sum(matching_cards$queue == -1)

      # Progress metrics
      started <- (total - new) / total * 100
      matured <- mature / total * 100

      # Retention for this tag
      tag_revlog <- recent_revlog[recent_revlog$cid %in% matching_cards$cid, ]
      retention <- if (nrow(tag_revlog) > 10) {
        (1 - sum(tag_revlog$ease == 1) / nrow(tag_revlog)) * 100
      } else {
        NA
      }

      tibble::tibble(
        topic = tag,
        total_cards = total,
        new_cards = new,
        started_pct = round(started, 1),
        mature_pct = round(matured, 1),
        retention_pct = round(retention, 1),
        suspended = suspended
      )
    })

    result <- do.call(rbind, Filter(Negate(is.null), coverage))

  } else {
    # By deck
    if (!is.null(pattern)) {
      decks <- decks[grepl(pattern, decks$name, ignore.case = TRUE), ]
    }

    coverage <- lapply(split(cards, cards$did), function(deck_cards) {
      did <- deck_cards$did[1]
      deck_name <- decks$name[decks$did == did]

      if (length(deck_name) == 0) deck_name <- paste("Deck", did)
      if (nrow(deck_cards) < min_cards) return(NULL)

      total <- nrow(deck_cards)
      new <- sum(deck_cards$type == 0)
      mature <- sum(deck_cards$ivl >= 21)
      suspended <- sum(deck_cards$queue == -1)

      started <- (total - new) / total * 100
      matured <- mature / total * 100

      deck_revlog <- recent_revlog[recent_revlog$cid %in% deck_cards$cid, ]
      retention <- if (nrow(deck_revlog) > 10) {
        (1 - sum(deck_revlog$ease == 1) / nrow(deck_revlog)) * 100
      } else {
        NA
      }

      tibble::tibble(
        topic = deck_name,
        total_cards = total,
        new_cards = new,
        started_pct = round(started, 1),
        mature_pct = round(matured, 1),
        retention_pct = round(retention, 1),
        suspended = suspended
      )
    })

    result <- do.call(rbind, Filter(Negate(is.null), coverage))
  }

  if (is.null(result) || nrow(result) == 0) {
    message("No topics with sufficient data")
    return(tibble::tibble())
  }

  # Calculate overall progress score
  result$progress_score <- with(result,
    started_pct * 0.3 + mature_pct * 0.5 + ifelse(is.na(retention_pct), 0, retention_pct * 0.2)
  )

  # Sort by progress
  result <- result[order(-result$progress_score), ]

  tibble::as_tibble(result)
}

#' Generate study priority list
#'
#' Identifies which topics/decks should be prioritized based on progress and retention.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Analysis type: "tag" or "deck" (default "tag")
#' @param n Number of priority topics to return (default 10)
#' @return A tibble with prioritized topics
#' @export
#' @examples
#' \dontrun{
#' priorities <- anki_study_priorities(n = 10)
#' }
anki_study_priorities <- function(path = NULL, profile = NULL,
                                  by = "tag", n = 10) {
  # Get coverage analysis
  coverage <- anki_coverage_analysis(path, profile, by = by, min_cards = 10)

  if (nrow(coverage) == 0) {
    return(tibble::tibble())
  }

  # Calculate priority score
  # High priority = low progress, low retention, many cards
  coverage$priority_score <- with(coverage, {
    # Invert progress (lower progress = higher priority)
    progress_factor <- 100 - progress_score

    # Weight by card count (more cards = higher priority)
    size_factor <- log1p(total_cards) / max(log1p(total_cards))

    # Retention penalty (lower retention = higher priority)
    retention_factor <- ifelse(is.na(retention_pct), 50, 100 - retention_pct)

    # Combine
    progress_factor * 0.4 + retention_factor * 0.4 + size_factor * 20
  })

  # Determine priority level
  coverage$priority_level <- cut(
    coverage$priority_score,
    breaks = quantile(coverage$priority_score, probs = c(0, 0.33, 0.66, 1)),
    labels = c("Low", "Medium", "High"),
    include.lowest = TRUE
  )

  # Generate action
  coverage$recommended_action <- with(coverage, ifelse(
    started_pct < 50,
    "Start studying - many new cards",
    ifelse(mature_pct < 30,
           "Continue daily reviews",
           ifelse(!is.na(retention_pct) & retention_pct < 85,
                  "Focus on retention - review leeches",
                  "Maintain - doing well"))
  ))

  # Sort and limit
  coverage <- coverage[order(-coverage$priority_score), ]
  result <- head(coverage, n)

  result <- result[, c("topic", "total_cards", "started_pct", "mature_pct",
                        "retention_pct", "priority_level", "recommended_action")]

  tibble::as_tibble(result)
}

#' Create study plan
#'
#' Generates a daily study plan based on exam date and current progress.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param target_date Exam/target date
#' @param daily_minutes Available study time per day in minutes (default 60)
#' @param deck_pattern Optional pattern to filter decks
#' @return A list with study plan
#' @export
#' @examples
#' \dontrun{
#' plan <- anki_study_plan(target_date = "2024-06-15", daily_minutes = 90)
#' }
anki_study_plan <- function(path = NULL, profile = NULL, target_date,
                            daily_minutes = 60, deck_pattern = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  cards <- col$cards()
  revlog <- col$revlog()
  decks <- col$decks()

  target_date <- as.Date(target_date)
  days_until <- as.integer(target_date - Sys.Date())

  if (days_until <= 0) {
    return(list(message = "Target date is in the past"))
  }

  # Filter decks if pattern provided
  if (!is.null(deck_pattern)) {
    relevant_decks <- decks$did[grepl(deck_pattern, decks$name, ignore.case = TRUE)]
    cards <- cards[cards$did %in% relevant_decks, ]
  }

  # Calculate average times from history
  recent_revlog <- revlog[revlog$review_date >= Sys.Date() - 30, ]

  if (nrow(recent_revlog) > 0) {
    avg_time_per_card <- mean(recent_revlog$time, na.rm = TRUE) / 1000 / 60  # minutes
  } else {
    avg_time_per_card <- 0.15  # 9 seconds default
  }

  # Cards available per day
  cards_per_day <- floor(daily_minutes / avg_time_per_card)

  # Current status
  new_cards <- sum(cards$type == 0 & cards$queue != -1)
  review_cards <- sum(cards$type == 2 & cards$queue != -1)
  due_today <- sum(cards$queue == 2 & cards$due <= as.integer(Sys.Date()))

  # Calculate phases
  # Phase 1: Catch up on reviews
  if (due_today > cards_per_day * 3) {
    catchup_days <- ceiling(due_today / (cards_per_day * 0.8))
    catchup_phase <- paste0("Days 1-", catchup_days, ": Catch up on ", due_today, " due cards")
  } else {
    catchup_days <- 0
    catchup_phase <- "No significant backlog"
  }

  # Phase 2: New card introduction
  remaining_days <- days_until - catchup_days
  new_cards_per_day <- if (remaining_days > 0) {
    # Reserve 70% of daily capacity for reviews
    min(floor(cards_per_day * 0.3), ceiling(new_cards / remaining_days))
  } else {
    0
  }

  days_to_introduce <- ceiling(new_cards / max(new_cards_per_day, 1))

  # Phase 3: Final review
  review_phase_days <- days_until - catchup_days - days_to_introduce
  review_phase_days <- max(0, review_phase_days)

  plan <- tibble::tibble(
    phase = c("Catch-up", "Introduction", "Final Review"),
    days = c(catchup_days, days_to_introduce, review_phase_days),
    focus = c(
      "Clear backlog of due cards",
      paste0("Introduce ", new_cards_per_day, " new cards/day"),
      "Focus on retention and weak areas"
    ),
    date_range = c(
      if (catchup_days > 0) paste0(Sys.Date(), " to ", Sys.Date() + catchup_days - 1) else "N/A",
      paste0(Sys.Date() + catchup_days, " to ", Sys.Date() + catchup_days + days_to_introduce - 1),
      if (review_phase_days > 0) paste0(target_date - review_phase_days + 1, " to ", target_date) else "N/A"
    )
  )

  # Daily targets
  daily_targets <- tibble::tibble(
    metric = c("Study Time (min)", "Review Cards", "New Cards", "Total Cards"),
    target = c(daily_minutes, round(cards_per_day * 0.7), new_cards_per_day, cards_per_day)
  )

  # Risk factors
  risks <- character()
  if (days_to_introduce > remaining_days) {
    risks <- c(risks, "Not enough time to introduce all cards - increase daily new cards")
  }
  if (review_phase_days < 14) {
    risks <- c(risks, "Limited time for final review phase")
  }
  if (new_cards_per_day > 30) {
    risks <- c(risks, "High new card rate may be unsustainable")
  }

  list(
    overview = tibble::tibble(
      metric = c("Days Until Target", "Total New Cards", "Total Review Cards",
                 "Avg Time/Card (min)", "Cards/Day Capacity"),
      value = c(days_until, new_cards, review_cards, round(avg_time_per_card, 2), cards_per_day)
    ),
    phases = plan,
    daily_targets = daily_targets,
    risks = if (length(risks) > 0) risks else "Plan looks feasible",
    recommendations = c(
      paste0("Study ", daily_minutes, " minutes daily"),
      paste0("Introduce ", new_cards_per_day, " new cards per day"),
      if (review_phase_days >= 7) "Use final week for targeted review of weak areas"
    )
  )
}
