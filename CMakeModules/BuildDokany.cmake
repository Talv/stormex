cmake_minimum_required(VERSION 2.8.5)
project(dokanfuse1)
include(GNUInstallDirs)

option(FUSE_PKG_CONFIG "Install a libfuse-compatible pkg-config file (fuse.pc)" ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -mwin32 -Wall")
add_definitions(-D_FILE_OFFSET_BITS=64)
include_directories(
    ${FUSE_DIR}/include
    ${FUSE_DIR}/../sys
)

if(FUSE_PKG_CONFIG)
    # Defining helper-function to deal with setups that manually set an
    # absolute path for CMAKE_INSTALL_(LIB|INCLUDE)DIR...
    # We could also just make all paths absolute, but this way the
    # pkg-config-file is more human-readable and pkg-config may be able to deal
    # with varying prefixes.
    # CMake does not have a ternary operator for generator expressions, so this
    # looks more complicated than it is.
    function ( make_pkg_config_absolute out_path in_path )
        if(IS_ABSOLUTE "${${in_path}}")
            set(${out_path} "${${in_path}}" PARENT_SCOPE)
        else()
            set(${out_path} "\${prefix}/${${in_path}}" PARENT_SCOPE)
        endif()
    endfunction()

    set(pkg_config_file "${CMAKE_CURRENT_BINARY_DIR}/fuse.pc")
    make_pkg_config_absolute(PKG_CONFIG_LIBDIR CMAKE_INSTALL_LIBDIR)
    make_pkg_config_absolute(PKG_CONFIG_INCLUDEDIR CMAKE_INSTALL_INCLUDEDIR)

    CONFIGURE_FILE(
        ${FUSE_DIR}/pkg-config.pc.in
        ${pkg_config_file}
        @ONLY
    )
endif()

file(GLOB sources
    ${FUSE_DIR}/src/*.cpp
    ${FUSE_DIR}/src/*.c
    ${FUSE_DIR}/src/*.rc
)

add_library(dokanfuse1 ${sources})
