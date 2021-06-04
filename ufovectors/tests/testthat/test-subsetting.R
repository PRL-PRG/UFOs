context("UFO vector subscripting")

test_ufo_subset <- function (data, subscript, ufo_constructor) {
  ufo <- ufo_constructor(length(data))
  ufo[seq_len(length(data))] <- data
  result <- ufovectors::ufo_subset(ufo, subscript)
  expect_equal(result, data[subscript])
}

test_that("ufo integer subset", {test_ufo_subset(data=as.integer(1:100000), subscript=1:1000, ufo_integer)})
test_that("ufo numeric subset", {test_ufo_subset(data=as.numeric(1:100000), subscript=1:1000, ufo_numeric)})