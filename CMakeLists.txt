cmake_minimum_required(VERSION 3.22)

project(Helix VERSION 0.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE lives in libs/JUCE (pinned to release 9.0.4).
add_subdirectory(libs/JUCE)

# COMPANY_NAME and the two 4-letter codes are placeholders for now.
# The codes identify the plugin to hosts, so they should stay unchanged once released.
juce_add_plugin(Helix
    COMPANY_NAME "Helix Project"
    BUNDLE_ID com.helixproject.helix
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Hlxp
    PLUGIN_CODE Hlx1
    FORMATS VST3 Standalone
    # Every build copies the plugin to Documents\VST3 (added to FL Studio's plugin search paths),
    # so updates need no admin prompt. FL Studio must be closed while building.
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "$ENV{USERPROFILE}/Documents/VST3"
    PRODUCT_NAME "Helix")

target_sources(Helix PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UI/SimplePage.cpp
    Source/UI/SimplePage.h
    Source/UI/AdvancedPage.cpp
    Source/UI/AdvancedPage.h
    Source/UI/Panel.h
    Source/UI/LfoView.h
    Source/UI/WavesPage.cpp
    Source/UI/WavesPage.h
    Source/UI/WaveCanvas.h
    Source/UI/KeyframeList.h
    Source/UI/WavetablePicker.h
    Source/Lfo.h
    Source/FactoryTables.h
    Source/WavetableFile.h
    Source/UI/Theme.h
    Source/UI/Settings.h
    Source/UI/HelixLookAndFeel.h
    Source/UI/LabeledKnob.h
    Source/UI/WaveformView.h
    Source/UI/FilterView.h
    Source/UI/EnvelopeView.h
    Source/Wavetable.h
    Source/HelixVoice.h
    Source/FilterMath.h
    Source/Version.h)

target_compile_definitions(Helix PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Fonts embedded in the plugin (Atkinson Hyperlegible, SIL Open Font License - see Resources/Fonts/OFL.txt).
juce_add_binary_data(HelixData SOURCES
    Resources/Fonts/AtkinsonHyperlegible-Regular.ttf
    Resources/Fonts/AtkinsonHyperlegible-Bold.ttf)

# Command-line test: plays notes through the synth and checks the numbers (see Tests/RenderTest.cpp).
juce_add_console_app(HelixTests PRODUCT_NAME "HelixTests")

target_sources(HelixTests PRIVATE Tests/RenderTest.cpp)
target_include_directories(HelixTests PRIVATE Source)
target_compile_definitions(HelixTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Helix")
target_link_libraries(HelixTests
    PRIVATE
        Helix
        HelixData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_link_libraries(Helix
    PRIVATE
        HelixData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
