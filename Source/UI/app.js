(() => {
  "use strict";

  const PARAM_META = {
    oscAOct: { min: -4, max: 4, def: 0, step: 1, fmt: "int" },
    oscASemi: { min: -12, max: 12, def: 0, step: 1, fmt: "int" },
    oscAFine: { min: -100, max: 100, def: 0, step: 0.5, bipolar: true, fmt: "cents" },
    oscACoarse: { min: -24, max: 24, def: 0, step: 0.01, bipolar: true, fmt: "st" },
    startPhase: { min: 0, max: 1, def: 0, fmt: "pct" },
    phaseRandom: { min: 0, max: 1, def: 0, fmt: "pct" },
    timbre: { min: 0, max: 1, def: 0, fmt: "pct" },
    warpMode: { min: 0, max: 5, def: 0, opts: ["Off", "Bend +/-", "Mirror", "Quantize", "Asymmetric", "Sync Fold"] },
    warpAmount: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    harmonicScale: { min: 0.25, max: 4.0, def: 1.0, fmt: "scale" },
    harmonicTilt: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },

    oscBOct: { min: -4, max: 4, def: 0, step: 1, fmt: "int" },
    oscBSemi: { min: -12, max: 12, def: 0, step: 1, fmt: "int" },
    oscBFine: { min: -100, max: 100, def: 0, step: 0.5, bipolar: true, fmt: "cents" },
    oscBPitch: { min: -24, max: 24, def: 0, step: 0.01, bipolar: true, fmt: "st" },
    oscBPhase: { min: 0, max: 1, def: 0, fmt: "pct" },
    timbreB: { min: 0, max: 1, def: 0, fmt: "pct" },
    warpModeB: { min: 0, max: 5, def: 0, opts: ["Off", "Bend +/-", "Mirror", "Quantize", "Asymmetric", "Sync Fold"] },
    warpAmountB: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    harmonicScaleB: { min: 0.25, max: 4.0, def: 1.0, fmt: "scale" },
    harmonicTiltB: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },

    prismEnable: { min: 0, max: 1, def: 1 },
    prismAmount: { min: 0, max: 1, def: 0, fmt: "pct" },
    prismMode: { min: 0, max: 3, def: 0, opts: ["Inharmonic", "Hz Shift", "Odd/Even", "Quantum"] },
    harmShift: { min: 0, max: 1, def: 0, fmt: "pct" },
    harmGap: { min: 0, max: 1, def: 0.25, fmt: "pct" },
    harmWidth: { min: 0, max: 1, def: 0.45, fmt: "pct" },

    pluckEnable: { min: 0, max: 1, def: 1 },
    pluckTime: { min: 0, max: 1, def: 1, fmt: "pct" },
    pluckBlur: { min: 0, max: 1, def: 0.3, fmt: "pct" },

    voicingMode: { min: 0, max: 2, def: 0, opts: ["Polyphonic", "Monophonic", "True Legato"] },
    maxVoices: { min: 1, max: 16, def: 8, step: 1, fmt: "int" },
    pitchFreq: { min: -24, max: 24, def: 0, bipolar: true, fmt: "st" },
    portaTime: { min: 0, max: 1, def: 0, fmt: "pct" },
    portaCurve: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    autoGlide: { min: 0, max: 1, def: 1 },

    unisonVoices: { min: 1, max: 16, def: 1, step: 1, fmt: "int" },
    unisonDetune: { min: 0, max: 1, def: 0.18, fmt: "pct" },
    unisonSpread: { min: 0, max: 1, def: 0.65, fmt: "pct" },
    unisonBlend: { min: 0, max: 1, def: 0.55, fmt: "pct" },
    unisonPhase: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    unisonStackMode: { min: 0, max: 7, def: 0, opts: ["Normal 1x", "+1 Octave", "+2 Octaves", "Sub + Oct", "Fifths", "Oct + 5th", "Minor Triad", "Major Triad"] },
    vibDepth: { min: 0, max: 1, def: 0, fmt: "pct" },
    vibSpeed: { min: 0.5, max: 15, def: 5.5, fmt: "hz" },

    filter1Enable: { min: 0, max: 1, def: 1 },
    filter1Type: { min: 0, max: 9, def: 1, opts: ["SVF LP 12", "SVF LP 24", "Crude LP", "SVF HP 24", "Bandpass", "Comb", "Vox Formant", "Custom", "Vocoder", "Talkbox"] },
    filter1Cutoff: { min: 0, max: 1, def: 1, fmt: "pct" },
    filter1Res: { min: 0, max: 1, def: 0.1, fmt: "pct" },
    filter1KeyTrack: { min: 0, max: 1, def: 0.5, fmt: "pct" },
    filter1Width: { min: 0, max: 1, def: 0.45, fmt: "pct" },

    filter2Enable: { min: 0, max: 1, def: 0 },
    filter2Type: { min: 0, max: 9, def: 2, opts: ["SVF LP 12", "SVF LP 24", "Crude LP", "SVF HP 24", "Bandpass", "Comb", "Vox Formant", "Custom", "Vocoder", "Talkbox"] },
    filter2Cutoff: { min: 0, max: 1, def: 0.85, fmt: "pct" },
    filter2Res: { min: 0, max: 1, def: 0.2, fmt: "pct" },
    filter2KeyTrack: { min: 0, max: 1, def: 0.5, fmt: "pct" },
    filter2Width: { min: 0, max: 1, def: 0.5, fmt: "pct" },

    filterRouting: { min: 0, max: 1, def: 0, fmt: "pct" },
    filterDrive: { min: 0, max: 1, def: 0, fmt: "pct" },

    noiseEnable: { min: 0, max: 1, def: 0 },
    noiseType: { min: 0, max: 5, def: 0, opts: ["White", "Pink", "Brown", "Vinyl", "Bitnoise", "Breath"] },
    noiseLevel: { min: 0, max: 1, def: 0.25, fmt: "pct" },
    noisePitch: { min: 0, max: 1, def: 0.5, fmt: "pct" },
    noiseKeyTrack: { min: 0, max: 1, def: 0.5, fmt: "pct" },
    noisePhase: { min: 0, max: 1, def: 0.25, fmt: "pct" },
    noisePan: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    noiseFilter: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },

    arpEnable: { min: 0, max: 1, def: 0 },
    arpMode: { min: 0, max: 5, def: 0, opts: ["Up", "Down", "Up/Down", "Random", "Chord", "Step Seq"] },
    arpOctaves: { min: 1, max: 4, def: 1, step: 1, fmt: "int" },
    arpRate: { min: 0, max: 5, def: 3, opts: ["1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32"] },
    arpGate: { min: 0.05, max: 1, def: 0.75, fmt: "pct" },
    arpSwing: { min: 0, max: 0.75, def: 0, fmt: "pct" },

    blurEnable: { min: 0, max: 1, def: 1 },
    blurTime: { min: 0, max: 1, def: 0, fmt: "pct" },
    blurHarmonic: { min: 0, max: 1, def: 0, fmt: "pct" },

    tremEnable: { min: 0, max: 1, def: 1 },
    tremDepth: { min: 0, max: 1, def: 0, fmt: "pct" },
    tremSpeed: { min: 0.1, max: 15, def: 3.5, fmt: "hz" },
    tremGap: { min: 0, max: 1, def: 0.35, fmt: "pct" },

    distEnable: { min: 0, max: 1, def: 0 },
    distType: { min: 0, max: 5, def: 0, opts: ["Tube", "Tape", "Hard Clip", "Wavefold", "Bitcrush Fuzz", "Asym Scream"] },
    distDrive: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    distTone: { min: 0, max: 1, def: 0.65, fmt: "pct" },
    distMix: { min: 0, max: 1, def: 0.5, fmt: "pct" },

    phaserEnable: { min: 0, max: 1, def: 1 },
    phaserMix: { min: 0, max: 1, def: 0, fmt: "pct" },
    phaserWidth: { min: 0, max: 1, def: 0.5, fmt: "pct" },
    phaserOffset: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    phaserSpeed: { min: 0.05, max: 10, def: 1.2, fmt: "hz" },

    chorusEnable: { min: 0, max: 1, def: 0 },
    chorusVoices: { min: 2, max: 6, def: 4, step: 1, fmt: "int" },
    chorusRate: { min: 0.05, max: 5, def: 0.85, fmt: "hz" },
    chorusDepth: { min: 0, max: 1, def: 0.45, fmt: "pct" },
    chorusSpread: { min: 0, max: 1, def: 0.8, fmt: "pct" },
    chorusMix: { min: 0, max: 1, def: 0.35, fmt: "pct" },

    eqEnable: { min: 0, max: 1, def: 1 },
    eqLowFreq: { min: 30, max: 400, def: 110, fmt: "hz" },
    eqLowGain: { min: -15, max: 15, def: 0, bipolar: true, fmt: "db" },
    eqMidFreq: { min: 200, max: 6000, def: 1200, fmt: "hz" },
    eqMidGain: { min: -15, max: 15, def: 0, bipolar: true, fmt: "db" },
    eqMidQ: { min: 0.3, max: 8, def: 1.2, fmt: "scale" },
    eqHighFreq: { min: 1500, max: 16000, def: 5500, fmt: "hz" },
    eqHighGain: { min: -15, max: 15, def: 0, bipolar: true, fmt: "db" },

    delayEnable: { min: 0, max: 1, def: 0 },
    delayTime: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    delayFeedback: { min: 0, max: 1, def: 0.4, fmt: "pct" },
    delayPingPong: { min: 0, max: 1, def: 0.65, fmt: "pct" },
    delayFilter: { min: 0, max: 1, def: 0.6, fmt: "pct" },
    delayMix: { min: 0, max: 1, def: 0.28, fmt: "pct" },

    reverbEnable: { min: 0, max: 1, def: 0 },
    reverbSize: { min: 0, max: 1, def: 0.68, fmt: "pct" },
    reverbDecay: { min: 0, max: 1, def: 0.55, fmt: "pct" },
    reverbPreDelay: { min: 0, max: 1, def: 0.18, fmt: "pct" },
    reverbDamping: { min: 0, max: 1, def: 0.4, fmt: "pct" },
    reverbShimmer: { min: 0, max: 1, def: 0.25, fmt: "pct" },
    reverbMix: { min: 0, max: 1, def: 0.25, fmt: "pct" },

    compEnable: { min: 0, max: 1, def: 0 },
    compMode: { min: 0, max: 3, def: 1, opts: ["Single VCA", "Multiband OTT", "Opto Smooth", "Punch Limiter"] },
    compThreshold: { min: -48, max: 0, def: -18, fmt: "db" },
    compRatio: { min: 1, max: 20, def: 4, fmt: "scale" },
    compAttack: { min: 0.1, max: 100, def: 10, fmt: "ms" },
    compRelease: { min: 10, max: 1000, def: 120, fmt: "ms" },
    compMakeup: { min: 0, max: 24, def: 3, fmt: "db" },
    compMix: { min: 0, max: 1, def: 0.75, fmt: "pct" },

    ringEnable: { min: 0, max: 1, def: 0 },
    ringMode: { min: 0, max: 3, def: 0, opts: ["Ring Mod", "Freq Shifter", "Auto-Pan", "Rotary"] },
    ringFreq: { min: 20, max: 2000, def: 220, fmt: "hz" },
    ringDepth: { min: 0, max: 1, def: 0.7, fmt: "pct" },
    ringLfoRate: { min: 0.05, max: 15, def: 1.2, fmt: "hz" },
    ringLfoDepth: { min: 0, max: 1, def: 0.25, fmt: "pct" },
    ringMix: { min: 0, max: 1, def: 0.5, fmt: "pct" },

    crushEnable: { min: 0, max: 1, def: 0 },
    crushMode: { min: 0, max: 3, def: 0, opts: ["Digital Crush", "Sample Decimate", "SP-1200", "Radio Lo-Fi"] },
    crushBits: { min: 1, max: 16, def: 8, fmt: "scale" },
    crushDownsample: { min: 1, max: 64, def: 4, fmt: "scale" },
    crushDrive: { min: 0, max: 1, def: 0.25, fmt: "pct" },
    crushTone: { min: 0, max: 1, def: 0.65, fmt: "pct" },
    crushMix: { min: 0, max: 1, def: 0.55, fmt: "pct" },

    flangerEnable: { min: 0, max: 1, def: 0 },
    flangerMode: { min: 0, max: 3, def: 0, opts: ["Jet Flanger", "Thru-Zero", "Barberpole", "Step Flanger"] },
    flangerRate: { min: 0.05, max: 8, def: 0.45, fmt: "hz" },
    flangerDepth: { min: 0, max: 1, def: 0.65, fmt: "pct" },
    flangerFeedback: { min: -0.95, max: 0.95, def: 0.55, bipolar: true, fmt: "pct" },
    flangerManual: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    flangerMix: { min: 0, max: 1, def: 0.5, fmt: "pct" },

    bodeEnable: { min: 0, max: 1, def: 0 },
    bodeShift: { min: -5000, max: 5000, def: 0, step: 1, bipolar: true, fmt: "hz" },
    bodeFeedback: { min: -0.88, max: 0.88, def: 0, bipolar: true, fmt: "pct" },
    bodeMix: { min: 0, max: 1, def: 0.5, fmt: "pct" },

    timeModEnable: { min: 0, max: 1, def: 0 },
    timeModMode: { min: 0, max: 5, def: 0, opts: ["Time Warp", "Stutter", "Tape Stop", "Reverse", "Half-Time", "DJ Scratch"] },
    timeModRate: { min: 0, max: 5, def: 1, opts: ["2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16"] },
    timeModDepth: { min: 0, max: 1, def: 0.75, fmt: "pct" },
    timeModStutter: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    timeModSpeed: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    timeModSmooth: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    timeModFilter: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    timeModMix: { min: 0, max: 1, def: 0.85, fmt: "pct" },

    envAttack: { min: 0, max: 1, def: 0.05, fmt: "pct" },
    envHold: { min: 0, max: 1, def: 0.05, fmt: "pct" },
    envDecay: { min: 0, max: 1, def: 0.35, fmt: "pct" },
    envSustain: { min: 0, max: 1, def: 0.72, fmt: "pct" },
    envRelease: { min: 0, max: 1, def: 0.25, fmt: "pct" },

    abMix: { min: 0, max: 1, def: 0, fmt: "pct" },
    velLink: { min: 0, max: 1, def: 0.65, fmt: "pct" },
    partPan: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    strumTime: { min: 0, max: 1, def: 0, fmt: "pct" },
    strumTension: { min: -1, max: 1, def: 0, bipolar: true, fmt: "pct" },
    masterGainDb: { min: -24, max: 12, def: 0, bipolar: true, fmt: "db" },
    limiterEnable: { min: 0, max: 1, def: 1 },
    limiterThreshDb: { min: -12, max: 0, def: -0.5, fmt: "db" }
  };

  const FX_MODULES = [
    { id: 0, short: "BLUR", title: "SPECTRAL BLUR & PLUCK", accent: "amber", toggle: "blurEnable",
      knobs: [["blurTime", "TIME", "amber"], ["blurHarmonic", "HARM", "amber"]] },
    { id: 1, short: "TREM", title: "HARMONIC TREMOLO", accent: "cyan", toggle: "tremEnable",
      knobs: [["tremDepth", "DEPTH", "cyan"], ["tremSpeed", "SPEED", "cyan"], ["tremGap", "GAP", "amber"]] },
    { id: 2, short: "DIST", title: "DISTORTION / DRIVE", accent: "magenta", toggle: "distEnable", select: "distType",
      knobs: [["distDrive", "DRIVE", "magenta"], ["distTone", "TONE", "amber"], ["distMix", "MIX", "cyan"]] },
    { id: 3, short: "PHASE", title: "SPECTRAL PHASER", accent: "magenta", toggle: "phaserEnable",
      knobs: [["phaserMix", "MIX", "magenta"], ["phaserWidth", "WIDTH", "cyan"], ["phaserOffset", "OFFST", "amber"], ["phaserSpeed", "SPEED", "magenta"]] },
    { id: 4, short: "CHORUS", title: "MULTI-VOICE CHORUS", accent: "cyan", toggle: "chorusEnable",
      knobs: [["chorusVoices", "VOICES", "cyan"], ["chorusRate", "RATE", "cyan"], ["chorusDepth", "DEPTH", "cyan"], ["chorusSpread", "SPRD", "amber"], ["chorusMix", "MIX", "cyan"]] },
    { id: 5, short: "EQ", title: "3-BAND PARAMETRIC EQ", accent: "amber", toggle: "eqEnable",
      knobs: [["eqLowFreq", "LOW F", "amber"], ["eqLowGain", "LOW G", "amber"], ["eqMidFreq", "MID F", "cyan"], ["eqMidGain", "MID G", "cyan"], ["eqMidQ", "MID Q", "magenta"], ["eqHighFreq", "HI F", "emerald"], ["eqHighGain", "HI G", "emerald"]] },
    { id: 6, short: "DELAY", title: "PING-PONG DELAY", accent: "amber", toggle: "delayEnable",
      knobs: [["delayTime", "TIME", "amber"], ["delayFeedback", "FDBK", "magenta"], ["delayPingPong", "PING", "cyan"], ["delayFilter", "FILT", "amber"], ["delayMix", "MIX", "cyan"]] },
    { id: 7, short: "REVERB", title: "SHIMMER REVERB", accent: "emerald", toggle: "reverbEnable",
      knobs: [["reverbSize", "SIZE", "emerald"], ["reverbDecay", "DECAY", "emerald"], ["reverbPreDelay", "PRE", "cyan"], ["reverbDamping", "DAMP", "amber"], ["reverbShimmer", "SHIM", "magenta"], ["reverbMix", "MIX", "emerald"]] },
    { id: 8, short: "OTT", title: "MULTIBAND OTT", accent: "cyan", toggle: "compEnable", select: "compMode",
      knobs: [["compThreshold", "THRS", "amber"], ["compRatio", "RATIO", "magenta"], ["compAttack", "ATK", "cyan"], ["compRelease", "REL", "cyan"], ["compMakeup", "GAIN", "emerald"], ["compMix", "MIX", "cyan"]] },
    { id: 9, short: "RING", title: "ZENOLOGY MOD", accent: "magenta", toggle: "ringEnable", select: "ringMode",
      knobs: [["ringFreq", "FREQ", "magenta"], ["ringDepth", "DEPTH", "magenta"], ["ringLfoRate", "LFO", "cyan"], ["ringLfoDepth", "L-DEP", "amber"], ["ringMix", "MIX", "cyan"]] },
    { id: 10, short: "LO-FI", title: "BITCRUSHER", accent: "amber", toggle: "crushEnable", select: "crushMode",
      knobs: [["crushBits", "BITS", "amber"], ["crushDownsample", "DOWN", "amber"], ["crushDrive", "DRIVE", "magenta"], ["crushTone", "TONE", "cyan"], ["crushMix", "MIX", "amber"]] },
    { id: 11, short: "FLANGE", title: "FLANGER PACK", accent: "cyan", toggle: "flangerEnable", select: "flangerMode",
      knobs: [["flangerRate", "RATE", "cyan"], ["flangerDepth", "DEPTH", "cyan"], ["flangerFeedback", "FDBK", "magenta"], ["flangerManual", "MANU", "amber"], ["flangerMix", "MIX", "cyan"]] },
    { id: 12, short: "BODE", title: "BODE FREQUENCY SHIFTER", accent: "magenta", toggle: "bodeEnable",
      knobs: [["bodeShift", "SHIFT Hz", "magenta"], ["bodeFeedback", "FEEDBCK", "amber"], ["bodeMix", "MIX", "cyan"]] }
  ];

  const ACCENT_HEX = {
    cyan: "#22d3ee",
    magenta: "#f43f8e",
    amber: "#f59e0b",
    emerald: "#10b981"
  };

  let paramValues = {};
  for (const [k, m] of Object.entries(PARAM_META)) paramValues[k] = m.def;

  let allPresets = [];
  let activeCategory = "ALL";
  let currentPresetIdx = 0;
  let fxChain = [0, 1, 2, 9, 10, 11, 12, 5, 8, 3, 4, 6, 7];
  let articulators = [];
  let timeModCurve = null;
  let arpSteps = [];
  let barsA = new Array(64).fill(0);
  let barsB = new Array(64).fill(0);
  let harmEditMode = false;
  let harmEditPart = 0;
  let activeArtTarget = 0;
  let latestTelemetry = null;
  let draggingFxSlot = -1;

  const knobElements = new Map();

  function send(action, params = {}) {
    const qs = Object.entries(params)
      .map(([k, v]) => `${encodeURIComponent(k)}=${encodeURIComponent(String(v))}`)
      .join("&");
    window.location.href = `apc://callback?action=${encodeURIComponent(action)}${qs ? "&" + qs : ""}`;
  }

  function formatVal(id, v) {
    const m = PARAM_META[id];
    if (!m) return v.toFixed(2);
    if (m.fmt === "int") return String(Math.round(v));
    if (m.fmt === "pct") return `${Math.round(v * 100)}%`;
    if (m.fmt === "hz") return v >= 1000 || v <= -1000 ? `${(v / 1000).toFixed(2)}k` : `${Math.round(v)}Hz`;
    if (m.fmt === "db") return `${v >= 0 ? "+" : ""}${v.toFixed(1)}dB`;
    if (m.fmt === "ms") return `${Math.round(v)}ms`;
    if (m.fmt === "cents") return `${Math.round(v)}c`;
    if (m.fmt === "st") return `${v >= 0 ? "+" : ""}${v.toFixed(1)}st`;
    if (m.fmt === "scale") return v.toFixed(2) + "x";
    return v.toFixed(2);
  }

  // CNC-machined black anodized hardware rotary knob SVG (NO outer solid colored rings!)
  let knobIdCounter = 0;
  function createKnobElement(container, paramId, labelText, accentName, sizePx) {
    const meta = PARAM_META[paramId] || { min: 0, max: 1, def: 0 };
    const accent = ACCENT_HEX[accentName] || ACCENT_HEX.cyan;
    const kid = ++knobIdCounter;

    const wrap = document.createElement("div");
    wrap.className = "knob-wrap";

    const ns = "http://www.w3.org/2000/svg";
    const svg = document.createElementNS(ns, "svg");
    svg.setAttribute("width", String(sizePx));
    svg.setAttribute("height", String(sizePx));
    svg.setAttribute("viewBox", "0 0 60 60");

    // 3D CNC brushed black anodized gradients
    const defs = document.createElementNS(ns, "defs");
    const capGrad = document.createElementNS(ns, "linearGradient");
    capGrad.setAttribute("id", `capGrad_${kid}`);
    capGrad.setAttribute("x1", "0%"); capGrad.setAttribute("y1", "0%");
    capGrad.setAttribute("x2", "0%"); capGrad.setAttribute("y2", "100%");
    [
      ["0%", "#2f384c"],
      ["55%", "#191e2b"],
      ["100%", "#10141e"]
    ].forEach(([off, col]) => {
      const s = document.createElementNS(ns, "stop");
      s.setAttribute("offset", off);
      s.setAttribute("stop-color", col);
      capGrad.appendChild(s);
    });
    defs.appendChild(capGrad);

    const innerGrad = document.createElementNS(ns, "radialGradient");
    innerGrad.setAttribute("id", `innerGrad_${kid}`);
    innerGrad.setAttribute("cx", "45%"); innerGrad.setAttribute("cy", "38%"); innerGrad.setAttribute("r", "60%");
    [
      ["0%", "#202736"],
      ["100%", "#0f131c"]
    ].forEach(([off, col]) => {
      const s = document.createElementNS(ns, "stop");
      s.setAttribute("offset", off);
      s.setAttribute("stop-color", col);
      innerGrad.appendChild(s);
    });
    defs.appendChild(innerGrad);
    svg.appendChild(defs);

    // Micro-ticks around 270 deg sweep
    for (let i = 0; i <= 12; i++) {
      const ang = (-225 + (i / 12) * 270) * (Math.PI / 180);
      const r1 = 26.2, r2 = (i === 0 || i === 6 || i === 12) ? 29.6 : 28.6;
      const tick = document.createElementNS(ns, "line");
      tick.setAttribute("x1", (30 + Math.cos(ang) * r1).toFixed(2));
      tick.setAttribute("y1", (30 + Math.sin(ang) * r1).toFixed(2));
      tick.setAttribute("x2", (30 + Math.cos(ang) * r2).toFixed(2));
      tick.setAttribute("y2", (30 + Math.sin(ang) * r2).toFixed(2));
      tick.setAttribute("stroke", (meta.bipolar && i === 6) ? accent : "#64748b");
      tick.setAttribute("stroke-width", (i === 0 || i === 6 || i === 12) ? "1.5" : "1.0");
      svg.appendChild(tick);
    }

    // Outer recessed bevel shadow (dark metallic anodized aluminum, NO solid colored ring)
    const rim = document.createElementNS(ns, "circle");
    rim.setAttribute("cx", "30"); rim.setAttribute("cy", "30"); rim.setAttribute("r", "24.0");
    rim.setAttribute("fill", "#07090e");
    rim.setAttribute("stroke", "#2f394e");
    rim.setAttribute("stroke-width", "1.2");
    svg.appendChild(rim);

    // CNC machined cap
    const cap = document.createElementNS(ns, "circle");
    cap.setAttribute("cx", "30"); cap.setAttribute("cy", "30"); cap.setAttribute("r", "21.2");
    cap.setAttribute("fill", `url(#capGrad_${kid})`);
    cap.setAttribute("stroke", "#47546e");
    cap.setAttribute("stroke-width", "1.1");
    svg.appendChild(cap);

    const inner = document.createElementNS(ns, "circle");
    inner.setAttribute("cx", "30"); inner.setAttribute("cy", "30"); inner.setAttribute("r", "16.6");
    inner.setAttribute("fill", `url(#innerGrad_${kid})`);
    inner.setAttribute("stroke", "#2b3347");
    inner.setAttribute("stroke-width", "0.7");
    svg.appendChild(inner);

    // Rotating glowing indicator pill
    const pillGroup = document.createElementNS(ns, "g");
    const pill = document.createElementNS(ns, "rect");
    pill.setAttribute("x", "28.1"); pill.setAttribute("y", "10.8");
    pill.setAttribute("width", "3.8"); pill.setAttribute("height", "10.8");
    pill.setAttribute("rx", "1.9");
    pill.setAttribute("fill", accent);
    pill.setAttribute("style", `filter: drop-shadow(0 0 2.5px ${accent})`);
    pillGroup.appendChild(pill);
    svg.appendChild(pillGroup);

    const lbl = document.createElement("div");
    lbl.className = "knob-label";
    lbl.textContent = labelText;

    const valEl = document.createElement("div");
    valEl.className = "knob-val";
    valEl.textContent = formatVal(paramId, paramValues[paramId] ?? meta.def);

    wrap.appendChild(svg);
    wrap.appendChild(lbl);
    wrap.appendChild(valEl);
    container.innerHTML = "";
    container.appendChild(wrap);

    const updateVisual = (plain) => {
      const norm = Math.max(0, Math.min(1, (plain - meta.min) / (meta.max - meta.min || 1)));
      const deg = -135 + norm * 270;
      pillGroup.setAttribute("transform", `rotate(${deg.toFixed(1)} 30 30)`);
      valEl.textContent = formatVal(paramId, plain);
    };
    updateVisual(paramValues[paramId] ?? meta.def);

    // Interaction
    let startY = 0, startNorm = 0;
    wrap.addEventListener("pointerdown", (e) => {
      e.preventDefault();
      wrap.setPointerCapture(e.pointerId);
      startY = e.clientY;
      const cur = paramValues[paramId] ?? meta.def;
      startNorm = (cur - meta.min) / (meta.max - meta.min || 1);
      const onMove = (ev) => {
        const sens = ev.shiftKey ? 450 : 140;
        const delta = (startY - ev.clientY) / sens;
        const n = Math.max(0, Math.min(1, startNorm + delta));
        let val = meta.min + n * (meta.max - meta.min);
        if (meta.step) val = Math.round(val / meta.step) * meta.step;
        val = Math.max(meta.min, Math.min(meta.max, val));
        paramValues[paramId] = val;
        updateVisual(val);
        send("setParam", { id: paramId, val });
      };
      const onUp = () => {
        window.removeEventListener("pointermove", onMove);
        window.removeEventListener("pointerup", onUp);
      };
      window.addEventListener("pointermove", onMove);
      window.addEventListener("pointerup", onUp);
    });

    wrap.addEventListener("dblclick", () => {
      paramValues[paramId] = meta.def;
      updateVisual(meta.def);
      send("setParam", { id: paramId, val: meta.def });
    });

    if (!knobElements.has(paramId)) knobElements.set(paramId, []);
    knobElements.get(paramId).push(updateVisual);
  }

  function mountAllControls() {
    document.querySelectorAll(".knob-slot").forEach(el => {
      createKnobElement(el, el.dataset.param, el.dataset.label, el.dataset.accent, 44);
    });
    document.querySelectorAll(".mini-knob-slot").forEach(el => {
      createKnobElement(el, el.dataset.param, el.dataset.label, el.dataset.accent, 30);
    });

    document.querySelectorAll("select[data-param-select]").forEach(sel => {
      const id = sel.dataset.paramSelect;
      const meta = PARAM_META[id];
      if (!meta || !meta.opts) return;
      sel.innerHTML = meta.opts.map((opt, idx) => `<option value="${idx}">${opt}</option>`).join("");
      sel.value = String(Math.round(paramValues[id] ?? meta.def));
      sel.addEventListener("change", () => {
        const val = parseFloat(sel.value);
        paramValues[id] = val;
        send("setParam", { id, val });
      });
    });

    document.querySelectorAll("button[data-param-toggle]").forEach(btn => {
      const id = btn.dataset.paramToggle;
      btn.addEventListener("click", () => {
        const cur = (paramValues[id] ?? 0) >= 0.5 ? 1 : 0;
        const next = cur ? 0 : 1;
        paramValues[id] = next;
        btn.classList.toggle("on", next === 1);
        send("setParam", { id, val: next });
      });
    });
  }

  function buildFxRackUI() {
    const addSel = document.getElementById("fxAddSelect");
    addSel.innerHTML = FX_MODULES.map(m => `<option value="${m.id}">${m.title}</option>`).join("");
    renderFxChainAndCards();
  }

  function renderFxChainAndCards() {
    const pillsEl = document.getElementById("fxChainPills");
    const gridEl  = document.getElementById("fxCardsGrid");
    document.getElementById("fxCountBadge").textContent = `[${fxChain.length}]`;

    pillsEl.innerHTML = "";
    fxChain.forEach((modId, slot) => {
      const m = FX_MODULES[modId];
      if (!m) return;
      const pill = document.createElement("div");
      const isOn = (paramValues[m.toggle] ?? 0) >= 0.5;
      pill.className = `fx-pill ${isOn ? "enabled" : ""}`;
      pill.textContent = `${slot + 1}.${m.short}`;
      pill.draggable = true;
      pill.addEventListener("dragstart", () => { draggingFxSlot = slot; });
      pill.addEventListener("dragover", (e) => { e.preventDefault(); pill.classList.add("drag-over"); });
      pill.addEventListener("dragleave", () => { pill.classList.remove("drag-over"); });
      pill.addEventListener("drop", (e) => {
        e.preventDefault();
        pill.classList.remove("drag-over");
        if (draggingFxSlot >= 0 && draggingFxSlot !== slot) {
          send("fxMove", { from: draggingFxSlot, to: slot });
        }
        draggingFxSlot = -1;
      });
      pillsEl.appendChild(pill);
    });

    // Clear stale knob refs for grid slots? Keep persistent references cleanly by rebuilding cards only when fxChain order changes
    const sig = fxChain.join(",");
    if (gridEl.dataset.chainSig === sig) return;
    gridEl.dataset.chainSig = sig;
    gridEl.innerHTML = "";

    // Filter out dynamic FX knob callbacks before re-creating FX cards
    for (const m of FX_MODULES) {
      for (const [k] of m.knobs) knobElements.delete(k);
    }

    fxChain.forEach((modId, slot) => {
      const m = FX_MODULES[modId];
      if (!m) return;
      const card = document.createElement("div");
      card.className = `module-card accent-${m.accent}`;

      const hdr = document.createElement("div");
      hdr.className = "card-header";
      hdr.innerHTML = `<span class="card-title">FX ${slot + 1} // ${m.title}</span>`;

      const acts = document.createElement("div");
      acts.className = "card-actions";
      if (m.select) {
        const sel = document.createElement("select");
        sel.className = `mini-select ${m.accent}`;
        const opts = (PARAM_META[m.select] && PARAM_META[m.select].opts) || [];
        sel.innerHTML = opts.map((o, idx) => `<option value="${idx}">${o}</option>`).join("");
        sel.dataset.paramSelect = m.select;
        sel.value = String(Math.round(paramValues[m.select] ?? 0));
        sel.addEventListener("change", () => {
          const val = parseFloat(sel.value);
          paramValues[m.select] = val;
          send("setParam", { id: m.select, val });
        });
        acts.appendChild(sel);
      }

      const tog = document.createElement("button");
      tog.className = `toggle-btn ${m.accent} ${(paramValues[m.toggle] ?? 0) >= 0.5 ? "on" : ""}`;
      tog.dataset.paramToggle = m.toggle;
      tog.textContent = "ON";
      tog.addEventListener("click", () => {
        const next = ((paramValues[m.toggle] ?? 0) >= 0.5) ? 0 : 1;
        paramValues[m.toggle] = next;
        tog.classList.toggle("on", next === 1);
        send("setParam", { id: m.toggle, val: next });
      });
      acts.appendChild(tog);

      const rem = document.createElement("button");
      rem.className = "mini-btn";
      rem.textContent = "X";
      rem.title = "Remove FX Slot";
      rem.addEventListener("click", () => send("fxRemove", { slot }));
      acts.appendChild(rem);

      hdr.appendChild(acts);
      card.appendChild(hdr);

      const colsClass = m.knobs.length <= 3 ? "cols-3"
        : (m.knobs.length === 4 ? "cols-4"
        : (m.knobs.length === 5 ? "cols-5"
        : (m.knobs.length === 6 ? "cols-6" : "cols-4")));
      const kGrid = document.createElement("div");
      kGrid.className = `knob-grid ${colsClass}`;
      for (const [kId, kLbl, kAcc] of m.knobs) {
        const slotDiv = document.createElement("div");
        slotDiv.className = "knob-slot";
        createKnobElement(slotDiv, kId, kLbl, kAcc, 42);
        kGrid.appendChild(slotDiv);
      }
      card.appendChild(kGrid);
      gridEl.appendChild(card);
    });
  }

  function buildPiano() {
    const el = document.getElementById("pianoKeys");
    const baseMidi = 48; // C3..E5
    const totalNotes = 29;
    const isBlack = [false, true, false, true, false, false, true, false, true, false, true, false];
    let whiteCount = 0;
    for (let i = 0; i < totalNotes; i++) if (!isBlack[i % 12]) whiteCount++;

    el.innerHTML = "";
    let wIdx = 0;
    let activeKeyNote = -1;

    const triggerNote = (note, on, div) => {
      if (on) {
        if (activeKeyNote !== note) {
          if (activeKeyNote >= 0) send("noteOff", { note: activeKeyNote });
          activeKeyNote = note;
          div.classList.add("active");
          send("noteOn", { note, vel: 0.85 });
        }
      } else {
        if (activeKeyNote >= 0) {
          el.querySelectorAll(".active").forEach(k => k.classList.remove("active"));
          send("noteOff", { note: activeKeyNote });
          activeKeyNote = -1;
        }
      }
    };

    for (let i = 0; i < totalNotes; i++) {
      const midi = baseMidi + i;
      const blk = isBlack[i % 12];
      const key = document.createElement("div");
      if (!blk) {
        key.className = "key-white";
        wIdx++;
      } else {
        key.className = "key-black";
        key.style.left = `${(wIdx / whiteCount) * 100}%`;
      }
      key.addEventListener("pointerdown", (e) => { e.preventDefault(); triggerNote(midi, true, key); });
      key.addEventListener("pointerenter", (e) => { if (e.buttons === 1) triggerNote(midi, true, key); });
      el.appendChild(key);
    }
    window.addEventListener("pointerup", () => { if (activeKeyNote >= 0) triggerNote(-1, false); });
  }

  function hexToRgba(hex, alpha) {
    const clean = (hex || "#22d3ee").replace("#", "");
    if (clean.length !== 6) return `rgba(34,211,238,${alpha})`;
    const r = parseInt(clean.slice(0, 2), 16);
    const g = parseInt(clean.slice(2, 4), 16);
    const b = parseInt(clean.slice(4, 6), 16);
    return `rgba(${r},${g},${b},${alpha})`;
  }

  function evalSegmentT(t, tension = 0, shapeMode = 0) {
    t = Math.max(0, Math.min(1, t));
    if (shapeMode === 2) return t; // Linear
    if (shapeMode === 1) {
      const cosineS = 0.5 - 0.5 * Math.cos(t * Math.PI);
      if (Math.abs(tension) < 0.01) return cosineS;
      const centered = (t - 0.5) * 2;
      if (tension > 0) {
        const k = 1 + tension * 10;
        return 0.5 + 0.5 * (Math.tanh(centered * k) / Math.tanh(k));
      }
      const amt = -tension;
      const raw = 0.5 + 0.5 * (centered * centered * centered);
      return cosineS * (1 - amt) + raw * amt;
    }
    if (Math.abs(tension) < 0.001) return t;
    const k = tension * 4.8;
    const denom = Math.exp(k) - 1;
    if (Math.abs(denom) < 1e-5) return t;
    return (Math.exp(k * t) - 1) / denom;
  }

  function drawAllSplines() {
    drawSplineCanvas("articulatorCanvas", articulators[activeArtTarget], ACCENT_HEX.cyan, latestTelemetry ? latestTelemetry.envCursor : -1);
    drawSplineCanvas("timeModCanvas", timeModCurve, ACCENT_HEX.amber, latestTelemetry ? latestTelemetry.tmCursor : -1, latestTelemetry ? latestTelemetry.tmWave : null);
  }

  function drawSplineCanvas(canvasId, curveData, accentColor, cursorNorm, wavePreview) {
    const cvs = document.getElementById(canvasId);
    if (!cvs) return;
    const rect = cvs.getBoundingClientRect();
    if (rect.width < 10 || rect.height < 10) return;
    const dpr = window.devicePixelRatio || 1;
    const w = Math.round(rect.width * dpr);
    const h = Math.round(rect.height * dpr);
    if (cvs.width !== w || cvs.height !== h) { cvs.width = w; cvs.height = h; }

    const ctx = cvs.getContext("2d");
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    const W = rect.width, H = rect.height;
    ctx.clearRect(0, 0, W, H);

    const padX = 6, padY = 8;
    const uW = Math.max(10, W - padX * 2);
    const uH = Math.max(10, H - padY * 2);
    const toPxX = (nx) => padX + nx * uW;
    const toPxY = (ny) => padY + (1 - ny) * uH;

    // Subtle grid
    ctx.strokeStyle = "rgba(255,255,255,0.05)";
    ctx.lineWidth = 1;
    for (let g = 1; g < 4; g++) {
      ctx.beginPath(); ctx.moveTo(padX + (uW * g) / 4, padY); ctx.lineTo(padX + (uW * g) / 4, padY + uH); ctx.stroke();
      ctx.beginPath(); ctx.moveTo(padX, padY + (uH * g) / 4); ctx.lineTo(padX + uW, padY + (uH * g) / 4); ctx.stroke();
    }

    // Optional live waveform preview
    if (wavePreview && wavePreview.length > 1) {
      ctx.strokeStyle = "rgba(34,211,238,0.22)";
      ctx.beginPath();
      wavePreview.forEach((v, i) => {
        const px = padX + (i / (wavePreview.length - 1)) * uW;
        const py = padY + uH * 0.5 - v * (uH * 0.38);
        if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
      });
      ctx.stroke();
    }

    if (curveData && Array.isArray(curveData.nodes) && curveData.nodes.length >= 2) {
      const nodes = curveData.nodes;
      const pts = [];
      for (let i = 0; i + 1 < nodes.length; i++) {
        const n0 = nodes[i], n1 = nodes[i + 1];
        const steps = 16;
        for (let s = 0; s < steps; s++) {
          const lt = s / steps;
          const shaped = evalSegmentT(lt, n0.tension || 0, n0.shapeMode || 0);
          pts.push({
            x: toPxX(n0.x + lt * (n1.x - n0.x)),
            y: toPxY(n0.y + shaped * (n1.y - n0.y))
          });
        }
      }
      pts.push({ x: toPxX(nodes[nodes.length - 1].x), y: toPxY(nodes[nodes.length - 1].y) });

      // Translucent vertical fill under curve
      const grad = ctx.createLinearGradient(0, padY, 0, padY + uH);
      grad.addColorStop(0, hexToRgba(accentColor, 0.18));
      grad.addColorStop(1, hexToRgba(accentColor, 0.01));
      ctx.fillStyle = grad;
      ctx.beginPath();
      pts.forEach((p, idx) => { if (idx === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y); });
      ctx.lineTo(pts[pts.length - 1].x, padY + uH);
      ctx.lineTo(pts[0].x, padY + uH);
      ctx.closePath();
      ctx.fill();

      // Main curve stroke
      ctx.strokeStyle = accentColor;
      ctx.lineWidth = 2;
      ctx.beginPath();
      pts.forEach((p, idx) => { if (idx === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y); });
      ctx.stroke();

      // Control dots
      nodes.forEach((n) => {
        ctx.fillStyle = "#0b0d14";
        ctx.strokeStyle = accentColor;
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(toPxX(n.x), toPxY(n.y), 4.5, 0, Math.PI * 2);
        ctx.fill();
        ctx.stroke();
      });
    }

    if (cursorNorm !== undefined && cursorNorm >= 0) {
      const cx = toPxX(Math.max(0, Math.min(1, cursorNorm)));
      ctx.strokeStyle = "rgba(255,255,255,0.55)";
      ctx.lineWidth = 1.2;
      ctx.beginPath(); ctx.moveTo(cx, padY); ctx.lineTo(cx, padY + uH); ctx.stroke();
    }
  }

  function setupSplineInteraction(canvasId, getCurve, onUpdate, onAdd, onRemove) {
    const cvs = document.getElementById(canvasId);
    const padX = 6, padY = 8;
    let dragIdx = -1;
    let lastRightTime = 0, lastRightX = 0, lastRightY = 0;

    const toNorm = (clientX, clientY, r) => {
      const uW = Math.max(1, r.width - padX * 2);
      const uH = Math.max(1, r.height - padY * 2);
      return {
        nx: Math.max(0, Math.min(1, (clientX - r.left - padX) / uW)),
        ny: Math.max(0, Math.min(1, 1 - (clientY - r.top - padY) / uH))
      };
    };

    const findHitNode = (curve, clientX, clientY, r) => {
      const uW = Math.max(1, r.width - padX * 2);
      const uH = Math.max(1, r.height - padY * 2);
      const px = clientX - r.left;
      const py = clientY - r.top;
      let hit = -1, bestPx = 11.5;
      curve.nodes.forEach((n, idx) => {
        const vx = padX + n.x * uW;
        const vy = padY + (1 - n.y) * uH;
        const d = Math.hypot(vx - px, vy - py);
        if (d < bestPx) { bestPx = d; hit = idx; }
      });
      return hit;
    };

    const addNodeOptimistic = (curve, nx, ny) => {
      const clampedX = Math.max(0.015, Math.min(0.985, nx));
      const clampedY = Math.max(0, Math.min(1, ny));
      curve.nodes.push({ x: clampedX, y: clampedY, tension: 0, shapeMode: curve.globalShapeMode || 0 });
      curve.nodes.sort((a, b) => a.x - b.x);
      drawAllSplines();
      onAdd(clampedX, clampedY);
    };

    cvs.addEventListener("pointerdown", (e) => {
      const curve = getCurve();
      if (!curve || !curve.nodes) return;
      const r = cvs.getBoundingClientRect();
      const { nx, ny } = toNorm(e.clientX, e.clientY, r);
      const hit = findHitNode(curve, e.clientX, e.clientY, r);

      if (e.button === 2) {
        e.preventDefault();
        if (hit > 0 && hit < curve.nodes.length - 1) {
          curve.nodes.splice(hit, 1);
          drawAllSplines();
          onRemove(hit);
          lastRightTime = 0;
          return;
        }
        if (hit < 0) {
          const now = performance.now();
          if (now - lastRightTime <= 420 && Math.hypot(e.clientX - lastRightX, e.clientY - lastRightY) <= 20) {
            lastRightTime = 0;
            addNodeOptimistic(curve, nx, ny);
          } else {
            lastRightTime = now;
            lastRightX = e.clientX;
            lastRightY = e.clientY;
          }
        }
        return;
      }

      if (e.button === 0 && hit >= 0) {
        dragIdx = hit;
        cvs.setPointerCapture(e.pointerId);
        const onMove = (ev) => {
          const { nx: mx, ny: my } = toNorm(ev.clientX, ev.clientY, r);
          const n = curve.nodes[dragIdx];
          if (n) {
            if (dragIdx === 0) n.x = 0;
            else if (dragIdx === curve.nodes.length - 1) n.x = 1;
            else {
              const minX = curve.nodes[dragIdx - 1].x + 0.01;
              const maxX = curve.nodes[dragIdx + 1].x - 0.01;
              n.x = Math.max(minX, Math.min(maxX, mx));
            }
            n.y = my;
            drawAllSplines();
          }
          onUpdate(dragIdx, mx, my);
        };
        const onUp = () => {
          dragIdx = -1;
          window.removeEventListener("pointermove", onMove);
          window.removeEventListener("pointerup", onUp);
        };
        window.addEventListener("pointermove", onMove);
        window.addEventListener("pointerup", onUp);
      }
    });

    cvs.addEventListener("dblclick", (e) => {
      if (e.button !== 0) return;
      const curve = getCurve();
      if (!curve || !curve.nodes) return;
      const r = cvs.getBoundingClientRect();
      if (findHitNode(curve, e.clientX, e.clientY, r) >= 0) return;
      const { nx, ny } = toNorm(e.clientX, e.clientY, r);
      addNodeOptimistic(curve, nx, ny);
    });

    cvs.addEventListener("contextmenu", e => e.preventDefault());
  }

  function drawSpectrumOrHarmonics() {
    const cvs = document.getElementById("spectrumCanvas");
    if (!cvs) return;
    const rect = cvs.getBoundingClientRect();
    if (rect.width < 10 || rect.height < 10) return;
    const dpr = window.devicePixelRatio || 1;
    const w = Math.round(rect.width * dpr);
    const h = Math.round(rect.height * dpr);
    if (cvs.width !== w || cvs.height !== h) { cvs.width = w; cvs.height = h; }

    const ctx = cvs.getContext("2d");
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    const W = rect.width, H = rect.height;
    ctx.clearRect(0, 0, W, H);

    const padX = 8;
    const padTop = 8;
    const padBottom = 16;
    const usableW = Math.max(10, W - padX * 2);
    const usableH = Math.max(10, H - padTop - padBottom);
    const baseY = H - padBottom;

    // Subtle studio dB gridlines
    ctx.strokeStyle = "rgba(255,255,255,0.05)";
    ctx.lineWidth = 1;
    for (let i = 0; i <= 4; i++) {
      const y = padTop + (usableH * i) / 4;
      ctx.beginPath(); ctx.moveTo(padX, y); ctx.lineTo(W - padX, y); ctx.stroke();
    }

    if (harmEditMode) {
      const bars = harmEditPart === 0 ? barsA : barsB;
      const col = harmEditPart === 0 ? ACCENT_HEX.cyan : ACCENT_HEX.magenta;
      const bw = usableW / 64;
      for (let i = 0; i < 64; i++) {
        const val = bars[i] || 0;
        const bh = val * usableH;
        const x = padX + i * bw + 0.5;
        const barW = Math.max(1.2, bw - 1.2);
        ctx.fillStyle = col;
        ctx.fillRect(Math.min(W - padX - barW, x), baseY - bh, barW, bh);
      }
      return;
    }

    // Log-frequency grid & axis markers across 20 Hz..20 kHz
    const invLogRange = 1 / Math.log2(1000);
    ctx.font = "9px monospace";
    ctx.textAlign = "center";
    for (const [hz, label] of [[50, "50"], [100, "100"], [500, "500"], [1000, "1k"], [5000, "5k"], [10000, "10k"]]) {
      const gx = padX + (Math.log2(hz / 20) * invLogRange) * usableW;
      ctx.strokeStyle = "rgba(255,255,255,0.055)";
      ctx.beginPath(); ctx.moveTo(gx, padTop); ctx.lineTo(gx, baseY); ctx.stroke();
      ctx.fillStyle = "rgba(127, 140, 168, 0.78)";
      ctx.fillText(label, gx, H - 3);
    }

    if (latestTelemetry) {
      const sA = latestTelemetry.specA || [];
      const sB = latestTelemetry.specB || [];
      const fA = latestTelemetry.freqA || [];
      const fB = latestTelemetry.freqB || [];
      const flt = latestTelemetry.filt || [];
      const n = Math.max(sA.length, sB.length);

      // Build smoothed per-pixel spectral glow envelopes
      const bins = Math.max(16, Math.round(usableW));
      let envA = new Float32Array(bins);
      let envB = new Float32Array(bins);

      const splat = (env, norm, amp) => {
        const c = Math.max(0, Math.min(bins - 1, Math.round(norm * (bins - 1))));
        env[c] = Math.max(env[c], amp);
        if (c > 0) env[c - 1] = Math.max(env[c - 1], amp * 0.65);
        if (c + 1 < bins) env[c + 1] = Math.max(env[c + 1], amp * 0.65);
        if (c > 1) env[c - 2] = Math.max(env[c - 2], amp * 0.30);
        if (c + 2 < bins) env[c + 2] = Math.max(env[c + 2], amp * 0.30);
      };

      const ptsA = [], ptsB = [];
      for (let i = 0; i < n; i++) {
        const vA = sA[i] || 0;
        if (vA > 0.002) {
          const normA = Math.max(0, Math.min(1, (fA.length === sA.length) ? fA[i] : (i / Math.max(1, n - 1))));
          splat(envA, normA, vA);
          ptsA.push([normA, vA]);
        }
        const vB = sB[i] || 0;
        if (vB > 0.002) {
          const normB = Math.max(0, Math.min(1, (fB.length === sB.length) ? fB[i] : (i / Math.max(1, n - 1))));
          splat(envB, normB, vB);
          ptsB.push([normB, vB]);
        }
      }

      const smoothEnv = (arr) => {
        for (let pass = 0; pass < 2; pass++) {
          const out = new Float32Array(bins);
          for (let j = 0; j < bins; j++) {
            const l = j > 0 ? arr[j - 1] : 0;
            const m = arr[j];
            const r = j + 1 < bins ? arr[j + 1] : 0;
            out[j] = Math.max(m * 0.65, 0.25 * l + 0.5 * m + 0.25 * r);
          }
          arr = out;
        }
        return arr;
      };
      envA = smoothEnv(envA);
      envB = smoothEnv(envB);

      const drawGlow = (env, r, g, b, fillAlpha, lineAlpha) => {
        let hasSignal = false;
        for (let j = 0; j < bins; j++) { if (env[j] > 0.003) { hasSignal = true; break; } }
        if (!hasSignal) return;
        const grad = ctx.createLinearGradient(0, padTop, 0, baseY);
        grad.addColorStop(0, `rgba(${r},${g},${b},${fillAlpha})`);
        grad.addColorStop(1, `rgba(${r},${g},${b},0.01)`);
        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.moveTo(padX, baseY);
        for (let j = 0; j < bins; j++) {
          const px = padX + (j / (bins - 1)) * usableW;
          const py = baseY - Math.min(1, env[j]) * usableH;
          ctx.lineTo(px, py);
        }
        ctx.lineTo(padX + usableW, baseY);
        ctx.closePath();
        ctx.fill();

        ctx.strokeStyle = `rgba(${r},${g},${b},${lineAlpha})`;
        ctx.lineWidth = 1.1;
        ctx.beginPath();
        for (let j = 0; j < bins; j++) {
          const px = padX + (j / (bins - 1)) * usableW;
          const py = baseY - Math.min(1, env[j]) * usableH;
          if (j === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
        }
        ctx.stroke();
      };

      drawGlow(envB, 244, 63, 142, 0.20, 0.45);
      drawGlow(envA, 34, 211, 238, 0.25, 0.55);

      // Luminous partial stems + peak tips
      for (const [normB, vB] of ptsB) {
        if (vB <= 0.003) continue;
        const xB = Math.min(W - padX - 2.2, padX + normB * usableW + 0.7);
        const bH = Math.min(usableH, vB * usableH);
        ctx.fillStyle = "rgba(244,63,142,0.52)";
        ctx.fillRect(xB, baseY - bH, 1.8, bH);
        ctx.fillStyle = "rgba(251,113,173,0.92)";
        ctx.fillRect(xB - 0.2, baseY - bH, 2.2, Math.min(2.4, bH));
      }
      for (const [normA, vA] of ptsA) {
        if (vA <= 0.003) continue;
        const xA = Math.min(W - padX - 2.2, padX + normA * usableW);
        const aH = Math.min(usableH, vA * usableH);
        ctx.fillStyle = "rgba(34,211,238,0.72)";
        ctx.fillRect(xA, baseY - aH, 1.8, aH);
        ctx.fillStyle = "#a5f3fc";
        ctx.fillRect(xA - 0.2, baseY - aH, 2.2, Math.min(2.4, aH));
      }

      // Shaded Amber Filter + EQ curve
      if (flt.length > 1) {
        const fltGrad = ctx.createLinearGradient(0, padTop, 0, baseY);
        fltGrad.addColorStop(0, "rgba(245,158,11,0.12)");
        fltGrad.addColorStop(1, "rgba(245,158,11,0.00)");
        ctx.fillStyle = fltGrad;
        ctx.beginPath();
        ctx.moveTo(padX, baseY);
        flt.forEach((v, i) => {
          const px = padX + (i / (flt.length - 1)) * usableW;
          const py = baseY - Math.max(0, Math.min(1, v)) * usableH;
          ctx.lineTo(px, py);
        });
        ctx.lineTo(padX + usableW, baseY);
        ctx.closePath();
        ctx.fill();

        ctx.strokeStyle = ACCENT_HEX.amber;
        ctx.lineWidth = 2.0;
        ctx.beginPath();
        flt.forEach((v, i) => {
          const px = padX + (i / (flt.length - 1)) * usableW;
          const py = baseY - Math.max(0, Math.min(1, v)) * usableH;
          if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
        });
        ctx.stroke();
      }
    }

    // Top-right HUD legend & live fundamental pitch
    const hudY = padTop + 9;
    let hx = W - padX - 4;
    ctx.font = "9px monospace";
    ctx.textAlign = "right";
    if (latestTelemetry && latestTelemetry.voiceActive && latestTelemetry.pitchHz) {
      const pTxt = `${latestTelemetry.pitchHz.toFixed(1)}Hz`;
      ctx.fillStyle = "#e2e8f0";
      ctx.fillText(pTxt, hx, hudY);
      hx -= ctx.measureText(pTxt).width + 8;
    }
    for (const [lbl, col] of [["FLT", ACCENT_HEX.amber], ["B", ACCENT_HEX.magenta], ["A", ACCENT_HEX.cyan]]) {
      ctx.fillStyle = "rgba(148,163,184,0.85)";
      ctx.fillText(lbl, hx, hudY);
      hx -= ctx.measureText(lbl).width + 3;
      ctx.fillStyle = col;
      ctx.beginPath(); ctx.arc(hx - 2.5, hudY - 3, 2.3, 0, Math.PI * 2); ctx.fill();
      hx -= 11;
    }
  }

  function renderArpGrid(curStep) {
    const grid = document.getElementById("arpGridContainer");
    if (!grid || grid.style.display === "none") return;
    if (grid.children.length !== 16) {
      grid.innerHTML = "";
      for (let i = 0; i < 16; i++) {
        const cell = document.createElement("div");
        cell.className = "arp-step";
        cell.innerHTML = `<div class="arp-bar-wrap"><div class="arp-bar"></div></div><div class="arp-meta"></div>`;
        cell.addEventListener("click", () => {
          const st = arpSteps[i] || { enabled: true, vel: 0.8, oct: 0, semi: 0 };
          st.enabled = !st.enabled;
          send("arpSetStep", { idx: i, enabled: st.enabled ? 1 : 0 });
          renderArpGrid(curStep);
        });
        grid.appendChild(cell);
      }
    }
    for (let i = 0; i < 16; i++) {
      const st = arpSteps[i] || { enabled: true, vel: 0.8, oct: 0, semi: 0 };
      const cell = grid.children[i];
      cell.classList.toggle("disabled", !st.enabled);
      cell.classList.toggle("active-step", i === curStep);
      cell.querySelector(".arp-bar").style.height = `${Math.round((st.vel || 0.8) * 100)}%`;
      cell.querySelector(".arp-meta").textContent = `${st.semi >= 0 ? "+" : ""}${st.semi || 0}`;
    }
  }

  function renderPresetBrowser() {
    const grid = document.getElementById("presetBrowserGrid");
    const catBar = document.getElementById("presetCatPills");
    const q = (document.getElementById("presetSearchInput").value || "").toLowerCase();

    const cats = ["ALL", ...new Set(allPresets.map(p => p.category))];
    catBar.innerHTML = cats.map(c => `<button class="cat-pill ${c === activeCategory ? "active" : ""}" data-cat="${c}">${c}</button>`).join("");
    catBar.querySelectorAll(".cat-pill").forEach(b => {
      b.addEventListener("click", () => { activeCategory = b.dataset.cat; renderPresetBrowser(); });
    });

    const filtered = allPresets.filter(p => {
      if (activeCategory !== "ALL" && p.category !== activeCategory) return false;
      if (q && !`${p.name} ${p.category} ${p.waves} ${p.fx} ${p.desc}`.toLowerCase().includes(q)) return false;
      return true;
    });

    grid.innerHTML = "";
    filtered.forEach(p => {
      const card = document.createElement("div");
      card.className = `preset-item ${(!p.isUser && p.index === currentPresetIdx) ? "active" : ""}`;
      card.innerHTML = `
        <div class="preset-top">
          <span class="preset-name">${p.name}</span>
          <span class="preset-cat">${p.category}</span>
        </div>
        <div class="preset-meta">${p.waves} &bull; ${p.fx}</div>
      `;
      card.addEventListener("click", () => {
        if (p.isUser && p.path) send("loadUserPreset", { path: p.path });
        else send("loadPreset", { idx: p.index });
      });
      grid.appendChild(card);
    });
  }

  function bindGlobalEvents() {
    document.querySelectorAll(".tab-btn").forEach(btn => {
      btn.addEventListener("click", () => {
        const idx = btn.dataset.tab;
        document.querySelectorAll(".tab-btn").forEach(b => b.classList.toggle("active", b === btn));
        document.querySelectorAll(".tab-view").forEach(v => v.classList.toggle("active", v.id === `tab-${idx}`));
      });
    });

    document.getElementById("prevPresetBtn").addEventListener("click", () => {
      const count = allPresets.filter(p => !p.isUser).length || 1;
      send("loadPreset", { idx: (currentPresetIdx - 1 + count) % count });
    });
    document.getElementById("nextPresetBtn").addEventListener("click", () => {
      const count = allPresets.filter(p => !p.isUser).length || 1;
      send("loadPreset", { idx: (currentPresetIdx + 1) % count });
    });
    document.getElementById("initPresetBtn").addEventListener("click", () => send("loadPreset", { idx: 0 }));
    document.getElementById("presetQuickSelect").addEventListener("change", (e) => {
      send("loadPreset", { idx: parseInt(e.target.value, 10) || 0 });
    });

    document.getElementById("waveASelect").addEventListener("change", (e) => send("setWave", { part: 0, wave: e.target.value }));
    document.getElementById("waveBSelect").addEventListener("change", (e) => send("setWave", { part: 1, wave: e.target.value }));
    document.getElementById("copyABBtn").addEventListener("click", () => send("harmOp", { part: 0, op: "copyAB" }));

    document.getElementById("toggleHarmEditBtn").addEventListener("click", (e) => {
      harmEditMode = !harmEditMode;
      e.target.classList.toggle("active", harmEditMode);
      document.getElementById("harmEditTools").style.display = harmEditMode ? "flex" : "none";
      document.getElementById("visCardTitle").textContent = harmEditMode ? "HARMONIC DRAWBAR EDITOR" : "LIVE HARMONIC SPECTRUM";
      drawSpectrumOrHarmonics();
    });

    document.getElementById("harmPartABtn").addEventListener("click", () => { harmEditPart = 0; drawSpectrumOrHarmonics(); });
    document.getElementById("harmPartBBtn").addEventListener("click", () => { harmEditPart = 1; drawSpectrumOrHarmonics(); });
    document.querySelectorAll("[data-harm-op]").forEach(b => {
      b.addEventListener("click", () => send("harmOp", { part: harmEditPart, op: b.dataset.harmOp }));
    });

    const specCvs = document.getElementById("spectrumCanvas");
    specCvs.addEventListener("pointerdown", (e) => {
      if (!harmEditMode) return;
      const paintBar = (ev) => {
        const r = specCvs.getBoundingClientRect();
        const barIdx = Math.max(0, Math.min(63, Math.floor(((ev.clientX - r.left - 8) / Math.max(1, r.width - 16)) * 64)));
        const val = Math.max(0, Math.min(1, 1 - (ev.clientY - r.top - 8) / Math.max(1, r.height - 24)));
        (harmEditPart === 0 ? barsA : barsB)[barIdx] = val;
        send("setHarmonicBar", { part: harmEditPart, idx: barIdx, val });
        drawSpectrumOrHarmonics();
      };
      paintBar(e);
      const onMove = (ev) => paintBar(ev);
      const onUp = () => { window.removeEventListener("pointermove", onMove); window.removeEventListener("pointerup", onUp); };
      window.addEventListener("pointermove", onMove);
      window.addEventListener("pointerup", onUp);
    });

    document.getElementById("filterSubSelect").addEventListener("change", (e) => {
      const idx = parseInt(e.target.value, 10);
      if (idx > 0) send("loadFilterSub", { idx });
    });

    document.getElementById("modeShaperBtn").addEventListener("click", () => {
      document.getElementById("modeShaperBtn").classList.add("active");
      document.getElementById("modeArpBtn").classList.remove("active");
      document.getElementById("shaperControlsBar").style.display = "flex";
      document.getElementById("arpControlsBar").style.display = "none";
      document.getElementById("articulatorCanvas").style.display = "block";
      document.getElementById("arpGridContainer").style.display = "none";
    });
    document.getElementById("modeArpBtn").addEventListener("click", () => {
      document.getElementById("modeArpBtn").classList.add("active");
      document.getElementById("modeShaperBtn").classList.remove("active");
      document.getElementById("shaperControlsBar").style.display = "none";
      document.getElementById("arpControlsBar").style.display = "flex";
      document.getElementById("articulatorCanvas").style.display = "none";
      document.getElementById("arpGridContainer").style.display = "grid";
      renderArpGrid(latestTelemetry ? latestTelemetry.arpStep : 0);
    });

    document.querySelectorAll(".art-target").forEach(b => {
      b.addEventListener("click", () => {
        activeArtTarget = parseInt(b.dataset.target, 10);
        document.querySelectorAll(".art-target").forEach(x => x.classList.toggle("active", x === b));
        drawAllSplines();
      });
    });
    document.getElementById("artPresetSelect").addEventListener("change", (e) => {
      send("artSetPreset", { target: activeArtTarget, preset: e.target.value });
    });
    document.getElementById("artSyncBtn").addEventListener("click", () => {
      const c = articulators[activeArtTarget];
      if (c) send("artSetProps", { target: activeArtTarget, sync: c.tempoSync ? 0 : 1 });
    });
    document.getElementById("arpPatternSelect").addEventListener("change", (e) => {
      send("arpPreset", { idx: e.target.value });
    });

    setupSplineInteraction(
      "articulatorCanvas",
      () => articulators[activeArtTarget],
      (idx, x, y) => send("artSetNode", { target: activeArtTarget, idx, x, y }),
      (x, y) => send("artAddNode", { target: activeArtTarget, x, y }),
      (idx) => send("artRemoveNode", { target: activeArtTarget, idx })
    );
    setupSplineInteraction(
      "timeModCanvas",
      () => timeModCurve,
      (idx, x, y) => send("timeModSetNode", { idx, x, y }),
      (x, y) => send("timeModAddNode", { x, y }),
      (idx) => send("timeModRemoveNode", { idx })
    );

    document.getElementById("fxAddBtn").addEventListener("click", () => {
      send("fxAdd", { modId: document.getElementById("fxAddSelect").value });
    });
    document.getElementById("fxResetBtn").addEventListener("click", () => send("fxReset"));

    document.querySelectorAll("[data-tm-preset]").forEach(b => {
      b.addEventListener("click", () => send("timeModPreset", { idx: b.dataset.tmPreset }));
    });

    document.getElementById("presetSearchInput").addEventListener("input", renderPresetBrowser);
    document.getElementById("savePresetBtn").addEventListener("click", () => {
      const name = document.getElementById("savePresetName").value.trim();
      if (name) {
        send("saveUserPreset", { name, cat: "USER", desc: "Custom patch" });
        document.getElementById("savePresetName").value = "";
      }
    });

    document.getElementById("panicBtn").addEventListener("click", () => send("panic"));
    document.getElementById("updateBtn").addEventListener("click", () => send("triggerUpdate"));
  }

  window.initPrismor = (info) => {
    if (info.version) document.getElementById("versionBadge").textContent = "v" + info.version;
    const waves = info.waveforms || [];
    const wHTML = waves.map((w, i) => `<option value="${i}">${w}</option>`).join("");
    document.getElementById("waveASelect").innerHTML = wHTML;
    document.getElementById("waveBSelect").innerHTML = wHTML;

    const fSubs = info.filterSubPresets || [];
    document.getElementById("filterSubSelect").innerHTML =
      `<option value="0">Filter Sub-Preset...</option>` + fSubs.map((s, i) => `<option value="${i + 1}">${s}</option>`).join("");
  };

  window.updatePresets = (list) => {
    allPresets = list || [];
    const qSel = document.getElementById("presetQuickSelect");
    qSel.innerHTML = allPresets.filter(p => !p.isUser).map(p => `<option value="${p.index}">${p.name}</option>`).join("");
    qSel.value = String(currentPresetIdx);
    renderPresetBrowser();
  };

  window.updateState = (st) => {
    if (!st) return;
    if (st.params) {
      for (const [k, v] of Object.entries(st.params)) {
        paramValues[k] = v;
        const cbs = knobElements.get(k);
        if (cbs) cbs.forEach(fn => fn(v));
      }
      document.querySelectorAll("select[data-param-select]").forEach(sel => {
        const v = paramValues[sel.dataset.paramSelect];
        if (v !== undefined) sel.value = String(Math.round(v));
      });
      document.querySelectorAll("button[data-param-toggle]").forEach(btn => {
        const v = paramValues[btn.dataset.paramToggle];
        if (v !== undefined) btn.classList.toggle("on", v >= 0.5);
      });
    }
    if (st.waveA !== undefined) document.getElementById("waveASelect").value = String(st.waveA);
    if (st.waveB !== undefined) document.getElementById("waveBSelect").value = String(st.waveB);
    if (st.currentPreset !== undefined) {
      currentPresetIdx = st.currentPreset;
      const qSel = document.getElementById("presetQuickSelect");
      if (qSel.options.length > 0) qSel.value = String(currentPresetIdx);
    }
    if (Array.isArray(st.fxChain)) {
      fxChain = st.fxChain;
      renderFxChainAndCards();
    }
    if (Array.isArray(st.articulators)) articulators = st.articulators;
    if (st.timeModCurve) timeModCurve = st.timeModCurve;
    if (Array.isArray(st.arpSteps)) arpSteps = st.arpSteps;
    if (Array.isArray(st.barsA)) barsA = st.barsA;
    if (Array.isArray(st.barsB)) barsB = st.barsB;
    if (st.update) {
      const btn = document.getElementById("updateBtn");
      if (st.update.state === 0) btn.classList.add("hidden");
      else {
        btn.classList.remove("hidden");
        const labels = ["", `UPDATE ${st.update.version}`, `DL ${Math.round((st.update.progress || 0) * 100)}%`, "EXTRACTING", "RESTART APP", "RELOAD DAW", "RETRY UPDATE"];
        btn.textContent = labels[st.update.state] || "UPDATE";
      }
    }
    drawSpectrumOrHarmonics();
    drawAllSplines();
  };

  window.updateTelemetry = (t) => {
    latestTelemetry = t;
    const toPct = (v) => `${Math.min(100, Math.round(Math.pow(Math.max(0, v), 0.55) * 100))}%`;
    document.getElementById("meterL").style.width = toPct(t.peakL);
    document.getElementById("meterR").style.width = toPct(t.peakR);
    document.getElementById("cpuStat").textContent = `CPU ${(t.cpuPct || 0).toFixed(1)}%`;
    document.getElementById("voiceStat").textContent = `V: ${t.voices || 0}/${Math.round(paramValues.maxVoices || 8)}`;

    drawSpectrumOrHarmonics();
    drawSplineCanvas("articulatorCanvas", articulators[activeArtTarget], ACCENT_HEX.cyan, t.envCursor);
    drawSplineCanvas("timeModCanvas", timeModCurve, ACCENT_HEX.amber, t.tmCursor, t.tmWave);
    renderArpGrid(t.arpStep || 0);
  };

  document.addEventListener("DOMContentLoaded", () => {
    mountAllControls();
    buildFxRackUI();
    buildPiano();
    bindGlobalEvents();
    send("ready");
  });
})();
