#-----------------------------------------------------------------------------
# Custom operators implementation: perform operations by chunks
#-----------------------------------------------------------------------------

ufo_add           <- function(x, y) 
                       if(missing(y)) .ufo_unary (.base_add,           UFO_C_neg_result, x) else
                                      .ufo_binary(.base_add,           UFO_C_fit_result, x, y)
ufo_subtract      <- function(x, y)   
                       if(missing(y)) .ufo_unary (.base_subtract,      UFO_C_neg_result, x) else
                                      .ufo_binary(.base_subtract,      UFO_C_fit_result, x, y) 
ufo_multiply      <- function(x, y)   .ufo_binary(.base_multiply,      UFO_C_fit_result, x, y)
ufo_divide        <- function(x, y)   .ufo_binary(.base_divide,        UFO_C_div_result, x, y)
ufo_int_divide    <- function(x, y)   .ufo_binary(.base_int_divide,    UFO_C_mod_result, x, y)
ufo_power         <- function(x, y)   .ufo_binary(.base_power,         UFO_C_div_result, x, y)
ufo_modulo        <- function(x, y)   .ufo_binary(.base_modulo,        UFO_C_mod_result, x, y)
ufo_less          <- function(x, y)   .ufo_binary(.base_less,          UFO_C_rel_result, x, y)
ufo_less_equal    <- function(x, y)   .ufo_binary(.base_less_equal,    UFO_C_rel_result, x, y)
ufo_greater       <- function(x, y)   .ufo_binary(.base_greater,       UFO_C_rel_result, x, y)
ufo_greater_equal <- function(x, y)   .ufo_binary(.base_greater_equal, UFO_C_rel_result, x, y)
ufo_equal         <- function(x, y)   .ufo_binary(.base_equal,         UFO_C_log_result, x, y)
ufo_unequal       <- function(x, y)   .ufo_binary(.base_unequal,       UFO_C_log_result, x, y)
ufo_or            <- function(x, y)   .ufo_binary(.base_or,            UFO_C_log_result, x, y)
ufo_and           <- function(x, y)   .ufo_binary(.base_and,           UFO_C_log_result, x, y)
ufo_not           <- function(x)      .ufo_unary (.base_not,           UFO_C_neg_result, x)


#-----------------------------------------------------------------------------
# Subsetting
#-----------------------------------------------------------------------------

ufo_subset <- function(x, subscript, ..., drop=FALSE, min_load_count=0) { # drop ignored for ordinary vectors, it seems?
  # choice of output type goes here? or inside
  .Call(UFO_C_subset, x, subscript, as.integer(min_load_count))
}

# ufo_subset_assign <- function(x, subscript, values, ..., drop=FALSE, min_load_count=0) { # drop ignored for ordinary vectors, it seems?
#   # choice of output type goes here? or inside
#   .Call(UFO_C_subset_assign, x, subscript, values, as.integer(min_load_count))
# }

ufo_subscript <- function(x, subscript, min_load_count=0) {
  .Call(UFO_C_subscript, x, subscript, as.integer(min_load_count))
}

# We can't really do subset_assign equivalent, without triggering copy-on-write
# when we do, I think.# Unless we really dig into it and re-create it from 
# scratch. This would perhaps be nicer, but subset assign works out of the box,
# provided the subscript is small enough. If the subscript is big, there is 
# v[ufo_subscript(v, i)] <- v as a potential workaround, but it will cause the
# subscript to be re-created anyway, to change to 0-indexing, so... Another 
# alternative is to explicitly mutate in-place with ufo_mutate.

# Warnign: buggy.
ufo_mutate <- function(x, subscript, values, ..., drop=FALSE, min_load_count=0) { # drop ignored for ordinary vectors, it seems?
  .Call(UFO_C_subset_assign, x, subscript, values, as.integer(min_load_count))
}

#-----------------------------------------------------------------------------
# Applying functions by chunks
#-----------------------------------------------------------------------------

