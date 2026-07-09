const express = require('express');
const axios = require('axios');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 3000;
const OLA_URL = 'http://localhost:9090';

// Serve static files from the 'public' directory
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());

// Path to our persistent data
const DATA_FILE = path.join(__dirname, 'data.json');

// Initial state
let state = {
    universes: [
        { id: 0, name: 'Universe 1' },
        { id: 1, name: 'Universe 2' },
        { id: 2, name: 'Universe 3' },
        { id: 3, name: 'Universe 4' }
    ],
    scenes: [],
    chases: [],
    cuelist: {
        items: [], // { id, sceneId, fadeTime }
        currentIndex: 0,
        status: 'stopped' // 'stopped', 'playing', 'paused'
    },
    dmxData: {}, // { universe_id: [512 values] }
    settings: {
        mode: 'standalone', // 'standalone' or 'artnet'
    }
};

// Initialize DMX arrays
for (let i = 0; i < 4; i++) {
    state.dmxData[i] = new Array(512).fill(0);
}

// Target DMX arrays for fading
let targetDmxData = {};
let startDmxData = {};
for (let i = 0; i < 4; i++) {
    targetDmxData[i] = new Array(512).fill(0);
    startDmxData[i] = new Array(512).fill(0);
}
let fadeProgress = 1.0;
let fadeStep = 0.0;

// Load data from file if it exists
function loadData() {
    if (fs.existsSync(DATA_FILE)) {
        try {
            const rawData = fs.readFileSync(DATA_FILE);
            const loadedData = JSON.parse(rawData);
            state = { ...state, ...loadedData };
        } catch (e) {
            console.error("Error loading data.json", e);
        }
    }
}

// Save data to file
function saveData() {
    try {
        fs.writeFileSync(DATA_FILE, JSON.stringify(state, null, 2));
    } catch (e) {
        console.error("Error saving data.json", e);
    }
}

loadData();

// Chase engine state
let activeChases = {}; // { chaseId: { stepIndex: 0, nextTime: Date.now() } }

// --- API ROUTES ---

// Get full state
app.get('/api/state', (req, res) => {
    res.json(state);
});

// Save scenes
app.post('/api/scenes', (req, res) => {
    state.scenes = req.body;
    saveData();
    res.json({ success: true });
});

// Save chases
app.post('/api/chases', (req, res) => {
    state.chases = req.body;
    saveData();
    res.json({ success: true });
});

// Cuelist API
app.post('/api/cuelist', (req, res) => {
    state.cuelist.items = req.body.items || [];
    saveData();
    res.json({ success: true });
});

app.post('/api/cuelist/go', (req, res) => {
    if (state.cuelist.items.length === 0) {
        return res.json({ success: false, message: "Cuelist is empty" });
    }

    // If stopped, start at current index (or 0)
    // If playing, go to next index
    if (state.cuelist.status === 'playing') {
        state.cuelist.currentIndex = (state.cuelist.currentIndex + 1) % state.cuelist.items.length;
    } else {
        state.cuelist.status = 'playing';
    }

    const currentCue = state.cuelist.items[state.cuelist.currentIndex];
    triggerCue(currentCue);

    res.json({ success: true, cuelist: state.cuelist });
});

app.post('/api/cuelist/pause', (req, res) => {
    state.cuelist.status = 'paused';
    res.json({ success: true, cuelist: state.cuelist });
});

app.post('/api/cuelist/stop', (req, res) => {
    state.cuelist.status = 'stopped';
    state.cuelist.currentIndex = 0;
    fadeProgress = 1.0; // cancel fade
    res.json({ success: true, cuelist: state.cuelist });
});

function triggerCue(cue) {
    const scene = state.scenes.find(s => s.id == cue.sceneId);
    if (!scene) return;

    // Load scene into targetDmxData and record start
    for (let u = 0; u < 4; u++) {
        if (scene.dmxData[u]) {
            targetDmxData[u] = [...scene.dmxData[u]];
            startDmxData[u] = [...state.dmxData[u]];
        }
    }

    const fadeTimeMs = (cue.fadeTime || 0) * 1000;
    if (fadeTimeMs <= 0) {
        // Instant snap
        fadeProgress = 1.0;
        for (let u = 0; u < 4; u++) {
            state.dmxData[u] = [...targetDmxData[u]];
        }
    } else {
        // Start fading
        fadeProgress = 0.0;
        const ticks = fadeTimeMs / 25.0; // 25ms tick rate
        fadeStep = 1.0 / ticks;
    }
}

