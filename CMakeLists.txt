project(STORMEXTRACT)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${STORMEXTRACT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${STORMEXTRACT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${STORMEXTRACT_BINARY_DIR}/bin")

set(CMAKE_CXX_FLAGS "-std=c++11")
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

if (NOT EXISTS "${STORMEXTRACT_SOURCE_DIR}/CascLib/CMakeLists.txt")
    message(FATAL_ERROR
"Missing dependency: CascLib
stormex requires the CascLib library.
It is provided as a GIT submodule of this repository.
Did you forgot to execute the following commands?
   git submodule init
   git submodule update")
endif()

if(WIN32)
    set(CASC_BUILD_STATIC_LIB ON CACHE BOOL "Force Static library building to link test app")
    set(CASC_BUILD_SHARED_LIB OFF CACHE BOOL "Compile dynamically linked library")
endif()

add_subdirectory(CascLib)
add_subdirectory(plog)

include_directories("${STORMEXTRACT_SOURCE_DIR}/include/"
                    "${STORMEXTRACT_SOURCE_DIR}/CascLib/src/"
                    "${STORMEXTRACT_SOURCE_DIR}/cxxopts/include/"
                    "${STORMEXTRACT_SOURCE_DIR}/plog/include/"
)

set(SRC_FILES
    src/util.cc
    src/storage.cc
    src/stormex.cc
)

add_executable(stormex ${SRC_FILES})

target_link_libraries(stormex plog)

if(WIN32)
    target_link_libraries(stormex casc_static)
else()
    target_link_libraries(stormex casc)
endif()

# Set the RPATH
if (APPLE)
    set_target_properties(stormex PROPERTIES LINK_FLAGS "-Wl,-rpath,@loader_path/.")
elseif (UNIX)
    set_target_properties(stormex PROPERTIES INSTALL_RPATH ".")
endif()
