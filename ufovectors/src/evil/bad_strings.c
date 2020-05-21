#include <R.h>
#define USE_RINTERNALS
#include <Rinternals.h>

#ifndef intCHARSXP
#define intCHARSXP 73
#endif

#ifndef CHAR
#define CHAR(x) ((const char *) STDVEC_DATAPTR(x))
#endif

#ifndef CHAR_RW
#define CHAR_RW(x) ((char *) CHAR(x))
#endif

#ifndef ASCII_MASK
#define ASCII_MASK (1<<6)
#endif

#ifndef SET_ASCII
#define SET_ASCII(x) ((x)->sxpinfo.gp) |= ASCII_MASK
#endif

SEXP/*CHARSXP*/ mkBadChar(const char* contents) {
    if (0 == strcmp(contents, ""))       return R_BlankString;
    if (0 == strcmp(contents, "NA"))     return NA_STRING;
    //if (0 == strcmp(contents, "base"))   return R_BaseNamespaceName;

    SEXP/*CHARSXP*/ bad_string;
    R_len_t size = strlen(contents);

    PROTECT(bad_string = allocVector(intCHARSXP, size));
    memcpy(CHAR_RW(bad_string), contents, size);
    SET_ASCII(bad_string);                                  // FIXME
    UNPROTECT(1);
    return bad_string;
}

SEXP/*STRSXP*/ mkBadString(const char* contents) {
    SEXP bad_string_vector;
    SEXP bad_string;

    PROTECT(bad_string_vector = allocVector(STRSXP, 1));
    PROTECT(bad_string = mkBadChar(contents));
    SET_STRING_ELT(bad_string_vector, 0, bad_string);
    UNPROTECT(2);
    return bad_string_vector;
}