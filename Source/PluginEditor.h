/*
  ==============================================================================
    SineSynth - PluginEditor.h
    Layout: large Oscillator panel (left, main focus) + compact Envelope (right)
    Unison/Detune lives inside the Oscillator panel.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Themed rotary knob
class SynthKnob : public juce::Component
{
public:
    juce::Slider slider;
    juce::Label  label;

    SynthKnob (const juce::String& labelText,
               double rangeMin, double rangeMax,
               double defaultVal,
               const juce::String& suffix = "")
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 14);
        slider.setRange (rangeMin, rangeMax);
        slider.setValue (defaultVal, juce::dontSendNotification);
        slider.setTextValueSuffix (suffix);

        slider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xff00e5ff));
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff1a3a4a));
        slider.setColour (juce::Slider::thumbColourId,               juce::Colour (0xff00e5ff));
        slider.setColour (juce::Slider::textBoxTextColourId,         juce::Colour (0xffaaeeff));
        slider.setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colour (0xff0d1f2d));
        slider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colour (0x00000000));

        label.setText (labelText, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff4fc3f7));
        label.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (slider);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds  (b.removeFromBottom (14));
        slider.setBounds (b);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthKnob)
};

//==============================================================================
// Waveform selector with inline SVG-drawn previews
class WaveformSelector : public juce::Component
{
public:
    // 0=Sine, 1=Saw, 2=Triangle, 3=Square
    int selectedWave = 0;
    std::function<void(int)> onChange;

    WaveformSelector() = default;

    void paint (juce::Graphics& g) override
    {
        const int n = 4;
        float itemW = (float)getWidth() / (float)n;
        float h     = (float)getHeight();

        for (int i = 0; i < n; ++i)
        {
            float x = (float)i * itemW;
            bool  sel = (i == selectedWave);

            // Tile background
            juce::Colour bgCol = sel ? juce::Colour (0xff0d2535)
                                     : juce::Colour (0xff060f18);
            g.setColour (bgCol);
            g.fillRoundedRectangle (x + 2.0f, 2.0f, itemW - 4.0f, h - 4.0f, 5.0f);

            // Border
            g.setColour (sel ? juce::Colour (0xff00e5ff) : juce::Colour (0xff1a3a4a));
            g.drawRoundedRectangle (x + 2.0f, 2.0f, itemW - 4.0f, h - 4.0f, 5.0f, sel ? 1.5f : 1.0f);

            // Wave preview
            drawWavePreview (g, i, x + 4.0f, 6.0f, itemW - 8.0f, h * 0.58f, sel);

            // Label
            juce::String names[] = { "SINE", "SAW", "TRI", "SQR" };
            g.setFont (juce::Font (juce::FontOptions (8.5f).withStyle ("Bold")));
            g.setColour (sel ? juce::Colour (0xff00e5ff) : juce::Colour (0xff4a7a8a));
            g.drawText (names[i], (int)(x), (int)(h - 18.0f), (int)itemW, 16,
                        juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        int n = 4;
        float itemW = (float)getWidth() / (float)n;
        int clicked = juce::jlimit (0, n - 1, (int)(e.position.x / itemW));
        if (clicked != selectedWave)
        {
            selectedWave = clicked;
            repaint();
            if (onChange) onChange (selectedWave);
        }
    }

private:
    void drawWavePreview (juce::Graphics& g, int type,
                          float x, float y, float w, float h, bool selected)
    {
        juce::Path p;
        float cy = y + h * 0.5f;
        float amp = h * 0.42f;
        const int steps = 80;

        for (int s = 0; s <= steps; ++s)
        {
            float t    = (float)s / (float)steps;
            float px   = x + t * w;
            float phase = t * juce::MathConstants<float>::twoPi * 1.5f; // 1.5 cycles
            float val  = 0.0f;

            switch (type)
            {
                case 0: // Sine
                    val = std::sin (phase);
                    break;
                case 1: // Saw
                    val = 1.0f - (std::fmod (phase, juce::MathConstants<float>::twoPi)
                                  / juce::MathConstants<float>::pi);
                    break;
                case 2: // Triangle
                {
                    float norm = std::fmod (phase, juce::MathConstants<float>::twoPi)
                                 / juce::MathConstants<float>::twoPi;
                    val = (norm < 0.5f) ? (4.0f * norm - 1.0f) : (3.0f - 4.0f * norm);
                    break;
                }
                case 3: // Square
                    val = (std::fmod (phase, juce::MathConstants<float>::twoPi)
                           < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
                    break;
            }

            float py = cy - val * amp;
            if (s == 0) p.startNewSubPath (px, py);
            else        p.lineTo (px, py);
        }

        g.setColour (selected ? juce::Colour (0xff00e5ff)
                              : juce::Colour (0xff2a6a7a));
        g.strokePath (p, juce::PathStrokeType (selected ? 1.5f : 1.0f));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformSelector)
};

//==============================================================================
// Compact ADSR display
class ADSRDisplay : public juce::Component
{
public:
    ADSRDisplay() = default;

    float attack  = 0.01f;
    float decay   = 0.1f;
    float sustain = 0.8f;
    float release = 0.3f;

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);

        g.setColour (juce::Colour (0xff060f18));
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (juce::Colour (0xff1a3a4a));
        g.drawRoundedRectangle (b, 5.0f, 1.0f);

        // Faint grid
        g.setColour (juce::Colour (0x0a4fc3f7));
        for (int i = 1; i < 3; ++i)
            g.drawHorizontalLine ((int)(b.getY() + b.getHeight() * i / 3),
                                  b.getX(), b.getRight());

        const float pad = 5.0f;
        const float w   = b.getWidth()  - pad * 2.0f;
        const float h   = b.getHeight() - pad * 2.0f;
        const float ox  = b.getX() + pad;
        const float oy  = b.getY() + pad;
        const float bot = oy + h;

        const float sw        = 0.22f;
        const float totalTime = attack + decay + sw + release;
        auto xOf = [&](float t) { return ox + (t / totalTime) * w; };

        float xA  = xOf (attack);
        float xD  = xOf (attack + decay);
        float xS  = xOf (attack + decay + sw);
        float yTop = oy;
        float yS   = oy + h * (1.0f - sustain);

        // Fill
        juce::Path fill;
        fill.startNewSubPath (ox, bot);
        fill.lineTo (xA, yTop);
        fill.lineTo (xD, yS);
        fill.lineTo (xS, yS);
        fill.lineTo (xOf (totalTime), bot);
        fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0x3800e5ff), ox, yTop,
                                                  juce::Colour (0x0600e5ff), ox, bot, false));
        g.fillPath (fill);

        // Line
        juce::Path line;
        line.startNewSubPath (ox, bot);
        line.lineTo (xA, yTop);
        line.lineTo (xD, yS);
        line.lineTo (xS, yS);
        line.lineTo (xOf (totalTime), bot);
        g.setColour (juce::Colour (0xff00e5ff));
        g.strokePath (line, juce::PathStrokeType (1.5f));

        // Segment labels
        g.setFont (juce::Font (juce::FontOptions (7.5f).withStyle ("Bold")));
        g.setColour (juce::Colour (0xff4fc3f7));
        auto lbl = [&](const juce::String& t, float x1, float x2, float labelY)
        {
            g.drawText (t, (int)((x1+x2)*0.5f - 10), (int)labelY, 20, 9,
                        juce::Justification::centred);
        };
        lbl ("A", ox, xA, yTop);
        lbl ("D", xA, xD, yTop);
        lbl ("S", xD, xS, yS - 9.0f);
        lbl ("R", xS, xOf(totalTime), yS - 9.0f);

        // Breakpoint dots
        auto dot = [&](float dx, float dy)
        {
            g.setColour (juce::Colour (0xff060f18));
            g.fillEllipse (dx - 3.5f, dy - 3.5f, 7.0f, 7.0f);
            g.setColour (juce::Colour (0xff00e5ff));
            g.drawEllipse (dx - 3.5f, dy - 3.5f, 7.0f, 7.0f, 1.2f);
        };
        dot (xA, yTop);
        dot (xD, yS);
        dot (xS, yS);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSRDisplay)
};

//==============================================================================
// Piano component with white + black keys, 2 octaves by default.
// Click/drag supports playing notes using the processor's GUI-safe handlers.
class PianoComponent : public juce::Component
{
public:
    PianoComponent (NewProjectAudioProcessor& p)
        : processor (p)
    {
        expanded = false;
        collapsedHeight = 26;
        expandedHeight = 160;

        // default two octaves starting at C3 (midi 48)
        firstMidiNote = 48;
        octaves = 2;

        buildNotes();
    }

    std::function<void()> onToggle; // editor hook to call resized()

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();

        // header (toggle area)
        auto header = r.removeFromTop (collapsedHeight);
        g.setColour (juce::Colour (0xff0f0f0f));
        g.fillRect (header);

        g.setColour (juce::Colour (0xffffffff));
        g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        g.drawText (expanded ? "Piano — Click keys to play (click header to collapse)" : "Piano — Click to expand",
                    header.reduced (8, 2), juce::Justification::centredLeft, true);

        // small arrow indicator on right
        juce::Path p;
        float cx = (float)getWidth() - 16.0f;
        float cy = (float)header.getCentreY();
        if (expanded)
        {
            p.startNewSubPath (cx - 5.0f, cy + 3.0f);
            p.lineTo (cx + 5.0f, cy + 3.0f);
            p.lineTo (cx, cy - 3.0f);
            p.closeSubPath();
        }
        else
        {
            p.startNewSubPath (cx - 3.0f, cy - 5.0f);
            p.lineTo (cx - 3.0f, cy + 5.0f);
            p.lineTo (cx + 3.0f, cy);
            p.closeSubPath();
        }
        g.setColour (juce::Colour (0xffcfcfcf));
        g.fillPath (p);

        if (!expanded) return;

        // draw key background
        const auto keyArea = getLocalBounds().removeFromTop (expandedHeight).withTop (collapsedHeight);
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillRect (keyArea);

        // draw white keys (use temp rect for separator so we don't mutate key areas)
        for (size_t i = 0; i < whiteKeys.size(); ++i)
        {
            auto& k = whiteKeys[i];
            g.setColour (k.isDown ? juce::Colour (0xffdcdcdc) : juce::Colour (0xffffffff));
            g.fillRect (k.area);

            // separator - do not call removeFromRight() on k.area (that mutates it)
            juce::Rectangle<int> sep = k.area;
            sep.setX (sep.getRight() - 1);
            sep.setWidth (1);
            g.setColour (juce::Colour (0xff2a2a2a));
            g.fillRect (sep);
        }

        // draw black keys on top
        for (size_t i = 0; i < blackKeys.size(); ++i)
        {
            auto& b = blackKeys[i];
            g.setColour (b.isDown ? juce::Colour (0xff4f4f4f) : juce::Colour (0xff0b0b0b));
            g.fillRect (b.area);
        }
    }

    void resized() override
    {
        buildKeyAreas();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.position.y <= (float)collapsedHeight)
        {
            expanded = !expanded;
            if (onToggle) onToggle();
            return;
        }

        if (!expanded) return;

        // check black keys first (they sit above white keys)
        int bidx = findBlackKeyIndexAtPosition (e.position);
        if (bidx >= 0)
        {
            pressBlackKey (bidx);
            return;
        }

        int widx = findWhiteKeyIndexAtPosition (e.position);
        if (widx >= 0)
        {
            pressWhiteKey (widx);
            return;
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (!expanded) return;
        releaseAllPressedKeys();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!expanded) return;

        int bidx = findBlackKeyIndexAtPosition (e.position);
        if (bidx >= 0)
        {
            if (lastPressedIsBlack != true || lastPressedIndex != bidx)
            {
                releaseAllPressedKeys();
                pressBlackKey (bidx);
            }
            return;
        }

        int widx = findWhiteKeyIndexAtPosition (e.position);
        if (widx >= 0)
        {
            if (lastPressedIsBlack != false || lastPressedIndex != widx)
            {
                releaseAllPressedKeys();
                pressWhiteKey (widx);
            }
            return;
        }

        // if dragged outside keys, release
        releaseAllPressedKeys();
    }

    int getPreferredHeight() const noexcept { return expanded ? expandedHeight : collapsedHeight; }

    bool expanded = false;
    int collapsedHeight = 26;
    int expandedHeight = 160;

private:
    struct WhiteKey { int midiNote; bool isDown; juce::Rectangle<int> area; };
    struct BlackKey { int midiNote; bool isDown; juce::Rectangle<int> area; };

    NewProjectAudioProcessor& processor;
    int firstMidiNote;
    int octaves;
    std::vector<WhiteKey> whiteKeys;
    std::vector<BlackKey> blackKeys;

    int lastPressedIndex = -1;
    bool lastPressedIsBlack = false;

    void buildNotes()
    {
        // White key offsets in an octave relative to C
        static const int whiteOffsetsInOctave[7] = { 0, 2, 4, 5, 7, 9, 11 }; // C D E F G A B
        whiteKeys.clear();
        blackKeys.clear();

        for (int o = 0; o < octaves; ++o)
        {
            for (int wi = 0; wi < 7; ++wi)
            {
                int midi = firstMidiNote + o * 12 + whiteOffsetsInOctave[wi];
                whiteKeys.push_back ({ midi, false, {} });

                // Black key mapping logic
                int base = firstMidiNote + o*12;
                int blackMidi = -1;
                switch (wi)
                {
                    case 0: blackMidi = base + 1; break; // C#
                    case 1: blackMidi = base + 3; break; // D#
                    case 2: blackMidi = -1;       break; // no black between E-F
                    case 3: blackMidi = base + 6; break; // F#
                    case 4: blackMidi = base + 8; break; // G#
                    case 5: blackMidi = base + 10;break; // A#
                    case 6: blackMidi = -1;       break;
                }
                if (blackMidi >= 0)
                    blackKeys.push_back ({ blackMidi, false, {} });
            }
        }

        buildKeyAreas();
    }

    void buildKeyAreas()
    {
        auto r = getLocalBounds();
        r.removeFromTop (collapsedHeight); // header taken
        if (!expanded) return;

        const int totalWhite = (int)whiteKeys.size();
        if (totalWhite == 0) return;

        constexpr int minWhiteKeyWidth = 18;

        // compute key width with a lower bound, and center if there's spare space
        int availableW = getWidth();
        int keyW = std::max(minWhiteKeyWidth, availableW / totalWhite);
        int totalW = keyW * totalWhite;
        int startX = (totalW <= availableW) ? ((availableW - totalW) / 2) : 0;

        int h = getHeight() - collapsedHeight;

        for (int i = 0; i < totalWhite; ++i)
        {
            int x = startX + i * keyW;
            int w = keyW;
            if (x + w > availableW) w = availableW - x;
            if (w <= 0) w = 1;
            whiteKeys[i].area = juce::Rectangle<int> (x, collapsedHeight, w, h);
        }

        // position black keys: narrower and shorter, centered between adjacent white keys
        int bi = 0;
        for (int o = 0; o < octaves; ++o)
        {
            for (int wi = 0; wi < 7; ++wi)
            {
                int base = firstMidiNote + o*12;
                int blackMidi = -1;
                switch (wi)
                {
                    case 0: blackMidi = base + 1; break;
                    case 1: blackMidi = base + 3; break;
                    case 2: blackMidi = -1;       break;
                    case 3: blackMidi = base + 6; break;
                    case 4: blackMidi = base + 8; break;
                    case 5: blackMidi = base + 10;break;
                    case 6: blackMidi = -1;       break;
                }
                if (blackMidi < 0) continue;
                int globalWhiteIndex = o*7 + wi;
                if (globalWhiteIndex < 0 || globalWhiteIndex+1 >= (int)whiteKeys.size()) continue;
                auto left = whiteKeys[globalWhiteIndex].area;
                auto right = whiteKeys[globalWhiteIndex+1].area;

                // center between the right edge of left and left edge of right
                int gapL = left.getRight();
                int gapR = right.getX();
                int centerX = (gapL + gapR) / 2;

                int bkW = std::max( (int)(keyW * 0.62f), 12 );
                int bkH = (int)std::round( h * 0.62f );
                int bx = centerX - bkW/2;
                // clamp to component bounds
                if (bx < 0) bx = 0;
                if (bx + bkW > availableW) bx = std::max(0, availableW - bkW);

                if (bi < (int)blackKeys.size())
                    blackKeys[bi].area = juce::Rectangle<int> (bx, collapsedHeight, bkW, bkH);
                ++bi;
            }
        }
    }

    int findWhiteKeyIndexAtPosition (juce::Point<float> p)
    {
        for (size_t i = 0; i < whiteKeys.size(); ++i)
            if (whiteKeys[i].area.contains ((int)p.x, (int)p.y))
                return (int)i;
        return -1;
    }

    int findBlackKeyIndexAtPosition (juce::Point<float> p)
    {
        for (size_t i = 0; i < blackKeys.size(); ++i)
            if (blackKeys[i].area.contains ((int)p.x, (int)p.y))
                return (int)i;
        return -1;
    }

    void pressWhiteKey (int idx)
    {
        if (idx < 0 || idx >= (int)whiteKeys.size()) return;
        releaseAllPressedKeys();
        whiteKeys[idx].isDown = true;
        lastPressedIndex = idx;
        lastPressedIsBlack = false;
        repaint (whiteKeys[idx].area);
        processor.handleGuiNoteOn (whiteKeys[idx].midiNote, 1.0f);
    }

    void pressBlackKey (int idx)
    {
        if (idx < 0 || idx >= (int)blackKeys.size()) return;
        releaseAllPressedKeys();
        blackKeys[idx].isDown = true;
        lastPressedIndex = idx;
        lastPressedIsBlack = true;
        repaint (blackKeys[idx].area);
        processor.handleGuiNoteOn (blackKeys[idx].midiNote, 1.0f);
    }

    void releaseAllPressedKeys()
    {
        if (lastPressedIndex < 0) return;

        if (lastPressedIsBlack)
        {
            if (lastPressedIndex >= 0 && lastPressedIndex < (int)blackKeys.size())
            {
                int midi = blackKeys[lastPressedIndex].midiNote;
                blackKeys[lastPressedIndex].isDown = false;
                repaint (blackKeys[lastPressedIndex].area);
                processor.handleGuiNoteOff (midi);
            }
        }
        else
        {
            if (lastPressedIndex >= 0 && lastPressedIndex < (int)whiteKeys.size())
            {
                int midi = whiteKeys[lastPressedIndex].midiNote;
                whiteKeys[lastPressedIndex].isDown = false;
                repaint (whiteKeys[lastPressedIndex].area);
                processor.handleGuiNoteOff (midi);
            }
        }

        lastPressedIndex = -1;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoComponent)
};

//==============================================================================
// Editor class
class NewProjectAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override;
    void syncSlidersToParams();
    void updateADSRDisplay();

    NewProjectAudioProcessor& audioProcessor;

    // Waveform selector (custom tiles with previews)
    WaveformSelector waveSelector;

    // Unison + detune (inside oscillator panel)
    SynthKnob knobUnison { "UNISON", 1.0, 8.0,  1.0       };
    SynthKnob knobDetune { "DETUNE", 0.0, 100.0, 0.0, "ct" };

    // Volume (inside oscillator panel)
    SynthKnob knobVolume { "VOLUME", 0.0, 1.0, 0.7 };

    // ADSR knobs (compact, right panel)
    SynthKnob knobAttack  { "ATK",  0.001, 4.0, 0.01, "s" };
    SynthKnob knobDecay   { "DCY",  0.001, 4.0, 0.1,  "s" };
    SynthKnob knobSustain { "SUS",  0.0,   1.0, 0.8       };
    SynthKnob knobRelease { "REL",  0.001, 8.0, 0.3,  "s" };

    ADSRDisplay adsrDisplay;

    // Piano preview component (bottom, extendable)
    PianoComponent piano { audioProcessor };

    int displayedVoiceCount = 0;
    // The editor height when the piano is collapsed. Used to expand
    // the window downward when the piano opens.
    int baseEditorHeight = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
