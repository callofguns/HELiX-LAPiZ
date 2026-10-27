#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryTables.h"
#include "WavetableFile.h"

HelixAudioProcessor::HelixAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "HelixState", createParameterLayout())
{
    selectFactoryTable (0);
    blockTable = audioTable.load();
    startTimer (250);

    VoiceParams voiceParams;
    voiceParams.position = parameters.getRawParameterValue ("position");
    voiceParams.attack   = parameters.getRawParameterValue ("attack");
    voiceParams.decay    = parameters.getRawParameterValue ("decay");
    voiceParams.sustain  = parameters.getRawParameterValue ("sustain");
    voiceParams.release  = parameters.getRawParameterValue ("release");
    voiceParams.filterType = parameters.getRawParameterValue ("filterType");
    voiceParams.cutoff     = parameters.getRawParameterValue ("cutoff");
    voiceParams.resonance  = parameters.getRawParameterValue ("resonance");
    voiceParams.fenvAttack  = parameters.getRawParameterValue ("fenvAttack");
    voiceParams.fenvDecay   = parameters.getRawParameterValue ("fenvDecay");
    voiceParams.fenvSustain = parameters.getRawParameterValue ("fenvSustain");
    voiceParams.fenvRelease = parameters.getRawParameterValue ("fenvRelease");
    voiceParams.fenvAmount  = parameters.getRawParameterValue ("fenvAmount");
    voiceParams.lfoShape     = parameters.getRawParameterValue ("lfoShape");
    voiceParams.lfoTarget    = parameters.getRawParameterValue ("lfoTarget");
    voiceParams.lfoAmount    = parameters.getRawParameterValue ("lfoAmount");
    voiceParams.lfoRetrigger = parameters.getRawParameterValue ("lfoRetrigger");
    voiceParams.lfoClock = &lfoClock;
    voiceParams.table = &blockTable;

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new HelixVoice (voiceParams));

    synth.addSound (new HelixSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout HelixAudioProcessor::createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<Param> (juce::ParameterID { "position", 1 }, "Wave Position",
                                         Range (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "attack", 1 }, "Attack",
                                         Range (0.001f, 5.0f, 0.0f, 0.4f), 0.01f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "decay", 1 }, "Decay",
                                         Range (0.001f, 5.0f, 0.0f, 0.4f), 0.2f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "sustain", 1 }, "Sustain",
                                         Range (0.0f, 1.0f), 0.8f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "release", 1 }, "Release",
                                         Range (0.001f, 5.0f, 0.0f, 0.4f), 0.3f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "filterType", 1 }, "Filter Type",
                                                              juce::StringArray { "Low-pass", "High-pass", "Band-pass" }, 0));

    // 20 Hz to 20 kHz, with the knob's halfway point at 1 kHz. Fully open by default.
    Range cutoffRange (20.0f, 20000.0f);
    cutoffRange.setSkewForCentre (1000.0f);
    layout.add (std::make_unique<Param> (juce::ParameterID { "cutoff", 1 }, "Cutoff", cutoffRange, 20000.0f));

    layout.add (std::make_unique<Param> (juce::ParameterID { "resonance", 1 }, "Resonance",
                                         Range (0.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<Param> (juce::ParameterID { "volume", 1 }, "Volume",
                                         Range (0.0f, 1.0f), 0.7f));

    // Filter envelope: its own Attack/Decay/Sustain/Release, and how far it moves the cutoff.
    // Sweep is 0 by default, so the filter only moves once you turn it.
    layout.add (std::make_unique<Param> (juce::ParameterID { "fenvAttack", 1 }, "Filter Env Attack",
                                         Range (0.001f, 5.0f, 0.0f, 0.4f), 0.01f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "fenvDecay", 1 }, "Filter Env Decay",
                                         Range (0.001f, 5.0f, 0.0f, 0.4f), 0.4f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "fenvSustain", 1 }, "Filter Env Sustain",
                                         Range (0.0f, 1.0f), 0.3f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "fenvRelease", 1 }, "Filter Env Release",
                                         Range (0.001f, 5.0f, 0.0f, 0.4f), 0.3f));
    layout.add (std::make_unique<Param> (juce::ParameterID { "fenvAmount", 1 }, "Filter Env Amount",
                                         Range (-1.0f, 1.0f), 0.0f));

    // LFO. Wobble (amount) is 0 by default, so nothing moves until you turn it.
    juce::StringArray shapes, targets, divisions;
    for (auto* name : Lfo::shapeNames)  shapes.add (name);
    for (auto* name : Lfo::targetNames) targets.add (name);
    for (auto& d : Lfo::divisions)      divisions.add (d.name);

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfoShape", 1 }, "LFO Shape", shapes, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfoTarget", 1 }, "LFO Target", targets, 0));
    layout.add (std::make_unique<Param> (juce::ParameterID { "lfoAmount", 1 }, "LFO Amount", Range (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "lfoSync", 1 }, "LFO Tempo Sync", true));

    Range rateRange (0.05f, 20.0f);
    rateRange.setSkewForCentre (2.0f);
    layout.add (std::make_unique<Param> (juce::ParameterID { "lfoRate", 1 }, "LFO Rate", rateRange, 2.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfoDivision", 1 }, "LFO Sync Rate",
                                                              divisions, Lfo::defaultDivision));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "lfoRetrigger", 1 }, "LFO Restart On Note", false));

    return layout;
}

void HelixAudioProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

// Sets the shared LFO's speed and position for the coming block.
// In tempo mode, while FL Studio is playing, the LFO is locked to the song position,
// so the wobble lands on the beat every time you press play.
void HelixAudioProcessor::advanceLfoClock (int numSamples)
{
    const double sampleRate = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    double cyclesPerSample = 0.0;

    if (parameters.getRawParameterValue ("lfoSync")->load() > 0.5f)
    {
        double bpm = 120.0;
        juce::Optional<double> songPosition;

        if (auto* host = getPlayHead())
            if (auto position = host->getPosition())
            {
                if (auto hostBpm = position->getBpm())
                    bpm = *hostBpm;
                if (position->getIsPlaying())
                    songPosition = position->getPpqPosition();
            }

        const int division = juce::jlimit (0, (int) Lfo::divisions.size() - 1,
                                           (int) parameters.getRawParameterValue ("lfoDivision")->load());
        const double beatsPerCycle = Lfo::divisions[(size_t) division].beats;
        cyclesPerSample = bpm / 60.0 / beatsPerCycle / sampleRate;

        if (songPosition)
            lfoCycles = *songPosition / beatsPerCycle;
    }
    else
    {
        cyclesPerSample = parameters.getRawParameterValue ("lfoRate")->load() / sampleRate;
    }

    lfoClock.cyclesAtBlockStart = lfoCycles;
    lfoClock.cyclesPerSample = cyclesPerSample;

    // Moved on now, ready for the next block. Kept small so it never loses precision.
    lfoCycles = std::fmod (lfoCycles + (double) numSamples * cyclesPerSample, 1048576.0);
}

void HelixAudioProcessor::publishLiveState()
{
    HelixVoice* newest = nullptr;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<HelixVoice*> (synth.getVoice (i)))
            if (voice->isVoiceActive() && (newest == nullptr || voice->getStartOrder() > newest->getStartOrder()))
                newest = voice;

    live.cutoff.store (newest != nullptr ? newest->getCurrentCutoff() : -1.0f);
    live.position.store (newest != nullptr ? newest->getCurrentPosition() : -1.0f);
    live.lfoCycles.store (newest != nullptr ? newest->getLfoCycles() : lfoClock.cyclesAtBlockStart);
}

bool HelixAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void HelixAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    // Marked before reading the table, so the interface never frees a table this block is using.
    inBlock.store (true);
    blockTable = audioTable.load();

    advanceLfoClock (buffer.getNumSamples());
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
    publishLiveState();
    buffer.applyGain (parameters.getRawParameterValue ("volume")->load());

    inBlock.store (false);
}

//==============================================================================
// Wavetables

std::shared_ptr<const Wavetable> HelixAudioProcessor::getTable() const
{
    const juce::ScopedLock lock (tableLock);
    return table;
}

TableChoice HelixAudioProcessor::getTableChoice() const
{
    const juce::ScopedLock lock (tableLock);
    return tableChoice;
}

void HelixAudioProcessor::selectFactoryTable (int index)
{
    index = juce::jlimit (0, (int) FactoryTables::list.size() - 1, index);

    TableChoice choice;
    choice.kind = TableChoice::Kind::factory;
    choice.factoryIndex = index;
    choice.name = FactoryTables::list[(size_t) index].name;
    installTable (std::make_shared<const Wavetable> (FactoryTables::make (index)), std::move (choice));
}

bool HelixAudioProcessor::selectTableFile (const juce::File& file)
{
    auto frames = WavetableFile::load (file);
    if (frames.empty())
        return false;

    TableChoice choice;
    choice.kind = TableChoice::Kind::file;
    choice.file = file;
    choice.name = file.getFileNameWithoutExtension();
    installTable (std::make_shared<const Wavetable> (frames), std::move (choice));
    return true;
}

void HelixAudioProcessor::setDrawnTable (std::vector<Wavetable::Frame> keyframes, const juce::String& name, juce::uint32 editorTag)
{
    if (keyframes.empty())
        return;

    auto newTable = std::make_shared<const Wavetable> (keyframes);

    TableChoice choice;
    choice.kind = TableChoice::Kind::drawn;
    choice.name = name;
    choice.keyframes = std::move (keyframes);
    choice.editorTag = editorTag;
    installTable (std::move (newTable), std::move (choice));
}

void HelixAudioProcessor::renameDrawnTable (const juce::String& name)
{
    {
        const juce::ScopedLock lock (tableLock);
        if (tableChoice.kind != TableChoice::Kind::drawn)
            return;
        tableChoice.name = name;
        lastDrawing.name = name;
    }
    sendChangeMessage();
}

