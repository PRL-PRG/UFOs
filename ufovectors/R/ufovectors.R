ufo_set_debug_mode <- function(debug=TRUE) {
  invisible(.Call(UFO_C_vectors_set_debug_mode, .expect_exactly_one(.expect_type(debug, "logical"))))
}

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

.add_class <- function(vector, cls, add_class, preserve_previous = TRUE) {
  if(add_class) {
    class(vector) <- if(preserve_previous) c(cls, attr(vector, "class")) else cls
  }
  return(vector)
}

.check_add_class <- function () isTRUE(getOption("ufovectors.add_class"))

ufo_integer_seq <- function(from, to, by = 1, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_intsxp_seq,
                    as.integer(from), as.integer(to), as.integer(by),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_numeric_seq <- function(from, to, by = 1, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_realsxp_seq,
                    as.integer(from), as.integer(to), as.integer(by),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_integer_bin <- function(path, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_vectors_intsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_numeric_bin <- function(path, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_vectors_realsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_complex_bin <- function(path, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_vectors_cplxsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_logical_bin <- function(path, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_vectors_lglsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_raw_bin <- function(path, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_vectors_rawsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_matrix_integer_bin <- function(path, rows, cols, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_matrix_intsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.integer(.expect_exactly_one(rows)),
                    as.integer(.expect_exactly_one(cols)),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class, preserve_previous = TRUE)
}

ufo_matrix_numeric_bin <- function(path, rows, cols, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_matrix_realsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.integer(.expect_exactly_one(rows)),
                    as.integer(.expect_exactly_one(cols)),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class, preserve_previous = TRUE)
}

ufo_matrix_complex_bin <- function(path, rows, cols, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_matrix_cplxsxp_bin,
                  path.expand(.check_path(.expect_exactly_one(path))),
                  as.integer(.expect_exactly_one(rows)),
                  as.integer(.expect_exactly_one(cols)),
                  as.logical(.expect_exactly_one(read_only)),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class, preserve_previous = TRUE)
}

ufo_matrix_logical_bin <- function(path, rows, cols, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_matrix_lglsxp_bin,
                  path.expand(.check_path(.expect_exactly_one(path))),
                  as.integer(.expect_exactly_one(rows)),
                  as.integer(.expect_exactly_one(cols)),

                  as.logical(.expect_exactly_one(read_only)),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class, preserve_previous = TRUE)
}

ufo_matrix_raw_bin <- function(path, rows, cols, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_matrix_rawsxp_bin,
                    path.expand(.check_path(.expect_exactly_one(path))),
                    as.integer(.expect_exactly_one(rows)),
                    as.integer(.expect_exactly_one(cols)),
                    as.logical(.expect_exactly_one(read_only)),
                    as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class, preserve_previous = TRUE)
}

ufo_vector_bin <- function(type, path, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  if (missing(type)) stop("Missing vector type.")

  if (type == "integer") return(ufo_integer_bin(path, read_only, min_load_count, add_class))
  if (type == "numeric" || type == "double") return(ufo_numeric_bin(path, read_only, min_load_count, add_class))
  if (type == "complex") return(ufo_complex_bin(path, read_only, min_load_count, add_class))
  if (type == "logical") return(ufo_logical_bin(path, read_only, min_load_count, add_class))
  if (type == "raw")     return(ufo_raw_bin    (path, read_only, min_load_count, add_class))

  stop(paste0("Unknown UFO vector type: ", type))
}

ufo_matrix_bin <- function(type, path, rows, cols, read_only = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  if (missing(type)) stop("Missing matrix type.")

  if (type == "integer") return(ufo_matrix_integer_bin(path), rows, cols, read_only, min_load_count, add_class)
  if (type == "numeric" || type == "double") return(ufo_matrix_numeric_bin(path), rows, cols, read_only, min_load_count, add_class)
  if (type == "complex") return(ufo_matrix_complex_bin(path), rows, cols, read_only, min_load_count, add_class)
  if (type == "logical") return(ufo_matrix_logical_bin(path), rows, cols, read_only, min_load_count, add_class)
  if (type == "raw")     return(ufo_matrix_raw_bin(path),     rows, cols, read_only, min_load_count, add_class)

  stop(paste0("Unknown UFO matrix type: ", type))
}

ufo_csv <- function(path, read_only = FALSE, min_load_count = 0, check_names=T, header=T, 
                    record_row_offsets_at_interval=1000, initial_buffer_size=32, col_names, 
                    add_class = .check_add_class()) {

  .expect_exactly_one(min_load_count)
  .expect_exactly_one(header)
  .expect_exactly_one(check_names)
  .expect_exactly_one(header)
  .expect_exactly_one(record_row_offsets_at_interval)
  .expect_exactly_one(initial_buffer_size)

  df <- .Call(UFO_C_csv,
              path.expand(.check_path(.expect_exactly_one(path))),                                      # SEXP/*STRSXP*/
              as.logical(.expect_exactly_one(read_only)),                                               # SEXP/*LGLP*/
              as.integer(.expect_exactly_one(min_load_count)),                                          # SEXP/*INTSXP*/
              as.logical(.expect_exactly_one(header)),                                                  # SEXP/*LGLSXP*/
              as.integer(.expect_exactly_one(record_row_offsets_at_interval)),                          # SEXP/*INTSXP*/
              as.integer(.expect_exactly_one(initial_buffer_size)),                                     # SEXP/*INTSXP*/
              as.logical(.expect_exactly_one(add_class)))                                               # SEXP/*LGLSXP*/

  if (!missing(col_names)) {
    names(df) <- col_names
  }
  else if (!header && missing(col_names)) {
    names(df) <- sapply(1:length(df), function(i) paste0("V", i))
  }
  if(check_names) {
    names(df) <- make.names(names(df), unique=T)
  }

  # handled internally, because it was misbehaving
  # if (add_class) for(col_name in names(df)) {
  #   attr(df[[col_name]], "class") <- "ufo"
  # }

  df
}
# todo row.names

ufo_vector <- function(mode = "logical", length = 0, populate_with_NAs = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  allowed_vector_types <- c("integer", "double", "logical", "complex", "raw", "character")
  if(!mode %in% allowed_vector_types) {
    stop("Vector mode ", mode, " is not supported by UFOs.")
  }

  constructor <- if (mode == "integer") UFO_C_intsxp_empty
    else if (mode == "double" || mode == "numeric") UFO_C_realsxp_empty
    else if (mode == "logical") UFO_C_lglsxp_empty
    else if (mode == "complex") UFO_C_cplxsxp_empty
    else if (mode == "raw") UFO_C_rawsxp_empty
    else if (mode == "character" || mode == "string") UFO_C_strsxp_empty
    else stop("Vector mode ", mode, " is not supported by UFOs.")

  .add_class(.Call(constructor, 
                   as.numeric(length),
                   as.logical(populate_with_NAs),
                   as.integer(.expect_exactly_one(min_load_count))), 
            "ufo", add_class)
}

ufo_integer <- function(size, populate_with_NAs = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_intsxp_empty,
                  as.numeric(size),
                  as.logical(populate_with_NAs),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_numeric <- function(size, populate_with_NAs = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_realsxp_empty,
                  as.numeric(size),
                  as.logical(populate_with_NAs),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_complex <- function(size, populate_with_NAs = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_cplxsxp_empty,
                  as.numeric(size),
                  as.logical(populate_with_NAs),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_logical <- function(size, populate_with_NAs = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_lglsxp_empty,
                  as.numeric(size),
                  as.logical(populate_with_NAs),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_raw <- function(size, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_rawsxp_empty,
                  as.numeric(size),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_character <- function(size, populate_with_NAs = FALSE, min_load_count = 0, add_class = .check_add_class()) {
  .add_class(.Call(UFO_C_strsxp_empty,
                  as.numeric(size),
                  as.logical(populate_with_NAs),
                  as.integer(.expect_exactly_one(min_load_count))),
             "ufo", add_class)
}

ufo_store_bin <- function(path, vector) {
   invisible(.Call(UFO_C_store_bin, .check_path(.expect_exactly_one(path)), vector))
}
