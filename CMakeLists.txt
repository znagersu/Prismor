cmake_minimum_required(VERSION 3.22)

project(Prismor VERSION 1.0.80.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(STATUS "Using local JUCE in ${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/JUCE" JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../IronBus/JUCE/CMakeLists.txt")
    message(STATUS "Using shared local JUCE in ../IronBus/JUCE")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../IronBus/JUCE" JUCE)
elseif(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(STATUS "Using local JUCE at ${JUCE_PATH}")
    add_subdirectory("${JUCE_PATH}" JUCE)
else()
    message(STATUS "Fetching JUCE via FetchContent...")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Prismor
    COMPANY_NAME "CyberSpectralAudio"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Cysa
    PLUGIN_CODE Prsm
    FORMATS VST3 Standalone
    PRODUCT_NAME "Prismor 1.0.80.0"
)

juce_generate_juce_header(Prismor)

target_sources(Prismor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DSP/FactoryPresetBank.h
        Source/DSP/SplineCurve.h
        Source/DSP/ResynthesisEngine.h
        Source/DSP/SpectralSynthEngine.h
        Source/UI/PrismorLookAndFeel.h
        Source/UI/RapidLookAndFeel.h
        Source/UI/RapidLookAndFeel.cpp
        Source/UI/ArticulatorEditor.h
        Source/UI/Visualizer3D.h
)

target_compile_definitions(Prismor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Prismor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MINGW)
    target_link_options(Prismor PUBLIC -static)
endif()
