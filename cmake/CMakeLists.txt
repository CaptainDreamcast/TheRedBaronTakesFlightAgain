cmake_minimum_required(VERSION 3.25) 
project(LD60 CXX)               
set(CMAKE_CXX_STANDARD 17)            
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include("${CMAKE_SOURCE_DIR}/../../addons/prism/cmake/prism.cmake" REQUIRED)

# Add source/header files to project
file(GLOB_RECURSE SOURCES ../*.cpp SOURCES ../*.h SOURCES ../*.rc)
list(FILTER SOURCES EXCLUDE REGEX ".*web.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")

add_link_options(/NODEFAULTLIB:libcmt.lib)
add_link_options(/IGNORE:4099,4286,4098)
add_link_options(/OPT:REF)
add_link_options(/OPT:ICF)

# Define exe
add_executable(LD60 ${SOURCES})

# Link with libraries
target_link_libraries(LD60 prism)
target_link_libraries(LD60 SDL2)
target_link_libraries(LD60 SDL2main)
target_link_libraries(LD60 SDL2_image)
target_link_libraries(LD60 fmod)
target_link_libraries(LD60 SDL2_ttf)
target_link_libraries(LD60 enet)
target_link_libraries(LD60 freetype)
target_link_libraries(LD60 glew)
target_link_libraries(LD60 curl)
target_link_libraries(LD60 libjpeg)
target_link_libraries(LD60 libpng)
target_link_libraries(LD60 libwebp)
target_link_libraries(LD60 zstd)
target_link_libraries(LD60 zlib)
target_link_libraries(LD60 crypt32)
target_link_libraries(LD60 wldap32)
target_link_libraries(LD60 ws2_32)
target_link_libraries(LD60 winmm)
target_link_libraries(LD60 OpenGL32)
target_link_libraries(LD60 version)

# Define preprocessor definitions
target_compile_definitions(LD60 PUBLIC UNICODE)
target_compile_definitions(LD60 PUBLIC _UNICODE)

set_property(TARGET LD60 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(LD60 PRIVATE /Gy)

set_target_properties(LD60 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../assets)
set_target_properties(LD60 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../assets)

# Copy over DLLs
file(GLOB DLLS "${CMAKE_SOURCE_DIR}/../../addons/prism/windows/vs17/DLL/*.dll")
foreach(DLL ${DLLS})
  add_custom_command(TARGET LD60 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${DLL}
    $<TARGET_FILE_DIR:LD60>)
endforeach()

# Optimize for release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(LD60 PRIVATE /O2)
endif()
