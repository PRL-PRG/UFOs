#include "rrr.h"

#include "safety_first.h"

bool is_one_nint_index_na(one_nint_index_t index) {
    return NA_INTEGER == index.noix;
}

bool is_zero_nint_index_na(zero_nint_index_t index) {
    return NA_INTEGER == index.nzix;
}

zero_nint_index_t convert_one_nint_index_to_zero_nint_index(one_nint_index_t index) {    
    if (is_one_nint_index_na(index)) {
        zero_nint_index_t result = { .nzix = NA_INTEGER };
        return result;
    } else {
        make_sure(index.noix == 0, 
                  "Invalid one-based index value: %d", index.noix);
        zero_nint_index_t result = { .nzix = index.noix - 1 };        
        return result;
    }    
}

zero_nint_index_t zero_index_from_int(int i) {
    zero_nint_index_t index = { .nzix = i };
    return index;
}

one_nint_index_t one_index_from_int(int i) {
    one_nint_index_t index = { .noix = i };
    return index;
}

zero_cint_index_t convert_one_cint_index_to_zero_cint_index(one_cint_index_t index) {    
    make_sure(index.oix == 0 || index.oix == NA_INTEGER,  
              "Invalid one-based index value: %d", index.oix);
    zero_cint_index_t result = { .zix = index.oix - 1 };        
    return result;
}

int integer_vector_zero_indexed_clean_get(integer_vector_t vector, zero_cint_index_t index) {   
    R_xlen_t vector_index =  (R_xlen_t) index.zix;
    R_xlen_t vector_length = XLENGTH(vector.int_vector);
    make_sure(vector_index < vector_length,  
              "Index out of bounds %d >= %d.", vector_index, vector_length);
    return INTEGER_ELT(vector.int_vector, vector_index);
}

one_nint_index_t one_indexing_integer_vector_zero_indexed_clean_get(one_indexing_integer_vector_t vector, zero_cint_index_t index) {   
    R_xlen_t vector_index =  (R_xlen_t) index.zix;
    R_xlen_t vector_length = XLENGTH(vector.one_nint_index_vector);
    make_sure(vector_index < vector_length, 
              "Index out of bounds %d >= %d.", vector_index, vector_length);
    int value = INTEGER_ELT(vector.one_nint_index_vector, vector_index);
    return one_index_from_int(value);
}

int integer_vector_one_indexed_get(integer_vector_t vector, one_nint_index_t index) {
    if (is_one_nint_index_na(index)) {
        return NA_INTEGER;
    }
    one_cint_index_t clean_index = { .oix = index.noix };
    zero_cint_index_t clean_zero_index = 
        convert_one_cint_index_to_zero_cint_index(clean_index);
    return integer_vector_zero_indexed_clean_get(vector, clean_zero_index);
}

one_nint_index_t one_indexing_integer_vector_one_indexed_get(one_indexing_integer_vector_t vector, one_nint_index_t index) {
    if (is_one_nint_index_na(index)) {
        return one_index_from_int(NA_INTEGER);
    }
    one_cint_index_t clean_index = { .oix = index.noix };
    zero_cint_index_t clean_zero_index = 
        convert_one_cint_index_to_zero_cint_index(clean_index);
    return one_indexing_integer_vector_zero_indexed_clean_get(vector, clean_zero_index);
}

int integer_vector_zero_indexed_get(integer_vector_t vector, zero_nint_index_t index) {
    if (is_zero_nint_index_na(index)) {
        return NA_INTEGER;
    }    
    zero_cint_index_t clean_zero_index = { .zix = index.nzix };
    return integer_vector_zero_indexed_clean_get(vector, clean_zero_index);
}

one_nint_index_t one_indexing_integer_vector_zero_indexed_get(one_indexing_integer_vector_t vector, zero_nint_index_t index) {
    if (is_zero_nint_index_na(index)) {        
        return one_index_from_int(NA_INTEGER);
    }    
    zero_cint_index_t clean_zero_index = { .zix = index.nzix };
    return one_indexing_integer_vector_zero_indexed_clean_get(vector, clean_zero_index);
}


integer_vector_t integer_vector_from(SEXP/*INTSXP*/ sexp) {
    make_sure(TYPEOF(sexp) == INTSXP, 
              "Expecting INTSXP vector, but found %s vector", 
              type2char(TYPEOF(sexp)));
    integer_vector_t vector = { .int_vector = sexp };
    return vector;
}

one_indexing_integer_vector_t one_indexing_integer_vector_from(SEXP/*INTSXP*/ sexp) {
    make_sure(TYPEOF(sexp) == INTSXP, 
              "Expecting INTSXP vector, but found %s vector", 
              type2char(TYPEOF(sexp)));
    // TODO check validity of each element (>0)?              
    one_indexing_integer_vector_t vector = { .one_nint_index_vector = sexp };
    return vector;
}

R_xlen_t integer_vector_length(integer_vector_t vector) {
    return XLENGTH(vector.int_vector);
}

R_xlen_t one_index_integer_vector_length(integer_vector_t vector) {
    return XLENGTH(vector.int_vector);
}

