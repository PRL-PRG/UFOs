#include "rash.h"

#include <stdint.h>

#include "safety_first.h"
#include "ufo_empty.h"

//-----------------------------------------------------------------------------
// Problem children
//
// R macros that are not accessible from outside R, but which I need and which
// I could not get around.
//-----------------------------------------------------------------------------

#ifndef IS_CACHED
#ifndef CACHED_MASK
#define CACHED_MASK (1<<5)
#endif
#define IS_CACHED(x) (((x)->sxpinfo.gp) & CACHED_MASK)
#endif

#ifndef IS_BYTES
#ifndef BYTES_MASK
#define BYTES_MASK (1<<1)
#endif
#define IS_BYTES(x) ((x)->sxpinfo.gp & BYTES_MASK)
#endif

#ifndef ENC_KNOWN
#ifndef LATIN1_MASK
#define LATIN1_MASK (1<<2)
#endif
#ifndef UTF8_MASK
#define UTF8_MASK (1<<3)
#endif
#define ENC_KNOWN(x) ((x)->sxpinfo.gp & (LATIN1_MASK | UTF8_MASK))
#endif

//--------------------------------------------------------------------------------
// rash: An implementation of a string hash table with SEXPs.
//
// Roughly meant to mirror a subset of the implementation used in match5 in
// unique.c.
//--------------------------------------------------------------------------------

inline static void ensure_type(SEXP sexp, SEXPTYPE expected_type) {
	(TYPEOF(sexp) == expected_type, Rf_error,
			"expected %s but found %s",
			type2char(expected_type),
			type2char(TYPEOF(sexp)));
}

// String comparison function for CHARSXPs (mirrors Seql from memory.c).
bool strings_are_equal (SEXP/*CHARSXP*/ string_a, SEXP/*CHARSXP*/ string_b) {
	ensure_type(string_a, CHARSXP);
	ensure_type(string_b, CHARSXP);

    if (string_a == string_b)  return true;
    if (string_a == NA_STRING) return false;
    if (string_b == NA_STRING) return false;

    if (IS_CACHED(string_a) && IS_CACHED(string_b)
        && ENC_KNOWN(string_a) == ENC_KNOWN(string_b)) {
    	return false;
    }

    const void *vmax = vmaxget();
	bool result = !strcmp(translateCharUTF8(string_a), translateCharUTF8(string_b));
	vmaxset(vmax); /* discard any memory used by translateChar */

	return result;
}

static inline R_xlen_t scatter(unsigned int key, unsigned int senior_bits_in_hash) {
    return 3141592653U * key >> (32 - senior_bits_in_hash);
}

// Produces a hash of a string (mirrors cshash from unique.c).
static inline R_xlen_t generate_c_string_hash(examined_string_t string,
											  unsigned int senior_bits_in_hash) {
	ensure_type(string.sexp, CHARSXP);

    intptr_t c_string_as_pointer = (intptr_t) string.sexp;
	unsigned int masked_pointer = (unsigned int)(c_string_as_pointer & 0xffffffff);

	#if SIZEOF_LONG == 8
			unsigned int exponent = (unsigned int)(c_string_as_pointer/0x100000000L);
	#else
			unsigned int exponent = 0;
	#endif

	return scatter(masked_pointer ^ exponent, senior_bits_in_hash);
}

// Produces a hash of a string (mirrors a subset of shash from unique.c).
static inline R_xlen_t generate_utf_string_hash(examined_string_t string,
												unsigned int senior_bits_in_hash) {
	ensure_type(string.sexp, CHARSXP);

	const void *vmax = vmaxget();
	const char *string_contents = translateCharUTF8(string.sexp);

	unsigned int key = 0;
	while (*string_contents++) {
		key = 11 * key + (unsigned int) *string_contents; /* was 8 but 11 isn't a power of 2 */
	}
	vmaxset(vmax); /* discard any memory used by translateChar */

	return scatter(key, senior_bits_in_hash);
}

// Produces a hash of a string (mirrors shash from unique.c).
R_xlen_t generate_string_hash(examined_string_t string,
								   unsigned int senior_bits_in_hash) {

	return (!string.uses_utf8 && string.uses_cache)
			? generate_c_string_hash(string, senior_bits_in_hash)
			: generate_utf_string_hash(string, senior_bits_in_hash);
}

