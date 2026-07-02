cmake_minimum_required(VERSION 3.15)

# Added 'C' alongside 'CXX' to resolve compile errors [43]
project(NavyArp VERSION 1.0.0 LANGUAGES C CXX)

# Include your local JUCE directory
add_subdirectory(JUCE)

# Add and define the Navy Arp plugin targets (STRICTLY VST3 ONLY) [43]
juce_add_plugin(NavyArp
    COMPANY_NAME "Navy Audio"
    BUNDLE_ID "com.navyaudio.navyarp"
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "Dgav"
    FORMATS VST3                     # Strictly VST3 format only [43]
    PRODUCT_NAME "Navy Arp"
    PLUGIN_NAME "Navy Arp"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE    # Disable default multi-folder copy step
)

# Mandatory generator macro call to create JuceHeader.h
juce_generate_juce_header(NavyArp)

# Custom post-build step to copy ONLY the single flat VST3 file to your custom folder
add_custom_command(TARGET NavyArp_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "$<TARGET_FILE:NavyArp_VST3>"
    "D:/Music - Software/VST/Navy Arp.vst3"
)

# Source Files mapping
target_sources(NavyArp PRIVATE
    source/AppTheme.h
    source/ChromaCapsLookAndFeel.cpp
    source/ChromaCapsLookAndFeel.h
    source/OledDisplay.cpp
    source/OledDisplay.h
    source/PluginEditor.cpp
    source/PluginEditor.h
    source/PluginProcessor.cpp
    source/PluginProcessor.h
)

# Global compiler definitions
target_compile_definitions(NavyArp
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1 # Added to bypass parameter automation ID conflict checks
)

# Library dependencies
target_link_libraries(NavyArp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_events
        juce::juce_data_structures
        juce::juce_core
)
