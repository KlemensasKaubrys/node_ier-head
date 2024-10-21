let eventSource;
let updateRate = 1000; // Default update rate in milliseconds

function startEventSource() {
    if (eventSource) {
        eventSource.close();
    }
    // Include the update rate as a query parameter
    eventSource = new EventSource('/backend/cpu_usage?rate=' + updateRate);
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
    if (isNaN(updateRate) || updateRate < 100) {
        updateRate = 1000; // Default to 1000 milliseconds if invalid
        rateInput.value = 1000;
    }
    // Restart the EventSource with the new update rate
    startEventSource();
}

// Start the event source when the page loads
window.onload = function() {
    startEventSource();
};

