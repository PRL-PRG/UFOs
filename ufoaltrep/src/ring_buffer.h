#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

SEXP ring_buffer_new(R_xlen_t size);
R_xlen_t ring_buffer_get_size(SEXP ring);
int ring_buffer_get_head(SEXP ring);
void ring_buffer_set_head(SEXP ring, int new_head);
void ring_buffer_set_boundary_start_at(SEXP ring, R_len_t position, int value);

void ring_buffer_set_boundary_end_at(SEXP ring, R_len_t position, int value);
int ring_buffer_get_boundary_start_at(SEXP ring, R_len_t position);
int ring_buffer_get_boundary_end_at(SEXP ring, R_len_t position);
int ring_buffer_bump_head(SEXP ring);
SEXP/*VECSXP*/ ring_buffer_get_cache(SEXP ring);
SEXP/*VECSXP*/ ring_buffet_get_cache_vector_at(SEXP ring, R_xlen_t position);
void ring_buffer_set_boundaries(SEXP ring, int start_index, int end_index);
void ring_buffer_add_vector_to_cache(SEXP ring, SEXP any_vector, int start_index, R_xlen_t length);

SEXP ring_buffer_make_cache_vector(SEXP ring, SEXPTYPE type, int start_index, R_xlen_t length);
int ring_buffer_find_cache(SEXP ring, int element_index);

int ring_buffer_get_element_at_cache(SEXP ring, int element_index, int cache_index, int *element);
int ring_buffer_get_element_at(SEXP ring, int element_index, int *element);

void ring_buffer_copy(SEXP source, SEXP target);
void ring_buffer_clear(SEXP ring);
