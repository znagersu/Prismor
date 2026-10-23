/**
 * Prismor Preset Bank Generator & Validator Script
 * Programmatically generates, validates, and injects 120+ diverse,
 * musically tuned factory presets into the Prismor C++ engine.
 */

const fs = require('fs');
const path = require('path');

// -----------------------------------------------------------------------------
// 1. Parameter Specifications & Range Validation Limits
// -----------------------------------------------------------------------------
const PARAM_LIMITS = {
  octave:        { min: -4, max: 4 },
  semi:          { min: -12, max: 12 },
  fine:          { min: -100, max: 100 },
  coarse:        { min: -24, max: 24 },
  norm:          { min: 0.0, max: 1.0 },
  bipolar:       { min: -1.0, max: 1.0 },
  harmScale:     { min: 0.25, max: 4.0 },
  maxVoices:     { min: 1, max: 16 },
  unisonVoices:  { min: 1, max: 16 },
  unisonStack:   { min: 0, max: 7 },
  vibSpeed:      { min: 0.5, max: 15.0 },
  filterType:    { min: 0, max: 9 },
  noiseType:     { min: 0, max: 5 },
  arpMode:       { min: 0, max: 5 },
  arpRate:       { min: 0, max: 5 },
  arpOctaves:    { min: 1, max: 4 },
  distType:      { min: 0, max: 5 },
  chorusVoices:  { min: 2, max: 6 },
  compMode:      { min: 0, max: 3 },
  compThresh:    { min: -48.0, max: 0.0 },
  ringMode:      { min: 0, max: 3 },
  ringFreq:      { min: 20.0, max: 2000.0 },
  crushMode:     { min: 0, max: 3 },
  crushBits:     { min: 1.0, max: 16.0 },
  crushDown:     { min: 1.0, max: 64.0 },
  flangerMode:   { min: 0, max: 3 },
  flangerFb:     { min: -0.95, max: 0.95 },
  timeModMode:   { min: 0, max: 5 },
  timeModRate:   { min: 0, max: 5 },
  masterGainDb:  { min: -24.0, max: 12.0 },
  limiterThresh: { min: -12.0, max: 0.0 }
};

// -----------------------------------------------------------------------------
// 2. Preset Definitions Factory
// -----------------------------------------------------------------------------
const presets = [];

function addPreset(data) {
  presets.push(data);
}

// Helper default template
function basePreset(index, name, category, author, wavesUsed, fxUsed, description) {
  return {
    index, name, category, author, wavesUsed, fxUsed, description,
    // Oscillator Part A & B
    waveA: 0, waveB: 1,
    octA: 0, semiA: 0, fineA: 0.0,
    octB: 0, semiB: 0, fineB: 0.0,
    warpModeA: 0, warpAmtA: 0.0,
    warpModeB: 0, warpAmtB: 0.0,
    timbreA: 0.0, timbreB: 0.0,
    harmScaleA: 1.0, harmTiltA: 0.0,
    harmScaleB: 1.0, harmTiltB: 0.0,
    // Voicing & Unison
    voicingMode: 0, maxVoices: 8,
    portaTime: 0.0, portaCurve: 0.0, autoGlide: true,
    unisonVoices: 1, unisonDetune: 0.18, unisonSpread: 0.65, unisonBlend: 0.55, unisonStack: 0,
    vibDepth: 0.0, vibSpeed: 5.5,
    // Filters
    filter1Enable: true, filter1Type: 1, f1Cutoff: 1.0, f1Res: 0.10, f1KeyTrack: 0.50, f1Width: 0.45,
    filter2Enable: false, filter2Type: 2, f2Cutoff: 0.85, f2Res: 0.20, f2KeyTrack: 0.50, f2Width: 0.50,
    filterDrive: 0.0, filterRouting: 0.0,
    // Noise
    noiseEnable: false, noiseType: 0, noiseLevel: 0.20, noisePitch: 0.50, noiseFilter: 0.0,
    // Arpeggiator
    arpEnable: false, arpMode: 0, arpOctaves: 1, arpRate: 3, arpGate: 0.75, arpSwing: 0.0,
    // FX Units
    distEnable: false, distType: 0, distDrive: 0.35, distTone: 0.65, distMix: 0.45,
    chorusEnable: false, chorusVoices: 4, chorusRate: 0.85, chorusDepth: 0.45, chorusMix: 0.35,
    phaserEnable: false, phaserMix: 0.30, phaserSpeed: 1.2,
    delayEnable: false, delayTime: 0.35, delayFeedback: 0.40, delayPingPong: 0.65, delayMix: 0.28,
    reverbEnable: false, reverbSize: 0.68, reverbDecay: 0.55, reverbShimmer: 0.25, reverbMix: 0.25,
    compEnable: false, compMode: 1, compThreshold: -18.0, compRatio: 4.0, compMakeup: 3.0, compMix: 0.75,
    ringEnable: false, ringMode: 0, ringFreq: 220.0, ringDepth: 0.70, ringMix: 0.50,
    crushEnable: false, crushMode: 0, crushBits: 8.0, crushDownsample: 4.0, crushMix: 0.55,
    flangerEnable: false, flangerMode: 0, flangerRate: 0.45, flangerDepth: 0.65, flangerFeedback: 0.55, flangerMix: 0.50,
    // Time Modulation
    timeModEnable: false, timeModMode: 0, timeModRate: 1, timeModDepth: 0.75, timeModMix: 0.85,
    // Envelopes & Shaper
    envAttack: 0.05, envHold: 0.05, envDecay: 0.35, envSustain: 0.72, envRelease: 0.25,
    shaperPresetTarget: -1, timeModPresetIdx: 0,
    // Mix & Output
    abMix: 0.0, velLink: 0.65,
    masterGainDb: 0.0, limiterThreshDb: -0.5
  };
}

// -----------------------------------------------------------------------------
// Category 0: INIT & TEMPLATES (3 Patches)
// -----------------------------------------------------------------------------
{
  let p = basePreset(0, "00. INIT // Clean Default Saw", "INIT", "Prismor Lab", "SAW + SQR", "Dry Calibration", "Clean 64-partial calibrated sawtooth & square reference patch.");
  addPreset(p);

  p = basePreset(1, "01. TEMPLATE // Dual-Layer Poly", "INIT", "Prismor Lab", "SAW + ORG", "Chorus + Reverb", "Dual-harmonic sketch template with lush stereo chorus and neutral EQ.");
  p.waveA = 0; p.waveB = 3; p.abMix = 0.40; p.chorusEnable = true; p.reverbEnable = true; p.reverbMix = 0.20;
  p.masterGainDb = -1.5;
  addPreset(p);

  p = basePreset(2, "02. TEMPLATE // Mono Solo Legato", "INIT", "Prismor Lab", "SAW + SQR", "Porta + Delay", "Clean mono legato template with 0.22s glide and gentle stereo echo.");
  p.voicingMode = 2; p.maxVoices = 1; p.portaTime = 0.22; p.unisonVoices = 4; p.unisonDetune = 0.20;
  p.delayEnable = true; p.delayMix = 0.22; p.masterGainDb = -2.0;
  addPreset(p);
}

// -----------------------------------------------------------------------------
// Category 1: BASS (20 Patches: indices 3 to 22)
// -----------------------------------------------------------------------------
const bassDefs = [
  { name: "03. BASS // Obsidian Reese Sub", waves: "SAW + SQR", fx: "Tape + Phaser + Chorus", desc: "Dark 12-voice detuned neuro Reese bass with crude brickwall lowpass.",
    wA: 0, wB: 1, octA: -1, octB: -1, semiB: 0, fineB: 8, vMode: 1, uV: 12, uDet: 0.28, uSpr: 0.85, f1T: 2, f1C: 0.38, f1R: 0.25, distE: true, distT: 1, distD: 0.42, phaserE: true, phaserM: 0.35, chorusE: true, chorusM: 0.25, mDb: -3.0 },
  { name: "04. BASS // Pure Sine 808 Sub", waves: "SINE + TRI", fx: "Tube Drive + Low EQ Boost", desc: "Deep phase-locked Sine fundamental 808 sub bass with warm harmonic punch.",
    wA: 6, wB: 2, octA: -2, octB: -2, vMode: 1, f1T: 0, f1C: 0.52, f1R: 0.08, distE: true, distT: 0, distD: 0.28, distM: 0.35, envA: 0.01, envD: 0.65, envS: 0.0, envR: 0.40, mDb: 0.0 },
  { name: "05. BASS // Aggressive 808 Sub", waves: "SQR + SAW", fx: "Warm Tube + Parametric EQ", desc: "Hard-hitting asymmetric warped sub bass with punchy low-shelf boost.",
    wA: 1, wB: 0, octA: -2, octB: -2, warpA: 4, warpAmtA: 0.45, vMode: 1, f1T: 1, f1C: 0.44, f1R: 0.22, distE: true, distT: 0, distD: 0.58, compE: true, compM: 0.60, envA: 0.01, envD: 0.55, envS: 0.0, mDb: -1.0 },
  { name: "06. BASS // Neuro OTT Talkbass", waves: "SAW + FMT", fx: "Talkbox + Wavefold + OTT", desc: "Growling sync-folded talkbox neuro bass with heavy OTT upward compression.",
    wA: 0, wB: 4, octA: -1, octB: -1, warpA: 5, warpAmtA: 0.52, vMode: 1, uV: 4, uDet: 0.22, f1T: 9, f1C: 0.58, f1R: 0.45, distE: true, distT: 3, distD: 0.48, compE: true, compM: 0.75, abM: 0.45, mDb: -2.5 },
  { name: "07. BASS // Bitcrushed Lo-Fi Sub", waves: "SINE + SYNC", fx: "Bitcrusher + OTT Comp", desc: "8-bit sample-decimated SP-1200 style crunch bass with sub foundation.",
    wA: 6, wB: 7, octA: -2, octB: -1, vMode: 1, crushE: true, crushMode: 2, crushBits: 7.5, crushDown: 5.0, crushM: 0.52, compE: true, compM: 0.65, f1T: 1, f1C: 0.48, mDb: -1.5 },
  { name: "08. BASS // Ring Mod Metallic Donk", waves: "X-MOD + SINE", fx: "Ring Mod + Punch Limiter", desc: "FM & ring-modulated hollow bass pluck for UK bassline and techno.",
    wA: 9, wB: 6, octA: -1, octB: -2, vMode: 1, ringE: true, ringMode: 0, ringFreq: 110.0, ringD: 0.65, ringM: 0.48, envA: 0.01, envD: 0.24, envS: 0.0, envR: 0.15, compE: true, compMode: 3, mDb: -2.0 },
  { name: "09. BASS // Acid 303 Reso Squelch", waves: "SAW + SQR", fx: "Acid SVF 24dB + Asym Scream", desc: "Squelchy resonant 303 acid bassline with high resonance and asymmetric drive.",
    wA: 0, wB: 1, octA: -1, octB: -1, vMode: 2, portaTime: 0.18, f1T: 1, f1C: 0.42, f1R: 0.78, distE: true, distT: 5, distD: 0.62, envA: 0.02, envD: 0.32, envS: 0.15, mDb: -2.0 },
  { name: "10. BASS // Deep Sub Foundation", waves: "SINE + SINE", fx: "Clean Sub Limiter", desc: "Pristine sub fundamental for hip-hop and trap with zero phase cancellation.",
    wA: 6, wB: 6, octA: -2, octB: -2, vMode: 1, f1T: 0, f1C: 0.35, f1R: 0.05, limiterThresh: -0.2, mDb: 0.5 },
  { name: "11. BASS // Cyberpunk Industrial Bass", waves: "SYNC + X-MOD", fx: "Wavefold + Jet Flanger + OTT", desc: "Aggressive dystopian industrial bass with wavefolding and high-feedback flanger.",
    wA: 7, wB: 9, octA: -1, octB: -1, warpA: 5, warpAmtA: 0.45, vMode: 1, uV: 6, uDet: 0.25, distE: true, distT: 3, distD: 0.55, flangerE: true, flangerM: 0.40, compE: true, compM: 0.70, mDb: -3.5 },
  { name: "12. BASS // Slapback Funk Bassline", waves: "TRI + SAW", fx: "Tube Saturation + Slap Delay", desc: "Funky analog bass pluck with fast attack, tight decay, and tape slapback.",
    wA: 2, wB: 0, octA: -1, octB: -1, vMode: 1, f1T: 1, f1C: 0.55, f1R: 0.30, envA: 0.01, envD: 0.28, envS: 0.20, distE: true, distT: 0, distD: 0.30, delayE: true, delayT: 0.12, delayM: 0.22, mDb: -1.0 },
  { name: "13. BASS // FM Metallic Growler", waves: "X-MOD + SQR", fx: "Freq Shifter + Comb Filter", desc: "Harsh metallic growl bass with frequency shifter modulation and comb resonances.",
    wA: 9, wB: 1, octA: -1, octB: -1, vMode: 1, ringE: true, ringMode: 1, ringFreq: 85.0, ringD: 0.58, ringM: 0.42, f1T: 5, f1C: 0.62, f1R: 0.48, distE: true, distT: 2, distD: 0.45, mDb: -2.5 },
  { name: "14. BASS // Vintage Moog 24dB Saw", waves: "SAW + SAW", fx: "SVF 24dB + Warm Saturation", desc: "Authentic ladder lowpass analog bass with slight third harmonic saturation.",
    wA: 0, wB: 0, octA: -1, octB: -1, fineB: 6.0, vMode: 1, f1T: 1, f1C: 0.45, f1R: 0.35, f1KeyTrack: 0.65, distE: true, distT: 0, distD: 0.25, envA: 0.02, envD: 0.42, envS: 0.35, mDb: -1.0 },
  { name: "15. BASS // Glitch Stutter Sub", waves: "SINE + SYNC", fx: "Time Mod 1/8 Stutter + OTT", desc: "Sub bass with automated rhythmic buffer slicing on sustained notes.",
    wA: 6, wB: 7, octA: -2, octB: -1, vMode: 1, timeModE: true, timeModMode: 1, timeModRate: 4, timeModD: 0.65, timeModM: 0.70, compE: true, compM: 0.65, mDb: -2.0 },
  { name: "16. BASS // Dubstep Wobble Grunt", waves: "SAW + FMT", fx: "Formant Vowel + Bitcrush Fuzz", desc: "LFO-swept formant vowel wobble with heavy bitcrush fuzz and sub anchor.",
    wA: 0, wB: 4, octA: -1, octB: -1, vMode: 1, f1T: 6, f1C: 0.52, f1R: 0.55, distE: true, distT: 4, distD: 0.40, compE: true, compM: 0.70, mDb: -2.5 },
  { name: "17. BASS // Midrange Neuro Spike", waves: "SYNC + SAW", fx: "Bandpass Reso + Jet Flanger", desc: "Piercing midrange sync sweep bass cutting through dense drum & bass mixes.",
    wA: 7, wB: 0, octA: -1, octB: -1, warpA: 5, warpAmtA: 0.62, vMode: 1, uV: 4, uDet: 0.22, f1T: 4, f1C: 0.68, f1R: 0.65, flangerE: true, flangerM: 0.35, mDb: -3.0 },
  { name: "18. BASS // Analog Warmth Pluck", waves: "TRI + SQR", fx: "Crude Lowpass + Tape Saturation", desc: "Warm and round analog plucked bass with mellow transient bite.",
    wA: 2, wB: 1, octA: -1, octB: -1, vMode: 1, f1T: 2, f1C: 0.50, f1R: 0.15, envA: 0.01, envD: 0.35, envS: 0.20, distE: true, distT: 1, distD: 0.20, mDb: -0.5 },
  { name: "19. BASS // Dark Matter Drone Bass", waves: "SAW + X-MOD", fx: "Phaser + Deep Delay + Hall", desc: "Low-end subterranean drone with slow phaser movement and octave sub.",
    wA: 0, wB: 9, octA: -2, octB: -2, vMode: 0, uV: 8, uDet: 0.18, phaserE: true, phaserM: 0.40, delayE: true, delayM: 0.25, reverbE: true, reverbM: 0.20, mDb: -3.0 },
  { name: "20. BASS // Punchy Attack Donk", waves: "SINE + SYNC", fx: "Pitch Envelope + Hard Clip", desc: "Hardstyle & bounce house donk with snappy octave pitch decay.",
    wA: 6, wB: 7, octA: -1, octB: -1, vMode: 1, envA: 0.01, envD: 0.18, envS: 0.0, envR: 0.12, distE: true, distT: 2, distD: 0.50, compE: true, compMode: 3, mDb: -1.5 },
  { name: "21. BASS // Stereo Wide Reese", waves: "SAW + SAW", fx: "16x Unison + Dimension Chorus", desc: "Cinema-scale stereo detuned Reese with extreme side spread.",
    wA: 0, wB: 0, octA: -1, octB: -1, vMode: 1, uV: 16, uDet: 0.32, uSpr: 1.0, uBl: 0.70, f1T: 1, f1C: 0.48, f1R: 0.20, chorusE: true, chorusM: 0.35, mDb: -3.5 },
  { name: "22. BASS // Talkbox Vowel Mover", waves: "SAW + FMT", fx: "Talkbox Wow + Tube Drive", desc: "Slow vowel-shifting talkbox bassline with expressive keyboard tracking.",
    wA: 0, wB: 4, octA: -1, octB: -1, vMode: 2, portaTime: 0.20, f1T: 9, f1C: 0.60, f1R: 0.42, distE: true, distT: 0, distD: 0.35, mDb: -1.5 }
,
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
];

