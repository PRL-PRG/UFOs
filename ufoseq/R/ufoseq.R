ufo_seq <- function(from, to, by = 1) {
  # check if any of the arguments were missing
  if (missing(from)) stop ("'from' is a required argument")
  if (missing(to)) stop ("'to' is a required argument")

  # check whether the arguments are non-zero length
  if (length(from) == 0) stop("'from' cannot be zero length")
  if (length(to) == 0) stop("'to' cannot be zero length")
  if (length(by) == 0) stop("'by' cannot be zero length")

  # check whether this sequence makes sense.
  if (from >= to) stop("'from' must not be less than 'to'")
  if (by <= 0) stop("'by' must be larger than zero")

  # check whether the arguments are of scalars
  if (length(from) > 1)
    warn("'from' has multiple values, only the first value will be used")
  if (length(to) > 1)
    warn("'to' has multiple values, only the first value will be used")
  if (length(by) > 1)
    warn("'by' has multiple values, only the first value will be used")

  # Convert inputs to integers and call the C function that actually creates
  # the vector
  .Call("ufo_seq", as.integer(from), as.integer(to), as.integer(by))
}
