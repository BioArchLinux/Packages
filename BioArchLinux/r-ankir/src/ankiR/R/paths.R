#' Get Anki base path
#'
#' Returns the default Anki2 directory for the current platform.
#'
#' @return Character string path to Anki2 directory
#' @export
#' @examples
#' \dontrun{
#' anki_base_path()
#' }
anki_base_path <- function() {
  if (.Platform$OS.type == "windows") {
    file.path(Sys.getenv("APPDATA"), "Anki2")
  } else if (Sys.info()["sysname"] == "Darwin") {
    path.expand("~/Library/Application Support/Anki2")
  } else {
    # Linux and other Unix-like systems
    # Check XDG_DATA_HOME first, fall back to default
    xdg_data <- Sys.getenv("XDG_DATA_HOME", "")
    if (xdg_data != "" && dir.exists(file.path(xdg_data, "Anki2"))) {
      file.path(xdg_data, "Anki2")
    } else {
      path.expand("~/.local/share/Anki2")
    }
  }
}

#' List Anki profiles
#'
#' Returns the names of all Anki profiles found in the Anki2 directory.
#'
#' @param base_path Path to Anki2 directory (auto-detected if NULL)
#' @return Character vector of profile names
#' @export
#' @examples
#' \dontrun{
#' anki_profiles()
#' }
anki_profiles <- function(base_path = NULL) {
  if (is.null(base_path)) base_path <- anki_base_path()

  if (!dir.exists(base_path)) {
    stop("Anki directory not found: ", base_path, "\n",
         "Is Anki installed?", call. = FALSE)
  }

  dirs <- list.dirs(base_path, full.names = FALSE, recursive = FALSE)

  # Filter to directories containing a collection database
  profiles <- dirs[vapply(dirs, function(d) {
    file.exists(file.path(base_path, d, "collection.anki2")) ||
      file.exists(file.path(base_path, d, "collection.anki21"))
  }, FUN.VALUE = logical(1))]

  # Exclude addon directories
  profiles <- profiles[!profiles %in% c("addons", "addons21")]

  if (length(profiles) == 0) {
    stop("No Anki profiles found in: ", base_path, call. = FALSE)
  }

  profiles
}

#' Get path to Anki database
#'
#' Returns the full path to an Anki collection database file.
#'
#' @param profile Profile name. If NULL, uses the first available profile.
#' @param base_path Path to Anki2 directory (auto-detected if NULL)
#' @return Character string path to collection.anki2 or collection.anki21
#' @export
#' @examples
#' \dontrun{
#' anki_db_path()
#' anki_db_path("User 1")
#' }
anki_db_path <- function(profile = NULL, base_path = NULL) {
  if (is.null(base_path)) base_path <- anki_base_path()

  if (is.null(profile)) {
    profiles <- anki_profiles(base_path)
    profile <- profiles[1]
    message("Using profile: ", profile)
  }

  # Check for both database versions
  db <- file.path(base_path, profile, "collection.anki2")
  if (!file.exists(db)) {
    db <- file.path(base_path, profile, "collection.anki21")
  }

  if (!file.exists(db)) {
    stop("Database not found for profile '", profile, "' in: ", base_path,
         call. = FALSE)
  }

  db
}
