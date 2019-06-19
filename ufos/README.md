# UFOs
Experiments with user faults to implement custom objects masquerading as plain 
old R vectors.

## Installation

```bash
kondziu@elma:~/Workspace/UFOs$ ../R-3.5.1/bin/R CMD INSTALL . --preclean
* installing to library ‘/home/kondziu/Workspace/R-3.5.1/library’
* installing *source* package ‘ufos’ ...
** libs
gcc -I"/home/kondziu/Workspace/R-3.5.1/include" -DNDEBUG   -ggdb   -fpic  -ggdb  -c init.c -o init.o
gcc -I"/home/kondziu/Workspace/R-3.5.1/include" -DNDEBUG   -ggdb   -fpic  -ggdb  -c ufos.c -o ufos.o
gcc -I"/home/kondziu/Workspace/R-3.5.1/include" -DNDEBUG   -ggdb   -fpic  -ggdb  -c ufos_altrep.c -o ufos_altrep.o
gcc -shared -L/usr/local/lib -o ufos.so init.o ufos.o ufos_altrep.o
installing to /home/kondziu/Workspace/R-3.5.1/library/ufos/libs
** R
** byte-compile and prepare package for lazy loading
** help
*** installing help indices
** building package indices
** testing if installed package can be loaded
* DONE (ufos)
```

## Getting started

Load the library:

```R
library(ufos)
```

Create a custom 10-element vector. 

```R
vec <- ufo.new(10)
```

Create a collection of custom vectors. 

```R
vecs <- ufo.new(c(5,10,15))
```