# Example:
# arguments:
# - [p, q, r]
# - [a, b, c, d]
# - [x, y]
# Expected output:
# - [f(p, a, x), f(q, b, y), f(r, c, x), f(p, d, y)]
ufo_apply <- function(FUN, ..., MoreArgs = NULL, USE.NAMES = TRUE, chunk_size=100000) {

  # List of vectors that we can create UFOs around.
  allowed_vector_types <- 
    c("integer", "double", "logical", "complex", "raw", "character")

  # Convert input arguments to a single list.
  # - [[1]] [p, q, r]
  # - [[2]] [a, b, c, d]
  # - [[3]] [x, y]
  input_vectors <- list(...)

  # Nothing to process.
  if (length(input_vectors) == 0) {
    return(list())
  }

  # The vectors we return all need to be as large as the largest input vector.
  return_vector_length <- max(mapply(length, input_vectors));
  number_of_chunks <- ceiling(return_vector_length/ chunk_size);

  # Initially the type of the result is not known, so NULL.
  # There is one NULL for each input vector.
  # results <- Map(function(input_vector) NULL, input_vectors)
  # Actually there's only one result vector.
  result <- NULL

  # Create a UFO for each input vector (of a supported type).
  for (chunk in 0:(.base_subtract(number_of_chunks, 1))) {

    # For each input vector, retrieve a chunk and store it in the list.
    # Example for chunk 1 (0):
    # list:
    # - [[1]] p, q
    # - [[2]] a, b
    # - [[3]] x, y
    # Example for chunk 2 (1):
    # list:
    # - [[1]] r, p
    # - [[2]] c, d
    # - [[3]] x, y
    input_chunks <- Map(function(input_vector) {
      .Call(UFO_C_get_chunk, input_vector, 
                             chunk, chunk_size, 
                             return_vector_length)
    }, input_vectors)

    # Execute function f for the chunk and store the result in 
    # reasonably-sized vectors first.
    arguments <- append(list(FUN), input_chunks)
    result_chunk = do.call(mapply, arguments)

    # If this is the first chunk, for each input vector, create a vector to 
    # write all results into. At this point we can determine the return type of
    # the result by inferring it from the type of the first result chunk. We 
    # ASSUME that all returned chunks will have the same type.
    if (chunk == 0) {
      result_type <- typeof(result_chunk)
      # If the data cannot be constructed as a UFO, warn the user, but
      # proceed nevertheless, using an ordinary vector (which may explode
      # the memory).
      result <- if(!result_type %in% allowed_vector_types) {
        warning("Vector type ", result_type, " is not supported by ",
                "UFOs. Returning an ordinary R object instead.")
        vector(result_type, return_vector_length)
      # Otherwise create a UFO to store the result.
      } else {
        ufo_vector(result_type, return_vector_length)
      }
    }

    # When the loop is finished, the function f should have been applied to all
    # chunks of all input vectors. The results should be inside `results`.

    # Write values of result chunks into the result vector.
    # The range to write the changes into is calculated by `ufo_get_chunk`
    # and attached to each input_chunk. We retrieve it here.
    input_chunk <- input_chunks[[1]]
    index_start <- attr(input_chunk, 'start_index')
    index_end <- attr(input_chunk, 'end_index')
    index_range <- index_start:index_end
    # Write the results of f for the chunk into the 
    # approporiate space in one of the result vector.
    result[index_range] <- result_chunk
  }  

  # Do Map naming exit stuff.
  if (USE.NAMES && length(input_vectors)) {
      if (is.null(names1 <- names(input_vectors[[1L]])) 
          && is.character(input_vectors[[1L]])) {
          names(result) <- input_vectors[[1L]]
      } else if (!is.null(names1)) {
          names(result) <- names1
      }
  }

  # We do not support unsimplified results, because the type would not work 
  # well with UFOs (list of vectors).
  # if (!isFALSE(SIMPLIFY) && length(results)) 
  #     simplify2array(results, higher = (SIMPLIFY == "array"))
  # else results

  result
}

