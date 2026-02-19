#' Advanced Streak Analytics
#'
#' Detailed streak analysis including best streaks, recovery time after breaks,
#' weekend vs weekday patterns, and streak predictions.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with detailed streak statistics
#' @export
#'
#' @examples
#' \dontrun{
#' streaks <- anki_streak_analytics()
#' streaks$current
#' streaks$history
#' }
anki_streak_analytics <- function(path = NULL, profile = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  revlog <- col$revlog()
  
  if (nrow(revlog) < 10) {
    message("Insufficient review data for streak analytics")
    return(list())
  }
  
  today <- Sys.Date()
  
  # Get all unique study dates
  study_dates <- sort(unique(revlog$review_date))
  
  # Calculate all streaks
  if (length(study_dates) < 2) {
    return(list(
      current = list(days = length(study_dates), status = "active"),
      streaks = tibble::tibble(),
      message = "Not enough data"
    ))
  }
  
  # Find streak boundaries
  date_diffs <- c(1, diff(study_dates))
  streak_starts <- which(date_diffs > 1)
  
  if (length(streak_starts) == 0) {
    # One continuous streak
    streaks <- tibble::tibble(
      streak_num = 1,
      start_date = min(study_dates),
      end_date = max(study_dates),
      length = length(study_dates)
    )
  } else {
    # Multiple streaks
    starts <- c(1, streak_starts)
    ends <- c(streak_starts - 1, length(study_dates))
    
    streaks <- tibble::tibble(
      streak_num = seq_along(starts),
      start_date = study_dates[starts],
      end_date = study_dates[ends],
      length = ends - starts + 1
    )
  }
  
  # Current streak
  last_study <- max(study_dates)
  if (last_study >= today - 1) {
    # Active streak (studied today or yesterday)
    current_streak_row <- streaks[nrow(streaks), ]
    current_streak <- current_streak_row$length
    streak_status <- if (last_study == today) "active_today" else "at_risk"
  } else {
    current_streak <- 0
    streak_status <- "broken"
  }
  
  # Analyze breaks between streaks
  if (nrow(streaks) > 1) {
    breaks <- tibble::tibble(
      after_streak = streaks$streak_num[-nrow(streaks)],
      break_start = streaks$end_date[-nrow(streaks)] + 1,
      break_end = streaks$start_date[-1] - 1,
      break_length = as.numeric(streaks$start_date[-1] - streaks$end_date[-nrow(streaks)] - 1),
      streak_before = streaks$length[-nrow(streaks)],
      streak_after = streaks$length[-1]
    )
    
    avg_break_length <- mean(breaks$break_length, na.rm = TRUE)
    avg_recovery_streak <- mean(breaks$streak_after, na.rm = TRUE)
  } else {
    breaks <- tibble::tibble()
    avg_break_length <- NA
    avg_recovery_streak <- NA
  }
  
  # Weekend vs weekday patterns
  revlog$day_of_week <- weekdays(revlog$review_date)
  revlog$is_weekend <- revlog$day_of_week %in% c("Saturday", "Sunday")
  
  weekday_dates <- unique(revlog$review_date[!revlog$is_weekend])
  weekend_dates <- unique(revlog$review_date[revlog$is_weekend])
  
  # Get total weekdays and weekends in range
  date_range <- seq(min(study_dates), max(study_dates), by = "day")
  total_weekdays <- sum(!weekdays(date_range) %in% c("Saturday", "Sunday"))
  total_weekends <- sum(weekdays(date_range) %in% c("Saturday", "Sunday"))
  
  weekday_rate <- length(weekday_dates) / total_weekdays * 100
  weekend_rate <- length(weekend_dates) / total_weekends * 100
  
  # Day-of-week breakdown
  dow_counts <- table(weekdays(study_dates))
  dow_df <- tibble::tibble(
    day = factor(names(dow_counts), 
                 levels = c("Monday", "Tuesday", "Wednesday", "Thursday", 
                           "Friday", "Saturday", "Sunday")),
    days_studied = as.integer(dow_counts)
  )
  dow_df <- dow_df[order(dow_df$day), ]
  
  # Streak statistics
  longest_streak <- max(streaks$length)
  avg_streak <- mean(streaks$length)
  median_streak <- median(streaks$length)
  total_streaks <- nrow(streaks)
  
  # Top 5 longest streaks
  top_streaks <- streaks[order(-streaks$length), ][1:min(5, nrow(streaks)), ]
  
  # Streak prediction (based on historical patterns)
  streak_survival <- sapply(1:30, function(day) {
    sum(streaks$length >= day) / nrow(streaks) * 100
  })
  
  # Monthly consistency
  revlog$month <- format(revlog$review_date, "%Y-%m")
  monthly_days <- aggregate(review_date ~ month, data = revlog, 
                            FUN = function(x) length(unique(x)))
  names(monthly_days) <- c("month", "days_studied")
  
  # Days in each month
  monthly_days$days_in_month <- sapply(monthly_days$month, function(m) {
    d <- as.Date(paste0(m, "-01"))
    as.numeric(as.Date(format(d + 31, "%Y-%m-01")) - d)
  })
  monthly_days$consistency_pct <- round(monthly_days$days_studied / monthly_days$days_in_month * 100, 1)
  
  # Risk assessment
  if (streak_status == "at_risk") {
    risk_message <- "You didn't study today - your streak is at risk!"
  } else if (streak_status == "broken") {
    days_since <- as.numeric(today - last_study)
    risk_message <- paste("Streak broken", days_since, "days ago - time to start fresh!")
  } else {
    risk_message <- "Great job! Your streak is active."
  }
  
  list(
    current = list(
      days = current_streak,
      status = streak_status,
      last_study = as.character(last_study),
      message = risk_message
    ),
    records = list(
      longest_streak = longest_streak,
      average_streak = round(avg_streak, 1),
      median_streak = round(median_streak, 1),
      total_streaks = total_streaks,
      total_study_days = length(study_dates)
    ),
    top_streaks = top_streaks,
    breaks = list(
      total_breaks = nrow(breaks),
      avg_break_length = round(avg_break_length, 1),
      avg_recovery_streak = round(avg_recovery_streak, 1),
      details = breaks
    ),
    patterns = list(
      weekday_rate = round(weekday_rate, 1),
      weekend_rate = round(weekend_rate, 1),
      by_day = dow_df
    ),
    monthly_consistency = tibble::as_tibble(monthly_days),
    survival_curve = tibble::tibble(
      day = 1:30,
      probability_pct = round(streak_survival, 1)
    )
  )
}

