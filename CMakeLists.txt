cmake_minimum_required(VERSION 3.15)

project(AudioSlicerVST VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
# You need to download JUCE and place it in the appropriate location
# Download from: https://github.com/juce-framework/JUCE
# Either clone it into a 'JUCE' subdirectory or set JUCE_DIR

if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE framework")
endif()

if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}. Please download JUCE from https://github.com/juce-framework/JUCE")
endif()

# Plugin metadata
set(PLUGIN_NAME "AudioSlicer")
set(PLUGIN_MANUFACTURER_CODE "Mnfr")
set(PLUGIN_CODE "Slcr")

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "YourCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ${PLUGIN_MANUFACTURER_CODE}
    PLUGIN_CODE ${PLUGIN_CODE}
    FORMATS VST3 AU Standalone
    PRODUCT_NAME ${PLUGIN_NAME}
)

# Source files
target_sources(${PROJECT_NAME}
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
