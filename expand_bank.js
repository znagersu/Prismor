/**
 * expand_bank.js
 * Generates an expanded factory preset bank (172 patches) for Prismor 1.0.8.4
 */

const fs = require('fs');
const path = require('path');

const srcFile = path.join(__dirname, 'generate_presets.js');
let code = fs.readFileSync(srcFile, 'utf8');

// The new patches to append to each array
const newBass = `
  { name: "BASS // 16x Detuned Reese Monolith", waves: "SAW + SAW", fx: "16x Unison + SVF 24dB + Tape", desc: "Wall of sound 16-voice detuned neuro Reese with maximum stereo width and tape drive.",
    wA: 0, wB: 0, octA: -1, octB: -1, vMode: 1, uV: 16, uDet: 0.38, uSpr: 1.0, uBl: 0.75, f1T: 1, f1C: 0.44, f1R: 0.22, distE: true, distT: 1, distD: 0.38, mDb: -3.5 },
  { name: "BASS // Ring Mod Metallic 808", waves: "SINE + X-MOD", fx: "Ring Mod + Tube Drive + Punch", desc: "Deep sine sub reinforced with 140Hz ring modulated upper harmonic crunch.",
    wA: 6, wB: 9, octA: -2, octB: -1, vMode: 1, ringE: true, ringMode: 0, ringFreq: 140.0, ringD: 0.60, ringM: 0.45, distE: true, distT: 0, distD: 0.32, compE: true, compM: 0.70, mDb: -1.5 },
  { name: "BASS // Lo-Fi SP-1200 Crunch", waves: "SQR + TRI", fx: "SP-1200 Lo-Fi + Crude LP", desc: "Gritty vintage 12-bit sampler emulation with warm punchy low-end bite.",
    wA: 1, wB: 2, octA: -1, octB: -1, vMode: 1, crushE: true, crushMode: 2, crushBits: 7.0, crushDown: 4.5, crushM: 0.55, f1T: 2, f1C: 0.50, f1R: 0.18, mDb: -1.0 },
  { name: "BASS // Gross Beat Stutter Sub", waves: "SINE + SYNC", fx: "Time Mod 1/8 Stutter + OTT", desc: "Rhythmic buffer-chopped sub bass with automated 1/8 stutter slices.",
    wA: 6, wB: 7, octA: -2, octB: -1, vMode: 1, timeModE: true, timeModMode: 1, timeModRate: 4, timeModD: 0.70, timeModM: 0.75, compE: true, compM: 0.72, mDb: -2.0 },
  { name: "BASS // Talkbox Growl Bass", waves: "SAW + FMT", fx: "Talkbox Mode + Asym Scream", desc: "Vocal formant talkbox growl bass designed for heavy bass music and dubstep.",
    wA: 0, wB: 4, octA: -1, octB: -1, vMode: 1, uV: 4, uDet: 0.24, f1T: 9, f1C: 0.62, f1R: 0.48, distE: true, distT: 5, distD: 0.45, compE: true, compM: 0.68, mDb: -2.5 },
  { name: "BASS // Cyber Jet Flange Bass", waves: "SYNC + SAW", fx: "Jet Flanger + Hard Clip", desc: "Aggressive sync bass swept with high-resonance jet flanging and clipper drive.",
    wA: 7, wB: 0, octA: -1, octB: -1, warpA: 5, warpAmtA: 0.50, vMode: 1, uV: 6, uDet: 0.22, flangerE: true, flangerM: 0.45, distE: true, distT: 2, distD: 0.50, mDb: -3.0 },
  { name: "BASS // Acid Screamer 303", waves: "SAW + SQR", fx: "SVF 24dB Reso + Overdrive", desc: "Classic squelchy 303 acid line with screaming resonance and fast slide portamento.",
    wA: 0, wB: 1, octA: -1, octB: -1, vMode: 2, portaTime: 0.22, f1T: 1, f1C: 0.46, f1R: 0.82, distE: true, distT: 0, distD: 0.55, mDb: -2.0 },
  { name: "BASS // Deep Sine Sub Core", waves: "SINE + SINE", fx: "Clean Sub Limiter + Warmth", desc: "Pure sub-bass foundation tuned for clean room playback with zero phase cancellation.",
    wA: 6, wB: 6, octA: -2, octB: -2, vMode: 1, f1T: 0, f1C: 0.38, f1R: 0.05, mDb: 0.0 }
`;

