ufo_bin_file_source <-function(path) {
  if (typeof(path) != "character") {
    stop(paste0("Path is of type", typeof(path),
                ", must be a character vector."))
  }
  if (length(path) == 0) {
    stop("Path is a 0-length vector")
  }
  if (length(path) > 1) {
    # TODO warning
  }
  .Call("ufo_make_bin_file_source", path)
}

# Create new masked integer vector. If multiple lengths are given, it creates
# a list of integer vectors that matches the number of provided lengths.
ufo_integer <- function(lengths, source) {
  # Check if argument is a real or integer vector and throw exception if that
  # is not the case.
  if (typeof(lengths) != "double" && typeof(lengths) != "integer") {
    stop(paste0("Vector is of type", typeof(lengths),
                ", must be either integer or double."))
  }

  # Call C function registered as "ufo_new" and pass vector as 1st
  # argument.
  .Call("ufo_new_intsxp", as.integer(lengths), source)
}

# Create new masked logical vector. If multiple lengths are given, it creates
# a list of integer vectors that matches the number of provided lengths.
ufo_logical <- function(lengths, source) {
  # Check if argument is a real or integer vector and throw exception if that
  # is not the case.
  if (typeof(lengths) != "double" && typeof(lengths) != "integer") {
    stop(paste0("Vector is of type", typeof(lengths),
                ", must be either integer or double."))
  }

  # Call C function registered as "ufo_new" and pass vector as 1st
  # argument.
  .Call("ufo_new_lglsxp", as.integer(lengths), source)
}