#-----------------------------------------------------------------------------
# Helper functions that do the actual chunking
#-----------------------------------------------------------------------------

.ufo_binary <- function(operation, result_inference, x, y, min_load_count=0, chunk_size=100000) {
  #cat("...\n")
  if (!is_ufo(x) && !is_ufo(y)) return(operation(x, y))

  result <- .Call(result_inference, x, y, as.integer(min_load_count))
  result_size <- length(result);
  number_of_chunks <- ceiling(result_size / chunk_size)

  for (chunk in 0:(.base_subtract(number_of_chunks, 1))) {
    x_chunk <- .Call(UFO_C_get_chunk, x, chunk, chunk_size, result_size)
    y_chunk <- .Call(UFO_C_get_chunk, y, chunk, chunk_size, result_size)
    result[attr(x_chunk, 'start_index'):attr(x_chunk, 'end_index')] <- operation(x_chunk, y_chunk)
  }

  # TODO copy attributes
  return(.add_class(result, "ufo", .check_add_class()))
}

.ufo_unary <- function(operation, result_inference, x, min_load_count=0, chunk_size=100000) {
  #cat("...\n")
  if (!is_ufo(x)) return(operation(x))

  result <- .Call(result_inference, x, as.integer(min_load_count))
  result_size <- length(result);
  number_of_chunks <- ceiling(result_size / chunk_size)

  for (chunk in 0:(.base_subtract(number_of_chunks, 1))) {
    x_chunk <- .Call(UFO_C_get_chunk, x, chunk, chunk_size, result_size)
    result[attr(x_chunk, 'start_index'):attr(x_chunk, 'end_index')] <- operation(x_chunk)
  }

  # TODO copy attributes
  return(.add_class(result, "ufo", .check_add_class()))
}

#-----------------------------------------------------------------------------
# Save base operators to disambiguate in case of overloading
#-----------------------------------------------------------------------------

.base_add           <- `+`
.base_subtract      <- `-`
.base_multiply      <- `*`
.base_divide        <- `/`
.base_power         <- `^`
.base_modulo        <- `%%`
.base_int_divide    <- `%/%`
.base_less          <- `<`
.base_less_equal    <- `<=`
.base_greater       <- `>`
.base_greater_equal <- `>=`
.base_equal         <- `==`
.base_unequal       <- `!=`
.base_not           <- `!`
.base_or            <- `|`
.base_and           <- `&`
.base_subset        <- `[`
.base_subset_assign <- `[<-`

#-----------------------------------------------------------------------------
# Set up S3 opertors or overload operators on load
#-----------------------------------------------------------------------------

# Poor workaround of the commented out code below
overload_operators <- function() {
  operator_overload_statements <- c(
    "`+`  <- ufovectors:::ufo_add",
    "`-`  <- ufovectors:::ufo_subtract",
    "`*`  <- ufovectors:::ufo_multiply",
    "`/`  <- ufovectors:::ufo_divide",
    "`^`  <- ufovectors:::ufo_power",
    "`%%` <- ufovectors:::ufo_modulo",
    "`%/%`<- ufovectors:::ufo_int_divide",
    "`<`  <- ufovectors:::ufo_less",
    "`<=` <- ufovectors:::ufo_less_equal",
    "`>`  <- ufovectors:::ufo_greater",
    "`>=` <- ufovectors:::ufo_greater_equal",
    "`==` <- ufovectors:::ufo_equal",
    "`!=` <- ufovectors:::ufo_unequal",
    "`!`  <- ufovectors:::ufo_not",
    "`|`  <- ufovectors:::ufo_or",
    "`&`  <- ufovectors:::ufo_and"
    #"`[` <- ufovectors:::ufo_subset",
    #"`[<-` <- ufovectors:::ufo_subset_assign"
  )

  eval(parse(text = operator_overload_statements), envir = globalenv())
}

