CC = clang

COMMON_CFLAGS = -Wall -Wextra -Iinclude -Irgb2spec -MMD -MP
LIB_CFLAGS    = -fPIC

CPU_CFLAGS = -Xclang -fopenmp -I/opt/homebrew/opt/libomp/include -L/opt/homebrew/opt/libomp/lib -lomp

lib_basename = hikari
lib_cpu      = $(lib_basename).dylib
lib_mtl      = $(lib_basename)_metal.dylib

demo_target = hikari_demo

shaders_dir  = assets/shaders
obj_c_dir    = obj-c
metal_shader = $(shaders_dir)/shader.metal
metal_lib    = $(shaders_dir)/shader.metallib

obj_lib_cpu_dir = obj/hikari
obj_lib_mtl_dir = obj/hikarimtl

rgb2spec_dir     = rgb2spec
rgb2spec_sources = $(wildcard $(rgb2spec_dir)/*.c)

lib_sources = $(filter-out src/display.c,$(wildcard src/*.c))

lib_cpu_objects = $(patsubst src/%.c,$(obj_lib_cpu_dir)/%.o,$(lib_sources)) \
                  $(patsubst $(rgb2spec_dir)/%.c,$(obj_lib_cpu_dir)/%.o,$(rgb2spec_sources))
lib_mtl_objects = $(patsubst src/%.c,$(obj_lib_mtl_dir)/%.o,$(lib_sources)) \
                  $(patsubst $(rgb2spec_dir)/%.c,$(obj_lib_mtl_dir)/%.o,$(rgb2spec_sources))

.PHONY: all lib lib-metal demo clean

all: lib demo


lib: $(lib_cpu)

$(lib_cpu): $(lib_cpu_objects)
	$(CC) -dynamiclib $(COMMON_CFLAGS) $(CPU_CFLAGS) $(LIB_CFLAGS) \
		$(lib_cpu_objects) \
		-install_name @rpath/$(lib_cpu) \
		-o $@

$(obj_lib_cpu_dir)/%.o: src/%.c | $(obj_lib_cpu_dir)
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) $(LIB_CFLAGS) -c $< -o $@

$(obj_lib_cpu_dir)/%.o: $(rgb2spec_dir)/%.c | $(obj_lib_cpu_dir)
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) $(LIB_CFLAGS) -c $< -o $@

$(obj_lib_cpu_dir):
	mkdir -p $@


lib-metal: $(lib_mtl) $(metal_lib)

$(lib_mtl): $(lib_mtl_objects) $(obj_lib_mtl_dir)/metal_backend.o
	$(CC) -dynamiclib $(COMMON_CFLAGS) $(CPU_CFLAGS) $(LIB_CFLAGS) \
		$(lib_mtl_objects) $(obj_lib_mtl_dir)/metal_backend.o \
		-install_name @rpath/$(lib_mtl) \
		-o $@ \
		-framework Metal -framework Foundation -framework QuartzCore -lobjc

$(obj_lib_mtl_dir)/%.o: src/%.c | $(obj_lib_mtl_dir)
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) $(LIB_CFLAGS) -DHIKARI_METAL -c $< -o $@

$(obj_lib_mtl_dir)/%.o: $(rgb2spec_dir)/%.c | $(obj_lib_mtl_dir)
	$(CC) $(COMMON_CFLAGS) $(CPU_CFLAGS) $(LIB_CFLAGS) -DHIKARI_METAL -c $< -o $@

$(obj_lib_mtl_dir)/metal_backend.o: $(obj_c_dir)/metal_backend.m include/metal_backend.h | $(obj_lib_mtl_dir)
	$(CC) -fobjc-arc -fPIC -x objective-c $(COMMON_CFLAGS) -DHIKARI_METAL -c $(obj_c_dir)/metal_backend.m -o $@

$(obj_lib_mtl_dir):
	mkdir -p $@

$(metal_lib): $(metal_shader)
	xcrun -sdk macosx metal -c $(metal_shader) -o $(shaders_dir)/shader.air
	xcrun -sdk macosx metallib $(shaders_dir)/shader.air -o $(metal_lib)
	rm $(shaders_dir)/shader.air


demo: $(demo_target)

$(demo_target): main.c $(lib_cpu)
	$(CC) $(COMMON_CFLAGS) main.c \
		-L. -l$(lib_basename) \
		-Wl,-rpath,@executable_path \
		-o $@

clean:
	rm -rf obj $(lib_cpu) $(lib_mtl) $(demo_target) $(shaders_dir)/shader.air $(metal_lib) hikari_demo.d

-include $(lib_cpu_objects:.o=.d)
-include $(lib_mtl_objects:.o=.d)