bassDefs.forEach((d, i) => {
  const numStr = (presets.length < 10 ? "0" : "") + presets.length;
  const cleanName = d.name.replace(/^\d+\.\s*/, "");
  const p = basePreset(presets.length, numStr + ". " + cleanName, "BASS", "Prismor Sound", d.waves, d.fx, d.desc);
  p.waveA = d.wA; p.waveB = d.wB; p.octA = d.octA; p.octB = d.octB;
  if (d.semiB !== undefined) p.semiB = d.semiB;
  if (d.fineB !== undefined) p.fineB = d.fineB;
  if (d.warpA !== undefined) { p.warpModeA = d.warpA; p.warpAmtA = d.warpAmtA; }
  if (d.vMode !== undefined) p.voicingMode = d.vMode;
  if (d.portaTime !== undefined) p.portaTime = d.portaTime;
  if (d.uV !== undefined) { p.unisonVoices = d.uV; p.unisonDetune = d.uDet || 0.20; p.unisonSpread = d.uSpr || 0.70; }
  if (d.f1T !== undefined) p.filter1Type = d.f1T;
  if (d.f1C !== undefined) p.f1Cutoff = d.f1C;
  if (d.f1R !== undefined) p.f1Res = d.f1R;
  if (d.f1KeyTrack !== undefined) p.f1KeyTrack = d.f1KeyTrack;
  if (d.distE) { p.distEnable = true; p.distType = d.distT; p.distDrive = d.distD; if (d.distM !== undefined) p.distMix = d.distM; }
  if (d.phaserE) { p.phaserEnable = true; p.phaserMix = d.phaserM; }
  if (d.chorusE) { p.chorusEnable = true; p.chorusMix = d.chorusM; }
  if (d.compE) { p.compEnable = true; if (d.compMode) p.compMode = d.compMode; p.compMix = d.compM || 0.70; }
  if (d.ringE) { p.ringEnable = true; p.ringMode = d.ringMode; p.ringFreq = d.ringFreq; p.ringDepth = d.ringD; p.ringMix = d.ringM; }
  if (d.crushE) { p.crushEnable = true; p.crushMode = d.crushMode; p.crushBits = d.crushBits; p.crushDownsample = d.crushDown; p.crushMix = d.crushM; }
  if (d.flangerE) { p.flangerEnable = true; p.flangerMix = d.flangerM; }
  if (d.timeModE) { p.timeModEnable = true; p.timeModMode = d.timeModMode; p.timeModRate = d.timeModRate; p.timeModDepth = d.timeModD; p.timeModMix = d.timeModM; }
  if (d.delayE) { p.delayEnable = true; p.delayTime = d.delayT || 0.25; p.delayMix = d.delayM; }
  if (d.reverbE) { p.reverbEnable = true; p.reverbMix = d.reverbM; }
  if (d.envA !== undefined) { p.envAttack = d.envA; p.envDecay = d.envD; p.envSustain = d.envS; if (d.envR) p.envRelease = d.envR; }
  if (d.abM !== undefined) p.abMix = d.abM;
  if (d.mDb !== undefined) p.masterGainDb = d.mDb;
  addPreset(p);
});

// -----------------------------------------------------------------------------
// Category 2: LEADS (20 Patches: indices 23 to 42)
// -----------------------------------------------------------------------------
const leadDefs = [
  { name: "23. LEAD // 16x Unison Hypersaw", waves: "SAW + SAW", fx: "OTT Comp + Shimmer Reverb", desc: "Massive 16-voice stereo detuned stadium supersaw with +1 octave stack.",
    wA: 0, wB: 0, octA: 0, octB: 1, uV: 16, uDet: 0.35, uSpr: 0.95, uBl: 0.68, uStack: 1, compE: true, compM: 0.75, reverbE: true, reverbM: 0.32, reverbShim: 0.35, mDb: -3.5 },
  { name: "24. LEAD // Talkbox Funk Lead", waves: "SAW + FMT", fx: "Talkbox + OTT Comp", desc: "Classic vocal tube talkbox lead with vowel articulation and multiband OTT.",
    wA: 0, wB: 4, octA: 0, octB: -1, vMode: 2, portaTime: 0.22, uV: 4, uDet: 0.18, f1T: 9, f1C: 0.72, f1R: 0.40, compE: true, compM: 0.65, delayE: true, delayM: 0.25, mDb: -2.0 },
  { name: "25. LEAD // Zenology Hard-Sync Lead", waves: "SYNC + SAW", fx: "Jet Flanger + Distortion + OTT", desc: "Screaming hard-sync harmonic cluster lead with jet flanger and tube drive.",
    wA: 7, wB: 0, octA: 0, octB: 0, warpA: 5, warpAmtA: 0.55, vMode: 2, portaTime: 0.20, uV: 6, uDet: 0.28, f1T: 1, f1C: 0.78, f1R: 0.44, distE: true, distT: 0, distD: 0.45, flangerE: true, flangerM: 0.45, compE: true, delayE: true, delayM: 0.28, mDb: -3.0 },
  { name: "26. LEAD // X-Mod Ring Cyber Lead", waves: "X-MOD + SYNC", fx: "Ring Mod + Delay + Reverb", desc: "Cross-modulated sideband lead running through Zenology diode ring modulation.",
    wA: 9, wB: 7, octA: 0, octB: 0, uV: 4, uDet: 0.22, ringE: true, ringMode: 0, ringFreq: 330.0, ringD: 0.65, ringM: 0.45, delayE: true, delayM: 0.26, reverbE: true, reverbM: 0.25, mDb: -2.0 },
  { name: "27. LEAD // Warm Legato Solo", waves: "SAW + ORG", fx: "Chorus + Ping-Pong + Reverb", desc: "Expressive true-legato solo lead with 6x unison and fifth harmonic layer.",
    wA: 0, wB: 3, octA: 0, octB: 0, semiB: 7, fineB: 8, vMode: 2, portaTime: 0.28, uV: 6, uDet: 0.25, vibD: 0.22, vibS: 5.8, chorusE: true, chorusM: 0.30, delayE: true, delayM: 0.24, reverbE: true, reverbM: 0.22, mDb: -2.0 },
  { name: "28. LEAD // Robotic Vocoder Stack", waves: "SAW + SQR", fx: "Vocoder + Chorus + OTT", desc: "16-channel spectral vocoder stack with 8x unison fifths.",
    wA: 0, wB: 1, octA: 0, octB: 0, uV: 8, uDet: 0.24, uStack: 4, f1T: 8, f1C: 0.75, f1R: 0.35, chorusE: true, chorusM: 0.35, compE: true, compM: 0.70, reverbE: true, reverbM: 0.24, mDb: -2.5 },
  { name: "29. LEAD // Cyber Anthem Chord Stack", waves: "SAW + ORG", fx: "Sidechain + Delay + Reverb", desc: "12-voice Major Triad unison stack with rhythmic ShaperBox ducking.",
    wA: 0, wB: 3, octA: 0, octB: 0, uV: 12, uDet: 0.30, uStack: 7, delayE: true, delayM: 0.30, reverbE: true, reverbM: 0.35, shaperTarget: 0, mDb: -3.0 },
  { name: "30. LEAD // Trance Euphoria Lead", waves: "SAW + SYNC", fx: "10x Unison + Shimmer Hall", desc: "Blissful uplifting trance lead cutting through stadium sound systems.",
    wA: 0, wB: 7, octA: 0, octB: 0, uV: 10, uDet: 0.30, delayE: true, delayT: 0.375, delayM: 0.32, reverbE: true, reverbSize: 0.85, reverbM: 0.38, reverbShim: 0.40, mDb: -2.5 },
  { name: "31. LEAD // Piercing Sync Screamer", waves: "SYNC + SQR", fx: "Asym Scream + Flanger", desc: "Aggressive sync screamer with hard asymmetric distortion clipping.",
    wA: 7, wB: 1, octA: 1, octB: 0, warpA: 5, warpAmtA: 0.68, vMode: 2, portaTime: 0.16, distE: true, distT: 5, distD: 0.55, flangerE: true, flangerM: 0.40, mDb: -2.5 },
  { name: "32. LEAD // Vintage Brass Solo", waves: "SAW + TRI", fx: "Warm SVF 24dB + Plate Reverb", desc: "Warm and dynamic CS-80 inspired synth brass with subtle pitch vibrato.",
    wA: 0, wB: 2, octA: 0, octB: 0, vMode: 0, f1T: 1, f1C: 0.65, f1R: 0.25, vibD: 0.18, vibS: 5.2, envA: 0.08, envD: 0.45, envS: 0.65, reverbE: true, reverbM: 0.25, mDb: -1.5 },
  { name: "33. LEAD // 80s Miami Synthwave", waves: "SAW + SQR", fx: "Juno Chorus + Stereo Echo", desc: "Nostalgic neon synthwave lead with wide Roland-style chorus emulation.",
    wA: 0, wB: 1, octA: 0, octB: 0, uV: 4, uDet: 0.20, chorusE: true, chorusM: 0.48, chorusV: 6, delayE: true, delayT: 0.33, delayM: 0.28, reverbE: true, reverbM: 0.22, mDb: -2.0 },
  { name: "34. LEAD // Chiptune 8-Bit Hero", waves: "SQR + SYNC", fx: "Bitcrusher + Digital Delay", desc: "Authentic GameBoy/NES style pulse wave with lo-fi downsample decimation.",
    wA: 1, wB: 7, octA: 0, octB: 1, vMode: 1, crushE: true, crushMode: 0, crushBits: 6.0, crushDown: 3.5, crushM: 0.60, delayE: true, delayM: 0.22, mDb: -1.5 },
  { name: "35. LEAD // Vocal Formant Screamer", waves: "FMT + SAW", fx: "Vowel Filter + OTT Compressor", desc: "Screaming vocal vowel lead cutting through EDM arrangements.",
    wA: 4, wB: 0, octA: 0, octB: 0, vMode: 2, portaTime: 0.18, f1T: 6, f1C: 0.70, f1R: 0.50, distE: true, distT: 2, distD: 0.45, compE: true, compM: 0.75, mDb: -2.5 },
  { name: "36. LEAD // Flanged Horizon Lead", waves: "SAW + SINE", fx: "Through-Zero Flanger + Chorus", desc: "Wide ethereal lead with through-zero tape flanging and soft delay reflections.",
    wA: 0, wB: 6, octA: 0, octB: 0, uV: 6, uDet: 0.22, flangerE: true, flangerMode: 1, flangerM: 0.55, chorusE: true, chorusM: 0.28, delayE: true, delayM: 0.25, mDb: -2.0 },
  { name: "37. LEAD // Melodic Techno Glide", waves: "SAW + SQR", fx: "Tape Drive + Ping-Pong Delay", desc: "Dark rolling melodic techno lead with long portamento and filtered delay.",
    wA: 0, wB: 1, octA: 0, octB: 0, vMode: 2, portaTime: 0.35, f1T: 1, f1C: 0.58, f1R: 0.32, distE: true, distT: 1, distD: 0.35, delayE: true, delayM: 0.32, reverbE: true, reverbM: 0.25, mDb: -2.0 },
  { name: "38. LEAD // Cyberpunk Laser Lead", waves: "SYNC + X-MOD", fx: "Fast Pitch Drop + Hard Clip", desc: "Snappy futuristic laser lead with instantaneous pitch drop transient.",
    wA: 7, wB: 9, octA: 1, octB: 0, vMode: 1, warpA: 5, warpAmtA: 0.60, distE: true, distT: 2, distD: 0.50, delayE: true, delayM: 0.24, mDb: -2.0 },
  { name: "39. LEAD // Glass Shimmer Solo", waves: "BEL + SAW", fx: "Harmonic Prism + Shimmer Reverb", desc: "Bell-infused crystalline solo lead with shimmering overtone tail.",
    wA: 8, wB: 0, octA: 0, octB: 0, uV: 4, uDet: 0.18, reverbE: true, reverbSize: 0.88, reverbShim: 0.50, reverbM: 0.35, delayE: true, delayM: 0.25, mDb: -2.0 },
  { name: "40. LEAD // Acid Screamer Lead", waves: "SAW + SYNC", fx: "Resonant 24dB + Screamer Drive", desc: "High-register 303 screamer with intense resonance and octave jumps.",
    wA: 0, wB: 7, octA: 1, octB: 1, vMode: 2, portaTime: 0.20, f1T: 1, f1C: 0.72, f1R: 0.75, distE: true, distT: 5, distD: 0.58, delayE: true, delayM: 0.28, mDb: -2.5 },
  { name: "41. LEAD // Stadium Anthem Supersaw", waves: "SAW + SAW", fx: "16x Unison + Multi-Band OTT", desc: "Festival mainstage supersaw lead with maximum stereo width and punch.",
    wA: 0, wB: 0, octA: 0, octB: 0, uV: 16, uDet: 0.38, uSpr: 1.0, uBl: 0.75, compE: true, compM: 0.80, reverbE: true, reverbM: 0.30, mDb: -4.0 },
  { name: "42. LEAD // Retro Funk Synth", waves: "SAW + SQR", fx: "Vibrato + Tube Saturation", desc: "Classic West Coast G-funk whistle lead with expressive vibrato LFO.",
    wA: 0, wB: 1, octA: 1, octB: 1, vMode: 2, portaTime: 0.26, vibD: 0.32, vibS: 6.2, f1T: 1, f1C: 0.65, f1R: 0.25, delayE: true, delayM: 0.24, mDb: -1.5 }
,
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
];

