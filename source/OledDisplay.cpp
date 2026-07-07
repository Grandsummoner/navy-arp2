#include "OledDisplay.h"
#include "PluginProcessor.h"
#include "AppTheme.h"

// =====================================================================
// PERSISTENT INSTANCE-SAFE STATE CONTAINER (ZERO HEADER OVERHEAD)
// =====================================================================
class CameoState : public juce::ReferenceCountedObject
{
public:
    struct ActiveCameo
    {
        int type = 0;              // 0 = Voyager, 1 = James Webb, 2 = Cartoon UFO
        float startX = 0.0f, startY = 0.0f;
        float targetX = 0.0f, targetY = 0.0f;
        double startTimeMs = 0.0;
        double durationMs = 0.0;   // Flight duration
        int trajectoryPattern = 0; // 0 = Straight, 1 = Arc, 2 = Jitter
        float arcAmplitude = 0.0f;
    };
    
    std::vector<ActiveCameo> activeCameos;
    double lastCameoTriggerTime = 0.0;
    double nextCameoInterval = 120000.0; // 2 minutes minimum

    struct FacetTriangle
    {
        int v1 = 0, v2 = 1, v3 = 12;
        double lastTeleportTime = 0.0;
        double currentPeriod = 1000.0;
        juce::Colour colour;
        bool isActive = true;
    };
    
    FacetTriangle triangles[4];
    bool isInitialized = false;
};

OledDisplay::OledDisplay (PluginProcessor& p)
    : processor (p)
{
}

OledDisplay::~OledDisplay()
{
}

