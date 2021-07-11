cmake_minimum_required(VERSION 3.10)

project(scriptpanel)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

# half-baked fallbacks for cmake in Ubuntu < 20.04
macro(target_from_library target lib)
	find_library( _${lib}_LIBRARY
		NAMES ${lib}
	)
	add_library(${target} UNKNOWN IMPORTED)
	set_target_properties(${target} PROPERTIES
		IMPORTED_LOCATION ${_${lib}_LIBRARY})
endmacro()

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
if(NOT TARGET OpenGL::GL)
	target_from_library(OpenGL::GL GL)
endif()
find_package(yaml-cpp REQUIRED)
find_package(Fontconfig QUIET)
if(NOT TARGET Fontconfig::Fontconfig)
	target_from_library(Fontconfig::Fontconfig fontconfig)
endif()

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
  Fontconfig::Fontconfig
  glfw
  yaml-cpp
  ${CMAKE_DL_LIBS}
)

install(TARGETS scriptpanel DESTINATION bin)
