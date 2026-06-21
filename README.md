# navy-arp

`navy-arp` is a modern, performative generative MIDI arpeggiator and step sequencer built with C++ and the JUCE 8 framework. 

Unlike traditional arpeggiators that rely on tedious grid drawing or tracker-style tables, `navy-arp` is designed as a hands-on, modular-inspired performance utility. It combines scale-quantized note probability with a real-time parameter-morphing crossfader, making it highly expressive and optimized for immediate MIDI controller mapping.

---

## Key Features

### 1. The Core Brain (Scale-Degree Probability)
*   **8 Scale-Degree Faders:** Instead of chromatic notes, the faders represent musical scale degrees (1st through 8th notes of your chosen key/scale). This ensures that every fader is always musically active with zero dead space.
*   **Dynamic Note Reader:** Visual fader labels automatically update in real-time to show the exact musical notes of your selected key (e.g., C, Eb, G).
*   **Ghost Glow Overlay:** When notes are played on your MIDI keyboard, a smooth visual glow fades in behind the active fader tracks, indicating which notes are currently being held.

### 2. Symmetrical Sidebar Modules
*   **Rhythm Module (Left):**
    *   **Rhythm Morph:** A single knob that morphs your groove from a straight, steady 1/16th pulse to a highly syncopated, stochastic shifting clock (including triplets and dotted notes).
    *   **Rest & Legato:** Fine-tune step mute probability and note gate lengths on the fly.
    *   **Latch Mode:** Lock your chord memory so the sequencer loops endlessly, allowing for fully hands-free performance.
*   **Harmony & Chaos Module (Right):**
    *   **Entropy:** Introduces a gradual "tape wear" loop decay that slowly mutates your sequence over time.
    *   **Harmony:** A stochastic polyphony control that dynamically adds random, scale-correct 2-to-3-note chords and pad layers on top of your melody.
    *   **Chaos:** Controls pitch drift, randomly shifting active notes up or down an octave.
    *   **DICE MELODY & DICE RHYTHM:** Dedicated momentary randomizer buttons to instantly generate fresh patterns within your probability limits.

### 3. High-Quality OLED display (Center)
*   Designed to mimic premium boutique hardware displays.
*   **Active Step Playhead:** Vertical bars representing the active steps pulse instantly on note-triggers with sharp, retro neon-bloom edges, decaying cleanly back to absolute black (`#000000`).
*   **Interactive Keyboard Brackets:** Two draggable bracket handles display the Lowest Note and Highest Note boundaries of your active pitch zone.

### 4. Scene Morphing (The Octatrack Crossfader)
*   A master horizontal crossfader that smoothly interpolates every slider, knob, and probability state between a custom **Scene A** (cyan glow) and **Scene B** (amber glow).
*   Moving the crossfader glides and rotates all on-screen sliders and knobs in real-time, allowing for dramatic, sweeping transitions with zero abrupt parameter jumps.

### 5. OLED Preset Slots
*   **8 Tactile Recall Slots:** Clicking a numbered block instantly recalls that preset.
*   **Hold-to-Save:** Holding down a slot button for **2.0 seconds** writes your current settings to that memory slot, triggering a sharp neon-blue outer-edge glow indicating the slot contains saved data.

---

## How to Compile (Cloud CI/CD Pipeline)

`navy-arp` is built on top of the modern **Pamplejuce** CMake template. You do not need to install local compilers or IDEs to build it.

1.  Commit and Push any code changes to your repository.
2.  GitHub Actions will automatically spin up virtual Windows environments in the cloud.
3.  Once the build is complete (approx. 2 minutes), go to your **Actions** tab on GitHub.
4.  Scroll to the **Artifacts** section at the bottom of the completed build to download your ready-to-run **VST3 installer (.exe)**.

---

## DAW Setup & MIDI Routing (Ableton Live)

`navy-arp` is compiled as a VST3 Instrument to allow stable multi-track routing in Ableton Live on Windows:

1.  Create a MIDI Track (**`2-MIDI`**) and load **Pamplejuce Demo** (navy-arp) onto it.
2.  Create your software synthesizer track (**`1-Analog Piano`** or any other virtual instrument).
3.  On your synthesizer track, set the **MIDI From** dropdown list to **`2-MIDI`**.
4.  Directly below that, change the second dropdown list from `Post FX` to **`Pamplejuce Demo`**.
5.  Set the **Monitor** on your synthesizer track to **`In`** (orange).
6.  Select your `2-MIDI` track, hold down a chord on your keyboard, and enjoy your generative arpeggiations!
