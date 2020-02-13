#include "ufoseq.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {
    // Constructors
    {"ufo_seq",  (DL_FUNC) &ufo_seq,  3},

    // Terminates the function list. Necessary, do not remove.
    {NULL, NULL, 0} 
};