cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) su macOS
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Su Windows: runtime C++ statico, cosi' il plug-in non richiede installare i Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(WantedLevel VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE viene scaricato automaticamente alla prima configurazione
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# Copia automatica nella cartella plug-in di sistema solo su macOS
# (su Windows richiederebbe permessi di amministratore)
if(APPLE)
    set(WL_COPY_AFTER_BUILD TRUE)
else()
    set(WL_COPY_AFTER_BUILD FALSE)
endif()

juce_add_plugin(WantedLevel
    PRODUCT_NAME             "Wanted Level"
    COMPANY_NAME             "Tune Talks"
    BUNDLE_ID                com.tunetalks.wantedlevel
    PLUGIN_MANUFACTURER_CODE Tntk
    PLUGIN_CODE              Wntd
    FORMATS                  VST3 AU Standalone   # AU viene compilato solo su macOS
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES          Fx Distortion
    AU_MAIN_TYPE             kAudioUnitType_Effect
    AU_SANDBOX_SAFE          TRUE                 # necessario per Logic Pro / GarageBand
    PLUGIN_DESCRIPTION       "5-star saturation and distortion"
    MICROPHONE_PERMISSION_ENABLED TRUE           # per l'app Standalone su macOS
    MICROPHONE_PERMISSION_TEXT    "Wanted Level needs audio input to process sound."
    COPY_PLUGIN_AFTER_BUILD  ${WL_COPY_AFTER_BUILD})

# Scritte in font Pricedown pre-renderizzate (il font non viene incluso)
juce_add_binary_data(WantedLevelAssets SOURCES
    Assets/title.png
    Assets/subtitle.png
    Assets/label_mix.png
    Assets/label_output.png
    Assets/status_0.png
    Assets/status_1.png
    Assets/status_2.png
    Assets/status_3.png
    Assets/status_4.png
    Assets/status_5.png)

target_sources(WantedLevel PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(WantedLevel PRIVATE Source)

target_compile_definitions(WantedLevel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(WantedLevel
    PRIVATE
        WantedLevelAssets
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
