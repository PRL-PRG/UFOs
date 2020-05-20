#pragma once

#include <R.h>

SEXP/*CHARSXP*/ mkBadChar   (const char* contents);
SEXP/*STRSXP*/  mkBadString (const char* contents);