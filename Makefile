# gcc всякае там типа флаги сурсы тота сёта
cxx = g++
cxxflags = -Wall -std=c++17 -I./imgui -I./imgui/backends -I./glad
flags = -lglfw -lGL -lm -ldl -lXi -lpthread -lX11 -lXrandr
imgui_dir = ./imgui
glad_dir = ./glad
sources = main.cpp \
	  $(imgui_dir)/imgui.cpp \
	  $(imgui_dir)/imgui_draw.cpp \
	  $(imgui_dir)/imgui_widgets.cpp \
          $(imgui_dir)/imgui_tables.cpp \
          $(imgui_dir)/imgui_demo.cpp \
          $(imgui_dir)/backends/imgui_impl_glfw.cpp \
          $(imgui_dir)/backends/imgui_impl_opengl3.cpp \
	  $(glad_dir)/glad.c

# ну название бинарника ну просто бос
target = test_file

all:
	$(cxx) $(cxxflags) $(sources) -o $(target) $(flags)