examined_string_vector_t make_examined_string_vector_from(SEXP/*STRSXP*/ strings) {
	ensure_type(strings, STRSXP);

	examined_string_vector_t examined;

	examined.sexp   = strings;
	examined.type   = TYPEOF(strings);
	examined.length = XLENGTH(strings);

	examined.uses_bytes = false;
	examined.uses_utf8  = false;
	examined.uses_cache = true;

	for (R_xlen_t index = 0; index < examined.length; index++) {
		SEXP element = STRING_ELT(examined.sexp, index);

		if (IS_BYTES(element)) {
			examined.uses_bytes = true;
			examined.uses_utf8  = false;
			break;
		}

		if (ENC_KNOWN(element)) {
			examined.uses_utf8  = true;
		}

		if(!IS_CACHED(element)) {
			examined.uses_cache = false;
			break;
		}
	}

	return examined;
}

examined_string_t make_examined_string_from(examined_string_vector_t strings, R_xlen_t index) {
	ensure_type(strings.sexp, STRSXP);
	make_sure(strings.length > index, "index %li out of bounds %li", index, strings.length);

	SEXP/*CHARSXP*/ string = STRING_ELT(strings.sexp, index);
	ensure_type(string, CHARSXP);

	examined_string_t examined;

	examined.sexp   = string;
	examined.type   = TYPEOF(string);
	examined.length = XLENGTH(string);

	examined.uses_bytes = strings.uses_bytes;
	examined.uses_utf8  = strings.uses_utf8;
	examined.uses_cache = strings.uses_cache;

	return examined;
}

R_xlen_t calculate_rash_length_for_string_vector(examined_string_vector_t strings) {
	ensure_type(strings.sexp, STRSXP);

	R_xlen_t hash_table_length = 2;

    while (hash_table_length < 2U * strings.length) {
    	hash_table_length *= 2;
    }

    return hash_table_length;
}

int32_t calculate_rash_senior_bits_in_hash(examined_string_vector_t strings) {
	ensure_type(strings.sexp, STRSXP);

	R_xlen_t hash_table_length = 2;
    int32_t senior_bits_in_hash = 1;

    while (hash_table_length < 2U * strings.length) {
    	hash_table_length *= 2;
    	senior_bits_in_hash++;
    }

    return senior_bits_in_hash;
}

rash_t rash_new(R_xlen_t size,  int32_t senior_bits_in_hash, int32_t min_load_count) {

	rash_t rash;
	rash.hash_table = PROTECT(ufo_empty(VECSXP, size, true, min_load_count));
	rash.available_space = size;
	rash.size = size;
	rash.senior_bits_in_hash = senior_bits_in_hash;

	return rash;
}

rash_t rash_from(examined_string_vector_t strings, int32_t min_load_count) {
	ensure_type(strings.sexp, STRSXP);

	R_xlen_t size = calculate_rash_length_for_string_vector(strings);
	R_xlen_t senior_bits_in_hash = calculate_rash_senior_bits_in_hash(strings);

	rash_t rash = rash_new(size, senior_bits_in_hash, min_load_count);
	rash_add_all(rash, strings);

	return rash;
}

void rash_free(rash_t rash) {
	UNPROTECT(1);
}

/*
 * Add an element to the hash table.
 *
 * Returns true if the element was added and false if it was already present.
 */
bool rash_add(rash_t rash, examined_string_t new_element) {
	ensure_type(rash.hash_table, VECSXP);
	ensure_type(new_element.sexp, CHARSXP);

	R_xlen_t new_element_hash = generate_string_hash(new_element, rash.senior_bits_in_hash);

	R_xlen_t index = new_element_hash;
	for (;; ) {
		SEXP/*CHARSXP*/ incumbent_element = VECTOR_ELT(rash.hash_table, index);

		if (incumbent_element == R_NilValue) break;
		if (strings_are_equal(incumbent_element, new_element.sexp)) return false;

		index = (index + 1) % rash.size;
	}

	if (rash.available_space == 0) {
		Rf_error("hash table is full");
		return false; // Nonsense, but keeps linter babies happy.
	} else {
		rash.available_space--;
	}

	SET_VECTOR_ELT(rash.hash_table, index, new_element.sexp);
	return true;
}

/*
 * Add multiple elements to the hash table.
 *
 * Returns true if at least one element was added and false if all elements were already present.
 */
bool rash_add_all(rash_t rash, examined_string_vector_t strings) {
	ensure_type(rash.hash_table, VECSXP);
	ensure_type(strings.sexp, STRSXP);

	R_xlen_t strings_length = strings.length;
	bool added_something = false;

	for (R_xlen_t i = 0; i < strings_length; i++) { // there's probably a better way to write this loop without using STRING_ELT
		examined_string_t string = make_examined_string_from(strings, i);
		added_something |= rash_add(rash, string);
	}

	UNPROTECT(1);
	return added_something;
}

/*
 * Checks if the element is a member in the rash.
 */