leadDefs.forEach((d, i) => {
  const numStr = (presets.length < 10 ? "0" : "") + presets.length;
  const cleanName = d.name.replace(/^\d+\.\s*/, "");
  const p = basePreset(presets.length, numStr + ". " + cleanName, "LEADS", "Prismor Sound", d.waves, d.fx, d.desc);
  p.waveA = d.wA; p.waveB = d.wB; p.octA = d.octA; p.octB = d.octB;
  if (d.semiB !== undefined) p.semiB = d.semiB;
  if (d.fineB !== undefined) p.fineB = d.fineB;
  if (d.warpA !== undefined) { p.warpModeA = d.warpA; p.warpAmtA = d.warpAmtA; }
  if (d.vMode !== undefined) p.voicingMode = d.vMode;
  if (d.portaTime !== undefined) p.portaTime = d.portaTime;
  if (d.uV !== undefined) { p.unisonVoices = d.uV; p.unisonDetune = d.uDet || 0.24; p.unisonSpread = d.uSpr || 0.75; p.unisonBlend = d.uBl || 0.60; p.unisonStack = d.uStack || 0; }
  if (d.vibD !== undefined) { p.vibDepth = d.vibD; p.vibSpeed = d.vibS || 5.5; }
  if (d.f1T !== undefined) p.filter1Type = d.f1T;
  if (d.f1C !== undefined) p.f1Cutoff = d.f1C;
  if (d.f1R !== undefined) p.f1Res = d.f1R;
  if (d.distE) { p.distEnable = true; if (d.distT !== undefined) p.distType = d.distT; if (d.distD !== undefined) p.distDrive = d.distD; }
  if (d.compE) { p.compEnable = true; if (d.compM !== undefined) p.compMix = d.compM; }
  if (d.chorusE) { p.chorusEnable = true; if (d.chorusM !== undefined) p.chorusMix = d.chorusM; if (d.chorusV !== undefined) p.chorusVoices = d.chorusV; }
  if (d.flangerE) { p.flangerEnable = true; if (d.flangerMode !== undefined) p.flangerMode = d.flangerMode; if (d.flangerM !== undefined) p.flangerMix = d.flangerM; }
  if (d.ringE) { p.ringEnable = true; if (d.ringMode !== undefined) p.ringMode = d.ringMode; if (d.ringFreq !== undefined) p.ringFreq = d.ringFreq; if (d.ringD !== undefined) p.ringDepth = d.ringD; if (d.ringM !== undefined) p.ringMix = d.ringM; }
  if (d.crushE) { p.crushEnable = true; if (d.crushMode !== undefined) p.crushMode = d.crushMode; if (d.crushBits !== undefined) p.crushBits = d.crushBits; if (d.crushDown !== undefined) p.crushDownsample = d.crushDown; if (d.crushM !== undefined) p.crushMix = d.crushM; }
  if (d.delayE) { p.delayEnable = true; if (d.delayT !== undefined) p.delayTime = d.delayT; if (d.delayM !== undefined) p.delayMix = d.delayM; }
  if (d.reverbE) { p.reverbEnable = true; if (d.reverbSize !== undefined) p.reverbSize = d.reverbSize; if (d.reverbShim !== undefined) p.reverbShimmer = d.reverbShim; if (d.reverbM !== undefined) p.reverbMix = d.reverbM; }
  if (d.envA !== undefined) p.envAttack = d.envA;
  if (d.envD !== undefined) p.envDecay = d.envD;
  if (d.envS !== undefined) p.envSustain = d.envS;
  if (d.shaperTarget !== undefined) p.shaperPresetTarget = d.shaperTarget;
  if (d.mDb !== undefined) p.masterGainDb = d.mDb;
  addPreset(p);
});

// -----------------------------------------------------------------------------
// Category 3: PADS & ATMOSPHERES (20 Patches: indices 43 to 62)
// -----------------------------------------------------------------------------
const padDefs = [
  { name: "43. PADS // Lush Stereo Choir Pad", waves: "ORG + FMT", fx: "Vox Choir + Shimmer Reverb", desc: "10-voice formant vowel choir pad with breath air noise and long swell.",
    wA: 3, wB: 4, octA: 0, octB: 0, uV: 10, uDet: 0.25, uSpr: 0.90, noiseE: true, noiseT: 5, noiseL: 0.15, envA: 0.45, envD: 0.60, envS: 0.85, envR: 0.55, reverbE: true, reverbSize: 0.85, reverbShim: 0.35, reverbM: 0.38, mDb: -2.5 },
  { name: "44. PADS // Valhalla Glass Horizon", waves: "TRI + ORG", fx: "Prism + Shimmer Hall", desc: "Crystalline inharmonic prism dispersion pad with massive shimmer tail.",
    wA: 2, wB: 3, octA: 0, octB: 1, uV: 8, uDet: 0.20, envA: 0.50, envD: 0.65, envS: 0.90, envR: 0.65, reverbE: true, reverbSize: 0.92, reverbShim: 0.55, reverbM: 0.45, chorusE: true, chorusM: 0.30, mDb: -3.0 },
  { name: "45. PADS // Through-Zero Flanged Pad", waves: "SAW + SINE", fx: "TZ Flanger + Chorus + Rev", desc: "Deep tape-style through-zero flanger sweep over warm analog-style strings.",
    wA: 0, wB: 6, octA: 0, octB: 0, uV: 8, uDet: 0.24, flangerE: true, flangerMode: 1, flangerM: 0.50, chorusE: true, chorusM: 0.32, reverbE: true, reverbM: 0.28, envA: 0.35, envR: 0.45, mDb: -2.0 },
  { name: "46. PADS // Zenology X-Mod Nebula", waves: "X-MOD + BEL", fx: "Freq Shifter + Blur + Rev", desc: "Evolving cross-modulated cosmic pad with frequency-shifted stereo halo.",
    wA: 9, wB: 8, octA: 0, octB: 0, uV: 6, uDet: 0.22, ringE: true, ringMode: 1, ringFreq: 65.0, ringD: 0.50, ringM: 0.35, reverbE: true, reverbSize: 0.88, reverbM: 0.40, mDb: -2.5 },
  { name: "47. PADS // Cyberpunk Dark Drones", waves: "SAW + SQR", fx: "Phaser + Tape Drive + Blur", desc: "Low-pitched ominous drone pad with slow phaser and spectral blur.",
    wA: 0, wB: 1, octA: -1, octB: -1, uV: 8, uDet: 0.26, phaserE: true, phaserM: 0.45, distE: true, distT: 1, distD: 0.30, envA: 0.60, envR: 0.65, mDb: -2.5 },
  { name: "48. PADS // Shimmering Celestial Choir", waves: "FMT + ORG", fx: "Air Noise + Octave Shimmer", desc: "Delicate upper-register choir with octave shimmer reverb and air noise.",
    wA: 4, wB: 3, octA: 1, octB: 0, uV: 8, uDet: 0.20, noiseE: true, noiseT: 5, noiseL: 0.18, reverbE: true, reverbSize: 0.90, reverbShim: 0.60, reverbM: 0.42, mDb: -3.0 },
  { name: "49. PADS // Ambient Crystal Swell", waves: "BEL + ORG", fx: "Chorus Spread + Plate Hall", desc: "Bell + Organ dual-layer with long envelope attack and stereo chorus.",
    wA: 8, wB: 3, octA: 0, octB: 1, uV: 6, uDet: 0.18, chorusE: true, chorusM: 0.38, reverbE: true, reverbM: 0.35, envA: 0.55, envR: 0.50, mDb: -2.0 },
  { name: "50. PADS // Deep Space Voyager", waves: "X-MOD + SAW", fx: "Stereo Delay + Damped Hall", desc: "X-Mod sideband evolving textures with ping-pong delays and damp hall.",
    wA: 9, wB: 0, octA: 0, octB: -1, uV: 6, uDet: 0.22, delayE: true, delayT: 0.45, delayM: 0.30, reverbE: true, reverbM: 0.32, mDb: -2.0 },
  { name: "51. PADS // Vintage Warm Strings", waves: "SAW + SAW", fx: "Solina Chorus + Warm EQ", desc: "Classic Solina-style ensemble strings with gentle multi-voice chorus.",
    wA: 0, wB: 0, octA: 0, octB: 0, fineB: 7.0, uV: 8, uDet: 0.24, chorusE: true, chorusV: 6, chorusM: 0.50, f1T: 1, f1C: 0.65, f1R: 0.20, envA: 0.30, envR: 0.40, mDb: -1.5 },
  { name: "52. PADS // Organic Forest Ambience", waves: "RND + TRI", fx: "Air Noise + Dispersion Reverb", desc: "Random partial dispersion with soft noise and shimmering reverb tail.",
    wA: 5, wB: 2, octA: 0, octB: 0, noiseE: true, noiseT: 1, noiseL: 0.16, reverbE: true, reverbSize: 0.85, reverbM: 0.35, envA: 0.40, envR: 0.50, mDb: -2.0 },
  { name: "53. PADS // Cold Industrial Drone", waves: "SQR + SYNC", fx: "Comb Filter + Wavefold", desc: "Crushed metallic pad with resonant comb filter and tape distortion.",
    wA: 1, wB: 7, octA: -1, octB: 0, f1T: 5, f1C: 0.58, f1R: 0.50, distE: true, distT: 3, distD: 0.35, envA: 0.45, envR: 0.60, mDb: -2.5 },
  { name: "54. PADS // Evolving Morph Pad", waves: "SAW + BEL", fx: "A/B Crossfade + Ping-Pong", desc: "A/B crossfade modulation pad shifting from saw harmonics to crystalline bell.",
    wA: 0, wB: 8, octA: 0, octB: 1, abM: 0.50, uV: 8, uDet: 0.22, delayE: true, delayM: 0.28, reverbE: true, reverbM: 0.30, envA: 0.48, envR: 0.52, mDb: -2.0 },
  { name: "55. PADS // Dreamstate Aurora", waves: "TRI + SINE", fx: "Harmonic Tilt + Auto-Pan", desc: "High-harmonic tilt pad with smooth stereo auto-pan and shimmer reverb.",
    wA: 2, wB: 6, octA: 0, octB: 0, ringE: true, ringMode: 2, ringD: 0.60, ringM: 0.40, reverbE: true, reverbSize: 0.90, reverbShim: 0.45, reverbM: 0.40, envA: 0.50, envR: 0.55, mDb: -2.5 },
  { name: "56. PADS // Subdued Cinema Bed", waves: "SAW + ORG", fx: "Crude LP + Long Decay Hall", desc: "Muted warm lowpass pad designed as an emotional cinematic backdrop.",
    wA: 0, wB: 3, octA: -1, octB: 0, f1T: 2, f1C: 0.42, f1R: 0.15, envA: 0.60, envR: 0.70, reverbE: true, reverbSize: 0.92, reverbM: 0.42, mDb: -2.0 },
  { name: "57. PADS // Vocal Breath Atmosphere", waves: "FMT + SINE", fx: "Breath Noise + Vox Filter", desc: "Airy formant pad with pink noise layer and subtle vibrato movement.",
    wA: 4, wB: 6, octA: 0, octB: 0, noiseE: true, noiseT: 5, noiseL: 0.20, f1T: 6, f1C: 0.65, f1R: 0.35, vibD: 0.15, vibS: 4.5, envA: 0.42, envR: 0.50, reverbE: true, reverbM: 0.35, mDb: -2.0 },
  { name: "58. PADS // Shaper Swell Pad", waves: "SAW + SQR", fx: "Rhythmic Swell + Delay", desc: "Articulator envelope swell pad synced to host tempo with soft saturation.",
    wA: 0, wB: 1, octA: 0, octB: 0, uV: 8, uDet: 0.24, shaperTarget: 7, delayE: true, delayM: 0.30, reverbE: true, reverbM: 0.32, mDb: -2.5 },
  { name: "59. PADS // Crystalline Starlight", waves: "BEL + RND", fx: "Prism Dispersion + Hall", desc: "Sparkle waveform with harmonic prism dispersion and long decay hall.",
    wA: 8, wB: 5, octA: 1, octB: 1, uV: 6, uDet: 0.18, reverbE: true, reverbSize: 0.95, reverbShim: 0.60, reverbM: 0.45, envA: 0.40, envR: 0.60, mDb: -3.0 },
  { name: "60. PADS // Dark Matter Nebula", waves: "X-MOD + SQR", fx: "Freq Shifter + Stereo Width", desc: "Slow evolving drone with frequency shifter and deep stereo width.",
    wA: 9, wB: 1, octA: -1, octB: 0, ringE: true, ringMode: 1, ringFreq: 45.0, ringD: 0.45, ringM: 0.35, phaserE: true, phaserM: 0.35, reverbE: true, reverbM: 0.30, mDb: -2.5 },
  { name: "61. PADS // Retro Tape Strings", waves: "SAW + TRI", fx: "Tape Wow + Vintage Lo-Fi", desc: "Analog strings running through tape wow/flutter and subtle lo-fi grit.",
    wA: 0, wB: 2, octA: 0, octB: 0, vibD: 0.20, vibS: 3.8, distE: true, distT: 1, distD: 0.30, crushE: true, crushMode: 2, crushBits: 12.0, crushM: 0.25, envA: 0.35, envR: 0.45, mDb: -1.5 },
  { name: "62. PADS // Floating Organ Cloud", waves: "ORG + ORG", fx: "Rotary Speaker + Shimmer", desc: "Sustained organ harmonics with Leslie-style rotary speaker modulation.",
    wA: 3, wB: 3, octA: 0, octB: 1, ringE: true, ringMode: 3, ringFreq: 180.0, ringD: 0.65, ringM: 0.45, reverbE: true, reverbSize: 0.88, reverbM: 0.35, envA: 0.30, envR: 0.45, mDb: -2.0 }
,
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
];

