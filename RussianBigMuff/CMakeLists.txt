cmake_minimum_required(VERSION 3.22)

project(RussianBigMuff VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal Mac build" FORCE)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(RBM_FORMATS AU VST3 Standalone)
else()
    set(RBM_FORMATS VST3 Standalone)
endif()

juce_add_plugin(RussianBigMuff
    COMPANY_NAME "Grok Audio"
    COMPANY_WEBSITE "https://x.ai"
    BUNDLE_ID "audio.grok.russianbigmuff"
    PLUGIN_MANUFACTURER_CODE GrkA
    PLUGIN_CODE RuBM
    FORMATS ${RBM_FORMATS}
    PRODUCT_NAME "Russian Big Muff"
    PLUGIN_DESCRIPTION "Russian Big Muff style 3-stage diode fuzz"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    AU_SANDBOX_SAFE TRUE
)

target_sources(RussianBigMuff
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/rbm_core.c
        Source/rbm_core.h
)

target_compile_definitions(RussianBigMuff
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(RussianBigMuff
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
