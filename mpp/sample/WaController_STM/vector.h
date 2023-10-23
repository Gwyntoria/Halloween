#ifndef VECTOR_H
#define VECTOR_H
#include <stddef.h>

typedef struct Vector_ Vector;

extern Vector* vector_new(size_t elem_size);
extern    void vector_free(Vector* v);
extern  size_t vector_length(Vector* v);
extern    void vector_get(Vector* v, size_t pos, void* elem_out);
extern    void vector_set(Vector* v, size_t pos, void* elem_in);
extern     int vector_append(Vector* v, void* elem_in); // On failure, returns -1. 

#endif // VECTOR_H