#mkInclude = sparseList.h ../unstdLib/*.h
#CC=gcc
UFO_KEY = -D__UFOs_CoreDev__
PKG_LIBS = -pthread
OBJ = sparseList.o userfaultCore.o userfaultGettersSetters.o oroboros.o
#CFLAGS = -fpic -g -O2 -Wall -Werror
CFLAGS = -Og -ggdb -Wall -Werror -fPIC
LIB = libmappedMemory.a
LIBS = -lcrypto

all: $(LIB) strideTest sparseListTest userfaultSpeedTest ufoTest

library: $(LIB)

mostlyclean: clean

clean:
	$(RM) *.o
	$(RM) *.a

%.o: %.c
	$(CC) $(CFLAGS) $(UFO_KEY) -c -o $@ $< $(LIBS)

$(LIB): $(OBJ)
	$(AR) -rcs $@ $^

strideTest: stride.o
	$(CC) $(CFLAGS) -o $@ $^

sparseListTest: sparseListTest.o sparseList.o
	$(CC) $(CFLAGS) -o $@ $^

userfaultSpeedTest: userfaultSpeed.o $(OBJ)
	$(CC) $(CFLAGS) $(PKG_LIBS) -o $@ $^ $(LIBS)

ufoTest: ufoTest.o $(OBJ)
	$(CC) $(CFLAGS) $(PKG_LIBS) -o $@ $^ $(LIBS)
