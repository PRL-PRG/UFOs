.check_path <- function(path) {
    name <- substitute(path)
    if (typeof(path) != "character") {
        stop(paste0("Path specified by `", name, "` is of type", typeof(path),
        ", must be a character vector."))
    }
    if (!file.exists(path)) {
        stop(paste0("File '", path, "' (specified by `", name, "`) does not exist."))
    }
    if (!file_test("-f", path)) {
        stop(paste0("File '", path, "' (specified by `", name, "`) exists but is not a file."))
    }
    if (0 != file.access(path, 4)) { # 0: existence, 1: execute, 2: write, 4: read
        stop(paste0("File '", path, "' (specified by `", name, "`) exists but is not readable."))
    }
    if (0 != file.access(path, 2)) { # 0: existence, 1: execute, 2: write, 4: read
        warning(paste0("File '", path, "' (specified by `", name, "`) exists but is not writeable."))
    }
    path
}

.expect_exactly_one <- function(vector, name=substitute(vector)) {
  if (length(vector) > 1) {
    warning(paste0("`", name, "` ",
                   "is a vector containing multiple values, ",
                   "picking the first one, ignoring the rest"))
  }
  if (length(vector) == 0) {
    stop(paste0("`", name, "` ",
                "is a zero-length vector, ",
                "but it should be a vector containing a single value"))
  }
  vector
}

.expect_type <- function(vector, expected_type, name=substitute(vector)) {
    if (typeof(vector) != expected_type) {
        stop(paste0("`", name, "` ",
                    "is a vector of type `", typeof(vector), "` ",
                    "but a vector of type `", expected_type, "` was found"))
    }
    vector
}


ufo_integer_bin <- function(path, min_load_count = 0) {
  .Call("ufo_vectors_intsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_numeric_bin <- function(path, min_load_count = 0) {
  .Call("ufo_vectors_realsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_complex_bin <- function(path, min_load_count = 0) {
  .Call("ufo_vectors_cplxsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_logical_bin <- function(path, min_load_count = 0) {
  .Call("ufo_vectors_lglsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_raw_bin <- function(path, min_load_count = 0) {
  .Call("ufo_vectors_rawsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_matrix_integer_bin <- function(path, rows, cols, min_load_count = 0) {
  .Call("ufo_matrix_intsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(rows)),
        as.integer(.expect_exactly_one(cols)),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_matrix_numeric_bin <- function(path, rows, cols, min_load_count = 0) {
  .Call("ufo_matrix_realsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(rows)),
        as.integer(.expect_exactly_one(cols)),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_matrix_complex_bin <- function(path, rows, cols, min_load_count = 0) {
  .Call("ufo_matrix_cplxsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(rows)),
        as.integer(.expect_exactly_one(cols)),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_matrix_logical_bin <- function(path, rows, cols, min_load_count = 0) {
  .Call("ufo_matrix_lglsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(rows)),
        as.integer(.expect_exactly_one(cols)),
        as.integer(.expect_exactly_one(min_load_count)))
}

ufo_matrix_raw_bin <- function(path, rows, cols, min_load_count = 0) {
  .Call("ufo_matrix_rawsxp_bin",
        path.expand(.check_path(.expect_exactly_one(path))),
        as.integer(.expect_exactly_one(rows)),
        as.integer(.expect_exactly_one(cols)),
        as.integer(.expect_exactly_one(min_load_count)))
}

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

ufo_csv <- function(path, min_load_count = 0, check_names=T, header=T, record_row_offsets_at_interval=1000, initial_buffer_size=32, col_names) {

  .expect_exactly_one(min_load_count)
  .expect_exactly_one(header)
  .expect_exactly_one(check_names)
  .expect_exactly_one(header)
  .expect_exactly_one(record_row_offsets_at_interval)
  .expect_exactly_one(initial_buffer_size)

  df <- .Call("ufo_csv",
              path.expand(.check_path(.expect_exactly_one(path))),                                      # SEXP/*STRSXP*/
              as.integer(.expect_exactly_one(min_load_count)),                                          # SEXP/*INTSXP*/
              as.logical(.expect_exactly_one(header)),                                                  # SEXP/*LGLSXP*/
              as.integer(.expect_exactly_one(record_row_offsets_at_interval)),                          # SEXP/*INTSXP*/
              as.integer(.expect_exactly_one(initial_buffer_size)))                                     # SEXP/*INTSXP*/

  if (!missing(col_names)) {
    names(df) <- col_names
  }
  else if (!header && missing(col_names)) {
    names(df) <- sapply(1:length(df), function(i) paste0("V", i))
  }
  if(check_names) {
    names(df) <- make.names(names(df), unique=T)
  }
  df
}
# todo row.names

ufo_set_debug_mode <- function(debug=TRUE) {
  invisible(.Call("ufo_vectors_set_debug_mode", .expect_exactly_one(.expect_type(debug, "logical"))))
}

ufo_store_bin <- function(path, vector) {
   invisible(.Call("ufo_store_bin", .check_path(.expect_exactly_one(path)), vector))
}