const newLeads = `
  { name: "LEAD // 16x Hyper Unison Supersaw", waves: "SAW + SAW", fx: "16x Unison + Octave + OTT", desc: "Colossal 16-voice stereo detuned stadium supersaw with +1 octave stack and shimmer tail.",
    wA: 0, wB: 0, octA: 0, octB: 1, uV: 16, uDet: 0.38, uSpr: 1.0, uBl: 0.72, uStack: 1, compE: true, compM: 0.80, reverbE: true, reverbSize: 0.88, reverbM: 0.35, reverbShim: 0.40, mDb: -4.0 },
  { name: "LEAD // Barberpole Flanged Hero", waves: "SAW + SYNC", fx: "Barberpole Flanger + Delay", desc: "Infinite ascending barberpole flanger modulation over bright cutting saw solo.",
    wA: 0, wB: 7, octA: 0, octB: 0, uV: 6, uDet: 0.24, flangerE: true, flangerMode: 2, flangerM: 0.50, delayE: true, delayT: 0.375, delayM: 0.28, mDb: -2.5 },
  { name: "LEAD // Talkbox Scream Solo", waves: "SAW + FMT", fx: "Talkbox Mode + Tube Sat + Delay", desc: "Expressive vocal talkbox lead with keyboard tracking and analog tape echo.",
    wA: 0, wB: 4, octA: 0, octB: -1, vMode: 2, portaTime: 0.22, uV: 4, uDet: 0.20, f1T: 9, f1C: 0.70, f1R: 0.42, distE: true, distT: 0, distD: 0.38, delayE: true, delayM: 0.26, mDb: -2.0 },
  { name: "LEAD // Through-Zero Sync Lead", waves: "SYNC + X-MOD", fx: "TZ Flanger + Dimension Chorus", desc: "Complex harmonic sync wave running through through-zero cancellation flanging.",
    wA: 7, wB: 9, octA: 0, octB: 0, warpA: 5, warpAmtA: 0.55, uV: 8, uDet: 0.26, flangerE: true, flangerMode: 1, flangerM: 0.52, chorusE: true, chorusM: 0.30, mDb: -3.0 },
  { name: "LEAD // 80s Miami Sunset Solo", waves: "TRI + SAW", fx: "True Legato + Plate Reverb", desc: "Silky smooth 80s synth solo with gentle pitch vibrato and warm analog plate.",
    wA: 2, wB: 0, octA: 0, octB: 0, vMode: 2, portaTime: 0.28, vibD: 0.25, vibS: 5.6, f1T: 1, f1C: 0.68, f1R: 0.22, reverbE: true, reverbM: 0.26, mDb: -1.5 },
  { name: "LEAD // Zenology Vowel Singer", waves: "FMT + SAW", fx: "Vox Formant + OTT Comp", desc: "Dynamic singing vowel formant lead articulating vowels across the keyboard.",
    wA: 4, wB: 0, octA: 0, octB: 0, vMode: 2, portaTime: 0.18, f1T: 6, f1C: 0.72, f1R: 0.48, compE: true, compM: 0.72, delayE: true, delayM: 0.25, mDb: -2.2 },
  { name: "LEAD // Gross Beat Halftime Lead", waves: "SAW + SQR", fx: "Time Mod 1/2x + Hall Reverb", desc: "Modern trap & hip-hop half-time time-stretched melodic lead.",
    wA: 0, wB: 1, octA: 0, octB: 0, timeModE: true, timeModMode: 4, timeModRate: 1, timeModD: 0.85, timeModM: 0.80, reverbE: true, reverbM: 0.32, mDb: -2.5 },
  { name: "LEAD // Piercing Sync Screamer", waves: "SYNC + SQR", fx: "Hard Clip + Jet Flanger", desc: "High-octane sync lead designed to cut through dense bass-heavy arrangements.",
    wA: 7, wB: 1, octA: 1, octB: 0, warpA: 5, warpAmtA: 0.65, vMode: 2, portaTime: 0.15, distE: true, distT: 2, distD: 0.52, flangerE: true, flangerM: 0.38, mDb: -2.8 }
`;