padDefs.forEach((d, i) => {
  const numStr = (presets.length < 10 ? "0" : "") + presets.length;
  const cleanName = d.name.replace(/^\d+\.\s*/, "");
  const p = basePreset(presets.length, numStr + ". " + cleanName, "PADS", "Prismor Sound", d.waves, d.fx, d.desc);
  p.waveA = d.wA; p.waveB = d.wB; p.octA = d.octA; p.octB = d.octB;
  if (d.fineB !== undefined) p.fineB = d.fineB;
  if (d.abM !== undefined) p.abMix = d.abM;
  if (d.uV !== undefined) { p.unisonVoices = d.uV; p.unisonDetune = d.uDet || 0.20; p.unisonSpread = d.uSpr || 0.80; }
  if (d.noiseE) { p.noiseEnable = true; if (d.noiseT !== undefined) p.noiseType = d.noiseT; if (d.noiseL !== undefined) p.noiseLevel = d.noiseL; }
  if (d.f1T !== undefined) p.filter1Type = d.f1T;
  if (d.f1C !== undefined) p.f1Cutoff = d.f1C;
  if (d.f1R !== undefined) p.f1Res = d.f1R;
  if (d.flangerE) { p.flangerEnable = true; if (d.flangerMode !== undefined) p.flangerMode = d.flangerMode; if (d.flangerM !== undefined) p.flangerMix = d.flangerM; }
  if (d.chorusE) { p.chorusEnable = true; if (d.chorusV !== undefined) p.chorusVoices = d.chorusV; if (d.chorusM !== undefined) p.chorusMix = d.chorusM; }
  if (d.ringE) { p.ringEnable = true; if (d.ringMode !== undefined) p.ringMode = d.ringMode; if (d.ringFreq !== undefined) p.ringFreq = d.ringFreq; if (d.ringD !== undefined) p.ringDepth = d.ringD; if (d.ringM !== undefined) p.ringMix = d.ringM; }
  if (d.phaserE) { p.phaserEnable = true; if (d.phaserM !== undefined) p.phaserMix = d.phaserM; }
  if (d.distE) { p.distEnable = true; if (d.distT !== undefined) p.distType = d.distT; if (d.distD !== undefined) p.distDrive = d.distD; }
  if (d.crushE) { p.crushEnable = true; if (d.crushMode !== undefined) p.crushMode = d.crushMode; if (d.crushBits !== undefined) p.crushBits = d.crushBits; if (d.crushM !== undefined) p.crushMix = d.crushM; }
  if (d.vibD !== undefined) { p.vibDepth = d.vibD; p.vibSpeed = d.vibS || 5.0; }
  if (d.delayE) { p.delayEnable = true; if (d.delayT !== undefined) p.delayTime = d.delayT; if (d.delayM !== undefined) p.delayMix = d.delayM; }
  if (d.reverbE) { p.reverbEnable = true; if (d.reverbSize !== undefined) p.reverbSize = d.reverbSize; if (d.reverbShim !== undefined) p.reverbShimmer = d.reverbShim; if (d.reverbM !== undefined) p.reverbMix = d.reverbM; }
  if (d.envA !== undefined) p.envAttack = d.envA;
  if (d.envD !== undefined) p.envDecay = d.envD;
  if (d.envS !== undefined) p.envSustain = d.envS;
  if (d.envR !== undefined) p.envRelease = d.envR;
  if (d.shaperTarget !== undefined) p.shaperPresetTarget = d.shaperTarget;
  if (d.mDb !== undefined) p.masterGainDb = d.mDb;
  addPreset(p);
});

// -----------------------------------------------------------------------------
// Category 4: PLUCKS & KEYS (20 Patches: indices 63 to 82)
// -----------------------------------------------------------------------------
const pluckDefs = [
  { name: "63. PLUCKS // Resynthesized Pluck", waves: "FMT + RND", fx: "Harmonic Strum + Delay", desc: "Quantized spectral pluck with harmonic onset strum and ping-pong echo.",
    wA: 4, wB: 5, octA: 0, octB: 0, envA: 0.01, envD: 0.26, envS: 0.0, envR: 0.18, delayE: true, delayM: 0.28, reverbE: true, reverbM: 0.22, mDb: -1.0 },
  { name: "64. PLUCKS // Acoustic Spectral Harp", waves: "TRI + ORG", fx: "Pluck Decay + Hall Reverb", desc: "Delicate plucked string harmonics with natural high-frequency damping.",
    wA: 2, wB: 3, octA: 0, octB: 1, envA: 0.01, envD: 0.32, envS: 0.0, envR: 0.25, reverbE: true, reverbSize: 0.85, reverbM: 0.35, mDb: -1.5 },
  { name: "65. PLUCKS // Zenology FM Crystal Bell", waves: "BEL + SINE", fx: "Chorus + Shimmer Reverb", desc: "Authentic modal Bessel bell waveform with shimmering overtone tail.",
    wA: 8, wB: 6, octA: 1, octB: 0, envA: 0.01, envD: 0.38, envS: 0.10, envR: 0.30, chorusE: true, chorusM: 0.32, reverbE: true, reverbShim: 0.45, reverbM: 0.35, mDb: -1.5 },
  { name: "66. PLUCKS // Ring Mod Tubular Chime", waves: "BEL + X-MOD", fx: "Ring Mod + Ping-Pong Delay", desc: "Clangorous metallic tubular chime sculpted with carrier ring modulation.",
    wA: 8, wB: 9, octA: 0, octB: 0, ringE: true, ringMode: 0, ringFreq: 440.0, ringD: 0.65, ringM: 0.45, delayE: true, delayM: 0.30, envA: 0.01, envD: 0.35, envS: 0.0, mDb: -2.0 },
  { name: "67. PLUCKS // Progressive House Pluck", waves: "SAW + SAW", fx: "6x Unison + Stereo Ping-Pong", desc: "Short-decay 6-voice detuned saw pluck with filtered ping-pong delay.",
    wA: 0, wB: 0, octA: 0, octB: 0, fineB: 8.0, uV: 6, uDet: 0.22, envA: 0.01, envD: 0.22, envS: 0.0, envR: 0.15, f1T: 1, f1C: 0.65, f1R: 0.25, delayE: true, delayT: 0.375, delayM: 0.32, reverbE: true, reverbM: 0.22, mDb: -2.0 },
  { name: "68. PLUCKS // Future Bass Kalimba", waves: "TRI + SINE", fx: "Pitch Drop + Warm Plate", desc: "Warm percussive mallet pluck with subtle pitch drop and room reverb.",
    wA: 2, wB: 6, octA: 0, octB: 1, envA: 0.01, envD: 0.20, envS: 0.0, envR: 0.15, reverbE: true, reverbM: 0.25, mDb: -1.0 },
  { name: "69. PLUCKS // Lo-Fi SP-1200 Keys", waves: "SQR + TRI", fx: "SP-1200 Crush + Tape Wow", desc: "12-bit vintage sampler electric piano with vinyl noise and vibrato.",
    wA: 1, wB: 2, octA: 0, octB: 0, crushE: true, crushMode: 2, crushBits: 10.0, crushDown: 3.0, crushM: 0.45, vibD: 0.18, vibS: 4.2, noiseE: true, noiseT: 3, noiseL: 0.12, envA: 0.02, envD: 0.45, envS: 0.40, mDb: -1.5 },
  { name: "70. PLUCKS // Glass Marimba", waves: "BEL + TRI", fx: "Percussive Transient + Room", desc: "Percussive bell mallet with sharp transient attack and warm decay.",
    wA: 8, wB: 2, octA: 1, octB: 0, envA: 0.01, envD: 0.18, envS: 0.0, envR: 0.12, reverbE: true, reverbM: 0.24, mDb: -1.0 },
  { name: "71. PLUCKS // Trance Gated Pluck", waves: "SAW + SYNC", fx: "Fast Decay + 1/16 Echo", desc: "Snappy pluck running into rhythmic 16th-note ping-pong delay.",
    wA: 0, wB: 7, octA: 0, octB: 0, envA: 0.01, envD: 0.16, envS: 0.0, envR: 0.10, delayE: true, delayT: 0.25, delayM: 0.35, reverbE: true, reverbM: 0.20, mDb: -1.5 },
  { name: "72. PLUCKS // Cyber Strum Mallet", waves: "FMT + ORG", fx: "Harmonic Strum + Chorus", desc: "Dual-layer strummed mallet with A/B crossfade and chorus spread.",
    wA: 4, wB: 3, octA: 0, octB: 1, abM: 0.45, chorusE: true, chorusM: 0.30, delayE: true, delayM: 0.25, envA: 0.01, envD: 0.25, envS: 0.0, mDb: -1.5 },
  { name: "73. PLUCKS // Vintage Electric Tines", waves: "SINE + TRI", fx: "Tremolo + Tube Saturation", desc: "Rhodes-style warm FM tines with gentle tremolo and tube drive.",
    wA: 6, wB: 2, octA: 0, octB: 0, distE: true, distT: 0, distD: 0.25, ringE: true, ringMode: 2, ringFreq: 120.0, ringD: 0.45, ringM: 0.30, envA: 0.01, envD: 0.48, envS: 0.35, mDb: -1.0 },
  { name: "74. PLUCKS // Metallic Karplus String", waves: "RND + SQR", fx: "Spectral Blur + Comb", desc: "Pluck engine with extreme blur for physical acoustic string resonance.",
    wA: 5, wB: 1, octA: 0, octB: 0, f1T: 5, f1C: 0.62, f1R: 0.55, envA: 0.01, envD: 0.35, envS: 0.0, delayE: true, delayM: 0.25, mDb: -1.5 },
  { name: "75. PLUCKS // 8-Bit Arcade Pluck", waves: "SQR + SQR", fx: "Digital Bitcrush + Arp Gate", desc: "Chiptune square-wave pluck with sample-decimated downsampling.",
    wA: 1, wB: 1, octA: 0, octB: 1, crushE: true, crushMode: 0, crushBits: 6.0, crushM: 0.55, envA: 0.01, envD: 0.15, envS: 0.0, mDb: -1.0 },
  { name: "76. PLUCKS // Tropical Steel Pan", waves: "BEL + ORG", fx: "Bessel Harmonics + Chorus", desc: "Harmonic bell waveform tuned to bright melodic Caribbean steel drums.",
    wA: 8, wB: 3, octA: 0, octB: 0, chorusE: true, chorusM: 0.28, reverbE: true, reverbM: 0.25, envA: 0.01, envD: 0.28, envS: 0.10, mDb: -1.0 },
  { name: "77. PLUCKS // Dark Ambient Bell", waves: "BEL + X-MOD", fx: "Deep Damping + Shimmer", desc: "Low-register deep bell chime with dark damping and long shimmer reverb.",
    wA: 8, wB: 9, octA: -1, octB: 0, f1T: 1, f1C: 0.52, f1R: 0.20, reverbE: true, reverbSize: 0.90, reverbShim: 0.45, reverbM: 0.40, envA: 0.01, envD: 0.55, envS: 0.15, mDb: -2.0 },
  { name: "78. PLUCKS // Synthwave Funk Clav", waves: "SAW + SQR", fx: "Bandpass Filter + Tube Drive", desc: "Piercing funky clavinet pluck with resonant bandpass filter and drive.",
    wA: 0, wB: 1, octA: 0, octB: 0, f1T: 4, f1C: 0.65, f1R: 0.40, distE: true, distT: 0, distD: 0.35, envA: 0.01, envD: 0.22, envS: 0.0, mDb: -1.0 },
  { name: "79. PLUCKS // Shimmer Harp Cascade", waves: "TRI + BEL", fx: "Octave Delay + Shimmer Hall", desc: "Rapid harp pluck with harmonic shift and stereo delay reflections.",
    wA: 2, wB: 8, octA: 0, octB: 1, delayE: true, delayT: 0.33, delayM: 0.32, reverbE: true, reverbShim: 0.50, reverbM: 0.35, envA: 0.01, envD: 0.30, envS: 0.0, mDb: -2.0 },
  { name: "80. PLUCKS // Analog Poly Keys", waves: "SAW + SQR", fx: "4-Voice Chorus + Plate", desc: "Classic 80s analog polyphonic synth keys with 4-voice unison.",
    wA: 0, wB: 1, octA: 0, octB: 0, uV: 4, uDet: 0.18, chorusE: true, chorusM: 0.35, envA: 0.02, envD: 0.40, envS: 0.50, reverbE: true, reverbM: 0.24, mDb: -1.5 },
  { name: "81. PLUCKS // Pizzicato Orchestra", waves: "SAW + TRI", fx: "Natural Damping + Room", desc: "Crisp orchestral-style string pluck with natural wood body resonance.",
    wA: 0, wB: 2, octA: 0, octB: 0, f1T: 1, f1C: 0.58, f1R: 0.22, envA: 0.01, envD: 0.20, envS: 0.0, reverbE: true, reverbM: 0.22, mDb: -1.0 },
  { name: "82. PLUCKS // Dreamy Music Box", waves: "BEL + SINE", fx: "Micro-Tuned Bell + Hall", desc: "Soft micro-tuned bell chime with delicate vibrato and hall reverb.",
    wA: 8, wB: 6, octA: 1, octB: 1, fineB: 5.0, vibD: 0.16, vibS: 4.8, reverbE: true, reverbSize: 0.85, reverbM: 0.32, envA: 0.01, envD: 0.32, envS: 0.0, mDb: -1.5 }
,
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
];

