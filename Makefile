CC=gcc

ifeq ($(shell uname), Darwin)
	argp:=-largp -pagezero_size 10000 -image_base 100000000
endif

ifdef BENCH_DURATION
	DURATION = -DBENCH_DURATION=$(BENCH_DURATION)
endif

ifndef LUAJIT_PATH
	LUAJIT_PATH = ./LuaJIT
endif

LUAJIT_A = $(LUAJIT_PATH)/src/libluajit.a

all: $(LUAJIT_PATH)/src/libluajit.a
	$(CC) main.c $(DURATION) -O3 -c -o main.o -I $(LUAJIT_PATH)/src
	$(CC) main.o -lpthread ${argp} $^ -lm -ldl -o bench

$(LUAJIT_A): $(LUAJIT_PATH)/Makefile
	make -C $(LUAJIT_PATH) -j

LuaJIT/Makefile:
	git submodule update --init --recursive

clean:
	rm bench main.o

distclean:
	rm -rf LuaJIT
	mkdir LuaJIT
	rm bench main.o

run: bench
	@echo "<benchmark>:<total runs>:<ops/s>"
	@ls *.lua | while read f; do echo -n "$$f:"; ./bench $$f; done
