cmake_minimum_required(VERSION 3.0)
project(ClipUpload)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
include_directories(include)
find_package(PkgConfig REQUIRED)

set(CORE_CXX_FLAGS "-static-libstdc++ -static-libgcc -pthread -Wstrict-aliasing -msse -msse2 -Wall -ffast-math")
if (NOT WIN32)
    set(EXTRA_DEBUG_FLAGS "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
else()
    set(EXTRA_DEBUG_FLAGS "")
endif()

pkg_check_modules(MY_PKG REQUIRED IMPORTED_TARGET libnotifymm-1.0 glibmm-2.4)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CORE_CXX_FLAGS} ${EXTRA_DEBUG_FLAGS} -g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CORE_CXX_FLAGS} -g -O2")

add_executable(ClipUpload main.cpp src/Clipboard.cpp include/Clipboard.h src/Notifier.cpp include/Notifier.h src/Keyboard.cpp include/Keyboard.h src/Uploader.cpp include/Uploader.h include/SystemUtil.h)
target_link_libraries(ClipUpload -lX11 -lmbedx509)

# Link frnetlib
FIND_PACKAGE(FRNETLIB)
INCLUDE_DIRECTORIES(${FRNETLIB_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(ClipUpload ${FRNETLIB_LIBRARIES})

# Link mbedtls
FIND_PACKAGE(MBEDTLS)
INCLUDE_DIRECTORIES(${MBEDTLS_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(ClipUpload ${MBEDTLS_LIBRARIES})

#Link pkg stuff
target_link_libraries(ClipUpload PkgConfig::MY_PKG)