pluckDefs.forEach((d, i) => {
  const numStr = (presets.length < 10 ? "0" : "") + presets.length;
  const cleanName = d.name.replace(/^\d+\.\s*/, "");
  const p = basePreset(presets.length, numStr + ". " + cleanName, "PLUCKS", "Prismor Sound", d.waves, d.fx, d.desc);
  p.waveA = d.wA; p.waveB = d.wB; p.octA = d.octA; p.octB = d.octB;
  if (d.fineB !== undefined) p.fineB = d.fineB;
  if (d.abM !== undefined) p.abMix = d.abM;
  if (d.uV !== undefined) { p.unisonVoices = d.uV; p.unisonDetune = d.uDet || 0.18; }
  if (d.f1T !== undefined) p.filter1Type = d.f1T;
  if (d.f1C !== undefined) p.f1Cutoff = d.f1C;
  if (d.f1R !== undefined) p.f1Res = d.f1R;
  if (d.envA !== undefined) p.envAttack = d.envA;
  if (d.envD !== undefined) p.envDecay = d.envD;
  if (d.envS !== undefined) p.envSustain = d.envS;
  if (d.envR !== undefined) p.envRelease = d.envR;
  if (d.distE) { p.distEnable = true; if (d.distT !== undefined) p.distType = d.distT; if (d.distD !== undefined) p.distDrive = d.distD; }
  if (d.chorusE) { p.chorusEnable = true; if (d.chorusM !== undefined) p.chorusMix = d.chorusM; }
  if (d.ringE) { p.ringEnable = true; if (d.ringMode !== undefined) p.ringMode = d.ringMode; if (d.ringFreq !== undefined) p.ringFreq = d.ringFreq; if (d.ringD !== undefined) p.ringDepth = d.ringD; if (d.ringM !== undefined) p.ringMix = d.ringM; }
  if (d.crushE) { p.crushEnable = true; if (d.crushMode !== undefined) p.crushMode = d.crushMode; if (d.crushBits !== undefined) p.crushBits = d.crushBits; if (d.crushDown !== undefined) p.crushDownsample = d.crushDown; if (d.crushM !== undefined) p.crushMix = d.crushM; }
  if (d.noiseE) { p.noiseEnable = true; if (d.noiseT !== undefined) p.noiseType = d.noiseT; if (d.noiseL !== undefined) p.noiseLevel = d.noiseL; }
  if (d.vibD !== undefined) { p.vibDepth = d.vibD; p.vibSpeed = d.vibS || 5.0; }
  if (d.delayE) { p.delayEnable = true; if (d.delayT !== undefined) p.delayTime = d.delayT; if (d.delayM !== undefined) p.delayMix = d.delayM; }
  if (d.reverbE) { p.reverbEnable = true; if (d.reverbSize !== undefined) p.reverbSize = d.reverbSize; if (d.reverbShim !== undefined) p.reverbShimmer = d.reverbShim; if (d.reverbM !== undefined) p.reverbMix = d.reverbM; }
  if (d.mDb !== undefined) p.masterGainDb = d.mDb;
  addPreset(p);
});

// -----------------------------------------------------------------------------
// Category 5: ARPS & SEQUENCES (20 Patches: indices 83 to 102)
// -----------------------------------------------------------------------------
const arpDefs = [
  { name: "83. ARP & SEQ // Hive-2 Berlin Sequence", waves: "SAW + SQR", fx: "16-Step Arp + Ping-Pong", desc: "Up/Down 2-octave driving Berlin modular sequence with swing.",
    wA: 0, wB: 1, octA: 0, octB: 0, arpE: true, arpMode: 2, arpOct: 2, arpRate: 3, arpG: 0.75, arpSw: 0.15, delayE: true, delayM: 0.28, reverbE: true, reverbM: 0.20, mDb: -1.5 },
  { name: "84. ARP & SEQ // Synthwave 16th Runner", waves: "SAW + SAW", fx: "Arp + Chorus + Delay", desc: "Classic 80s rolling 16th-note synthwave arpeggio across 3 octaves.",
    wA: 0, wB: 0, octA: 0, octB: 0, fineB: 7.0, uV: 4, uDet: 0.20, arpE: true, arpMode: 0, arpOct: 3, arpRate: 3, arpG: 0.70, chorusE: true, chorusM: 0.35, delayE: true, delayM: 0.28, mDb: -2.0 },
  { name: "85. ARP & SEQ // Acid 303 Stepper", waves: "SAW + SQR", fx: "Acid 24dB + Asym Scream", desc: "Squelchy resonant 303 step sequence with octave jumps and fifths.",
    wA: 0, wB: 1, octA: -1, octB: -1, arpE: true, arpMode: 2, arpOct: 2, arpRate: 3, f1T: 1, f1C: 0.48, f1R: 0.75, distE: true, distT: 5, distD: 0.55, mDb: -2.0 },
  { name: "86. ARP & SEQ // Bitcrushed Chiptune", waves: "SQR + SYNC", fx: "Arp + 6-Bit Crusher", desc: "Arcade chiptune arpeggiator running through the Zenology Lo-Fi Bitcrusher.",
    wA: 1, wB: 7, octA: 0, octB: 1, arpE: true, arpMode: 0, arpOct: 2, arpRate: 3, crushE: true, crushMode: 0, crushBits: 6.0, crushM: 0.55, delayE: true, delayM: 0.20, mDb: -1.5 },
  { name: "87. ARP & SEQ // Cyberpunk Trance Gate", waves: "SAW + SQR", fx: "Shaper Gate + Wavefold", desc: "Rhythmic 16-step trance gate sequence with comb filter and wavefolding.",
    wA: 0, wB: 1, octA: 0, octB: 0, uV: 8, uDet: 0.25, shaperTarget: 1, distE: true, distT: 3, distD: 0.40, delayE: true, delayM: 0.25, reverbE: true, reverbM: 0.22, mDb: -2.5 },
  { name: "88. ARP & SEQ // Melodic Techno Pulse", waves: "SAW + SINE", fx: "Tape Drive + Ping-Pong", desc: "Dark rolling 16th-note bassline arpeggio with subtle delay and reverb.",
    wA: 0, wB: 6, octA: -1, octB: -1, arpE: true, arpMode: 0, arpOct: 2, arpRate: 3, arpG: 0.65, f1T: 1, f1C: 0.52, f1R: 0.35, distE: true, distT: 1, distD: 0.35, delayE: true, delayM: 0.25, mDb: -1.5 },
  { name: "89. ARP & SEQ // Euphoric Trance 1/16", waves: "SAW + SYNC", fx: "8x Unison + Stereo Ping-Pong", desc: "Bright supersaw arpeggio climbing 3 octaves with stereo ping-pong.",
    wA: 0, wB: 7, octA: 0, octB: 0, uV: 8, uDet: 0.28, arpE: true, arpMode: 0, arpOct: 3, arpRate: 3, delayE: true, delayT: 0.375, delayM: 0.32, reverbE: true, reverbM: 0.28, mDb: -2.5 },
  { name: "90. ARP & SEQ // Random Generative Bell", waves: "BEL + TRI", fx: "Random Arp + Shimmer", desc: "Algorithmic random arpeggiator picking glassy bell harmonics.",
    wA: 8, wB: 2, octA: 1, octB: 0, arpE: true, arpMode: 3, arpOct: 3, arpRate: 2, reverbE: true, reverbShim: 0.50, reverbM: 0.35, delayE: true, delayM: 0.25, mDb: -2.0 },
  { name: "91. ARP & SEQ // Industrial Clockwork", waves: "SYNC + X-MOD", fx: "Jet Flanger + Ring Mod", desc: "Heavy bitcrushed sequence with jet flanger and metallic ring modulation.",
    wA: 7, wB: 9, octA: 0, octB: 0, arpE: true, arpMode: 2, arpOct: 2, arpRate: 3, flangerE: true, flangerM: 0.45, ringE: true, ringMode: 0, ringFreq: 150.0, ringD: 0.55, ringM: 0.35, mDb: -2.5 },
  { name: "92. ARP & SEQ // Chord Strum Arp", waves: "SAW + ORG", fx: "Chord Arp + Delay", desc: "Arpeggiated chord voicings with strum delay and harmonic tilt.",
    wA: 0, wB: 3, octA: 0, octB: 0, uV: 6, uDet: 0.20, arpE: true, arpMode: 4, arpOct: 2, arpRate: 1, delayE: true, delayM: 0.25, reverbE: true, reverbM: 0.22, mDb: -2.0 },
  { name: "93. ARP & SEQ // 80s Tangerine Runner", waves: "SAW + TRI", fx: "Analog Delay + Filter Sweep", desc: "Vintage modular sequence with smooth cutoff envelope and analog delay.",
    wA: 0, wB: 2, octA: 0, octB: 0, arpE: true, arpMode: 0, arpOct: 2, arpRate: 3, f1T: 1, f1C: 0.58, f1R: 0.30, delayE: true, delayM: 0.30, reverbE: true, reverbM: 0.20, mDb: -1.5 },
  { name: "94. ARP & SEQ // Future Garage Swing", waves: "SQR + FMT", fx: "Swing Arp + Vinyl Noise", desc: "Syncopated swing arpeggio with vinyl noise and sidechain ducking.",
    wA: 1, wB: 4, octA: 0, octB: 0, arpE: true, arpMode: 2, arpOct: 2, arpRate: 3, arpSw: 0.35, noiseE: true, noiseT: 3, noiseL: 0.14, delayE: true, delayM: 0.25, mDb: -1.5 },
  { name: "95. ARP & SEQ // ShaperBox Stutter Arp", waves: "SAW + SQR", fx: "Gross Beat 1/8 Gate + OTT", desc: "Buffer-chopped rhythmic sequence with Gross Beat style 1/8 gates.",
    wA: 0, wB: 1, octA: 0, octB: 0, uV: 6, uDet: 0.22, timeModE: true, timeModMode: 1, timeModRate: 4, timeModD: 0.70, timeModM: 0.75, compE: true, compM: 0.65, mDb: -2.5 },
  { name: "96. ARP & SEQ // Neuro Bassline Arp", waves: "SAW + FMT", fx: "Formant Vowel + Bitcrush", desc: "Rapid 16th-note resonant bass sequence with formant vowel movement.",
    wA: 0, wB: 4, octA: -1, octB: -1, arpE: true, arpMode: 2, arpOct: 2, arpRate: 3, f1T: 6, f1C: 0.65, f1R: 0.45, distE: true, distT: 4, distD: 0.35, mDb: -2.0 },
  { name: "97. ARP & SEQ // Cyber Velocity Groove", waves: "SYNC + SAW", fx: "Velocity Link + Ping-Pong", desc: "Dynamic velocity-linked arpeggio with opening filter on accents.",
    wA: 7, wB: 0, octA: 0, octB: 0, arpE: true, arpMode: 0, arpOct: 2, arpRate: 3, velL: 0.85, delayE: true, delayM: 0.28, reverbE: true, reverbM: 0.20, mDb: -1.5 },
  { name: "98. ARP & SEQ // Retro Computer Lab", waves: "SQR + X-MOD", fx: "Fast 1/32 Arp + Crusher", desc: "Rapid arcade computer telemetry arpeggio with high bitcrush drive.",
    wA: 1, wB: 9, octA: 1, octB: 1, arpE: true, arpMode: 3, arpOct: 3, arpRate: 5, crushE: true, crushBits: 7.0, crushM: 0.50, delayE: true, delayM: 0.22, mDb: -1.5 },
  { name: "99. ARP & SEQ // Ambient Shimmer Arp", waves: "BEL + ORG", fx: "Slow Arp + Massive Shimmer", desc: "Slow tempo-synced bell sequence floating inside massive shimmer hall.",
    wA: 8, wB: 3, octA: 1, octB: 0, arpE: true, arpMode: 0, arpOct: 2, arpRate: 1, reverbE: true, reverbSize: 0.95, reverbShim: 0.55, reverbM: 0.45, delayE: true, delayM: 0.30, mDb: -2.5 },
  { name: "100. ARP & SEQ // Triplet Driving Bass", waves: "SAW + SQR", fx: "1/8T Arp + Asym Overdrive", desc: "1/8-triplet driving techno bass arpeggio with asymmetric overdrive.",
    wA: 0, wB: 1, octA: -1, octB: -1, arpE: true, arpMode: 0, arpOct: 2, arpRate: 2, distE: true, distT: 5, distD: 0.45, f1T: 1, f1C: 0.55, f1R: 0.35, mDb: -2.0 },
  { name: "101. ARP & SEQ // Dual-Arp Layer Split", waves: "TRI + BEL", fx: "A/B Mix + Chorus", desc: "Polyphonic arpeggiator sending Part A high and Part B low.",
    wA: 2, wB: 8, octA: -1, octB: 1, abM: 0.50, arpE: true, arpMode: 2, arpOct: 2, arpRate: 3, chorusE: true, chorusM: 0.30, reverbE: true, reverbM: 0.22, mDb: -1.5 },
  { name: "102. ARP & SEQ // Deep Space Pulser", waves: "SAW + SINE", fx: "TZ Flanger + Ping-Pong", desc: "Hypnotic pulse arpeggio with through-zero flanger and stereo ping-pong.",
    wA: 0, wB: 6, octA: 0, octB: 0, arpE: true, arpMode: 0, arpOct: 2, arpRate: 3, flangerE: true, flangerMode: 1, flangerM: 0.45, delayE: true, delayM: 0.28, mDb: -2.0 }
,
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
];

