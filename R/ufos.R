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
    warning(paste0("Path is a vector containing multiple values, picking the",
                   "first one, ignoring the rest"));
  }
  .Call("ufo_make_bin_file_source", path)
}

# Create new masked integer vector using a specified source. If multiple lengths
# are given, it creates a list of integer vectors that matches the number of
# provided lengths.
ufo_integer <- function(lengths, source) {
  if (typeof(lengths) != "double" && typeof(lengths) != "integer")
    stop(paste0("Vector is of type", typeof(lengths),
                ", must be either integer or double."))

  .Call("ufo_new_intsxp", as.integer(lengths), source)
}

# Create new masked logical vector using a specified source. If multiple lengths
# are given, it creates a list of integer vectors that matches the number of
# provided lengths.
ufo_logical <- function(lengths, source) {
  if (typeof(lengths) != "double" && typeof(lengths) != "integer")
    stop(paste0("Vector is of type", typeof(lengths),
                ", must be either integer or double."))


  .Call("ufo_new_lglsxp", as.integer(lengths), source)
}