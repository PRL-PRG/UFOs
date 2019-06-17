#include "ufos.h"
#include "bin_file_source.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {
    // Creates a new UFO vector. The constructor takes 1 argument which defines
    // a source. A source is specifically a EXTPTRSXP that points to a
    // ufo_source_t struct.
    //{"ufo_new", (DL_FUNC) &ufo_new, 1},

    // Creates a source that reads data from a binary file into a UFO vector.
    // A source is specifically a EXTPTRSXP that points to a ufo_source_t
    // struct. The constructor takes 1 argument that defines a path to a
    // binary file.
    //{"ufo_bin_file_source", (DL_FUNC) &ufo_bin_file_source, 1},

    // Shut the UFO system down.
    {"ufo_shutdown", (DL_FUNC) &ufo_shutdown, 0},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

// Initializes the package and registers the routines with the Rdynload 
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_ufos(DllInfo *dll) {
    //InitUFOAltRepClass(dll);
    //R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    //R_useDynamicSymbols(dll, FALSE);
    //R_forceSymbols(dll, TRUE);
}


