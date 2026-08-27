cmake_minimum_required(VERSION 3.22)
project(AlphaPassiveComponent VERSION 1.0.0 LANGUAGES CXX)

# Configure with:
#   cmake -B build -G "Visual Studio 18 2026" -A x64
#   cmake --build build --config Release

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# Static MSVC runtime. REQUIRED for FL Studio: if this plugin's CRT is dynamic
# (/MD) and FL Studio's host process was built/linked against a different CRT
# instance, the DLL can load but crash or silently fail during the very first
# few lines of static/global init -- this shows up in FL as "the wrapper could
# not initialize the plugin" with no further detail, because the failure is
# below JUCE's own error reporting.
# ------------------------------------------------------------------------------
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if (MSVC)
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

# ------------------------------------------------------------------------------
# JUCE 8.0.12 -- point this at your local JUCE checkout/submodule.
# ------------------------------------------------------------------------------
add_subdirectory(JUCE)

juce_add_plugin(AlphaPassiveComponent
    COMPANY_NAME                "AlphaAudio"
    PLUGIN_MANUFACTURER_CODE    Awam
    PLUGIN_CODE                 Apc1
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "AlphaAudio Passive Component"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             "Fx" "EQ"
    # PLUGIN_CHANNEL_CONFIGURATIONS intentionally omitted (left unset), matching
    # the Projucer convention "must be empty" -- isBusesLayoutSupported() in the
    # processor is what actually governs supported layouts. Setting this to any
    # non-empty value here re-enables the legacy channel-config path and can
    # silently disagree with isBusesLayoutSupported(), which is another known
    # cause of a host refusing to instantiate the plugin at all.
)

juce_generate_juce_header(AlphaPassiveComponent)

target_sources(AlphaPassiveComponent PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(AlphaPassiveComponent PRIVATE
    Source
)

target_compile_definitions(AlphaPassiveComponent PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(AlphaPassiveComponent PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ------------------------------------------------------------------------------
# FL Studio only scans C:\Program Files\Common Files\VST3\ for VST3 plugins --
# it will not pick up a plugin sitting anywhere else, and a plugin copied to
# the wrong depth inside that folder (e.g. missing the .vst3 bundle's Contents
# subfolder) will fail to load even though it appears in the plugin database.
# COPY_PLUGIN_AFTER_BUILD above handles this automatically via JUCE's default
# install path; if you're copying manually, copy the whole *.vst3 folder, not
# its contents, directly into that directory.
# ------------------------------------------------------------------------------
