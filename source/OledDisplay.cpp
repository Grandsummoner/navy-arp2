#include "OledDisplay.h"
#include "PluginProcessor.h"
#include "AppTheme.h"

OledDisplay::OledDisplay (PluginProcessor& p)
    : processor (p)
{
}

OledDisplay::~OledDisplay()
{
}

void OledDisplay::showParameterOverlay (const juce::String& paramName, float baseValue, const juce::String& lfoVibeText)
{
    activeParamName = paramName;
    activeParamValue = baseValue;
    activeLfoVibe = lfoVibeText;
    isOverlayActive = true;
    
    repaint();
    startTimer (1500); // 1.5 second display timeout
}

void OledDisplay::setFreezeActive (bool shouldBeActive)
{
    if (isFreezeActive != shouldBeActive)
    {
        isFreezeActive = shouldBeActive;
        repaint();
    }
}

void OledDisplay::timerCallback()
{
    isOverlayActive = false;
    stopTimer();
    repaint();
}

void OledDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Fill Screen Background with flat, high-contrast obsidian black void [43]
    g.fillAll (juce::Colour (0xFF05070A));

    // 2. Draw Bezel with Inset Glass Look
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawHorizontalLine (0, 0.0f, bounds.getWidth());
    g.drawVerticalLine (0, 0.0f, bounds.getHeight());

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (static_cast<int> (bounds.getHeight() - 1.0f), 0.0f, bounds.getWidth());
    g.drawVerticalLine (static_cast<int> (bounds.getWidth() - 1.0f), 0.0f, bounds.getHeight());

    auto displayArea = bounds.reduced (12.0f);

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    if (isOverlayActive)
    {
        // =====================================================================
        // RENDER: MOMENTARY PARAMETER OVERLAY SCREEN (ACTIVE ONLY ON INTERACTION)
        // =====================================================================
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour (0xFFFF5533));
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (activeParamName.toUpperCase(), displayArea.removeFromTop (18.0f), juce::Justification::left, true);

        displayArea.removeFromTop (4.0f);

        auto valueBarArea = displayArea.removeFromTop (12.0f);
        g.setColour (juce::Colours::darkgrey.withAlpha (0.3f));
        g.fillRoundedRectangle (valueBarArea, 2.0f);
        
        float fillWidth = valueBarArea.getWidth() * activeParamValue;
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour (0xFFFF5533));
        g.fillRoundedRectangle (valueBarArea.withWidth (fillWidth), 2.0f);

        displayArea.removeFromTop (6.0f);

        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (9.5f, juce::Font::plain));
        
        juce::String valuePercentStr = "VALUE: " + juce::String (static_cast<int> (activeParamValue * 100.0f)) + "%";
        g.drawText (valuePercentStr, displayArea.removeFromTop (12.0f), juce::Justification::left, true);
        g.drawText ("LFO: " + activeLfoVibe, displayArea.removeFromTop (12.0f), juce::Justification::left, true);
    }
    else
    {
        // =====================================================================
        // MATH SETUP: DEEP GEODESIC 3D CONSTELLATION GLOBE [43]
        // =====================================================================
        struct Point3D { float x, y, z; };
        std::vector<Point3D> vertices;

        // Generate a highly complex sphere layout with 5 rings of 12 points each plus poles [43]
        vertices.push_back ({ 0.0f, 1.0f, 0.0f }); // Top pole (Index 0)
        for (int ring = 1; ring <= 5; ++ring)
        {
            float ringPitch = juce::MathConstants<float>::pi * (static_cast<float> (ring) / 6.0f);
            float sinPitch = std::sin (ringPitch);
            float cosPitch = std::cos (ringPitch);
            for (int pt = 0; pt < 12; ++pt)
            {
                float yawAngle = juce::MathConstants<float>::twoPi * (static_cast<float> (pt) / 12.0f);
                vertices.push_back ({ sinPitch * std::cos (yawAngle), cosPitch, sinPitch * std::sin (yawAngle) });
            }
        }
        vertices.push_back ({ 0.0f, -1.0f, 0.0f }); // Bottom pole (Index 61)

        // Slow, continuous rotation angle mapped over system millisecond timers [43]
        double timeMs = juce::Time::getMillisecondCounterHiRes();
        float yaw = static_cast<float> (timeMs * 0.00012);
        float pitch = static_cast<float> (timeMs * 0.00005);

        auto rotatePoint = [&](Point3D p, float yAngle, float pAngle) -> Point3D
        {
            // Yaw (around Y axis)
            float x1 = p.x * std::cos (yAngle) - p.z * std::sin (yAngle);
            float z1 = p.x * std::sin (yAngle) + p.z * std::cos (yAngle);
            
            // Pitch (around X axis)
            float y2 = p.y * std::cos (pAngle) - z1 * std::sin (pAngle);
            float z2 = p.y * std::sin (pAngle) + z1 * std::cos (pAngle);
            
            return { x1, y2, z2 };
        };

        // Project 3D coordinates onto central background viewport
        float globeCenterX = displayArea.getCentreX(); // British English spelling [43]
        float globeCenterY = displayArea.getCentreY() + 8.0f; // Shift slightly down to center
        float globeRadius = displayArea.getHeight() * 0.45f;
        float cameraDistance = 2.2f;

        std::vector<juce::Point<float>> projectedPoints;
        for (const auto& v : vertices)
        {
            auto rotated = rotatePoint (v, yaw, pitch);
            float scale = 1.0f / (1.0f + rotated.z / cameraDistance);
            float px = globeCenterX + rotated.x * globeRadius * scale;
            float py = globeCenterY + rotated.y * globeRadius * scale;
            projectedPoints.push_back ({ px, py });
        }

        // =====================================================================
        // RENDER: LAYER 2 - BACKGROUND DENSE 3D GLOBE (276 lines) [43]
        // =====================================================================
        const int activeStep = processor.currentStep.load();
        const bool isPlaying = processor.isCurrentlyPlayingUI.load();

        // Neon Palette colors derived from your visual reference
        juce::Colour lineColour      = juce::Colour::fromString ("#FF0066FF").withAlpha (0.35f); // High-contrast Cobalt Blue [43]
        juce::Colour nodeGlowColour  = juce::Colour::fromString ("#FF00E1FF");                  // Electric Cyan Glow [43]
        juce::Colour nodeCoreColour  = juce::Colour::fromString ("#FF80F3FF");                  // White/Cyan Core
        juce::Colour triangleColour  = juce::Colour::fromString ("#FFFF3366").withAlpha (0.25f); // Cyberpunk Coral Red [43]

        // 1. Draw glowing, step-reactive facets (triangles) [43]
        if (isPlaying)
        {
            // Seed deterministic random triangles based on activeStep to prevent jittering within a step
            juce::Random r (activeStep + 100);
            int v1 = r.nextInt (62);
            int v2 = (v1 + 1) % 62;
            int v3 = (v1 + 12) % 62;

            juce::Path triPath;
            triPath.startNewSubPath (projectedPoints[v1]);
            triPath.lineTo (projectedPoints[v2]);
            triPath.lineTo (projectedPoints[v3]);
            triPath.closeSubPath();

            g.setColour (triangleColour);
            g.fillPath (triPath);
        }

        // 2. Draw wireframe connection lines (Intricate 276-line geodesic mesh) [43]
        g.setColour (lineColour);
        auto drawEdge = [&](int idx1, int idx2)
        {
            g.drawLine (projectedPoints[idx1].x, projectedPoints[idx1].y,
                        projectedPoints[idx2].x, projectedPoints[idx2].y, 0.75f);
        };
        for (int i = 1; i <= 12; ++i) drawEdge (0, i); // Top pole to first ring
        for (int ringIdx = 0; ringIdx < 4; ++ringIdx)
        {
            int offset1 = 1 + ringIdx * 12;
            int offset2 = 1 + (ringIdx + 1) * 12;
            for (int i = 0; i < 12; ++i)
            {
                // Horizontal ring connections
                drawEdge (offset1 + i, offset1 + (i + 1) % 12);
                
                // Vertical bridging lines
                drawEdge (offset1 + i, offset2 + i);
                
                // Diagonal cross connections to increase geometric density to 276 lines [43]
                drawEdge (offset1 + i, offset2 + (i + 1) % 12);
                drawEdge (offset1 + (i + 1) % 12, offset2 + i);
            }
        }
        int offset5 = 49;
        for (int i = 0; i < 12; ++i)
        {
            drawEdge (offset5 + i, offset5 + (i + 1) % 12);
            drawEdge (offset5 + i, 61); // Bottom pole to ring 5
        }

        // 3. Draw active glowing star nodes, modulated in vertical waves by the 8 LFOs! [43]
        for (size_t i = 0; i < projectedPoints.size(); ++i)
        {
            float nodeAlpha = 0.40f; // Default baseline brightness
            
            // Map rings of the globe to different LFOs [43]
            int lfoIdx = -1;
            if (i == 0 || i == 61) lfoIdx = 0;       // Poles -> Morph LFO (LFO 0)
            else if (i >= 1 && i <= 12) lfoIdx = 1;   // Ring 1 -> Rest LFO (LFO 1)
            else if (i >= 13 && i <= 24) lfoIdx = 2;  // Ring 2 -> Legato LFO (LFO 2)
            else if (i >= 25 && i <= 36) lfoIdx = 3;  // Ring 3 -> Rate LFO (LFO 3)
            else if (i >= 37 && i <= 48) lfoIdx = 4;  // Ring 4 -> Entropy LFO (LFO 4)
            else if (i >= 49 && i <= 60) lfoIdx = 5;  // Ring 5 -> Harmony LFO (LFO 5)

            if (lfoIdx != -1)
            {
                auto* ratePtr = processor.lfoRatePtrs[lfoIdx];
                auto* depthPtr = processor.lfoDepthPtrs[lfoIdx];
                if (ratePtr != nullptr && depthPtr != nullptr)
                {
                    int rateChoice = static_cast<int> (ratePtr->load());
                    float depth = depthPtr->load();
                    if (rateChoice > 0 && depth > 0.02f)
                    {
                        double currentPhase = processor.lfoPhases[lfoIdx];
                        // Modulate nodeAlpha dynamically based on LFO wave
                        float mod = static_cast<float> (std::sin (currentPhase * juce::MathConstants<double>::twoPi)) * depth * 0.5f + 0.5f;
                        nodeAlpha = juce::jlimit (0.15f, 1.0f, nodeAlpha + mod * 0.6f);
                    }
                }
            }

            // Draw Level 2: Outer Cyan Glow
            g.setColour (nodeGlowColour.withAlpha (nodeAlpha * 0.7f));
            g.fillEllipse (projectedPoints[i].x - 2.5f, projectedPoints[i].y - 2.5f, 5.0f, 5.0f);

            // Draw Level 1: Bright White/Cyan Core
            g.setColour (nodeCoreColour.withAlpha (nodeAlpha));
            g.fillEllipse (projectedPoints[i].x - 1.0f, projectedPoints[i].y - 1.0f, 2.0f, 2.0f);
        }

        // =====================================================================
        // RENDER: HIGHLIGHTS, HEADERS, AND LABELS
        // =====================================================================
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("NAVY-ARP MONITOR", displayArea.removeFromTop (16.0f), juce::Justification::centred, true);

        displayArea.removeFromTop (3.0f);

        int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rootKey.getParamID())));
        juce::StringArray keys { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, scales { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" };
        juce::String keyStr = keys[rootKeyIdx];

        int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*processor.apvts.getRawParameterValue (IDs::scaleType.getParamID())));
        juce::String scaleStr = scales[scaleIdx];

        bool isPolyActive = *processor.apvts.getRawParameterValue (IDs::poly.getParamID()) > 0.5f;
        float currentHarmony = *processor.apvts.getRawParameterValue (IDs::harmony.getParamID());
        juce::String voiceStr = "MONO";
        if (isPolyActive)
        {
            if (currentHarmony >= 0.25f && currentHarmony < 0.5f) voiceStr = "DUO";
            else if (currentHarmony >= 0.5f) voiceStr = "TRIAD";
            else voiceStr = "MONO";
        }

        int rateIdx = juce::jlimit (0, 3, static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rate.getParamID())));
        juce::StringArray rates { "1/4", "1/8", "1/16", "1/32" };
        juce::String rateStr = rates[rateIdx];

        int rangeShift = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
        juce::String octStr = (rangeShift >= 0 ? "+" : "") + juce::String (rangeShift);

        juce::String metaText = "KEY: " + keyStr + " | SCALE: " + scaleStr + " | VOICE: " + voiceStr + " | RATE: " + rateStr + " | OCT: " + octStr;
        
        g.setColour (juce::Colour (0xFFFFB300)); // Gold/Yellow metadata font
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (metaText, displayArea.removeFromTop (12.0f), juce::Justification::centred, true);

        // =====================================================================
        // RENDER: LAYER 3 - RESTORED FULL-WIDTH STEP LEVEL MONITOR (VU) [43]
        // =====================================================================
        const float spacing = 6.0f;
        const float colWidth = (displayArea.getWidth() - (7.0f * spacing)) / 8.0f;

        const int numSegments = 16;
        const float segmentHeight = 6.0f;      // Chunky, bold hardware-style LEDs
        const float segmentSpacing = 2.0f;
        const float maxLaddersHeight = (numSegments * segmentHeight) + ((numSegments - 1) * segmentSpacing);

        // Position fader monitor area to sit right above the bottom step numbers
        float fadersY = bounds.getHeight() - maxLaddersHeight - 24.0f;
        auto laddersArea = juce::Rectangle<float> (displayArea.getX(), fadersY, displayArea.getWidth(), maxLaddersHeight);

        for (int i = 0; i < 8; ++i)
        {
            auto colBounds = laddersArea.removeFromLeft (colWidth);
            
            // Fetch morphed fader level for this step dynamically
            float faderVal = (processor.sceneA.faders[i] * (1.0f - morphVal)) + (processor.sceneB.faders[i] * morphVal);
            int activeSegments = static_cast<int> (std::round (faderVal * static_cast<float> (numSegments)));

            const bool isActiveStep = isPlaying && (i == activeStep);

            // Draw individual segmented horizontal blocks bottom-up
            for (int seg = 0; seg < numSegments; ++seg)
            {
                float segY = colBounds.getY() + maxLaddersHeight - ((seg + 1) * (segmentHeight + segmentSpacing));
                auto segmentRect = juce::Rectangle<float> (colBounds.getX() + 2.0f, segY, colBounds.getWidth() - 4.0f, segmentHeight);

                if (seg < activeSegments)
                {
                    // Active filled segments overlapping the rotating globe [43]
                    if (isActiveStep)
                        g.setColour (juce::Colour (0xFFFF4500)); // Glowing orange-red playhead
                    else
                        g.setColour (juce::Colour (0xFF441105).withAlpha (0.85f)); // Muted dim red/brown
                }
                else
                {
                    // Unfilled empty segments [43]
                    g.setColour (juce::Colour (0xFF111317).withAlpha (0.75f));
                }

                g.fillRect (segmentRect);
            }

            // Draw small step number indicator sitting exactly at the bottom screen boundary
            float textY = colBounds.getY() + maxLaddersHeight + 4.0f;
            auto stepNumRect = juce::Rectangle<float> (colBounds.getX(), textY, colBounds.getWidth(), 12.0f);
            
            if (isActiveStep)
            {
                g.setColour (juce::Colour (0xFFFF4500));
                g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            }
            else
            {
                g.setColour (juce::Colours::grey.withAlpha (0.6f));
                g.setFont (juce::FontOptions (9.0f, juce::Font::plain));
            }

            g.drawText (juce::String (i + 1), stepNumRect, juce::Justification::centred, true);

            laddersArea.removeFromLeft (spacing); // Advance column offset
        }
    }
}

void OledDisplay::resized()
{
}