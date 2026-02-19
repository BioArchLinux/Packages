## ----include = FALSE----------------------------------------------------------
knitr::opts_chunk$set(
  collapse = TRUE,
  comment = "#>",
  eval = FALSE
)

## ----install------------------------------------------------------------------
# # From CRAN
# install.packages("ankiR")
# 
# # Or from GitHub for the development version
# remotes::install_github("chrislongros/ankiR")

## ----open---------------------------------------------------------------------
# library(ankiR)
# 
# # Auto-detect (uses first profile found)
# col <- anki_collection()
# 
# # Specify a profile
# col <- anki_collection(profile = "User 1")
# 
# # Or provide a path directly
# col <- anki_collection(path = "/path/to/collection.anki2")

## ----methods------------------------------------------------------------------
# notes <- col$notes()
# cards <- col$cards()
# reviews <- col$revlog()
# decks <- col$decks()
# models <- col$models()
# 
# # Always close when done
# col$close()

## ----convenience--------------------------------------------------------------
# # These are equivalent to opening, querying, and closing
# notes <- anki_notes()
# cards <- anki_cards()
# reviews <- anki_revlog()
# decks <- anki_decks()
# models <- anki_models()

## ----notes--------------------------------------------------------------------
# notes <- anki_notes()
# # nid: Note ID
# # mid: Model (note type) ID
# # tags: Space-separated tags
# # flds: Fields separated by \x1f character
# # sfld: Sort field (usually the front)

## ----cards--------------------------------------------------------------------
# cards <- anki_cards()
# # cid: Card ID
# # nid: Note ID (links to notes table)
# # did: Deck ID
# # type: 0=new, 1=learning, 2=review, 3=relearning
# # queue: -1=suspended, 0=new, 1=learning, 2=review
# # due: Due date/position
# # ivl: Current interval in days
# # reps: Number of reviews
# # lapses: Number of times forgotten

## ----decks--------------------------------------------------------------------
# decks <- anki_decks()
# # did: Deck ID
# # name: Deck name (includes parent::child hierarchy)

## ----revlog-------------------------------------------------------------------
# reviews <- anki_revlog()
# # rid: Review ID (timestamp in milliseconds)
# # cid: Card ID
# # ease: Button pressed (1=Again, 2=Hard, 3=Good, 4=Easy)
# # ivl: Interval after review
# # time: Time taken in milliseconds
# # review_date: Date of review

## ----fsrs---------------------------------------------------------------------
# cards_fsrs <- anki_cards_fsrs()
# 
# # Additional columns:
# # stability: Time in days for recall probability to drop to 90%
# # difficulty: How hard the card is (1-10)
# # desired_retention: Target recall probability
# # decay: FSRS-6 decay parameter (w20)

## ----retrievability-----------------------------------------------------------
# # For a card with 30-day stability, reviewed 15 days ago
# fsrs_retrievability(stability = 30, days_elapsed = 15)
# #> 0.946
# 
# # Using the per-card decay from FSRS-6
# fsrs_retrievability(stability = 30, days_elapsed = 15, decay = 0.3)

## ----intervals----------------------------------------------------------------
# # When should I review for 90% retention?
# fsrs_interval(stability = 30, desired_retention = 0.9)
# #> 30
# 
# # For 85% retention (more reviews, better memory)
# fsrs_interval(stability = 30, desired_retention = 0.85)
# #> 21.3

## ----analysis, message=FALSE--------------------------------------------------
# library(ankiR)
# library(dplyr)
# library(ggplot2)
# 
# # Get data
# reviews <- anki_revlog()
# cards <- anki_cards()
# decks <- anki_decks()
# 
# # Daily review count
# daily_reviews <- reviews |>
#   count(review_date, name = "reviews")
# 
# ggplot(daily_reviews, aes(review_date, reviews)) +
#   geom_col(fill = "steelblue") +
#   labs(title = "Daily Reviews", x = NULL, y = "Reviews") +
#   theme_minimal()
# 
# # Card maturity by deck
# cards |>
#   left_join(decks, by = "did") |>
#   filter(type == 2) |>  # Review cards only
#   group_by(name) |>
#   summarise(
#     cards = n(),
#     avg_interval = mean(ivl),
#     mature = sum(ivl >= 21),  # Cards with 21+ day interval
#     .groups = "drop"
#   ) |>
#   arrange(desc(cards))

## ----fsrs-analysis------------------------------------------------------------
# cards_fsrs <- anki_cards_fsrs()
# 
# # Distribution of stability values
# cards_fsrs |>
#   filter(!is.na(stability), stability > 0) |>
#   ggplot(aes(stability)) +
#   geom_histogram(bins = 50, fill = "steelblue") +
#   scale_x_log10() +
#   labs(
#     title = "Distribution of Card Stability",
#     x = "Stability (days, log scale)",
#     y = "Count"
#   ) +
#   theme_minimal()
# 
# # Difficulty vs Stability
# cards_fsrs |>
#   filter(!is.na(stability), !is.na(difficulty)) |>
#   ggplot(aes(difficulty, stability)) +
#   geom_point(alpha = 0.3) +
#   scale_y_log10() +
#   labs(
#     title = "Card Difficulty vs Stability",
#     x = "Difficulty (1-10)",
#     y = "Stability (days, log scale)"
#   ) +
#   theme_minimal()

