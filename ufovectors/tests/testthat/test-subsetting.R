context("UFO vector subsetting")

test_ufo_subset <- function (data, subscript, ufo_constructor) {
  ufo <- ufo_constructor(length(data))
  ufo[seq_len(length(data))] <- data
  result <- ufovectors::ufo_subset(ufo, subscript)
  expect_equal(result, data[subscript])
}

test_that("ufo integer subset: NULL",       {test_ufo_subset(data=as.integer(1:100000), subscript=NULL,             ufo_integer)})
test_that("ufo integer subset: 0",          {test_ufo_subset(data=as.integer(1:100000), subscript=0,                ufo_integer)})
test_that("ufo integer subset: 1",          {test_ufo_subset(data=as.integer(1:100000), subscript=1,                ufo_integer)})
test_that("ufo integer subset: N/2",        {test_ufo_subset(data=as.integer(1:100000), subscript=50000,            ufo_integer)})
test_that("ufo integer subset: N",          {test_ufo_subset(data=as.integer(1:100000), subscript=100000,           ufo_integer)})
test_that("ufo integer subset: 1:10",       {test_ufo_subset(data=as.integer(1:100000), subscript=1:10,             ufo_integer)})
test_that("ufo integer subset: 1:1000",     {test_ufo_subset(data=as.integer(1:100000), subscript=1:1000,           ufo_integer)})
test_that("ufo integer subset: 1:N",        {test_ufo_subset(data=as.integer(1:100000), subscript=1:100000,         ufo_integer)})
test_that("ufo integer subset: N/2:N",      {test_ufo_subset(data=as.integer(1:100000), subscript=50000:100000,     ufo_integer)})
test_that("ufo integer subset: 2*(2:N/2)",  {test_ufo_subset(data=as.integer(1:100000), subscript=2*(1:50000),      ufo_integer)})
test_that("ufo integer subset: -1",         {test_ufo_subset(data=as.integer(1:100000), subscript=-1,               ufo_integer)})
test_that("ufo integer subset: -(N/2)",     {test_ufo_subset(data=as.integer(1:100000), subscript=-50000,           ufo_integer)})
test_that("ufo integer subset: -N",         {test_ufo_subset(data=as.integer(1:100000), subscript=-100000,          ufo_integer)})
test_that("ufo integer subset: -(1:10)",    {test_ufo_subset(data=as.integer(1:100000), subscript=-(1:10),          ufo_integer)})
test_that("ufo integer subset: -(1:1000)",  {test_ufo_subset(data=as.integer(1:100000), subscript=-(1:1000),        ufo_integer)})
test_that("ufo integer subset: -(1:N)",     {test_ufo_subset(data=as.integer(1:100000), subscript=-(1:100000),      ufo_integer)})
test_that("ufo integer subset: -(N/2:N)",   {test_ufo_subset(data=as.integer(1:100000), subscript=-(50000:100000),  ufo_integer)})
test_that("ufo integer subset: -2*(2:N/2)", {test_ufo_subset(data=as.integer(1:100000), subscript=-2*(1:50000),     ufo_integer)})

