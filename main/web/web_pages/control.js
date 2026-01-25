async function refreshColor() {
  try {
    const res = await fetch('/api/color', { cache: 'no-store' });
    if (!res.ok) return;
    const c = await res.json();
    document.body.style.background = `rgb(${c.r}, ${c.g}, ${c.b})`;
    document.getElementById('color').textContent = `LED color: R=${c.r} G=${c.g} B=${c.b}`;
  } catch (err) {
    // Ignore transient errors.
  }
}

setInterval(refreshColor, 500);
refreshColor();
