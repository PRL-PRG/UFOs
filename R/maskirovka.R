# This is the R code for the hamstr package.
#
# Some useful keyboard shortcuts for package authoring in Rstudio:
#
#   Build and Reload Package:  'Ctrl + Shift + B'
#   Check Package:             'Ctrl + Shift + E'
#   Test Package:              'Ctrl + Shift + T'

# Create new masked integer vector. If multiple lengths are given, it creates
# a list of integer vectors that matches the number of provided lengths.
mask_new <- function(lengths) {
  # Check if argument is a real or integer vector and throw exception if that
  # is not the case.
  if (typeof(lengths) != "double" && typeof(lengths) != "integer")
    stop(paste0("Vector is of type", typeof(lengths),
                ", must be either integer or double."))

  # Call C function registered as "mask_new" and pass vector as 1st
  # argument.
  .Call("mask_new", as.integer(lengths))
}

mask_new_altrep <- function(lengths) {
    # Check if argument is a real or integer vector and throw exception if that
    # is not the case.
    if (typeof(lengths) != "double" && typeof(lengths) != "integer")
    stop(paste0("Vector is of type", typeof(lengths),
    ", must be either integer or double."))

    # Call C function registered as "mask_new" and pass vector as 1st
    # argument.
    .Call("mask_new_altrep", as.integer(lengths))
}