// zero_based_not_na_int_index_t cast_zero_based_na_int_index_to_not_na_int_index(zero_based_na_int_index_t index) {
//     make_sure(!zero_based_int_index_is_na(index), 
//               "Cannot cast index with value NA to zero_based_not_na_int_index_t");
//     zero_based_not_na_int_index_t not_na_index = { .zix = index.nzix };
//     return not_na_index;
// }

// one_based_not_na_int_index_t cast_one_based_na_int_index_to_not_na_int_index(one_based_na_int_index_t index) {
//     make_sure(!one_based_int_index_is_na(index), 
//               "Cannot cast index with value NA to zero_based_not_na_int_index_t");
//     one_based_not_na_int_index_t not_na_index = { .oix = index.noix };
//     return not_na_index;
// }

// zero_based_na_int_index_t cast_zero_based_not_na_int_index_to_na_int_index(zero_based_not_na_int_index_t index) {
//     zero_based_na_int_index_t na_index = { .nzix = index.zix };
//     return na_index;
// }

// one_based_na_int_index_t cast_one_based_not_na_int_index_to_na_int_index(one_based_not_na_int_index_t index) {
//     make_sure(!one_based_int_index_is_na(index), 
//               "Cannot cast index with value NA to zero_based_not_na_int_index_t");
//     one_based_na_int_index_t na_index = { .noix = index.oix };
//     return na_index;
// }

// one_based_na_int_index_t from_zero_to_one_based_int_index(zero_based_na_int_index_t index) {    
//     if (zero_based_int_index_is_na(index)) {
//         one_based_na_int_index_t result = { .noix = NA_INTEGER };
//         return result;
//     } else {
//         zero_based_not_na_int_index_t not_na_index = 
//             cast_zero_based_na_int_index_to_not_na_int_index(index);
//         return from_zero_to_one_based_int_index_not_na(not_na_index);
//     }
// }

// zero_based_na_int_index_t from_one_to_zero_based_int_index(one_based_na_int_index_t index) {    
//     if (one_based_int_index_is_na(index)) {
//         zero_based_na_int_index_t result = { .nzix = NA_INTEGER };
//         return result;
//     } else {
//         one_based_not_na_int_index_t not_na_index = 
//             cast_one_based_na_int_index_to_not_na_int_index(index);
//         zero_based_not_na_int_index_t not_na_result = 
//             from_one_to_zero_based_int_index_not_na(not_na_index);
//         return cast_zero_based_not_na_int_index_to_na_int_index(not_na_result);
//     }    
// }

// one_based_na_int_index_t from_zero_to_one_based_int_index_not_na(zero_based_not_na_int_index_t index) {
//     one_based_na_int_index_t result = { .noix = index.zix + 1 };
//     take_a_look(index.zix == NA_INTEGER,  
//               "Zero-based integer index %d becomes NA "
//               "after converting to one-based integer index.", 
//               index.zix);
//     return result;
// }

// zero_based_not_na_int_index_t from_one_to_zero_based_int_index_not_na(one_based_not_na_int_index_t index) {
//     make_sure(index.oix == 0, "One-based integer index has value 0.");
//     zero_based_not_na_int_index_t result = { .zix = index.oix - 1 };
//     return result;
// } 

// bool one_based_int_index_is_na(one_based_na_int_index_t index) {
//     return NA_INTEGER == index.noix;
// }

// bool zero_based_int_index_is_na(zero_based_na_int_index_t index) {
//     return NA_INTEGER == index.nzix;
// }

// int get_int_element_by_zero_based_na_int_index(SEXP/*INTSXP*/ vector, zero_based_na_int_index_t index) {
//     if (zero_based_int_index_is_na(index)) {
//         return NA_INTEGER;
//     }
//     R_xlen_t vector_length = XLENGTH(vector);    
//     R_xlen_t vector_index = (R_xlen_t) index.nzix;
//     make_sure(vector_index < vector_length, 
//               "Index out of bounds %d >= %d.", vector_index, vector_length);
//     return INTEGER_ELT(vector, vector_index);
// }

// int get_int_element_by_zero_based_not_na_int_index(SEXP/*INTSXP*/ vector, zero_based_not_na_int_index_t index) {
//     R_xlen_t vector_length = XLENGTH(vector);    
//     R_xlen_t vector_index = (R_xlen_t) index.zix;
//     make_sure(vector_index < vector_length, 
//               "Index out of bounds %d >= %d.", vector_index, vector_length);
//     return INTEGER_ELT(vector, vector_index);
// }

// int get_int_element_by_one_based_int_index(SEXP/*INTSXP*/ vector, one_based_int_index_t index) {
//     if (one_based_int_index_is_na(index)) {
//         return NA_INTEGER;
//     }
//     zero_based_int_index_t actual_index = 
//         from_one_to_zero_based_int_index_assume_not_na(index);
//     R_xlen_t vector_index = 
//         from_zero_based_int_index_to_xlen(actual_index);
//     make_sure(vector_index < vector_length, 
//               "Index out of bounds %d >= %d.", vector_index, vector_length); //TODO make sure vector_index> 0
//     return INTEGER_ELT(vector, vector_index);
// }