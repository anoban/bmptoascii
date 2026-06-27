CC = /usr/bin/gcc

DEBUG = -DDEBUG -D_DEBUG -O0 -g3

NODEBUG = -D_NDEBUG -DNDEBUG -O3 -g0

CCFLAGS = -Wall -Wextra -I./include -std=c23 -march=tigerlake -mavx512f -ffast-math -mprefer-vector-width=512

INCLUDE = -I./include/

build:
	$(CC) $(INCLUDE) ./src/main.c $(CFLAGS) $(NODEBUG) -o bmpasc.out

test:
	$(CC) $(INCLUDE) ./src/test.c $(CFLAGS) -D__TEST__ $(NODEBUG) -o test.out

clean:
	rm -f ./*.out
	rm -f ./*.o
