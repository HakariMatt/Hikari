CC = clang

CFLAGS = -Wall -Wextra -Iinclude -Irgb2spec -fPIC -DHIKARI_METAL -MMD -MP

lib          = libhikari.dylib
metal_lib    = assets/shaders/shader.metallib
metal_shader = assets/shaders/shader.metal
obj_dir      = obj

demo = hikari_demo

sources = $(filter-out src/display.c,$(wildcard src/*.c)) $(wildcard rgb2spec/*.c)
objects = $(patsubst %.c,$(obj_dir)/%.o,$(notdir $(sources)))
objects += $(obj_dir)/metal_backend.o

vpath %.c src rgb2spec

.PHONY: all clean

all: $(lib) $(metal_lib)

$(lib): $(objects)
	$(CC) -dynamiclib $(CFLAGS) $(objects) \
		-install_name @rpath/$(lib) \
		-framework Metal -framework Foundation -framework QuartzCore -lobjc \
		-o $@

$(obj_dir)/%.o: %.c | $(obj_dir)
	$(CC) $(CFLAGS) -c $< -o $@

$(obj_dir)/metal_backend.o: obj-c/metal_backend.m include/metal_backend.h | $(obj_dir)
	$(CC) -fobjc-arc -x objective-c $(CFLAGS) -c obj-c/metal_backend.m -o $@

$(obj_dir):
	mkdir -p $@

$(metal_lib): $(metal_shader)
	xcrun -sdk macosx metal -c $(metal_shader) -o assets/shaders/shader.air
	xcrun -sdk macosx metallib assets/shaders/shader.air -o $(metal_lib)
	rm assets/shaders/shader.air

demo: $(lib) $(metal_lib)
	$(CC) -L./ -lhikari -Wl,-rpath,@executable_path main.c -o hikari_demo

clean:
	rm -rf $(obj_dir) $(lib) $(metal_lib) hikari_demo

-include $(objects:.o=.d)
