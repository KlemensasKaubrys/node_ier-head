let eventSource;
let updateRate = 1; // Default update rate

function startEventSource() {
    if (eventSource) {
        eventSource.close();
    }
    eventSource = new EventSource('/backend/cpu_usage');
    eventSource.onmessage = function(e) {
        document.getElementById('cpuUsage').innerText = e.data + '%';
    };
    eventSource.onerror = function() {
        document.getElementById('cpuUsage').innerText = 'Connection Error';
    };
}

function updateRate() {
    const rateInput = document.getElementById('updateRate');
    updateRate = parseInt(rateInput.value);
    if (isNaN(updateRate) || updateRate < 1) {
        updateRate = 1;
        rateInput.value = 1;
    }
    // Send new update rate to backend (Not implemented in this simple example)
    // For now, we'll just restart the event source
    startEventSource();
}

// Start the event source when the page loads
window.onload = function() {
    startEventSource();
};

