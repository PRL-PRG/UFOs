context("UFO vector subscripting")

test_that("ufo integer subset", {
  data <- 1:100000
  ufo <- ufo_integer(length(data))
  ufo[seq_len(length(data))] <- data

  subscript <- 1:1000

  result <- ufovectors::subset(ufo, subscript)

  expect_equal(result, data[subscript])
})