// Save settings
app.post('/api/settings', async (req, res) => {
    state.settings = req.body;
    saveData();
    // In a real app, here we would configure OLA using its REST API.
    // e.g. turning on/off Art-Net input depending on settings.mode
    res.json({ success: true });
});

// Update DMX value directly (from fader)
app.post('/api/dmx', (req, res) => {
    const { universe, channel, value } = req.body;
    if (state.dmxData[universe]) {
        state.dmxData[universe][channel] = value;
    }
    res.json({ success: true });
});

// Recall a scene
app.post('/api/scenes/:id/recall', (req, res) => {
    const sceneId = req.params.id;
    const scene = state.scenes.find(s => s.id == sceneId);
    if (scene) {
        // apply scene data to DMX arrays
        for (const [u, channels] of Object.entries(scene.dmxData)) {
            const uInt = parseInt(u);
            if (state.dmxData[uInt]) {
                channels.forEach((val, index) => {
                    state.dmxData[uInt][index] = val;
                });
            }
        }
    }
    res.json({ success: true });
});

// Start/Stop chase
app.post('/api/chases/:id/toggle', (req, res) => {
    const chaseId = req.params.id;
    const chase = state.chases.find(c => c.id == chaseId);

    if (!chase) {
        return res.status(404).json({ error: "Chase not found" });
    }

    if (activeChases[chaseId]) {
        delete activeChases[chaseId];
    } else {
        if (chase.steps && chase.steps.length > 0) {
            activeChases[chaseId] = { stepIndex: 0, nextTime: Date.now() + (chase.steps[0].holdTime || 1000) };
            applyScene(chase.steps[0].sceneId);
        }
    }
    res.json({ success: true, active: !!activeChases[chaseId] });
});

function applyScene(sceneId) {
    const scene = state.scenes.find(s => s.id == sceneId);
    if (scene) {
        for (const [u, channels] of Object.entries(scene.dmxData)) {
            const uInt = parseInt(u);
            if (state.dmxData[uInt]) {
                channels.forEach((val, index) => {
                    state.dmxData[uInt][index] = val;
                });
            }
        }
    }
}

// --- DMX ENGINE TICK ---
// Send data to OLA at roughly 40Hz (25ms)
setInterval(() => {
    const now = Date.now();

    // Process active chases
    for (const [chaseId, chaseState] of Object.entries(activeChases)) {
        if (now >= chaseState.nextTime) {
            const chase = state.chases.find(c => c.id == chaseId);
            if (chase && chase.steps && chase.steps.length > 0) {
                chaseState.stepIndex = (chaseState.stepIndex + 1) % chase.steps.length;
                const nextStep = chase.steps[chaseState.stepIndex];
                applyScene(nextStep.sceneId);
                chaseState.nextTime = now + (nextStep.holdTime || 1000);
            }
        }
    }

    // Process Cuelist fading
    if (fadeProgress < 1.0 && state.cuelist.status !== 'paused') {
        fadeProgress += fadeStep;
        if (fadeProgress >= 1.0) {
            fadeProgress = 1.0;
        }

        for (let u = 0; u < 4; u++) {
            for (let c = 0; c < 512; c++) {
                const start = startDmxData[u][c];
                const target = targetDmxData[u][c];
                if (start !== target) {
                    // linear interpolate
                    const current = Math.round(start + (target - start) * fadeProgress);
                    state.dmxData[u][c] = current;
                } else {
                    state.dmxData[u][c] = target;
                }
            }
        }
    }

    // Send to OLA if we are in standalone mode or need to send output
    // Note: DMX format for OLA API is usually a comma separated string: "d=0,255,10,..."
    // We send data for all 4 configured universes
    if (state.settings.mode === 'standalone') {
        for (let i = 0; i < 4; i++) {
            const dmxArray = state.dmxData[i];
            const dmxString = dmxArray.join(',');

            // Fire and forget, don't crash if OLA is down
            axios.post(`${OLA_URL}/set_dmx`, `u=${i}&d=${dmxString}`, {
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
            }).catch(e => {
                // Ignore errors to not spam console if OLA is not running during dev
            });
        }
    }

}, 25);

// Start server
app.listen(PORT, () => {
    console.log(`Web console server running on http://localhost:${PORT}`);
});
