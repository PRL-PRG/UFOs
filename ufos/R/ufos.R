# Finalizer for the entire UFO framework, called on session exit.
.onLoad <- function(libname, pkgname) {
  jeff_goldbloom <- function(...) invisible(.Call("ufo_shutdown"))
  invisible(reg.finalizer(.GlobalEnv, jeff_goldbloom, onexit = TRUE))
}

# Finalizer for the entire UFO framework, called on package unload.
# AFAIK this cannot be relied on to be called on shutdown.
.onUnload <- function(libname, pkgname) {
  .Call("ufo_shutdown")
}