const newPads = `
  { name: "PAD // 16x Unison Horizon Pad", waves: "SAW + SAW", fx: "16x Unison + Shimmer Hall", desc: "Gigantic cinematic soundscape with 16 stereo detuned voices and cosmic shimmer tail.",
    wA: 0, wB: 0, octA: 0, octB: 1, uV: 16, uDet: 0.32, uSpr: 1.0, uBl: 0.70, envA: 0.45, envD: 0.65, envS: 0.88, envR: 0.65, reverbE: true, reverbSize: 0.90, reverbShim: 0.45, reverbM: 0.40, mDb: -3.5 },
  { name: "PAD // Shimmering Celestial Choir", waves: "ORG + FMT", fx: "Vox Formant + Breath Air", desc: "Heavenly vocal choir pad with formant shaping and subtle breath noise texture.",
    wA: 3, wB: 4, octA: 0, octB: 0, uV: 10, uDet: 0.24, f1T: 6, f1C: 0.65, f1R: 0.35, noiseE: true, noiseT: 5, noiseL: 0.18, envA: 0.50, envR: 0.58, reverbE: true, reverbShim: 0.40, reverbM: 0.38, mDb: -2.5 },
  { name: "PAD // Halftime Granular Ambience", waves: "SAW + ORG", fx: "Time Mod 1/2x + Large Hall", desc: "Evolving granular wash created by half-time time-warping sustained harmonics.",
    wA: 0, wB: 3, octA: 0, octB: 0, timeModE: true, timeModMode: 4, timeModRate: 1, timeModD: 0.80, timeModM: 0.75, reverbE: true, reverbSize: 0.88, reverbM: 0.35, mDb: -2.5 },
  { name: "PAD // Zenology Rotary Flange Pad", waves: "ORG + SAW", fx: "Rotary Speaker + Jet Flanger", desc: "Vintage rotating speaker modulation combined with deep resonant jet flanging.",
    wA: 3, wB: 0, octA: 0, octB: 0, uV: 8, uDet: 0.22, ringE: true, ringMode: 3, ringFreq: 220.0, ringD: 0.50, ringM: 0.40, flangerE: true, flangerM: 0.35, reverbE: true, reverbM: 0.30, mDb: -2.5 },
  { name: "PAD // Cyber Dystopian Drone", waves: "X-MOD + SYNC", fx: "Comb Filter + Tape Saturation", desc: "Dark industrial sci-fi atmosphere with metallic comb resonance and octave sub.",
    wA: 9, wB: 7, octA: -1, octB: 0, uV: 6, uDet: 0.20, f1T: 5, f1C: 0.58, f1R: 0.52, distE: true, distT: 1, distD: 0.35, delayE: true, delayM: 0.28, reverbE: true, reverbM: 0.30, mDb: -3.0 },
  { name: "PAD // Glass Spectral Bellpad", waves: "BEL + SINE", fx: "Inharmonic Prism + Shimmer", desc: "Pure crystalline glass texture with dispersing partials and warm sub pad.",
    wA: 8, wB: 6, octA: 0, octB: 0, uV: 6, uDet: 0.18, reverbE: true, reverbSize: 0.92, reverbShim: 0.55, reverbM: 0.42, chorusE: true, chorusM: 0.28, mDb: -2.5 },
  { name: "PAD // Lo-Fi 12-Bit Ambient", waves: "SAW + TRI", fx: "SP-1200 Lo-Fi + SVF Lowpass", desc: "Dusty vintage cassette tape ambient texture with 12-bit crunch and subtle flutter.",
    wA: 0, wB: 2, octA: 0, octB: 0, uV: 8, uDet: 0.22, crushE: true, crushMode: 2, crushBits: 8.5, crushDown: 3.5, crushM: 0.45, f1T: 1, f1C: 0.52, f1R: 0.18, mDb: -2.0 },
  { name: "PAD // Reverse Grain Shaper Cloud", waves: "SAW + ORG", fx: "Time Mod Reverse + Hall", desc: "Surreal reverse-grain time modulation creating backwards blossoming chord clouds.",
    wA: 0, wB: 3, octA: 0, octB: 0, timeModE: true, timeModMode: 3, timeModRate: 1, timeModD: 0.85, timeModM: 0.80, reverbE: true, reverbSize: 0.85, reverbM: 0.35, mDb: -2.5 }
`;

