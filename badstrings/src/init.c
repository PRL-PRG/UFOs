#include <R.h>
#define USE_RINTERNALS
#include <Rinternals.h>

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

SEXP bad_character() {
    const char *name = "hello world!";
    R_len_t len = 12;

    SEXP c;
    PROTECT(c = allocVector(73, len)); // intCHARSXP
    memcpy((char *)(((SEXPREC_ALIGN *) (c)) + 1), name, len); //CHAR_RW(c)
    ((c)->sxpinfo.gp) |= (1<<6);    //SET_ASCII(c);

    SEXP s;
    PROTECT(s = allocVector(STRSXP, 1));
    SET_STRING_ELT(s, 0, c);

    UNPROTECT(2);
    return s;
}

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {
    {"bad_character",  (DL_FUNC) &bad_character,  0},

    // Terminates the function list. Necessary.
    {NULL, NULL, 0} 
};

// Initializes the package and registers the routines with the Rdynload
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_ufovectors(DllInfo *dll) {
    //InitUFOAltRepClass(dll);
    //R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    //R_useDynamicSymbols(dll, FALSE);
    //R_forceSymbols(dll, TRUE);
}


