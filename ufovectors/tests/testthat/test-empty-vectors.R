context("Empty UFO vectors")

test_that("empty raw vector", {
	ufo <- ufo_raw(1000000);
	ordinary <- raw(1000000);
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