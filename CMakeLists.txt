cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PamplejuceVersion)
include(CPM)
include(PamplejuceMacOS)
include(JUCEDefaults)

# Rebranded Project Variables
set(PROJECT_NAME "NavyArp")
set(PRODUCT_NAME "Navy Arp")
set(COMPANY_NAME "Navy Audio")
set(BUNDLE_ID "com.navyaudio.navyarp")
set(FORMATS Standalone VST3)

set(CMAKE_POLICY_DEFAULT_CMP0141 NEW CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})
include(PamplejuceLog)

add_subdirectory(JUCE)
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)
add_subdirectory(modules/melatonin_inspector)

juce_add_plugin("${PROJECT_NAME}"
    # Relocated icon path to allow safe deletion of the packaging/ directory
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/icon.png"
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Navy
    PLUGIN_CODE Narp
    FORMATS "${FORMATS}"
    PRODUCT_NAME "${PRODUCT_NAME}"
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
)

add_library(SharedCode INTERFACE)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES instrument synthesiser)

include(SharedCodeDefaults)

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

include(Assets)
include(XcodePrettify)

target_compile_definitions(SharedCode
    INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"
    PRODUCT_NAME_WITHOUT_VERSION="NavyArp"
)

target_link_libraries(SharedCode
    INTERFACE
    Assets
    melatonin_inspector
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

include(PamplejuceIPP)
# Removed Tests, Benchmarks, and GitHubENV includes to cleanly match the deleted modular folders
