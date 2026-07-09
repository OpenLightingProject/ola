// State from server
let appState = {
    scenes: [],
    chases: [],
    cuelist: { items: [], currentIndex: 0, status: 'stopped' },
    settings: {},
    dmxData: { 0: [], 1: [], 2: [], 3: [] }
};

let currentUniverse = 0;
let currentPage = 0; // 0 = ch 1-12, 1 = ch 13-24, etc.

document.addEventListener('DOMContentLoaded', () => {
    initUI();
    fetchState();
    setInterval(fetchState, 1000); // Poll state periodically
});

function initUI() {
    // Populate Page Select (512 / 12 = 43 pages)
    const pageSelect = document.getElementById('pageSelect');
    for (let i = 0; i < 43; i++) {
        let opt = document.createElement('option');
        opt.value = i;
        opt.innerText = `Channels ${i*12 + 1} - ${Math.min((i+1)*12, 512)}`;
        pageSelect.appendChild(opt);
    }

    pageSelect.addEventListener('change', (e) => {
        currentPage = parseInt(e.target.value);
        renderFaders();
    });

    document.getElementById('universeSelect').addEventListener('change', (e) => {
        currentUniverse = parseInt(e.target.value);
        renderFaders();
    });

    document.getElementById('btnSaveScene').addEventListener('click', () => {
        let modal = new bootstrap.Modal(document.getElementById('newSceneModal'));
        modal.show();
    });

    document.getElementById('btnConfirmSaveScene').addEventListener('click', saveScene);

    document.getElementById('btnNewChase').addEventListener('click', () => {
        document.getElementById('chaseStepsContainer').innerHTML = '';
        let modal = new bootstrap.Modal(document.getElementById('newChaseModal'));
        modal.show();
    });

    document.getElementById('btnAddStep').addEventListener('click', addChaseStepUI);
    document.getElementById('btnConfirmSaveChase').addEventListener('click', saveChase);

    document.getElementById('btnSaveSettings').addEventListener('click', saveSettings);

    // Cuelist UI bindings
    document.getElementById('btnAddCue').addEventListener('click', addCue);
    document.getElementById('btnCueGo').addEventListener('click', () => { fetch('/api/cuelist/go', { method: 'POST' }); });
    document.getElementById('btnCuePause').addEventListener('click', () => { fetch('/api/cuelist/pause', { method: 'POST' }); });
    document.getElementById('btnCueStop').addEventListener('click', () => { fetch('/api/cuelist/stop', { method: 'POST' }); });

    // Sync nav styling to simulate active state since we changed to buttons
    document.querySelectorAll('.nav-link').forEach(btn => {
        btn.addEventListener('show.bs.tab', (e) => {
            document.querySelectorAll('.nav-link').forEach(b => {
                b.classList.remove('active', 'text-white');
                b.classList.add('text-white-50');
            });
            e.target.classList.add('active', 'text-white');
            e.target.classList.remove('text-white-50');
        });
    });
}

function fetchState() {
    fetch('/api/state')
        .then(res => res.json())
        .then(data => {
            appState = data;
            renderScenes();
            renderChases();
            renderCuelist();
            if (document.getElementById('modeSelect').value !== (appState.settings.mode || 'standalone')) {
                document.getElementById('modeSelect').value = appState.settings.mode || 'standalone';
            }
            // Only render faders initially or if values changed outside
            if(document.querySelectorAll('.fader-wrapper').length === 0) {
                renderFaders();
            }
        });
}

