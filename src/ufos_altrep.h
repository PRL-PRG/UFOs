#ifndef __UFOS_ALTREP_H__
#define __UFOS_ALTREP_H__

#include <R.h>
#include <Rinternals.h>

void InitUFOAltRepClass(DllInfo * dll);

SEXP/*INTSXP|VECSXP<INTSXP>*/ ufo_new_altrep(SEXP/*INTSXP*/ vector_lengths);

#endif
