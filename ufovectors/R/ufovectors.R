ufo_integer_bin <- function(path, min_load_count = 0)
  .Call("ufo_vectors_intsxp_bin",
        path.expand(.check_path(path)),
        as.integer(min_load_count))

ufo_numeric_bin <- function(path, min_load_count = 0)
  .Call("ufo_vectors_realsxp_bin",
        path.expand(.check_path(path)),
        as.integer(min_load_count))

ufo_complex_bin <- function(path, min_load_count = 0)
  .Call("ufo_vectors_cplxsxp_bin",
        path.expand(.check_path(path)),
        as.integer(min_load_count))

ufo_logical_bin <- function(path, min_load_count = 0)
  .Call("ufo_vectors_lglsxp_bin",
        path.expand(.check_path(path)),
        as.integer(min_load_count))

ufo_raw_bin <- function(path, min_load_count = 0)
  .Call("ufo_vectors_rawsxp_bin",
        path.expand(.check_path(path)),
        as.integer(min_load_count))

ufo_matrix_integer_bin <- function(path, rows, cols, min_load_count = 0)
  .Call("ufo_matrix_intsxp_bin",
        path.expand(.check_path(path)),
        as.integer(rows),
        as.integer(cols),
        as.integer(min_load_count))

ufo_matrix_numeric_bin <- function(path, rows, cols, min_load_count = 0)
  .Call("ufo_matrix_realsxp_bin",
        path.expand(.check_path(path)),
        as.integer(rows),
        as.integer(cols),
        as.integer(min_load_count))

ufo_matrix_complex_bin <- function(path, rows, cols, min_load_count = 0)
  .Call("ufo_matrix_cplxsxp_bin",
        path.expand(.check_path(path)),
        as.integer(rows),
        as.integer(cols),
        as.integer(min_load_count))

ufo_matrix_logical_bin <- function(path, rows, cols, min_load_count = 0)
  .Call("ufo_matrix_lglsxp_bin",
        path.expand(.check_path(path)),
        as.integer(rows),
        as.integer(cols),
        as.integer(min_load_count))

ufo_matrix_raw_bin <- function(path, rows, cols, min_load_count = 0)
  .Call("ufo_matrix_rawsxp_bin",
        path.expand(.check_path(path)),
        as.integer(rows),
        as.integer(cols),
        as.integer(min_load_count))

ufo_vector_bin <- function(type, path, min_load_count = 0) {
  if (missing(type)) stop("Missing vector type.")

  if (type == "integer") return(ufo_integer_bin(path, min_load_count))
  if (type == "numeric") return(ufo_numeric_bin(path, min_load_count))
  if (type == "complex") return(ufo_complex_bin(path, min_load_count))
  if (type == "logical") return(ufo_logical_bin(path, min_load_count))
  if (type == "raw")     return(ufo_raw_bin(path))

  stop(paste0("Unknown UFO vector type: ", type))
}

ufo_matrix_bin <- function(type, path, rows, cols, min_load_count = 0) {
  if (missing(type)) stop("Missing matrix type.")

  if (type == "integer") return(ufo_matrix_integer_bin(path), rows, cols, min_load_count)
  if (type == "numeric") return(ufo_matrix_numeric_bin(path), rows, cols, min_load_count)
  if (type == "complex") return(ufo_matrix_complex_bin(path), rows, cols, min_load_count)
  if (type == "logical") return(ufo_matrix_logical_bin(path), rows, cols, min_load_count)
  if (type == "raw")     return(ufo_matrix_raw_bin(path), rows, cols, min_load_count)

  stop(paste0("Unknown UFO matrix type: ", type))
}

ufo_set_debug_mode <- function(debug=TRUE) {
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
  invisible(.Call("ufo_vectors_set_debug_mode", debug))
}

ufo_shutdown <- function() {
  invisible(.Call("ufo_vectors_shutdown"))
}

ufo_store_bin <- function(path, vector) {
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

  invisible(.Call("ufo_store_bin", path, vector))
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
  path
}