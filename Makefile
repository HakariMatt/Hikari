CC = clang
CFLAGS = -Xclang -fopenmp -I/opt/homebrew/opt/libomp/include -L/opt/homebrew/opt/libomp/lib -lomp \
         -I/opt/homebrew/opt/raylib/include -L/opt/homebrew/opt/raylib/lib -lraylib \
         -framework Cocoa -framework OpenGL -framework IOKit -Wall -Wextra
target = hikari

sources = $(wildcard src/*.c)

all:
	$(CC) $(CFLAGS) $(sources) -o $(target)

clean:
	rm $(target)
