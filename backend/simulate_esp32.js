const admin = require("firebase-admin");
const path = require("path");

const serviceAccount = require(path.join(__dirname, "serviceAccountKey.json"));

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL:
    "https://test2-8c525-default-rtdb.europe-west1.firebasedatabase.app",
});

const db = admin.database();
const testRef = db.ref("test/last");

let energyKwh = 0.0;

// Push a reading every 2 seconds
setInterval(async () => {
  energyKwh += 0.0014;
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

console.log(" ESP32 Simulator running — pushing a reading every 2 seconds...\n");
