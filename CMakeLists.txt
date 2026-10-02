# =============================================================================
#  YASAG  -  Yet Another Space Alien Game  -  V2.00A     (by Thomas Dzubin)
#  ---------------------------------------------------------------------------
#  V2.xx is the HARDWARE-INDEPENDENT line: the same gameplay as
#  picocalc-space-game (V1.xx, PicoCalc only), but every hardware operation is
#  behind the HAL, so ONE C codebase builds for several machines - pick one
#  with -DYASAG_TARGET:
#
#      -DYASAG_TARGET=picocalc      ClockworkPi PicoCalc  (RP2040 / RP2350)   [Pico SDK]
#      -DYASAG_TARGET=picosystem    Pimoroni PicoSystem   (RP2040)            [Pico SDK]   (Stage 4)
#      -DYASAG_TARGET=sdl           Windows / Linux, static SDL2                           (Stage 2)
#
#  Screen size is a compile-time constant; override per build:
#      -DYASAG_SCREEN_W=320 -DYASAG_SCREEN_H=320
#  (blank = the target's own default: PicoCalc 320x320, PicoSystem 240x240,
#   SDL 640x480.)  The SDL build also takes -DYASAG_SDL_ZOOM=N (default 2) to
#   show the render in an NxN-times-larger window.
#
#  The build + compile is done on a separate machine.  For the MCU targets set
#  PICO_SDK_PATH (or use the VS Code Pico extension) and pick the board with
#  -DPICO_BOARD=pico | pico2 | pimoroni_picosystem.  The SDL target needs SDL2
#  (static lib preferred) findable by find_package(SDL2).
# =============================================================================
cmake_minimum_required(VERSION 3.13)

set(YASAG_TARGET   "picocalc" CACHE STRING "picocalc | picosystem | sdl")
set(YASAG_SCREEN_W ""         CACHE STRING "screen width  px (blank = target default)")
set(YASAG_SCREEN_H ""         CACHE STRING "screen height px (blank = target default)")

# --- portable core: identical on every target -------------------------------
set(YASAG_CORE
    game/game.c
    game/text.c
    game/font_8x10.c
)
set(YASAG_CORE_HEADERS          # listed for IDE visibility only
    game/game.h
    game/text.h
    game/font.h
    game/screenshot.h
    hal/hal.h
)

# ===========================================================================
if(YASAG_TARGET STREQUAL "picocalc")
# ===========================================================================
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # -- Raspberry Pi Pico VS Code extension bootstrap (do not edit) ----------
    if(WIN32)
        set(USERHOME $ENV{USERPROFILE})
    else()
        set(USERHOME $ENV{HOME})
    endif()
    set(sdkVersion 2.2.0)
    set(toolchainVersion 14_2_Rel1)
    set(picotoolVersion 2.2.0)
    set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
    if (EXISTS ${picoVscode})
        include(${picoVscode})
    endif()
    # -----------------------------------------------------------------------
    # set(PICO_BOARD pico CACHE STRING "Board type")   # choose on the build machine

    include(pico_sdk_import.cmake)
    project(yasag C CXX ASM)
    pico_sdk_init()

    set(STARTER ${CMAKE_CURRENT_LIST_DIR}/third_party/picocalc-text-starter-main)

    add_executable(yasag
        ${YASAG_CORE}
        ${YASAG_CORE_HEADERS}
        hal/hal_picocalc.c
        hal/screenshot_picocalc.c
        # -- vendored picocalc-text-starter drivers (Blair Leduc), PicoCalc only --
        ${STARTER}/drivers/lcd.c
        ${STARTER}/drivers/southbridge.c
        ${STARTER}/drivers/audio.c
        ${STARTER}/drivers/sdcard.c        # SD + FAT32 - only the screenshot needs
        ${STARTER}/drivers/fat32.c         #   these; gc-sections drops them otherwise
        GAME-DETAILS.txt                   # design notes - not compiled, IDE only
        CHANGELOG.md                       #   "
    )

    pico_set_program_name(yasag "yasag")
    pico_set_program_version(yasag "V2.00A")   # keep in sync with VERSION_STR in game/game.h

    pico_generate_pio_header(yasag ${STARTER}/drivers/audio.pio)

    pico_enable_stdio_uart(yasag 0)        # keep USB/UART free so '~' -> bootloader is clean
    pico_enable_stdio_usb(yasag 0)

    target_compile_definitions(yasag PRIVATE YASAG_HAS_SCREENSHOT)

    target_include_directories(yasag PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/game
        ${CMAKE_CURRENT_LIST_DIR}/hal
        ${STARTER}
        ${STARTER}/drivers
    )
    target_compile_options(yasag PRIVATE -Wall)

    target_link_libraries(yasag
        pico_stdlib
        pico_multicore        # drivers/lcd.c, drivers/southbridge.c
        pico_sync             # drivers/fat32.c  (pico/sem.h)
        pico_float
        hardware_gpio
        hardware_i2c
        hardware_spi
        hardware_pio
        hardware_clocks
    )
# The chip is part of every output file name (yasag-RP2040.uf2 /
# yasag-RP2350.uf2, and the matching .elf/.bin/.hex), so a downloaded file
# always says which board it was built for.
if(PICO_PLATFORM MATCHES "^rp2040")
    set(CHIP_NAME RP2040)