const newPlucks = `
  { name: "PLUCK // Progressive House Pluck", waves: "SAW + SQR", fx: "Fast AHDSR + Tape Slap", desc: "Short, snappy progressive house pluck with sharp envelope decay and tape echo.",
    wA: 0, wB: 1, octA: 0, octB: 0, vMode: 0, uV: 6, uDet: 0.20, envA: 0.01, envD: 0.22, envS: 0.0, envR: 0.18, distE: true, distT: 1, distD: 0.28, delayE: true, delayT: 0.25, delayM: 0.26, mDb: -1.5 },
  { name: "PLUCK // Glass Bell Mallet", waves: "BEL + SINE", fx: "Ring Carrier + Shimmer Hall", desc: "Glassy struck mallet sound infused with subtle 440Hz ring modulation.",
    wA: 8, wB: 6, octA: 0, octB: 0, envA: 0.01, envD: 0.32, envS: 0.0, envR: 0.24, ringE: true, ringMode: 0, ringFreq: 440.0, ringD: 0.45, ringM: 0.35, reverbE: true, reverbShim: 0.45, reverbM: 0.30, mDb: -2.0 },
  { name: "PLUCK // 1/4 Tape Stop Pluck", waves: "TRI + SAW", fx: "Time Mod Tape Stop + Echo", desc: "Acoustic-synthetic hybrid pluck undergoing rhythmic tape stop deceleration.",
    wA: 2, wB: 0, octA: 0, octB: 0, timeModE: true, timeModMode: 2, timeModRate: 3, timeModD: 0.70, timeModM: 0.75, delayE: true, delayM: 0.28, mDb: -2.0 },
  { name: "PLUCK // 16-Voice Supersaw Pluck", waves: "SAW + SAW", fx: "16x Unison + Fast SVF Decay", desc: "Wide festival anthem pluck with 16 unison voices and snappy filter envelope.",
    wA: 0, wB: 0, octA: 0, octB: 0, uV: 16, uDet: 0.32, uSpr: 0.95, envA: 0.01, envD: 0.25, envS: 0.0, envR: 0.20, f1T: 1, f1C: 0.55, f1R: 0.30, compE: true, compM: 0.75, mDb: -3.5 },
  { name: "PLUCK // SP-1200 Vintage Chime", waves: "SQR + SINE", fx: "SP-1200 Lo-Fi + Stereo Delay", desc: "Nostalgic 90s hip-hop bell chime with 12-bit warmth and subtle vibrato.",
    wA: 1, wB: 6, octA: 1, octB: 0, crushE: true, crushMode: 2, crushBits: 8.0, crushDown: 4.0, crushM: 0.50, envA: 0.01, envD: 0.38, envS: 0.0, delayE: true, delayM: 0.24, mDb: -1.5 },
  { name: "PLUCK // Metallic Comb Pluck", waves: "SYNC + SQR", fx: "Comb Filter + Dimension Chorus", desc: "Acoustic string emulation created by feeding snappy impulse into comb filter.",
    wA: 7, wB: 1, octA: 0, octB: 0, f1T: 5, f1C: 0.65, f1R: 0.58, chorusE: true, chorusM: 0.35, envA: 0.01, envD: 0.28, envS: 0.0, mDb: -2.0 },
  { name: "PLUCK // Future Bass Chime", waves: "BEL + SAW", fx: "OTT Comp + Wide Spread", desc: "Bright futuristic mallet chord pluck cutting through heavy sidechain saw stacks.",
    wA: 8, wB: 0, octA: 0, octB: 0, uV: 8, uDet: 0.24, compE: true, compM: 0.78, reverbE: true, reverbM: 0.28, envA: 0.01, envD: 0.30, envS: 0.0, mDb: -2.5 },
  { name: "PLUCK // Vocal Formant Kalimba", waves: "FMT + TRI", fx: "Vox Formant + Ping-Pong", desc: "African kalimba style pluck filtered through vocal formant vowel resonance.",
    wA: 4, wB: 2, octA: 0, octB: 0, f1T: 6, f1C: 0.58, f1R: 0.42, delayE: true, delayT: 0.25, delayM: 0.30, envA: 0.01, envD: 0.35, envS: 0.0, mDb: -1.5 }
`;