arpDefs.forEach((d, i) => {
  const numStr = (presets.length < 10 ? "0" : "") + presets.length;
  const cleanName = d.name.replace(/^\d+\.\s*/, "");
  const p = basePreset(presets.length, numStr + ". " + cleanName, "ARP & SEQ", "Prismor Sound", d.waves, d.fx, d.desc);
  p.waveA = d.wA; p.waveB = d.wB; p.octA = d.octA; p.octB = d.octB;
  if (d.fineB !== undefined) p.fineB = d.fineB;
  if (d.abM !== undefined) p.abMix = d.abM;
  if (d.uV !== undefined) { p.unisonVoices = d.uV; p.unisonDetune = d.uDet || 0.20; }
  if (d.arpE) { p.arpEnable = true; if (d.arpMode !== undefined) p.arpMode = d.arpMode; if (d.arpOct !== undefined) p.arpOctaves = d.arpOct; if (d.arpRate !== undefined) p.arpRate = d.arpRate; p.arpGate = d.arpG || 0.75; if (d.arpSw !== undefined) p.arpSwing = d.arpSw; }
  if (d.f1T !== undefined) p.filter1Type = d.f1T;
  if (d.f1C !== undefined) p.f1Cutoff = d.f1C;
  if (d.f1R !== undefined) p.f1Res = d.f1R;
  if (d.distE) { p.distEnable = true; if (d.distT !== undefined) p.distType = d.distT; if (d.distD !== undefined) p.distDrive = d.distD; }
  if (d.chorusE) { p.chorusEnable = true; if (d.chorusM !== undefined) p.chorusMix = d.chorusM; }
  if (d.flangerE) { p.flangerEnable = true; if (d.flangerMode !== undefined) p.flangerMode = d.flangerMode; if (d.flangerM !== undefined) p.flangerMix = d.flangerM; }
  if (d.ringE) { p.ringEnable = true; if (d.ringMode !== undefined) p.ringMode = d.ringMode; if (d.ringFreq !== undefined) p.ringFreq = d.ringFreq; if (d.ringD !== undefined) p.ringDepth = d.ringD; if (d.ringM !== undefined) p.ringMix = d.ringM; }
  if (d.crushE) { p.crushEnable = true; if (d.crushMode !== undefined) p.crushMode = d.crushMode; if (d.crushBits !== undefined) p.crushBits = d.crushBits; if (d.crushM !== undefined) p.crushMix = d.crushM; }
  if (d.timeModE) { p.timeModEnable = true; if (d.timeModMode !== undefined) p.timeModMode = d.timeModMode; if (d.timeModRate !== undefined) p.timeModRate = d.timeModRate; if (d.timeModD !== undefined) p.timeModDepth = d.timeModD; if (d.timeModM !== undefined) p.timeModMix = d.timeModM; }
  if (d.noiseE) { p.noiseEnable = true; if (d.noiseT !== undefined) p.noiseType = d.noiseT; if (d.noiseL !== undefined) p.noiseLevel = d.noiseL; }
  if (d.compE) { p.compEnable = true; if (d.compM !== undefined) p.compMix = d.compM; }
  if (d.shaperTarget !== undefined) p.shaperPresetTarget = d.shaperTarget;
  if (d.velL !== undefined) p.velLink = d.velL;
  if (d.delayE) { p.delayEnable = true; if (d.delayT !== undefined) p.delayTime = d.delayT; if (d.delayM !== undefined) p.delayMix = d.delayM; }
  if (d.reverbE) { p.reverbEnable = true; if (d.reverbSize !== undefined) p.reverbSize = d.reverbSize; if (d.reverbShim !== undefined) p.reverbShimmer = d.reverbShim; if (d.reverbM !== undefined) p.reverbMix = d.reverbM; }
  if (d.mDb !== undefined) p.masterGainDb = d.mDb;
  addPreset(p);
});

// -----------------------------------------------------------------------------
// Category 6: FX & TRANSITIONS / TIME MOD (21 Patches: indices 103 to 123)
// -----------------------------------------------------------------------------
const fxDefs = [
  { name: "103. FX & TRANS // Gross Beat Half-Time", waves: "BEL + SAW", fx: "Time Mod 1/2x + Reverb", desc: "Instant Gross Beat style 1/2x half-speed time-warped bell & pad atmosphere.",
    wA: 8, wB: 0, octA: 0, octB: 0, timeModE: true, timeModMode: 4, timeModRate: 2, timeModD: 0.85, timeModM: 0.90, reverbE: true, reverbM: 0.32, mDb: -1.5 },
  { name: "104. FX & TRANS // 1/8 Stutter Glitch", waves: "SYNC + X-MOD", fx: "Time Stutter + Flanger", desc: "Beat-synced 1/8 buffer stutter repeat with jet flanger and OTT.",
    wA: 7, wB: 9, octA: 0, octB: 0, timeModE: true, timeModMode: 1, timeModRate: 4, timeModD: 0.80, timeModM: 0.85, flangerE: true, flangerM: 0.40, compE: true, compM: 0.65, mDb: -2.0 },
  { name: "105. FX & TRANS // Vinyl Tape-Stop Lead", waves: "SAW + SINE", fx: "Tape Stop + Lo-Fi Crush", desc: "Rhythmic bar-end vinyl tape-stop deceleration with vintage SP-1200 grit.",
    wA: 0, wB: 6, octA: 0, octB: 0, timeModE: true, timeModMode: 2, timeModRate: 1, timeModD: 0.85, timeModM: 0.88, crushE: true, crushMode: 2, crushBits: 10.0, crushM: 0.40, mDb: -1.5 },
  { name: "106. FX & TRANS // Reverse Crystal Pad", waves: "BEL + FMT", fx: "Reverse Buffer + Shimmer", desc: "Haunting tempo-synced reverse buffer grains with lush shimmer reverb.",
    wA: 8, wB: 4, octA: 1, octB: 0, timeModE: true, timeModMode: 3, timeModRate: 2, timeModD: 0.80, timeModM: 0.85, reverbE: true, reverbSize: 0.90, reverbShim: 0.50, reverbM: 0.40, mDb: -2.0 },
  { name: "107. FX & TRANS // Turntable Scratch", waves: "SYNC + SAW", fx: "DJ Scratch + Ring Mod + OTT", desc: "Aggressive turntable buffer scrubbing with ring-modulated overtones.",
    wA: 7, wB: 0, octA: 0, octB: 0, timeModE: true, timeModMode: 5, timeModRate: 2, timeModD: 0.78, timeModM: 0.85, ringE: true, ringFreq: 220.0, ringD: 0.60, ringM: 0.35, compE: true, mDb: -2.0 },
  { name: "108. FX & TRANS // 8-Bar Tension Riser", waves: "SAW + SQR", fx: "Pitch Riser + Jet Flanger + Noise", desc: "Rising pitch-modulated noise and unison supersaw sweep for EDM build-ups.",
    wA: 0, wB: 1, octA: 0, octB: 1, uV: 12, uDet: 0.35, noiseE: true, noiseT: 0, noiseL: 0.30, flangerE: true, flangerM: 0.55, reverbE: true, reverbM: 0.38, mDb: -3.5 },
  { name: "109. FX & TRANS // Cyber Sci-Fi Downlifter", waves: "SYNC + X-MOD", fx: "Pitch Down + Ping-Pong Echo", desc: "Pitch-plummeting resonant sweep with stereo delay and frequency shifter.",
    wA: 7, wB: 9, octA: 1, octB: 0, ringE: true, ringMode: 1, ringFreq: 80.0, ringD: 0.60, ringM: 0.40, delayE: true, delayM: 0.35, reverbE: true, reverbM: 0.30, mDb: -2.5 },
  { name: "110. FX & TRANS // Sub Drop Impact", waves: "SINE + SAW", fx: "Pitch Bomb + Tube Saturation", desc: "Low-end devastating 808 sub pitch bomb with tube saturation and limiter.",
    wA: 6, wB: 0, octA: -1, octB: -2, distE: true, distT: 0, distD: 0.40, envA: 0.01, envD: 0.70, envS: 0.0, mDb: 0.0 },
  { name: "111. FX & TRANS // Alien Transmission Glitch", waves: "X-MOD + RND", fx: "Ring Mod + Noise Crackle", desc: "Ring-modulated frequency-shifted alien radio signals with noise crackle.",
    wA: 9, wB: 5, octA: 0, octB: 1, ringE: true, ringMode: 0, ringFreq: 680.0, ringD: 0.80, ringM: 0.55, noiseE: true, noiseT: 3, noiseL: 0.22, delayE: true, delayM: 0.25, mDb: -2.0 },
  { name: "112. FX & TRANS // White Noise Stadium Sweep", waves: "TRI + SQR", fx: "White Noise + Jet Flanger", desc: "Highpass-swept stereo white noise with jet flanger and shimmer tail.",
    wA: 2, wB: 1, octA: 0, octB: 0, noiseE: true, noiseT: 0, noiseL: 0.50, f1T: 3, f1C: 0.40, f1R: 0.50, flangerE: true, flangerM: 0.50, reverbE: true, reverbM: 0.40, mDb: -3.0 },
  { name: "113. FX & TRANS // Tape Rewind Effect", waves: "SAW + SINE", fx: "Reverse Buffer + Vinyl Grit", desc: "High-speed reverse buffer tape scrubbing with vinyl dust and motor noise.",
    wA: 0, wB: 6, octA: 0, octB: 0, timeModE: true, timeModMode: 3, timeModRate: 3, timeModD: 0.85, timeModM: 0.88, noiseE: true, noiseT: 3, noiseL: 0.20, mDb: -1.5 },
  { name: "114. FX & TRANS // Metallic Sci-Fi Blast", waves: "BEL + X-MOD", fx: "Diode Ring + Long Hall Delay", desc: "Clangorous diode ring-modulated impact with long decay and delay.",
    wA: 8, wB: 9, octA: 1, octB: 0, ringE: true, ringMode: 0, ringFreq: 520.0, ringD: 0.75, ringM: 0.50, delayE: true, delayM: 0.35, reverbE: true, reverbM: 0.38, mDb: -2.5 },
  { name: "115. FX & TRANS // Time Warp Slowdown", waves: "ORG + SAW", fx: "Buffer Decel + Filter Sweep", desc: "Gradual tempo deceleration buffer effect with harmonic lowpass filter.",
    wA: 3, wB: 0, octA: 0, octB: 0, timeModE: true, timeModMode: 2, timeModRate: 0, timeModD: 0.90, timeModM: 0.85, f1T: 1, f1C: 0.60, f1R: 0.30, mDb: -1.5 },
  { name: "116. FX & TRANS // Formant Vowel Morph", waves: "FMT + FMT", fx: "Vowel Filter + Phaser Spread", desc: "Slow automated vocal vowel transition (A-E-I-O-U) across a dense drone.",
    wA: 4, wB: 4, octA: 0, octB: 0, fineB: 12.0, f1T: 6, f1C: 0.65, f1R: 0.45, phaserE: true, phaserM: 0.45, reverbE: true, reverbM: 0.30, mDb: -2.0 },
  { name: "117. FX & TRANS // Jet Flanger Flyby", waves: "SAW + SQR", fx: "Extreme Through-Zero Feedback", desc: "Dramatic jet flanger audio pass with heavy through-zero feedback.",
    wA: 0, wB: 1, octA: 0, octB: 0, uV: 8, uDet: 0.25, flangerE: true, flangerMode: 0, flangerFb: 0.85, flangerM: 0.65, reverbE: true, reverbM: 0.25, mDb: -3.0 },
  { name: "118. FX & TRANS // Granular Buffer Freeze", waves: "BEL + ORG", fx: "Time Mod Loop + Shimmer", desc: "Frozen audio buffer with shimmer reverb creating endless ambient cloud.",
    wA: 8, wB: 3, octA: 0, octB: 1, timeModE: true, timeModMode: 1, timeModRate: 5, timeModD: 0.95, timeModM: 0.90, reverbE: true, reverbSize: 0.95, reverbShim: 0.60, reverbM: 0.45, mDb: -2.5 },
  { name: "119. FX & TRANS // Cyber Space Siren", waves: "SYNC + SAW", fx: "Dual Alarm + Ping-Pong Delay", desc: "Alternating dual-frequency alarm siren with ping-pong echo and distortion.",
    wA: 7, wB: 0, octA: 1, octB: 0, semiB: 7, vibD: 0.45, vibS: 2.5, distE: true, distT: 2, distD: 0.40, delayE: true, delayM: 0.35, mDb: -2.0 },
  { name: "120. FX & TRANS // Bit-Decimated Explosion", waves: "RND + SQR", fx: "4-Bit Decimate + Sub Boom", desc: "Lo-Fi crushed noise impact with crumbling 4-bit sample reduction.",
    wA: 5, wB: 1, octA: -1, octB: -1, crushE: true, crushMode: 0, crushBits: 4.0, crushDown: 6.0, crushM: 0.70, distE: true, distT: 4, distD: 0.50, mDb: -2.5 },
  { name: "121. FX & TRANS // Spectral Prism Riser", waves: "SAW + SYNC", fx: "Inharmonic Sweep + Shimmer", desc: "Upward inharmonic dispersion sweep building intense high-frequency energy.",
    wA: 0, wB: 7, octA: 0, octB: 1, uV: 8, uDet: 0.30, reverbE: true, reverbShim: 0.50, reverbM: 0.35, flangerE: true, flangerM: 0.40, mDb: -2.5 },
  { name: "122. FX & TRANS // DJ Scratch Stutter Combo", waves: "SYNC + SAW", fx: "Scratch + 1/16 Stutter Repeat", desc: "Fast turntable scrub combined with 1/16 buffer stutter repeat.",
    wA: 7, wB: 0, octA: 0, octB: 0, timeModE: true, timeModMode: 5, timeModRate: 5, timeModD: 0.85, timeModM: 0.88, compE: true, compM: 0.65, mDb: -2.0 },
  { name: "123. FX & TRANS // Cinematic Cosmic Swarm", waves: "BEL + X-MOD", fx: "16x Unison + Shimmer Tail", desc: "Multi-voice detuned spectral particle swarm with massive hall reverb.",
    wA: 8, wB: 9, octA: 0, octB: 1, uV: 16, uDet: 0.35, uSpr: 1.0, reverbE: true, reverbSize: 0.95, reverbShim: 0.55, reverbM: 0.45, mDb: -3.5 }
,
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
];

