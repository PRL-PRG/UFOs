`.base.+`   <- 	`+`
`.base.-`   <- 	`-`
`.base.*`   <- 	`*`
`.base./`   <- 	`/`
`.base.^`   <- 	`^`
`.base.**`  <- 	`**`
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
`.base.||`  <- 	`|`
`.base.&&`	<-	`&&`

`.base.[`   <- 	`[`
`.base.[<-` <- 	`[<-`

`.ufo.+`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_add",  x, y) else `.base.+`  (x,y)
`.ufo.-`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_sub",  x, y) else `.base.-`  (x,y)
`.ufo.*`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_mul",  x, y) else `.base.*`  (x,y)
`.ufo./`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_div",  x, y) else `.base./`  (x,y)
`.ufo.^`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_pow",  x, y) else `.base.^`  (x,y)
`.ufo.**`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_pow",  x, y) else `.base.**` (x,y)
`.ufo.%%`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_mod",  x, y) else `.base.%%` (x,y)
`.ufo.%/%`  <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_idiv", x, y) else `.base.%/%`(x,y)

`.ufo.<`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_lt",   x, y) else `.base.<`  (x,y)
`.ufo.<=`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_le",   x, y) else `.base.<=` (x,y)
`.ufo.>`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_gt",   x, y) else `.base.>`  (x,y)
`.ufo.>=`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_ge",   x, y) else `.base.>=` (x,y)
`.ufo.==`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_eq",   x, y) else `.base.==` (x,y)
`.ufo.!=`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_neq",  x, y) else `.base.!=` (x,y)
`.ufo.!`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_neg",  x)    else `.base.!`  (x)
`.ufo.|`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_bor",  x, y) else `.base.|`  (x,y)
`.ufo.&`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_band", x, y) else `.base.&`  (x,y)
`.ufo.||`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_or",   x, y) else `.base.||` (x,y)
`.ufo.&&`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_and",  x, y) else `.base.&&` (x,y)

`.ufo.[`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_subset",        x, y) else `.base.[`  (x,y)
`.ufo.[<-`  <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Internal("ufo_subset_assign", x, y) else `.base.[<-`(x,y)

#.onLoad <- function(libname, pkgname) {
    `+`		<- `.ufo.+`
	`-` 	<- `.ufo.-`
	`*`		<- `.ufo.*`
	`/`		<- `.ufo./`
	`^`		<- `.ufo.^`
	`**`	<- `.ufo.**`
	`%%`	<- `.ufo.%%`
	`%/%`	<- `.ufo.%/%`
	
	`<`		<- `.ufo.<`
	`<=`	<- `.ufo.<=`
	`>`		<- `.ufo.>`
	`>=`	<- `.ufo.>=`
	`==`	<- `.ufo.==`
	`!=`	<- `.ufo.!=`
	`!`		<- `.ufo.!`
	`|`		<- `.ufo.|`
	`&`		<- `.ufo.&`
	`||`	<- `.ufo.||`
	`&&`	<- `.ufo.&&`
	
	`[`		<- `.ufo.[`
	`[<-`	<- `.ufo.[<-`
#}