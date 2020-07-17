context("UFO vector operators")

test_that("ufo binary +", {
	ufo <- ufo_integer(100000);
	ufo[1:100000]<-1:100000
	reference <- 1:100000
	argument <- 1:100000
	
	result_ufo       <- ufovectors:::`+.ufo`(ufo, argument)
	result_reference <- reference + argument
	
	expect_equal(result_ufo, result_reference)
	expect_true(is_ufo(result_ufo))
})

test_that("ufo binary -", {
  ufo <- ufo_integer(100000);
  ufo[1:100000]<-1:100000
  reference <- 1:100000
  argument <- 1:100000
  
  result_ufo       <- ufovectors:::`-.ufo`(ufo, argument)
  result_reference <- reference - argument
  
  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary *", {
  ufo <- ufo_integer(100000);
  ufo[1:100000]<-1:100000
  reference <- 1:100000
  argument <- 1:100000
  
  result_ufo       <- ufovectors:::`*.ufo`(ufo, argument)
  result_reference <- reference * argument
  
  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})

test_that("ufo binary /", {
  ufo <- ufo_integer(100000);
  ufo[1:100000]<-1:100000
  reference <- 1:100000
  argument <- 1:100000
  
  result_ufo       <- ufovectors:::`/.ufo`(ufo, argument)
  result_reference <- reference / argument
  
  expect_equal(result_ufo, result_reference)
  expect_true(is_ufo(result_ufo))
})