cmake_minimum_required(VERSION 3.16)
project(rychip8)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Set this to build the project with debug options.
# This puts the debug symbols from SDl and other vendors into our executable, allowing us to open a debugger
# and set up breakpoints, step through code, and other debugging stuff.
set(CMAKE_BUILD_TYPE "Debug")

# This assumes the SDL source is available in vendor/SDL
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

# Create your game executable target as usual
add_executable(hello MACOSX_BUNDLE src/hello.c)

# Link to the actual SDL3 library.
target_link_libraries(hello PRIVATE SDL3::SDL3)