const newArps = `
  { name: "ARP // DJ Scratch Rhythm Gate", waves: "SAW + SQR", fx: "Time Mod DJ Scratch + Acid SVF", desc: "Virtuosic vinyl turntable scratching rhythm superimposed over 16th-note arpeggio.",
    wA: 0, wB: 1, octA: 0, octB: 0, arpE: true, arpRate: 3, arpOct: 2, timeModE: true, timeModMode: 5, timeModRate: 2, timeModD: 0.85, timeModM: 0.80, f1T: 1, f1C: 0.62, f1R: 0.45, mDb: -2.5 },
  { name: "ARP // Step Flanger 1/16 Sequence", waves: "SYNC + SAW", fx: "Step Flanger + Ping-Pong", desc: "Rhythmic step-modulated flanging locking to 1/16th arpeggio pulses.",
    wA: 7, wB: 0, octA: 0, octB: 0, arpE: true, arpRate: 3, arpOct: 2, flangerE: true, flangerMode: 3, flangerM: 0.55, delayE: true, delayM: 0.28, mDb: -2.5 },
  { name: "ARP // Cyberpunk Neuro Arp", waves: "X-MOD + SAW", fx: "Hard Clip + Comb Filter", desc: "Aggressive dystopian industrial sequence with heavy clipper distortion.",
    wA: 9, wB: 0, octA: -1, octB: 0, arpE: true, arpRate: 3, arpOct: 2, distE: true, distT: 2, distD: 0.55, f1T: 5, f1C: 0.60, f1R: 0.50, mDb: -3.0 },
  { name: "ARP // Uplifting Trance Gate", waves: "SAW + SAW", fx: "8x Unison + 1/16 Gate + Shimmer", desc: "Classic euphoria trance arpeggio with 8x unison and shimmering space.",
    wA: 0, wB: 0, octA: 0, octB: 0, uV: 8, uDet: 0.28, arpE: true, arpRate: 3, arpOct: 3, reverbE: true, reverbShim: 0.45, reverbM: 0.35, delayE: true, delayM: 0.30, mDb: -3.0 },
  { name: "ARP // Talkbox Vocal Sequence", waves: "SAW + FMT", fx: "Talkbox Mode + Delay", desc: "Bouncy vocal formant arpeggio shifting vowel textures with each step.",
    wA: 0, wB: 4, octA: 0, octB: -1, arpE: true, arpRate: 3, arpOct: 2, f1T: 9, f1C: 0.68, f1R: 0.44, delayE: true, delayM: 0.26, mDb: -2.0 },
  { name: "ARP // Lo-Fi Bitcrushed Chiptune", waves: "SQR + SQR", fx: "Bitcrush Decimator + Delay", desc: "Fast retro 8-bit arcade arpeggiator with crunchy sample decimation.",
    wA: 1, wB: 1, octA: 1, octB: 0, arpE: true, arpRate: 3, arpOct: 3, crushE: true, crushMode: 0, crushBits: 6.0, crushDown: 3.0, crushM: 0.60, delayE: true, delayM: 0.22, mDb: -1.5 },
  { name: "ARP // Halftime Acid Arpeggio", waves: "SAW + TRI", fx: "Time Mod 1/2x + SVF 24dB", desc: "Slow rolling half-time acid arpeggio with high resonance squelch.",
    wA: 0, wB: 2, octA: 0, octB: 0, arpE: true, arpRate: 3, arpOct: 2, timeModE: true, timeModMode: 4, timeModRate: 1, timeModD: 0.85, timeModM: 0.75, f1T: 1, f1C: 0.48, f1R: 0.75, mDb: -2.5 },
  { name: "ARP // Stutter Glitch Matrix", waves: "SYNC + SINE", fx: "Time Mod Stutter 1/16 + Tape", desc: "Hectic glitch arpeggio slicing incoming buffer with high density stuttering.",
    wA: 7, wB: 6, octA: 0, octB: 0, arpE: true, arpRate: 3, arpOct: 2, timeModE: true, timeModMode: 1, timeModRate: 5, timeModD: 0.80, timeModM: 0.80, delayE: true, delayM: 0.25, mDb: -2.5 }
`;