fxDefs.forEach((d, i) => {
  const numStr = (presets.length < 10 ? "0" : "") + presets.length;
  const cleanName = d.name.replace(/^\d+\.\s*/, "");
  const p = basePreset(presets.length, numStr + ". " + cleanName, "FX & TRANS", "Prismor Sound", d.waves, d.fx, d.desc);
  p.waveA = d.wA; p.waveB = d.wB; p.octA = d.octA; p.octB = d.octB;
  if (d.semiB !== undefined) p.semiB = d.semiB;
  if (d.fineB !== undefined) p.fineB = d.fineB;
  if (d.uV !== undefined) { p.unisonVoices = d.uV; p.unisonDetune = d.uDet || 0.25; p.unisonSpread = d.uSpr || 0.80; }
  if (d.noiseE) { p.noiseEnable = true; if (d.noiseT !== undefined) p.noiseType = d.noiseT; if (d.noiseL !== undefined) p.noiseLevel = d.noiseL; }
  if (d.f1T !== undefined) p.filter1Type = d.f1T;
  if (d.f1C !== undefined) p.f1Cutoff = d.f1C;
  if (d.f1R !== undefined) p.f1Res = d.f1R;
  if (d.distE) { p.distEnable = true; if (d.distT !== undefined) p.distType = d.distT; if (d.distD !== undefined) p.distDrive = d.distD; }
  if (d.flangerE) { p.flangerEnable = true; if (d.flangerMode !== undefined) p.flangerMode = d.flangerMode; if (d.flangerFb !== undefined) p.flangerFeedback = d.flangerFb; if (d.flangerM !== undefined) p.flangerMix = d.flangerM; }
  if (d.ringE) { p.ringEnable = true; if (d.ringMode !== undefined) p.ringMode = d.ringMode; if (d.ringFreq !== undefined) p.ringFreq = d.ringFreq; if (d.ringD !== undefined) p.ringDepth = d.ringD; if (d.ringM !== undefined) p.ringMix = d.ringM; }
  if (d.crushE) { p.crushEnable = true; if (d.crushMode !== undefined) p.crushMode = d.crushMode; if (d.crushBits !== undefined) p.crushBits = d.crushBits; if (d.crushDown !== undefined) p.crushDownsample = d.crushDown; if (d.crushM !== undefined) p.crushMix = d.crushM; }
  if (d.timeModE) { p.timeModEnable = true; if (d.timeModMode !== undefined) p.timeModMode = d.timeModMode; if (d.timeModRate !== undefined) p.timeModRate = d.timeModRate; if (d.timeModD !== undefined) p.timeModDepth = d.timeModD; if (d.timeModM !== undefined) p.timeModMix = d.timeModM; }
  if (d.compE) { p.compEnable = true; if (d.compM !== undefined) p.compMix = d.compM; }
  if (d.vibD !== undefined) { p.vibDepth = d.vibD; p.vibSpeed = d.vibS || 5.0; }
  if (d.envA !== undefined) p.envAttack = d.envA;
  if (d.envD !== undefined) p.envDecay = d.envD;
  if (d.envS !== undefined) p.envSustain = d.envS;
  if (d.delayE) { p.delayEnable = true; if (d.delayM !== undefined) p.delayMix = d.delayM; }
  if (d.reverbE) { p.reverbEnable = true; if (d.reverbSize !== undefined) p.reverbSize = d.reverbSize; if (d.reverbShim !== undefined) p.reverbShimmer = d.reverbShim; if (d.reverbM !== undefined) p.reverbMix = d.reverbM; }
  if (d.mDb !== undefined) p.masterGainDb = d.mDb;
  addPreset(p);
});

// -----------------------------------------------------------------------------
// 2.8 Procedural Archetype Expansion Packs (Phase 5.5: 160 Production Patches)
// -----------------------------------------------------------------------------
const { generateProceduralExpansionPacks, exportPrismorPresetFiles } = require('./procedural_archetypes.js');
const proceduralPatches = generateProceduralExpansionPacks(presets.length);
proceduralPatches.forEach(p => presets.push(p));

const expansionDir = path.join(__dirname, 'Presets', 'ExpansionPacks');
const exportedCount = exportPrismorPresetFiles(proceduralPatches, expansionDir);
console.log(`Exported ${exportedCount} standalone .prismor files into ${expansionDir}`);

// -----------------------------------------------------------------------------
// 3. Automated Validation & Safety Checker
// -----------------------------------------------------------------------------
console.log(`\n=======================================================`);
console.log(`PRISMOR PRESET BANK GENERATOR & VALIDATOR`);
console.log(`=======================================================`);
console.log(`Total Presets to generate: ${presets.length}`);

let validationErrors = 0;
let validationWarnings = 0;

// Category counter
const catCounts = {};

presets.forEach((p, idx) => {
  if (p.index !== idx) {
    console.error(`[ERROR] Preset index mismatch: declared ${p.index}, actual ${idx}`);
    validationErrors++;
  }

  catCounts[p.category] = (catCounts[p.category] || 0) + 1;

  // Validate Octaves
  if (p.octA < PARAM_LIMITS.octave.min || p.octA > PARAM_LIMITS.octave.max) {
    console.error(`[ERROR] ${p.name}: octA ${p.octA} out of range`);
    validationErrors++;
  }
  if (p.octB < PARAM_LIMITS.octave.min || p.octB > PARAM_LIMITS.octave.max) {
    console.error(`[ERROR] ${p.name}: octB ${p.octB} out of range`);
    validationErrors++;
  }

  // Validate Semitones
  if (p.semiA < PARAM_LIMITS.semi.min || p.semiA > PARAM_LIMITS.semi.max) {
    console.error(`[ERROR] ${p.name}: semiA out of range`);
    validationErrors++;
  }
  if (p.semiB < PARAM_LIMITS.semi.min || p.semiB > PARAM_LIMITS.semi.max) {
    console.error(`[ERROR] ${p.name}: semiB out of range`);
    validationErrors++;
  }

  // Validate Gain Staging
  if (p.masterGainDb < PARAM_LIMITS.masterGainDb.min || p.masterGainDb > PARAM_LIMITS.masterGainDb.max) {
    console.error(`[ERROR] ${p.name}: masterGainDb out of range`);
    validationErrors++;
  }
  if (p.limiterThreshDb > 0.0) {
    console.error(`[ERROR] ${p.name}: limiterThreshDb > 0.0`);
    validationErrors++;
  }

  // Unison Gain Compensation Check
  if (p.unisonVoices >= 12 && p.masterGainDb > -2.0) {
    console.warn(`[WARN] ${p.name}: High unison (${p.unisonVoices}x) with high masterGain (${p.masterGainDb}dB)`);
    validationWarnings++;
  }
});

console.log(`\nCategory Breakdown:`);
for (const [cat, cnt] of Object.entries(catCounts)) {
  console.log(`  - ${cat.padEnd(16)}: ${cnt} patches`);
  if (cat !== 'INIT' && cnt < 15) {
    console.error(`[ERROR] Category ${cat} has less than 15 patches (${cnt})!`);
    validationErrors++;
  }
}

if (validationErrors > 0) {
  console.error(`\nValidation FAILED with ${validationErrors} errors! Aborting generation.`);
  process.exit(1);
}

console.log(`\nValidation PASSED: 0 Errors, ${validationWarnings} Warnings.`);

// -----------------------------------------------------------------------------
// 4. Generate C++ Header: Source/DSP/FactoryPresetBank.h
// -----------------------------------------------------------------------------
const outPath = path.join(__dirname, 'Source', 'DSP', 'FactoryPresetBank.h');

let cpp = `// =============================================================================
// PRISMOR 1.0.12.4 FACTORY PRESET BANK (${presets.length} PRODUCTION-READY PATCHES)
// Programmatically generated by generate_presets.js & procedural_archetypes.js
// ZERO SYNTHESIS PLACEHOLDERS - FULL PRODUCTION SOUND DESIGN
// =============================================================================

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SpectralSynthEngine.h"
#include <vector>
#include <functional>

namespace PrismorDSP
{

struct FactoryPresetEntry
{
    int index;
    const char* name;
    const char* category;
    const char* author;
    const char* wavesUsed;
    const char* fxUsed;
    const char* description;

    // Field 1 & 2 Oscillators
    int waveA, waveB;
    int octA, semiA; float fineA;
    int octB, semiB; float fineB;
    int warpModeA; float warpAmtA;
    int warpModeB; float warpAmtB;
    float timbreA, timbreB;
    float harmScaleA, harmTiltA;
    float harmScaleB, harmTiltB;

    // Voicing & Unison
    int voicingMode, maxVoices;
    float portaTime, portaCurve; bool autoGlide;
    int unisonVoices; float unisonDetune, unisonSpread, unisonBlend; int unisonStack;
    float vibDepth, vibSpeed;

    // Filters
    bool filter1Enable; int filter1Type; float f1Cutoff, f1Res, f1KeyTrack, f1Width;
    bool filter2Enable; int filter2Type; float f2Cutoff, f2Res, f2KeyTrack, f2Width;
    float filterDrive, filterRouting;

    // Noise
    bool noiseEnable; int noiseType; float noiseLevel, noisePitch, noiseFilter;

    // Arpeggiator
    bool arpEnable; int arpMode, arpOctaves, arpRate; float arpGate, arpSwing;

    // Multi-FX
    bool distEnable; int distType; float distDrive, distTone, distMix;
    bool chorusEnable; int chorusVoices; float chorusRate, chorusDepth, chorusMix;
    bool phaserEnable; float phaserMix, phaserSpeed;
    bool delayEnable; float delayTime, delayFeedback, delayPingPong, delayMix;
    bool reverbEnable; float reverbSize, reverbDecay, reverbShimmer, reverbMix;
    bool compEnable; int compMode; float compThreshold, compRatio, compMakeup, compMix;
    bool ringEnable; int ringMode; float ringFreq, ringDepth, ringMix;
    bool crushEnable; int crushMode; float crushBits, crushDownsample, crushMix;
    bool flangerEnable; int flangerMode; float flangerRate, flangerDepth, flangerFeedback, flangerMix;

    // Time Modulation
    bool timeModEnable; int timeModMode, timeModRate; float timeModDepth, timeModMix;

    // Envelopes & Shaper
    float envAttack, envHold, envDecay, envSustain, envRelease;
    int shaperPresetTarget; // -1 for none, or 0..7
    int timeModPresetIdx;   // 0..5

    // Mix & Output
    float abMix, velLink;
    float masterGainDb, limiterThreshDb;
};

class FactoryPresetBank
{
public:
    static const std::vector<FactoryPresetEntry>& getEntries()
    {
        static const std::vector<FactoryPresetEntry> entries = {
`;

