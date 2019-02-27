#include "maskirovka.h"
#include "maskirovka_altrep.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {
    {"mask_new", (DL_FUNC) &mask_new, 1},
    {"mask_new_altrep", (DL_FUNC) &mask_new_altrep, 1},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

// Initializes the package and registers the routines with the Rdynload 
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_maskirovka(DllInfo *dll) {
    InitMaskirovkaAltRepClass(dll);
    //R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    //R_useDynamicSymbols(dll, FALSE);
    //R_forceSymbols(dll, TRUE);
}


