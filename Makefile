CC=gcc
CFLAGS=-Wall -Wextra -O2
LDFLAGS=-lrt
main: main.o FIFO.o RR.o SJF.o PSJF.o heap.o
