# Maskirovka
Experiments with custom objects masquerading as plain old R objects.

## Installation

```bash
kondziu@elma:~/Workspace/maskirovka$ R CMD INSTALL --preclean $PATH_TO_PROJECT
* installing to library ‘/home/kondziu/R/x86_64-pc-linux-gnu-library/3.4’
* installing *source* package ‘maskirovka’ ...
** libs
gcc -std=gnu99 -I/usr/share/R/include -DNDEBUG      -fpic  -g -O2 -fdebug-prefix-map=/build/r-base-AitvI6/r-base-3.4.4=. -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -g  -c init.c -o init.o
gcc -std=gnu99 -I/usr/share/R/include -DNDEBUG      -fpic  -g -O2 -fdebug-prefix-map=/build/r-base-AitvI6/r-base-3.4.4=. -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -g  -c maskirovka.c -o maskirovka.o
g++ -shared -L/usr/lib/R/lib -Wl,-Bsymbolic-functions -Wl,-z,relro -o maskirovka.so init.o maskirovka.o -L/usr/lib/R/lib -lR
installing to /home/kondziu/R/x86_64-pc-linux-gnu-library/3.4/maskirovka/libs
** R
** preparing package for lazy loading
** help
*** installing help indices
** building package indices
** testing if installed package can be loaded
* DONE (maskirovka)
```

## Getting started

Load the library:

```R
library(maskirovka)
```

Create a custom 10-element vector. 

```R
vec <- mask_new(10)
```

Create a collection of custom vectors. 

```R
vecs <- mask_new(c(5,10,15))
```