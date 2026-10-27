// Showcase Application Logic

document.addEventListener('DOMContentLoaded', () => {
    setupTabNavigation();
    setupTelemetry();
    setupBenchmark();
    setupDialogs();
    setupTextProcessing();
});

// Tab Switching
function setupTabNavigation() {
    const navItems = document.querySelectorAll('.aura-nav-item');
    const tabPanes = document.querySelectorAll('.tab-pane');

    navItems.forEach(item => {
        item.addEventListener('click', () => {
            const targetTab = item.getAttribute('data-tab');

            navItems.forEach(i => i.classList.remove('active'));
            tabPanes.forEach(p => p.classList.remove('active'));

            item.classList.add('active');
            const targetEl = document.getElementById(targetTab);
            if (targetEl) targetEl.classList.add('active');
        });
    });
}

// System Telemetry from C++
async function setupTelemetry() {
    const refreshBtn = document.getElementById('btn-refresh-telemetry');
    const statOs = document.getElementById('stat-os');
    const statCores = document.getElementById('stat-cores');
    const statRamPct = document.getElementById('stat-ram-pct');
    const statRamAvail = document.getElementById('stat-ram-avail');
    const statHost = document.getElementById('stat-host');
    const statRamTotal = document.getElementById('stat-ram-total');
    const ramBar = document.getElementById('ram-progress-bar');

    async function loadTelemetry() {
        try {
            const data = await aura.call('get_system_telemetry');
            if (data.overview) {
                statOs.textContent = `${data.overview.os} (${data.overview.arch})`;
                statCores.textContent = `${data.overview.cpu_cores} Logical`;
                statHost.textContent = data.overview.computer_name;
            }

            if (data.memory && !data.memory.error) {
                const mem = data.memory;
                statRamPct.textContent = `${mem.percent_used}%`;
                statRamAvail.textContent = `${mem.avail_gb.toFixed(1)} GB`;
                statRamTotal.textContent = `${mem.total_gb.toFixed(1)} GB`;
                ramBar.style.width = `${mem.percent_used}%`;
            }
        } catch (err) {
            console.error('Failed to load telemetry:', err);
        }
    }

    refreshBtn.addEventListener('click', loadTelemetry);
    loadTelemetry();
    // Auto refresh every 5 seconds
    setInterval(loadTelemetry, 5000);
}

// C++ Multi-Threaded Benchmark
function setupBenchmark() {
    const btnRun = document.getElementById('btn-run-benchmark');
    const inputLimit = document.getElementById('bench-limit');
    const selectThreads = document.getElementById('bench-threads');
    const boxResults = document.getElementById('benchmark-results');
    const resPrimes = document.getElementById('res-primes');
    const resTime = document.getElementById('res-time');
    const resSpeed = document.getElementById('res-speed');

    btnRun.addEventListener('click', async () => {
        const limit = parseInt(inputLimit.value, 10) || 1000000;
        const threads = parseInt(selectThreads.value, 10) || 4;

        try {
            btnRun.disabled = true;
            btnRun.textContent = 'Calculating in C++...';

            const res = await aura.call('run_benchmark', { limit, threads });

            boxResults.style.display = 'flex';
            resPrimes.textContent = res.primes_found.toLocaleString();
            resTime.textContent = `${res.elapsed_ms.toFixed(1)} ms`;

            const primesPerSec = Math.round((res.primes_found / (res.elapsed_ms / 1000.0)));
            resSpeed.textContent = `${primesPerSec.toLocaleString()} primes/sec`;
        } catch (err) {
            alert('Benchmark failed: ' + err.message);
        } finally {
            btnRun.disabled = false;
            btnRun.textContent = 'Start C++ Benchmark';
        }
    });
}

// Win32 Native Dialogs
function setupDialogs() {
    const btnOpen = document.getElementById('btn-open-file');
    const btnSave = document.getElementById('btn-save-file');
    const output = document.getElementById('dialog-output');

    btnOpen.addEventListener('click', async () => {
        output.textContent = 'Opening native file dialog...';
        const res = await aura.dialog.open({ filter: 'All Files (*.*)\0*.*\0' });
        if (res.cancelled) {
            output.textContent = 'User cancelled file selection.';
        } else {
            output.textContent = `Selected File: ${res.path}`;
        }
    });

    btnSave.addEventListener('click', async () => {
        output.textContent = 'Opening native save dialog...';
        const res = await aura.dialog.save({ default_name: 'export.txt' });
        if (res.cancelled) {
            output.textContent = 'User cancelled save dialog.';
        } else {
            output.textContent = `Save Location: ${res.path}`;
        }
    });
}

// C++ String Processing
function setupTextProcessing() {
    const input = document.getElementById('text-input');
    const chars = document.getElementById('text-chars');
    const words = document.getElementById('text-words');
    const reversed = document.getElementById('text-reversed');

    let debounceTimer = null;

    async function triggerProcessing() {
        const text = input.value;
        try {
            const res = await aura.call('process_text', { text });
            chars.textContent = res.char_count;
            words.textContent = res.word_count;
            reversed.textContent = res.reversed;
        } catch (err) {
            console.error(err);
        }
    }

    input.addEventListener('input', () => {
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(triggerProcessing, 150);
    });

    triggerProcessing();
}
