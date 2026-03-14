const admin = require("firebase-admin");
const path = require("path");
const http = require("http");

//test

// ── Configuration ───────────────────────────────────────────
const CONFIG = {
  // Path where ESP32 pushes raw readings
  RAW_READINGS_PATH: "test",

  // Meter identifier
  METER_ID: "meter_001",

  // Time in ms
  HOURLY_INTERVAL_MS: 3600000,

  // Firebase RTDB URL (update this to match your project)
  DATABASE_URL:
    "https://test2-8c525-default-rtdb.europe-west1.firebasedatabase.app",
};

// ── Firebase Initialization ─────────────────────────────────
// Load service account from environment variable or local file 
let serviceAccount;
if (process.env.FIREBASE_SERVICE_ACCOUNT) {
  serviceAccount = JSON.parse(process.env.FIREBASE_SERVICE_ACCOUNT);
  console.log("Using service account from environment variable");
} else {
  serviceAccount = require(path.join(__dirname, "serviceAccountKey.json"));
  console.log("Using service account from local file");
}

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: CONFIG.DATABASE_URL,
});

const db = admin.database();

// References
const rawReadingsRef = db.ref(CONFIG.RAW_READINGS_PATH);
const currentReadingRef = db.ref("current_reading");
const logsRef = db.ref("logs");

// Keeps track of the latest reading in memory
let latestReading = null;

// ──  Listen for new readings ──────────────────────────────


rawReadingsRef.on("child_added", async (snapshot) => {
  const data = snapshot.val();
  const key = snapshot.key;

  if (!data) return;

  // Build the current reading object
  latestReading = {
    energy_kwh: data.energy_kwh ?? null,
    ts: data.ts ?? Date.now(),
  };

  try {
    // Overwrite the current_reading node with the latest values
    await currentReadingRef.set(latestReading);

    // Delete the processed raw reading
    await rawReadingsRef.child(key).remove();

    console.log(
      `[${new Date().toLocaleTimeString()}]  Current reading updated: ${latestReading.energy_kwh} kWh  (raw entry ${key} deleted)`
    );
  } catch (err) {
    console.error("Error processing reading:", err.message);
  }
});

// ── Save hourly snapshot ─────────────────────────────────


async function saveHourlyLog() {
  if (!latestReading) {
    console.log(
      `[${new Date().toLocaleTimeString()}]  No reading available yet, skipping hourly log.`
    );
    return;
  }

  const logEntry = {
    energy_kwh: latestReading.energy_kwh,
    ts: Date.now(),
  };

  try {
    const ref = await logsRef.push(logEntry);
    console.log(
      `[${new Date().toLocaleTimeString()}]  Hourly log saved: ${ref.key} → ${logEntry.energy_kwh} kWh`
    );
  } catch (err) {
    console.error("Error saving hourly log:", err.message);
  }
}

// Start the hourly timer
function startHourlyTimer() {
  console.log(
    `[${new Date().toLocaleTimeString()}]  Logs will be saved every ${CONFIG.HOURLY_INTERVAL_MS / 1000} seconds`
  );

  // Save first log after one interval, then repeat
  setInterval(saveHourlyLog, CONFIG.HOURLY_INTERVAL_MS);
}

// ── Start ────────────────────────────────────────────────────
console.log("╔══════════════════════════════════════════════════╗");
console.log("║   Smart Electric Meter — Backend Server          ║");
console.log("╠══════════════════════════════════════════════════╣");
console.log(`║  RTDB URL  : ${CONFIG.DATABASE_URL}`);
console.log(`║  Meter ID  : ${CONFIG.METER_ID}`);
console.log(`║  Listening : ${CONFIG.RAW_READINGS_PATH}/`);
console.log("╚══════════════════════════════════════════════════╝");
console.log("");

startHourlyTimer();

console.log(
  `[${new Date().toLocaleTimeString()}] Server running. Listening for ESP32 readings...\n`
);

// ── Health Check Server for cloud hosting ─────────
const PORT = process.env.PORT || 3000;
http.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "application/json" });
  res.end(JSON.stringify({
    status: "running",
    meter: CONFIG.METER_ID,
    latestReading: latestReading,
  }));
}).listen(PORT, () => {
  console.log(`[${new Date().toLocaleTimeString()}] Health check server on port ${PORT}`);
});
