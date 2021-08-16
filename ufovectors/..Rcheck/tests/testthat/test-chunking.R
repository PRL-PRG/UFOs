context("Vector ufo apply")

test_apply <- function (f, ..., chunk_size) {
  ufo_result <- ufo_apply(FUN=f, ..., chunk_size=chunk_size)
  reference_result <- mapply(FUN=f, ..., SIMPLIFY=TRUE)
  expect_equal(ufo_result, reference_result)
  expect_equal(is_ufo(ufo_result), TRUE)
}

test_that("ufo_apply 1 input  tiny chunks",        test_apply(function(x)       x + 1,         1:10000,                chunk_size=1))
test_that("ufo_apply 2 inputs tiny chunks",        test_apply(function(x, y)    x + y + 1,     1:10000, 1:1000,        chunk_size=1))
test_that("ufo_apply 3 inputs tiny chunks",        test_apply(function(x, y, z) x + y + z + 1, 1:10000, 1:1000, 1:100, chunk_size=1))
test_that("ufo_apply 1 input  small chunks",       test_apply(function(x)       x + 1,         1:10000,                chunk_size=100))
test_that("ufo_apply 2 inputs small chunks",       test_apply(function(x, y)    x + y + 1,     1:10000, 1:1000,        chunk_size=100))
test_that("ufo_apply 3 inputs small chunks",       test_apply(function(x, y, z) x + y + z + 1, 1:10000, 1:1000, 1:100, chunk_size=100))
test_that("ufo_apply 1 input  medium chunks",      test_apply(function(x)       x + 1,         1:10000,                chunk_size=1000))
test_that("ufo_apply 2 inputs medium chunks",      test_apply(function(x, y)    x + y + 1,     1:10000, 1:1000,        chunk_size=1000))
test_that("ufo_apply 3 inputs medium chunks",      test_apply(function(x, y, z) x + y + z + 1, 1:10000, 1:1000, 1:100, chunk_size=1000))
test_that("ufo_apply 1 input  big chunks",         test_apply(function(x)       x + 1,         1:10000,                chunk_size=10000))
test_that("ufo_apply 2 inputs big chunks",         test_apply(function(x, y)    x + y + 1,     1:10000, 1:1000,        chunk_size=10000))
test_that("ufo_apply 3 inputs big chunks",         test_apply(function(x, y, z) x + y + z + 1, 1:10000, 1:1000, 1:100, chunk_size=10000))
test_that("ufo_apply 1 input  hugenormous chunks", test_apply(function(x)       x + 1,         1:10000,                chunk_size=100000))
test_that("ufo_apply 2 inputs hugenormous chunks", test_apply(function(x, y)    x + y + 1,     1:10000, 1:1000,        chunk_size=100000))
test_that("ufo_apply 3 inputs hugenormous chunks", test_apply(function(x, y, z) x + y + z + 1, 1:10000, 1:1000, 1:100, chunk_size=100000))