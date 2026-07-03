#include "OledDisplay.h"
#include "PluginProcessor.h"

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

    // 1. Fill Screen Background with a rich, deep midnight radial gradient [43]
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float centerX = width * 0.20f; // Centered behind your 3D globe [43]
    float centerY = height * 0.5f;

    juce::ColourGradient bgGlow (juce::Colour (0xFF102030).withAlpha (0.4f), centerX, centerY,
                                 juce::Colour (0xFF030508), width * 0.5f, centerY,
                                 true);
    g.setGradientFill (bgGlow);
    g.fillRoundedRectangle (bounds, 4.0f);

    // 2. Draw Bezel with Inset Glass Look
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawHorizontalLine (0, 0.0f, bounds.getWidth());
    g.drawVerticalLine (0, 0.0f, bounds.getHeight());

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (static_cast<int> (bounds.getHeight() - 1.0f), 0.0f, bounds.getWidth());
    g.drawVerticalLine (static_cast<int> (bounds.getWidth() - 1.0f), 0.0f, bounds.getHeight());

    auto displayArea = bounds.reduced (12.0f);

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
        // MATH SETUP: 3D CONSTELLATION GLOBE [43]
        // =====================================================================
        struct Point3D { float x, y, z; };
        std::vector<Point3D> vertices;

        // Generate a simple sphere layout with 3 rings of 6 points plus top and bottom poles [43]
        vertices.push_back ({ 0.0f, 1.0f, 0.0f }); // Top pole
        for (int ring = 1; ring <= 3; ++ring)
        {
            float ringPitch = juce::MathConstants<float>::pi * (static_cast<float> (ring) / 4.0f);
            float sinPitch = std::sin (ringPitch);
            float cosPitch = std::cos (ringPitch);
            for (int pt = 0; pt < 6; ++pt)
            {
                float yawAngle = juce::MathConstants<float>::twoPi * (static_cast<float> (pt) / 6.0f);
                vertices.push_back ({ sinPitch * std::cos (yawAngle), cosPitch, sinPitch * std::sin (yawAngle) });
            }
        }
        vertices.push_back ({ 0.0f, -1.0f, 0.0f }); // Bottom pole

        // Slow, continuous rotation angle mapped over system millisecond timers [43]
        double timeMs = juce::Time::getMillisecondCounterHiRes();
        float yaw = static_cast<float> (timeMs * 0.00015);
        float pitch = static_cast<float> (timeMs * 0.00007);

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

        // Project 3D coordinates onto Left-hand viewport
        auto globeArea = displayArea.removeFromLeft (width * 0.38f); // Allocate 38% of OLED space to the Globe [43]
        float globeCenterX = globeArea.getCenterX();
        float globeCenterY = globeArea.getCenterY();
        float globeRadius = juce::jmin (globeArea.getWidth(), globeArea.getHeight()) * 0.40f;
        float cameraDistance = 2.0f;

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
        // RENDER: LEFT SIDEBAR - 3D CONSTELLATION GLOBE & SUNFLARES [43]
        // =====================================================================
        const int activeStep = processor.currentStep.load();
        const bool isPlaying = processor.isCurrentlyPlayingUI.load();
        
        float morphVal = *processor.apvts.getRawParameterValue (IDs::morph.getParamID());
        juce::Colour activeColor = t.knobFillLeft.interpolatedWith (t.knobFillRight, morphVal);

        // 1. Draw glowing, step-reactive facets (triangles) [43]
        if (isPlaying)
        {
            // Seed deterministic random triangles based on activeStep to prevent jittering within a step
            juce::Random r (activeStep + 100);
            int v1 = r.nextInt (20);
            int v2 = (v1 + 1) % 20;
            int v3 = (v1 + 5) % 20;

            juce::Path triPath;
            triPath.startNewSubPath (projectedPoints[v1]);
            triPath.lineTo (projectedPoints[v2]);
            triPath.lineTo (projectedPoints[v3]);
            triPath.closeSubPath();

            g.setColour (activeColor.withAlpha (0.15f));
            g.fillPath (triPath);
        }

        // 2. Draw wireframe connection lines [43]
        g.setColour (activeColor.withAlpha (0.18f));
        auto drawEdge = [&](int idx1, int idx2)
        {
            g.drawLine (projectedPoints[idx1].x, projectedPoints[idx1].y,
                        projectedPoints[idx2].x, projectedPoints[idx2].y, 0.75f);
        };
        for (int i = 1; i <= 6; ++i) drawEdge (0, i); // Top pole to first ring
        for (int ringIdx = 0; ringIdx < 2; ++ringIdx)
        {
            int offset1 = 1 + ringIdx * 6;
            int offset2 = 1 + (ringIdx + 1) * 6;
            for (int i = 0; i < 6; ++i)
            {
                drawEdge (offset1 + i, offset1 + (i + 1) % 6);
                drawEdge (offset1 + i, offset2 + i);
            }
        }
        int offset3 = 13;
        for (int i = 0; i < 6; ++i)
        {
            drawEdge (offset3 + i, offset3 + (i + 1) % 6);
            drawEdge (offset3 + i, 19); // Bottom pole to last ring
        }

        // 3. Draw active glowing star nodes [43]
        for (const auto& pt : projectedPoints)
        {
            g.setColour (activeColor);
            g.fillEllipse (pt.x - 1.25f, pt.y - 1.25f, 2.5f, 2.5f);
        }

        // 4. Draw rare high-voltage lightning discharges/sunflares snapping to edge [43]
        std::uint64_t frameCounter = static_cast<std::uint64_t> (timeMs / 33.3); // ~30fps steps
        bool triggerFlare = (frameCounter % 300) < 4; // Active for 4 frames (130ms) every 10 seconds

        if (triggerFlare)
        {
            juce::Random flareRand (frameCounter / 300);
            int nodeIdx = flareRand.nextInt (20);

            // Determine target boundary wall point [43]
            float targetX = (flareRand.nextBool()) ? displayArea.getX() : (displayArea.getX() + width * 0.38f);
            float targetY = displayArea.getY() + flareRand.nextFloat() * displayArea.getHeight();

            juce::Path flarePath;
            flarePath.startNewSubPath (projectedPoints[nodeIdx]);

            // Generate fractal jagged divisions
            auto startPt = projectedPoints[nodeIdx];
            for (int step = 1; step <= 3; ++step)
            {
                float stepT = static_cast<float> (step) / 3.0f;
                float nominalX = startPt.x + (targetX - startPt.x) * stepT;
                float nominalY = startPt.y + (targetY - startPt.y) * stepT;

                // Perpendicular displacement jitter
                float jitterX = (flareRand.nextFloat() - 0.5f) * 10.0f;
                float jitterY = (flareRand.nextFloat() - 0.5f) * 10.0f;

                if (step == 3) { jitterX = 0.0f; jitterY = 0.0f; } // Close perfectly at the target edge [43]

                flarePath.lineTo (nominalX + jitterX, nominalY + jitterY);
            }

            // High-voltage double-stroke glow render [43]
            g.setColour (juce::Colours::white);
            g.strokePath (flarePath, juce::PathStrokeType (1.0f));
            g.setColour (activeColor.withAlpha (0.45f));
            g.strokePath (flarePath, juce::PathStrokeType (3.0f));
        }

        // =====================================================================
        // RENDER: RIGHT SIDEBAR - METADATA HEADER & COMPACT VU LADDERS [43]
        // =====================================================================
        displayArea.removeFromLeft (12.0f); // 12px blank gutter partition between Globe and VU Monitor [43]
        auto rightArea = displayArea;

        // 1. Draw Metadata Header Box [43]
        auto headerArea = rightArea.removeFromTop (18.0f);
        
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
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawText (metaText, headerArea, juce::Justification::centred, true);

        // 2. Draw 8 Segmented VU Step Ladders [43]
        rightArea.removeFromTop (4.0f); // Small padding below header

        const float spacing = 4.0f;
        const float colWidth = (rightArea.getWidth() - (7.0f * spacing)) / 8.0f;

        const int numSegments = 12; // Scaled down to 12 segments to fit beautifully on the side [43]
        const float segmentHeight = 4.0f; 
        const float segmentSpacing = 1.5f;
        const float maxLaddersHeight = (numSegments * segmentHeight) + ((numSegments - 1) * segmentSpacing);

        // Center the faders vertically inside the remaining area [43]
        float fadersY = rightArea.getY() + (rightArea.getHeight() - maxLaddersHeight - 14.0f) * 0.5f;
        auto laddersArea = juce::Rectangle<float> (rightArea.getX(), fadersY, rightArea.getWidth(), maxLaddersHeight);

        for (int i = 0; i < 8; ++i)
        {
            auto colBounds = laddersArea.removeFromLeft (colWidth);
            
            float faderVal = (processor.sceneA.faders[i] * (1.0f - morphVal)) + (processor.sceneB.faders[i] * morphVal);
            int activeSegments = static_cast<int> (std::round (faderVal * static_cast<float> (numSegments)));

            const bool isActiveStep = isPlaying && (i == activeStep);

            // Draw individual segmented horizontal blocks bottom-up
            for (int seg = 0; seg < numSegments; ++seg)
            {
                float segY = colBounds.getY() + maxLaddersHeight - ((seg + 1) * (segmentHeight + segmentSpacing));
                auto segmentRect = juce::Rectangle<float> (colBounds.getX() + 1.0f, segY, colBounds.getWidth() - 2.0f, segmentHeight);

                if (seg < activeSegments)
                {
                    // Active filled segments [43]
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
            float textY = colBounds.getY() + maxLaddersHeight + 2.0f;
            auto stepNumRect = juce::Rectangle<float> (colBounds.getX(), textY, colBounds.getWidth(), 12.0f);
            
            if (isActiveStep)
            {
                g.setColour (juce::Colour (0xFFFF4500));
                g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            }
            else
            {
                g.setColour (juce::Colours::grey.withAlpha (0.6f));
                g.setFont (juce::FontOptions (8.5f, juce::Font::plain));
            }

            g.drawText (juce::String (i + 1), stepNumRect, juce::Justification::centred, true);

            laddersArea.removeFromLeft (spacing); // Advance column offset
        }
    }
}

void OledDisplay::resized()
{
}