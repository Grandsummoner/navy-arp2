#include "OledDisplay.h"

OledDisplay::OledDisplay (PluginProcessor& p) : processor (p) 
{ 
    startTimerHz (30); 
}

OledDisplay::~OledDisplay() 
{ 
    stopTimer(); 
}

void OledDisplay::timerCallback()
{
    juce::String prefixes[] = { "rhythmMorph", "rest", "legato", "rate", "entropy", "harmony", "chaos", "octaves" };
    bool lfoParamChanged = false; 
    int changedIdx = -1;
    for (int i = 0; i < 8; ++i) {
        int rate = static_cast<int> (*processor.apvts.getRawParameterValue (prefixes[i] + "LfoRate"));
        float depth = *processor.apvts.getRawParameterValue (prefixes[i] + "LfoDepth");
        if (rate != lastLfoRates[i] || depth != lastLfoDepths[i]) {
            lastLfoRates[i] = rate; lastLfoDepths[i] = depth; lfoParamChanged = true; changedIdx = i;
        }
    }
    if (lfoParamChanged) { lfoOverlayTimer = 45; lfoActiveParamIdx = changedIdx; }
    else if (lfoOverlayTimer > 0) { lfoOverlayTimer--; }
    repaint();
}

void OledDisplay::paint (juce::Graphics& g)
{
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load()); auto t = AppTheme::get (themeIdx);
    g.fillAll (juce::Colour (0xFF08080A)); g.setColour (t.border); g.drawRect (getLocalBounds().toFloat(), 2.0f);
    auto area = getLocalBounds().reduced (15);

    if (lfoOverlayTimer > 0 && lfoActiveParamIdx >= 0) {
        juce::String prefixes[] = { "MORPH", "REST", "LEGATO", "RATE", "ENTROPY", "HARMONY", "CHAOS", "OCTAVES" };
        g.setColour (t.leftAccent); g.setFont (juce::Font (juce::FontOptions (15.0f).withStyle ("Bold")));
        g.drawText ("LFO PREVIEW: " + prefixes[lfoActiveParamIdx], 15, 12, getWidth() - 30, 20, juce::Justification::centred);
        g.setFont (juce::Font (juce::FontOptions (12.0f))); g.setColour (juce::Colour (0xFFFFB300));
        juce::String speedRate = processor.apvts.getParameter(prefixes[lfoActiveParamIdx].toLowerCase() + "LfoRate")->getCurrentValueAsText();
        g.drawText ("RATE: " + speedRate + " | DEPTH: " + juce::String(static_cast<int>(lastLfoDepths[lfoActiveParamIdx] * 100.0f)) + "%", 15, 27, getWidth() - 30, 15, juce::Justification::centred);

        juce::Path wavePath; float waveYCenter = area.getCentreY() + 10.0f, waveHeight = (area.getHeight() - 40.0f) * lastLfoDepths[lfoActiveParamIdx] * 0.45f;
        int rateChoice = lastLfoRates[lfoActiveParamIdx]; float frequencyFactor = (rateChoice == 1) ? 0.25f : (rateChoice == 2) ? 0.5f : (rateChoice == 3) ? 1.0f : 2.0f;
        static float phaseOffset = 0.0f; phaseOffset += 0.15f * frequencyFactor;
        if (phaseOffset >= juce::MathConstants<float>::twoPi) phaseOffset -= juce::MathConstants<float>::twoPi;
        for (int x = area.getX(); x < area.getRight(); ++x) {
            float pct = static_cast<float>(x - area.getX()) / static_cast<float>(area.getWidth());
            float sineVal = std::sin (pct * juce::MathConstants<float>::twoPi * 2.0f * frequencyFactor + phaseOffset);
            float yVal = waveYCenter + sineVal * waveHeight;
            if (x == area.getX()) wavePath.startNewSubPath (static_cast<float>(x), yVal); else wavePath.lineTo (static_cast<float>(x), yVal);
        }
        g.setColour (lfoActiveParamIdx < 4 ? t.leftAccent : t.rightAccent); g.strokePath (wavePath, juce::PathStrokeType (1.8f));
        return; 
    }

    auto headerArea = getLocalBounds().removeFromTop (25); g.setColour (juce::Colour (0xFF181C24)); g.fillRect (headerArea);
    g.setColour (juce::Colour (0xFFFFFFFF)); g.setFont (juce::Font (juce::FontOptions (15.0f).withStyle ("Bold")));
    g.drawText ("NAVY-ARP MONITOR", headerArea, juce::Justification::centred, true);

    juce::String scaleName = processor.apvts.getParameter(IDs::scaleType.getParamID())->getCurrentValueAsText();
    juce::String keyName = processor.apvts.getParameter(IDs::rootKey.getParamID())->getCurrentValueAsText();
    juce::String speedRate = processor.apvts.getParameter(IDs::rate.getParamID())->getCurrentValueAsText();
    int octValue = static_cast<int>(*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
    juce::String activeOcts = (octValue >= 0) ? "+" + juce::String (octValue) : juce::String (octValue);
    g.setFont (juce::Font (juce::FontOptions (12.0f))); g.setColour (juce::Colour (0xFFFFB300));
    g.drawText ("KEY: " + keyName + " | SCALE: " + scaleName + " | VOICE: TRIAD | RATE: " + speedRate + " | OCT: " + activeOcts, 10, 27, getWidth() - 20, 15, juce::Justification::centred);

    area.removeFromTop (27);
    int barWidth = area.getWidth() / 8, spacing = 6;
    for (int i = 0; i < 8; ++i) {
        float faderProb = *processor.apvts.getRawParameterValue ("fader" + juce::String (i + 1));
        int stepSegments = static_cast<int>((area.getHeight() - 20) * faderProb * 0.75f) / 8;
        juce::Rectangle<int> bar (area.getX() + (i * barWidth) + spacing, area.getBottom() - (stepSegments * 8) - 15, barWidth - (spacing * 2), stepSegments * 8);
        
        bool isPlaying = processor.isCurrentlyPlayingUI.load();
        for (int seg = 0; s