bool rash_member(rash_t rash, examined_string_t outside_element) {
	ensure_type(rash.hash_table, VECSXP);
	ensure_type(outside_element.sexp, CHARSXP);

	R_xlen_t outside_element_hash = generate_string_hash(outside_element, rash.senior_bits_in_hash);

	for (R_xlen_t index = outside_element_hash;; index = (index + 1) % rash.size) {
		SEXP/*CHARSXP*/ incumbent_element = VECTOR_ELT(rash.hash_table, index);

		if (incumbent_element == R_NilValue) return false;
		if (strings_are_equal(incumbent_element, outside_element.sexp)) return true;
	}

	return false;
}

/*
 * Checks if the elements are members in the rash.
 *
 * Returns a logical vector containing R_TRUE or R_FALSE for each element,
 * comporting to position in the strings vector.
 */
SEXP/*LGLSXP*/ rash_member_all(rash_t rash, examined_string_vector_t strings, int32_t min_load_count) {
	ensure_type(rash.hash_table, VECSXP);
	ensure_type(strings.sexp, STRSXP);

	R_xlen_t strings_length = strings.length;
	SEXP/*LGLSXP*/ result = PROTECT(ufo_empty(LGLSXP, strings_length, false, min_load_count));

	for (R_xlen_t i = 0; i < strings_length; i++) {
		examined_string_t string = make_examined_string_from(strings, i);
		bool is_member = rash_member(rash, string);
		SET_LOGICAL_ELT(result, i, is_member);
	}

	UNPROTECT(1);
	return result;
}

irash_t irash_from(examined_string_vector_t strings, int32_t min_load_count) {
	ensure_type(strings.sexp, STRSXP);

	R_xlen_t size = calculate_rash_length_for_string_vector(strings);
	R_xlen_t senior_bits_in_hash = calculate_rash_senior_bits_in_hash(strings);

	irash_t irash;
	irash.hash_to_index_table = //PROTECT(allocVector(REALSXP, size));
		PROTECT(ufo_empty(REALSXP, size, true, min_load_count));

	//for (R_xlen_t i = 0; i < size; i++) { SET_REAL_ELT(irash.hash_to_index_table, i, NA_REAL); }

	irash.origin = strings;
	irash.available_space = size;
	irash.size = size;
	irash.senior_bits_in_hash = senior_bits_in_hash;

	irash_add_all(irash, strings);

	return irash;
}

void irash_free(irash_t irash) {
	UNPROTECT(1);
}

/*
 * Add an element to the index-based hash table.
 *
 * Returns true if the element was added and false if it was already present.
 */
bool irash_add(irash_t irash, R_xlen_t index_of_element_in_origin) {
	ensure_type(irash.hash_to_index_table, REALSXP);
	ensure_type(irash.origin.sexp, STRSXP);

	make_sure(index_of_element_in_origin >= 0 && index_of_element_in_origin < irash.origin.length,
			  "index out of range");

	examined_string_t new_element =
			make_examined_string_from(irash.origin, index_of_element_in_origin);

	ensure_type(new_element.sexp, CHARSXP);

	R_xlen_t hash_to_index_table_length = XLENGTH(irash.hash_to_index_table);
	R_xlen_t new_element_hash = generate_string_hash(new_element, irash.senior_bits_in_hash);

	R_xlen_t index = new_element_hash;
	for (;; ) {
		if (index >= hash_to_index_table_length) {
			Rf_error("String hash %i exceeds the size of the hash table %i.", 
			          index, hash_to_index_table_length);
		}

		R_xlen_t incumbent_index = (R_xlen_t) REAL_ELT(irash.hash_to_index_table, index); // seggy ix: 134446

		R_xlen_t butts = (R_xlen_t) NA_REAL;
		if (incumbent_index == butts) break;

       	SEXP/*CHARSXP*/ incumbent_element = VECTOR_ELT(irash.origin.sexp, incumbent_index);

		if (incumbent_element == R_NilValue) break;
		if (strings_are_equal(incumbent_element, new_element.sexp)) return false;

		index = (index + 1) % irash.size;
	}

	if (irash.available_space == 0) {
		Rf_error("hash table is full");
		return false; // Nonsense, but keeps linter babies happy.
	} else {
		irash.available_space--;
	}

	if (index >= XLENGTH(irash.hash_to_index_table)) {
		Rf_error("Hash table index out of bounds.");
	}
	SET_REAL_ELT(irash.hash_to_index_table, index, index_of_element_in_origin);
	return true;
}

/*
 * Add multiple elements to the index-based hash table.
 *
 * Returns true if at least one element was added and false if all elements
 * were already present.
 */
