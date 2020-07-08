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
`.base.||`  <- 	`|`
`.base.&&`	<-	`&&`

`.base.[`   <- 	`[`
`.base.[<-` <- 	`[<-`

`.ufo.+`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_add",  x, y) else `.base.+`  (x,y)
`.ufo.-`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_sub",  x, y) else `.base.-`  (x,y)
`.ufo.*`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_mul",  x, y) else `.base.*`  (x,y)
`.ufo./`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_div",  x, y) else `.base./`  (x,y)
`.ufo.^`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_pow",  x, y) else `.base.^`  (x,y)
`.ufo.%%`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_mod",  x, y) else `.base.%%` (x,y)
`.ufo.%/%`  <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_idiv", x, y) else `.base.%/%`(x,y)

`.ufo.<`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_lt",   x, y) else `.base.<`  (x,y)
`.ufo.<=`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_le",   x, y) else `.base.<=` (x,y)
`.ufo.>`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_gt",   x, y) else `.base.>`  (x,y)
`.ufo.>=`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_ge",   x, y) else `.base.>=` (x,y)
`.ufo.==`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_eq",   x, y) else `.base.==` (x,y)
`.ufo.!=`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_neq",  x, y) else `.base.!=` (x,y)
`.ufo.!`    <- 	function(x)	   if (is_ufo(x) || is_ufo(y)) .Call("ufo_neg",  x)    else `.base.!`  (x)
`.ufo.|`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_bor",  x, y) else `.base.|`  (x,y)
`.ufo.&`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_band", x, y) else `.base.&`  (x,y)
`.ufo.||`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_or",   x, y) else `.base.||` (x,y)
`.ufo.&&`   <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_and",  x, y) else `.base.&&` (x,y)

`.ufo.[`    <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_subset",        x, y) else `.base.[`  (x,y)
`.ufo.[<-`  <- 	function(x, y) if (is_ufo(x) || is_ufo(y)) .Call("ufo_subset_assign", x, y) else `.base.[<-`(x,y)

#   `+`		<- `.ufo.+`
#	`-` 	<- `.ufo.-`
#	`*`		<- `.ufo.*`
#	`/`		<- `.ufo./`
#	`^`		<- `.ufo.^`
#	`%%`	<- `.ufo.%%`
#	`%/%`	<- `.ufo.%/%`

#	`<`		<- `.ufo.<`
#	`<=`	<- `.ufo.<=`
#	`>`		<- `.ufo.>`
#	`>=`	<- `.ufo.>=`
#	`==`	<- `.ufo.==`
#	`!=`	<- `.ufo.!=`
#	`!`		<- `.ufo.!`
#	`|`		<- `.ufo.|`
#	`&`		<- `.ufo.&`
#	`||`	<- `.ufo.||`
#	`&&`	<- `.ufo.&&`
	
#	`[`		<- `.ufo.[`
#	`[<-`	<- `.ufo.[<-`