// ============================================================
//  ESP32 Simulator — Pushes fake readings to Firebase RTDB
//  Use this to test server.js without the actual hardware.
//  Run:  node simulate_esp32.js
// ============================================================

const admin = require("firebase-admin");
const path = require("path");

const serviceAccount = require(path.join(__dirname, "serviceAccountKey.json"));

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL:
    "https://test2-8c525-default-rtdb.europe-west1.firebasedatabase.app",
});

const db = admin.database();
const testRef = db.ref("test");

let energyKwh = 0.0;

// Push a fake reading every 2 seconds (simulating ESP32)
setInterval(async () => {
  energyKwh += 0.0014; // Simulate energy accumulating
  energyKwh = parseFloat(energyKwh.toFixed(4));

  const reading = {
    energy_kwh: energyKwh,
    ts: Math.floor(Date.now() / 1000),
  };

  try {
    const ref = await testRef.push(reading);
    console.log(
      `[${new Date().toLocaleTimeString()}] Pushed: ${energyKwh} kWh  (key: ${ref.key})`
    );
  } catch (err) {
    console.error("Error pushing reading:", err.message);
  }
}, 2000);

console.log("🔌 ESP32 Simulator running — pushing a reading every 2 seconds...\n");
