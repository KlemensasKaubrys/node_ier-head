let eventSource;
let updateRate = 1; // Default update rate

function startEventSource() {
    if (eventSource) {
        eventSource.close();
    }
    eventSource = new EventSource('/backend/cpu_usage');
    eventSource.onmessage = function(e) {
        document.getElementById('cpuUsage').innerText = parseFloat(e.data).toFixed(2) + '%';
    };
    eventSource.onerror = function() {
        document.getElementById('cpuUsage').innerText = 'Connection Error';
    };
}

function updateRateFunction() {
    const rateInput = document.getElementById('updateRate');
    updateRate = parseInt(rateInput.value);
    if (isNaN(updateRate) || updateRate < 1) {
        updateRate = 1;
        rateInput.value = 1;
    }
    // Restart the backend with the new update rate
    // Note: For this to work, the backend needs to support per-client update rates or receive the update rate via query parameters
    // For now, we can refresh the event source
    startEventSource();
}

// Start the event source when the page loads
window.onload = function() {
    startEventSource();
};

