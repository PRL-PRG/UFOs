#mkInclude = sparseList.h ../unstdLib/*.h
LIB = blake3.a

OBJ = blake3_avx2_x86-64_unix.o blake3_avx512_x86-64_unix.o blake3.o blake3_dispatch.o blake3_portable.o blake3_sse2_x86-64_unix.o blake3_sse41_x86-64_unix.o
all: $(LIB) $(OBJ)

library: $(LIB)

mostlyclean: clean

clean:
	$(RM) *.o
	$(RM) *.a

%.o: %.c %.S
	gcc -fPIC -O3 -c -o $@ $^

$(LIB): $(OBJ)
	$(AR) -rcs $@ $^


