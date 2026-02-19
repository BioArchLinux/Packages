#' A/B Comparison
#'
#' Compare retention, efficiency, and performance across different
#' note types, deck settings, card formats, or custom groups.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param by Comparison dimension: "note_type", "deck", "tag", "created_period" (default "note_type")
#' @param min_reviews Minimum reviews per group to include (default 100)
#' @return A tibble with comparison statistics
#' @export
#'
#' @examples
#' \dontrun{
#' # Compare note types
#' comp <- anki_ab_comparison(by = "note_type")
#' 
#' # Compare decks
#' comp <- anki_ab_comparison(by = "deck")
#' }
anki_ab_comparison <- function(path = NULL, profile = NULL,
                               by = "note_type",
                               min_reviews = 100) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()
  decks <- col$decks()
  models <- col$models()
  
  if (nrow(revlog) < 200) {
    message("Insufficient review data for A/B comparison")
    return(tibble::tibble())
  }
  
  # Merge cards with notes to get note type
  cards_notes <- merge(cards[, c("cid", "nid", "did", "type", "ivl", "reps", "lapses")],
                       notes[, c("nid", "mid", "tags")], by = "nid", all.x = TRUE)
  
  # Add deck names
  deck_map <- setNames(decks$name, decks$did)
  cards_notes$deck_name <- deck_map[as.character(cards_notes$did)]
  
  # Add model names
  model_map <- setNames(models$name, models$mid)
  cards_notes$note_type <- model_map[as.character(cards_notes$mid)]
  
  # Add created period
  cards_notes$created_date <- as.Date(as.POSIXct(cards_notes$cid / 1000, origin = "1970-01-01"))
  cards_notes$created_period <- format(cards_notes$created_date, "%Y-%m")
  
  # Determine grouping variable
  group_var <- switch(by,
    "note_type" = "note_type",
    "deck" = "deck_name",
    "tag" = "tags",
    "created_period" = "created_period",
    "note_type"
  )
  
  # Handle tags specially (expand to one row per tag)
  if (by == "tag") {
    # Split tags and expand
    tag_list <- strsplit(as.character(cards_notes$tags), " ")
    expanded_rows <- lapply(seq_len(nrow(cards_notes)), function(i) {
      tags <- tag_list[[i]]
      tags <- tags[nchar(tags) > 0]
      if (length(tags) == 0) tags <- "(untagged)"
      data.frame(cards_notes[i, ], tag = tags, stringsAsFactors = FALSE)
    })
    cards_notes <- do.call(rbind, expanded_rows)
    group_var <- "tag"
  }
  
  # Calculate review statistics per card
  card_stats <- aggregate(
    cbind(
      total_reviews = ease,
      correct = ease > 1,
      again = ease == 1,
      hard = ease == 2,
      good = ease == 3,
      easy = ease == 4,
      total_time = time
    ) ~ cid,
    data = revlog,
    FUN = function(x) {
      if (length(x) > 1) {
        if (all(x %in% c(TRUE, FALSE))) sum(x) else length(x)
      } else {
        if (is.logical(x)) sum(x) else length(x)
      }
    }
  )
  
  # Get review counts
  review_counts <- aggregate(ease ~ cid, data = revlog, FUN = length)
  names(review_counts) <- c("cid", "total_reviews")
  
  # Get retention
  retention_stats <- aggregate(ease > 1 ~ cid, data = revlog, FUN = mean)
  names(retention_stats) <- c("cid", "retention")
  
  # Get time stats
  time_stats <- aggregate(time ~ cid, data = revlog, FUN = sum)
  names(time_stats) <- c("cid", "total_time")
  
  # Merge all
  card_stats <- merge(review_counts, retention_stats, by = "cid")
  card_stats <- merge(card_stats, time_stats, by = "cid")
  
  # Merge with card info
  analysis_df <- merge(cards_notes, card_stats, by = "cid", all.x = TRUE)
  analysis_df <- analysis_df[!is.na(analysis_df$total_reviews), ]
  
  # Aggregate by group
  group_col <- analysis_df[[group_var]]
  
  comparison <- aggregate(
    cbind(
      card_count = 1,
      total_reviews = total_reviews,
      retention = retention,
      total_time = total_time,
      avg_reps = reps,
      avg_lapses = lapses,
      mature_count = type == 2 & ivl >= 21
    ) ~ group_col,
    data = analysis_df,
    FUN = function(x) {
      if (is.logical(x)) sum(x, na.rm = TRUE)
      else if (length(x) > 0) mean(x, na.rm = TRUE)
      else NA
    }
  )
  names(comparison)[1] <- "group"
  
  # Get proper counts
  card_counts <- aggregate(cid ~ group_col, data = analysis_df, FUN = length)
  names(card_counts) <- c("group", "card_count")
  
  review_totals <- aggregate(total_reviews ~ group_col, data = analysis_df, FUN = sum)
  names(review_totals) <- c("group", "total_reviews")
  
  retention_avg <- aggregate(retention ~ group_col, data = analysis_df, FUN = mean)
  names(retention_avg) <- c("group", "retention")
  
  time_avg <- aggregate(total_time ~ group_col, data = analysis_df, FUN = mean)
  names(time_avg) <- c("group", "avg_time_per_card")
  
  mature_counts <- aggregate(type == 2 & ivl >= 21 ~ group_col, data = analysis_df, FUN = sum)
  names(mature_counts) <- c("group", "mature_cards")
  
  # Merge
  result <- merge(card_counts, review_totals, by = "group")
  result <- merge(result, retention_avg, by = "group")
  result <- merge(result, time_avg, by = "group")
  result <- merge(result, mature_counts, by = "group")
  
  # Filter by minimum reviews
  result <- result[result$total_reviews >= min_reviews, ]
  
  # Calculate derived metrics
  result$reviews_per_card <- round(result$total_reviews / result$card_count, 1)
  result$retention_pct <- round(result$retention * 100, 1)
  result$mature_pct <- round(result$mature_cards / result$card_count * 100, 1)
  result$avg_time_min <- round(result$avg_time_per_card / 60000, 2)
  result$efficiency <- round(result$retention_pct / result$reviews_per_card, 2)
  
  # Rank groups
  result$retention_rank <- rank(-result$retention_pct)
  result$efficiency_rank <- rank(-result$efficiency)
  
  # Sort by retention
  result <- result[order(-result$retention_pct), ]
  
  # Clean up columns
  final_result <- result[, c("group", "card_count", "total_reviews", "reviews_per_card",
                              "retention_pct", "mature_pct", "avg_time_min", "efficiency",
                              "retention_rank", "efficiency_rank")]
  
  # Add metadata
  attr(final_result, "comparison_by") <- by
  attr(final_result, "min_reviews") <- min_reviews
  attr(final_result, "best_retention") <- final_result$group[1]
  attr(final_result, "best_efficiency") <- final_result$group[which.max(final_result$efficiency)]
  
  # Summary
  cat("A/B Comparison by:", by, "\n")
  cat("Best retention:", final_result$group[1], "-", final_result$retention_pct[1], "%\n")
  cat("Most efficient:", final_result$group[which.max(final_result$efficiency)], 
      "- score:", max(final_result$efficiency), "\n\n")
  
  tibble::as_tibble(final_result)
}

