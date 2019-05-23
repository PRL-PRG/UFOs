#pragma once

typedef int (*reader_function)(FILE* file, uint64_t start, uint64_t end,
                               ufUserData user_data);

enum ufo_vector_type_t {
    UFO_CHAR = 9,
    UFO_LGL = 10,
    UFO_INT = 13,
    UFO_REAL = 14,
    UFO_CPLX = 15
};

typedef struct {
    ufUserData*         data;
    ufPopulateRange*    population_function;
    enum ufo_vector_type_t vector_type;
    //ufUpdateRange*    update_function;
    R_xlen_t            length;
} ufo_source_t;

