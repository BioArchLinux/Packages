#' Export to Obsidian Spaced Repetition plugin format
#'
#' Exports cards in a format compatible with the Obsidian Spaced Repetition plugin.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param output_dir Directory for output files (default: current directory)
#' @param model_id Optional model ID to export (NULL for all)
#' @param deck_pattern Optional pattern to filter decks
#' @param include_scheduling Include scheduling info as YAML frontmatter
#' @return Invisibly returns the number of cards exported
#' @export
#' @examples
#' \dontrun{
#' anki_to_obsidian_sr(output_dir = "~/obsidian/vault/flashcards")
#' }
anki_to_obsidian_sr <- function(path = NULL, profile = NULL,
                                 output_dir = ".", model_id = NULL,
                                 deck_pattern = NULL, include_scheduling = FALSE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  notes <- col$notes()
  cards <- col$cards()
  decks <- col$decks()
  models <- col$models()

  # Filter by model
  if (!is.null(model_id)) {
    notes <- notes[notes$mid == model_id, ]
  }

  # Filter by deck
  if (!is.null(deck_pattern)) {
    relevant_decks <- decks$did[grepl(deck_pattern, decks$name, ignore.case = TRUE)]
    cards <- cards[cards$did %in% relevant_decks, ]
    notes <- notes[notes$nid %in% cards$nid, ]
  }

  if (nrow(notes) == 0) {
    message("No matching notes found")
    return(invisible(0))
  }

  # Create output directory
  dir.create(output_dir, recursive = TRUE, showWarnings = FALSE)

  # Get model info for field names
  model_fields <- lapply(models$mid, function(mid) {
    idx <- which(models$mid == mid)
    if (length(idx) > 0 && length(models$flds[[idx]]) > 0) {
      models$flds[[idx]]
    } else {
      c("Front", "Back")
    }
  })
  names(model_fields) <- models$mid

  # Process each note
  exported <- 0

  for (i in seq_len(nrow(notes))) {
    note <- notes[i, ]

    # Get fields
    fields <- strsplit(note$flds, "\x1f")[[1]]

    # Get field names
    fnames <- model_fields[[as.character(note$mid)]]
    if (is.null(fnames)) fnames <- paste0("Field", seq_along(fields))

    # Clean fields
    clean_fields <- lapply(fields, function(f) {
      # Remove HTML (keep basic formatting)
      f <- gsub("<br\\s*/?>", "\n", f, ignore.case = TRUE)
      f <- gsub("</?p>", "\n", f, ignore.case = TRUE)
      f <- gsub("<b>([^<]*)</b>", "**\\1**", f, ignore.case = TRUE)
      f <- gsub("<i>([^<]*)</i>", "*\\1*", f, ignore.case = TRUE)
      f <- gsub("<[^>]+>", "", f)
      f <- gsub("&nbsp;", " ", f)
      f <- gsub("&amp;", "&", f)
      f <- gsub("&lt;", "<", f)
      f <- gsub("&gt;", ">", f)
      trimws(f)
    })

    # Create Obsidian SR format
    # Format: Question::Answer or multi-line with :::
    if (length(clean_fields) >= 2 && nchar(clean_fields[[1]]) > 0) {
      front <- clean_fields[[1]]
      back <- clean_fields[[2]]

      # Check if multi-line needed
      if (grepl("\n", front) || grepl("\n", back)) {
        # Multi-line format
        content <- paste0(front, "\n?\n", back)
      } else {
        # Single-line format
        content <- paste0(front, "::", back)
      }

      # Add tags
      tags <- trimws(unlist(strsplit(note$tags, "\\s+")))
      tags <- tags[tags != ""]

      # Create frontmatter if needed
      frontmatter <- ""
      if (include_scheduling) {
        card_data <- cards[cards$nid == note$nid, ]
        if (nrow(card_data) > 0) {
          frontmatter <- paste0(
            "---\n",
            "sr-due: ", Sys.Date() + card_data$due[1], "\n",
            "sr-interval: ", card_data$ivl[1], "\n",
            "sr-ease: ", 250, "\n",
            "---\n\n"
          )
        }
      }

      # Add tags as Obsidian tags
      tag_str <- if (length(tags) > 0) {
        paste0("\n\n", paste0("#", gsub("::", "/", tags), collapse = " "))
      } else ""

      # Write file
      filename <- paste0("card_", note$nid, ".md")
      filepath <- file.path(output_dir, filename)

      writeLines(paste0(frontmatter, content, tag_str), filepath)
      exported <- exported + 1
    }
  }

  message("Exported ", exported, " cards to ", output_dir)
  invisible(exported)
}