// 3-Argument Signature preserved for compile-safety [1.2.3]
void OledDisplay::showParameterOverlay (const juce::String& paramName, float baseValue, const juce::String& lfoVibeText)
{
    activeParamName = paramName;
    activeParamValue = baseValue;
    activeLfoVibe = lfoVibeText;
    isOverlayActive = true;
    
    repaint();
    startTimer (1000); // 1.0 second snappy display timeout [1.2.3]
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
    float width = bounds.getWidth();   
    float height = bounds.getHeight(); 

    g.fillAll (juce::Colour (0xFF05070A));

    auto displayArea = bounds.reduced (16.0f);

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    // Apply active chosen theme colors
    juce::Colour themeColor = juce::Colour (0xFF00E5FF); // Theme 0 (Navy): Teal
    if (themeIdx == 1)      themeColor = juce::Colour (0xFFECEFF1); // Theme 1 (Monochrome): White/Silver
    else if (themeIdx == 2) themeColor = juce::Colour (0xFF00FF66); // Theme 2 (Matrix): Neon Green

    if (isOverlayActive)
    {
        // 1. Calculate dynamic absolute LFO modulated progress directly inside OledDisplay [1.2.3]
        float lfoProgress = 0.0f;
        int lfoIdx = -1;
        if (activeParamName == "Rhythm Morph")  lfoIdx = 0;
        else if (activeParamName == "Rest")      lfoIdx = 1;
        else if (activeParamName == "Legato")    lfoIdx = 2;
        else if (activeParamName == "BPM")       lfoIdx = 3; 
        else if (activeParamName == "Entropy")   lfoIdx = 4;
        else if (activeParamName == "Harmony")   lfoIdx = 5;
        else if (activeParamName == "Chaos")     lfoIdx = 6;
        else if (activeParamName == "Octaves")   lfoIdx = 7;

        if (lfoIdx != -1 && activeLfoVibe != "Off")
        {
            auto* ratePtr = processor.lfoRatePtrs[lfoIdx];
            auto* depthPtr = processor.lfoDepthPtrs[lfoIdx];
            if (ratePtr != nullptr && depthPtr != nullptr)
            {
                int rChoice = static_cast<int> (ratePtr->load());
                float depth = depthPtr->load();
                if (rChoice > 0 && depth > 0.0f)
                {
                    double currentPhase = processor.lfoPhases[lfoIdx];
                    float sinVal = static_cast<float> (std::sin (currentPhase * juce::MathConstants<double>::twoPi));
                    lfoProgress = activeParamValue + (sinVal * depth * 0.5f);
                    lfoProgress = juce::jlimit (0.0f, 1.0f, lfoProgress);
                }
            }
        }

        // 2. Draw subtle diagnostic background grid
        g.setColour (themeColor.withAlpha (0.04f));
        for (float gridX = displayArea.getX(); gridX < displayArea.getRight(); gridX += 16.0f)
            g.drawVerticalLine (static_cast<int> (gridX), displayArea.getY(), displayArea.getBottom());
        for (float gridY = displayArea.getY(); gridY < displayArea.getBottom(); gridY += 16.0f)
            g.drawHorizontalLine (static_cast<int> (gridY), displayArea.getX(), displayArea.getRight());

        // 3. Draw partition lines and upper system metrics
        float startY = displayArea.getY();
        g.setColour (themeColor.withAlpha (0.4f));
        g.drawHorizontalLine (static_cast<int> (startY + 18.0f), displayArea.getX(), displayArea.getRight());
        
        g.setColour (themeColor.withAlpha (0.8f));
        g.setFont (juce::FontOptions ("Courier New", 9.0f, juce::Font::bold));
        g.drawText ("PARAMETER OVERLAY", displayArea.getX(), startY, 150, 15, juce::Justification::left);
        g.drawText ("SYSTEM MONITOR // ACTIVE", displayArea.getX(), startY, displayArea.getWidth(), 15, juce::Justification::right);

        // 4. Render Large Parameter Title and Oversized Readout with custom string formatting [1.2.3]
        float contentY = startY + 28.0f;
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        g.drawText (activeParamName.toUpperCase(), displayArea.getX() + 10, contentY, 300, 20, juce::Justification::left);

        // Format continuous inputs based on parameter ranges
        juce::String valStr = "";
        if (activeParamName == "BPM")
        {
            auto* syncPtr = processor.apvts.getRawParameterValue ("sync");
            bool syncActive = (syncPtr != nullptr && syncPtr->load() > 0.5f);
            if (syncActive)
            {
                int rateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (activeParamValue * 3.0f)));
                juce::StringArray rates { "1/4", "1/8", "1/16", "1/32" };
                valStr = rates[rateIdx]; // Synced fraction display [1.2.3]
            }
            else
            {
                int manualBpm = static_cast<int> (std::round (40.0f + activeParamValue * 200.0f));
                valStr = juce::String (manualBpm) + " BPM"; // Free-running BPM numerical display [1.2.3]
            }
        }
        else if (activeParamName == "Octaves")
        {
            int octVal = static_cast<int> (std::round (activeParamValue * 6.0f - 3.0f));
            valStr = (octVal >= 0 ? "+" : "") + juce::String (octVal); // Signed integer display [1.2.3]
        }
        else
        {
            valStr = juce::String (static_cast<int> (std::round (activeParamValue * 100.0f))) + "%"; // Standard percentage
        }
        
        g.setColour (themeColor);
        g.setFont (juce::FontOptions ("Courier New", 26.0f, juce::Font::bold));
        g.drawText (valStr, displayArea.getX(), contentY - 6.0f, displayArea.getWidth() - 10.0f, 30, juce::Justification::right);

        // 5. Render Dual Horizontal Segmented LED Bars
        float baseBarY = contentY + 28.0f;
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.setFont (juce::FontOptions ("Courier New", 10.0f, juce::Font::bold));
        g.drawText ("[BASE]", displayArea.getX() + 10, baseBarY, 50, 12, juce::Justification::left);

        float barStartX = displayArea.getX() + 70.0f;
        int numSegs = 30;
        float segW = 10.0f;
        float segH = 8.0f;
        float segG = 2.0f;
        
        // Render top BASE segment bar
        int litSegsBase = static_cast<int> (std::round (activeParamValue * numSegs));
        for (int s = 0; s < numSegs; ++s)
        {
            float sx = barStartX + s * (segW + segG);
            auto segRect = juce::Rectangle<float> (sx, baseBarY + 2.0f, segW, segH);
            if (s < litSegsBase)
                g.setColour (themeColor);
            else
                g.setColour (juce::Colour (0xFF11141C).withAlpha (0.6f)); // Faint unlit segment
            
            g.fillRect (segRect);
        }

        // Render bottom LFO segment bar
        float lfoBarY = baseBarY + 16.0f;
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText ("[LFO ]", displayArea.getX() + 10, lfoBarY, 50, 12, juce::Justification::left);

        // If LFO is disabled (activeLfoVibe == "Off"), litSegsLfo is 0 and bottom bar reads completely empty
        int litSegsLfo = (activeLfoVibe == "Off") ? 0 : static_cast<int> (std::round (lfoProgress * numSegs));
        for (int s = 0; s < numSegs; ++s)
        {
            float sx = barStartX + s * (segW + segG);
            auto segRect = juce::Rectangle<float> (sx, lfoBarY + 2.0f, segW, segH);
            if (s < litSegsLfo)
                g.setColour (themeColor.interpolatedWith (juce::Colours::white, 0.25f)); // Pulsing core
            else
                g.setColour (juce::Colour (0xFF11141C).withAlpha (0.6f)); // Faint unlit segment
            
            g.fillRect (segRect);
        }

        // 6. Draw visual framing frames at the bottom
        float frameY = lfoBarY + 26.0f;
        float frameH = 48.0f;
        
        // Box A: Focus targeted details
        juce::Rectangle<float> boxA (displayArea.getX() + 10.0f, frameY, 280.0f, frameH);
        g.setColour (themeColor.withAlpha (0.10f));
        g.fillRoundedRectangle (boxA, 2.0f);
        g.setColour (themeColor.withAlpha (0.35f));
        g.drawRoundedRectangle (boxA, 2.0f, 1.0f);
        
        g.setColour (juce::Colours::white.withAlpha (0.80f));
        g.setFont (juce::FontOptions ("Courier New", 9.0f, juce::Font::bold));
        juce::String sideText = processor.isSceneBActiveAnchor.load() ? "SCENE B (FOCUSED)" : "SCENE A (FOCUSED)";
        g.drawText (" TARGET SNAP : " + sideText, boxA.getX() + 5.0f, boxA.getY() + 8.0f, boxA.getWidth() - 10.0f, 12, juce::Justification::left);
        g.drawText (" INPUT DRAG  : USER CAPTURE ACTIVE", boxA.getX() + 5.0f, boxA.getY() + 24.0f, boxA.getWidth() - 10.0f, 12, juce::Justification::left);

        // Box B: Sat LFO metrics
        juce::Rectangle<float> boxB (displayArea.getX() + 300.0f, frameY, displayArea.getWidth() - 310.0f, frameH);
        g.setColour (themeColor.withAlpha (0.10f));
        g.fillRoundedRectangle (boxB, 2.0f);
        g.setColour (themeColor.withAlpha (0.35f));
        g.drawRoundedRectangle (boxB, 2.0f, 1.0f);
        
        juce::String lfoStatusText = (activeLfoVibe == "Off") ? "ROUTING INACTIVE" : "SATELLITE LFO SYNCED";
        g.drawText (" LFO ROUTING : " + lfoStatusText, boxB.getX() + 5.0f, boxB.getY() + 8.0f, boxB.getWidth() - 10.0f, 12, juce::Justification::left);
        g.drawText (" TEMPO RATE  : " + activeLfoVibe, boxB.getX() + 5.0f, boxB.getY() + 24.0f, boxB.getWidth() - 10.0f, 12, juce::Justification::left);

        // 7. Draw clean high-tech corner brackets around the viewport boundaries
        g.setColour (themeColor.withAlpha (0.5f));
        float thick = 2.0f;
        float size = 8.0f;
        auto drawBracket = [&](float cx, float cy, bool left, bool top)
        {
            float hDir = left ? 1.0f : -1.0f;
            float vDir = top ? 1.0f : -1.0f;
            g.fillRect (cx, cy, hDir * size, thick * vDir);
            g.fillRect (cx, cy, thick * hDir, vDir * size);
        };
        drawBracket (displayArea.getX(), displayArea.getY(), true, true);
        drawBracket (displayArea.getRight(), displayArea.getY(), false, true);
        drawBracket (displayArea.getX(), displayArea.getBottom(), true, false);
        drawBracket (displayArea.getRight(), displayArea.getBottom(), false, false);
    }
    else
    {
        float morphVal = *processor.apvts.getRawParameterValue (IDs::morph.getParamID());
        const int activeStep = processor.currentStep.load();          
        const bool isPlaying = processor.isCurrentlyPlayingUI.load(); 

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

        // Continuous Float Rate dial dynamically evaluated inside metadata bar [1.2.3]
        float rawRateVal = *processor.apvts.getRawParameterValue (IDs::rate.getParamID());
        int rateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRateVal * 3.0f)));
        juce::StringArray rates { "1/4", "1/8", "1/16", "1/32" };
        juce::String rateStr = rates[rateIdx];

        int rangeShift = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
        juce::String octStr = (rangeShift >= 0 ? "+" : "") + juce::String (rangeShift);

        // Check active tempo source sync mode [1.2.3]
        auto* syncPtr = processor.apvts.getRawParameterValue ("sync");
        bool syncActive = (syncPtr != nullptr && syncPtr->load() > 0.5f);
        juce::String tempoSourceText = syncActive ? "SYNC (" + rateStr + ")" : "FREE RUN";

        juce::String metaText = "SYSTEM STATUS: ACTIVE | KEY: " + keyStr + " | SCALE: " + scaleStr.toUpperCase() + " | TEMPO: " + tempoSourceText + " | VOICING: " + voiceStr;

        struct Point3D { float x, y, z; };
        std::vector<Point3D> vertices;

        vertices.push_back ({ 0.0f, 1.0f, 0.0f }); 
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
        vertices.push_back ({ 0.0f, -1.0f, 0.0f }); 

        double timeMs = juce::Time::getMillisecondCounterHiRes();
        float yaw = static_cast<float> (timeMs * 0.00010);
        float pitch = static_cast<float> (timeMs * 0.00004);

        auto rotatePoint = [&](Point3D p, float yAngle, float pAngle) -> Point3D
        {
            float x1 = p.x * std::cos (yAngle) - p.z * std::sin (yAngle);
            float z1 = p.x * std::sin (yAngle) + p.z * std::cos (yAngle);
            float y2 = p.y * std::cos (pAngle) - z1 * std::sin (pAngle);
            float z2 = p.y * std::sin (pAngle) + z1 * std::cos (pAngle);
            return { x1, y2, z2 };
        };

        float globeCenterX = displayArea.getCentreX(); 
        float globeCenterY = displayArea.getY() + 110.0f; // Adjusted slightly upward for larger globe (was + 120.0f)
        float globeRadius = displayArea.getHeight() * 0.38f; // Sized up from 0.28f to 0.38f for bigger globe  
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

        auto* cameoVar = getProperties().getVarPointer ("cameoState");
        CameoState* state = nullptr; 

        if (cameoVar == nullptr || cameoVar->getObject() == nullptr)
        {
            auto* newState = new CameoState();
            newState->lastCameoTriggerTime = timeMs;
            
            juce::Random randInit;
            newState->nextCameoInterval = 120000.0 + randInit.nextDouble() * 120000.0; 
            getProperties().set ("cameoState", newState);
            state = newState;
        }
        else
        {
            state = dynamic_cast<CameoState*> (cameoVar->getObject());
        }

        juce::Colour lineColour     = juce::Colour::fromString ("#FF0066FF").withAlpha (0.24f); 
        juce::Colour nodeGlowColour = juce::Colour::fromString ("#FF00E1FF");                  
        juce::Colour nodeCoreColour = juce::Colour::fromString ("#FF80F3FF");                  

        if (isPlaying && state != nullptr)
        {
            juce::Random triRand (static_cast<int64_t> (timeMs));

            juce::Colour colors[] = {
                juce::Colour::fromString ("#FFFF3366"), 
                juce::Colour::fromString ("#FFFF8D11"), 
                juce::Colour::fromString ("#FF00FF66"), 
                juce::Colour::fromString ("#FF0055FF")  
            };
            double speeds[] = { 533.0, 1200.0, 1600.0, 8000.0 }; 

            if (! state->isInitialized)
            {
                state->isInitialized = true;
                for (int i = 0; i < 4; ++i)
                {
                    state->triangles[i].v1 = triRand.nextInt (62);
                    state->triangles[i].v2 = (state->triangles[i].v1 + 1) % 62;
                    state->triangles[i].v3 = (state->triangles[i].v1 + 12) % 62;
                    state->triangles[i].lastTeleportTime = timeMs - (triRand.nextDouble() * 300.0);
                    state->triangles[i].currentPeriod = speeds[i];
                    state->triangles[i].colour = colors[i];
                    state->triangles[i].isActive = (triRand.nextFloat() < 0.55f); 
                }
            }

            for (int i = 0; i < 4; ++i)
            {
                auto& tri = state->triangles[i];

                if (timeMs - tri.lastTeleportTime >= tri.currentPeriod)
                {
                    tri.lastTeleportTime = timeMs;
                    tri.v1 = triRand.nextInt (62);
                    tri.v2 = (tri.v1 + 1) % 62;
                    tri.v3 = (tri.v1 + 12) % 62;
                    tri.currentPeriod = speeds[triRand.nextInt (4)];
                    tri.colour = colors[triRand.nextInt (4)];
                    tri.isActive = (triRand.nextFloat() < 0.55f);
                }

                if (tri.isActive)
                {
                    float frequencyFactor = static_cast<float> (500.0 / tri.currentPeriod);
                    float flicker = static_cast<float> (std::sin (timeMs * 0.05f * frequencyFactor)) * 0.4f + 0.6f;

                    juce::Path triPath;
                    triPath.startNewSubPath (projectedPoints[tri.v1]);
                    triPath.lineTo (projectedPoints[tri.v2]);
                    triPath.lineTo (projectedPoints[tri.v3]);
                    triPath.closeSubPath();

                    g.setColour (tri.colour.withAlpha (0.22f * flicker));
                    g.fillPath (triPath);
                }
            }
        }

        if (state != nullptr)
        {
            for (size_t i = 0; i < projectedPoints.size(); ++i)
            {
                float nodeAlpha = 0.35f; 
                
                int lfoIdx = -1;
                if (i == 0 || i == 61) lfoIdx = 0;       
                else if (i >= 1 && i <= 12) lfoIdx = 1;   
                else if (i >= 13 && i <= 24) lfoIdx = 2;  
                else if (i >= 25 && i <= 36) lfoIdx = 3;  
                else if (i >= 37 && i <= 48) lfoIdx = 4;  
                else if (i >= 49 && i <= 60) lfoIdx = 5;  

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
                            float mod = static_cast<float> (std::sin (currentPhase * juce::MathConstants<double>::twoPi)) * depth * 0.5f + 0.5f;
                            nodeAlpha = juce::jlimit (0.15f, 1.0f, nodeAlpha + mod * 0.6f);
                        }
                    }
                }

                g.setColour (nodeGlowColour.withAlpha (nodeAlpha * 0.65f));
                g.fillEllipse (projectedPoints[i].x - 2.5f, projectedPoints[i].y - 2.5f, 5.0f, 5.0f);

                g.setColour (nodeCoreColour.withAlpha (nodeAlpha));
                g.fillEllipse (projectedPoints[i].x - 1.0f, projectedPoints[i].y - 1.0f, 2.0f, 2.0f);
            }
        }

        g.setColour (lineColour);
        auto drawEdge = [&](int idx1, int idx2)
        {
            g.drawLine (projectedPoints[idx1].x, projectedPoints[idx1].y,
                        projectedPoints[idx2].x, projectedPoints[idx2].y, 0.75f);
        };
        for (int i = 1; i <= 12; ++i) drawEdge (0, i); 
        for (int ringIdx = 0; ringIdx < 4; ++ringIdx)
        {
            int offset1 = 1 + ringIdx * 12;
            int offset2 = 1 + (ringIdx + 1) * 12;
            for (int i = 0; i < 12; ++i) 
            {
                drawEdge (offset1 + i, offset1 + (i + 1) % 12);
                drawEdge (offset1 + i, offset2 + i);
                drawEdge (offset1 + i, offset2 + (i + 1) % 12);
                drawEdge (offset1 + (i + 1) % 12, offset2 + i);
            }
        }
        int offset5 = 49;
        for (int i = 0; i < 12; ++i)
        {
            drawEdge (offset5 + i, offset5 + (i + 1) % 12);
            drawEdge (offset5 + i, 61); 
        }

        if (state != nullptr)
        {
            if (timeMs - state->lastCameoTriggerTime >= state->nextCameoInterval)
            {
                state->lastCameoTriggerTime = timeMs;
                
                juce::Random randEngine;
                state->nextCameoInterval = 120000.0 + randEngine.nextDouble() * 120000.0; 
                
                int spawnCount = (randEngine.nextFloat() < 0.20f) ? 2 : 1; 
                for (int k = 0; k < spawnCount; ++k)
                {
                    CameoState::ActiveCameo cameo;
                    cameo.type = randEngine.nextInt (3); 
                    cameo.startTimeMs = timeMs;
                    cameo.durationMs = 5000.0 + randEngine.nextDouble() * 7000.0; 
                    cameo.trajectoryPattern = randEngine.nextInt (3); 
                    cameo.arcAmplitude = 15.0f + randEngine.nextFloat() * 25.0f;
                    
                    cameo.startX = (k == 1 || randEngine.nextBool()) ? -30.0f : (width + 30.0f);
                    cameo.targetX = (cameo.startX < 0.0f) ? (width + 30.0f) : -30.0f;
                    cameo.startY = displayArea.getY() + randEngine.nextFloat() * displayArea.getHeight() * 0.7f;
                    cameo.targetY = displayArea.getY() + randEngine.nextFloat() * displayArea.getHeight() * 0.7f;
                    
                    state->activeCameos.push_back (cameo);
                }
            }

            for (auto it = state->activeCameos.begin(); it != state->activeCameos.end();)
            {
                double elapsed = timeMs - it->startTimeMs;
                float progress = static_cast<float> (elapsed / it->durationMs);

                if (progress >= 1.0f)
                {
                    it = state->activeCameos.erase (it); 
                }
                else
                {
                    float camX = it->startX + (it->targetX - it->startX) * progress;
                    float camY = it->startY + (it->targetY - it->startY) * progress;

                    if (it->trajectoryPattern == 1) 
                    {
                        camY -= std::sin (progress * juce::MathConstants<float>::pi) * it->arcAmplitude;
                    }
                    else if (it->trajectoryPattern == 2) 
                    {
                        camY += std::sin (progress * juce::MathConstants<float>::pi * 6.0f) * 8.0f;
                    }

                    if (it->type == 0) // Voyager
                    {
                        g.setColour (juce::Colours::lightgrey.withAlpha (0.75f));
                        g.drawLine (camX - 10.0f, camY + 4.0f, camX + 10.0f, camY - 4.0f, 0.75f);
                        g.setColour (juce::Colours::white.withAlpha (0.90f));
                        g.fillEllipse (camX - 4.0f, camY - 4.0f, 8.0f, 8.0f);
                        g.setColour (juce::Colour (0xFF05070A));
                        g.fillEllipse (camX - 2.5f, camY - 2.5f, 5.0f, 5.0f);
                        g.setColour (juce::Colours::white);
                        g.drawLine (camX, camY, camX + 6.0f, camY - 6.0f, 1.25f);
                    }
                    else if (it->type == 1) // James Webb
                    {
                        juce::Path shieldPath;
                        shieldPath.startNewSubPath (camX - 12.0f, camY + 2.0f);
                        shieldPath.lineTo (camX, camY - 5.0f);
                        shieldPath.lineTo (camX + 12.0f, camY + 2.0f);
                        shieldPath.lineTo (camX, camY + 6.0f);
                        shieldPath.closeSubPath();
                        g.setColour (juce::Colours::grey.withAlpha (0.55f));
                        g.fillPath (shieldPath);

                        g.setColour (juce::Colour (0xFFFFB300).withAlpha (0.90f)); 
                        g.fillEllipse (camX - 3.5f, camY - 3.0f, 7.0f, 6.0f);
                        g.setColour (juce::Colours::white.withAlpha (0.6f));
                        g.drawEllipse (camX - 3.5f, camY - 3.0f, 7.0f, 6.0f, 0.75f);
                    }
                    else if (it->type == 2) // UFO
                    {
                        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.75f)); 
                        g.fillEllipse (camX - 9.0f, camY - 3.0f, 18.0f, 6.0f);
                        g.setColour (juce::Colours::white);
                        g.fillEllipse (camX - 4.5f, camY - 6.0f, 9.0f, 4.5f); 
                        g.setColour (juce::Colour (0xFFFF3366)); 
                        g.fillEllipse (camX - 6.0f, camY, 1.5f, 1.5f);
                        g.fillEllipse (camX, camY + 1.0f, 1.5f, 1.5f);
                        g.fillEllipse (camX + 4.5f, camY, 1.5f, 1.5f);
                    }
                    ++it;
                }
            }
        }

        // REMOVED program-stamp drawn inside the OLED box (NAVY-ARP MONITOR title is now stamped on parent editor) [1.2.3]

        // Shifted status bar parameters to absolute top of OLED inside box boundary
        g.setColour (juce::Colour (0xFF00FF66).withAlpha (0.85f)); 
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (metaText, displayArea.withY (displayArea.getY() + 8.0f).withHeight (15.0f), juce::Justification::centred, true);

        const int numSegments = 16;
        const float segmentHeight = 5.0f;      // Halved segment height (was 10.0f)
        const float segmentSpacing = 1.0f;     // Closer segment spacing (was 2.0f)
        const float maxLaddersHeight = (numSegments * segmentHeight) + ((numSegments - 1) * segmentSpacing); 
        float fadersY = bounds.getHeight() - maxLaddersHeight; // Ends exactly at the bottom border of the OLED box (320.0f)

        // Continuous, fat, gapless columns completely filling the 680px width (8 columns * 85px = 680px)
        const float colWidth = 85.0f;
        const float spacing = 0.0f;
        const float startX = 0.0f;

        for (int i = 0; i < 8; ++i)
        {
            float colX = startX + static_cast<float> (i) * (colWidth + spacing);
            auto colBounds = juce::Rectangle<float> (colX, fadersY, colWidth, maxLaddersHeight);
            
            float faderVal = (processor.sceneA.faders[i] * (1.0f - morphVal)) + (processor.sceneB.faders[i] * morphVal);
            int activeSegments = static_cast<int> (std::round (faderVal * static_cast<float> (numSegments)));

            const bool isActiveStep = isPlaying && (i == activeStep);

            for (int seg = 0; seg < numSegments; ++seg)
            {
                float segY = colBounds.getY() + maxLaddersHeight - ((seg + 1) * (segmentHeight + segmentSpacing));
                auto segmentRect = juce::Rectangle<float> (colBounds.getX() + 2.0f, segY, colBounds.getWidth() - 4.0f, segmentHeight);

                if (seg < activeSegments)
                {
                    if (isActiveStep)
                        g.setColour (juce::Colour (0xFFFF4500)); 
                    else
                        g.setColour (juce::Colour (0xFF441105).withAlpha (0.75f)); 
                }
                else
                {
                    g.setColour (juce::Colour (0xFF111317).withAlpha (0.65f));
                }

                g.fillRect (segmentRect);
            }
        }
    }
}

void OledDisplay::resized()
{
}