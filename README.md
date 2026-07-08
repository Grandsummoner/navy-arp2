# Navy Arp 2

**Navy Arp 2** is a hardware-inspired, 8-step MIDI arpeggiator and step sequencer designed for expressive real-time performance, parameter morphing, and tactile studio experimentation. 

Combining the fluid workflow of classic hardware sequencers with deep digital precision, Navy Arp 2 provides a visual and highly interactive playground for generating intricate melodies, rhythmic modulations, and unexpected grooves inside your DAW.

![Module Graphic](./res/Navy-Arp2.png)

---

## What Makes Navy Arp 2 Unique?

### 1. Dual-Scene Snapshot Morphing (A/B)
Instead of forcing you to stick to a single static pattern, Navy Arp 2 lets you construct two entirely different worlds at the same time:
* **Two Scenes in Memory:** Program one sequence inside **Scene A** (e.g., tight, low-prob, high-staccato notes) and an entirely different sequence inside **Scene B** (e.g., polyphonic, long legato chords with heavy swing).
* **Elektron-Style Workflow:** Simply click the **A** or **B** buttons (which glow red to indicate focus) to select your target. Any knob tweaks, upfader slides, or generative randomizations you make will write *only* to the currently focused scene.
* **The Morph Crossfader:** Gently slide the central horizontal fader to smoothly blend every single parameter between Scene A and Scene B. This creates an infinite range of hybrid sequences in between your two snapshots.
* **Motorized Dial Release:** When you grab and tweak a knob, you edit its raw base value in the active scene. The moment you let go of your mouse, the knob smoothly glides back like a motorized spring to match the crossfader's current interpolated position.

### 2. Boutique Hardware Design
Navy Arp 2 moves away from typical flat software layouts to deliver a clean, physical synth console aesthetic:
* **Pristine Metal Caps:** Knobs are designed as clean, solid silver metal caps, removing distracting drawn pointer needles and pre-baked highlights.
* **Single-Point LED Rings:** Value positions are shown by a single, sharp glowing LED dot on the circular ring, providing high-contrast visual placement that mimics luxury infinite rotary encoders.
* **"Corona" LFO Backlighting:** Every small parameter has a built-in LFO (Low-Frequency Oscillator) that can be accessed via a simple **right-click**. When active, a soft colored ring of light leaks out from underneath the base of the knob cap:
  * The backlighting breathes in brightness matching the LFO speed.
  * Its maximum intensity matches the LFO depth.
  * Subtle LFO depths (such as 10%) are mathematically boosted to remain easily visible on screen, so you can always tell when subtle automated modulation is taking place.
* **Logical Symmetric Division:** Left-side controls (Time/Rhythm) use a cool theme accent color, while right-side controls (Pitch/Melody) use a high-contrast color, making the panel split highly intuitive.

### 3. Dynamic OLED Screen
The central monitor window keeps you informed of the engine's status through an advanced multi-mode screen:
* **The 3D Space Viewport:** In its default idle state, a rotating 3D wireframe globe floats in space, morphing its geometry in real-time to active LFO modulation shapes. Bouncing beneath it are 8 thick, gapless vertical probability towers utilizing retro phosphor-decay style rendering.
* **Smart Parameter HUD Overlay:** The moment you touch any dial, the 3D globe dims and a snappy technical Heads-Up-Display overrides the viewport. It displays the active parameter name, an oversized digital readout, and dual 45-segment linear LED meters comparing your raw `[BASE]` setting against your active `[LFO]` swing limits.
* **Intelligent Value Formatting:** Rather than showing generic percentages everywhere, the OLED formats numerical readouts depending on what you adjust (e.g., displaying musical beat divisions like `1/16` or `1/8` when synced, actual speed like `154 BPM` when free-running, signed intervals like `+1` or `-2` for octaves, and clean percentages for relative values).

### 4. Live Preset Surfing & Generative Dice
* **Performance Memory Bank:** Save your best configurations into 8 quick-recall slots. Toggle **Recall** to latch it, allowing you to instantly swap/surf presets mid-song during a live set with zero lag.
* **2x2 Generative "Dice" Grid:** Instant, high-quality pattern generators let you roll randomized ideas with a single click:
  * **Melo:** Generates randomized step probabilities.
  * **Arti:** Randomizes note lengths (Legato) and silent steps (Rest).
  * **Time:** Randomizes speed, division rates, and octave shifts.
  * **Navy:** Adjusts and randomizes rhythm-morphing and chaos rules.
* **Note Density (DEN):** A dedicated master control scales step probabilities. Turning it under 50% thins out the notes to create breathing room, while pushing it past 50% fills empty steps to generate complex drum-rolls and fills.

### 5. Seamless Multi-Theme Support
Switch between three visual themes dynamically to match your environment:
* **Navy:** Vibrant Teal indicators with glowing Warm Amber LFO backlights.
* **Monochrome:** Pure Silver-White elements with glowing Tech Cyan LFO backlights.
* **Matrix:** Glowing Neon Green indicators with Hot Magenta LFO backlights.

---

## DAW Integration & System Stability

* **Reliable State Recall:** Built to resolve common session recall issues on DAW restart, Navy Arp 2 saves the active state of both Scene A and Scene B, your global bank presets, and your active LFO speeds, ensuring your project loads exactly as you left it—including tested stability under **Ableton Live**.
* **Collapsible Sidecar Cheat Sheet:** Click the `?` icon in the top-right corner to temporarily expand the plugin window. A clean, easy-to-read, monospace manual slips out on the right with colored "Island Pill" headers (Coral, Violet, Cyan, Amber) to give you a quick-start guide on the fly. Click the toggle again to collapse it.

---

## Installation & Setup

1. Copy the `Navy Arp.vst3` file into your system's standard VST3 directory:
   * **Windows:** `C:\Program Files\Common Files\VST3\`
2. Rescan plugins inside your DAW.
3. Insert **Navy Arp 2** on a MIDI track, route its MIDI output to your favorite software synthesizer, and begin sequencing!
