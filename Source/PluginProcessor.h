#pragma once

#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Wavetable.h"
#include "LapizVoice.h"

// Which wavetable is loaded, and where it came from.
struct TableChoice
{
    enum class Kind { factory, file, drawn };

    Kind kind = Kind::factory;
    int factoryIndex = 0;                      // for factory
    juce::File file;                           // for file
    juce::String name;                         // shown in the wavetable picker
    std::vector<Wavetable::Frame> keyframes;   // for drawn: the waves you drew
    juce::uint32 editorTag = 0;                // for drawn: which Waves tab sent it (0 = loaded from a project)
};

// The "processor" is the plugin itself: it receives MIDI notes and produces audio.
// It sends a change message whenever a different wavetable is loaded.
class LapizAudioProcessor : public juce::AudioProcessor,
                            public juce::ChangeBroadcaster,
                            private juce::Timer
{
public:
    LapizAudioProcessor();
    ~LapizAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // What the newest note is doing right now, after the filter envelope and LFO have moved things.
    // Written by the audio thread, read by the displays. cutoff and position are -1 when nothing plays.
    struct LiveState
    {
        std::atomic<float> cutoff { -1.0f };
        std::atomic<float> position { -1.0f };
        std::atomic<double> lfoCycles { 0.0 };
    };

    // Used by the interface to attach knobs and draw the live visuals.
    juce::AudioProcessorValueTreeState& getState() { return parameters; }
    const LiveState& getLive() const { return live; }

    // Wavetables. Call these from the interface (not from the audio thread).
    std::shared_ptr<const Wavetable> getTable() const;
    TableChoice getTableChoice() const;
    void selectFactoryTable (int index);
    bool selectTableFile (const juce::File& file);                        // false if the file couldn't be read
    void setDrawnTable (std::vector<Wavetable::Frame> keyframes, const juce::String& name, juce::uint32 editorTag = 0);
    void renameDrawnTable (const juce::String& name);

    // Your most recent drawing, kept after you switch to another table so it can be picked again.
    TableChoice getLastDrawing() const;

    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void advanceLfoClock (int numSamples);
    void publishLiveState();

    void installTable (std::shared_ptr<const Wavetable> newTable, TableChoice newChoice);
    void timerCallback() override; // frees old wavetables once the audio thread is done with them

    static constexpr int numVoices = 8;

    juce::Synthesiser synth;
    juce::AudioProcessorValueTreeState parameters;

    // The current wavetable. The audio thread only ever reads audioTable, once per block.
    // A replaced table is kept in `retired` until the audio thread can no longer be using it.
    mutable juce::CriticalSection tableLock;
    std::shared_ptr<const Wavetable> table;
    TableChoice tableChoice, lastDrawing;
    std::vector<std::shared_ptr<const Wavetable>> retired;
    std::atomic<const Wavetable*> audioTable { nullptr };
    std::atomic<bool> inBlock { false };
    const Wavetable* blockTable = nullptr; // what the voices play during the current block

    Lfo::Clock lfoClock;
    double lfoCycles = 0.0; // the shared LFO's position, in cycles
    LiveState live;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LapizAudioProcessor)
};
