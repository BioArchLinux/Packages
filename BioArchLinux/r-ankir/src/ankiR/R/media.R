#' List media files in collection
#'
#' Returns all media files stored in the collection's media folder.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with filename, size, and extension
#' @export
#' @examples
#' \dontrun{
#' media <- anki_media_list()
#' }
anki_media_list <- function(path = NULL, profile = NULL) {
  if (is.null(path)) {
    path <- anki_db_path(profile)
  }
 
  media_dir <- file.path(dirname(path), "collection.media")
 
  if (!dir.exists(media_dir)) {
    message("Media directory not found: ", media_dir
)
    return(tibble::tibble(
      filename = character(),
      size = numeric(),
      extension = character()
    ))
  }
 
  files <- list.files(media_dir, full.names = TRUE)
 
  if (length(files) == 0) {
    return(tibble::tibble(
      filename = character(),
      size = numeric(),
      extension = character()
    ))
  }
 
  file_info <- file.info(files)
 
  tibble::tibble(
    filename = basename(files),
    size = file_info$size,
    extension = tolower(tools::file_ext(files)),
    modified = file_info$mtime
  )
}

#' Find unused media files
#'
#' Identifies media files that are not referenced in any note.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble of unused media files
#' @export
#' @examples
#' \dontrun{
#' unused <- anki_media_unused()
#' }
anki_media_unused <- function(path = NULL, profile = NULL) {
  if (is.null(path)) {
    path <- anki_db_path(profile)
  }
 
  # Get all media files
  media <- anki_media_list(path = path)
 
  if (nrow(media) == 0) {
    return(media)
  }
 
  # Get all notes
  notes <- anki_notes(path = path)
 
  # Find referenced media in note fields
  all_fields <- paste(notes$flds, collapse = " ")
 
  # Check which media files are referenced
  used <- vapply(media$filename, function(f) {
    grepl(f, all_fields, fixed = TRUE)
  }, FUN.VALUE = logical(1))
 
  media[!used, ]
}

#' Find missing media references
#'
#' Identifies media files referenced in notes but not present in the media folder.
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A tibble with missing filenames and the notes referencing them
#' @export
#' @examples
#' \dontrun{
#' missing <- anki_media_missing()
#' }
anki_media_missing <- function(path = NULL, profile = NULL) {
  if (is.null(path)) {
    path <- anki_db_path(profile)
  }
 
  # Get existing media files
  media <- anki_media_list(path = path)
  existing_files <- media$filename
 
  # Get all notes
  notes <- anki_notes(path = path)
 
  # Extract media references from fields
  # Common patterns: <img src="..."> and [sound:...]
  all_refs <- character()
 
  for (i in seq_len(nrow(notes))) {
    flds <- notes$flds[i]
   
    # Image references
    img_matches <- regmatches(flds, gregexpr('src="([^"]+)"', flds))[[1]]
    img_files <- gsub('src="|"', '', img_matches)
   
    # Sound references
    sound_matches <- regmatches(flds, gregexpr('\\[sound:([^\\]]+)\\]', flds))[[1]]
    sound_files <- gsub('\\[sound:|\\]', '', sound_matches)
   
    all_refs <- c(all_refs, img_files, sound_files)
  }
 
  # Find unique references
  unique_refs <- unique(all_refs[all_refs != ""])
 
  # Filter to missing files
  missing <- unique_refs[!unique_refs %in% existing_files]
 
  # Exclude URLs
  missing <- missing[!grepl("^https?://", missing)]
 
  if (length(missing) == 0) {
    return(tibble::tibble(filename = character()))
  }
 
  tibble::tibble(filename = missing)
}

#' Get media folder path
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return Character string path to collection.media folder
#' @export
#' @examples
#' \dontrun{
#' anki_media_path()
#' }
anki_media_path <- function(path = NULL, profile = NULL) {
  if (is.null(path)) {
    path <- anki_db_path(profile)
  }
  file.path(dirname(path), "collection.media")
}

#' Get media statistics
#'
#' @param path Path to collection.anki2 (auto-detected if NULL)
#' @param profile Profile name (first profile if NULL)
#' @return A list with media statistics
#' @export
#' @examples
#' \dontrun{
#' stats <- anki_media_stats()
#' }
anki_media_stats <- function(path = NULL, profile = NULL) {
  media <- anki_media_list(path, profile)
 
  if (nrow(media) == 0) {
    return(list(
      total_files = 0L,
      total_size_mb = 0,
      by_extension = tibble::tibble(extension = character(), count = integer(), size_mb = numeric())
    ))
  }
 
  by_ext <- lapply(split(media, media$extension), function(m) {
    tibble::tibble(
      count = nrow(m),
      size_mb = sum(m$size) / 1024 / 1024
    )
  })
 
  by_ext_df <- do.call(rbind, by_ext)
  by_ext_df$extension <- names(by_ext)
  by_ext_df <- by_ext_df[order(-by_ext_df$size_mb), ]
 
  list(
    total_files = nrow(media),
    total_size_mb = sum(media$size) / 1024 / 1024,
    by_extension = tibble::as_tibble(by_ext_df[, c("extension", "count", "size_mb")])
  )
}
