context("Empty UFO vectors")

test_that("empty integer vector", {
  ufo <- ufo_integer(1000000);
  ordinary <- integer(1000000);
  expect_equal(ufo, ordinary);
})

test_that("empty numeric vector", {
  ufo <- ufo_numeric(1000000);
  ordinary <- numeric(1000000);
  expect_equal(ufo, ordinary);
})

test_that("empty logical vector", {
  ufo <- ufo_logical(1000000);
  ordinary <- logical(1000000);
  expect_equal(ufo, ordinary);
})

test_that("empty complex vector", {
  ufo <- ufo_complex(1000000);
  ordinary <- complex(1000000);
  expect_equal(ufo, ordinary);
})

test_that("empty raw vector", {
  ufo <- ufo_raw(1000000);
  ordinary <- raw(1000000);
  expect_equal(ufo, ordinary);
})

#test_that("empty character vector", {
  #ufo <- ufo_character(1000000);
  #ordinary <- character(1000000);
  #expect_equal(ufo, ordinary);
#})