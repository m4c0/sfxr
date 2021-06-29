cmake_minimum_required(VERSION 3.17..3.19)
project(sfxr
  VERSION 1.0.0
  DESCRIPTION "DrPetter Sound Effect Generator"
  LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(m4c0-stl)

add_executable(sfxr WIN32 MACOSX_BUNDLE
  font.tga
  ld48.tga
  src/m4c0.cpp
  src/m4c0.ddk.cpp
  src/main.cpp
  src/sprite_set.cpp
  )
target_compile_definitions(sfxr PRIVATE M4C0)
target_link_libraries(sfxr PRIVATE
  m4c0::assets
  m4c0::audio
  m4c0::casein
  m4c0::fuji
  m4c0::vulkan)
set_target_properties(sfxr PROPERTIES
  MACOSX_BUNDLE_BUNDLE_NAME "SFXR"
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.m4c0.sfxr"
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_PATCH}
  RESOURCE "ld48.tga;font.tga"
  )

if(WIN32)
  target_link_libraries(sfxr PRIVATE m4c0::win::main)
  target_sources(sfxr PRIVATE src/m4c0.win.cpp src/m4c0.win.rc)
elseif(APPLE)
  target_link_libraries(sfxr PRIVATE m4c0::osx::main)
  target_sources(sfxr PRIVATE src/m4c0.osx.cpp)
endif()
