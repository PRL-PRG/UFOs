context("UFO vector operators")

test_that("ufo binary +", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_add(ufo, argument)
  result_reference <- reference + argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary -", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_subtract(ufo, argument)
  result_reference <- reference - argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary *", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_multiply(ufo, argument)
  result_reference <- reference * argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary /", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_divide(ufo, argument)
  result_reference <- reference / argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary ^", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_power(ufo, argument)
  result_reference <- reference ^ argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary %%", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_modulo(ufo, argument)
  result_reference <- reference %% argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary %/%", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_int_divide(ufo, argument)
  result_reference <- reference %/% argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary <", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_less(ufo, argument)
  result_reference <- reference < argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary >", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_greater(ufo, argument)
  result_reference <- reference > argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary <=", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_less_equal(ufo, argument)
  result_reference <- reference <= argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary >=", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_greater_equal(ufo, argument)
  result_reference <- reference >= argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary ==", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_equal(ufo, argument)
  result_reference <- reference == argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary !=", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_unequal(ufo, argument)
  result_reference <- reference != argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary |", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_or(ufo, argument)
  result_reference <- reference | argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary &", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_and(ufo, argument)
  result_reference <- reference & argument

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo unary !", {
  ufo <- ufo_logical(100000);
  reference <- logical(100000);

  ufo[1:100000] <- c(TRUE, FALSE, NA, FALSE)
  reference[1:100000]<-c(TRUE, FALSE, NA, FALSE)

  result_ufo <- ufo_not(ufo)
  result_reference <- !reference

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo unary +", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000

  result_ufo <- ufo_add(ufo)
  result_reference <- +reference

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo unary -", {
  ufo <- ufo_integer(100000);
  ufo[1:100000] <- 1:100000
  reference <- 1:100000
  argument <- 1:100000

  result_ufo <- ufo_subtract(ufo)
  result_reference <- -reference

  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})