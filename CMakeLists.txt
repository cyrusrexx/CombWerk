# ==============================================================================
# CombWerk – Multistage Comb Drum FX Plugin
# CLAP (primary) + VST3 + AU via JUCE + clap-juce-extensions
# ==============================================================================

cmake_minimum_required(VERSION 3.21)

# --- macOS universal binary (Apple Silicon + Intel) ---------------------------
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(CombWerk VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- JUCE (submodule at modules/JUCE) ----------------------------------------
add_subdirectory(modules/JUCE)

# --- clap-juce-extensions (submodule at dependencies/clap-juce-extensions) ----
add_subdirectory(dependencies/clap-juce-extensions EXCLUDE_FROM_ALL)

# --- Plugin target ------------------------------------------------------------
juce_add_plugin(CombWerk
    COMPANY_NAME            "CombWerk"
    PLUGIN_MANUFACTURER_CODE Cwrk
    PLUGIN_CODE              Cwfx
    FORMATS                  VST3 AU Standalone     # CLAP added below
    PRODUCT_NAME             "CombWerk"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  TRUE
)

# --- Enable CLAP via clap-juce-extensions ------------------------------------
clap_juce_extensions_plugin(
    TARGET CombWerk
    CLAP_ID "com.combwerk.drumfx"
    CLAP_FEATURES audio-effect mixing filter distortion reverb
    CLAP_USE_JUCE_PARAMETER_RANGES ALL
)

# --- Source files -------------------------------------------------------------
target_sources(CombWerk PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# --- Compile definitions ------------------------------------------------------
target_compile_definitions(CombWerk PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# --- Link libraries -----------------------------------------------------------
target_link_libraries(CombWerk
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        clap_juce_extensions
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