function renderCuelist() {
    // Populate dropdown only if the number of scenes has changed to prevent closing the dropdown menu
    const select = document.getElementById('cueSceneSelect');
    if (select.options.length !== appState.scenes.length) {
        const currentVal = select.value;
        select.innerHTML = '';
        appState.scenes.forEach(s => {
            let opt = document.createElement('option');
            opt.value = s.id;
            opt.textContent = s.name;
            select.appendChild(opt);
        });
        if (currentVal && Array.from(select.options).some(o => o.value === currentVal)) {
            select.value = currentVal;
        }
    }

    // Populate table conditionally to prevent flickering or losing focus
    const tbody = document.getElementById('cuelistTableBody');

    let tableHtml = '';
    if (appState.cuelist && appState.cuelist.items) {
        appState.cuelist.items.forEach((cue, index) => {
            const isActive = (index === appState.cuelist.currentIndex && appState.cuelist.status !== 'stopped');
            const rowStyle = isActive ? 'border-left: 4px solid #28a745; background-color: rgba(40, 167, 69, 0.2);' : 'border-left: 4px solid transparent;';
            const scene = appState.scenes.find(s => s.id == cue.sceneId);
            const sceneName = scene ? scene.name : 'Unknown';
            // Use simple string concat but escape name in logic below or textContent in DOM creation
            // We use DOM API to ensure safe escaping and avoid reflows
        });
    }

    // Better way: compare state length and status, but to be robust and safe from XSS, we build elements
    // We only wipe and rebuild if there's a difference in length, active index, or status
    const currentSig = JSON.stringify({
        len: appState.cuelist?.items?.length,
        idx: appState.cuelist?.currentIndex,
        stat: appState.cuelist?.status
    });

    if (tbody.dataset.sig !== currentSig) {
        tbody.innerHTML = '';
        if (appState.cuelist && appState.cuelist.items) {
            appState.cuelist.items.forEach((cue, index) => {
                let tr = document.createElement('tr');
                const isActive = (index === appState.cuelist.currentIndex && appState.cuelist.status !== 'stopped');

                if (isActive) {
                    tr.style.borderLeft = "4px solid #28a745";
                    tr.style.backgroundColor = "rgba(40, 167, 69, 0.2)";
                } else {
                    tr.style.borderLeft = "4px solid transparent";
                }

                const scene = appState.scenes.find(s => s.id == cue.sceneId);
                const sceneName = scene ? scene.name : 'Unknown';

                let tdIdx = document.createElement('td');
                tdIdx.textContent = index + 1;

                let tdName = document.createElement('td');
                tdName.textContent = sceneName;

                let tdTime = document.createElement('td');
                tdTime.textContent = cue.fadeTime + 's';

                let tdActions = document.createElement('td');
                tdActions.className = "text-end";

                let badge = document.createElement('span');
                badge.className = "badge " + (isActive ? 'bg-success' : 'bg-secondary');
                badge.textContent = isActive ? 'ACTIVE' : '';

                let btn = document.createElement('button');
                btn.className = "btn btn-sm btn-outline-danger ms-2";
                btn.textContent = "X";
                btn.onclick = () => removeCue(index);

                tdActions.appendChild(badge);
                tdActions.appendChild(btn);

                tr.appendChild(tdIdx);
                tr.appendChild(tdName);
                tr.appendChild(tdTime);
                tr.appendChild(tdActions);

                tbody.appendChild(tr);
            });
        }
        tbody.dataset.sig = currentSig;
    }
}

function addCue() {
    const sceneId = document.getElementById('cueSceneSelect').value;
    const fadeTime = parseFloat(document.getElementById('cueFadeTime').value) || 0;

    if (!sceneId) return;

    if (!appState.cuelist.items) {
        appState.cuelist.items = [];
    }

    appState.cuelist.items.push({
        id: Date.now().toString(),
        sceneId: sceneId,
        fadeTime: fadeTime
    });

    fetch('/api/cuelist', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(appState.cuelist)
    }).then(() => fetchState());
}

window.removeCue = function(index) {
    appState.cuelist.items.splice(index, 1);
    fetch('/api/cuelist', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(appState.cuelist)
    }).then(() => fetchState());
}

function renderFaders() {
    const faderRow = document.getElementById('faderRow');
    faderRow.innerHTML = '';

    let startCh = currentPage * 12;
    let endCh = Math.min(startCh + 12, 512);

    for (let i = startCh; i < endCh; i++) {
        let val = 0;
        if (appState.dmxData[currentUniverse] && appState.dmxData[currentUniverse][i] !== undefined) {
            val = appState.dmxData[currentUniverse][i];
        }

        let faderHTML = `
            <div class="fader-wrapper">
                <div class="fader-label">CH ${i + 1}</div>
                <input type="range" orient="vertical" min="0" max="255" value="${val}" class="fader-input" data-channel="${i}">
                <input type="text" value="${val}" class="fader-val" readonly>
            </div>
        `;
        faderRow.innerHTML += faderHTML;
    }

    // Add event listeners to newly created faders
    document.querySelectorAll('.fader-input').forEach(input => {
        input.addEventListener('input', (e) => {
            const channel = parseInt(e.target.dataset.channel);
            const value = parseInt(e.target.value);
            e.target.nextElementSibling.value = value;

            // Optimistic local update
            appState.dmxData[currentUniverse][channel] = value;

            // Send to server
            fetch('/api/dmx', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ universe: currentUniverse, channel, value })
            });
        });
    });
}

