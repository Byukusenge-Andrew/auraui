/**
 * AuraUI Starter Application Logic
 * Demonstrates C++ RPC calls, window controls, and native OS capabilities.
 */

document.addEventListener('DOMContentLoaded', () => {
    // -------------------------------------------------------------
    // 1. Interactive C++ Counter
    // -------------------------------------------------------------
    const btnCounter = document.getElementById('btn-counter');
    const counterVal = document.getElementById('counter-val');

    if (btnCounter && counterVal) {
        btnCounter.addEventListener('click', async () => {
            try {
                btnCounter.classList.add('aura-btn-loading');
                // Call in-process C++ handler bound in src/main.cpp
                const res = await aura.call('counter.increment');
                if (res && typeof res.count !== 'undefined') {
                    counterVal.textContent = res.count;
                }
            } catch (err) {
                console.warn('Counter RPC fallback:', err);
                // Fallback for preview in plain browser
                const current = parseInt(counterVal.textContent, 10) || 0;
                counterVal.textContent = current + 1;
            } finally {
                btnCounter.classList.remove('aura-btn-loading');
            }
        });
    }

    // -------------------------------------------------------------
    // 2. Compute Pipeline (Typed RPC with Arguments)
    // -------------------------------------------------------------
    const numA = document.getElementById('num-a');
    const numB = document.getElementById('num-b');
    const opSelect = document.getElementById('op-select');
    const btnCalc = document.getElementById('btn-calc');
    const calcResult = document.getElementById('calc-result');

    if (btnCalc && calcResult) {
        btnCalc.addEventListener('click', async () => {
            const a = parseFloat(numA.value) || 0;
            const b = parseFloat(numB.value) || 0;
            const operation = opSelect.value;

            try {
                btnCalc.disabled = true;
                btnCalc.textContent = 'Computing...';

                // Send typed parameters to native C++ calculate handler
                const res = await aura.call('calculate', { a, b, operation });
                calcResult.innerHTML = `Result: <strong>${res.result}</strong>`;
            } catch (err) {
                console.warn('Calculate RPC fallback:', err);
                // In-browser math fallback
                let result = 0;
                if (operation === '+') result = a + b;
                else if (operation === '-') result = a - b;
                else if (operation === '*') result = a * b;
                else if (operation === '/' && b !== 0) result = a / b;
                calcResult.innerHTML = `Result (JS fallback): <strong>${result}</strong>`;
            } finally {
                btnCalc.disabled = false;
                btnCalc.textContent = 'Run';
            }
        });
    }

    // -------------------------------------------------------------
    // 3. Native OS Capabilities (Dialog & Clipboard)
    // -------------------------------------------------------------
    const btnOpenFile = document.getElementById('btn-open-file');
    const btnCopyLink = document.getElementById('btn-copy-link');
    const dialogResult = document.getElementById('dialog-result');

    if (btnOpenFile && dialogResult) {
        btnOpenFile.addEventListener('click', async () => {
            try {
                dialogResult.textContent = 'Waiting for file selection...';
                const selected = await aura.dialog.open({
                    title: 'Select File (AuraUI Native Dialog)',
                    multiple: false
                });

                if (selected) {
                    dialogResult.innerHTML = `Selected: <code>${escapeHtml(selected)}</code>`;
                } else {
                    dialogResult.textContent = 'File dialog was cancelled.';
                }
            } catch (err) {
                dialogResult.textContent = `Native dialog: ${err.message || err}`;
            }
        });
    }

    if (btnCopyLink && dialogResult) {
        btnCopyLink.addEventListener('click', async () => {
            const textToCopy = 'https://github.com/Byukusenge-Andrew/auraui';
            try {
                if (window.aura && aura.clipboard && aura.clipboard.writeText) {
                    await aura.clipboard.writeText(textToCopy);
                } else if (navigator.clipboard) {
                    await navigator.clipboard.writeText(textToCopy);
                }
                dialogResult.innerHTML = `✓ Copied repository URL to clipboard!`;
                setTimeout(() => {
                    dialogResult.textContent = 'Click a button to test native OS integration';
                }, 3500);
            } catch (err) {
                dialogResult.textContent = `Clipboard error: ${err.message || err}`;
            }
        });
    }

    // Utility: HTML Escaper for safe rendering
    function escapeHtml(str) {
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#039;');
    }
});
