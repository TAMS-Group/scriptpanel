cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(pr2_scriptpanel
  src/main.cpp imgui/examples/imgui_impl_glfw.cpp imgui/examples/imgui_impl_opengl3.cpp
  imgui/examples/libs/gl3w/GL/gl3w.c imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp
  )
target_include_directories(pr2_scriptpanel
  PUBLIC imgui
  PUBLIC imgui/examples
  PUBLIC imgui/examples/libs/gl3w
  )
target_link_libraries(pr2_scriptpanel
  OpenGL::GL
  glfw
  yaml-cpp
  ${CMAKE_DL_LIBS}
)
