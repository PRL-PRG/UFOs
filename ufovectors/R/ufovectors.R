ufo_init <- function() {
  invisible(.Call("ufo_vectors_initialize"))
}

ufo_integer_bin <- function(path) {
  .check_path(path)
  .Call("ufo_vectors_intsxp_bin", path)
}

# ufo_numeric_bin <- function(path) {
#   .check_path(path)
#   .Call("ufo_vectors_realsxp_bin", path)
# }
#
# ufo_character_bin <- function(path) {
#   .check_path(path)
#   .Call("ufo_vectors_strsxp_bin", path)
# }
#
# ufo_complex_bin <- function(path) {
#   .check_path(path)
#   .Call("ufo_vectors_cplxsxp_bin", path)
# }
#
# ufo_logical_bin <- function(path) {
#   .check_path(path)
#   .Call("ufo_vectors_lglsxp_bin", path)
# }

ufo_shutdown <- function() {
  invisible(.Call("ufo_vectors_shutdown"))
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
    "first one, ignoring the rest"));
  }
}