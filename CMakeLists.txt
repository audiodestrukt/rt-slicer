cmake_minimum_required(VERSION 3.15)

# Versions are injected by CI. VERSION becomes CFBundleShortVersionString (the
# marketing version, taken from the release tag) and BUILD_VERSION becomes
# CFBundleVersion (a Unix epoch, so TestFlight always sees a larger build number
# than any previous upload). JUCE generates the Info.plist itself, so these have
# to arrive at CONFIGURE time -- a MARKETING_VERSION override on the xcodebuild
# command line does not reach a generated plist.
set(SLICER_VERSION "1.0.0" CACHE STRING "Marketing version (CFBundleShortVersionString)")
set(SLICER_BUILD_VERSION "" CACHE STRING "Build number (CFBundleVersion); defaults to SLICER_VERSION")

if(SLICER_BUILD_VERSION STREQUAL "")
    set(SLICER_BUILD_VERSION "${SLICER_VERSION}")
endif()

project(RipSlice VERSION ${SLICER_VERSION})

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
set(PLUGIN_NAME "RipSlice")
# Four-character manufacturer code. Must be unique and start with a capital;
# "Adsk" is Autodesk's and "Mnfr" was JUCE's placeholder. Changing this changes
# the plugin's unique ID, so it has to settle BEFORE anything ships.
set(PLUGIN_MANUFACTURER_CODE "Adst")
set(PLUGIN_CODE "Rips")

# iOS has no VST3; AUv3 is the only plugin format Apple allows, and it ships as
# an app extension inside a container app, so the Standalone target is not
# optional there -- it IS the container. Desktop keeps VST3 + Standalone, and
# macOS additionally builds AU so the same source covers Logic.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(SLICER_FORMATS Standalone AUv3)
    set(SLICER_COPY_AFTER_BUILD FALSE)
elseif(APPLE)
    set(SLICER_FORMATS VST3 AU Standalone)
    set(SLICER_COPY_AFTER_BUILD TRUE)
else()
    set(SLICER_FORMATS VST3 Standalone)
    set(SLICER_COPY_AFTER_BUILD TRUE)
endif()

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "Audiodestrukt"
    COMPANY_WEBSITE "https://audiodestrukt.com"
    # The AUv3 extension's bundle ID is derived by JUCE from the last
    # dot-component of this one, giving com.audiodestrukt.ripslice.ripsliceAUv3.
    # That is why the identifier is "ripslice" and not the "Rip.wav" wordmark --
    # a dotted identifier would yield ...rip.wav.wavAUv3.
    BUNDLE_ID "com.audiodestrukt.ripslice"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # An effect that is played by incoming MIDI is a *music effect*, not a plain
    # effect. JUCE infers this from IS_SYNTH + NEEDS_MIDI_INPUT, but it is
    # stated explicitly because it is load-bearing and silent when wrong: as
    # kAudioUnitType_Effect the audio path still works while iOS hosts route no
    # MIDI to it, so every slice trigger stops working with nothing to see.
    AU_MAIN_TYPE kAudioUnitType_MusicEffect

    # The plugin records live input, so the container app must declare why it
    # wants the microphone or iOS terminates it on first access.
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT
        "RipSlice slices the audio coming into your device, so it needs access to your microphone and audio inputs."

    # Keep processing when the host is foregrounded or the screen locks --
    # without this an AUv3 stops running as soon as the container app is
    # backgrounded.
    BACKGROUND_AUDIO_ENABLED TRUE

    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS  UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    REQUIRES_FULL_SCREEN FALSE

    COPY_PLUGIN_AFTER_BUILD ${SLICER_COPY_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE ${PLUGIN_MANUFACTURER_CODE}
    PLUGIN_CODE ${PLUGIN_CODE}
    VERSION ${SLICER_VERSION}
    BUILD_VERSION ${SLICER_BUILD_VERSION}

    FORMATS ${SLICER_FORMATS}
    PRODUCT_NAME ${PLUGIN_NAME}
)

# Generate JuceHeader.h for legacy include style
juce_generate_juce_header(${PROJECT_NAME})

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
