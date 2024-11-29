# Install script for directory: D:/proj/IBLBaker/src/critter/dependencies/FreeImage/Source

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/IBLBaker")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/OpenEXR/cmake_install.cmake")
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/LibWebP/cmake_install.cmake")
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/LibTIFF4/cmake_install.cmake")
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/LibRawLite/cmake_install.cmake")
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/LibPNG/cmake_install.cmake")
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/LibOpenJPEG/cmake_install.cmake")
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/LibJXR/cmake_install.cmake")
  include("D:/proj/IBLBaker/Build64/ext_build/FreeImage/LibJPEG/cmake_install.cmake")

endif()

