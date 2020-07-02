context("Sum on file-backed integer vectors")

create_bin_file <- function(name, template, repeats = 1) {
    path <- tempfile(name)
    handle <- file(path, "wb")
    for (i in 1:repeats) { writeBin(template, handle) }
    close(handle)
    path
} 

destroy_bin_file <- function(path) {
	unlink(path)
}

#test_that("sum 1K int ones", {
#    path <- create_bin_file(name="ufo1K", template=as.integer(rep(1,1000)), repeats=1)
#    expect_equal(sum(ufo_integer_bin(path)), 1000)
#    destroy_bin_file(path)
#})

#test_that("sum 1M int ones", {
#	path <- create_bin_file(name="ufo1M", template=as.integer(rep(1,1000000)), repeats=1)
#    expect_equal(sum(ufo_integer_bin(path)), 1000000)
#    destroy_bin_file(path)
#})

test_that("sum 1G int ones", {
	path <- create_bin_file(name="ufo1M", template=as.integer(rep(1,1000000)), repeats=1)
	a <- ufo_integer(100000000)
	b <- ufo_integer(100000000)
	c <- ufo_integer(100000000)
	d <- ufo_integer(100000000)
	e <- ufo_integer(100000000)
	f <- ufo_integer(100000000)
	g <- ufo_integer(100000000)
	h <- ufo_integer(100000000)
	rm(a); rm(b); rm(c); rm(d); rm(e); rm(f); rm(g); rm(h);
	print("butt");
	 
	#ufo_integer(1000000000)
    v <- ufo_integer(1000000) 
    #create_bin_file(name="ufo1G", template=as.integer(rep(1,1000000)), repeats=1000)
    #v <- ufo_integer_bin(path)
    s <- sum(v)
    expect_equal(s, 1000000000)
    #destroy_bin_file(path)
})

