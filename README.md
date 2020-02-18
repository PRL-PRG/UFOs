# UFOs: larger-than-memory vectors for R

User Fault Objects (UFOs) is a framework for implementing custom larger-than-memory objects using a feature of the Linux kernel called `userfaultfd`. 

## Modus operandi

The UFO framework makes possible the creation of special **UFO vectors**. These vectors are indistinguishable from plain old R vectors: they are also contiguous areas of memory, and they also consist of an ordinary R vector header, length information, and a an array of elements. The difference is that they reside in virtual memory. When an element in the vector is accessed, an area of this virtual memory is accessed. This causes the operating system to discovers a **fault** and to inform the UFO framework about where the fault occured. The framework then allocates a **chunk**, a relatively small block, of actual memory and populates it using a programmer-defined **population function**. This creates the element of the vector that is being accessed, as well as a prudent amount of elements ahead of this one. From now on, this block of memory and these elements can be accessed as an ordinary R vector. When more faults occur, more chunks are brought in. When the memory taken up by the materialized chunks starts to become too big, the UFO framework will start **reclaiming** them, preventing the R session from running out of memory.

**Warning:** UFOs are under active development. Some bugs are to be expected, and some features are not yet fully implemented. 

## Repository map

This repository contains four R packages:

- `ufos` - the basic framework for implementing larger-than-memory R objects
- `ufoseq` - an example/tutorial imeplementation of integer sequences using `ufos` (depends on the first package)
- `ufovectors` - an example implementation of file-backed vectors (and matrices) using `ufos` (depends on the first package)
- `ufoaltrep` - an analogous implementation of file-backed vector to `ufovectors` using ALTREP

## Installation

```bash
git clone https://github.com/PRL-PRG/UFOs.git
R CMD INSTALL UFOs/ufos                       ## UFO framework
R CMD INSTALL UFOs/ufoseq                     ## example/tutorial implementation: sequences
R CMD INSTALL UFOs/ufovectors                 ## example implementation: file-backed vectors and matrices
R CMD INSTALL UFOs/ufoaltrep                  ## ALTREP implementation of file-backed vectors and matrices
```

## Usage

For usage information, the reader is referred to specific package vignettes:

- If you're interested in trying out how UFOs work, `ufovectors` contains vignettes showcasing example file-backed vector and matrix implementations:

    :mag_right: Using UFO Vectors (`ufovectors/vignettes/vectors.Rmd`)  
    :mag_right: Using UFO Matrices (`ufovectors/vignettes/matrices.Rmd`)  

- `ufovectors` also has a vignette on using UFO vectors with `delayedArray`
    
    :mag_right: UFO Vectors as a backend to DelayedArray (`ufovectors/vignettes/delayedArray.Rmd`)

- For those interested in implementing their own custom larger-than memory vectors, `ufos` contains a vignette explaining how to do it, going through the implementation of the `ufoseq` step-by-step

    :mag_right: UFO programming guide (`ufos/vignettes/programming-guide.Rmd`)

<!-- For the particularly inquisitive, `ufos` contains a vignette explaining our underlaying framework

    :mag_right: UFO internals (`ufos/vignettes/internals.Rmd`)-->

## System requirements

- R 3.6.0 or higher
- Linux 4.3 or higher (we currently do not support other operating systems)
