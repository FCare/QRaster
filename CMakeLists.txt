project(QRaster)

cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

include(FindOpenGL)
if (OPENGL_FOUND)
  set(QRASTER_LIBRARIES ${QRASTER_LIBRARIES} ${OPENGL_LIBRARIES})
  add_definitions(-D_USEGLEW_)
  add_definitions(-DGLEW_STATIC)
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/gllibs/include/ )
  set(QRASTER_SOURCES ${QRASTER_SOURCES} src/gllibs/glew/glew.c )
endif (OPENGL_FOUND)

if (GLFW_FOUND)
  include_directories( ${GLFW_INCLUDE_DIRS} )
  set(QRASTER_LIBRARIES ${QRASTER_LIBRARIES} ${GLFW_LIBRARIES})
  #set(QRASTER_HEADERS ${QRASTER_HEADERS} platform/glfw/platform.h
endif (GLFW_FOUND)

add_executable(QRaster ${QRASTER_SOURCES} ${QRASTER_HEADERS} src/main.c )

install(TARGETS QRaster DESTINATION "bin")
