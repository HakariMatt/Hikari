CC = clang

COMMON_CFLAGS = -Wall -Wextra -Iinclude -MMD -MP

CPU_CFLAGS = -Xclang -fopenmp -I/opt/homebrew/opt/libomp/include -L/opt/homebrew/opt/libomp/lib -lomp

RAYLIB_FLAGS = -I/opt/homebrew/opt/raylib/include -L/opt/homebrew/opt/raylib/lib -lraylib \
				-framework Cocoa -framework OpenGL -framework IOKit

target_cpu = hikari
target_mtl = hikarimtl

shaders_dir = assets/shaders
obj_c_dir   = obj-c
metal_shader = $(shaders_dir)/shader.metal
metal_lib    = $(shaders_dir)/shader.metallib

obj_cpu_dir = obj/cpu
obj_mtl_dir = obj/mtl

cpu_sources = $(wildcard src/*.c)
mtl_sources = $(wildcard src/*.c)

cpu_objects = $(patsubst src/%.c, $(obj_cpu_dir)/%.o, $(cpu_sources))
mtl_objects = $(patsubst src/%.c, $(obj_mtl_dir)/%.o, $(mtl_sources))

.PHONY: all cpu metal clean

all: cpu


cpu: $(target_cpu)

$(target_cpu): $(cpu_objects)
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) $(RAYLIB_FLAGS) $(cpu_objects) -o $@

$(obj_cpu_dir)/%.o: src/%.c | $(obj_cpu_dir)
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) -c $< -o $@

$(obj_cpu_dir):
	mkdir -p $@


metal: $(target_mtl) $(metal_lib)

$(target_mtl): $(mtl_objects) $(obj_mtl_dir)/metal_backend.o
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) $(RAYLIB_FLAGS) $(mtl_objects) $(obj_mtl_dir)/metal_backend.o -o $@ \
		-framework Metal -framework Foundation -framework QuartzCore -lobjc

$(obj_mtl_dir)/%.o: src/%.c | $(obj_mtl_dir)
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) -DHIKARI_METAL -c $< -o $@

$(obj_mtl_dir)/metal_backend.o: $(obj_c_dir)/metal_backend.m include/metal_backend.h | $(obj_mtl_dir)
	$(CC) -fobjc-arc -x objective-c $(COMMON_CFLAGS) -DHIKARI_METAL -c $(obj_c_dir)/metal_backend.m -o $@

$(obj_mtl_dir):
	mkdir -p $@

$(metal_lib): $(metal_shader)
	xcrun -sdk macosx metal -c $(metal_shader) -o $(shaders_dir)/shader.air
	xcrun -sdk macosx metallib $(shaders_dir)/shader.air -o $(metal_lib)
	rm $(shaders_dir)/shader.air

clean:
	rm -rf obj $(target_cpu) $(target_mtl) $(shaders_dir)/shader.air $(metal_lib)

-include $(cpu_objects:.o=.d)
-include $(mtl_objects:.o=.d)
