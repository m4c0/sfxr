cmake_minimum_required(VERSION 3.17..3.19)
project(sfxr
  VERSION 1.0.0
  DESCRIPTION "DrPetter Sound Effect Generator"
  LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(m4c0-stl)

add_executable(sfxr src/main.cpp)
target_compile_definitions(sfxr PRIVATE M4C0)
target_link_libraries(sfxr PRIVATE m4c0::fuji m4c0::vulkan)

if(WIN32)
  target_link_libraries(sfxr PRIVATE m4c0::win::main)
  target_sources(sfxr PRIVATE src/m4c0.win.cpp)
elseif(APPLE)
  target_link_libraries(sfxr PRIVATE m4c0::osx::main)
  target_sources(sfxr PRIVATE src/m4c0.osx.cpp)
endif()
