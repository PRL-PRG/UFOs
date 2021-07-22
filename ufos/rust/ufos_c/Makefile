

ifeq ($(DEBUGGING), 1)
	CARGOFLAGS=build
	LIB_PATH="target/debug"
else
	CARGOFLAGS=build --release
	LIB_PATH="target/release"
endif

.PHONY: ufos_c clean

all: ufos_c

ufos_c:
	cargo $(CARGOFLAGS)
	cp $(LIB_PATH)/libufos_c.so ./
	cp $(LIB_PATH)/libufos_c.a ./

clean:
	cargo clean
	rm -f libufos_c.so
	rm -f libufos_c.a
