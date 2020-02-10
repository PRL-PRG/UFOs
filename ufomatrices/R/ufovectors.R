ufo_matrix_set_debug_mode <- function(debug=TRUE) {
  if (typeof(debug) != "logical") {
    stop(paste0("Argument is of type", typeof(debug),
    ", must be a logical vector."))
  }
  if (length(debug) == 0) {
    stop("Argument is a 0-length vector")
  }
  if (length(debug) > 1) {
    warning(paste0("Argument is a vector containing multiple values, picking the",
    "first one, ignoring the rest"))
  }
  invisible(.Call("ufo_matrix_set_debug_mode", debug))
}

ufo_matrix_bin <- function(type, path) {
  if (missing(type)) stop("Missing matrix type.")

  if (type == "integer") return(ufo_matrix_integer_bin(path))
  if (type == "numeric") return(ufo_matrix_numeric_bin(path))
  if (type == "complex") return(ufo_matrix_complex_bin(path))
  if (type == "logical") return(ufo_matrix_logical_bin(path))
  if (type == "raw")     return(ufo_matrix_raw_bin(path))

  stop(paste0("Unknown UFO matrix type: ", type))
}

ufo_matrix_integer_bin <- function(path) {
  .check_path(path)
  .Call("ufo_matrix_intsxp_bin", path.expand(path))
}

ufo_matrix_numeric_bin <- function(path) {
  .check_path(path)
  .Call("ufo_matrix_realsxp_bin", path.expand(path))
}

# ufo_character_bin <- function(path) {
#   .check_path(path)
#   .Call("ufo_vectors_strsxp_bin", path)
# }

ufo_matrix_complex_bin <- function(path) {
  .check_path(path)
  .Call("ufo_matrix_cplxsxp_bin", path.expand(path))
}

ufo_matrix_logical_bin <- function(path) {
  .check_path(path)
  .Call("ufo_matrix_lglsxp_bin", path.expand(path))
}

ufo_matrix_raw_bin <- function(path) {
  .check_path(path)
  .Call("ufo_matrix_rawsxp_bin", path.expand(path))
}

ufo_matrix_shutdown <- function() {
  invisible(.Call("ufo_matrix_shutdown"))
}

.check_path <- function(path) {
  if (typeof(path) != "character") {
    stop(paste0("Path is of type", typeof(path),
    ", must be a character vector."))
  }
  if (length(path) == 0) {
    stop("Path is a 0-length vector")
  }
  if (length(path) > 1) {
    warning(paste0("Path is a vector containing multiple values, picking the",
    "first one, ignoring the rest"))
  }
  if (!file.exists(path)) {
    stop(paste0("File '", path, "' does not exist."))
  }
  if (!file_test("-f", path)) {
    stop(paste0("File '", path, "' exists but is not a file."))
  }
  if (0 != file.access(path, 4)) { # 0: existence, 1: execute, 2: write, 4: read
    stop(paste0("File '", path, "' exists but is not readable."))
  }
  if (0 != file.access(path, 2)) { # 0: existence, 1: execute, 2: write, 4: read
    warning(paste0("File '", path, "' exists but is not writeable."))
  }
}

ufo_matrix_store_bin <- function(path, vector) {
  if (typeof(path) != "character") {
    stop(paste0("Path is of type", typeof(path),
    ", must be a character vector."))
  }
  if (length(path) == 0) {
    stop("Path is a 0-length vector")
  }
  if (length(path) > 1) {
    warning(paste0("Path is a vector containing multiple values, picking the",
    "first one, ignoring the rest"))
  }

  invisible(.Call("ufo_matrix_store_bin", path, vector))
}

