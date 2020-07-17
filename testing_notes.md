1. Download R

```bash
wget https://cran.r-project.org/src/base/R-4/R-4.0.2.tar.gz
tar -xzf R-4.0.2.tar.gz
mv R-4.0.2 /opt
cd /opt/R-4.0.2
```

2. Set environment to make R slow, but easy to debug. 

```bash
export CFLAGS="-O0 -g"
```

3. Build R

```bash
./configure && make
```

(add missing dependencies etc.)

4. Add some libs that will be needed.

```bash
bin/R
```

And in R:

```R
install.packages(c("testthat", "devtools", "dplyr", "knitr"));
devtools::install_github("PRL-PRG/viewports");
q();
```

5. Get UFOs repo

```bash
git clone https://github.com/PRL-PRG/UFOs.git PATH/UFOs
```


5. Install packages

```bash
bin/R CMD INSTALL PATH/UFOs/ufos --clean
bin/R CMD INSTALL PATH/UFOs/ufovectors --clean
```

6. Open R with gdb attached

```bash
bin/R -d gdb
```

```gdb
r
```

```R
setwd("PATH/UFOs/ufovectors"); devtools::test()
```

7. Experience SEGFAULT

The code that runs during tests is in ufovectors:

    - tests/testthat/test-empty-vectors.R, and then
    - rests/testthat/test-file-backed-sum.R

It looks like both of these are necessary to make the SEGFAULT happen.
