set(CMAKE_TOOLCHAIN_FILE "Kobo.cmake")
cmake_minimum_required(VERSION 3.10)
project(typewriter)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH third-party/install;third-party/install/share)


set(sources
    src/typewriter/eink.cpp
    src/typewriter/term.cpp
    src/typewriter/pty.cpp
    src/typewriter/events.cpp
    src/typewriter/input.cpp
    )

set(entry src/typewriter/main.cpp)


include_directories(src/typewriter third-party/FBInk third-party/libvterm/include
    third-party/libvterm/src third-party/install/include)


#set(deps ${libevdev})


add_executable(typewriter ${entry} ${sources} ${deps})
target_link_libraries(typewriter ${CMAKE_SOURCE_DIR}/third-party/FBInk/Release/libfbink.a 
    ${CMAKE_SOURCE_DIR}/third-party/libvterm/build/libvterm.a 
    ${CMAKE_SOURCE_DIR}/third-party/install/lib/libevdev.a util)
target_link_options(typewriter PRIVATE -static-libgcc -static-libstdc++ -static)


add_executable(etest src/einktest.cpp ${sources} ${deps})
target_link_libraries(etest ${CMAKE_SOURCE_DIR}/third-party/FBInk/Release/libfbink.a 
    ${CMAKE_SOURCE_DIR}/third-party/libvterm/build/libvterm.a 
    ${CMAKE_SOURCE_DIR}/third-party/install/lib/libevdev.a util)
target_link_options(etest PRIVATE -static-libgcc -static-libstdc++ -static)

