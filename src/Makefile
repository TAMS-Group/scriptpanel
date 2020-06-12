CXXFLAGS = -std=c++11 -I ../imgui -I ../imgui/examples -I../imgui/examples/libs/gl3w -DIMGUI_IMPL_OPENGL_LOADER_GL3W
SOURCES = main.cpp ../imgui/examples/imgui_impl_glfw.cpp ../imgui/examples/imgui_impl_opengl3.cpp
SOURCES += ../imgui/examples/libs/gl3w/GL/gl3w.c ../imgui/imgui.cpp ../imgui/imgui_demo.cpp ../imgui/imgui_draw.cpp ../imgui/imgui_widgets.cpp
LIBS = -lGL `pkg-config --static --libs glfw3`
DEBUG = -ggdb

all:
	g++ -o pr2_scriptpanel $(CXXFLAGS) $(SOURCES) $(LIBS) $(DEBUG)
