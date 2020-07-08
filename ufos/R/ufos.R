# Initializes the UFO framework
.initialize <- function(...) invisible(.Call("ufo_initialize"))

# Kills the UFO framework.
.jeff_goldbloom <- function(...) invisible(.Call("ufo_shutdown"))

# Finalizer for the entire UFO framework, called on session exit.
.onLoad <- function(libname, pkgname) {
  invisible(.initialize())
  invisible(reg.finalizer(.GlobalEnv, .jeff_goldbloom, onexit = TRUE))
}

# Finalizer for the entire UFO framework, called on package unload.
# AFAIK this cannot be relied on to be called on shutdown.
.onUnload <- function(libname, pkgname) {
  .jeff_goldbloom()
}

# Checks whether a vector is a UFO.
is_ufo <- function(x) {
	.Call("is_ufo", x)
}