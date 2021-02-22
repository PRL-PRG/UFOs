context("UFO vector subscripting")

test_that("ufo null subscript", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- NULL

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo boolean subscript all true", {
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
                                  #!!!
  expect_equal(result, reference) #!!!
})
  # ── Warning (test-operators.R:35:3): ufo binary * ───────────────────────────────
  # NAs produced by integer overflow
  # Backtrace:
  #  1. ufovectors::ufo_multiply(ufo, argument) test-operators.R:35:2
  #  2. ufovectors:::.ufo_binary(...)
  # ── Warning (test-operators.R:36:3): ufo binary * ───────────────────────────────
  # NAs produced by integer overflow

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


test_that("ufo boolean subscript true-false-NA", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(TRUE, FALSE, NA)

  reference <- (1:100000)[subscript]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript a zero", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- 0

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript a couple of zeros", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(0, 0)

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript a couple of zeros mixed in with non-zeros", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(0, 1, 0, 100000)

  reference <- as.integer(c(1, 100000))
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript length=0", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(0)

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript length=1", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(42)

  reference <- as.integer(42)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript length=small subset", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(c(4, 10, 7, 100))

  reference <- as.integer(c(4, 10, 7 , 100))
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript length=small subset with NAs", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(c(4, 10, NA, 7, NA, 100, NA))

  reference <- as.integer(c(4, 10, NA, 7, NA, 100, NA))
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript length=large subset", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(c(1:1000, 2000:5000, 10:1000, 6000:10000))

  reference <- as.integer(c(1:1000, 2000:5000, 10:1000, 6000:10000))
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo integer subscript length=one negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(-10)

  reference <- (1:100000)[-10]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})


test_that("ufo integer subscript length=a few negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(c(-10, -100, -100, -1000))

  reference <- (1:100000)[c(-10, -100, -100, -1000)]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})


test_that("ufo integer subscript length=many negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(-c(1:1000, 2000:5000, 10:1000, 6000:10000))

  reference <-
    (1:100000)[as.integer(-c(1:1000, 2000:5000, 10:1000, 6000:10000))]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})


test_that("ufo integer subscript length=all negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.integer(-as.integer(1:100000))

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript a zero", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- 0

  reference <- numeric(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript a couple of zeros", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(0, 0)

  reference <- numeric(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript a couple of zeros mixed in with non-zeros", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(0, 1, 0, 100000)

  reference <- c(1, 100000)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript length=0", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.numeric(0)

  reference <- numeric(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript length=1", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.numeric(42)

  reference <- as.numeric(42)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript length=small subset", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(4, 10, 7, 100)

  reference <- c(4, 10, 7, 100)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript length=small subset and NAs", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(4, 10, NA, 7, NA, 100, 100, NA)

  reference <- c(4, 10, NA, 7, NA, 100, 100, NA)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript length=large subset", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(1:1000, 2000:5000, 10:1000, 6000:10000)

  reference <- c(1:1000, 2000:5000, 10:1000, 6000:10000)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript length=one negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- as.numeric(-10)

  reference <- (1:100000)[-10]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})


test_that("ufo numeric subscript length=a few negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c(-10, -100, -100, -1000)

  reference <- (1:100000)[c(-10, -100, -100, -1000)]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo numeric subscript length=many negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- -c(1:1000, 2000:5000, 10:1000, 6000:10000)

  reference <- (1:100000)[-c(1:1000, 2000:5000, 10:1000, 6000:10000)]
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})


test_that("ufo numeric subscript length=all negative", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- -as.numeric(1:100000)

  reference <- numeric(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo string subscript no names one element", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- "butts"

  reference <- as.integer(NA)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo string subscript no names many elements", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000

  subscript <- c("a", "b", "c")

  reference <- as.integer(c(NA, NA, NA))
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo string hash subscript one element", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000
  ufo_names <- paste0("N", 1:100000)
  ufo <- setNames(ufo, ufo_names)

  subscript <- "N42"

  reference <- as.integer(42)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo string hash subscript lentgth=zero", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000
  ufo_names <- paste0("N", 1:100000)
  ufo <- setNames(ufo, ufo_names)

  subscript <- character(0)

  reference <- integer(0)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference)
})

test_that("ufo string hash subscript length=small subset", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000
  ufo_names <- paste0("N", 1:100000)
  ufo <- setNames(ufo, ufo_names)

  subscript <- c("N4", "N10", "N7", "N100", "N100")

  reference <- c(4, 10, 7, 100, 100)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference) # !!!
})

test_that("ufo string hash subscript length=small subset with NAs", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000
  ufo_names <- paste0("N", 1:100000)
  ufo <- setNames(ufo, ufo_names)

  subscript <- c("N4", NA, "N10", NA, "N7", "N100", "N100", NA)

  reference <- c(4, NA, 10, NA, 7, 100, 100, NA)
  result <- ufovectors::subscript(ufo, subscript)

  print(result)

  expect_equal(result, reference) # !!!
})
# ── Failure (test-subscripting.R:443:3): ufo numeric subscript length=small subset with NAs ──
# `result` not equal to `reference`.
# 4/7 mismatches (average diff: NaN)
# [1] NA -   4 == NA
# [3] NA -  10 == NA
# [5] NA -   7 == NA
# [6] NA - 100 == NA

test_that("ufo string hash subscript length=large subset", {
  ufo <- ufo_integer(100000)
  ufo[1:100000] <- 1:100000
  ufo_names <- paste0("N", 1:100000)
  ufo <- setNames(ufo, ufo_names)

  subscript <- paste0("N", c(1:1000, 2000:5000, 10:1000, 6000:10000))

  reference <- c(1:1000, 2000:5000, 10:1000, 6000:10000)
  result <- ufovectors::subscript(ufo, subscript)

  expect_equal(result, reference) # !!!
})

# TODO text NAs in integers and numerics

# Warning: stack imbalance in '<-', 77 then 78
# Warning: stack imbalance in '{', 73 then 74
# Warning: stack imbalance in '<-', 77 then 78
# Warning: stack imbalance in '{', 73 then 74
# Warning: stack imbalance in '<-', 77 then 78
# Warning: stack imbalance in '{', 73 then 74
# Warning: stack imbalance in '<-', 77 then 78
# Warning: stack imbalance in '{', 73 then 74