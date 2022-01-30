CC = gcc
CFLAGS=-O3

WDIR=../

all: basic_test

basic_test: basic_test.c
	$(CC) $(CFLAGS) -I$(WDIR) -L$(WDIR) -Wl,-rpath=$(WDIR) -o $@ $< -lmymalloc -lrt -lpthread
