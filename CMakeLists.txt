cmake_minimum_required(VERSION 3.22)
project(Petrofia VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COPY_AFTER_BUILD "Copy plugin to system VST3 folder after build" ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Petrofia
    COMPANY_NAME "Oceanus Peregrinus"
    BUNDLE_ID com.oceanusperegrinus.petrofia
    PLUGIN_MANUFACTURER_CODE Ocpg
    PLUGIN_CODE Ptrf
    FORMATS VST3 Standalone
    PRODUCT_NAME "Petrofia"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

target_sources(Petrofia PRIVATE
    Source/Dsp.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(Petrofia PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Petrofia
    PRIVATE juce::juce_audio_utils
            juce::juce_dsp
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
