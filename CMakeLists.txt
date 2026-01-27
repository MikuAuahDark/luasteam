cmake_minimum_required(VERSION 4.0)

project(luasteam LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


find_package(LuaJIT)
if(LUAJIT_FOUND)
    add_library(lualib INTERFACE IMPORTED)
    set_target_properties(lualib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${LUAJIT_LIBRARY}"
    )
else()
    find_package(Lua 5.1 REQUIRED)
    add_library(lualib INTERFACE IMPORTED)
    set_target_properties(lualib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
    )
endif()

if(NOT EXISTS sdk)
    message(FATAL_ERROR "Make sure to have Steamworks SDK")
endif()

# Determine Steamworks lib
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if(MSVC)
            # Normally we should rely on CMAKE_SYSTEM_PROCESSOR
            # but as of CMake 4.0.3, it sets to AMD64 even when targetting
            # Windows ARM64. This needs to be reevaluated in the future.
            set(_MEGATEMP_CHECKVAR CMAKE_SYSTEM_PROCESSOR)
            if("${CMAKE_GENERATOR}" MATCHES "^Visual Studio")
                set(_MEGATEMP_CHECKVAR CMAKE_VS_PLATFORM_NAME)
            # https://gitlab.kitware.com/cmake/cmake/-/issues/15170
            elseif(DEFINED MSVC_CXX_ARCHITECTURE_ID)
                set(_MEGATEMP_CHECKVAR MSVC_CXX_ARCHITECTURE_ID)
            endif()
            # FIXME: Code below also duplicates MEGA_X64 architecture assignment.
            if(${${_MEGATEMP_CHECKVAR}} MATCHES "^ARM64EC")
                set(STEAMWORKS_LIB win64/steam_api64.lib)
            elseif(NOT ${${_MEGATEMP_CHECKVAR}} MATCHES "^ARM64")
                set(STEAMWORKS_LIB win64/steam_api64.lib)
            endif()
            unset(_MEGATEMP_CHECKVAR)
        else()
            set(STEAMWORKS_LIB win64/steam_api64.lib)
        endif()
    else()
        set(STEAMWORKS_LIB steam_api.lib)
    endif()
elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(STEAMWORKS_LIB osx/libsteam_api.dylib)
elseif(ANDROID AND CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(STEAMWORKS_LIB androidarm64/libsteam_api.so)
elseif(UNIX)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
            set(STEAMWORKS_LIB linuxarm64/libsteam_api.so)
        else()
            set(STEAMWORKS_LIB linux64/libsteam_api.so)
        endif()
    else()
        set(STEAMWORKS_LIB linux32/libsteam_api.so)
    endif()
endif()

if(NOT STEAMWORKS_LIB)
    message(FATAL_ERROR "Steamworks is not supported for current target platform")
endif()
message(STATUS "Using Steamworks lib: ${STEAMWORKS_LIB}")

add_library(luasteam MODULE
    src/apps.cpp
    src/apps.hpp
    src/common.cpp
    src/common.hpp
    src/const.hpp
    src/core.cpp
    src/core.hpp
    src/extra.cpp
    src/extra.hpp
    src/friends.cpp
    src/friends.hpp
    src/gameServer.cpp
    src/gameServer.hpp
    src/input.cpp
    src/input.hpp
    src/inventory.cpp
    src/inventory.hpp
    src/main.cpp
    src/networkingSockets.cpp
    src/networkingSockets.hpp
    src/networkingUtils.cpp
    src/networkingUtils.hpp
    src/UGC.cpp
    src/UGC.hpp
    src/user.cpp
    src/user.hpp
    src/user_stats.cpp
    src/user_stats.hpp
    src/utils.cpp
    src/utils.hpp
)
target_link_libraries(luasteam lualib "${CMAKE_CURRENT_SOURCE_DIR}/sdk/redistributable_bin/${STEAMWORKS_LIB}")
set_target_properties(luasteam PROPERTIES PREFIX "")
if(APPLE)
    # Make sure macOS also has .so extension
    set_target_properties(luasteam PROPERTIES SUFFIX ".so")
endif()

install(TARGETS luasteam DESTINATION .)