unload_operators <- function() {
  operator_overload_statements <- c(
    "`+`   <- ufovectors:::.base_add",
    "`-`   <- ufovectors:::.base_subtract",
    "`*`   <- ufovectors:::.base_multiply",
    "`/`   <- ufovectors:::.base_divide",
    "`^`   <- ufovectors:::.base_power",
    "`%%`  <- ufovectors:::.base_modulo",
    "`%/%` <- ufovectors:::.base_int_divide",
    "`<`   <- ufovectors:::.base_less",
    "`<=`  <- ufovectors:::.base_less_equal",
    "`>`   <- ufovectors:::.base_greater",
    "`>=`  <- ufovectors:::.base_greater_equal",
    "`==`  <- ufovectors:::.base_equal",
    "`!=`  <- ufovectors:::.base_unequal",
    "`!`   <- ufovectors:::.base_not",
    "`|`   <- ufovectors:::.base_or",
    "`&`   <- ufovectors:::.base_and"
    #"`[` <- ufovectors:::.base_subset",
    #"`[<-`	<- ufovectors:::.base_subset_assign"
  )

  eval(parse(text=operator_overload_statements), envir=globalenv())
}

# options(ufovectors.add_class = TRUE)
# options(ufovectors.overload_operators = TRUE)
.onLoad <- function(...) {
  if (isTRUE(getOption("ufovectors.add_class"))) {
    write("Creating S3 methods for UFO vectors\n", stderr())
    registerS3method("+",   "ufo", ufovectors:::ufo_add)
    registerS3method("-",   "ufo", ufovectors:::ufo_subtract)
    registerS3method("*",   "ufo", ufovectors:::ufo_multiply)
    registerS3method("/",   "ufo", ufovectors:::ufo_divide)
    registerS3method("^",   "ufo", ufovectors:::ufo_power)
    registerS3method("%%",  "ufo", ufovectors:::ufo_modulo)
    registerS3method("%/%", "ufo", ufovectors:::ufo_int_divide)
    registerS3method("<",   "ufo", ufovectors:::ufo_less)
    registerS3method("<=",  "ufo", ufovectors:::ufo_less_equal)
    registerS3method(">",   "ufo", ufovectors:::ufo_greater)
    registerS3method(">=",  "ufo", ufovectors:::ufo_greater_equal)
    registerS3method("==",  "ufo", ufovectors:::ufo_equal)
    registerS3method("!=",  "ufo", ufovectors:::ufo_unequal)
    registerS3method("!",   "ufo", ufovectors:::ufo_not)
    registerS3method("|",   "ufo", ufovectors:::ufo_or)
    registerS3method("&",   "ufo", ufovectors:::ufo_and)
    registerS3method("[",   "ufo", ufovectors:::ufo_subset)
    #registerS3method("[<-", "ufo", ufovectors:::ufo_subset_assign)	
  }

  if (isTRUE(getOption("ufovectors.overload_operators"))) {
    write("Overloading operators to return UFO vectors\n", stderr())
    ufovectors:::overload_operators()
  }
}

# Notes on subsetting:
#
# +-------------------+-------------------+----------+------+
# | capture method    | works from C code | hygene   | done |
# +-------------------+-------------------+----------+------+
# | ALTREP            | Y                 | good     |      |
# | S3                | N                 | good     |      |
# | redefine operator | N                 | criminal |      |
# +-------------------+-------------------+----------+------+
#
# +-------------------+-------------+-------------+--------------------+------+
# | store result in   | disk use    | memory use  | overhead on access |      |
# +-------------------+-------------+-------------+--------------------+------+
# | ALTREP / viewport | none        | index size  | some               |      |
# | UFO copy          | result size | negligible  | negligible         |      |
# | R copy            | none        | result size | none               |      |
# +-------------------+-------------+-------------+--------------------+------+
# | UFO viewport      |             currently not possible             |      |
# +-------------------+------------------------------------------------+------+
#

# TODO parameterize minloadcount globally