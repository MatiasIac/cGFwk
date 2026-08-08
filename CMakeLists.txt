cmake_minimum_required(VERSION 3.21)
project(Retro2D VERSION 0.1.0 LANGUAGES C)

option(RETRO2D_BUILD_EXAMPLES "Build the example games" ON)
option(RETRO2D_BUILD_TESTS "Build platform-independent tests" ON)
option(RETRO2D_FETCH_SDL3 "Download SDL3 when no package is installed" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 3.2 CONFIG QUIET)
if(NOT SDL3_FOUND)
    if(NOT RETRO2D_FETCH_SDL3)
        message(FATAL_ERROR "SDL3 was not found. Set SDL3_DIR or enable RETRO2D_FETCH_SDL3.")
    endif()
    include(FetchContent)
    set(SDL_SHARED ON CACHE BOOL "Build SDL3 shared" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "Do not build SDL3 static" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Do not build SDL test library" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "Do not build SDL tests" FORCE)
    FetchContent_Declare(SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.tar.gz
        URL_HASH SHA256=12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(SDL3)
endif()

add_library(retro2d STATIC
    src/animation.c
    src/audio.c
    src/collision.c
    src/core.c
    src/draw.c
    src/effects.c
    src/error.c
    src/font.c
    src/image.c
    src/input.c
    src/platform_sdl.c
    src/resources.c
    src/screen.c
    src/state.c)
target_include_directories(retro2d PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(retro2d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(retro2d PUBLIC SDL3::SDL3)
if(NOT MSVC)
    target_link_libraries(retro2d PRIVATE m)
endif()
if(MSVC)
    target_compile_options(retro2d PRIVATE /W4 /permissive-)
    target_compile_definitions(retro2d PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(retro2d PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

function(copy_sdl_dll target)
    if(WIN32 AND TARGET SDL3::SDL3-shared)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${target}>)
    endif()
endfunction()

if(RETRO2D_BUILD_EXAMPLES)
    add_executable(retro2d_make_assets tools/make_assets.c)
    if(MSVC)
        target_compile_options(retro2d_make_assets PRIVATE /W4)
        target_compile_definitions(retro2d_make_assets PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(retro2d_make_assets PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    foreach(example IN ITEMS sprite_demo point_click effects_demo fullscreen_pong)
        add_executable(${example} examples/${example}.c)
        target_link_libraries(${example} PRIVATE retro2d)
        if(MSVC)
            target_compile_options(${example} PRIVATE /W4)
        else()
            target_compile_options(${example} PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
        endif()
        add_dependencies(${example} retro2d_make_assets)
        add_custom_command(TARGET ${example} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${example}>/assets
            COMMAND $<TARGET_FILE:retro2d_make_assets> $<TARGET_FILE_DIR:${example}>/assets)
        copy_sdl_dll(${example})
    endforeach()
endif()

if(RETRO2D_BUILD_TESTS)
    enable_testing()
    add_executable(retro2d_tests tests/test_retro2d.c tests/platform_stub.c)
    target_include_directories(retro2d_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(retro2d_tests PRIVATE retro2d)
    if(MSVC)
        target_compile_options(retro2d_tests PRIVATE /W4)
        target_compile_definitions(retro2d_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(retro2d_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    endif()
    add_test(NAME retro2d_logic COMMAND retro2d_tests)
    copy_sdl_dll(retro2d_tests)
endif()
