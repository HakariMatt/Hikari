CC = gcc
CFLAGS =
target = ray

sources = main.c

all:
	$(CC) $(CFLAGS) $(sources) -o $(target)

clean:
	rm $(target)
