#' Convert Anki timestamp to datetime
#'
#' Converts Anki's millisecond-epoch timestamps to POSIXct datetime objects.
#'
#' @param x Numeric timestamp in milliseconds since Unix epoch (1970-01-01)
#' @return POSIXct datetime object in local timezone
#' @export
#' @examples
#' # Convert a typical Anki timestamp
#' anki_timestamp_to_datetime(1368291917470)
anki_timestamp_to_datetime <- function(x) {
  as.POSIXct(x / 1000, origin = "1970-01-01")
}

#' Convert Anki timestamp to date
#'
#' Converts Anki's millisecond-epoch timestamps to Date objects.
#'
#' @param x Numeric timestamp in milliseconds since Unix epoch (1970-01-01)
#' @return Date object
#' @export
#' @examples
#' # Convert a typical Anki timestamp
#' anki_timestamp_to_date(1368291917470)
anki_timestamp_to_date <- function(x) {
  as.Date(anki_timestamp_to_datetime(x))
}

#' Convert date to Anki timestamp
#'
#' Converts a Date or POSIXct to Anki's millisecond-epoch format.
#'
#' @param x Date or POSIXct object
#' @return Numeric timestamp in milliseconds since Unix epoch
#' @export
#' @examples
#' date_to_anki_timestamp(Sys.Date())
date_to_anki_timestamp <- function(x) {
 as.numeric(as.POSIXct(x)) * 1000
}

#' Null coalescing operator
#'
#' Returns the left-hand side if not NULL, otherwise returns the right-hand side.
#' @param a Value to check
#' @param b Default value if a is NULL
#' @return a if not NULL, otherwise b
#' @keywords internal
#' @noRd
`%||%` <- function(a, b) {
  if (is.null(a)) b else a
}

# Declare global variables to avoid R CMD check notes
# These are column names used in ggplot2 aes() and data.frame operations
utils::globalVariables(c(".data", 
  "difficulty", "hour", "ivl", "name", "new_due", "quantile",
  "reorder", "retention", "reviews", "rolling_retention", "sd",
  "stability", "total", "week", "weekday", "date", "value",
  "component", "cid", "did", "nid", "ease", "time", "review_date",
  "mid", "sfld", "flds", "tags", "lapses", "type", "queue",
  "retrievability", "decay", "last_review", "period", "age_group"
))

# Import .data from rlang for ggplot2 tidy evaluation
#' @importFrom rlang .data
NULL