const newFx = `
  { name: "FX // 2-Bar Reverse Riser", waves: "SAW + SYNC", fx: "Time Mod Reverse + Hall", desc: "Epic 2-bar reverse grain riser sucking audio backward into sudden drop impact.",
    wA: 0, wB: 7, octA: 0, octB: 0, timeModE: true, timeModMode: 3, timeModRate: 0, timeModD: 0.95, timeModM: 0.90, reverbE: true, reverbSize: 0.92, reverbM: 0.45, mDb: -3.0 },
  { name: "FX // Cyber Vinyl Tape Drop", waves: "SAW + SQR", fx: "Time Mod Tape Stop + Jet Flanger", desc: "Simulated analog master turntable power-down with pitch decelerating to standstill.",
    wA: 0, wB: 1, octA: 0, octB: 0, timeModE: true, timeModMode: 2, timeModRate: 1, timeModD: 0.95, timeModM: 0.90, flangerE: true, flangerM: 0.40, mDb: -2.5 },
  { name: "FX // Sci-Fi Ring Mod Laser", waves: "X-MOD + SINE", fx: "Ring Mod 800Hz + Delay", desc: "Futuristic raygun laser zap created by fast sweeping diode ring modulation.",
    wA: 9, wB: 6, octA: 1, octB: 0, ringE: true, ringMode: 0, ringFreq: 800.0, ringD: 0.85, ringM: 0.65, delayE: true, delayT: 0.18, delayM: 0.35, mDb: -2.0 },
  { name: "FX // Downsample Decimator Zap", waves: "SYNC + X-MOD", fx: "Bitcrush 16x Down + Hard Clip", desc: "Extreme digital downsampling zap with aliasing artifacts for transition sweeps.",
    wA: 7, wB: 9, octA: 0, octB: 0, crushE: true, crushMode: 1, crushDown: 16.0, crushBits: 10.0, crushM: 0.75, distE: true, distT: 2, distD: 0.50, mDb: -3.0 },
  { name: "FX // Barberpole Infinite Riser", waves: "SAW + FMT", fx: "Barberpole Flanger + OTT Comp", desc: "Shepard-tone style perpetual rising flanger sensation over bright spectral noise.",
    wA: 0, wB: 4, octA: 0, octB: 0, flangerE: true, flangerMode: 2, flangerM: 0.65, compE: true, compM: 0.80, reverbE: true, reverbM: 0.35, mDb: -3.0 },
  { name: "FX // Stutter Matrix Transition", waves: "SQR + SYNC", fx: "Time Mod Stutter 1/16 + Echo", desc: "Rapid rhythmic buffer stutter transition for high-energy build-ups.",
    wA: 1, wB: 7, octA: 0, octB: 0, timeModE: true, timeModMode: 1, timeModRate: 5, timeModD: 0.85, timeModM: 0.85, delayE: true, delayM: 0.30, mDb: -2.5 },
  { name: "FX // Sub Boom Tape Stop", waves: "SINE + SAW", fx: "Tape Stop + Low EQ Boost + Punch", desc: "Cinematic 808 sub boom slowing down to dead halt with heavy low-shelf saturation.",
    wA: 6, wB: 0, octA: -2, octB: -1, timeModE: true, timeModMode: 2, timeModRate: 1, timeModD: 0.90, timeModM: 0.85, distE: true, distT: 0, distD: 0.42, compE: true, compM: 0.75, mDb: -1.0 },
  { name: "FX // Glitch Chaos Sweep", waves: "SYNC + X-MOD", fx: "Ring Mod + Comb Filter + Shimmer", desc: "Complex chaotic spectral transition sweep with shifting ring carrier frequencies.",
    wA: 7, wB: 9, octA: 0, octB: 0, ringE: true, ringMode: 1, ringFreq: 280.0, ringD: 0.75, ringM: 0.55, f1T: 5, f1C: 0.65, f1R: 0.60, reverbE: true, reverbShim: 0.50, reverbM: 0.40, mDb: -3.0 }
`;

