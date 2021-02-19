context("UFO vector subscripting")

test_that("ufo null subscript", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- NULL

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo boolean subscript all true", { # segfaults
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- TRUE

  reference <- 1:100000
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo boolean subscript all false", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- FALSE

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo boolean subscript half and half", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(FALSE, TRUE)

  reference <- (1:100000)[subscript]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo boolean subscript true-true-false", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(TRUE, TRUE, FALSE)

  reference <- (1:100000)[subscript]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})