bool irash_add_all(irash_t irash, examined_string_vector_t strings) {
	make_sure(irash.origin.sexp == strings.sexp,
			  "cannot add from vector other than origin");

	ensure_type(irash.hash_to_index_table, REALSXP);
	ensure_type(irash.origin.sexp, STRSXP);
	ensure_type(strings.sexp, STRSXP);

	bool added_something = false;
	for (R_xlen_t i = 0; i < strings.length; i++) {
		added_something |= irash_add(irash, i);
	}
	return added_something;
}

/*
 * Checks if the element is a member in the rash and, if so, retrieves its
 * index in the origin vector.
 *
 * If the element is not a member,  the function returns false. If the element
 * is a member, the function returns true and writes the elements index in
 * the origin vector to out_index. The out-index is zero-based.
 */
bool irash_member(irash_t irash, examined_string_t outside_element, R_xlen_t *out_index) {
	ensure_type(irash.hash_to_index_table, REALSXP);
	ensure_type(irash.origin.sexp, STRSXP);
	ensure_type(outside_element.sexp, CHARSXP);

	R_xlen_t outside_element_hash = generate_string_hash(outside_element, irash.senior_bits_in_hash);
	R_xlen_t hash_to_index_table_length = XLENGTH(irash.hash_to_index_table);

	for (R_xlen_t index = outside_element_hash;; index = (index + 1) % irash.size) {

		if (index >= hash_to_index_table_length) {
			Rf_error("String hash %i exceeds the size of the hash table (size=%i).", 
			          index, hash_to_index_table_length);
		}

		R_xlen_t incumbent_index = (R_xlen_t) REAL_ELT(irash.hash_to_index_table, index);

		R_xlen_t butts = (R_xlen_t) NA_REAL;
		if (incumbent_index == butts) return false;

		if (incumbent_index >= irash.origin.length) {
			Rf_error("No string at index %i in the hash table (size=%i).", 
			          incumbent_index, irash.origin.length);
		}

		SEXP/*CHARSXP*/ incumbent_element = VECTOR_ELT(irash.origin.sexp, incumbent_index);

		if (incumbent_element == R_NilValue) return false;
		if (!strings_are_equal(incumbent_element, outside_element.sexp)) continue;

		*out_index = incumbent_index;
		return true;
	}

	return false;
}

/*
 * Counts the number of elements from strings that are found in the
 * index-based hash table.
 */
R_xlen_t irash_count_members(irash_t irash, examined_string_vector_t strings) {
	ensure_type(irash.hash_to_index_table, REALSXP);
	ensure_type(irash.origin.sexp, STRSXP);
	ensure_type(strings.sexp, STRSXP);

	// XXX there's probably a better way to write this loop without using STRING_ELT
	R_xlen_t member_count = 0;
	for (R_xlen_t i = 0; i < strings.length; i++) {
		examined_string_t string = make_examined_string_from(strings, i);
		R_xlen_t index; // ignored
		if (irash_member(irash, string, &index)) {
			member_count++;
		}
	}
	return member_count;
}

/*
 * Retrieves the indices in the irash's origin vector of all elements of
 * strings that occur in the irash.
 *
 * Return a vector of indices (REALSXP that contains R_xlen_t). For each
 * element of strings that occurs in the irash, if the element is a member,
 * its index is inserted into the output vector. If the element is not a
 * member, then it is disregarded when creating the output vector.
 *
 * The returned indices are ONE-based.
 */
SEXP/*REALSXP:R_xlen_t*/ irash_all_member_indices(irash_t irash, examined_string_vector_t strings, int32_t min_load_count) {
	ensure_type(irash.hash_to_index_table, REALSXP);
	ensure_type(strings.sexp, STRSXP);

	//R_xlen_t result_length = irash_count_members(irash, strings);
	R_xlen_t result_length = strings.length; // because this just is positional, right?
	SEXP/*REALSXP:R_xlen_t*/ result = PROTECT(ufo_empty(REALSXP, result_length, true, min_load_count));

	for (R_xlen_t si = 0, ri = 0; si < strings.length; si++) {
		examined_string_t string = make_examined_string_from(strings, si);

		if (string.sexp == R_NaString) {
			SET_REAL_ELT(result, ri, NA_REAL);
			ri++;
			continue;
		}

		R_xlen_t index;
		bool is_member = irash_member(irash, string, &index);

		if (is_member) {
			SET_REAL_ELT(result, ri, index + 1); // FIXME this +1 stuff will lead tor overflows, must fix
		}
		ri++;
	}

	UNPROTECT(1);
	return result;
}