// Insert the new definitions right before the closing bracket of each array
code = code.replace(/(const bassDefs = \[[\s\S]*?)(];)/, '$1,' + newBass + '$2');
code = code.replace(/(const leadDefs = \[[\s\S]*?)(];)/, '$1,' + newLeads + '$2');
code = code.replace(/(const padDefs = \[[\s\S]*?)(];)/, '$1,' + newPads + '$2');
code = code.replace(/(const pluckDefs = \[[\s\S]*?)(];)/, '$1,' + newPlucks + '$2');
code = code.replace(/(const arpDefs = \[[\s\S]*?)(];)/, '$1,' + newArps + '$2');
code = code.replace(/(const fxDefs = \[[\s\S]*?)(];)/, '$1,' + newFx + '$2');

// Update forEach loops to use dynamic sequential indexing and formatted names
code = code.replace(/bassDefs\.forEach\(\(d, i\) => \{\s*const p = basePreset\(3 \+ i, d\.name, "BASS"/,
  'bassDefs.forEach((d, i) => {\n  const numStr = (presets.length < 10 ? "0" : "") + presets.length;\n  const cleanName = d.name.replace(/^\\d+\\.\\s*/, "");\n  const p = basePreset(presets.length, numStr + ". " + cleanName, "BASS"');

code = code.replace(/leadDefs\.forEach\(\(d, i\) => \{\s*const p = basePreset\(23 \+ i, d\.name, "LEADS"/,
  'leadDefs.forEach((d, i) => {\n  const numStr = (presets.length < 10 ? "0" : "") + presets.length;\n  const cleanName = d.name.replace(/^\\d+\\.\\s*/, "");\n  const p = basePreset(presets.length, numStr + ". " + cleanName, "LEADS"');

code = code.replace(/padDefs\.forEach\(\(d, i\) => \{\s*const p = basePreset\(43 \+ i, d\.name, "PADS"/,
  'padDefs.forEach((d, i) => {\n  const numStr = (presets.length < 10 ? "0" : "") + presets.length;\n  const cleanName = d.name.replace(/^\\d+\\.\\s*/, "");\n  const p = basePreset(presets.length, numStr + ". " + cleanName, "PADS"');

code = code.replace(/pluckDefs\.forEach\(\(d, i\) => \{\s*const p = basePreset\(63 \+ i, d\.name, "PLUCKS"/,
  'pluckDefs.forEach((d, i) => {\n  const numStr = (presets.length < 10 ? "0" : "") + presets.length;\n  const cleanName = d.name.replace(/^\\d+\\.\\s*/, "");\n  const p = basePreset(presets.length, numStr + ". " + cleanName, "PLUCKS"');

code = code.replace(/arpDefs\.forEach\(\(d, i\) => \{\s*const p = basePreset\(83 \+ i, d\.name, "ARP & SEQ"/,
  'arpDefs.forEach((d, i) => {\n  const numStr = (presets.length < 10 ? "0" : "") + presets.length;\n  const cleanName = d.name.replace(/^\\d+\\.\\s*/, "");\n  const p = basePreset(presets.length, numStr + ". " + cleanName, "ARP & SEQ"');

code = code.replace(/fxDefs\.forEach\(\(d, i\) => \{\s*const p = basePreset\(103 \+ i, d\.name, "FX & TRANS"/,
  'fxDefs.forEach((d, i) => {\n  const numStr = (presets.length < 10 ? "0" : "") + presets.length;\n  const cleanName = d.name.replace(/^\\d+\\.\\s*/, "");\n  const p = basePreset(presets.length, numStr + ". " + cleanName, "FX & TRANS"');

// Update header comment
code = code.replace(/PRISMOR 1\.0\.6\.2 FACTORY PRESET BANK \(\d+ PRODUCTION-READY PATCHES\)/g,
  'PRISMOR 1.0.8.4 FACTORY PRESET BANK (172 PRODUCTION-READY PATCHES)');

fs.writeFileSync(srcFile, code, 'utf8');
console.log('Successfully updated generate_presets.js with 172 presets!');
