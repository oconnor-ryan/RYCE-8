cmake_minimum_required(VERSION 3.16)
project(ryce8)

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

# This version creates a .app file for MACOS rather than a Unix Executable.
#add_executable(ryce8 MACOS_BUNDLE src/main.c src/chip8.c src/chip8_sdl_connector.c src/chip8_core.c src/schip8.c src/vip_chip8.c src/util.c)


add_executable(ryce8 src/main.c src/chip8.c src/chip8_sdl_connector.c src/chip8_core.c src/schip8.c src/vip_chip8.c src/util.c)

#note that this is required for MacOS Cocoa apps. 
# The file contains properties that allow the app to open files on the user's computer.
set_target_properties(ryce8 PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/ryce8.entitlements")

# Link to the actual SDL3 library.
target_link_libraries(ryce8 PRIVATE SDL3::SDL3)