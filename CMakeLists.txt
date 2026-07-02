cmake_minimum_required(VERSION 3.15)

# Added 'C' alongside 'CXX' to resolve the CMAKE_C_COMPILE_OBJECT missing variable crash
project(NavyArp VERSION 1.0.0 LANGUAGES C CXX)

# 1. Include your local JUCE directory
add_subdirectory(JUCE)

# 2. Add and define the Navy Arp plugin targets
juce_add_plugin(NavyArp
    COMPANY_NAME "Navy Audio"
    BUNDLE_ID "com.navyaudio.navyarp"       # Added to resolve the spaces warning
    PLUGIN_MANUFACTURER_CODE "Manu"         # Changed from hex to clean 4-char string literal
    PLUGIN_CODE "Dgav"                      # Changed from hex to clean 4-char string literal
    FORMATS VST3 Standalone CLAP
    PRODUCT_NAME "Navy Arp"
    PLUGIN_NAME "Navy Arp"
    IS_SYNTH TRUE
    PRODUCES_MIDI_OUTPUT TRUE
    WANTS_MIDI_INPUT TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE           # Disable default multi-folder copy step
)

# 3. Custom post-build step to copy ONLY the single flat VST3 file to your custom folder
add_custom_command(TARGET NavyArp_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "$<TARGET_FILE:NavyArp_VST3>"
    "D:/Music - Software/VST/Navy Arp.vst3"
)

# 4. Source Files mapping
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

# 5. Global compiler definitions
target_compile_definitions(NavyArp
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# 6. Library dependencies
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
