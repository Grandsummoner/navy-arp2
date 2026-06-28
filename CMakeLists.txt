cmake_minimum_required(VERSION 3.22)

project(NavyArp VERSION 1.0.0 LANGUAGES C CXX)

# Force standard C++20 compilation
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX /utf-8 /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# Include JUCE Framework sub-repository
if(NOT TARGET juce::juce_recommended_config_flags)
    add_subdirectory(JUCE)
endif()

# Include CLAP JUCE Extensions sub-repository
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/clap-juce-extensions")
    add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)
endif()

# Define the main Audio Plugin Target (Configured as Synth for cross-DAW VST3 compatibility)
juce_add_plugin(NavyArp
    VERSION "1.0.0"
    ICON_LOCAL_PATH "assets/images/icon.png"
    FORMATS Standalone VST3
    PRODUCT_NAME "Navy Arp"
    PLUGIN_NAME "Navy Arp"
    COMPANY_NAME "Navy Audio"
    IS_SYNTH TRUE                 # Reclassified as Synth/Instrument to allow Ableton VST3 scanning
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE          # Disabled pure MIDI Effect classification to prevent DAW scanner reject
    VST3_CATEGORIES "Instrument" "Generator"
)

# Instructs CMake to generate the JuceHeader.h file in the build tree
juce_generate_juce_header(NavyArp)

# Generate the companion CLAP format target if the extension subdirectory is loaded
if(TARGET clap_juce_extensions)
    clap_juce_extensions_plugin(TARGET NavyArp
        CLAP_ID "com.Navy Audio.NavyArp"
        CLAP_FEATURES "instrument" "arpeggiator" "utility" "midi"
    )
endif()

# Target Source Files Listing
target_sources(NavyArp PRIVATE
    source/AppTheme.h
    source/OledDisplay.h
    source/OledDisplay.cpp
    source/ChromaCapsLookAndFeel.h
    source/ChromaCapsLookAndFeel.cpp
    source/PluginProcessor.h
    source/PluginProcessor.cpp
    source/PluginEditor.h
    source/PluginEditor.cpp
)

# Standard JUCE Core Modules Dependencies
target_link_libraries(NavyArp PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_data_structures
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

# Propagate suppression macro to prevent VST2 parameter automation warnings/errors on VST3 wrapper compilation
target_compile_definitions(NavyArp PUBLIC
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

target_include_directories(NavyArp PRIVATE source)

if(TARGET clap_juce_extensions)
    target_link_libraries(NavyArp PRIVATE clap_juce_extensions)
endif()
