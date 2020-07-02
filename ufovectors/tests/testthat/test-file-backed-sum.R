context("Sum on file-backed integer vectors")

create_bin_file <- function(name, template, repeats = 1) {
    path <- tempfile(name)
    handle <- file(path, "wb")
    for (i in 1:repeats) { writeBin(template, handle) }
    close(handle)
    path
} 

destroy_bin_file <- function(path) {
	unlink(path)
}

test_that("sum 1K ones", {
    path <- create_bin_file(name="ufo1K", template=as.integer(rep(1,1000)), repeats=1)
    expect_equal(sum(ufo_integer_bin(path)), 1000)
    destroy_bin_file(path)
})

test_that("sum 1M ones", {
	path <- create_bin_file(name="ufo1M", template=as.integer(rep(1,1000000)), repeats=1)
    expect_equal(sum(ufo_integer_bin(path)), 1000000)
    destroy_bin_file(path)
})

test_that("sum 1G ones", {
    path <- create_bin_file(name="ufo1G", template=as.integer(rep(1,1000000)), repeats=1000)
    expect_equal(sum(ufo_integer_bin(path)), 1000000000)
    destroy_bin_file(path)
})