CC=gcc

ifeq ($(shell uname), Darwin)
	argp:=-largp -pagezero_size 10000 -image_base 100000000
endif

all:
	git submodule update --init --recursive
	cd ./LuaJIT && make -j
	$(CC) main.c -O3 -c -o main.o -I ./LuaJIT/src
	$(CC) main.o -lpthread -lm ${argp} -ldl ./LuaJIT/src/libluajit.a -o bench

