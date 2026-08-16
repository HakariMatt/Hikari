CC = clang
CFLAGS = -Xclang -fopenmp -I/opt/homebrew/opt/libomp/include -L/opt/homebrew/opt/libomp/lib -lomp \
         -I/opt/homebrew/opt/raylib/include -L/opt/homebrew/opt/raylib/lib -lraylib \
         -framework Cocoa -framework OpenGL -framework IOKit -Wall -Wextra
target = hikari
target_mtl = hikarimtl

sources = $(wildcard src/*.c)
shaders_dir = assets/shaders
obj_c_dir = obj-c
metal_shader = assets/shaders/shader.metal

all:
	$(CC) $(CFLAGS) $(sources) -o $(target)

shader.metallib: $(shaders_dir)/shader.metal
	xcrun -sdk macosx metal -c $(shaders_dir)/shader.metal -o $(shaders_dir)/shader.air
	xcrun -sdk macosx metallib $(shaders_dir)/shader.air -o $(shaders_dir)/shader.metallib
	rm $(shaders_dir)/shader.air

metal_backend.o: $(obj_c_dir)/metal_backend.m include/metal_backend.h
	clang -fobjc-arc -x objective-c -c $(obj_c_dir)/metal_backend.m -o metal_backend.o

test_metal: src/main_mtl.c metal_backend.o shader.metallib
	clang src/main_mtl.c src/camera.c metal_backend.o -o $(target_mtl) \
		-framework Metal -framework Foundation -framework QuartzCore -lobjc
	rm metal_backend.o

metal: test_metal shader.metallib

clean:
	rm $(target)
