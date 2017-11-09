CC = gcc -ggdb -std=c99 -Wall -Werror -O3
OUT = sighandler

stacktrace.o:
	$(CC) -c stacktrace.c

$(OUT).o:
	$(CC) -c $(OUT).c

all: clean $(OUT).o stacktrace.o
	$(CC) -o $(OUT) $(OUT).o stacktrace.o

clean:
	rm -f *.o
	rm -f $(OUT)

install: clean all

run: install
	./$(OUT)

rerun: all
	./$(OUT)

valgrind: clean all
	valgrind -v --leak-check=full ./$(OUT)