function renderScenes() {
    const list = document.getElementById('sceneList');

    // Check if re-render is needed
    const sig = JSON.stringify(appState.scenes.map(s => s.id + s.name));
    if (list.dataset.sig === sig) return;

    list.innerHTML = '';
    appState.scenes.forEach(scene => {
        let li = document.createElement('li');
        li.className = 'list-group-item d-flex justify-content-between align-items-center';

        let span = document.createElement('span');
        span.textContent = scene.name;

        let btn = document.createElement('button');
        btn.className = 'btn btn-sm btn-success btn-recall-scene';
        btn.dataset.id = scene.id;
        btn.textContent = 'Recall';

        li.appendChild(span);
        li.appendChild(btn);
        list.appendChild(li);
    });

    list.dataset.sig = sig;

    document.querySelectorAll('.btn-recall-scene').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const id = e.target.dataset.id;
            fetch(`/api/scenes/${id}/recall`, { method: 'POST' }).then(() => fetchState());
        });
    });
}

function saveScene() {
    const name = document.getElementById('sceneNameInput').value;
    if(!name) return;

    const newScene = {
        id: Date.now().toString(),
        name: name,
        // Clone current DMX state
        dmxData: JSON.parse(JSON.stringify(appState.dmxData))
    };

    appState.scenes.push(newScene);

    fetch('/api/scenes', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(appState.scenes)
    }).then(() => {
        let modal = bootstrap.Modal.getInstance(document.getElementById('newSceneModal'));
        modal.hide();
        fetchState();
    });
}

function addChaseStepUI() {
    const container = document.getElementById('chaseStepsContainer');
    let stepDiv = document.createElement('div');
    stepDiv.className = 'd-flex mb-2 chase-step-row';

    let sceneSelect = `<select class="form-select me-2 step-scene">`;
    appState.scenes.forEach(s => {
        sceneSelect += `<option value="${s.id}">${s.name}</option>`;
    });
    sceneSelect += `</select>`;

    stepDiv.innerHTML = `
        ${sceneSelect}
        <input type="number" class="form-control me-2 step-time" placeholder="Hold ms" value="1000">
        <button class="btn btn-danger btn-sm" onclick="this.parentElement.remove()">X</button>
    `;
    container.appendChild(stepDiv);
}

function saveChase() {
    const name = document.getElementById('chaseNameInput').value;
    const stepsElements = document.querySelectorAll('.chase-step-row');

    let steps = [];
    stepsElements.forEach(row => {
        steps.push({
            sceneId: row.querySelector('.step-scene').value,
            holdTime: parseInt(row.querySelector('.step-time').value)
        });
    });

    if(!name || steps.length === 0) return;

    const newChase = {
        id: Date.now().toString(),
        name: name,
        steps: steps
    };

    appState.chases.push(newChase);

    fetch('/api/chases', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(appState.chases)
    }).then(() => {
        let modal = bootstrap.Modal.getInstance(document.getElementById('newChaseModal'));
        modal.hide();
        fetchState();
    });
}

function renderChases() {
    const list = document.getElementById('chaseList');

    const sig = JSON.stringify(appState.chases.map(c => c.id + c.name + c.steps.length));
    if (list.dataset.sig === sig) return;

    list.innerHTML = '';
    appState.chases.forEach(chase => {
        let li = document.createElement('li');
        li.className = 'list-group-item d-flex justify-content-between align-items-center';

        let span = document.createElement('span');
        span.textContent = `${chase.name} (${chase.steps.length} steps)`;

        let btn = document.createElement('button');
        btn.className = 'btn btn-sm btn-warning btn-toggle-chase';
        btn.dataset.id = chase.id;
        btn.textContent = 'Play / Stop';

        li.appendChild(span);
        li.appendChild(btn);
        list.appendChild(li);
    });

    list.dataset.sig = sig;

    document.querySelectorAll('.btn-toggle-chase').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const id = e.target.dataset.id;
            fetch(`/api/chases/${id}/toggle`, { method: 'POST' });
        });
    });
}

function saveSettings() {
    const mode = document.getElementById('modeSelect').value;
    appState.settings.mode = mode;

    fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(appState.settings)
    }).then(() => {
        alert("Settings saved!");
    });
}