#' Compare Two Specific Groups
#'
#' Detailed statistical comparison between two groups (decks, note types, etc.)
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param group_a First group name/pattern
#' @param group_b Second group name/pattern  
#' @param by Comparison dimension: "note_type", "deck", "tag"
#' @return A list with detailed comparison statistics
#' @export
#'
#' @examples
#' \dontrun{
#' comp <- anki_compare_groups("Basic", "Cloze", by = "note_type")
#' comp$winner
#' comp$differences
#' }
anki_compare_groups <- function(path = NULL, profile = NULL,
                                group_a, group_b,
                                by = "deck") {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  cards <- col$cards()
  notes <- col$notes()
  revlog <- col$revlog()
  decks <- col$decks()
  models <- col$models()
  
  # Merge data
  cards_notes <- merge(cards[, c("cid", "nid", "did", "type", "ivl", "reps", "lapses")],
                       notes[, c("nid", "mid", "tags")], by = "nid", all.x = TRUE)
  
  # Add names
  deck_map <- setNames(decks$name, decks$did)
  cards_notes$deck_name <- deck_map[as.character(cards_notes$did)]
  model_map <- setNames(models$name, models$mid)
  cards_notes$note_type <- model_map[as.character(cards_notes$mid)]
  
  # Filter to groups
  if (by == "deck") {
    group_a_cards <- cards_notes$cid[grepl(group_a, cards_notes$deck_name, ignore.case = TRUE)]
    group_b_cards <- cards_notes$cid[grepl(group_b, cards_notes$deck_name, ignore.case = TRUE)]
  } else if (by == "note_type") {
    group_a_cards <- cards_notes$cid[grepl(group_a, cards_notes$note_type, ignore.case = TRUE)]
    group_b_cards <- cards_notes$cid[grepl(group_b, cards_notes$note_type, ignore.case = TRUE)]
  } else if (by == "tag") {
    group_a_cards <- cards_notes$cid[grepl(group_a, cards_notes$tags, ignore.case = TRUE)]
    group_b_cards <- cards_notes$cid[grepl(group_b, cards_notes$tags, ignore.case = TRUE)]
  } else {
    stop("Invalid 'by' parameter")
  }
  
  if (length(group_a_cards) < 10 || length(group_b_cards) < 10) {
    stop("Need at least 10 cards in each group")
  }
  
  # Get reviews for each group
  revlog_a <- revlog[revlog$cid %in% group_a_cards, ]
  revlog_b <- revlog[revlog$cid %in% group_b_cards, ]
  
  # Calculate stats for each group
  calc_stats <- function(rv, cds, name) {
    cards_in_group <- cds[cds$cid %in% rv$cid, ]
    list(
      name = name,
      card_count = length(unique(rv$cid)),
      total_reviews = nrow(rv),
      retention = mean(rv$ease > 1, na.rm = TRUE),
      again_rate = mean(rv$ease == 1, na.rm = TRUE),
      easy_rate = mean(rv$ease == 4, na.rm = TRUE),
      avg_time_ms = mean(rv$time, na.rm = TRUE),
      median_time_ms = median(rv$time, na.rm = TRUE),
      mature_cards = sum(cards_in_group$type == 2 & cards_in_group$ivl >= 21),
      avg_interval = mean(cards_in_group$ivl[cards_in_group$ivl > 0], na.rm = TRUE),
      avg_lapses = mean(cards_in_group$lapses, na.rm = TRUE)
    )
  }
  
  stats_a <- calc_stats(revlog_a, cards_notes, group_a)
  stats_b <- calc_stats(revlog_b, cards_notes, group_b)
  
  # Calculate differences
  differences <- tibble::tibble(
    metric = c("Retention", "Again Rate", "Easy Rate", "Avg Time", 
               "Mature %", "Avg Interval", "Avg Lapses"),
    group_a = c(
      round(stats_a$retention * 100, 1),
      round(stats_a$again_rate * 100, 1),
      round(stats_a$easy_rate * 100, 1),
      round(stats_a$avg_time_ms / 1000, 2),
      round(stats_a$mature_cards / stats_a$card_count * 100, 1),
      round(stats_a$avg_interval, 1),
      round(stats_a$avg_lapses, 2)
    ),
    group_b = c(
      round(stats_b$retention * 100, 1),
      round(stats_b$again_rate * 100, 1),
      round(stats_b$easy_rate * 100, 1),
      round(stats_b$avg_time_ms / 1000, 2),
      round(stats_b$mature_cards / stats_b$card_count * 100, 1),
      round(stats_b$avg_interval, 1),
      round(stats_b$avg_lapses, 2)
    )
  )
  differences$difference <- differences$group_a - differences$group_b
  differences$winner <- ifelse(
    differences$metric %in% c("Again Rate", "Avg Lapses"),
    ifelse(differences$difference < 0, group_a, group_b),
    ifelse(differences$difference > 0, group_a, group_b)
  )
  names(differences)[2:3] <- c(group_a, group_b)
  
  # Overall winner (by retention)
  overall_winner <- if (stats_a$retention > stats_b$retention) group_a else group_b
  
  # Statistical significance (simple chi-square for retention)
  correct_a <- sum(revlog_a$ease > 1)
  incorrect_a <- sum(revlog_a$ease == 1)
  correct_b <- sum(revlog_b$ease > 1)
  incorrect_b <- sum(revlog_b$ease == 1)
  
  contingency <- matrix(c(correct_a, incorrect_a, correct_b, incorrect_b), nrow = 2)
  chi_test <- tryCatch(chisq.test(contingency), error = function(e) NULL)
  
  list(
    summary = tibble::tibble(
      group = c(group_a, group_b),
      cards = c(stats_a$card_count, stats_b$card_count),
      reviews = c(stats_a$total_reviews, stats_b$total_reviews),
      retention_pct = c(round(stats_a$retention * 100, 1), round(stats_b$retention * 100, 1))
    ),
    differences = differences,
    winner = overall_winner,
    retention_difference = round((stats_a$retention - stats_b$retention) * 100, 2),
    significance = if (!is.null(chi_test)) {
      list(
        chi_squared = round(chi_test$statistic, 2),
        p_value = round(chi_test$p.value, 4),
        significant = chi_test$p.value < 0.05
      )
    } else NULL,
    recommendation = paste0(
      overall_winner, " has ", 
      abs(round((stats_a$retention - stats_b$retention) * 100, 1)), 
      "% ", ifelse(stats_a$retention > stats_b$retention, "higher", "lower"),
      " retention. ",
      if (!is.null(chi_test) && chi_test$p.value < 0.05) 
        "This difference is statistically significant." 
      else 
        "Consider collecting more data for statistical significance."
    )
  )
}
