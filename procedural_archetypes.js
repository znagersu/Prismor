/**
 * Prismor Procedural Preset Generator Script (Phase 5.5)
 * Programmatically generates 160+ production-ready presets across 4 modern genre expansion packs:
 * - CYBERPUNK & SYNTHWAVE (40 patches)
 * - CINEMATIC & AMBIENT (40 patches)
 * - MODERN TECH HOUSE (40 patches)
 * - LO-FI & CHILLHOP (40 patches)
 *
 * Injects all presets directly into Source/DSP/FactoryPresetBank.h (total 332+ patches)
 * and exports standalone .prismor XML preset files into Presets/ExpansionPacks/.
 */

const fs = require('fs');
const path = require('path');

// -----------------------------------------------------------------------------
// 1. Strict Parameter Limits (AudioProcessorValueTreeState bounds)
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

function clamp(v, min, max) {
  return Math.min(Math.max(v, min), max);
}

function basePreset(index, name, category, author, wavesUsed, fxUsed, description) {
  return {
    index, name, category, author, wavesUsed, fxUsed, description,
    waveA: 0, waveB: 1,
    octA: 0, semiA: 0, fineA: 0.0,
    octB: 0, semiB: 0, fineB: 0.0,
    warpModeA: 0, warpAmtA: 0.0,
    warpModeB: 0, warpAmtB: 0.0,
    timbreA: 0.0, timbreB: 0.0,
    harmScaleA: 1.0, harmTiltA: 0.0,
    harmScaleB: 1.0, harmTiltB: 0.0,
    voicingMode: 0, maxVoices: 8,
    portaTime: 0.0, portaCurve: 0.0, autoGlide: true,
    unisonVoices: 1, unisonDetune: 0.18, unisonSpread: 0.65, unisonBlend: 0.55, unisonStack: 0,
    vibDepth: 0.0, vibSpeed: 5.5,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 1.0, f1Res: 0.10, f1KeyTrack: 0.50, f1Width: 0.45,
    filter2Enable: false, filter2Type: 2, f2Cutoff: 0.85, f2Res: 0.20, f2KeyTrack: 0.50, f2Width: 0.50,
    filterDrive: 0.0, filterRouting: 0.0,
    noiseEnable: false, noiseType: 0, noiseLevel: 0.20, noisePitch: 0.50, noiseFilter: 0.0,
    arpEnable: false, arpMode: 0, arpOctaves: 1, arpRate: 3, arpGate: 0.75, arpSwing: 0.0,
    distEnable: false, distType: 0, distDrive: 0.35, distTone: 0.65, distMix: 0.45,
    chorusEnable: false, chorusVoices: 4, chorusRate: 0.85, chorusDepth: 0.45, chorusMix: 0.35,
    phaserEnable: false, phaserMix: 0.30, phaserSpeed: 1.2,
    delayEnable: false, delayTime: 0.35, delayFeedback: 0.40, delayPingPong: 0.65, delayMix: 0.28,
    reverbEnable: false, reverbSize: 0.68, reverbDecay: 0.55, reverbShimmer: 0.25, reverbMix: 0.25,
    compEnable: false, compMode: 1, compThreshold: -18.0, compRatio: 4.0, compMakeup: 3.0, compMix: 0.75,
    ringEnable: false, ringMode: 0, ringFreq: 220.0, ringDepth: 0.70, ringMix: 0.50,
    crushEnable: false, crushMode: 0, crushBits: 8.0, crushDownsample: 4.0, crushMix: 0.55,
    flangerEnable: false, flangerMode: 0, flangerRate: 0.45, flangerDepth: 0.65, flangerFeedback: 0.55, flangerMix: 0.50,
    timeModEnable: false, timeModMode: 0, timeModRate: 1, timeModDepth: 0.75, timeModMix: 0.85,
    envAttack: 0.05, envHold: 0.05, envDecay: 0.35, envSustain: 0.72, envRelease: 0.25,
    shaperPresetTarget: -1, timeModPresetIdx: 0,
    abMix: 0.0, velLink: 0.65,
    masterGainDb: 0.0, limiterThreshDb: -0.5
  };
}

// -----------------------------------------------------------------------------
// 2. Musical Variation Engine & Automatic Gain Staging Quality Control
// -----------------------------------------------------------------------------
function applyMusicalGainStaging(p) {
  let gain = 0.0;

  // Unison headroom compensation
  if (p.unisonVoices >= 16)      gain -= 4.0;
  else if (p.unisonVoices >= 12) gain -= 3.5;
  else if (p.unisonVoices >= 8)  gain -= 2.6;
  else if (p.unisonVoices >= 4)  gain -= 1.6;
  else if (p.unisonVoices > 1)   gain -= 0.8;

  // Distortion compensation
  if (p.distEnable) {
    gain -= p.distDrive * 2.2;
  }

  // Resonance peak compensation
  if (p.f1Res > 0.5) {
    gain -= (p.f1Res - 0.5) * 2.5;
  }

  // Shimmer Reverb build compensation
  if (p.reverbEnable && p.reverbShimmer > 0.3) {
    gain -= (p.reverbShimmer - 0.3) * 1.5;
  }

  // Compressor makeup adjustment
  if (p.compEnable && p.compMakeup > 2.0) {
    gain -= (p.compMakeup - 2.0) * 0.4;
  }

  // Pure sub sine boost
  if (p.category === "BASS" && p.waveA === 6 && p.unisonVoices === 1 && !p.distEnable) {
    gain = 0.5;
  }

  p.masterGainDb = clamp(Math.round(gain * 2.0) / 2.0, -12.0, 1.0);
  p.limiterThreshDb = -0.5;
}

