#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "Wavetable.h"

// Reading and writing wavetables as .wav files, in the common layout Serum and most
// other wavetable synths use: frames of 2048 samples one after another in a mono file.
namespace WavetableFile
{
    // Where LAPiZ keeps your saved wavetables. They show up in the wavetable list.
    inline juce::File folder()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("LAPiZ").getChildFile ("Wavetables");
    }

    inline juce::Array<juce::File> listUserFiles()
    {
        auto files = folder().findChildFiles (juce::File::findFiles, false, "*.wav");
        files.sort();
        return files;
    }

    // Returns no frames if the file can't be read.
    // A file shorter than one frame is treated as a single cycle and stretched to fit.
    inline std::vector<Wavetable::Frame> load (const juce::File& file)
    {
        juce::WavAudioFormat wav;
        auto stream = file.createInputStream();
        if (stream == nullptr)
            return {};

        std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (stream.release(), true));
        if (reader == nullptr || reader->lengthInSamples <= 0)
            return {};

        constexpr int size = Wavetable::tableSize;
        const auto length = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) size * Wavetable::maxFrames);
        juce::AudioBuffer<float> buffer (1, length);
        reader->read (&buffer, 0, length, 0, true, false);
        const float* samples = buffer.getReadPointer (0);

        std::vector<Wavetable::Frame> frames;
        if (length < size)
        {
            Wavetable::Frame frame ((size_t) size);
            for (int i = 0; i < size; ++i)
            {
                const float pos = (float) i * (float) length / (float) size;
                const int a = (int) pos, b = (a + 1) % length;
                frame[(size_t) i] = samples[a] + (pos - (float) a) * (samples[b] - samples[a]);
            }
            frames.push_back (std::move (frame));
        }
        else
        {
            for (int start = 0; start + size <= length; start += size)
                frames.emplace_back (samples + start, samples + start + size);
        }
        return frames;
    }

    // Writes the frames as a 32-bit float mono .wav. Returns false if it couldn't be written.
    inline bool save (const juce::File& file, const std::vector<Wavetable::Frame>& frames)
    {
        if (frames.empty())
            return false;

        file.getParentDirectory().createDirectory();
        file.deleteFile();

        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
        if (static_cast<juce::FileOutputStream*> (stream.get())->failedToOpen())
            return false;

        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                       .withSampleRate (44100.0)
                                                       .withNumChannels (1)
                                                       .withBitsPerSample (32)
                                                       .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
        if (writer == nullptr)
            return false;

        for (const auto& frame : frames)
        {
            const float* channels[] = { frame.data() };
            if (! writer->writeFromFloatArrays (channels, 1, (int) frame.size()))
                return false;
        }
        return true;
    }
}
