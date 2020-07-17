#-----------------------------------------------------------------------------
# Custom operators
#-----------------------------------------------------------------------------

.ufo_binary <- function(operation, result_inference, x, y, min_load_count=0, chunk_size=100000) {
  if (!is_ufo(x) && !is_ufo(y)) {
    return(operation(x, y))
  }

  result <- .Call(result_inference, x, y, as.integer(min_load_count))
  result_size <- length(result);    
  number_of_chunks <- ceiling(result_size / chunk_size)
            
  for (chunk in 0:(`.base.-`(number_of_chunks, 1))) {
    x_chunk <- .Call("ufo_get_chunk", x, chunk, chunk_size, result_size)
    y_chunk <- .Call("ufo_get_chunk", y, chunk, chunk_size, result_size)
    result[attr(x_chunk, 'start_index'):attr(x_chunk, 'end_index')] <- operation(x_chunk, y_chunk)
  }
    
  return(result)
}

.ufo_unary <- function(operation, inference, x, min_load_count=0, chunk_size=100000) {
  if (!is_ufo(x)) {
    return(operation(x))
  }
  
  result <- .Call(result_inference, x, as.integer(min_load_count))
  result_size <- length(result);    
  number_of_chunks <- ceiling(result_size / chunk_size)
  
  for (chunk in 0:(number_of_chunks - 1)) {
    x_chunk <- .Call("ufo_get_chunk", x, chunk, chunk_size, result_size)
    result[attr(x_chunk, 'start_index'):attr(x_chunk, 'end_index')] <- operation(x_chunk)
  }
  
  return(result)
}

`+.ufo`   <- function(x, y) .ufo_binary(`.base.+`,   "ufo_fit_result", x, y)
`-.ufo`   <- function(x, y) .ufo_binary(`.base.-`,   "ufo_fit_result", x, y)
`*.ufo`   <- function(x, y) .ufo_binary(`.base.*`,   "ufo_fit_result", x, y)
`/.ufo`   <- function(x, y) .ufo_binary(`.base./`,   "ufo_div_result", x, y)
`^.ufo`   <- function(x, y) .ufo_binary(`.base.^`,   "ufo_div_result", x, y)
`%%.ufo`  <- function(x, y) .ufo_binary(`.base.%%`,  "ufo_mod_result", x, y)
`%/%.ufo` <- function(x, y) .ufo_binary(`.base.%/%`, "ufo_mod_result", x, y)
`<.ufo`   <- function(x, y) .ufo_binary(`.base.<`,   "ufo_rel_result", x, y)
`<=.ufo`  <- function(x, y) .ufo_binary(`.base.<=`,  "ufo_rel_result", x, y)
`>.ufo`   <- function(x, y) .ufo_binary(`.base.>`,   "ufo_rel_result", x, y)
`>=.ufo`  <- function(x, y) .ufo_binary(`.base.>=`,  "ufo_rel_result", x, y)
`==.ufo`  <- function(x, y) .ufo_binary(`.base.==`,  "ufo_log_result", x, y)
`!=.ufo`  <- function(x, y) .ufo_binary(`.base.!=`,  "ufo_log_result", x, y)
`|.ufo`   <- function(x, y) .ufo_binary(`.base.|`,   "ufo_log_result", x, y)
`&.ufo`   <- function(x, y) .ufo_binary(`.base.&`,   "ufo_log_result", x, y)
`!.ufo`   <- function(x)    .ufo_unary (`.base.!`,   "ufo_neg_result", x, y)

#`[.ufo`   <- function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_subset",        x, y) else `.base.[`  (x,y)
#`[<-.ufo` <- function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_subset_assign", x, y) else `.base.[<-`(x,y)

#-----------------------------------------------------------------------------
# Save base operators for use later
#-----------------------------------------------------------------------------

`.base.+`   <- 	`+`
`.base.-`   <- 	`-`
`.base.*`   <- 	`*`
`.base./`   <- 	`/`
`.base.^`   <- 	`^`
`.base.%%`  <- 	`%%`
`.base.%/%` <- 	`%/%`

`.base.<`   <- 	`<`
`.base.<=`  <- 	`<=`
`.base.>`   <- 	`>`
`.base.>=`  <- 	`>=`
`.base.==`  <- 	`==`
`.base.!=`  <- 	`!=`
`.base.!`   <- 	`!`
`.base.|`   <- 	`|`
`.base.&`   <- 	`&`

`.base.[`   <- 	`[`
`.base.[<-` <- 	`[<-`

#-----------------------------------------------------------------------------
# Overload base operators
#-----------------------------------------------------------------------------

#`+`		<- `+.ufo`
#`-` 	<- `-.ufo`
#`*`		<- `*.ufo`
#`/`		<- `/.ufo`
#`^`		<- `^.ufo`
#`%%`	<- `%%.ufo`
#`%/%`	<- `%/%.ufo`
#`<`		<- `<.ufo`
#`<=`	<- `<=.ufo`
#`>`		<- `>.ufo`
#`>=`	<- `>=.ufo`
#`==`	<- `==.ufo`
#`!=`	<- `!=.ufo`
#`!`		<- `!.ufo`
#`|`		<- `|.ufo`
#`&`		<- `&.ufo`
	
#	`[`		<- `[.ufo`
#	`[<-`	<- `[<-.ufo`