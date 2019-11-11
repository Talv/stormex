project(STORMEX)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${STORMEX_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${STORMEX_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${STORMEX_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
    STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# options
option(ENABLE_FUSE "Enable FUSE" ON)

# compile flags
set(CMAKE_CXX_FLAGS "-std=c++11")
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
# link statically with mingw
if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/CMakeModules")

if (NOT EXISTS "${STORMEX_SOURCE_DIR}/CascLib/CMakeLists.txt")
    message(FATAL_ERROR "Missing dependency: CascLib")
endif()

set(CASC_BUILD_STATIC_LIB ON CACHE BOOL "Force Static library building to link test app")
set(CASC_BUILD_SHARED_LIB OFF CACHE BOOL "Compile dynamically linked library")

add_subdirectory(CascLib)
add_subdirectory(plog)

# stormex
set(SRC_FILES
    src/util.cc
    src/storage.cc
    src/cascfuse.cc
    src/stormex.cc
)
add_executable(stormex ${SRC_FILES})

# FUSE
if (ENABLE_FUSE)
    set(FUSE_REQUIRED_FLAGS "-D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUSE_REQUIRED_FLAGS}")

    if (WIN32 AND FUSE_DIR)
        set(FUSE_INCLUDE_DIR ${FUSE_DIR}/include)
        set(FUSE_LIBRARIES ${FUSE_DIR})
        include("CMakeModules/BuildDokany.cmake")
        set(FUSE_NAMES dokanfuse1)
    else()
        find_package(FUSE REQUIRED)
    endif()

    include_directories(${FUSE_INCLUDE_DIR})
    target_link_libraries(stormex ${FUSE_NAMES})
endif()

include_directories("${STORMEX_SOURCE_DIR}/include/"
                    "${STORMEX_SOURCE_DIR}/CascLib/src/"
                    "${STORMEX_SOURCE_DIR}/cxxopts/include/"
                    "${STORMEX_SOURCE_DIR}/plog/include/"
)

target_link_libraries(stormex casc_static)

# Set the RPATH
if (APPLE)
    set_target_properties(stormex PROPERTIES LINK_FLAGS "-Wl,-rpath,@loader_path/.")
elseif (UNIX)
    set_target_properties(stormex PROPERTIES INSTALL_RPATH ".")
endif()
