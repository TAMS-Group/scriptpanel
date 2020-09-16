cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(scriptpanel
  src/main.cpp imgui/examples/imgui_impl_glfw.cpp imgui/examples/imgui_impl_opengl3.cpp
  imgui/examples/libs/gl3w/GL/gl3w.c imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp
  )
target_include_directories(scriptpanel
  PUBLIC imgui
  PUBLIC imgui/examples
  PUBLIC imgui/examples/libs/gl3w
  )
target_link_libraries(scriptpanel
  OpenGL::GL
  glfw
  yaml-cpp
  ${CMAKE_DL_LIBS}
)
