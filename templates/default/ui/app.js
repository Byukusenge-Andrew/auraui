// Frontend Application Logic

document.addEventListener('DOMContentLoaded', () => {
    const numA = document.getElementById('num-a');
    const numB = document.getElementById('num-b');
    const opSelect = document.getElementById('op-select');
    const btnCalc = document.getElementById('btn-calc');
    const calcResult = document.getElementById('calc-result');

    btnCalc.addEventListener('click', async () => {
        const a = parseFloat(numA.value) || 0;
        const b = parseFloat(numB.value) || 0;
        const operation = opSelect.value;

        try {
            btnCalc.disabled = true;
            btnCalc.textContent = 'Calculating...';

            // Call C++ bound function seamlessly!
            const res = await aura.call('calculate', { a, b, operation });
            
            calcResult.innerHTML = `Result: <strong>${res.result}</strong>`;
        } catch (err) {
            calcResult.innerHTML = `<span style="color: var(--danger)">Error: ${err.message}</span>`;
        } finally {
            btnCalc.disabled = false;
            btnCalc.textContent = 'Execute in C++';
        }
    });
});
