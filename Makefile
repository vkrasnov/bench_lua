CC=gcc

ifeq ($(shell uname), Darwin)
	argp:=-largp -pagezero_size 10000 -image_base 100000000
endif

ifdef BENCH_DURATION
	DURATION = -DBENCH_DURATION=$(BENCH_DURATION)
endif

all: LuaJIT/Makefile
	cd ./LuaJIT && make -j
	$(CC) main.c $(DURATION) -O3 -c -o main.o -I ./LuaJIT/src
	$(CC) main.o -lpthread ${argp} ./LuaJIT/src/libluajit.a -lm -ldl -o bench

LuaJIT/Makefile:
	git submodule update --init --recursive

clean:
	cd ./LuaJIT && make clean
	rm bench main.o

distclean:
	rm -rf LuaJIT
	mkdir LuaJIT
	rm bench main.o
