function toggleLED() {
    fetch('/api/led/toggle', { method: 'POST' })
        .then(response => response.json())
        .then(data => alert(data.status));
}
