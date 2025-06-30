cmake_minimum_required(VERSION 3.22)

project(scriptpanel)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Fontconfig REQUIRED)

set(BINFOLDER ${CMAKE_INSTALL_PREFIX}/bin)
configure_file(resources/scriptpanel.desktop.in scriptpanel.desktop)

include_directories(imgui/backends)

add_executable(scriptpanel
  src/main.cpp
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/backends/imgui_impl_glfw.cpp
  imgui/backends/imgui_impl_opengl3.cpp
  )
target_include_directories(scriptpanel
  PUBLIC imgui
  PUBLIC ${glfw3_INCLUDE_DIRS}
  )
target_link_libraries(scriptpanel
  OpenGL::GL
  fontconfig
  glfw
  yaml-cpp
  ${CMAKE_DL_LIBS}
)

install(TARGETS scriptpanel DESTINATION ${BINFOLDER})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/scriptpanel.desktop DESTINATION share/applications)
install(FILES resources/scriptpanel.svg DESTINATION share/icons/hicolor/scalable/apps)