function validateAndClamp(p) {
  p.octA = clamp(p.octA, PARAM_LIMITS.octave.min, PARAM_LIMITS.octave.max);
  p.octB = clamp(p.octB, PARAM_LIMITS.octave.min, PARAM_LIMITS.octave.max);
  p.semiA = clamp(p.semiA, PARAM_LIMITS.semi.min, PARAM_LIMITS.semi.max);
  p.semiB = clamp(p.semiB, PARAM_LIMITS.semi.min, PARAM_LIMITS.semi.max);
  p.fineA = clamp(p.fineA, PARAM_LIMITS.fine.min, PARAM_LIMITS.fine.max);
  p.fineB = clamp(p.fineB, PARAM_LIMITS.fine.min, PARAM_LIMITS.fine.max);

  p.warpAmtA = clamp(p.warpAmtA, PARAM_LIMITS.bipolar.min, PARAM_LIMITS.bipolar.max);
  p.warpAmtB = clamp(p.warpAmtB, PARAM_LIMITS.bipolar.min, PARAM_LIMITS.bipolar.max);
  p.timbreA = clamp(p.timbreA, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.timbreB = clamp(p.timbreB, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.harmScaleA = clamp(p.harmScaleA, PARAM_LIMITS.harmScale.min, PARAM_LIMITS.harmScale.max);
  p.harmScaleB = clamp(p.harmScaleB, PARAM_LIMITS.harmScale.min, PARAM_LIMITS.harmScale.max);
  p.harmTiltA = clamp(p.harmTiltA, PARAM_LIMITS.bipolar.min, PARAM_LIMITS.bipolar.max);
  p.harmTiltB = clamp(p.harmTiltB, PARAM_LIMITS.bipolar.min, PARAM_LIMITS.bipolar.max);

  p.unisonVoices = clamp(p.unisonVoices, PARAM_LIMITS.unisonVoices.min, PARAM_LIMITS.unisonVoices.max);
  p.unisonDetune = clamp(p.unisonDetune, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.unisonSpread = clamp(p.unisonSpread, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.unisonBlend = clamp(p.unisonBlend, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);

  p.f1Cutoff = clamp(p.f1Cutoff, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.f1Res = clamp(p.f1Res, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.f2Cutoff = clamp(p.f2Cutoff, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.f2Res = clamp(p.f2Res, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);

  p.envAttack = clamp(p.envAttack, 0.005, 5.0);
  p.envHold = clamp(p.envHold, 0.0, 2.0);
  p.envDecay = clamp(p.envDecay, 0.01, 5.0);
  p.envSustain = clamp(p.envSustain, 0.0, 1.0);
  p.envRelease = clamp(p.envRelease, 0.01, 5.0);

  p.abMix = clamp(p.abMix, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);
  p.velLink = clamp(p.velLink, PARAM_LIMITS.norm.min, PARAM_LIMITS.norm.max);

  applyMusicalGainStaging(p);
}

// -----------------------------------------------------------------------------
// 3. Archetype Templates and Variations
// -----------------------------------------------------------------------------

function generateProceduralExpansionPacks(startIndex) {
  const newPresets = [];
  let curIndex = startIndex;

  function make(tpl, name, cat, author, waves, fx, desc, modFn) {
    const p = basePreset(curIndex++, name, cat, author, waves, fx, desc);
    Object.assign(p, tpl);
    if (modFn) modFn(p);
    validateAndClamp(p);
    newPresets.push(p);
    return p;
  }

  // ===========================================================================
  // PACK 1: CYBERPUNK & SYNTHWAVE (40 Patches)
  // ===========================================================================
  const cyberAuthor = "Prismor Cyberpunk";
  const synthwaveAuthor = "Prismor Synthwave";

  // Archetype 1: Distorted Analog Reese (BASS) - 10 Variations
  const reeseTpl = {
    waveA: 0, waveB: 1, octA: -1, octB: -1, voicingMode: 1,
    unisonVoices: 8, unisonDetune: 0.28, unisonSpread: 0.85, unisonBlend: 0.65,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.42, f1Res: 0.22,
    distEnable: true, distType: 0, distDrive: 0.38, distMix: 0.50,
    compEnable: true, compMode: 1, compThreshold: -16.0, compRatio: 4.0, compMakeup: 3.0, compMix: 0.70,
    envAttack: 0.02, envDecay: 0.45, envSustain: 0.75, envRelease: 0.30
  };

  make(reeseTpl, "CYBER // Neon Reese Monolith", "BASS", cyberAuthor, "SAW + SQR", "8x Unison + SVF 24dB + Tube",
    "[CYBERPUNK & SYNTHWAVE] Wall of sound 8-voice detuned neuro Reese with warm tube drive.",
    p => { p.unisonVoices = 8; p.unisonDetune = 0.32; p.f1Cutoff = 0.44; });

  make(reeseTpl, "CYBER // Dystopian Sub-Grinder", "BASS", cyberAuthor, "SAW + SAW", "12x Unison + Crude LP + Asym",
    "[CYBERPUNK & SYNTHWAVE] Heavy 12-voice industrial sub-grinder with asymmetric scream distortion.",
    p => { p.waveB = 0; p.unisonVoices = 12; p.unisonDetune = 0.36; p.f1Type = 2; p.distType = 5; p.distDrive = 0.48; });

  make(reeseTpl, "CYBER // Megacity Neuro Squelch", "BASS", cyberAuthor, "SYNC + FMT", "6x Unison + Jet Flanger + Wavefold",
    "[CYBERPUNK & SYNTHWAVE] Sync-folded neuro bass with high-resonance jet flanging and vocal bite.",
    p => { p.waveA = 7; p.waveB = 4; p.warpModeA = 5; p.warpAmtA = 0.45; p.unisonVoices = 6; p.flangerEnable = true; p.flangerMix = 0.40; p.distType = 3; });

  make(reeseTpl, "CYBER // Acid Runner 303 Reese", "BASS", cyberAuthor, "SAW + SQR", "Legato Glide + SVF Reso + Clip",
    "[CYBERPUNK & SYNTHWAVE] Squelchy resonant 303 acid runner with fast slide portamento and hard clipper.",
    p => { p.voicingMode = 2; p.portaTime = 0.18; p.unisonVoices = 4; p.unisonDetune = 0.20; p.f1Res = 0.78; p.distType = 2; p.distDrive = 0.55; });

  make(reeseTpl, "CYBER // Carbon Fibre Monolith", "BASS", cyberAuthor, "SAW + SAW", "16x Unison + Tape Drive + OTT",
    "[CYBERPUNK & SYNTHWAVE] Colossal 16-voice stereo detuned Reese with maximum stereo width and tape saturation.",
    p => { p.waveB = 0; p.unisonVoices = 16; p.unisonDetune = 0.40; p.unisonSpread = 1.0; p.distType = 1; p.distDrive = 0.35; });

  make(reeseTpl, "CYBER // Toxic Waste Sub-Crush", "BASS", cyberAuthor, "SQR + TRI", "SP-1200 Crush + Lowpass",
    "[CYBERPUNK & SYNTHWAVE] Decimated 8-bit sample crunch bass with deep sub foundation.",
    p => { p.waveA = 1; p.waveB = 2; p.unisonVoices = 4; p.crushEnable = true; p.crushMode = 2; p.crushBits = 7.5; p.crushDownsample = 4.0; p.crushMix = 0.55; });

  make(reeseTpl, "CYBER // Alleyway Slag Bass", "BASS", cyberAuthor, "X-MOD + SAW", "Comb Filter + Tape Glue",
    "[CYBERPUNK & SYNTHWAVE] Metallic inharmonic comb resonance with tight lowpass anchor.",
    p => { p.waveA = 9; p.f1Type = 5; p.f1Cutoff = 0.52; p.f1Res = 0.45; p.distType = 1; });

  make(reeseTpl, "CYBER // Night City Sub-Pulse", "BASS", cyberAuthor, "SINE + SAW", "Clean Sub + OTT Comp",
    "[CYBERPUNK & SYNTHWAVE] Deep sine sub reinforced with detuned upper saw sizzle.",
    p => { p.waveA = 6; p.octA = -2; p.unisonVoices = 6; p.abMix = 0.35; p.f1Cutoff = 0.38; });

  make(reeseTpl, "CYBER // Cyberware Overload Reese", "BASS", cyberAuthor, "SAW + FMT", "12x Unison + Ring Mod Crunch",
    "[CYBERPUNK & SYNTHWAVE] Dense 12-voice Reese layered with 140Hz ring modulated upper harmonics.",
    p => { p.waveB = 4; p.unisonVoices = 12; p.ringEnable = true; p.ringFreq = 140.0; p.ringMix = 0.35; p.f1Cutoff = 0.48; });

  make(reeseTpl, "CYBER // Industrial Sub-Pumper", "BASS", cyberAuthor, "SINE + SYNC", "Time Mod 1/8 Stutter + Clipper",
    "[CYBERPUNK & SYNTHWAVE] Rhythmic buffer-chopped sub bass with automated 1/8 stutter slices.",
    p => { p.waveA = 6; p.waveB = 7; p.timeModEnable = true; p.timeModMode = 1; p.timeModRate = 4; p.timeModMix = 0.70; p.distType = 2; });

  // Archetype 2: Bitcrushed Sync Lead (LEADS) - 10 Variations
  const syncLeadTpl = {
    waveA: 7, waveB: 0, octA: 0, octB: 0, voicingMode: 2, portaTime: 0.16,
    unisonVoices: 6, unisonDetune: 0.24, unisonSpread: 0.80, unisonBlend: 0.60,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.75, f1Res: 0.28,
    distEnable: true, distType: 2, distDrive: 0.40, distMix: 0.45,
    delayEnable: true, delayTime: 0.35, delayFeedback: 0.40, delayPingPong: 0.60, delayMix: 0.28,
    reverbEnable: true, reverbSize: 0.75, reverbMix: 0.25,
    envAttack: 0.02, envDecay: 0.38, envSustain: 0.70, envRelease: 0.28
  };

  make(syncLeadTpl, "CYBER // Hologram Laser Lead", "LEADS", cyberAuthor, "SYNC + SAW", "6x Unison + Bitcrush + PingPong",
    "[CYBERPUNK & SYNTHWAVE] Sharp cutting sync lead with 8-bit digital crunch and stereo ping-pong echo.",
    p => { p.crushEnable = true; p.crushBits = 8.0; p.crushMix = 0.45; });

  make(syncLeadTpl, "CYBER // Bitcrushed Data Stream", "LEADS", cyberAuthor, "SYNC + SQR", "SP-1200 Lo-Fi + Tape Echo",
    "[CYBERPUNK & SYNTHWAVE] Gritty 12-bit sampler sync solo with warm analog tape delay.",
    p => { p.waveB = 1; p.crushEnable = true; p.crushMode = 2; p.crushBits = 7.0; p.crushDownsample = 4.5; p.crushMix = 0.50; });

  make(syncLeadTpl, "CYBER // Cyberdeck Penetrator", "LEADS", cyberAuthor, "SYNC + X-MOD", "TZ Flanger + Asym Scream",
    "[CYBERPUNK & SYNTHWAVE] Complex sync FM hybrid swept with through-zero flanging and screaming drive.",
    p => { p.waveB = 9; p.flangerEnable = true; p.flangerMode = 1; p.flangerMix = 0.48; p.distType = 5; p.distDrive = 0.48; });

  make(syncLeadTpl, "CYBER // Chrome Piercing Sync", "LEADS", cyberAuthor, "SYNC + SAW", "8x Unison + Hard Clip + Hall",
    "[CYBERPUNK & SYNTHWAVE] High-octane octave sync lead designed to slice through heavy industrial beats.",
    p => { p.octA = 1; p.unisonVoices = 8; p.distDrive = 0.55; p.reverbSize = 0.85; });

  make(syncLeadTpl, "CYBER // Rogue AI Vocal Lead", "LEADS", cyberAuthor, "FMT + SYNC", "Formant Filter + OTT Comp",
    "[CYBERPUNK & SYNTHWAVE] Expressive vocal talkbox formant lead with automated keyboard tracking.",
    p => { p.waveA = 4; p.f1Type = 6; p.f1Cutoff = 0.68; p.f1Res = 0.45; p.compEnable = true; p.compMix = 0.70; });

  make(syncLeadTpl, "CYBER // Glitch Terminal Solo", "LEADS", cyberAuthor, "SYNC + SQR", "Time Mod 1/16 Stutter + Tube",
    "[CYBERPUNK & SYNTHWAVE] Snappy sync lead with rhythmic 1/16 buffer stutter slices on sustained notes.",
    p => { p.waveB = 1; p.timeModEnable = true; p.timeModMode = 1; p.timeModRate = 5; p.timeModMix = 0.65; });

  make(syncLeadTpl, "CYBER // Megastructure Screamer", "LEADS", cyberAuthor, "SYNC + SAW", "Jet Flanger + Dimension Width",
    "[CYBERPUNK & SYNTHWAVE] Ascending resonant flanger sweeps over bright, wide supersync solo.",
    p => { p.flangerEnable = true; p.flangerMode = 0; p.flangerDepth = 0.75; p.flangerMix = 0.45; p.unisonSpread = 1.0; });

  make(syncLeadTpl, "CYBER // Neon Horizon Lead", "LEADS", cyberAuthor, "SAW + SAW", "12x Unison + Vibrato + Shimmer",
    "[CYBERPUNK & SYNTHWAVE] Massive 12-voice supersaw lead with 5.5Hz vibrato and ethereal shimmer reverb.",
    p => { p.waveA = 0; p.unisonVoices = 12; p.unisonDetune = 0.32; p.vibDepth = 0.22; p.vibSpeed = 5.5; p.reverbShimmer = 0.35; });

  make(syncLeadTpl, "CYBER // Cybernetic Shredder", "LEADS", cyberAuthor, "SYNC + X-MOD", "Mono Glide + Brutal Wavefold",
    "[CYBERPUNK & SYNTHWAVE] Aggressive wavefolded sync lead built for high-speed melodic shredding.",
    p => { p.warpModeA = 5; p.warpAmtA = 0.65; p.distType = 3; p.distDrive = 0.52; p.portaTime = 0.12; });

  make(syncLeadTpl, "CYBER // Sub-Zero Pixel Lead", "LEADS", cyberAuthor, "SQR + TRI", "4-Bit Crunch + High Shelf",
    "[CYBERPUNK & SYNTHWAVE] Ultra-lo-fi 4-bit decimated square lead with retro arcade aesthetic.",
    p => { p.waveA = 1; p.waveB = 2; p.crushEnable = true; p.crushBits = 4.5; p.crushDownsample = 8.0; p.crushMix = 0.60; });

  // Archetype 3: Outrun 80s Brass & Poly (LEADS / PLUCKS) - 10 Variations
  const outrunBrassTpl = {
    waveA: 0, waveB: 1, octA: 0, octB: 0, voicingMode: 0, maxVoices: 8,
    unisonVoices: 4, unisonDetune: 0.18, unisonSpread: 0.70, unisonBlend: 0.55,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.62, f1Res: 0.18, f1KeyTrack: 0.55,
    chorusEnable: true, chorusVoices: 4, chorusRate: 0.65, chorusDepth: 0.55, chorusMix: 0.35,
    reverbEnable: true, reverbSize: 0.72, reverbMix: 0.28,
    envAttack: 0.08, envDecay: 0.45, envSustain: 0.65, envRelease: 0.35,
    vibDepth: 0.18, vibSpeed: 5.2
  };

  make(outrunBrassTpl, "SYNTHWAVE // Miami 1984 Brass", "LEADS", synthwaveAuthor, "SAW + SQR", "4x Unison + SVF 24dB + Chorus",
    "[CYBERPUNK & SYNTHWAVE] Authentic 1984 analog brass with pitch vibrato and warm dimension chorus.",
    p => { p.f1Cutoff = 0.65; p.chorusMix = 0.38; });

  make(outrunBrassTpl, "SYNTHWAVE // Outrun Sunset Polysynth", "LEADS", synthwaveAuthor, "SAW + SAW", "8x Unison + Plate Reverb",
    "[CYBERPUNK & SYNTHWAVE] Wide 8-voice polysynth with lush plate reverb for retro outrun chords.",
    p => { p.waveB = 0; p.unisonVoices = 8; p.unisonDetune = 0.24; p.reverbSize = 0.80; });

  make(outrunBrassTpl, "SYNTHWAVE // Testarossa Highway Brass", "LEADS", synthwaveAuthor, "SAW + SQR", "Punchy Attack + Tube Sat",
    "[CYBERPUNK & SYNTHWAVE] Snappy analog brass stab with warm tube saturation and tight low-end body.",
    p => { p.envAttack = 0.04; p.distEnable = true; p.distType = 0; p.distDrive = 0.28; p.distMix = 0.35; });

  make(outrunBrassTpl, "SYNTHWAVE // Retro Palm Tree Poly", "PLUCKS", synthwaveAuthor, "TRI + SAW", "Dimension Spread + Fast Decay",
    "[CYBERPUNK & SYNTHWAVE] Nostalgic warm synthwave keys with gentle pitch drift and wide stereo image.",
    p => { p.waveA = 2; p.envDecay = 0.35; p.envSustain = 0.40; p.unisonSpread = 0.90; });

  make(outrunBrassTpl, "SYNTHWAVE // Midnight Cruiser Brass", "LEADS", synthwaveAuthor, "SAW + SQR", "Tape Slapback + SVF 24dB",
    "[CYBERPUNK & SYNTHWAVE] Moody evening brass with analog tape slapback echo and mellow lowpass filter.",
    p => { p.delayEnable = true; p.delayTime = 0.15; p.delayFeedback = 0.35; p.delayMix = 0.25; p.f1Cutoff = 0.55; });

  make(outrunBrassTpl, "SYNTHWAVE // VHS Tape Memory Synth", "PLUCKS", synthwaveAuthor, "ORG + SAW", "Lo-Fi Wow & Flutter + Warm EQ",
    "[CYBERPUNK & SYNTHWAVE] Degraded VHS cassette polysynth with subtle pitch wow and lo-fi warmth.",
    p => { p.waveA = 3; p.crushEnable = true; p.crushBits = 10.0; p.crushMix = 0.30; p.vibDepth = 0.30; p.vibSpeed = 4.2; });

  make(outrunBrassTpl, "SYNTHWAVE // Turbo Drive 80s Horns", "LEADS", synthwaveAuthor, "SAW + SAW", "Octave Stack + Punch Comp",
    "[CYBERPUNK & SYNTHWAVE] Colossal dual-octave brass horns with high-energy punch compressor.",
    p => { p.waveB = 0; p.octB = 1; p.unisonVoices = 6; p.compEnable = true; p.compMix = 0.65; });

  make(outrunBrassTpl, "SYNTHWAVE // Neon Boulevard Keys", "PLUCKS", synthwaveAuthor, "SQR + TRI", "Snappy Brass Pluck + Chorus",
    "[CYBERPUNK & SYNTHWAVE] Snappy 80s synth keys with prominent chorus modulation and crisp transients.",
    p => { p.envAttack = 0.01; p.envDecay = 0.28; p.envSustain = 0.30; p.chorusDepth = 0.65; });

  make(outrunBrassTpl, "SYNTHWAVE // Arcade Hero Polysynth", "LEADS", synthwaveAuthor, "SAW + SYNC", "Filter Sweep + PingPong Delay",
    "[CYBERPUNK & SYNTHWAVE] Bright cutting polysynth with dynamic filter envelope and stereo bounce delay.",
    p => { p.waveB = 7; p.delayEnable = true; p.delayMix = 0.30; p.f1Cutoff = 0.72; });

  make(outrunBrassTpl, "SYNTHWAVE // Sunset Strip Solo Brass", "LEADS", synthwaveAuthor, "SAW + SQR", "Legato Mono + Analog Plate",
    "[CYBERPUNK & SYNTHWAVE] Smooth mono legato lead brass with 0.22s glide and warm studio reverb.",
    p => { p.voicingMode = 2; p.portaTime = 0.22; p.maxVoices = 1; p.f1Cutoff = 0.68; });

  // Archetype 4: Cyber Rhythmic Arp & FX (ARP & SEQ / FX & TRANS) - 10 Variations
  const cyberSeqTpl = {
    waveA: 0, waveB: 7, octA: 0, octB: 0, voicingMode: 1,
    arpEnable: true, arpMode: 0, arpOctaves: 2, arpRate: 3, arpGate: 0.70,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.55, f1Res: 0.35,
    delayEnable: true, delayTime: 0.375, delayFeedback: 0.45, delayPingPong: 0.70, delayMix: 0.30,
    reverbEnable: true, reverbSize: 0.65, reverbMix: 0.22,
    envAttack: 0.01, envDecay: 0.25, envSustain: 0.0, envRelease: 0.18
  };

  make(cyberSeqTpl, "CYBER // Cyberpunk Arp 138", "ARP & SEQ", cyberAuthor, "SAW + SYNC", "16th Arp + SVF Reso + Delay",
    "[CYBERPUNK & SYNTHWAVE] High-energy 16th-step cyberpunk arpeggiator with dynamic resonance sweeps.",
    p => { p.f1Res = 0.55; p.distEnable = true; p.distDrive = 0.35; });

  make(cyberSeqTpl, "CYBER // Matrix Terminal Sequence", "ARP & SEQ", cyberAuthor, "SQR + X-MOD", "Octave Arp + 8-Bit Crush",
    "[CYBERPUNK & SYNTHWAVE] Computer terminal data sequence with 8-bit sample decimation and bounce delay.",
    p => { p.waveA = 1; p.waveB = 9; p.crushEnable = true; p.crushBits = 8.0; p.crushMix = 0.45; });

  make(cyberSeqTpl, "CYBER // Dystopian Riser FX", "FX & TRANS", cyberAuthor, "NOISE + SYNC", "Jet Flanger + Infinite Delay",
    "[CYBERPUNK & SYNTHWAVE] Tension-building noise and sync pitch riser with high-feedback jet flanger.",
    p => { p.arpEnable = false; p.noiseEnable = true; p.noiseType = 0; p.noiseLevel = 0.40; p.flangerEnable = true; p.flangerMix = 0.50; });

  make(cyberSeqTpl, "CYBER // EMP Blast Impact", "FX & TRANS", cyberAuthor, "SINE + NOISE", "Sub Drop + Bitcrush + Reverb",
    "[CYBERPUNK & SYNTHWAVE] Massive low-end electromagnetic pulse impact with decaying bitcrush tail.",
    p => { p.arpEnable = false; p.waveA = 6; p.octA = -2; p.crushEnable = true; p.crushBits = 6.0; p.reverbSize = 0.90; p.reverbMix = 0.40; });

  make(cyberSeqTpl, "CYBER // Glitch Grid Sequencer", "ARP & SEQ", cyberAuthor, "SYNC + SQR", "Time Mod 1/16 Stutter + Drive",
    "[CYBERPUNK & SYNTHWAVE] Rhythmic buffer-sliced sequence with automated 1/16 stutter chops.",
    p => { p.timeModEnable = true; p.timeModMode = 1; p.timeModRate = 5; p.timeModMix = 0.70; p.distEnable = true; });

  make(cyberSeqTpl, "CYBER // Neon Rain Downer FX", "FX & TRANS", cyberAuthor, "NOISE + SAW", "Comb Filtered Descending Fall",
    "[CYBERPUNK & SYNTHWAVE] Atmospheric rain transition sweep created with comb filtered white noise.",
    p => { p.arpEnable = false; p.noiseEnable = true; p.f1Type = 5; p.f1Cutoff = 0.45; p.f1Res = 0.65; });

  make(cyberSeqTpl, "CYBER // Hack The Planet Arp", "ARP & SEQ", cyberAuthor, "X-MOD + SQR", "Snappy FM Pluck Sequence",
    "[CYBERPUNK & SYNTHWAVE] Fast-paced FM arpeggio with metallic transients and dimension stereo spread.",
    p => { p.waveA = 9; p.waveB = 1; p.arpRate = 4; p.unisonSpread = 0.85; });

  make(cyberSeqTpl, "CYBER // Subterranean Drone Sweep", "FX & TRANS", cyberAuthor, "SAW + X-MOD", "Slow Phaser + Cosmic Delay",
    "[CYBERPUNK & SYNTHWAVE] Dark dystopian subterranean drone modulated with slow stereo phaser sweeps.",
    p => { p.arpEnable = false; p.octA = -2; p.octB = -2; p.phaserEnable = true; p.phaserMix = 0.45; p.envAttack = 0.50; });

  make(cyberSeqTpl, "CYBER // Cyber Tunnel Riser", "FX & TRANS", cyberAuthor, "SYNC + SAW", "Barberpole Flanger + Reverb",
    "[CYBERPUNK & SYNTHWAVE] Infinite ascending barberpole flanger modulation over rising sync harmonics.",
    p => { p.arpEnable = false; p.flangerEnable = true; p.flangerMode = 2; p.flangerMix = 0.55; p.reverbSize = 0.85; });

  make(cyberSeqTpl, "CYBER // Rewind Glitch Transition", "FX & TRANS", cyberAuthor, "SQR + SYNC", "Time Mod Tape Reverse Warp",
    "[CYBERPUNK & SYNTHWAVE] Dynamic tape reverse transition effect utilizing circular buffer manipulation.",
    p => { p.arpEnable = false; p.timeModEnable = true; p.timeModMode = 3; p.timeModMix = 0.85; });

  // ===========================================================================
  // PACK 2: CINEMATIC & AMBIENT (40 Patches)
  // ===========================================================================
  const cineAuthor = "Prismor Cinematic";
  const ambientAuthor = "Prismor Ambient";

  // Archetype 5: 16x Unison Evolving Horizon Pad (PADS) - 10 Variations
  const horizonPadTpl = {
    waveA: 0, waveB: 0, octA: 0, octB: 1, voicingMode: 0, maxVoices: 8,
    unisonVoices: 16, unisonDetune: 0.32, unisonSpread: 1.0, unisonBlend: 0.70,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.52, f1Res: 0.15,
    reverbEnable: true, reverbSize: 0.90, reverbDecay: 0.70, reverbShimmer: 0.45, reverbMix: 0.40,
    envAttack: 0.65, envDecay: 0.80, envSustain: 0.85, envRelease: 0.90
  };

  make(horizonPadTpl, "CINEMATIC // 16x Cosmic Horizon Pad", "PADS", cineAuthor, "SAW + SAW", "16x Unison + Shimmer Hall",
    "[CINEMATIC & AMBIENT] Gigantic cinematic soundscape with 16 stereo detuned voices and cosmic shimmer tail.",
    p => { p.unisonVoices = 16; p.f1Cutoff = 0.50; });

  make(horizonPadTpl, "CINEMATIC // Aurora Borealis Wash", "PADS", cineAuthor, "SAW + ORG", "16x Unison + Dimension Chorus",
    "[CINEMATIC & AMBIENT] Luminous arctic northern lights wash with organ drawbars and 0.95 hall space.",
    p => { p.waveB = 3; p.chorusEnable = true; p.chorusMix = 0.35; p.reverbSize = 0.95; });

  make(horizonPadTpl, "CINEMATIC // Deep Space Nebula", "PADS", cineAuthor, "SAW + SQR", "Sub Layer + Slow Phaser Sweep",
    "[CINEMATIC & AMBIENT] Sub-octave pad with gentle 0.3Hz phaser movement across stereo field.",
    p => { p.waveB = 1; p.octB = -1; p.phaserEnable = true; p.phaserMix = 0.38; p.phaserSpeed = 0.4; });

  make(horizonPadTpl, "CINEMATIC // Interstellar Departure", "PADS", cineAuthor, "SAW + SAW", "Harmonic Crossfade + Swell",
    "[CINEMATIC & AMBIENT] Slow evolving pad swell with dynamic spectral harmonic tilt.",
    p => { p.envAttack = 1.0; p.harmTiltA = -0.25; p.harmTiltB = 0.35; });

  make(horizonPadTpl, "CINEMATIC // Glacial Ice Shelf Pad", "PADS", cineAuthor, "SYNC + SAW", "Comb Filter + Crystal Shimmer",
    "[CINEMATIC & AMBIENT] Crystalline frozen landscape with subtle metallic comb resonances.",
    p => { p.waveA = 7; p.f1Type = 5; p.f1Cutoff = 0.62; p.f1Res = 0.40; p.reverbShimmer = 0.55; });

  make(horizonPadTpl, "CINEMATIC // Event Horizon Solitude", "PADS", cineAuthor, "TRI + SAW", "Dark Filtered 16x Unison",
    "[CINEMATIC & AMBIENT] Somber cinematic mood pad with muted lowpass filtering and long 1.2s attack.",
    p => { p.waveA = 2; p.f1Cutoff = 0.38; p.envAttack = 1.2; p.envRelease = 1.2; });

  make(horizonPadTpl, "CINEMATIC // Shimmering Starfield", "PADS", cineAuthor, "SAW + SAW", "Octave Stack + Ethereal Tail",
    "[CINEMATIC & AMBIENT] Dual octave stack unison pad generating bright twinkling stellar halos.",
    p => { p.unisonStack = 1; p.reverbShimmer = 0.50; p.reverbSize = 0.92; });

  make(horizonPadTpl, "CINEMATIC // Floating In The Void", "PADS", cineAuthor, "SAW + SQR", "Tape Saturation + Stereo Width",
    "[CINEMATIC & AMBIENT] Warm analog space pad glued together with subtle tape saturation.",
    p => { p.distEnable = true; p.distType = 1; p.distDrive = 0.22; p.distMix = 0.30; });

  make(horizonPadTpl, "CINEMATIC // Supernova Afterglow", "PADS", cineAuthor, "SAW + TRI", "1.5s Release + Soft Glue",
    "[CINEMATIC & AMBIENT] Expansive emotional chord pad with 1.5-second natural decay tail.",
    p => { p.waveB = 2; p.envRelease = 1.5; p.f1Cutoff = 0.48; });

  make(horizonPadTpl, "CINEMATIC // Black Hole Gravitation", "PADS", cineAuthor, "SAW + X-MOD", "Deep Lowpad + Slow Tremolo",
    "[CINEMATIC & AMBIENT] Subterranean cinematic pad with gentle tremolo undulations.",
    p => { p.waveB = 9; p.octA = -1; p.octB = -1; p.f1Cutoff = 0.36; });

  // Archetype 6: Celestial Vocal Vowel Choir (PADS) - 10 Variations
  const choirTpl = {
    waveA: 4, waveB: 3, octA: 0, octB: 0, voicingMode: 0, maxVoices: 8,
    unisonVoices: 8, unisonDetune: 0.22, unisonSpread: 0.85, unisonBlend: 0.60,
    filter1Enable: true, filter1Type: 6, f1Cutoff: 0.65, f1Res: 0.42,
    noiseEnable: true, noiseType: 5, noiseLevel: 0.16,
    reverbEnable: true, reverbSize: 0.88, reverbDecay: 0.65, reverbShimmer: 0.35, reverbMix: 0.38,
    envAttack: 0.50, envDecay: 0.70, envSustain: 0.80, envRelease: 0.65
  };

  make(choirTpl, "CINEMATIC // Cathedral Angel Choir", "PADS", cineAuthor, "FMT + ORG", "Vocal Formant + Breath Air",
    "[CINEMATIC & AMBIENT] Heavenly vowel choir pad with formant shaping and subtle breath air noise texture.",
    p => { p.f1Cutoff = 0.68; });

  make(choirTpl, "CINEMATIC // Gregorian Drone Choir", "PADS", cineAuthor, "FMT + SAW", "Low Masculine Vowel Swell",
    "[CINEMATIC & AMBIENT] Resonant medieval Gregorian chant choir tuned in lower registers.",
    p => { p.octA = -1; p.octB = -1; p.f1Cutoff = 0.52; p.f1Res = 0.50; });

  make(choirTpl, "CINEMATIC // Ethereal Siren Call", "PADS", cineAuthor, "FMT + SINE", "High Register Vowel + Chorus",
    "[CINEMATIC & AMBIENT] Delicate female vocal vowel pad with silky dimension chorus.",
    p => { p.octA = 1; p.octB = 1; p.chorusEnable = true; p.chorusMix = 0.35; });

  make(choirTpl, "CINEMATIC // Sacred Temple Voices", "PADS", cineAuthor, "FMT + FMT", "Dual Formant + Tape Delay",
    "[CINEMATIC & AMBIENT] Rich choral harmonies echoing inside an ancient acoustic temple.",
    p => { p.waveB = 4; p.delayEnable = true; p.delayMix = 0.25; });

  make(choirTpl, "CINEMATIC // Cyber Vowel Meditation", "PADS", cineAuthor, "FMT + TRI", "Rotary Speaker Modulation",
    "[CINEMATIC & AMBIENT] Futuristic robotic choir with warm rotating speaker texture.",
    p => { p.ringEnable = true; p.ringMode = 3; p.ringFreq = 220.0; p.ringMix = 0.35; });

  make(choirTpl, "CINEMATIC // Ancient Sanctuary Choir", "PADS", cineAuthor, "FMT + ORG", "Cathedral Reverb 0.92",
    "[CINEMATIC & AMBIENT] Vast liturgical choral wash with 0.92 cathedral space and warm tube drive.",
    p => { p.reverbSize = 0.92; p.distEnable = true; p.distType = 0; p.distDrive = 0.20; });

  make(choirTpl, "CINEMATIC // Whispering Spirits", "PADS", cineAuthor, "FMT + NOISE", "Air Noise + Bandpass Sweep",
    "[CINEMATIC & AMBIENT] Ghostly choral whispering created with bandpass filtered breath noise.",
    p => { p.noiseLevel = 0.30; p.f1Type = 4; p.f1Cutoff = 0.58; });

  make(choirTpl, "CINEMATIC // Celestial Hymn Pad", "PADS", cineAuthor, "FMT + SAW", "Dual Formant Swell",
    "[CINEMATIC & AMBIENT] Grand harmonic hymn pad with slow, uplifting 1.0s attack envelope.",
    p => { p.envAttack = 1.0; p.unisonVoices = 12; });

  make(choirTpl, "CINEMATIC // Gregorian Cyber Chants", "PADS", cineAuthor, "FMT + X-MOD", "Through-Zero Flanger Choir",
    "[CINEMATIC & AMBIENT] Dystopian religious vocal drone with through-zero flanger phasing.",
    p => { p.waveB = 9; p.flangerEnable = true; p.flangerMode = 1; p.flangerMix = 0.40; });

  make(choirTpl, "CINEMATIC // Requiem For Stars", "PADS", cineAuthor, "FMT + SAW", "12-Voice Choral Unison",
    "[CINEMATIC & AMBIENT] Somber cinematic choir pad written for tragic sci-fi moments.",
    p => { p.unisonVoices = 12; p.f1Cutoff = 0.58; p.envRelease = 0.95; });

  // Archetype 7: Glass Bell & Shimmer Drone (PADS / PLUCKS) - 10 Variations
  const bellAmbientTpl = {
    waveA: 6, waveB: 8, octA: 0, octB: 1, voicingMode: 0, maxVoices: 8,
    unisonVoices: 6, unisonDetune: 0.20, unisonSpread: 0.80, unisonBlend: 0.55,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.72, f1Res: 0.18,
    reverbEnable: true, reverbSize: 0.88, reverbDecay: 0.68, reverbShimmer: 0.50, reverbMix: 0.42,
    delayEnable: true, delayTime: 0.375, delayFeedback: 0.45, delayPingPong: 0.75, delayMix: 0.32,
    envAttack: 0.05, envDecay: 0.65, envSustain: 0.55, envRelease: 0.75
  };

  make(bellAmbientTpl, "AMBIENT // Glass Cathedral Bells", "PADS", ambientAuthor, "SINE + BELL", "Glass Harmonics + Shimmer Reverb",
    "[CINEMATIC & AMBIENT] Crystalline glass bell keys with endless shimmer reverb reflections.",
    p => { p.f1Cutoff = 0.75; });

  make(bellAmbientTpl, "AMBIENT // Tibetan Singing Bowl", "PADS", ambientAuthor, "SINE + SINE", "Harmonic Beating + Pure Sine",
    "[CINEMATIC & AMBIENT] Meditative singing bowl emulation with subtle micro-tuning beating.",
    p => { p.waveB = 6; p.fineB = 4.0; p.delayMix = 0.20; p.reverbShimmer = 0.30; });

  make(bellAmbientTpl, "AMBIENT // Frozen Stalactite Mallet", "PLUCKS", ambientAuthor, "BELL + TRI", "Snappy Glass + Crystal Echo",
    "[CINEMATIC & AMBIENT] Snappy percussive ice mallet with bright crystal delay repeats.",
    p => { p.waveB = 2; p.envAttack = 0.01; p.envDecay = 0.35; p.envSustain = 0.20; });

  make(bellAmbientTpl, "AMBIENT // Shimmering Crystal Drops", "PADS", ambientAuthor, "BELL + SINE", "Slow Arp + Hall Space",
    "[CINEMATIC & AMBIENT] Ethereal raindrops hitting water with cosmic pitch shimmer tail.",
    p => { p.reverbShimmer = 0.60; p.delayTime = 0.25; });

  make(bellAmbientTpl, "AMBIENT // Warm Analog Rain Pad", "PADS", ambientAuthor, "TRI + SAW", "Brownian Noise + Soft LP",
    "[CINEMATIC & AMBIENT] Soothing ambient pad layered with brownian noise rain texture.",
    p => { p.waveA = 2; p.waveB = 0; p.noiseEnable = true; p.noiseType = 2; p.noiseLevel = 0.20; p.f1Cutoff = 0.48; });

  make(bellAmbientTpl, "AMBIENT // Zen Garden Wind Chimes", "PLUCKS", ambientAuthor, "BELL + BELL", "Spectral Bells + Stereo PingPong",
    "[CINEMATIC & AMBIENT] Organic wind chime plucks drifting across stereo panorama.",
    p => { p.waveA = 8; p.envAttack = 0.01; p.envDecay = 0.40; p.delayPingPong = 0.90; });

  make(bellAmbientTpl, "AMBIENT // Deep Ocean Glow", "PADS", ambientAuthor, "SINE + TRI", "Sub Drone + Phaser Glow",
    "[CINEMATIC & AMBIENT] Subaquatic ambient atmosphere with gentle phaser modulation.",
    p => { p.waveB = 2; p.octA = -1; p.phaserEnable = true; p.phaserMix = 0.35; });

  make(bellAmbientTpl, "AMBIENT // Arctic Windscape", "PADS", ambientAuthor, "NOISE + SINE", "Breath Noise + Resonant Peak",
    "[CINEMATIC & AMBIENT] Desolate arctic blizzard wind soundscape with tonal bell core.",
    p => { p.noiseEnable = true; p.noiseType = 5; p.noiseLevel = 0.35; p.f1Cutoff = 0.55; p.f1Res = 0.45; });

  make(bellAmbientTpl, "AMBIENT // Moonlit Mirage Glass", "PADS", ambientAuthor, "BELL + ORG", "Dual Bell + Dimension Chorus",
    "[CINEMATIC & AMBIENT] Shimmering nighttime mirage with glassy harmonic bells and warm organ.",
    p => { p.waveB = 3; p.chorusEnable = true; p.chorusMix = 0.40; });

  make(bellAmbientTpl, "AMBIENT // Timeless Void Ambient", "PADS", ambientAuthor, "SINE + SAW", "Granular Halftime + Space",
    "[CINEMATIC & AMBIENT] Half-time time-stretched ambient drone with 1.5-second envelope times.",
    p => { p.timeModEnable = true; p.timeModMode = 4; p.timeModMix = 0.70; p.envAttack = 1.2; p.envRelease = 1.4; });

  // Archetype 8: Subterranean Drones & Impacts (FX & TRANS / PADS) - 10 Variations
  const droneTpl = {
    waveA: 0, waveB: 9, octA: -2, octB: -2, voicingMode: 0, maxVoices: 4,
    unisonVoices: 8, unisonDetune: 0.22, unisonSpread: 0.85, unisonBlend: 0.60,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.34, f1Res: 0.25,
    distEnable: true, distType: 1, distDrive: 0.32, distMix: 0.40,
    reverbEnable: true, reverbSize: 0.85, reverbMix: 0.35,
    envAttack: 0.45, envDecay: 0.80, envSustain: 0.85, envRelease: 0.90
  };

  make(droneTpl, "CINEMATIC // Subterranean Titan Drone", "PADS", cineAuthor, "SAW + X-MOD", "12x Unison + Sub Bass + Tape",
    "[CINEMATIC & AMBIENT] Massive subterranean bass drone with tape saturation and 12-voice detune.",
    p => { p.unisonVoices = 12; p.f1Cutoff = 0.32; });

  make(droneTpl, "CINEMATIC // Deep Trench Resonance", "PADS", cineAuthor, "X-MOD + SINE", "Comb Filter + Tape Delay",
    "[CINEMATIC & AMBIENT] Oceanic trench drone with metallic comb resonances and slow tape echo.",
    p => { p.waveA = 9; p.waveB = 6; p.f1Type = 5; p.f1Cutoff = 0.42; p.delayEnable = true; p.delayMix = 0.28; });

  make(droneTpl, "CINEMATIC // Tectonic Shift Drone", "PADS", cineAuthor, "NOISE + SAW", "Brown Noise + Tube Saturation",
    "[CINEMATIC & AMBIENT] Low-frequency geological rumble with warm tube drive.",
    p => { p.noiseEnable = true; p.noiseType = 2; p.noiseLevel = 0.35; p.f1Cutoff = 0.28; });

  make(droneTpl, "CINEMATIC // Dark Matter Cluster", "PADS", cineAuthor, "X-MOD + SYNC", "Inharmonic Drone + Phaser",
    "[CINEMATIC & AMBIENT] Mysterious extraterrestrial atmosphere with slow sweeping phaser notches.",
    p => { p.waveB = 7; p.phaserEnable = true; p.phaserMix = 0.45; });

  make(droneTpl, "CINEMATIC // Post-Apocalyptic Horizon", "PADS", cineAuthor, "SAW + SQR", "Muted Saw Drone + Shimmer Tail",
    "[CINEMATIC & AMBIENT] Desolate wasteland drone with gentle harmonic shimmer glow.",
    p => { p.waveB = 1; p.reverbShimmer = 0.40; p.f1Cutoff = 0.36; });

  make(droneTpl, "CINEMATIC // Cinematic Braam Hit", "FX & TRANS", cineAuthor, "SAW + SYNC", "Low Brass Impact + Hard Clip",
    "[CINEMATIC & AMBIENT] Signature blockbuster trailer brass impact with hard clipper saturation.",
    p => { p.envAttack = 0.01; p.envDecay = 0.65; p.envSustain = 0.20; p.distType = 2; p.distDrive = 0.55; });

  make(droneTpl, "CINEMATIC // Sub-Bass Trailer Boom", "FX & TRANS", cineAuthor, "SINE + TRI", "808 Sub Drop + Punch Comp",
    "[CINEMATIC & AMBIENT] Deep earth-shaking sub drop for cinematic movie transitions.",
    p => { p.waveA = 6; p.waveB = 2; p.octA = -3; p.octB = -2; p.envAttack = 0.01; p.envDecay = 0.85; p.compEnable = true; });

  make(droneTpl, "CINEMATIC // Orbital Re-entry Drone", "FX & TRANS", cineAuthor, "SYNC + NOISE", "Jet Flanger Sweep + Space",
    "[CINEMATIC & AMBIENT] Spacecraft heat-shield atmospheric friction sound with jet flanging.",
    p => { p.flangerEnable = true; p.flangerMix = 0.50; p.noiseEnable = true; p.noiseLevel = 0.25; });

  make(droneTpl, "CINEMATIC // Sunken Ruins Ambiance", "PADS", cineAuthor, "SINE + ORG", "Time Mod Filter + Dark Drone",
    "[CINEMATIC & AMBIENT] Ancient submerged temple drone modulated with circular buffer filters.",
    p => { p.timeModEnable = true; p.timeModMode = 0; p.timeModMix = 0.60; });

  make(droneTpl, "CINEMATIC // Ghost Ship Wind Drone", "PADS", cineAuthor, "NOISE + FMT", "Pink Noise + Formant Swell",
    "[CINEMATIC & AMBIENT] Ghostly nautical wind drone with distant vocal formant resonances.",
    p => { p.noiseEnable = true; p.noiseType = 1; p.noiseLevel = 0.30; p.f1Type = 6; });

  // ===========================================================================
  // PACK 3: MODERN TECH HOUSE (40 Patches)
  // ===========================================================================
  const houseAuthor = "Prismor Tech House";

  // Archetype 9: Punchy FM Sub-Donk Bass (BASS) - 10 Variations
  const donkSubTpl = {
    waveA: 6, waveB: 9, octA: -2, octB: -1, voicingMode: 1,
    unisonVoices: 1, unisonDetune: 0.15, unisonSpread: 0.50, unisonBlend: 0.50,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.44, f1Res: 0.18,
    distEnable: true, distType: 0, distDrive: 0.28, distMix: 0.40,
    compEnable: true, compMode: 3, compThreshold: -14.0, compRatio: 6.0, compMakeup: 2.5, compMix: 0.75,
    envAttack: 0.01, envDecay: 0.24, envSustain: 0.0, envRelease: 0.16
  };

  make(donkSubTpl, "TECH HOUSE // Club Punch FM Sub", "BASS", houseAuthor, "SINE + X-MOD", "Snappy Decay 0.24s + Punch Comp",
    "[MODERN TECH HOUSE] Snappy club-ready FM sub donk bass with punchy transient attack.",
    p => { p.f1Cutoff = 0.46; });

  make(donkSubTpl, "TECH HOUSE // Warehouse Donk Bass", "BASS", houseAuthor, "SQR + SINE", "Pitch Envelope + Tube Drive",
    "[MODERN TECH HOUSE] Heavy warehouse donk bass with octave pitch blip and low-shelf boost.",
    p => { p.waveA = 1; p.octA = -1; p.distDrive = 0.42; });

  make(donkSubTpl, "TECH HOUSE // Ibiza Morning Acid Bass", "BASS", houseAuthor, "SAW + SQR", "303 Squelch + Slide + Clip",
    "[MODERN TECH HOUSE] Squelchy resonant acid bassline with fast slide portamento and hard clipper.",
    p => { p.waveA = 0; p.waveB = 1; p.octA = -1; p.voicingMode = 2; p.portaTime = 0.18; p.f1Res = 0.72; p.distType = 2; });

  make(donkSubTpl, "TECH HOUSE // Underground Roller Bass", "BASS", houseAuthor, "SAW + SAW", "Dual Saw + 24dB Lowpass + Tape",
    "[MODERN TECH HOUSE] Rolling underground tech house bassline with warm tape saturation.",
    p => { p.waveA = 0; p.waveB = 0; p.octA = -1; p.octB = -1; p.unisonVoices = 4; p.unisonDetune = 0.20; p.f1Cutoff = 0.40; });

  make(donkSubTpl, "TECH HOUSE // Peak Time Square Donk", "BASS", houseAuthor, "SQR + SINE", "Short 0.18s Decay + Snappy",
    "[MODERN TECH HOUSE] Short, punchy square bass donk designed for peak-time driving tracks.",
    p => { p.waveA = 1; p.envDecay = 0.18; p.f1Cutoff = 0.48; });

  make(donkSubTpl, "TECH HOUSE // Raw Analog Jack Bass", "BASS", houseAuthor, "TRI + SAW", "Tube Saturation + Fast Attack",
    "[MODERN TECH HOUSE] Chicago jack-style raw analog bass with punchy low-end bite.",
    p => { p.waveA = 2; p.waveB = 0; p.distDrive = 0.35; p.f1Cutoff = 0.50; });

  make(donkSubTpl, "TECH HOUSE // Metallic Groove Sub", "BASS", houseAuthor, "SINE + X-MOD", "Ring Mod 110Hz + Snappy LP",
    "[MODERN TECH HOUSE] Hollow ring-modulated bass pluck with distinct metallic transient bite.",
    p => { p.ringEnable = true; p.ringFreq = 110.0; p.ringMix = 0.45; });

  make(donkSubTpl, "TECH HOUSE // Dark Minimal Sub-Donk", "BASS", houseAuthor, "SINE + SINE", "Clean Sub Fundamental + Crude LP",
    "[MODERN TECH HOUSE] Minimalist deep tech sub bass with zero upper-end clutter.",
    p => { p.waveB = 6; p.f1Type = 2; p.f1Cutoff = 0.38; p.distEnable = false; });

  make(donkSubTpl, "TECH HOUSE // Bouncy House Bassline", "BASS", houseAuthor, "SINE + SYNC", "Snappy Drop + Punch Limiter",
    "[MODERN TECH HOUSE] Snappy bounce house bass with sync harmonic edge and tight punch.",
    p => { p.waveB = 7; p.envDecay = 0.22; p.distDrive = 0.30; });

  make(donkSubTpl, "TECH HOUSE // Deep Tech Acid Roller", "BASS", houseAuthor, "SAW + SAW", "Legato Mono + Reso Overdrive",
    "[MODERN TECH HOUSE] Hypnotic rolling acid line with automated resonance bite and fast glide.",
    p => { p.waveA = 0; p.waveB = 0; p.voicingMode = 2; p.portaTime = 0.16; p.f1Res = 0.65; p.distDrive = 0.45; });

  // Archetype 10: Snappy Percussive Mallet Pluck (PLUCKS) - 10 Variations
  const malletTpl = {
    waveA: 2, waveB: 8, octA: 0, octB: 1, voicingMode: 0, maxVoices: 8,
    unisonVoices: 2, unisonDetune: 0.14, unisonSpread: 0.65, unisonBlend: 0.50,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.68, f1Res: 0.22,
    delayEnable: true, delayTime: 0.375, delayFeedback: 0.38, delayPingPong: 0.65, delayMix: 0.26,
    reverbEnable: true, reverbSize: 0.65, reverbMix: 0.22,
    envAttack: 0.008, envDecay: 0.20, envSustain: 0.0, envRelease: 0.18
  };

  make(malletTpl, "TECH HOUSE // Snappy Woodblock Pluck", "PLUCKS", houseAuthor, "TRI + BELL", "Short 0.16s Decay + Slap Echo",
    "[MODERN TECH HOUSE] Snappy wooden percussion pluck with tight lowpass envelope and stereo slapback.",
    p => { p.envDecay = 0.16; p.f1Cutoff = 0.62; });

  make(malletTpl, "TECH HOUSE // Glassy Marimba Mallet", "PLUCKS", houseAuthor, "BELL + SINE", "Glass Harmonics + Plate Reverb",
    "[MODERN TECH HOUSE] Bright percussive marimba pluck designed for infectious tech house melodies.",
    p => { p.waveA = 8; p.waveB = 6; p.f1Cutoff = 0.75; });

  make(malletTpl, "TECH HOUSE // Hollow Tribal Perc Pluck", "PLUCKS", houseAuthor, "SQR + TRI", "Bandpass Reso + Tape Saturation",
    "[MODERN TECH HOUSE] Hollow tribal percussive pluck with resonant bandpass filtering.",
    p => { p.waveA = 1; p.f1Type = 4; p.f1Cutoff = 0.58; p.f1Res = 0.45; });

  make(malletTpl, "TECH HOUSE // Minimalist Click Pluck", "PLUCKS", houseAuthor, "SYNC + NOISE", "Highpass Click + Chorus",
    "[MODERN TECH HOUSE] Ultra-short click transient pluck for subtle background rhythm layers.",
    p => { p.waveA = 7; p.noiseEnable = true; p.noiseLevel = 0.20; p.envDecay = 0.12; });

  make(malletTpl, "TECH HOUSE // Kalimba Dance Mallet", "PLUCKS", houseAuthor, "SINE + TRI", "Sine Pluck + Stereo Delay 3/16",
    "[MODERN TECH HOUSE] Warm kalimba pluck with dotted eighth delay repeats for house hooks.",
    p => { p.waveA = 6; p.delayTime = 0.375; p.delayMix = 0.32; });

  make(malletTpl, "TECH HOUSE // Steel Tongue Drum", "PLUCKS", houseAuthor, "BELL + X-MOD", "Metallic FM Ring + Punch Comp",
    "[MODERN TECH HOUSE] Resonant steel pan percussion pluck with harmonic FM overtones.",
    p => { p.waveB = 9; p.f1Cutoff = 0.70; p.compEnable = true; });

  make(malletTpl, "TECH HOUSE // Groovy Organ Stabs", "PLUCKS", houseAuthor, "ORG + SQR", "Short Organ Stab + Tube Drive",
    "[MODERN TECH HOUSE] Classic house organ stab with fast attack and warm tube saturation.",
    p => { p.waveA = 3; p.waveB = 1; p.distEnable = true; p.distDrive = 0.30; p.envDecay = 0.26; });

  make(malletTpl, "TECH HOUSE // Sync Percussive Blip", "PLUCKS", houseAuthor, "SYNC + SINE", "Sync Warp + Snappy Limiter",
    "[MODERN TECH HOUSE] Cutting sync blip pluck designed for polyrhythmic top-line grooves.",
    p => { p.waveA = 7; p.warpModeA = 5; p.warpAmtA = 0.40; p.envDecay = 0.14; });

  make(malletTpl, "TECH HOUSE // African Synth Balafon", "PLUCKS", houseAuthor, "TRI + BELL", "Pitch Envelope Drop + Dimension",
    "[MODERN TECH HOUSE] Organic wooden balafon mallet with slight pitch decay and stereo spread.",
    p => { p.unisonSpread = 0.85; p.f1Cutoff = 0.65; });

  make(malletTpl, "TECH HOUSE // Deep Groove Chord Pluck", "PLUCKS", houseAuthor, "SAW + SQR", "Minor Chord Stab + Plate",
    "[MODERN TECH HOUSE] Snappy polyphonic chord pluck for rhythmic minor key stabs.",
    p => { p.waveA = 0; p.waveB = 1; p.unisonVoices = 4; p.f1Cutoff = 0.58; p.envDecay = 0.28; });

  // Archetype 11: 16-Step Trancegate & Groove Arp (ARP & SEQ) - 10 Variations
  const houseArpTpl = {
    waveA: 0, waveB: 1, octA: 0, octB: 0, voicingMode: 1,
    arpEnable: true, arpMode: 0, arpOctaves: 1, arpRate: 3, arpGate: 0.65,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.58, f1Res: 0.25,
    delayEnable: true, delayTime: 0.375, delayFeedback: 0.42, delayPingPong: 0.65, delayMix: 0.28,
    reverbEnable: true, reverbSize: 0.60, reverbMix: 0.20,
    envAttack: 0.01, envDecay: 0.22, envSustain: 0.0, envRelease: 0.15
  };

  make(houseArpTpl, "TECH HOUSE // 16th Minimal Groove Arp", "ARP & SEQ", houseAuthor, "SAW + SQR", "16th-Step + Tight LP",
    "[MODERN TECH HOUSE] Clean 16th-note driving groove arpeggiator with tight lowpass cutoff.",
    p => { p.f1Cutoff = 0.52; });

  make(houseArpTpl, "TECH HOUSE // Trancegate Chords 128", "ARP & SEQ", houseAuthor, "SAW + SAW", "Gross Beat 1/16 Gate + Chorus",
    "[MODERN TECH HOUSE] Rhythmic 16th-gate pattern chopping sustained house chords.",
    p => { p.arpEnable = false; p.timeModEnable = true; p.timeModMode = 1; p.timeModRate = 5; p.timeModMix = 0.80; p.chorusEnable = true; });

  make(houseArpTpl, "TECH HOUSE // Acid Slide Sequence", "ARP & SEQ", houseAuthor, "SAW + SQR", "16th Acid Pattern + Reso Sweeps",
    "[MODERN TECH HOUSE] Dynamic acid groove with screaming resonance and automated accents.",
    p => { p.f1Res = 0.68; p.distEnable = true; p.distDrive = 0.45; });

  make(houseArpTpl, "TECH HOUSE // Warehouse Shaker Arp", "ARP & SEQ", houseAuthor, "NOISE + SAW", "1/16 Swing 0.35 + Delay",
    "[MODERN TECH HOUSE] Percussive rhythmic shaker groove layered with white noise bursts.",
    p => { p.noiseEnable = true; p.noiseLevel = 0.30; p.arpSwing = 0.35; });

  make(houseArpTpl, "TECH HOUSE // Hypnotic FM Arpeggio", "ARP & SEQ", houseAuthor, "X-MOD + SINE", "X-Mod Harmonics + PingPong",
    "[MODERN TECH HOUSE] Hypnotic FM arpeggiator with crystal clear repeating delays.",
    p => { p.waveA = 9; p.waveB = 6; p.f1Cutoff = 0.65; });

  make(houseArpTpl, "TECH HOUSE // Ibiza Sunrise Sequence", "ARP & SEQ", houseAuthor, "BELL + SAW", "Melodic Pluck Arp + Shimmer",
    "[MODERN TECH HOUSE] Uplifting melodic pluck sequence with shimmering reverb tail.",
    p => { p.waveA = 8; p.reverbShimmer = 0.40; p.delayMix = 0.35; });

  make(houseArpTpl, "TECH HOUSE // Bouncy Offbeat Staccato", "ARP & SEQ", houseAuthor, "SQR + TRI", "Offbeat 1/16 + Tube Drive",
    "[MODERN TECH HOUSE] Staccato offbeat pattern designed to lock with four-on-the-floor kicks.",
    p => { p.waveA = 1; p.waveB = 2; p.distEnable = true; p.distDrive = 0.32; });

  make(houseArpTpl, "TECH HOUSE // Percussive Mallet Arp", "ARP & SEQ", houseAuthor, "TRI + BELL", "Snappy Notes + 3/16 Delay",
    "[MODERN TECH HOUSE] Rhythmic wooden mallet arpeggio with polyrhythmic dotted repeats.",
    p => { p.waveA = 2; p.waveB = 8; p.delayTime = 0.375; });

  make(houseArpTpl, "TECH HOUSE // Deep Tech Chord Roller", "ARP & SEQ", houseAuthor, "SAW + SQR", "1/8T Triplet Groove + Phaser",
    "[MODERN TECH HOUSE] Rolling triplet groove with gentle stereo phaser movement.",
    p => { p.arpRate = 2; p.phaserEnable = true; p.phaserMix = 0.32; });

  make(houseArpTpl, "TECH HOUSE // Peak-Time Driving Arp", "ARP & SEQ", houseAuthor, "SAW + SAW", "8x Unison + OTT Upward Comp",
    "[MODERN TECH HOUSE] Massive 8-voice supersaw arpeggio built for peak-time dancefloors.",
    p => { p.waveB = 0; p.unisonVoices = 8; p.unisonDetune = 0.26; p.compEnable = true; p.compMix = 0.75; });

  // Archetype 12: White Noise Sweeps & Build FX (FX & TRANS) - 10 Variations
  const sweepTpl = {
    waveA: 0, waveB: 0, octA: 0, octB: 0, voicingMode: 1,
    noiseEnable: true, noiseType: 0, noiseLevel: 0.45, noiseFilter: 0.50,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.50, f1Res: 0.40,
    flangerEnable: true, flangerMode: 0, flangerDepth: 0.65, flangerMix: 0.45,
    delayEnable: true, delayTime: 0.375, delayFeedback: 0.55, delayMix: 0.35,
    reverbEnable: true, reverbSize: 0.85, reverbMix: 0.40,
    envAttack: 0.50, envDecay: 1.0, envSustain: 0.60, envRelease: 1.0
  };

  make(sweepTpl, "TECH HOUSE // 4-Bar White Noise Riser", "FX & TRANS", houseAuthor, "NOISE ONLY", "Pitch Sweep + Jet Flanger",
    "[MODERN TECH HOUSE] Classic 4-bar white noise tension riser with resonant jet flanging.",
    p => { p.noiseLevel = 0.55; p.f1Cutoff = 0.65; });

  make(sweepTpl, "TECH HOUSE // Endless Build Uplifter", "FX & TRANS", houseAuthor, "NOISE + SYNC", "Barberpole Flanger + Hall",
    "[MODERN TECH HOUSE] Infinite climbing build uplifter with barberpole frequency modulation.",
    p => { p.waveB = 7; p.flangerMode = 2; p.reverbSize = 0.92; });

  make(sweepTpl, "TECH HOUSE // Sub Drop Impact Boom", "FX & TRANS", houseAuthor, "SINE + NOISE", "808 Drop + Sub Limiter",
    "[MODERN TECH HOUSE] Deep sub-bass impact with decaying white noise crash.",
    p => { p.waveA = 6; p.octA = -2; p.envAttack = 0.01; p.envDecay = 0.85; p.flangerEnable = false; });

  make(sweepTpl, "TECH HOUSE // Snare Roll Noise Sweep", "FX & TRANS", houseAuthor, "NOISE ONLY", "Bandpass Swept Noise + Delay",
    "[MODERN TECH HOUSE] Bandpass filtered noise sweep designed to sit behind drum build-ups.",
    p => { p.f1Type = 4; p.f1Cutoff = 0.60; p.f1Res = 0.55; });

  make(sweepTpl, "TECH HOUSE // Downlifter Crash FX", "FX & TRANS", houseAuthor, "NOISE ONLY", "Descending LP Noise + 2.5s Tail",
    "[MODERN TECH HOUSE] High-impact downlifter crash with long natural reverb tail.",
    p => { p.envAttack = 0.01; p.envRelease = 2.0; p.f1Cutoff = 0.42; });

  make(sweepTpl, "TECH HOUSE // Laser Zap Transition", "FX & TRANS", houseAuthor, "SYNC ONLY", "Fast Pitch Drop + Tube Sat",
    "[MODERN TECH HOUSE] Snappy analog laser zap transition with warm tube overdrive.",
    p => { p.noiseEnable = false; p.waveA = 7; p.envAttack = 0.01; p.envDecay = 0.15; p.distEnable = true; p.distDrive = 0.45; });

  make(sweepTpl, "TECH HOUSE // Vinyl Air Drop", "FX & TRANS", houseAuthor, "NOISE ONLY", "Vinyl Noise Burst + Sampler LP",
    "[MODERN TECH HOUSE] Vintage vinyl surface noise drop for subtle underground transitions.",
    p => { p.noiseType = 5; p.f1Type = 2; p.f1Cutoff = 0.50; });

  make(sweepTpl, "TECH HOUSE // Jet Wash Transition", "FX & TRANS", houseAuthor, "NOISE ONLY", "High-Reso Jet Flanger Sweep",
    "[MODERN TECH HOUSE] Jet aircraft engine swoosh effect with screaming flanger resonance.",
    p => { p.flangerDepth = 0.85; p.flangerFeedback = 0.75; });

  make(sweepTpl, "TECH HOUSE // Stutter Buffer Riser", "FX & TRANS", houseAuthor, "NOISE + SAW", "Time Mod 1/16 to 1/64 Stutter",
    "[MODERN TECH HOUSE] Modern buffer-chopped riser accelerating into the drop.",
    p => { p.timeModEnable = true; p.timeModMode = 1; p.timeModRate = 5; p.timeModMix = 0.80; });

  make(sweepTpl, "TECH HOUSE // Crowd Air Shaker FX", "FX & TRANS", houseAuthor, "NOISE ONLY", "Stereo Shaker Noise + Spread",
    "[MODERN TECH HOUSE] Stereo wide high-frequency noise shaker providing continuous dancefloor energy.",
    p => { p.f1Type = 3; p.f1Cutoff = 0.65; p.chorusEnable = true; p.chorusMix = 0.40; });

  // ===========================================================================
  // PACK 4: LO-FI & CHILLHOP (40 Patches)
  // ===========================================================================
  const lofiAuthor = "Prismor Lo-Fi";
  const chillAuthor = "Prismor Chillhop";

  // Archetype 13: Pitch-Drift SP-1200 Tape Keys (PLUCKS / KEYS) - 10 Variations
  const lofiKeysTpl = {
    waveA: 0, waveB: 1, octA: 0, octB: 0, voicingMode: 0, maxVoices: 8,
    unisonVoices: 2, unisonDetune: 0.15, unisonSpread: 0.60, unisonBlend: 0.50,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.52, f1Res: 0.15,
    crushEnable: true, crushMode: 2, crushBits: 9.5, crushDownsample: 3.5, crushMix: 0.45,
    distEnable: true, distType: 1, distDrive: 0.24, distMix: 0.35,
    reverbEnable: true, reverbSize: 0.65, reverbMix: 0.25,
    envAttack: 0.03, envDecay: 0.48, envSustain: 0.55, envRelease: 0.38,
    vibDepth: 0.22, vibSpeed: 4.8
  };

  make(lofiKeysTpl, "LO-FI // Nostalgic SP-1200 Piano", "PLUCKS", lofiAuthor, "SAW + SQR", "10-Bit Bitcrush + Pitch Drift",
    "[LO-FI & CHILLHOP] Nostalgic lo-fi hip-hop piano with 10-bit sampler decimation and warm tape drift.",
    p => { p.f1Cutoff = 0.54; });

  make(lofiKeysTpl, "LO-FI // Dusty Rhodes Electric Keys", "PLUCKS", lofiAuthor, "TRI + SINE", "Gentle Tremolo + Vintage Air",
    "[LO-FI & CHILLHOP] Warm vintage electric piano tines with subtle tremolo and mellow lowpass filter.",
    p => { p.waveA = 2; p.waveB = 6; p.f1Cutoff = 0.48; p.vibDepth = 0.28; p.vibSpeed = 5.2; });

  make(lofiKeysTpl, "LO-FI // Vinyl Wobble Melancholy", "PLUCKS", lofiAuthor, "SAW + TRI", "0.32 Pitch Vibrato + Sampler LP",
    "[LO-FI & CHILLHOP] Expressive pitch-wobbling keys reminiscent of warped vintage vinyl.",
    p => { p.waveB = 2; p.vibDepth = 0.32; p.vibSpeed = 4.4; p.crushBits = 8.5; });

  make(lofiKeysTpl, "LO-FI // 90s Boom Bap Chords", "PLUCKS", lofiAuthor, "ORG + SQR", "7-Bit Crunch + Plate Reverb",
    "[LO-FI & CHILLHOP] Gritty 90s golden era boom bap chord keys with snappy acoustic transient bite.",
    p => { p.waveA = 3; p.crushBits = 7.5; p.crushDownsample = 4.5; p.envAttack = 0.01; });

  make(lofiKeysTpl, "LO-FI // Rainy Afternoon Keys", "PLUCKS", lofiAuthor, "SAW + ORG", "Mellow Blend + Subtle Delay",
    "[LO-FI & CHILLHOP] Melancholic rainy day keyboard soundscape with warm tape echo repeats.",
    p => { p.waveB = 3; p.delayEnable = true; p.delayTime = 0.25; p.delayMix = 0.25; });

  make(lofiKeysTpl, "LO-FI // Cassette Tape Drift Piano", "PLUCKS", lofiAuthor, "TRI + SAW", "Tape Saturation Glue + Wow",
    "[LO-FI & CHILLHOP] Analog cassette tape emulation with warm third-harmonic saturation and pitch flutter.",
    p => { p.waveA = 2; p.distDrive = 0.35; p.vibDepth = 0.26; });

  make(lofiKeysTpl, "LO-FI // Midnight Study Beats", "PLUCKS", lofiAuthor, "TRI + TRI", "Warm Triangle + Brown Noise",
    "[LO-FI & CHILLHOP] Pure soothing triangle keys accompanied by gentle brownian vinyl noise.",
    p => { p.waveA = 2; p.waveB = 2; p.noiseEnable = true; p.noiseType = 2; p.noiseLevel = 0.15; p.f1Cutoff = 0.45; });

  make(lofiKeysTpl, "LO-FI // Sunny Sunday Upright", "PLUCKS", lofiAuthor, "SAW + TRI", "Felt Hammer Mallet + Room",
    "[LO-FI & CHILLHOP] Intimate felt upright piano sound with cozy acoustic room ambience.",
    p => { p.waveB = 2; p.envAttack = 0.015; p.envDecay = 0.42; p.reverbSize = 0.55; });

  make(lofiKeysTpl, "LO-FI // Coffee Shop Electric Tines", "PLUCKS", lofiAuthor, "SINE + ORG", "Dimension Chorus + 12dB LP",
    "[LO-FI & CHILLHOP] Smooth electric piano keys with wide stereo chorus for coffee shop beats.",
    p => { p.waveA = 6; p.waveB = 3; p.chorusEnable = true; p.chorusMix = 0.38; });

  make(lofiKeysTpl, "LO-FI // Memory Box Broken Toy", "PLUCKS", lofiAuthor, "SQR + BELL", "8-Bit Toy Crunch + Vibrato",
    "[LO-FI & CHILLHOP] Whimsical music box toy piano sound with nostalgic pitch drift.",
    p => { p.waveB = 8; p.crushBits = 8.0; p.vibDepth = 0.30; p.vibSpeed = 5.8; });

  // Archetype 14: Muted Low-Pass Vinyl Warm Chords (PADS / KEYS) - 10 Variations
  const lofiPadTpl = {
    waveA: 2, waveB: 0, octA: 0, octB: 0, voicingMode: 0, maxVoices: 8,
    unisonVoices: 6, unisonDetune: 0.18, unisonSpread: 0.75, unisonBlend: 0.55,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.42, f1Res: 0.12,
    noiseEnable: true, noiseType: 5, noiseLevel: 0.14,
    distEnable: true, distType: 1, distDrive: 0.22, distMix: 0.30,
    reverbEnable: true, reverbSize: 0.70, reverbMix: 0.28,
    envAttack: 0.35, envDecay: 0.65, envSustain: 0.75, envRelease: 0.55
  };

  make(lofiPadTpl, "LO-FI // Muted Sunset Velvet Pad", "PADS", lofiAuthor, "TRI + SAW", "Warm LP 0.42 + Vinyl Noise",
    "[LO-FI & CHILLHOP] Velvet smooth low-pass pad layered with vinyl crackle and analog warmth.",
    p => { p.f1Cutoff = 0.42; });

  make(lofiPadTpl, "LO-FI // Autumn Leaves Chill Pad", "PADS", lofiAuthor, "SAW + ORG", "Soft Attack 0.35s + Plate",
    "[LO-FI & CHILLHOP] Gentle chord pad with nostalgic autumn vibes and intimate plate reverb.",
    p => { p.waveA = 0; p.waveB = 3; p.reverbSize = 0.75; });

  make(lofiPadTpl, "LO-FI // Smoke & Espresso Chords", "PADS", lofiAuthor, "TRI + ORG", "Slow Phaser Drift + Muted",
    "[LO-FI & CHILLHOP] Dark, rich chord pad with slow stereo phaser movement and warm mids.",
    p => { p.waveB = 3; p.phaserEnable = true; p.phaserMix = 0.35; p.f1Cutoff = 0.38; });

  make(lofiPadTpl, "LO-FI // Vintage Mellotron Strings", "PADS", lofiAuthor, "FMT + SAW", "Formant Strings + Tape Flutter",
    "[LO-FI & CHILLHOP] Authentic 70s tape-replay Mellotron string section with subtle wow & flutter.",
    p => { p.waveA = 4; p.f1Type = 6; p.vibDepth = 0.22; p.vibSpeed = 5.0; });

  make(lofiPadTpl, "LO-FI // Wool Sweater Warmth", "PADS", lofiAuthor, "SAW + SAW", "Dark Filtered 8x Unison + Tape",
    "[LO-FI & CHILLHOP] Cozy, warm analog blanket pad with 8 detuned voices and gentle tape saturation.",
    p => { p.waveA = 0; p.unisonVoices = 8; p.unisonDetune = 0.22; p.f1Cutoff = 0.36; });

  make(lofiPadTpl, "LO-FI // Bedroom Producer Glow", "PADS", lofiAuthor, "TRI + SINE", "Muted LP + 1/4 Delay + Room",
    "[LO-FI & CHILLHOP] Intimate bedroom beatmaker pad with quarter-note stereo echo repeats.",
    p => { p.waveB = 6; p.delayEnable = true; p.delayTime = 0.25; p.delayMix = 0.25; });

  make(lofiPadTpl, "LO-FI // Melancholy Dreams Pad", "PADS", lofiAuthor, "SAW + TRI", "Slow Swell + Nostalgic Vibrato",
    "[LO-FI & CHILLHOP] Emotional dream pad with 0.6s swell and nostalgic 4.5Hz pitch vibrato.",
    p => { p.envAttack = 0.60; p.vibDepth = 0.24; p.vibSpeed = 4.5; });

  make(lofiPadTpl, "LO-FI // Cassette Hiss Ambiance", "PADS", lofiAuthor, "TRI + SAW", "Pink Noise Layer + Saturated Mids",
    "[LO-FI & CHILLHOP] Analog cassette hiss texture wrapped around soft low-frequency chords.",
    p => { p.noiseType = 1; p.noiseLevel = 0.22; p.distDrive = 0.28; });

  make(lofiPadTpl, "LO-FI // Cozy Midnight Blanket", "PADS", lofiAuthor, "SAW + SQR", "Sub-Octave Warmth + Soft Chorus",
    "[LO-FI & CHILLHOP] Deep midnight chord pad with sub-octave layer and lush stereo chorus.",
    p => { p.octB = -1; p.chorusEnable = true; p.chorusMix = 0.35; p.f1Cutoff = 0.40; });

  make(lofiPadTpl, "LO-FI // Golden Hour Skyline", "PADS", lofiAuthor, "TRI + ORG", "Evolving LP Sweep + Shimmer",
    "[LO-FI & CHILLHOP] Uplifting sunset pad with gentle lowpass filter sweep and subtle shimmer.",
    p => { p.waveB = 3; p.reverbShimmer = 0.30; p.f1Cutoff = 0.46; });

  // Archetype 15: Half-Time Gross Beat Stutter Keys (ARP & SEQ / KEYS) - 10 Variations
  const lofiTimeTpl = {
    waveA: 0, waveB: 3, octA: 0, octB: 0, voicingMode: 0, maxVoices: 8,
    timeModEnable: true, timeModMode: 4, timeModRate: 1, timeModDepth: 0.80, timeModMix: 0.75,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.55, f1Res: 0.15,
    delayEnable: true, delayTime: 0.35, delayFeedback: 0.40, delayMix: 0.25,
    reverbEnable: true, reverbSize: 0.75, reverbMix: 0.30,
    envAttack: 0.04, envDecay: 0.50, envSustain: 0.65, envRelease: 0.45
  };

  make(lofiTimeTpl, "CHILLHOP // Halftime Granular Keys", "ARP & SEQ", chillAuthor, "SAW + ORG", "Time Mod 1/2x Halftime",
    "[LO-FI & CHILLHOP] Signature modern trap & chillhop half-time time-stretched melodic keys.",
    p => { p.f1Cutoff = 0.52; });

  make(lofiTimeTpl, "CHILLHOP // Lazy Sunday Stutter", "ARP & SEQ", chillAuthor, "TRI + SQR", "Time Mod 1/8 Stutter + Tube",
    "[LO-FI & CHILLHOP] Lazy Sunday morning keys with automated rhythmic 1/8 buffer stutter chops.",
    p => { p.waveA = 2; p.waveB = 1; p.timeModMode = 1; p.timeModRate = 4; p.distEnable = true; });

  make(lofiTimeTpl, "CHILLHOP // Tape Reverse Chord Flow", "ARP & SEQ", chillAuthor, "SAW + SINE", "Gross Beat Reverse Warp",
    "[LO-FI & CHILLHOP] Ethereal backward-flowing chord swell created with circular buffer tape reverse.",
    p => { p.waveB = 6; p.timeModMode = 3; p.reverbSize = 0.85; });

  make(lofiTimeTpl, "CHILLHOP // Slow Motion Lo-Fi Hop", "ARP & SEQ", chillAuthor, "ORG + TRI", "1/2x Halftime + 9-Bit Crush",
    "[LO-FI & CHILLHOP] Decimated half-time sampler keys with warm upward compressor glue.",
    p => { p.waveA = 3; p.waveB = 2; p.crushEnable = true; p.crushBits = 9.0; p.compEnable = true; });

  make(lofiTimeTpl, "CHILLHOP // Beat Tape Turntable Brake", "ARP & SEQ", chillAuthor, "SAW + SQR", "Time Mod Tape Stop + Vinyl",
    "[LO-FI & CHILLHOP] Turntable vinyl brake simulation on sustained notes with record crackle.",
    p => { p.timeModMode = 0; p.noiseEnable = true; p.noiseType = 5; p.noiseLevel = 0.20; });

  make(lofiTimeTpl, "CHILLHOP // Nostalgic Triplet Stumble", "ARP & SEQ", chillAuthor, "TRI + ORG", "Gross Beat Triplet Groove",
    "[LO-FI & CHILLHOP] Dotted triplet buffer warping for unquantized, stumbling beat tape grooves.",
    p => { p.waveA = 2; p.timeModMode = 2; p.delayMix = 0.30; });

  make(lofiTimeTpl, "CHILLHOP // Lo-Fi Dreamcatcher Arp", "ARP & SEQ", chillAuthor, "BELL + TRI", "1/8 Slow Arp + Flutter",
    "[LO-FI & CHILLHOP] Hypnotic slow bell arpeggio with subtle pitch wow and analog tape flutter.",
    p => { p.timeModEnable = false; p.arpEnable = true; p.arpRate = 2; p.vibDepth = 0.25; });

  make(lofiTimeTpl, "CHILLHOP // Midnight Beat Sequence", "ARP & SEQ", chillAuthor, "SQR + SINE", "Snappy Key Seq + 1/16 Gate",
    "[LO-FI & CHILLHOP] Snappy percussive key sequence locked to sixteen-step groove.",
    p => { p.timeModEnable = false; p.arpEnable = true; p.arpRate = 3; p.arpGate = 0.55; });

  make(lofiTimeTpl, "CHILLHOP // Retro Walkman Granular", "ARP & SEQ", chillAuthor, "SAW + ORG", "1/2x Stretch + Cassette Sat",
    "[LO-FI & CHILLHOP] Vintage portable cassette player playback with warm saturation and half-time warble.",
    p => { p.distEnable = true; p.distType = 1; p.distDrive = 0.30; });

  make(lofiTimeTpl, "CHILLHOP // Drifting Through Clouds", "ARP & SEQ", chillAuthor, "TRI + SINE", "Reverse Halftime Pad + Shimmer",
    "[LO-FI & CHILLHOP] Dreamy atmospheric reverse wash drifting into shimmering cosmic space.",
    p => { p.waveA = 2; p.waveB = 6; p.reverbShimmer = 0.45; p.timeModMode = 3; });

  // Archetype 16: Deep Sub Foundation & Dust Pluck (BASS / PLUCKS) - 10 Variations
  const lofiSubPluckTpl = {
    waveA: 6, waveB: 2, octA: -2, octB: -2, voicingMode: 1,
    unisonVoices: 1, unisonDetune: 0.15, unisonSpread: 0.50, unisonBlend: 0.50,
    filter1Enable: true, filter1Type: 1, f1Cutoff: 0.42, f1Res: 0.10,
    distEnable: true, distType: 0, distDrive: 0.20, distMix: 0.30,
    envAttack: 0.015, envDecay: 0.45, envSustain: 0.0, envRelease: 0.28,
    masterGainDb: 0.0
  };

  make(lofiSubPluckTpl, "LO-FI // Warm Muted Upright Bass", "BASS", lofiAuthor, "TRI + SINE", "Acoustic Felt Body + Tube",
    "[LO-FI & CHILLHOP] Muted acoustic upright bass with warm tube body and rounded fundamental.",
    p => { p.waveA = 2; p.waveB = 6; p.octA = -1; p.octB = -2; p.f1Cutoff = 0.45; });

  make(lofiSubPluckTpl, "LO-FI // Vinyl Dust 808 Sub", "BASS", lofiAuthor, "SINE ONLY", "Pure Sub Sine + Vinyl Dust",
    "[LO-FI & CHILLHOP] Pristine sub fundamental layered with authentic vintage vinyl noise crackle.",
    p => { p.waveB = 6; p.noiseEnable = true; p.noiseType = 5; p.noiseLevel = 0.18; p.f1Cutoff = 0.38; });

  make(lofiSubPluckTpl, "LO-FI // Boom Bap Fingered Bass", "BASS", lofiAuthor, "TRI + SAW", "Crude LP 0.45 + Tape Comp",
    "[LO-FI & CHILLHOP] Organic fingered electric bass pluck for classic 90s hip-hop beats.",
    p => { p.waveA = 2; p.waveB = 0; p.octA = -1; p.octB = -1; p.f1Type = 2; p.f1Cutoff = 0.45; p.compEnable = true; });

  make(lofiSubPluckTpl, "LO-FI // Mellow Electric Bass Pluck", "BASS", lofiAuthor, "SAW + SINE", "Warm Round Pluck + Slapback",
    "[LO-FI & CHILLHOP] Warm round bass pluck with gentle tape slapback echo.",
    p => { p.waveA = 0; p.octA = -1; p.delayEnable = true; p.delayTime = 0.12; p.delayMix = 0.20; });

  make(lofiSubPluckTpl, "LO-FI // Subterranean Chill Sub", "BASS", lofiAuthor, "SINE + X-MOD", "Clean Sub + SP-1200 Crunch",
    "[LO-FI & CHILLHOP] Deep subterranean sub reinforced with 12-bit sampler crunch in the upper register.",
    p => { p.waveB = 9; p.crushEnable = true; p.crushBits = 8.5; p.crushMix = 0.35; });

  make(lofiSubPluckTpl, "LO-FI // SP-1200 Crunch Bassline", "BASS", lofiAuthor, "SQR + TRI", "7.5-Bit Decimation + Warm Tube",
    "[LO-FI & CHILLHOP] Gritty vintage 12-bit sampler bassline emulation with warm low-end weight.",
    p => { p.waveA = 1; p.octA = -1; p.crushEnable = true; p.crushBits = 7.5; p.crushDownsample = 4.0; p.crushMix = 0.50; });

  make(lofiSubPluckTpl, "LO-FI // Jazzy Walkin' Sub Pluck", "BASS", lofiAuthor, "TRI + SINE", "Snappy Attack + Room Ambience",
    "[LO-FI & CHILLHOP] Snappy jazz walking bass pluck with cozy acoustic room reflections.",
    p => { p.waveA = 2; p.envDecay = 0.35; p.reverbEnable = true; p.reverbSize = 0.45; p.reverbMix = 0.18; });

  make(lofiSubPluckTpl, "LO-FI // Deep Sleep Sub Foundation", "BASS", lofiAuthor, "SINE ONLY", "Pure Sub Sine + Zero Saturation",
    "[LO-FI & CHILLHOP] Perfectly smooth sub bass anchor for ambient and chill beats with zero distortion.",
    p => { p.waveB = 6; p.distEnable = false; p.f1Cutoff = 0.35; });

  make(lofiSubPluckTpl, "LO-FI // Acoustic Felt Piano Pluck", "PLUCKS", lofiAuthor, "TRI + BELL", "Felt Hammer Transient + Body",
    "[LO-FI & CHILLHOP] Snappy acoustic felt hammer transient for delicate chillhop melodies.",
    p => { p.waveA = 2; p.waveB = 8; p.octA = 0; p.octB = 0; p.envAttack = 0.01; p.envDecay = 0.30; p.f1Cutoff = 0.65; });

  make(lofiSubPluckTpl, "LO-FI // Nostalgic Music Box", "PLUCKS", lofiAuthor, "BELL + SQR", "Snappy High Bell + Flutter",
    "[LO-FI & CHILLHOP] Delicate vintage music box chime with gentle pitch flutter and tape saturation.",
    p => { p.waveA = 8; p.waveB = 1; p.octA = 1; p.octB = 1; p.envAttack = 0.008; p.envDecay = 0.38; p.vibDepth = 0.28; });

  return newPresets;
}

// -----------------------------------------------------------------------------
// 4. Export Standalone .prismor XML Files
// -----------------------------------------------------------------------------
function exportPrismorPresetFiles(presets, baseDir) {
  const packs = {
    "Cyberpunk_Synthwave": presets.filter(p => p.description.includes("CYBERPUNK & SYNTHWAVE")),
    "Cinematic_Ambient":   presets.filter(p => p.description.includes("CINEMATIC & AMBIENT")),
    "Modern_Tech_House":   presets.filter(p => p.description.includes("MODERN TECH HOUSE")),
    "LoFi_Chillhop":       presets.filter(p => p.description.includes("LO-FI & CHILLHOP"))
  };

  let totalFiles = 0;
  for (const [packName, packPresets] of Object.entries(packs)) {
    const dir = path.join(baseDir, packName);
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }

    for (const p of packPresets) {
      const safeName = p.name.replace(/[/\\?%*:|"<>]/g, '_');
      const xml = `<?xml version="1.0" encoding="UTF-8"?>
<PRISMOR_PRESET version="1.0.12.4" presetName="${p.name}" category="${p.category}" author="${p.author}" description="${p.description}">
  <PARAM id="oscAOct" value="${p.octA}"/>
  <PARAM id="oscASemi" value="${p.semiA}"/>
  <PARAM id="oscAFine" value="${p.fineA.toFixed(1)}"/>
  <PARAM id="timbre" value="${p.timbreA.toFixed(2)}"/>
  <PARAM id="warpMode" value="${p.warpModeA}"/>
  <PARAM id="warpAmount" value="${p.warpAmtA.toFixed(2)}"/>
  <PARAM id="harmonicScale" value="${p.harmScaleA.toFixed(2)}"/>
  <PARAM id="harmonicTilt" value="${p.harmTiltA.toFixed(2)}"/>
  <PARAM id="oscBOct" value="${p.octB}"/>
  <PARAM id="oscBSemi" value="${p.semiB}"/>
  <PARAM id="oscBFine" value="${p.fineB.toFixed(1)}"/>
  <PARAM id="timbreB" value="${p.timbreB.toFixed(2)}"/>
  <PARAM id="warpModeB" value="${p.warpModeB}"/>
  <PARAM id="warpAmountB" value="${p.warpAmtB.toFixed(2)}"/>
  <PARAM id="harmonicScaleB" value="${p.harmScaleB.toFixed(2)}"/>
  <PARAM id="harmonicTiltB" value="${p.harmTiltB.toFixed(2)}"/>
  <PARAM id="voicingMode" value="${p.voicingMode}"/>
  <PARAM id="maxVoices" value="${p.maxVoices}"/>
  <PARAM id="portaTime" value="${p.portaTime.toFixed(2)}"/>
  <PARAM id="unisonVoices" value="${p.unisonVoices}"/>
  <PARAM id="unisonDetune" value="${p.unisonDetune.toFixed(2)}"/>
  <PARAM id="unisonSpread" value="${p.unisonSpread.toFixed(2)}"/>
  <PARAM id="unisonBlend" value="${p.unisonBlend.toFixed(2)}"/>
  <PARAM id="unisonStackMode" value="${p.unisonStack}"/>
  <PARAM id="filter1Enable" value="${p.filter1Enable ? 1.0 : 0.0}"/>
  <PARAM id="filter1Type" value="${p.filter1Type}"/>
  <PARAM id="filter1Cutoff" value="${p.f1Cutoff.toFixed(2)}"/>
  <PARAM id="filter1Res" value="${p.f1Res.toFixed(2)}"/>
  <PARAM id="distEnable" value="${p.distEnable ? 1.0 : 0.0}"/>
  <PARAM id="distType" value="${p.distType}"/>
  <PARAM id="distDrive" value="${p.distDrive.toFixed(2)}"/>
  <PARAM id="distMix" value="${p.distMix.toFixed(2)}"/>
  <PARAM id="chorusEnable" value="${p.chorusEnable ? 1.0 : 0.0}"/>
  <PARAM id="chorusMix" value="${p.chorusMix.toFixed(2)}"/>
  <PARAM id="delayEnable" value="${p.delayEnable ? 1.0 : 0.0}"/>
  <PARAM id="delayMix" value="${p.delayMix.toFixed(2)}"/>
  <PARAM id="reverbEnable" value="${p.reverbEnable ? 1.0 : 0.0}"/>
  <PARAM id="reverbMix" value="${p.reverbMix.toFixed(2)}"/>
  <PARAM id="compEnable" value="${p.compEnable ? 1.0 : 0.0}"/>
  <PARAM id="compMix" value="${p.compMix.toFixed(2)}"/>
  <PARAM id="crushEnable" value="${p.crushEnable ? 1.0 : 0.0}"/>
  <PARAM id="crushMix" value="${p.crushMix.toFixed(2)}"/>
  <PARAM id="flangerEnable" value="${p.flangerEnable ? 1.0 : 0.0}"/>
  <PARAM id="flangerMix" value="${p.flangerMix.toFixed(2)}"/>
  <PARAM id="timeModEnable" value="${p.timeModEnable ? 1.0 : 0.0}"/>
  <PARAM id="timeModMix" value="${p.timeModMix.toFixed(2)}"/>
  <PARAM id="envAttack" value="${p.envAttack.toFixed(2)}"/>
  <PARAM id="envDecay" value="${p.envDecay.toFixed(2)}"/>
  <PARAM id="envSustain" value="${p.envSustain.toFixed(2)}"/>
  <PARAM id="envRelease" value="${p.envRelease.toFixed(2)}"/>
  <PARAM id="masterGainDb" value="${p.masterGainDb.toFixed(1)}"/>
  <PARAM id="limiterEnable" value="1.0"/>
  <PARAM id="limiterThreshDb" value="-0.5"/>
</PRISMOR_PRESET>
`;
      fs.writeFileSync(path.join(dir, safeName + ".prismor"), xml, 'utf8');
      totalFiles++;
    }
  }
  return totalFiles;
}

module.exports = {
  generateProceduralExpansionPacks,
  exportPrismorPresetFiles
};
