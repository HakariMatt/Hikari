CC = clang
CFLAGS = -Xclang -fopenmp -I/opt/homebrew/opt/libomp/include -L/opt/homebrew/opt/libomp/lib -lomp
target = ray

sources = main.c

all:
	$(CC) $(CFLAGS) $(sources) -o $(target)

clean:
	rm $(target)