#' Card Content Analysis
#'
#' Analyze card content: word count, cloze density, complexity score,
#' and correlations with retention.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param sample_size Max cards to analyze (default 1000, NULL for all)
#' @return A list with content analysis
#' @export
#'
#' @examples
#' \dontrun{
#' content <- anki_card_content()
#' content$summary
#' content$complexity_retention
#' }
anki_card_content <- function(path = NULL, profile = NULL, sample_size = 1000) {
  col <- anki_collection(path, profile)
  on.exit(col$close())
  
  notes <- col$notes()
  cards <- col$cards()
  revlog <- col$revlog()
  
  if (nrow(notes) < 10) {
    message("Insufficient notes for content analysis")
    return(list())
  }
  
  # Sample if needed
  if (!is.null(sample_size) && nrow(notes) > sample_size) {
    notes <- notes[sample(nrow(notes), sample_size), ]
  }
  
  # Analyze each note's fields
  content_stats <- lapply(seq_len(nrow(notes)), function(i) {
    flds <- notes$flds[i]
    
    # Split fields and clean HTML
    clean_text <- gsub("<[^>]+>", " ", flds)
    clean_text <- gsub("\\s+", " ", clean_text)
    clean_text <- trimws(clean_text)
    
    # Word count
    words <- strsplit(clean_text, "\\s+")[[1]]
    word_count <- length(words[nchar(words) > 0])
    
    # Character count (excluding spaces)
    char_count <- nchar(gsub("\\s", "", clean_text))
    
    # Cloze count
    cloze_count <- length(gregexpr("\\{\\{c\\d+::", flds)[[1]])
    if (cloze_count < 0) cloze_count <- 0
    
    # Image count
    img_count <- length(gregexpr("<img", flds, ignore.case = TRUE)[[1]])
    if (img_count < 0) img_count <- 0
    
    # Sound count
    sound_count <- length(gregexpr("\\[sound:", flds)[[1]])
    if (sound_count < 0) sound_count <- 0
    
    # Average word length (complexity indicator)
    avg_word_len <- if (word_count > 0) mean(nchar(words[nchar(words) > 0])) else 0
    
    # Field count (separator is 0x1f)
    field_count <- length(strsplit(flds, "\x1f")[[1]])
    
    list(
      nid = notes$nid[i],
      word_count = word_count,
      char_count = char_count,
      cloze_count = cloze_count,
      img_count = img_count,
      sound_count = sound_count,
      avg_word_len = avg_word_len,
      field_count = field_count
    )
  })
  
  content_df <- do.call(rbind, lapply(content_stats, function(x) {
    data.frame(
      nid = x$nid,
      word_count = x$word_count,
      char_count = x$char_count,
      cloze_count = x$cloze_count,
      img_count = x$img_count,
      sound_count = x$sound_count,
      avg_word_len = x$avg_word_len,
      field_count = x$field_count
    )
  }))
  
  # Get card-level data and merge with notes
  card_notes <- merge(cards[, c("cid", "nid", "type", "ivl", "reps", "lapses")],
                      content_df, by = "nid")
  
  # Get retention per card
  card_retention <- aggregate(ease > 1 ~ cid, data = revlog, FUN = mean)
  names(card_retention) <- c("cid", "retention")
  
  card_content <- merge(card_notes, card_retention, by = "cid", all.x = TRUE)
  card_content$retention[is.na(card_content$retention)] <- NA
  
  # Calculate complexity score (composite)
  card_content$complexity <- with(card_content, 
    (word_count / max(word_count, na.rm = TRUE) * 0.3 +
     avg_word_len / max(avg_word_len, na.rm = TRUE) * 0.3 +
     field_count / max(field_count, na.rm = TRUE) * 0.2 +
     (cloze_count > 0) * 0.2) * 100
  )
  
  # Categorize complexity
  card_content$complexity_cat <- cut(card_content$complexity,
    breaks = c(0, 25, 50, 75, 100),
    labels = c("Simple", "Moderate", "Complex", "Very Complex"),
    include.lowest = TRUE
  )
  
  # Summary statistics
  summary_stats <- tibble::tibble(
    metric = c("Avg Word Count", "Avg Char Count", "Cards with Clozes", 
               "Cards with Images", "Cards with Audio", "Avg Complexity"),
    value = c(
      round(mean(card_content$word_count, na.rm = TRUE), 1),
      round(mean(card_content$char_count, na.rm = TRUE), 1),
      sum(card_content$cloze_count > 0),
      sum(card_content$img_count > 0),
      sum(card_content$sound_count > 0),
      round(mean(card_content$complexity, na.rm = TRUE), 1)
    )
  )
  
  # Complexity vs retention correlation
  cards_with_retention <- card_content[!is.na(card_content$retention) & card_content$reps >= 5, ]
  
  if (nrow(cards_with_retention) > 20) {
    complexity_groups <- aggregate(
      cbind(avg_retention = retention, card_count = retention) ~ complexity_cat,
      data = cards_with_retention,
      FUN = function(x) c(mean(x, na.rm = TRUE), length(x))[1]
    )
    
    count_by_cat <- aggregate(cid ~ complexity_cat, data = cards_with_retention, FUN = length)
    names(count_by_cat) <- c("complexity_cat", "card_count")
    
    ret_by_cat <- aggregate(retention ~ complexity_cat, data = cards_with_retention, FUN = mean)
    
    complexity_retention <- merge(ret_by_cat, count_by_cat, by = "complexity_cat")
    complexity_retention$retention <- round(complexity_retention$retention * 100, 1)
    
    # Correlation
    retention_cor <- cor(cards_with_retention$complexity, 
                         cards_with_retention$retention, 
                         use = "complete.obs")
  } else {
    complexity_retention <- tibble::tibble()
    retention_cor <- NA
  }
  
  # Word count distribution
  word_count_dist <- tibble::tibble(
    range = c("1-10", "11-25", "26-50", "51-100", "100+"),
    count = c(
      sum(card_content$word_count >= 1 & card_content$word_count <= 10),
      sum(card_content$word_count >= 11 & card_content$word_count <= 25),
      sum(card_content$word_count >= 26 & card_content$word_count <= 50),
      sum(card_content$word_count >= 51 & card_content$word_count <= 100),
      sum(card_content$word_count > 100)
    )
  )
  
  # Media usage
  media_summary <- tibble::tibble(
    type = c("Text Only", "With Images", "With Audio", "With Both"),
    count = c(
      sum(card_content$img_count == 0 & card_content$sound_count == 0),
      sum(card_content$img_count > 0 & card_content$sound_count == 0),
      sum(card_content$img_count == 0 & card_content$sound_count > 0),
      sum(card_content$img_count > 0 & card_content$sound_count > 0)
    )
  )
  
  list(
    summary = summary_stats,
    complexity_retention = tibble::as_tibble(complexity_retention),
    complexity_correlation = round(retention_cor, 3),
    word_count_distribution = word_count_dist,
    media_usage = media_summary,
    sample_size = nrow(card_content),
    recommendations = if (!is.na(retention_cor) && retention_cor < -0.1) {
      "Complex cards show lower retention - consider simplifying"
    } else if (!is.na(retention_cor) && retention_cor > 0.1) {
      "Complex cards are working well for you!"
    } else {
      "Card complexity doesn't strongly affect your retention"
    }
  )
}
