cmake_minimum_required(VERSION 3.20)
project(Final_Fantasy_7 LANGUAGES C CXX)

set(PSXRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp" CACHE PATH
    "Path to psxrecomp")

# --- Opt-in product surfaces (OFF by default in the framework) -------------
# Scaffold prompts / flags fill these. Without recomp-ui, PSX_RECOMP_UI is OFF.
# Setup-host CI requires WIZARD_CMAKE_BLOCK / ENABLE_SETUP_WIZARD (FORCE_SETUP_HOST
# alone does not open first-run / Generate & rebuild).
# recomp-ui submodule present (PSX_RECOMP_UI defaults ON).
# Full netplay UI — enable after adding recomp-ui + testing:
# if(EXISTS "${PSXRECOMP_ROOT}/lib/recomp-net/CMakeLists.txt")
#     set(PSX_NETPLAY ON CACHE BOOL "…" FORCE)
# endif()
set(PSX_SETUP_WIZARD ON CACHE BOOL
    "Advertise first-run setup wizard + Generate & rebuild in recomp-ui")
set(PSX_BUILTIN_MODS_EXCLUDE
    "psx.enhancement.cd-speed" "psx.enhancement.fast-loading")
include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

set(_FF7_SETUP_OPTION "")
if(PSX_SETUP_WIZARD)
    set(_FF7_SETUP_OPTION ENABLE_SETUP_WIZARD)
endif()

psxrecomp_add_game_runtime(psx-runtime
    # ENABLE_NETPLAY_IF_PRESENT
    # NETPLAY_LOBBY_URL "ws://netplay.retcomm.net:8765"
    ${_FF7_SETUP_OPTION}
    WINDOW_TITLE "Final Fantasy 7"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    MAX_PLAYERS 1
    LAUNCHER_BOXART "${CMAKE_CURRENT_SOURCE_DIR}/launcher_assets/img/boxart.tga"
    APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/psxrecomp.ico"
    # The framework stages this alongside its own mods/builtin/packages into
    # <exe-dir>/mods/bundled; the title must not copy it itself. Must sit
    # BEFORE GEN_FULL_GLOB/CODEGEN_SETUP_SOURCES: psxrecomp_add_game_runtime
    # does not know this keyword, so after a multi-value keyword it would be
    # swallowed into that list instead of forwarded.
    PRELOADED_MODS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded"
    GEN_MARKER "generated/SCUS_941.63_dispatch.c"
    GEN_FULL_GLOB "generated/SCUS_941.63_full_*.c"
    CODEGEN_SETUP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/codegen_setup.c"
        # Game-owned trusted mod plugins (widescreen). Registered by a
        # constructor, selected by mods/preloaded/packages/ff7.enhancements.
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ff7_mods.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ff7_auto_disc.c"
)

# Stage runtime configs next to the exe for direct build/ launches. Only copy
# files that exist — never fail POST_BUILD when game_options.toml is absent.
foreach(_PSX_RUNTIME_CONFIG game.toml game_options.toml)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}")
        add_custom_command(TARGET psx-runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}"
                "$<TARGET_FILE_DIR:psx-runtime>/${_PSX_RUNTIME_CONFIG}"
            COMMENT "Staging ${_PSX_RUNTIME_CONFIG}"
            VERBATIM)
    endif()
endforeach()