elseif(PICO_PLATFORM MATCHES "^rp2350")
    set(CHIP_NAME RP2350)
else()
    set(CHIP_NAME ${PICO_PLATFORM})
endif()
set_target_properties(yasag PROPERTIES OUTPUT_NAME yasag-${CHIP_NAME})

    pico_add_extra_outputs(yasag)

# Copy the finished .uf2 (same name) to the top-level project folder, so the
# RP2040 and RP2350 builds can sit side by side.
add_custom_command(TARGET yasag POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:yasag>/$<TARGET_FILE_BASE_NAME:yasag>.uf2
            ${CMAKE_CURRENT_LIST_DIR}/
)


    if(YASAG_SCREEN_W)
        target_compile_definitions(yasag PRIVATE YASAG_SCREEN_W=${YASAG_SCREEN_W})
    endif()
    if(YASAG_SCREEN_H)
        target_compile_definitions(yasag PRIVATE YASAG_SCREEN_H=${YASAG_SCREEN_H})
    endif()

# ===========================================================================
elseif(YASAG_TARGET STREQUAL "sdl")
# ===========================================================================
    project(yasag C)

    find_package(SDL2 REQUIRED)

    add_executable(yasag
        ${YASAG_CORE}
        ${YASAG_CORE_HEADERS}
        hal/hal_sdl.h
        hal/hal_sdl.c
        hal/screenshot_sdl.c
    )

    # SDL default resolution 640x480 (override with -DYASAG_SCREEN_W/H); TS=2.
    if(YASAG_SCREEN_W)
        set(_SW ${YASAG_SCREEN_W})
    else()
        set(_SW 640)
    endif()
    if(YASAG_SCREEN_H)
        set(_SH ${YASAG_SCREEN_H})
    else()
        set(_SH 480)
    endif()
    target_compile_definitions(yasag PRIVATE
        YASAG_SCREEN_W=${_SW}
        YASAG_SCREEN_H=${_SH}
        YASAG_TEXT_SCALE=2
        YASAG_HAS_SCREENSHOT
    )
    # Window zoom: 1 = native 640x480 (default), 0 = auto-fit desktop, N = force NxN.
    if(DEFINED YASAG_SDL_ZOOM)
        target_compile_definitions(yasag PRIVATE YASAG_SDL_ZOOM=${YASAG_SDL_ZOOM})
    endif()

    target_include_directories(yasag PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/game
        ${CMAKE_CURRENT_LIST_DIR}/hal
    )
    target_compile_options(yasag PRIVATE -Wall)

    # Windows: prefer the static SDL2 target (one self-contained .exe).
    # Linux: always the shared SDL2 (see the libSDL2 handling below).
    if(WIN32 AND TARGET SDL2::SDL2-static)
        target_link_libraries(yasag SDL2::SDL2-static)
    elseif(TARGET SDL2::SDL2)
        target_link_libraries(yasag SDL2::SDL2)
        if(WIN32)
            message(WARNING "static SDL2 not found - linking the shared SDL2")
        endif()
    else()
        target_include_directories(yasag PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(yasag ${SDL2_LIBRARIES})
    endif()

    # Linux: link the shared libSDL2 (not static) and ship it beside the
    # executable. "$ORIGIN" in the run-path makes the executable look in its own
    # folder first, so the two files travel together. The real shared library
    # (following the .so symlinks) is copied under its soname next to the
    # executable in the build folder and into the top-level project folder.
    if(NOT WIN32)
        target_link_libraries(yasag m)
        target_link_options(yasag PRIVATE "LINKER:-rpath,$ORIGIN")
        find_library(SDL2_SHARED_LIB NAMES libSDL2-2.0.so.0 SDL2-2.0 SDL2)
        if(NOT SDL2_SHARED_LIB)
            message(FATAL_ERROR "libSDL2 shared library not found - install the SDL2 development package (e.g. libsdl2-dev)")
        endif()
        get_filename_component(SDL2_SHARED_REAL "${SDL2_SHARED_LIB}" REALPATH)
        add_custom_command(TARGET yasag POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_SHARED_REAL}" $<TARGET_FILE_DIR:yasag>/libSDL2-2.0.so.0
            COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_SHARED_REAL}" ${CMAKE_CURRENT_LIST_DIR}/libSDL2-2.0.so.0
        )
    endif()

    # The platform is part of the executable's file name (yasag-Windows.exe /
    # yasag-Linux), and the finished file is copied to the top-level project
    # folder (this file's folder).
    if(WIN32)
        set_target_properties(yasag PROPERTIES OUTPUT_NAME yasag-Windows)
    else()
        set_target_properties(yasag PROPERTIES OUTPUT_NAME yasag-Linux)
    endif()
    add_custom_command(TARGET yasag POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:yasag> ${CMAKE_CURRENT_LIST_DIR}/
    )

# ===========================================================================
elseif(YASAG_TARGET STREQUAL "picosystem")
# ===========================================================================
    message(FATAL_ERROR
        "YASAG_TARGET=picosystem arrives in Stage 4 (hal_picosystem.c + 240x240 "
        "layout). picocalc and sdl build today.")

else()
    message(FATAL_ERROR "Set -DYASAG_TARGET to picocalc | picosystem | sdl")
endif()
