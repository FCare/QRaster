project(QRaster)

cmake_minimum_required(VERSION 2.8)

include (CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

include(FindOpenGL)
if (OPENGL_FOUND)
  #OpenGL lib
  set(QRASTER_LIBRARIES ${QRASTER_LIBRARIES} ${OPENGL_LIBRARIES})

  #Glew
  add_definitions(-D_USEGLEW_)
  add_definitions(-DGLEW_STATIC)
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/gllibs/include/ )
  set(QRASTER_SOURCES ${QRASTER_SOURCES} src/gllibs/glew/glew.c )

  # glXGetProcAddress
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OPENGL_LIBRARIES})
  check_function_exists(glXGetProcAddress GLXGETPROCADDRESS_OK)
  if (GLXGETPROCADDRESS_OK)
    add_definitions(-DHAVE_GLXGETPROCADDRESS=1)
  endif()
endif (OPENGL_FOUND)

if (GLFW_FOUND)
  include_directories( ${GLFW_INCLUDE_DIRS} )
  set(QRASTER_LIBRARIES ${QRASTER_LIBRARIES} ${GLFW_LIBRARIES})
  #set(QRASTER_HEADERS ${QRASTER_HEADERS} platform/glfw/platform.h
endif (GLFW_FOUND)

set(QRASTER_SOURCES ${QRASTER_SOURCES}
  src/main.c
  src/vdp1_compute.c
)

set(QRASTER_HEADERS ${QRASTER_HEADERS}
  src/vdp1_compute.h
  src/vdp1_prog_compute.h
  src/standard_compute.h
)

set(QRASTER_LIBRARIES ${QRASTER_LIBRARIES} m)

add_executable(QRaster ${QRASTER_SOURCES} ${QRASTER_HEADERS} )
target_link_libraries(QRaster ${QRASTER_LIBRARIES})

install(TARGETS QRaster DESTINATION "bin")