#' Export to Mochi format
#'
#' Exports cards to Mochi's JSON import format.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param output_path Path for output JSON file
#' @param deck_pattern Optional pattern to filter decks
#' @return Invisibly returns the number of cards exported
#' @export
#' @examples
#' \dontrun{
#' anki_to_mochi(output_path = "mochi_import.json")
#' }
anki_to_mochi <- function(path = NULL, profile = NULL,
                          output_path = "mochi_cards.json",
                          deck_pattern = NULL) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  notes <- col$notes()
  cards <- col$cards()
  decks <- col$decks()
  models <- col$models()

  # Filter by deck
  if (!is.null(deck_pattern)) {
    relevant_decks <- decks$did[grepl(deck_pattern, decks$name, ignore.case = TRUE)]
    cards <- cards[cards$did %in% relevant_decks, ]
    notes <- notes[notes$nid %in% cards$nid, ]
  }

  if (nrow(notes) == 0) {
    message("No matching notes found")
    return(invisible(0))
  }

  # Mochi format
  mochi_cards <- lapply(seq_len(nrow(notes)), function(i) {
    note <- notes[i, ]
    fields <- strsplit(note$flds, "\x1f")[[1]]

    if (length(fields) < 2) return(NULL)

    # Clean HTML
    clean_field <- function(f) {
      f <- gsub("<br\\s*/?>", "\n", f, ignore.case = TRUE)
      f <- gsub("</?p>", "\n", f, ignore.case = TRUE)
      f <- gsub("<[^>]+>", "", f)
      f <- gsub("&nbsp;", " ", f)
      f <- gsub("&amp;", "&", f)
      trimws(f)
    }

    front <- clean_field(fields[[1]])
    back <- clean_field(fields[[2]])

    # Get deck name
    card_data <- cards[cards$nid == note$nid, ]
    deck_name <- if (nrow(card_data) > 0) {
      decks$name[decks$did == card_data$did[1]]
    } else {
      "Default"
    }
    if (length(deck_name) == 0) deck_name <- "Default"

    # Get tags
    tags <- trimws(unlist(strsplit(note$tags, "\\s+")))
    tags <- tags[tags != ""]

    list(
      content = paste0("# ", front, "\n\n---\n\n", back),
      deck = deck_name,
      tags = as.list(tags)
    )
  })

  mochi_cards <- Filter(Negate(is.null), mochi_cards)

  # Write JSON
  json_content <- jsonlite::toJSON(
    list(cards = mochi_cards),
    auto_unbox = TRUE,
    pretty = TRUE
  )

  writeLines(json_content, output_path)

  message("Exported ", length(mochi_cards), " cards to ", output_path)
  invisible(length(mochi_cards))
}

