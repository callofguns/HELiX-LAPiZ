#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "../Wavetable.h"

// The big area on the Waves tab where you draw one wave with the mouse.
//   Draw tool: drag to paint the wave freehand.
//   Line tool: press, drag and let go to draw a straight line.
//   Grid: snaps to steps across and levels up and down, for blocky, chiptune-like waves.
class WaveCanvas : public juce::Component
{
public:
    enum class Tool { draw, line };

    std::function<void()> onStrokeStart; // just before a change (used to save an undo step)
    std::function<void()> onChange;      // while drawing
    std::function<void()> onStrokeEnd;  // after the mouse is let go

    // The wave to draw on, or nullptr to show readOnlyFrame with a message instead.
    void setFrame (Wavetable::Frame* frameToEdit)
    {
        frame = frameToEdit;
        repaint();
    }

    void setReadOnly (const float* frameToShow, const juce::String& message)
    {
        frame = nullptr;
        readOnlyFrame = frameToShow;
        readOnlyMessage = message;
        repaint();
    }

    void setTool (Tool t)     { tool = t; }
    void setGrid (int cells)  { gridCells = cells; repaint(); } // 0 = off

    void paint (juce::Graphics& g) override
    {
        const auto& p = Theme::get();
        g.setColour (p.well);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        const auto area = drawArea();

        // Grid (or simple guides when the grid is off)
        g.setColour (p.edge.withAlpha (0.6f));
        const int columns = gridCells > 0 ? gridCells : 4;
        const int rows = gridCells > 0 ? gridCells : 2;
        for (int c = 1; c < columns; ++c)
            g.drawVerticalLine ((int) (area.getX() + area.getWidth() * (float) c / (float) columns), area.getY(), area.getBottom());
        for (int r = 1; r < rows; ++r)
            g.drawHorizontalLine ((int) (area.getY() + area.getHeight() * (float) r / (float) rows), area.getX(), area.getRight());

        g.setColour (p.edge);
        g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

        const float* samples = frame != nullptr ? frame->data() : readOnlyFrame;
        if (samples != nullptr)
        {
            juce::Path wave;
            const int width = juce::jmax (2, (int) area.getWidth());
            for (int x = 0; x <= width; ++x)
            {
                const int i = juce::jmin (Wavetable::tableSize - 1, x * Wavetable::tableSize / width);
                const float px = area.getX() + (float) x;
                const float py = yFor (juce::jlimit (-1.0f, 1.0f, samples[i]));
                if (x == 0) wave.startNewSubPath (px, py);
                else        wave.lineTo (px, py);
            }

            g.setColour (frame != nullptr ? p.sound : p.soft);
            g.strokePath (wave, juce::PathStrokeType (2.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }

        // The straight line being drawn
        if (lineInProgress)
        {
            juce::Path line;
            line.startNewSubPath (xFor (lineStart.first), yFor (lineStart.second));
            line.lineTo (xFor (lineEnd.first), yFor (lineEnd.second));
            juce::Path dashed;
            const float dashes[] = { 6.0f, 4.0f };
            juce::PathStrokeType (1.5f).createDashedStroke (dashed, line, dashes, 2);
            g.setColour (p.ink);
            g.fillPath (dashed);
        }

        if (frame == nullptr && readOnlyMessage.isNotEmpty())
        {
            auto box = getLocalBounds().toFloat().withSizeKeepingCentre (juce::jmin (440.0f, getWidth() - 40.0f), 62.0f);
            g.setColour (p.panel.withAlpha (0.94f));
            g.fillRoundedRectangle (box, 8.0f);
            g.setColour (p.edge);
            g.drawRoundedRectangle (box, 8.0f, 1.0f);
            g.setColour (p.ink);
            g.setFont (Theme::regular (14.0f));
            g.drawFittedText (readOnlyMessage, box.reduced (14.0f, 6.0f).toNearestInt(), juce::Justification::centred, 3);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (frame == nullptr)
            return;

        if (onStrokeStart) onStrokeStart();

        const auto point = pointAt (e.position);
        if (tool == Tool::line)
        {
            lineInProgress = true;
            lineStart = lineEnd = point;
            repaint();
            return;
        }

        last = point;
        writeSegment (point, point);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (frame == nullptr)
            return;

        const auto point = pointAt (e.position);
        if (tool == Tool::line)
        {
            lineEnd = point;
            repaint();
            return;
        }

        writeSegment (last, point);
        last = point;
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (frame == nullptr)
            return;

        if (lineInProgress)
        {
            lineInProgress = false;
            writeSegment (lineStart, lineEnd);
        }

        if (onStrokeEnd) onStrokeEnd();
    }

private:
    using Point = std::pair<float, float>; // (sample position 0..tableSize, value -1..1)

    juce::Rectangle<float> drawArea() const { return getLocalBounds().toFloat().reduced (10.0f, 14.0f); }

    float xFor (float sample) const { const auto a = drawArea(); return a.getX() + a.getWidth() * sample / (float) Wavetable::tableSize; }
    float yFor (float value) const  { const auto a = drawArea(); return a.getCentreY() - value * a.getHeight() * 0.5f; }

    Point pointAt (juce::Point<float> pos) const
    {
        const auto a = drawArea();
        const float sample = juce::jlimit (0.0f, (float) Wavetable::tableSize - 1.0f,
                                           (pos.x - a.getX()) / a.getWidth() * (float) Wavetable::tableSize);
        const float value = juce::jlimit (-1.0f, 1.0f, (a.getCentreY() - pos.y) / (a.getHeight() * 0.5f));
        return { sample, value };
    }

    float snapValue (float v) const
    {
        const float step = 2.0f / (float) gridCells;
        return juce::jlimit (-1.0f, 1.0f, std::round (v / step) * step);
    }

    // Sets the wave along a line between two points (in sample/value units).
    void writeSegment (Point from, Point to)
    {
        auto& f = *frame;
        constexpr int size = Wavetable::tableSize;

        if (gridCells > 0)
        {
            // Each grid column becomes one flat step at a snapped level.
            const int cellSize = size / gridCells;
            const int c0 = (int) from.first / cellSize, c1 = (int) to.first / cellSize;
            for (int c = juce::jmin (c0, c1); c <= juce::jmax (c0, c1); ++c)
            {
                const float t = c1 == c0 ? 1.0f : (float) (c - c0) / (float) (c1 - c0);
                const float v = snapValue (from.second + t * (to.second - from.second));
                std::fill (f.begin() + c * cellSize, f.begin() + (c + 1) * cellSize, v);
            }
        }
        else
        {
            const int i0 = (int) from.first, i1 = (int) to.first;
            for (int i = juce::jmin (i0, i1); i <= juce::jmax (i0, i1); ++i)
            {
                const float t = i1 == i0 ? 1.0f : (float) (i - i0) / (float) (i1 - i0);
                f[(size_t) i] = from.second + t * (to.second - from.second);
            }
        }

        repaint();
        if (onChange) onChange();
    }

    Wavetable::Frame* frame = nullptr;
    const float* readOnlyFrame = nullptr;
    juce::String readOnlyMessage;

    Tool tool = Tool::draw;
    int gridCells = 0;

    Point last { 0.0f, 0.0f };
    bool lineInProgress = false;
    Point lineStart { 0.0f, 0.0f }, lineEnd { 0.0f, 0.0f };
};
