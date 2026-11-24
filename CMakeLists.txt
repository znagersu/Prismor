cmake_minimum_required(VERSION 3.22)

project(Prismor VERSION 2.0.0 LANGUAGES C CXX)

enable_testing()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(NOT TARGET juce::juce_audio_utils)
    if(NOT JUCE_DIR)
        if(EXISTS "C:/Users/endio/SDKs/JUCE/CMakeLists.txt")
            set(JUCE_DIR "C:/Users/endio/SDKs/JUCE")
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
            set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
        else()
            message(FATAL_ERROR "JUCE not found. Configure with -DJUCE_DIR=C:/Users/endio/SDKs/JUCE")
        endif()
    endif()
    message(STATUS "Using JUCE at: ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/_tools/JUCE" EXCLUDE_FROM_ALL)
endif()

juce_add_plugin(Prismor
    VERSION 2.0.0
    FORMATS VST3 Standalone
    PRODUCT_NAME "Prismor"
    DESCRIPTION "Prismor - Spectral Additive & Resynthesis Synthesizer"
    COMPANY_NAME "CyberSpectralAudio"
    COMPANY_COPYRIGHT "CyberSpectralAudio 2026"
    BUNDLE_ID "com.CyberSpectralAudio.Prismor"
    PLUGIN_MANUFACTURER_CODE Cysa
    PLUGIN_CODE Prsm
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    NEEDS_CURL FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(Prismor)

target_sources(Prismor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/WebViewEditor.cpp
        Source/WebViewEditor.h
        Source/Updater.h
        Source/DSP/DspUtils.h
        Source/DSP/SplineCurve.h
        Source/DSP/ResynthesisEngine.h
        Source/DSP/FxRack.h
        Source/DSP/SpectralSynthEngine.h
        Source/DSP/FactoryPresetBank.h
)

target_compile_features(Prismor PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(Prismor PUBLIC /bigobj /utf-8)
endif()

juce_add_binary_data(Prismor_WebData
    NAMESPACE Prismor_UI
    SOURCES
        Source/UI/index.html
        Source/UI/style.css
        Source/UI/app.js
)

target_compile_definitions(Prismor
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Prismor
    PRIVATE
        Prismor_WebData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

add_executable(PrismorTests Tests/PrismorTests.cpp)
target_link_libraries(PrismorTests
    PRIVATE
        Prismor
        Prismor_WebData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
)
target_compile_features(PrismorTests PRIVATE cxx_std_20)
target_include_directories(PrismorTests PRIVATE
    $<TARGET_PROPERTY:Prismor,INCLUDE_DIRECTORIES>
)
target_compile_definitions(PrismorTests PRIVATE
    $<TARGET_PROPERTY:Prismor,COMPILE_DEFINITIONS>
    JUCE_STANDALONE_APPLICATION=1
)
add_test(NAME PrismorTests COMMAND PrismorTests)

add_executable(PrismorRender Tests/PrismorRender.cpp)
target_link_libraries(PrismorRender
    PRIVATE
        Prismor
        Prismor_WebData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
)
target_compile_features(PrismorRender PRIVATE cxx_std_20)
target_include_directories(PrismorRender PRIVATE
    $<TARGET_PROPERTY:Prismor,INCLUDE_DIRECTORIES>
)
target_compile_definitions(PrismorRender PRIVATE
    $<TARGET_PROPERTY:Prismor,COMPILE_DEFINITIONS>
    JUCE_STANDALONE_APPLICATION=1
)