#' Export collection as JSON
#'
#' Full collection export as structured JSON for web dashboards or custom apps.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param output_path Path for output JSON file
#' @param include_revlog Whether to include review log (can be large)
#' @param include_content Whether to include card content
#' @return Invisibly returns the exported data structure
#' @export
#' @examples
#' \dontrun{
#' anki_to_json(output_path = "collection.json")
#' }
anki_to_json <- function(path = NULL, profile = NULL,
                         output_path = "anki_collection.json",
                         include_revlog = TRUE,
                         include_content = TRUE) {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  # Gather all data
  export_data <- list(
    exported_at = as.character(Sys.time()),
    collection_path = col$path,

    summary = list(
      total_notes = nrow(col$notes()),
      total_cards = nrow(col$cards()),
      total_decks = nrow(col$decks()),
      total_reviews = nrow(col$revlog())
    ),

    decks = as.list(col$decks()),
    models = as.list(col$models())
  )

  if (include_content) {
    notes <- col$notes()
    cards <- col$cards()

    # Clean up fields
    notes$field_list <- lapply(strsplit(notes$flds, "\x1f"), function(f) {
      lapply(f, function(x) gsub("<[^>]+>", "", x))
    })
    notes$flds <- NULL  # Remove raw field

    export_data$notes <- as.list(notes)
    export_data$cards <- as.list(cards)
  }

  if (include_revlog) {
    revlog <- col$revlog()
    # Sample if too large
    if (nrow(revlog) > 100000) {
      message("Sampling review log (", nrow(revlog), " entries)")
      revlog <- revlog[sample(nrow(revlog), 100000), ]
    }
    export_data$revlog <- as.list(revlog)
  }

  # Add analytics
  export_data$analytics <- list(
    daily_stats = as.list(head(anki_stats_daily(path = col$path), 90)),
    deck_stats = as.list(anki_stats_deck(path = col$path)),
    streak = anki_streak(path = col$path)
  )

  # Write JSON
  json_content <- jsonlite::toJSON(
    export_data,
    auto_unbox = TRUE,
    pretty = TRUE
  )

  writeLines(json_content, output_path)

  message("Exported collection to ", output_path)
  invisible(export_data)
}