TableChoice HelixAudioProcessor::getLastDrawing() const
{
    const juce::ScopedLock lock (tableLock);
    return lastDrawing;
}

void HelixAudioProcessor::installTable (std::shared_ptr<const Wavetable> newTable, TableChoice newChoice)
{
    {
        const juce::ScopedLock lock (tableLock);
        if (table != nullptr)
            retired.push_back (table);

        table = std::move (newTable);
        tableChoice = std::move (newChoice);
        if (tableChoice.kind == TableChoice::Kind::drawn)
            lastDrawing = tableChoice;
        audioTable.store (table.get());
    }
    sendChangeMessage();
}

void HelixAudioProcessor::timerCallback()
{
    // If the audio thread is not inside a block now, it can't still hold an old table:
    // the next block will read the new one.
    const juce::ScopedLock lock (tableLock);
    if (! retired.empty() && ! inBlock.load())
        retired.clear();
}

juce::AudioProcessorEditor* HelixAudioProcessor::createEditor()
{
    return new HelixAudioProcessorEditor (*this);
}

// The knobs are saved by the parameter tree. The wavetable is saved alongside it:
// built-in and file tables by name or path, drawn tables with the drawings themselves.
void HelixAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.removeChild (state.getChildWithName ("Wavetable"), nullptr);

    const auto choice = getTableChoice();
    const auto currentTable = getTable();
    juce::ValueTree tableTree ("Wavetable");
    tableTree.setProperty ("name", choice.name, nullptr);

    // Drawn and file tables also store the waves themselves, so the project still sounds
    // the same if the file is moved or opened on another computer.
    auto storeFrames = [&tableTree] (int numFrames, const std::function<const float* (int)>& frameData)
    {
        juce::MemoryBlock samples;
        for (int f = 0; f < numFrames; ++f)
            samples.append (frameData (f), (size_t) Wavetable::tableSize * sizeof (float));
        tableTree.setProperty ("frames", numFrames, nullptr);
        tableTree.setProperty ("data", samples.toBase64Encoding(), nullptr);
    };

    switch (choice.kind)
    {
        case TableChoice::Kind::factory:
            tableTree.setProperty ("kind", "factory", nullptr);
            tableTree.setProperty ("index", choice.factoryIndex, nullptr);
            break;

        case TableChoice::Kind::file:
            tableTree.setProperty ("kind", "file", nullptr);
            tableTree.setProperty ("path", choice.file.getFullPathName(), nullptr);
            storeFrames (currentTable->getNumFrames(), [&currentTable] (int f) { return currentTable->getFrame (f); });
            break;

        case TableChoice::Kind::drawn:
            tableTree.setProperty ("kind", "drawn", nullptr);
            storeFrames ((int) choice.keyframes.size(), [&choice] (int f) { return choice.keyframes[(size_t) f].data(); });
            break;
    }
    state.appendChild (tableTree, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HelixAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    const auto tableTree = state.getChildWithName ("Wavetable");
    state.removeChild (tableTree, nullptr);
    parameters.replaceState (state);

    if (! tableTree.isValid())
    {
        selectFactoryTable (0); // saved by Helix v0.3 or earlier, which only had the basic shapes
        return;
    }

    // The stored waves, or none if they are missing or damaged.
    auto storedFrames = [&tableTree]
    {
        std::vector<Wavetable::Frame> frames;
        juce::MemoryBlock samples;
        const int numFrames = juce::jlimit (0, Wavetable::maxFrames, (int) tableTree["frames"]);
        if (samples.fromBase64Encoding (tableTree["data"].toString())
            && numFrames > 0 && samples.getSize() == (size_t) numFrames * Wavetable::tableSize * sizeof (float))
        {
            frames.resize ((size_t) numFrames);
            auto* source = static_cast<const float*> (samples.getData());
            for (int f = 0; f < numFrames; ++f)
                frames[(size_t) f].assign (source + f * Wavetable::tableSize, source + (f + 1) * Wavetable::tableSize);
        }
        return frames;
    };

    const auto kind = tableTree["kind"].toString();
    if (kind == "drawn")
    {
        auto frames = storedFrames();
        if (! frames.empty())
        {
            setDrawnTable (std::move (frames), tableTree["name"].toString());
            return;
        }
    }
    else if (kind == "file")
    {
        const juce::File file (tableTree["path"].toString());
        if (selectTableFile (file))
            return;

        // The file has gone: use the copy stored in the project.
        auto frames = storedFrames();
        if (! frames.empty())
        {
            TableChoice choice;
            choice.kind = TableChoice::Kind::file;
            choice.file = file;
            choice.name = tableTree["name"].toString();
            installTable (std::make_shared<const Wavetable> (frames), std::move (choice));
            return;
        }
    }
    else
    {
        selectFactoryTable ((int) tableTree["index"]);
        return;
    }

    selectFactoryTable (0); // nothing usable was saved
}

// This creates the plugin when the host (FL Studio) loads it.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HelixAudioProcessor();
}
