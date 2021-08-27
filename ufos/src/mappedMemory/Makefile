#mkInclude = sparseList.h ../unstdLib/*.h
#CC=gcc
# LDLIBS = -L. -lufos_c -pthread
LDLIBS = -Wl,--no-as-needed -lpthread -lrt -ldl -lm -lstdc++ libufos_c.a
#LDFLAGS = "-Wl,-rpath=\$$ORIGIN/" -Wl,-z,origin
LDFLAGS = 
CFLAGS = -fpic -g -O2 -Wall -Werror -DNDEBUG -I../../rust/ufos_c/target/
#CFLAGS = -Og -ggdb -Wall -Werror -fPIC

.PHONY: clib

clib:
	$(MAKE) -C ../../rust/ufos_c/

all: userfaultSpeedTest ufoTest ufoSpeedTest

mostlyclean: clean

clean:
	$(RM) *.o userfaultSpeedTest ufoTest ufoSpeedTest

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

userfaultSpeedTest: userfaultSpeed.o clib $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) userfaultSpeed.o $(OBJ) $(LDLIBS) -o $@

ufoTest: ufoTest.o clib $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) ufoTest.o $(OBJ) $(LDLIBS) -o $@

ufoSpeedTest: ufoSpeedTest.o clib $(OBJ) 
	$(CC) $(CFLAGS) $(LDFLAGS) ufoSpeedTest.o $(OBJ) $(LDLIBS) -o $@