#' Generate progress report
#'
#' Generates a shareable HTML or Markdown progress report with charts.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param output_path Path for output file
#' @param format Output format: "html" or "markdown"
#' @param period Period to analyze: "week", "month", "year"
#' @param title Report title
#' @return Invisibly returns the report content
#' @export
#' @examples
#' \dontrun{
#' anki_progress_report(output_path = "my_progress.html", period = "month")
#' }
anki_progress_report <- function(path = NULL, profile = NULL,
                                  output_path = "anki_progress.html",
                                  format = "html", period = "month",
                                  title = "Anki Progress Report") {
  col <- anki_collection(path, profile)
  on.exit(col$close())

  # Determine date range
  days <- switch(period,
    "week" = 7,
    "month" = 30,
    "year" = 365,
    30
  )

  from_date <- Sys.Date() - days

  # Gather statistics
  cards <- col$cards()
  revlog <- col$revlog()
  recent_revlog <- revlog[revlog$review_date >= from_date, ]

  # Summary stats
  total_cards <- nrow(cards)
  total_reviews <- nrow(recent_revlog)
  total_time_hrs <- round(sum(recent_revlog$time, na.rm = TRUE) / 3600000, 1)
  retention <- round((1 - sum(recent_revlog$ease == 1) / max(nrow(recent_revlog), 1)) * 100, 1)
  days_studied <- length(unique(recent_revlog$review_date))

  streak <- anki_streak(path = col$path)

  # Daily stats
  daily <- anki_stats_daily(path = col$path, from = from_date)

  if (format == "html") {
    # Generate HTML report
    html_content <- paste0(
      '<!DOCTYPE html>\n<html>\n<head>\n',
      '<meta charset="UTF-8">\n',
      '<title>', title, '</title>\n',
      '<style>\n',
      'body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; ',
      'max-width: 800px; margin: 40px auto; padding: 20px; line-height: 1.6; }\n',
      'h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }\n',
      'h2 { color: #34495e; margin-top: 30px; }\n',
      '.stat-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px; margin: 20px 0; }\n',
      '.stat-card { background: #f8f9fa; border-radius: 8px; padding: 20px; text-align: center; }\n',
      '.stat-value { font-size: 32px; font-weight: bold; color: #3498db; }\n',
      '.stat-label { color: #7f8c8d; font-size: 14px; }\n',
      'table { width: 100%; border-collapse: collapse; margin: 20px 0; }\n',
      'th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }\n',
      'th { background: #f8f9fa; }\n',
      '.good { color: #27ae60; }\n',
      '.warning { color: #f39c12; }\n',
      '.footer { margin-top: 40px; padding-top: 20px; border-top: 1px solid #ddd; ',
      'color: #7f8c8d; font-size: 12px; }\n',
      '</style>\n</head>\n<body>\n',

      '<h1>', title, '</h1>\n',
      '<p>Period: ', format(from_date, "%B %d, %Y"), ' to ', format(Sys.Date(), "%B %d, %Y"), '</p>\n',

      '<div class="stat-grid">\n',
      '<div class="stat-card"><div class="stat-value">', total_reviews, '</div>',
      '<div class="stat-label">Reviews</div></div>\n',
      '<div class="stat-card"><div class="stat-value">', total_time_hrs, 'h</div>',
      '<div class="stat-label">Study Time</div></div>\n',
      '<div class="stat-card"><div class="stat-value ', if(retention >= 85) "good" else "warning", '">',
      retention, '%</div><div class="stat-label">Retention</div></div>\n',
      '</div>\n',

      '<div class="stat-grid">\n',
      '<div class="stat-card"><div class="stat-value">', days_studied, '</div>',
      '<div class="stat-label">Days Studied</div></div>\n',
      '<div class="stat-card"><div class="stat-value">', streak$current_streak, '</div>',
      '<div class="stat-label">Current Streak</div></div>\n',
      '<div class="stat-card"><div class="stat-value">', streak$longest_streak, '</div>',
      '<div class="stat-label">Longest Streak</div></div>\n',
      '</div>\n',

      '<h2>Collection Overview</h2>\n',
      '<table>\n',
      '<tr><th>Metric</th><th>Value</th></tr>\n',
      '<tr><td>Total Cards</td><td>', total_cards, '</td></tr>\n',
      '<tr><td>Mature Cards</td><td>', sum(cards$ivl >= 21), '</td></tr>\n',
      '<tr><td>New Cards</td><td>', sum(cards$type == 0), '</td></tr>\n',
      '<tr><td>Suspended</td><td>', sum(cards$queue == -1), '</td></tr>\n',
      '</table>\n',

      '<div class="footer">\n',
      'Generated by ankiR on ', format(Sys.time(), "%Y-%m-%d %H:%M"), '\n',
      '</div>\n',

      '</body>\n</html>'
    )

    writeLines(html_content, output_path)

  } else {
    # Markdown format
    md_content <- paste0(
      '# ', title, '\n\n',
      '**Period:** ', format(from_date, "%B %d, %Y"), ' to ', format(Sys.Date(), "%B %d, %Y"), '\n\n',

      '## Summary\n\n',
      '| Metric | Value |\n',
      '|--------|-------|\n',
      '| Reviews | ', total_reviews, ' |\n',
      '| Study Time | ', total_time_hrs, ' hours |\n',
      '| Retention | ', retention, '% |\n',
      '| Days Studied | ', days_studied, ' |\n',
      '| Current Streak | ', streak$current_streak, ' |\n',
      '| Longest Streak | ', streak$longest_streak, ' |\n\n',

      '## Collection Overview\n\n',
      '| Metric | Value |\n',
      '|--------|-------|\n',
      '| Total Cards | ', total_cards, ' |\n',
      '| Mature Cards | ', sum(cards$ivl >= 21), ' |\n',
      '| New Cards | ', sum(cards$type == 0), ' |\n',
      '| Suspended | ', sum(cards$queue == -1), ' |\n\n',

      '---\n',
      '*Generated by ankiR on ', format(Sys.time(), "%Y-%m-%d %H:%M"), '*\n'
    )

    writeLines(md_content, output_path)
  }

  message("Generated report: ", output_path)
  invisible(if (format == "html") html_content else md_content)
}
