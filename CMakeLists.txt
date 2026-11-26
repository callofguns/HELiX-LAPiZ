cmake_minimum_required(VERSION 3.22)

project(Lapiz VERSION 1.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE lives in libs/JUCE (pinned to release 9.0.4).
add_subdirectory(libs/JUCE)

# Release builds (made by .github/workflows/release.yml) turn this on so the plugin updates itself
# from GitHub Releases. Leave it off for your own builds, or they'd replace themselves with the published version.
option(LAPIZ_AUTO_UPDATE "Let this build download and install new releases by itself (Windows)" OFF)

# Copying to Documents\VST3 after every build is handy at home but not on a build server.
option(LAPIZ_COPY_AFTER_BUILD "Copy the plugin to Documents\\VST3 after building" ON)

# HELiX is the maker name shared by all the plugins; LAPiZ is this one.
# The codes identify the plugin to hosts, so they should stay unchanged once released.
juce_add_plugin(Lapiz
    COMPANY_NAME "HELiX"
    BUNDLE_ID com.helix.lapiz
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Hlxp
    PLUGIN_CODE Lpz1
    FORMATS VST3 Standalone
    # Every build copies the plugin to Documents\VST3 (added to FL Studio's plugin search paths),
    # so updates need no admin prompt. FL Studio must be closed while building.
    COPY_PLUGIN_AFTER_BUILD ${LAPIZ_COPY_AFTER_BUILD}
    VST3_COPY_DIR "$ENV{USERPROFILE}/Documents/VST3"
    PRODUCT_NAME "LAPiZ")

target_sources(Lapiz PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UI/SimplePage.cpp
    Source/UI/SimplePage.h
    Source/UI/AdvancedPage.cpp
    Source/UI/AdvancedPage.h
    Source/UI/EffectsPage.cpp
    Source/UI/EffectsPage.h
    Source/UI/Panel.h
    Source/UI/LfoView.h
    Source/UI/WavesPage.cpp
    Source/UI/WavesPage.h
    Source/UI/WaveCanvas.h
    Source/UI/KeyframeList.h
    Source/UI/WavetablePicker.h
    Source/UI/PresetBar.h
    Source/UI/PianoKeyboard.h
    Source/Lfo.h
    Source/Effects.h
    Source/FactoryTables.h
    Source/FactoryPresets.h
    Source/WavetableFile.h
    Source/UI/Theme.h
    Source/UI/Settings.h
    Source/UI/LapizLookAndFeel.h
    Source/UI/LabeledKnob.h
    Source/UI/WaveformView.h
    Source/UI/FilterView.h
    Source/UI/EnvelopeView.h
    Source/Wavetable.h
    Source/LapizVoice.h
    Source/FilterMath.h
    Source/UpdateLogic.h
    Source/Updater.cpp
    Source/Updater.h
    Source/Version.h)

target_compile_definitions(Lapiz PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

if(LAPIZ_AUTO_UPDATE)
    target_compile_definitions(Lapiz PRIVATE LAPIZ_AUTO_UPDATE=1)
endif()

# Fonts embedded in the plugin (Inter, SIL Open Font License - see Resources/Fonts/OFL.txt).
# Apple's SF Pro would be the first choice, but its license doesn't allow building it into a Windows plugin.
juce_add_binary_data(LapizData SOURCES
    Resources/Fonts/Inter-Regular.ttf
    Resources/Fonts/Inter-Bold.ttf)

# Command-line test: plays notes through the synth and checks the numbers (see Tests/RenderTest.cpp).
juce_add_console_app(LapizTests PRODUCT_NAME "LapizTests")

target_sources(LapizTests PRIVATE Tests/RenderTest.cpp)
target_include_directories(LapizTests PRIVATE Source)
target_compile_definitions(LapizTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="LAPiZ")
target_link_libraries(LapizTests
    PRIVATE
        Lapiz
        LapizData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_link_libraries(Lapiz
    PRIVATE
        LapizData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
