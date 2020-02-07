#devtools::install_github("olafmersmann/microbenchmarkCore")
#devtools::install_github("olafmersmann/microbenchmark")

library(microbenchmark)
library(ggplot2)

library(ufovectors)
library(ufoaltrep)

setwd("~/Workspace/ufo_workspace/UFOs/ufovectors/benchmark")

result1 <- microbenchmark(
  "UFO" = {
    sum(ufo_integer_bin("32Mints.bin"))
  },
  #"ALTREP" = {
  #  sum(altrep_ufo_integer_bin("32Mints.bin"))
  #},
  times = 10L
)
#ufo_shutdown()

autoplot(result)