$(info Ugly Invaders Space Game - Compilation Makefile)

CC = gcc
LD = gcc

BUILD_DIR = build
CFLAGS = -g -std=c17 -I. -Isrc -I../V7Core -I../V7Vulkan2 -I$(BUILD_DIR)
LDFLAGS	= -lm -L../V7Core -L../V7Vulkan2 -ljpeg -lavif

OBJS = $(BUILD_DIR)/UglyInvaders.o

SHADER_SRCS = $(wildcard shaders/*.glsl)
SHADER_HEADER = uglyshaders.h

ifeq ($(OS),Windows_NT)
	UNAME := Windows
else
	UNAME := $(shell uname)
endif

ifeq ($(UNAME), Linux)
	CFLAGS += -DV7V_PLATFORM_GLFW
	LDFLAGS += -lvulkan -lglfw
else ifeq ($(UNAME), Darwin)
	VULKAN_SDK := $(foreach file, $(wildcard $(HOME)/VulkanSDK/*),$(file))
	CFLAGS += -DV7V_PLATFORM_GLFW
	CFLAGS += -I/opt/homebrew/include -I$(VULKAN_SDK)/macOS/include
	LDFLAGS += -lvulkan -L/opt/homebrew/lib -lMoltenVK -lglfw -L$(VULKAN_SDK)/macOS/lib -Wl,-rpath,/usr/local/lib -Wl,-rpath,/opt/homebrew/lib -Wl,-rpath,/usr/lib -framework Cocoa -framework QuartzCore
else ifeq ($(UNAME), Windows)
	CFLAGS += -V7V_PLATFORM_WIN32 -I$(VULKAN_SDK)/Include
	LDFLAGS	+= -lvulkan-1 -L$(VULKAN_SDK)/Lib
endif

all: $(SHADER_HEADER) UglyInvaders

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generate shader header
$(SHADER_HEADER): $(SHADER_SRCS) | $(BUILD_DIR)
	@echo "Compiling shaders to SPIR-V..."
	@for shader in $(sort $(SHADER_SRCS)); do name=$$(basename $$shader .glsl); \
		if echo $$name | grep -q "_vert$$"; then stage=vertex; \
		elif echo $$name | grep -q "_frag$$"; then stage=fragment; \
		else continue; \
		fi; \
		glslc -fshader-stage=$$stage $$shader -o $(BUILD_DIR)/$$name.spv; \
	done
	@echo "#ifndef SHADERS_H" > $@
	@echo "#define SHADERS_H" >> $@
	@echo "" >> $@
	@for shader in $(sort $(SHADER_SRCS)); do name=$$(basename $$shader .glsl); \
		xxd -i $(BUILD_DIR)/$$name.spv | sed "s/build_$${name}_spv/$$name/" >> $@; \
		echo "" >> $@; \
	done
	@echo "#endif // SHADERS_H" >> $@
	@echo "Generated $@"

$(BUILD_DIR)/%.o: %.c $(SHADER_HEADER) | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

UglyInvaders: $(OBJS)
	$(LD) -lV7Core -lV7V $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf build UglyInvaders uglyshaders.h
