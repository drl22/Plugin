/*
  ==============================================================================
    SineSynth - PluginEditor.cpp
    Layout: large Oscillator panel (left 2/3) + compact Envelope panel (right 1/3)
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (720, 460);

    // ---- Waveform selector ----
    waveSelector.selectedWave = audioProcessor.paramWaveType->get();
    waveSelector.onChange = [this](int w)
    {
        *audioProcessor.paramWaveType = w;
    };
    addAndMakeVisible (waveSelector);

    // ---- Unison ----
    knobUnison.slider.setRange (1.0, 8.0, 1.0);
    knobUnison.slider.onValueChange = [this] {
        *audioProcessor.paramUnison = (int)knobUnison.slider.getValue();
    };
    addAndMakeVisible (knobUnison);

    // ---- Detune ----
    knobDetune.slider.onValueChange = [this] {
        *audioProcessor.paramDetune = (float)knobDetune.slider.getValue();
    };
    addAndMakeVisible (knobDetune);

    // ---- Volume ----
    knobVolume.slider.onValueChange = [this] {
        *audioProcessor.masterVolume = (float)knobVolume.slider.getValue();
    };
    addAndMakeVisible (knobVolume);

    // ---- ADSR knobs ----
    knobAttack.slider.onValueChange = [this] {
        *audioProcessor.paramAttack = (float)knobAttack.slider.getValue();
        updateADSRDisplay();
    };
    knobDecay.slider.onValueChange = [this] {
        *audioProcessor.paramDecay = (float)knobDecay.slider.getValue();
        updateADSRDisplay();
    };
    knobSustain.slider.onValueChange = [this] {
        *audioProcessor.paramSustain = (float)knobSustain.slider.getValue();
        updateADSRDisplay();
    };
    knobRelease.slider.onValueChange = [this] {
        *audioProcessor.paramRelease = (float)knobRelease.slider.getValue();
        updateADSRDisplay();
    };
    addAndMakeVisible (knobAttack);
    addAndMakeVisible (knobDecay);
    addAndMakeVisible (knobSustain);
    addAndMakeVisible (knobRelease);

    // ---- ADSR display ----
    addAndMakeVisible (adsrDisplay);

    syncSlidersToParams();
    startTimerHz (30);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void NewProjectAudioProcessorEditor::syncSlidersToParams()
{
    knobAttack.slider.setValue  (audioProcessor.paramAttack->get(),  juce::dontSendNotification);
    knobDecay.slider.setValue   (audioProcessor.paramDecay->get(),   juce::dontSendNotification);
    knobSustain.slider.setValue (audioProcessor.paramSustain->get(), juce::dontSendNotification);
    knobRelease.slider.setValue (audioProcessor.paramRelease->get(), juce::dontSendNotification);
    knobVolume.slider.setValue  (audioProcessor.masterVolume->get(), juce::dontSendNotification);
    knobUnison.slider.setValue  ((double)audioProcessor.paramUnison->get(), juce::dontSendNotification);
    knobDetune.slider.setValue  (audioProcessor.paramDetune->get(),  juce::dontSendNotification);
    waveSelector.selectedWave = audioProcessor.paramWaveType->get();
    waveSelector.repaint();
    updateADSRDisplay();
}

void NewProjectAudioProcessorEditor::updateADSRDisplay()
{
    adsrDisplay.attack  = (float)knobAttack.slider.getValue();
    adsrDisplay.decay   = (float)knobDecay.slider.getValue();
    adsrDisplay.sustain = (float)knobSustain.slider.getValue();
    adsrDisplay.release = (float)knobRelease.slider.getValue();
    adsrDisplay.repaint();
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    auto syncKnob = [](SynthKnob& k, float val)
    {
        if (std::abs ((float)k.slider.getValue() - val) > 0.0001f)
            k.slider.setValue (val, juce::dontSendNotification);
    };

    syncKnob (knobAttack,  audioProcessor.paramAttack->get());
    syncKnob (knobDecay,   audioProcessor.paramDecay->get());
    syncKnob (knobSustain, audioProcessor.paramSustain->get());
    syncKnob (knobRelease, audioProcessor.paramRelease->get());
    syncKnob (knobVolume,  audioProcessor.masterVolume->get());
    syncKnob (knobUnison,  (float)audioProcessor.paramUnison->get());
    syncKnob (knobDetune,  audioProcessor.paramDetune->get());

    int waveId = audioProcessor.paramWaveType->get();
    if (waveSelector.selectedWave != waveId)
    {
        waveSelector.selectedWave = waveId;
        waveSelector.repaint();
    }

    updateADSRDisplay();

    int voices = audioProcessor.activeVoiceCount.load();
    if (voices != displayedVoiceCount)
    {
        displayedVoiceCount = voices;
        repaint();
    }
}

//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float W = (float)getWidth();
    const float H = (float)getHeight();

    // ---- Background ----
    g.setGradientFill (juce::ColourGradient (
        juce::Colour (0xff070e18), 0.0f, 0.0f,
        juce::Colour (0xff0b1c2c), W, H, false));
    g.fillRect (0.0f, 0.0f, W, H);

    // Dot-grid texture
    g.setColour (juce::Colour (0x0800e5ff));
    for (float x = 0; x < W; x += 24.0f)
        for (float y = 0; y < H; y += 24.0f)
            g.fillEllipse (x, y, 1.5f, 1.5f);

    // ---- Title bar ----
    g.setColour (juce::Colour (0xff040c14));
    g.fillRect (0.0f, 0.0f, W, 44.0f);
    g.setColour (juce::Colour (0xff1a3a4a));
    g.drawHorizontalLine (44, 0.0f, W);

    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::Font (juce::FontOptions (20.0f).withStyle ("Bold")));
    g.drawText ("POLY SYNTH", 16, 7, 220, 28, juce::Justification::left);

    g.setColour (juce::Colour (0xff1e4a5a));
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.drawText ("sine  /  saw  /  triangle  /  square", 16, 28, 300, 13,
                juce::Justification::left);

    // Voice counter — top right of title bar
    {
        const int vcX = (int)W - 160, vcY = 6;
        g.setColour (juce::Colour (0xff0d1f2d));
        g.fillRoundedRectangle ((float)vcX, (float)vcY, 148.0f, 32.0f, 4.0f);
        g.setColour (juce::Colour (0xff1a3a4a));
        g.drawRoundedRectangle ((float)vcX, (float)vcY, 148.0f, 32.0f, 4.0f, 1.0f);

        g.setColour (juce::Colour (0xff2a6a7a));
        g.setFont (juce::Font (juce::FontOptions (8.0f).withStyle ("Bold")));
        g.drawText ("VOICES", vcX + 4, vcY + 3, 44, 11, juce::Justification::left);

        bool hasV = displayedVoiceCount > 0;
        g.setColour (hasV ? juce::Colour (0xff00e5ff) : juce::Colour (0xff1a3a4a));
        g.setFont (juce::Font (juce::FontOptions (18.0f).withStyle ("Bold")));
        g.drawText (juce::String (displayedVoiceCount), vcX + 4, vcY + 10, 36, 20,
                    juce::Justification::left);

        // 16 dot indicators in two rows of 8
        const int dotS = 6, dotSp = 9;
        int dsx = vcX + 48, dsy = vcY + 6;
        for (int i = 0; i < 16; ++i)
        {
            float dx = (float)(dsx + (i % 8) * dotSp);
            float dy = (float)(dsy + (i / 8) * dotSp);
            g.setColour (i < displayedVoiceCount ? juce::Colour (0xff00e5ff)
                                                  : juce::Colour (0xff1a3a4a));
            g.fillEllipse (dx, dy, (float)dotS, (float)dotS);
        }
    }

    // ---- Oscillator panel (left, large) ----
    const float oscPanX = 10.0f, oscPanY = 52.0f;
    const float oscPanW = 470.0f, oscPanH = H - 60.0f;

    g.setColour (juce::Colour (0xff060e18));
    g.fillRoundedRectangle (oscPanX, oscPanY, oscPanW, oscPanH, 8.0f);
    g.setColour (juce::Colour (0xff1a3550));
    g.drawRoundedRectangle (oscPanX, oscPanY, oscPanW, oscPanH, 8.0f, 1.5f);

    // Oscillator panel title strip
    g.setColour (juce::Colour (0xff091525));
    g.fillRoundedRectangle (oscPanX, oscPanY, oscPanW, 26.0f, 8.0f);
    g.setColour (juce::Colour (0xff1a3550));
    g.drawHorizontalLine ((int)(oscPanY + 26), oscPanX, oscPanX + oscPanW);
    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.drawText ("OSCILLATOR", (int)oscPanX + 12, (int)oscPanY + 6, 120, 14,
                juce::Justification::left);

    // Divider between waveform area and unison area
    g.setColour (juce::Colour (0xff132030));
    g.drawHorizontalLine ((int)(oscPanY + 26 + 200), oscPanX + 8, oscPanX + oscPanW - 8);

    // Sub-label: WAVEFORM
    g.setColour (juce::Colour (0xff2a5a6a));
    g.setFont (juce::Font (juce::FontOptions (8.5f).withStyle ("Bold")));
    g.drawText ("WAVEFORM", (int)oscPanX + 12, (int)(oscPanY + 30), 80, 12,
                juce::Justification::left);

    // Sub-label: UNISON
    g.drawText ("UNISON", (int)oscPanX + 12, (int)(oscPanY + 232), 80, 12,
                juce::Justification::left);

    // Sub-label: VOLUME
    g.drawText ("VOLUME", (int)(oscPanX + oscPanW) - 100, (int)(oscPanY + 232), 88, 12,
                juce::Justification::left);

    // ---- Envelope panel (right, compact) ----
    const float envPanX = 488.0f, envPanY = 52.0f;
    const float envPanW = W - envPanX - 10.0f, envPanH = H - 60.0f;

    g.setColour (juce::Colour (0xff060e18));
    g.fillRoundedRectangle (envPanX, envPanY, envPanW, envPanH, 8.0f);
    g.setColour (juce::Colour (0xff1a3550));
    g.drawRoundedRectangle (envPanX, envPanY, envPanW, envPanH, 8.0f, 1.5f);

    // Envelope panel title strip
    g.setColour (juce::Colour (0xff091525));
    g.fillRoundedRectangle (envPanX, envPanY, envPanW, 26.0f, 8.0f);
    g.setColour (juce::Colour (0xff1a3550));
    g.drawHorizontalLine ((int)(envPanY + 26), envPanX, envPanX + envPanW);
    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.drawText ("ENVELOPE", (int)envPanX + 12, (int)envPanY + 6, 100, 14,
                juce::Justification::left);
}

//==============================================================================
void NewProjectAudioProcessorEditor::resized()
{
    // ---- Oscillator panel layout ----
    // Waveform selector tiles — tall and prominent
    waveSelector.setBounds (18, 88, 454, 168);

    // Unison + Detune knobs (below wave selector, left side)
    const int unisonY = 300;
    knobUnison.setBounds (18,  unisonY, 80, 90);
    knobDetune.setBounds (108, unisonY, 80, 90);

    // Volume knob (bottom right of osc panel)
    knobVolume.setBounds (368, unisonY, 90, 90);

    // ---- Envelope panel layout (right panel) ----
    // ADSR display
    adsrDisplay.setBounds (494, 84, 218, 150);

    // ADSR knobs in 2x2 grid
    const int kW = 50, kH = 72;
    const int kGap = 8;
    const int kRow1 = 244, kRow2 = kRow1 + kH + kGap;
    const int kCol1 = 496, kCol2 = kCol1 + kW + kGap + 10;

    knobAttack.setBounds  (kCol1,        kRow1, kW + 10, kH);
    knobDecay.setBounds   (kCol2 + 10,   kRow1, kW + 10, kH);
    knobSustain.setBounds (kCol1,        kRow2, kW + 10, kH);
    knobRelease.setBounds (kCol2 + 10,   kRow2, kW + 10, kH);
}
