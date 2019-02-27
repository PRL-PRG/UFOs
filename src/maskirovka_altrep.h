#ifndef __MASKIROVKA_ALTREP_H__
#define __MASKIROVKA_ALTREP_H__

#include <R.h>
#include <Rinternals.h>

void InitMaskirovkaAltRepClass(DllInfo * dll);

SEXP/*INTSXP|VECSXP<INTSXP>*/ mask_new_altrep(SEXP/*INTSXP*/ vector_lengths);

#endif
