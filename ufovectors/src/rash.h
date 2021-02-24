#pragma once

#include <stdbool.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

typedef struct {
	SEXP/*CHARSXP*/ sexp;
	SEXPTYPE        type;
	R_xlen_t      length;

	bool uses_bytes;
	bool uses_utf8;
	bool uses_cache;

} examined_string_t;

typedef struct {
	SEXP/*STRSXP*/  sexp;
	SEXPTYPE        type;
	R_xlen_t      length;

	bool uses_bytes;
	bool uses_utf8;
	bool uses_cache;

} examined_string_vector_t;

examined_string_vector_t make_examined_string_vector_from(SEXP/*STRSXP*/ strings);
examined_string_t 		 make_examined_string_from       (examined_string_vector_t strings, R_xlen_t index);

R_xlen_t generate_string_hash(examined_string_t string, unsigned int senior_bits_in_hash);
bool 	 strings_are_equal	 (SEXP/*CHARSXP*/ string_a, SEXP/*CHARSXP*/ string_b);

typedef struct {
	SEXP/*VECSXP*/  		 hash_table;
	R_xlen_t	    		 size;
	R_xlen_t        	 	 available_space;
	int32_t       			 senior_bits_in_hash;

} rash_t; // R-ish-like hash set

typedef struct {
	SEXP/*REALSXP:R_xlen_t*/ hash_to_index_table;
	examined_string_vector_t origin;
	R_xlen_t	    		 size;
	R_xlen_t       			 available_space;
	int32_t        			 senior_bits_in_hash;

} irash_t; // index-based R-ish-like hash set

rash_t 		   rash_new        (R_xlen_t size, int32_t senior_bits_in_hash, int32_t min_load_count);
rash_t 		   rash_from       (examined_string_vector_t strings, int32_t min_load_count);
void   		   rash_free       (rash_t);
bool   		   rash_add        (rash_t, examined_string_t);
bool   		   rash_add_all    (rash_t, examined_string_vector_t);
bool  		   rash_member     (rash_t, examined_string_t);
SEXP/*LGLSXP*/ rash_member_all (rash_t, examined_string_vector_t, int32_t min_load_count);

irash_t 				 irash_from               (examined_string_vector_t strings, int32_t min_load_count);
void 					 irash_free               (irash_t);
bool                     irash_add                (irash_t, R_xlen_t index_of_element_in_origin);
bool                     irash_add_all            (irash_t, examined_string_vector_t);
R_xlen_t 				 irash_count_members      (irash_t, examined_string_vector_t);
bool 				     irash_member             (irash_t, examined_string_t, R_xlen_t *out_index);
SEXP/*REALSXP:R_xlen_t*/ irash_all_member_indices (irash_t, examined_string_vector_t, int32_t min_load_count);

