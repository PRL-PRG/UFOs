altrep_ufo_set_debug_mode <- function(debug=TRUE) {
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

altrep_ufo_vector_bin <- function(type, path) {
  if (missing(type)) stop("Missing vector type.")
  
  if (type == "integer") return(altrep_ufo_integer_bin(path))
  if (type == "numeric") return(altrep_ufo_numeric_bin(path))
  if (type == "complex") return(altrep_ufo_complex_bin(path))
  if (type == "logical") return(altrep_ufo_logical_bin(path))
  if (type == "raw")     return(altrep_ufo_raw_bin(path))
  
  stop(paste0("Unknown UFO vector type: ", type))
}

altrep_integer_seq <- function(from, to, by=1) {
  .Call("altrep_intsxp_seq", as.integer(from), as.integer(to), as.integer(by))
}

altrep_numeric_seq <- function(from, to, by=1) {
  .Call("altrep_realsxp_seq", as.integer(from), as.integer(to), as.integer(by))
}


altrep_ufo_integer_bin <- function(path) {
  .check_path(path)
  .Call("altrep_ufo_vectors_intsxp_bin", path.expand(path))
}

altrep_ufo_numeric_bin <- function(path) {
  .check_path(path)
  .Call("altrep_ufo_vectors_realsxp_bin", path.expand(path))
}

# altrep_ufo_character_bin <- function(path) {
#   .check_path(path)
#   .Call("ufo_vectors_strsxp_bin", path)
# }

altrep_ufo_complex_bin <- function(path) {
  .check_path(path)
  .Call("altrep_ufo_vectors_cplxsxp_bin", path.expand(path))
}

altrep_ufo_logical_bin <- function(path) {
  .check_path(path)
  .Call("altrep_ufo_vectors_lglsxp_bin", path.expand(path))
}

altrep_ufo_raw_bin <- function(path) {
  .check_path(path)
  .Call("altrep_ufo_vectors_rawsxp_bin", path.expand(path))
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

altrep_ufo_matrix_integer_bin <- function(path, nrows, ncols) {
  .check_path(path)
  .Call("altrep_ufo_matrix_intsxp_bin", path.expand(path),
                                        as.integer(nrows),
                                        as.integer(ncols))
}

altrep_ufo_matrix_numeric_bin <- function(path, nrows, ncols) {
  .check_path(path)
  .Call("altrep_ufo_matrix_realsxp_bin", path.expand(path),
                                         as.integer(nrows),
                                         as.integer(ncols))
}

altrep_ufo_matrix_complex_bin <- function(path, nrows, ncols) {
  .check_path(path)
  .Call("altrep_ufo_matrix_cplxsxp_bin", path.expand(path),
                                         as.integer(nrows),
                                         as.integer(ncols))
}

altrep_ufo_matrix_logical_bin <- function(path, nrows, ncols) {
  .check_path(path)
  .Call("altrep_ufo_matrix_lglsxp_bin", path.expand(path),
                                        as.integer(nrows),
                                        as.integer(ncols))
}

altrep_ufo_matrix_raw_bin <- function(path, nrows, ncols) {
  .check_path(path)
  .Call("altrep_ufo_matrix_rawsxp_bin", path.expand(path),
                                        as.integer(nrows),
                                        as.integer(ncols))
}

altrep_ufo_matrix_vector_bin <- function(type, path, nrows, ncols) {
  if (missing(type)) stop("Missing matrix type.")

  if (type == "integer") return(altrep_ufo_matrix_integer_bin(path, nrows, ncols))
  if (type == "numeric") return(altrep_ufo_matrix_numeric_bin(path, nrows, ncols))
  if (type == "complex") return(altrep_ufo_matrix_complex_bin(path, nrows, ncols))
  if (type == "logical") return(altrep_ufo_matrix_logical_bin(path, nrows, ncols))
  if (type == "raw")     return(altrep_ufo_matrix_raw_bin(path, nrows, ncols))

  stop(paste0("Unknown UFO matrix type: ", type))
}