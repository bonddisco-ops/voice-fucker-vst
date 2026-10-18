cmake_minimum_required(VERSION 3.22)

# Universal macOS build that also runs on older macOS versions
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
# Static MSVC runtime: the plugin does not depend on the host's C++ runtime DLLs
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(VoiceFucker VERSION 2.2.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Local test builds can point at an already downloaded JUCE folder
set(VF_JUCE_DIR "" CACHE PATH "Optional path to a local JUCE checkout")
option(VF_HEADLESS_TEST "Linux test build without optional system libraries" OFF)

if(VF_JUCE_DIR)
    add_subdirectory(${VF_JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(VF_FORMATS VST3 AU Standalone)
else()
    set(VF_FORMATS VST3 Standalone)
endif()

juce_add_plugin(VoiceFucker
    COMPANY_NAME "Primal Rage Lab"
    COMPANY_WEBSITE ""
    BUNDLE_ID "com.primalragelab.voicefucker"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
    VST3_CATEGORIES Fx Distortion
    PLUGIN_MANUFACTURER_CODE Prlb
    PLUGIN_CODE Vfkr
    FORMATS ${VF_FORMATS}
    COPY_PLUGIN_AFTER_BUILD FALSE
    PRODUCT_NAME "Voice Fucker")

target_sources(VoiceFucker PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Components.cpp
    Source/PresetManager.cpp)

target_compile_definitions(VoiceFucker PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

if(VF_HEADLESS_TEST)
    target_compile_definitions(VoiceFucker PUBLIC
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JUCE_USE_XRENDER=0 JUCE_USE_XSHM=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_link_libraries(VoiceFucker
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# Offline test tool: renders audio through the processor and a screenshot of the UI
if(VF_HEADLESS_TEST)
    juce_add_console_app(VFTest PRODUCT_NAME "VFTest")
    target_sources(VFTest PRIVATE
        Tests/OfflineTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Components.cpp
        Source/PresetManager.cpp)
    target_include_directories(VFTest PRIVATE Source)
    target_compile_definitions(VFTest PRIVATE
        JucePlugin_Name="Voice Fucker" JucePlugin_VersionString="2.2.2" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JUCE_USE_XRENDER=0 JUCE_USE_XSHM=0 JUCE_ALSA=0 JUCE_JACK=0)
    target_link_libraries(VFTest PRIVATE juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