presets.forEach((p, idx) => {
  cpp += `            // [${idx}] ${p.name}\n`;
  cpp += `            {\n`;
  cpp += `                ${p.index}, "${p.name}", "${p.category}", "${p.author}",\n`;
  cpp += `                "${p.wavesUsed}", "${p.fxUsed}",\n`;
  cpp += `                "${p.description.replace(/"/g, '\\"')}",\n`;
  cpp += `                ${p.waveA}, ${p.waveB},\n`;
  cpp += `                ${p.octA}, ${p.semiA}, ${p.fineA.toFixed(1)}f,\n`;
  cpp += `                ${p.octB}, ${p.semiB}, ${p.fineB.toFixed(1)}f,\n`;
  cpp += `                ${p.warpModeA}, ${p.warpAmtA.toFixed(2)}f,\n`;
  cpp += `                ${p.warpModeB}, ${p.warpAmtB.toFixed(2)}f,\n`;
  cpp += `                ${p.timbreA.toFixed(2)}f, ${p.timbreB.toFixed(2)}f,\n`;
  cpp += `                ${p.harmScaleA.toFixed(2)}f, ${p.harmTiltA.toFixed(2)}f,\n`;
  cpp += `                ${p.harmScaleB.toFixed(2)}f, ${p.harmTiltB.toFixed(2)}f,\n`;
  cpp += `                ${p.voicingMode}, ${p.maxVoices},\n`;
  cpp += `                ${p.portaTime.toFixed(2)}f, ${p.portaCurve.toFixed(2)}f, ${p.autoGlide ? 'true' : 'false'},\n`;
  cpp += `                ${p.unisonVoices}, ${p.unisonDetune.toFixed(2)}f, ${p.unisonSpread.toFixed(2)}f, ${p.unisonBlend.toFixed(2)}f, ${p.unisonStack},\n`;
  cpp += `                ${p.vibDepth.toFixed(2)}f, ${p.vibSpeed.toFixed(2)}f,\n`;
  cpp += `                ${p.filter1Enable ? 'true' : 'false'}, ${p.filter1Type}, ${p.f1Cutoff.toFixed(2)}f, ${p.f1Res.toFixed(2)}f, ${p.f1KeyTrack.toFixed(2)}f, ${p.f1Width.toFixed(2)}f,\n`;
  cpp += `                ${p.filter2Enable ? 'true' : 'false'}, ${p.filter2Type}, ${p.f2Cutoff.toFixed(2)}f, ${p.f2Res.toFixed(2)}f, ${p.f2KeyTrack.toFixed(2)}f, ${p.f2Width.toFixed(2)}f,\n`;
  cpp += `                ${p.filterDrive.toFixed(2)}f, ${p.filterRouting.toFixed(2)}f,\n`;
  cpp += `                ${p.noiseEnable ? 'true' : 'false'}, ${p.noiseType}, ${p.noiseLevel.toFixed(2)}f, ${p.noisePitch.toFixed(2)}f, ${p.noiseFilter.toFixed(2)}f,\n`;
  cpp += `                ${p.arpEnable ? 'true' : 'false'}, ${p.arpMode}, ${p.arpOctaves}, ${p.arpRate}, ${p.arpGate.toFixed(2)}f, ${p.arpSwing.toFixed(2)}f,\n`;
  cpp += `                ${p.distEnable ? 'true' : 'false'}, ${p.distType}, ${p.distDrive.toFixed(2)}f, ${p.distTone.toFixed(2)}f, ${p.distMix.toFixed(2)}f,\n`;
  cpp += `                ${p.chorusEnable ? 'true' : 'false'}, ${p.chorusVoices}, ${p.chorusRate.toFixed(2)}f, ${p.chorusDepth.toFixed(2)}f, ${p.chorusMix.toFixed(2)}f,\n`;
  cpp += `                ${p.phaserEnable ? 'true' : 'false'}, ${p.phaserMix.toFixed(2)}f, ${p.phaserSpeed.toFixed(2)}f,\n`;
  cpp += `                ${p.delayEnable ? 'true' : 'false'}, ${p.delayTime.toFixed(2)}f, ${p.delayFeedback.toFixed(2)}f, ${p.delayPingPong.toFixed(2)}f, ${p.delayMix.toFixed(2)}f,\n`;
  cpp += `                ${p.reverbEnable ? 'true' : 'false'}, ${p.reverbSize.toFixed(2)}f, ${p.reverbDecay.toFixed(2)}f, ${p.reverbShimmer.toFixed(2)}f, ${p.reverbMix.toFixed(2)}f,\n`;
  cpp += `                ${p.compEnable ? 'true' : 'false'}, ${p.compMode}, ${p.compThreshold.toFixed(1)}f, ${p.compRatio.toFixed(1)}f, ${p.compMakeup.toFixed(1)}f, ${p.compMix.toFixed(2)}f,\n`;
  cpp += `                ${p.ringEnable ? 'true' : 'false'}, ${p.ringMode}, ${p.ringFreq.toFixed(1)}f, ${p.ringDepth.toFixed(2)}f, ${p.ringMix.toFixed(2)}f,\n`;
  cpp += `                ${p.crushEnable ? 'true' : 'false'}, ${p.crushMode}, ${p.crushBits.toFixed(1)}f, ${p.crushDownsample.toFixed(1)}f, ${p.crushMix.toFixed(2)}f,\n`;
  cpp += `                ${p.flangerEnable ? 'true' : 'false'}, ${p.flangerMode}, ${p.flangerRate.toFixed(2)}f, ${p.flangerDepth.toFixed(2)}f, ${p.flangerFeedback.toFixed(2)}f, ${p.flangerMix.toFixed(2)}f,\n`;
  cpp += `                ${p.timeModEnable ? 'true' : 'false'}, ${p.timeModMode}, ${p.timeModRate}, ${p.timeModDepth.toFixed(2)}f, ${p.timeModMix.toFixed(2)}f,\n`;
  cpp += `                ${p.envAttack.toFixed(2)}f, ${p.envHold.toFixed(2)}f, ${p.envDecay.toFixed(2)}f, ${p.envSustain.toFixed(2)}f, ${p.envRelease.toFixed(2)}f,\n`;
  cpp += `                ${p.shaperPresetTarget}, ${p.timeModPresetIdx},\n`;
  cpp += `                ${p.abMix.toFixed(2)}f, ${p.velLink.toFixed(2)}f,\n`;
  cpp += `                ${p.masterGainDb.toFixed(1)}f, ${p.limiterThreshDb.toFixed(1)}f\n`;
  cpp += `            }${idx < presets.length - 1 ? ',' : ''}\n`;
});

cpp += `        };
        return entries;
    }

    static int getPresetCount() noexcept
    {
        return static_cast<int>(getEntries().size());
    }

    static void applyPreset(int index,
                            std::function<void(const char*, float)> setParam,
                            SpectralSynthEngine& synthEngine)
    {
        const auto& entries = getEntries();
        if (entries.empty()) return;
        const auto& p = entries[static_cast<size_t>(juce::jlimit(0, static_cast<int>(entries.size()) - 1, index))];

        // 1. Frequency Field 1 & 2 Waveform Tables
        synthEngine.getResynthesisEngine().resetPartTableToWaveform(0, p.waveA);
        synthEngine.getResynthesisEngine().resetPartTableToWaveform(1, p.waveB);

        // 2. Clear / Reset Engine Modulators
        synthEngine.resetFxChainOrder();
        synthEngine.resetArpStepsToDefault(0);
        synthEngine.getTimeModCurve().applyTimeModPreset(p.timeModPresetIdx);

        for (int i = 0; i < static_cast<int>(ArticulatorTarget::Count); ++i)
            synthEngine.getArticulator(static_cast<ArticulatorTarget>(i)).resetToDefault(static_cast<ArticulatorTarget>(i));

        if (p.shaperPresetTarget >= 0)
            synthEngine.getArticulator(ArticulatorTarget::Volume).applyPresetShape(p.shaperPresetTarget);

        // 3. Inject Full Parameter State into APVTS
        setParam("oscAOct",         static_cast<float>(p.octA));
        setParam("oscASemi",        static_cast<float>(p.semiA));
        setParam("oscAFine",        p.fineA);
        setParam("oscACoarse",      0.0f);
        setParam("startPhase",      0.0f);
        setParam("phaseRandom",     0.0f);
        setParam("timbre",          p.timbreA);
        setParam("warpMode",        static_cast<float>(p.warpModeA));
        setParam("warpAmount",      p.warpAmtA);
        setParam("harmonicScale",   p.harmScaleA);
        setParam("harmonicTilt",    p.harmTiltA);

        setParam("oscBOct",         static_cast<float>(p.octB));
        setParam("oscBSemi",        static_cast<float>(p.semiB));
        setParam("oscBFine",        p.fineB);
        setParam("oscBPitch",       0.0f);
        setParam("oscBPhase",       0.0f);
        setParam("timbreB",         p.timbreB);
        setParam("warpModeB",       static_cast<float>(p.warpModeB));
        setParam("warpAmountB",     p.warpAmtB);
        setParam("harmonicScaleB",  p.harmScaleB);
        setParam("harmonicTiltB",   p.harmTiltB);

        setParam("prismEnable",     1.0f);
        setParam("prismAmount",     0.0f);
        setParam("prismMode",       0.0f);
        setParam("harmShift",       0.0f);
        setParam("harmGap",         0.25f);
        setParam("harmWidth",       0.45f);

        setParam("pluckEnable",     1.0f);
        setParam("pluckTime",       1.0f);
        setParam("pluckBlur",       0.30f);

        setParam("voicingMode",     static_cast<float>(p.voicingMode));
        setParam("maxVoices",       static_cast<float>(p.maxVoices));
        setParam("pitchFreq",       0.0f);
        setParam("portaTime",       p.portaTime);
        setParam("portaCurve",      p.portaCurve);
        setParam("autoGlide",       p.autoGlide ? 1.0f : 0.0f);

        setParam("unisonVoices",    static_cast<float>(p.unisonVoices));
        setParam("unisonDetune",    p.unisonDetune);
        setParam("unisonSpread",    p.unisonSpread);
        setParam("unisonBlend",     p.unisonBlend);
        setParam("unisonPhase",     0.35f);
        setParam("unisonStackMode", static_cast<float>(p.unisonStack));

        setParam("vibDepth",        p.vibDepth);
        setParam("vibSpeed",        p.vibSpeed);

        setParam("filter1Enable",   p.filter1Enable ? 1.0f : 0.0f);
        setParam("filter1Type",     static_cast<float>(p.filter1Type));
        setParam("filter1Cutoff",   p.f1Cutoff);
        setParam("filter1Res",      p.f1Res);
        setParam("filter1KeyTrack", p.f1KeyTrack);
        setParam("filter1Width",    p.f1Width);

        setParam("filter2Enable",   p.filter2Enable ? 1.0f : 0.0f);
        setParam("filter2Type",     static_cast<float>(p.filter2Type));
        setParam("filter2Cutoff",   p.f2Cutoff);
        setParam("filter2Res",      p.f2Res);
        setParam("filter2KeyTrack", p.f2KeyTrack);
        setParam("filter2Width",    p.f2Width);

        setParam("filterRouting",   p.filterRouting);
        setParam("filterDrive",     p.filterDrive);

        setParam("noiseEnable",     p.noiseEnable ? 1.0f : 0.0f);
        setParam("noiseType",       static_cast<float>(p.noiseType));
        setParam("noiseLevel",      p.noiseLevel);
        setParam("noisePitch",      p.noisePitch);
        setParam("noiseKeyTrack",   0.50f);
        setParam("noisePhase",      0.25f);
        setParam("noisePan",        0.0f);
        setParam("noiseFilter",     p.noiseFilter);

        setParam("arpEnable",       p.arpEnable ? 1.0f : 0.0f);
        setParam("arpMode",         static_cast<float>(p.arpMode));
        setParam("arpOctaves",      static_cast<float>(p.arpOctaves));
        setParam("arpRate",         static_cast<float>(p.arpRate));
        setParam("arpGate",         p.arpGate);
        setParam("arpSwing",        p.arpSwing);

        setParam("blurEnable",      1.0f);
        setParam("blurTime",        0.0f);
        setParam("blurHarmonic",    0.0f);

        setParam("tremEnable",      1.0f);
        setParam("tremDepth",       0.0f);
        setParam("tremSpeed",       3.5f);
        setParam("tremGap",         0.35f);

        setParam("distEnable",      p.distEnable ? 1.0f : 0.0f);
        setParam("distType",        static_cast<float>(p.distType));
        setParam("distDrive",       p.distDrive);
        setParam("distTone",        p.distTone);
        setParam("distMix",         p.distMix);

        setParam("phaserEnable",    1.0f);
        setParam("phaserMix",       p.phaserEnable ? p.phaserMix : 0.0f);
        setParam("phaserWidth",     0.50f);
        setParam("phaserOffset",    0.35f);
        setParam("phaserSpeed",     p.phaserSpeed);

        setParam("chorusEnable",    p.chorusEnable ? 1.0f : 0.0f);
        setParam("chorusVoices",    static_cast<float>(p.chorusVoices));
        setParam("chorusRate",      p.chorusRate);
        setParam("chorusDepth",     p.chorusDepth);
        setParam("chorusSpread",    0.80f);
        setParam("chorusMix",       p.chorusMix);

        setParam("eqEnable",        1.0f);
        setParam("eqLowFreq",       110.0f);
        setParam("eqLowGain",       0.0f);
        setParam("eqMidFreq",       1200.0f);
        setParam("eqMidGain",       0.0f);
        setParam("eqMidQ",          1.2f);
        setParam("eqHighFreq",      5500.0f);
        setParam("eqHighGain",      0.0f);

        setParam("delayEnable",     p.delayEnable ? 1.0f : 0.0f);
        setParam("delayTime",       p.delayTime);
        setParam("delayFeedback",   p.delayFeedback);
        setParam("delayPingPong",   p.delayPingPong);
        setParam("delayFilter",     0.60f);
        setParam("delayMix",        p.delayMix);

        setParam("reverbEnable",    p.reverbEnable ? 1.0f : 0.0f);
        setParam("reverbSize",      p.reverbSize);
        setParam("reverbDecay",     p.reverbDecay);
        setParam("reverbPreDelay",  0.18f);
        setParam("reverbDamping",   0.40f);
        setParam("reverbShimmer",   p.reverbShimmer);
        setParam("reverbMix",       p.reverbMix);

        setParam("compEnable",      p.compEnable ? 1.0f : 0.0f);
        setParam("compMode",        static_cast<float>(p.compMode));
        setParam("compThreshold",   p.compThreshold);
        setParam("compRatio",       p.compRatio);
        setParam("compAttack",      10.0f);
        setParam("compRelease",     120.0f);
        setParam("compMakeup",      p.compMakeup);
        setParam("compMix",         p.compMix);

        setParam("ringEnable",      p.ringEnable ? 1.0f : 0.0f);
        setParam("ringMode",        static_cast<float>(p.ringMode));
        setParam("ringFreq",        p.ringFreq);
        setParam("ringDepth",       p.ringDepth);
        setParam("ringLfoRate",     1.20f);
        setParam("ringLfoDepth",    0.25f);
        setParam("ringMix",         p.ringMix);

        setParam("crushEnable",     p.crushEnable ? 1.0f : 0.0f);
        setParam("crushMode",       static_cast<float>(p.crushMode));
        setParam("crushBits",       p.crushBits);
        setParam("crushDownsample", p.crushDownsample);
        setParam("crushDrive",      0.25f);
        setParam("crushTone",       0.65f);
        setParam("crushMix",        p.crushMix);

        setParam("flangerEnable",   p.flangerEnable ? 1.0f : 0.0f);
        setParam("flangerMode",     static_cast<float>(p.flangerMode));
        setParam("flangerRate",     p.flangerRate);
        setParam("flangerDepth",    p.flangerDepth);
        setParam("flangerFeedback", p.flangerFeedback);
        setParam("flangerManual",   0.35f);
        setParam("flangerMix",      p.flangerMix);

        setParam("timeModEnable",   p.timeModEnable ? 1.0f : 0.0f);
        setParam("timeModMode",     static_cast<float>(p.timeModMode));
        setParam("timeModRate",     static_cast<float>(p.timeModRate));
        setParam("timeModDepth",    p.timeModDepth);
        setParam("timeModStutter",  0.35f);
        setParam("timeModSpeed",    0.0f);
        setParam("timeModSmooth",   0.35f);
        setParam("timeModFilter",   0.0f);
        setParam("timeModMix",      p.timeModMix);

        setParam("envAttack",       p.envAttack);
        setParam("envHold",         p.envHold);
        setParam("envDecay",        p.envDecay);
        setParam("envSustain",      p.envSustain);
        setParam("envRelease",      p.envRelease);

        setParam("abMix",           p.abMix);
        setParam("velLink",         p.velLink);
        setParam("partPan",         0.0f);
        setParam("strumTime",       0.0f);
        setParam("strumTension",    0.0f);

        setParam("masterGainDb",    p.masterGainDb);
        setParam("limiterEnable",   1.0f);
        setParam("limiterThreshDb", p.limiterThreshDb);
    }
};

} // namespace PrismorDSP
`;

fs.writeFileSync(outPath, cpp, 'utf8');
console.log(`\nSuccessfully wrote ${presets.length} presets to: ${outPath}`);
