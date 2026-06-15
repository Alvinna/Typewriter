set(CMAKE_TOOLCHAIN_FILE "Kobo.cmake")
cmake_minimum_required(VERSION 3.10)
project(typewriter)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 11)


set(sources
    src/main.cpp
    )

set(entry src/main.cpp)


include_directories(src third-party/FBInk third-party/libvterm/include)

set(deps )


add_executable(typewriter ${entry} ${deps})
target_link_libraries(typewriter ${CMAKE_SOURCE_DIR}/third-party/FBInk/Release/libfbink.a 
    ${CMAKE_SOURCE_DIR}/third-party/libvterm/build/libvterm.a)
target_link_options(typewriter PRIVATE -static-libstdc++)

