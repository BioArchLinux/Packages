#' Calculate forgetting index
#'
#' Estimates the percentage of cards currently below target retention.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @param target_retention Target retention (default 0.9)
#' @return A list with forgetting analysis
#' @export
#' @examples
#' \dontrun{
#' fsrs_forgetting_index()
#' }
fsrs_forgetting_index <- function(path = NULL, profile = NULL, target_retention = 0.9) {
  cards <- fsrs_current_retrievability(path, profile)
 
  if (nrow(cards) == 0 || all(is.na(cards$retrievability))) {
    message("No FSRS data available")
    return(list(forgetting_index = NA))
  }
 
  cards <- cards[!is.na(cards$retrievability), ]
 
  # Calculate forgetting index
  below_target <- sum(cards$retrievability < target_retention)
  total <- nrow(cards)
  forgetting_index <- below_target / total * 100
 
  # Distribution
  dist <- list(
    below_70 = sum(cards$retrievability < 0.7) / total * 100,
    r70_80 = sum(cards$retrievability >= 0.7 & cards$retrievability < 0.8) / total * 100,
    r80_90 = sum(cards$retrievability >= 0.8 & cards$retrievability < 0.9) / total * 100,
    above_90 = sum(cards$retrievability >= 0.9) / total * 100
  )
 
  # Urgency analysis
  urgent_cards <- cards[cards$retrievability < 0.7, ]
 
  list(
    forgetting_index = round(forgetting_index, 1),
    target_retention = target_retention,
    cards_analyzed = total,
    cards_below_target = below_target,
    distribution = tibble::tibble(
      range = c("< 70%", "70-80%", "80-90%", "> 90%"),
      percentage = round(c(dist$below_70, dist$r70_80, dist$r80_90, dist$above_90), 1)
    ),
    avg_retrievability = round(mean(cards$retrievability), 3),
    min_retrievability = round(min(cards$retrievability), 3),
    urgent_cards_count = nrow(urgent_cards)
  )
}
