const admin = require("firebase-admin");
const path = require("path");
const http = require("http");

// ── Configuration ───────────────────────────────────────────
const CONFIG = {
  METER_ID: "meter_001",

  // Time in ms
  HOURLY_INTERVAL_MS: 3600000,       //3600000  //60000

  // Log retention (7 days in seconds)
  LOG_RETENTION_SECONDS: 7 * 24 * 3600,

  // API key for ESP32 authentication
  ESP32_API_KEY: process.env.ESP32_API_KEY || "sem-dev-key-2026",

  // Firebase RTDB URL
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
const currentReadingRef = db.ref("current_reading");
const logsRef = db.ref("logs");

// Keeps track of the latest reading in memory
let latestReading = null;

// ── Process a reading (used by both HTTP endpoint and legacy listener) ──

async function processReading(energy_kwh, ts) {
  latestReading = {
    energy_kwh: energy_kwh ?? null,
    ts: ts ?? Math.floor(Date.now() / 1000),
  };

  await currentReadingRef.set(latestReading);

  console.log(
    `[${new Date().toLocaleTimeString()}] Current reading updated: ${latestReading.energy_kwh} kWh`
  );
}

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
    ts: Math.floor(Date.now() / 1000),
  };

  try {
    const ref = await logsRef.push(logEntry);
    console.log(
      `[${new Date().toLocaleTimeString()}]  Hourly log saved: ${ref.key} → ${logEntry.energy_kwh} kWh`
    );
  } catch (err) {
    console.error("Error saving hourly log:", err.message);
  }

  // Delete logs older than 7 days
  const cutoff = Math.floor(Date.now() / 1000) - CONFIG.LOG_RETENTION_SECONDS;
  try {
    const oldLogs = await logsRef.orderByChild("ts").endAt(cutoff).once("value");
    if (oldLogs.exists()) {
      const updates = {};
      oldLogs.forEach((child) => { updates[child.key] = null; });
      await logsRef.update(updates);
      console.log(
        `[${new Date().toLocaleTimeString()}]  Pruned ${Object.keys(updates).length} logs older than 7 days`
      );
    }
  } catch (err) {
    console.error("Error pruning old logs:", err.message);
  }
}

// Start the hourly timer
function startHourlyTimer() {
  console.log(
    `[${new Date().toLocaleTimeString()}]  Logs will be saved every ${CONFIG.HOURLY_INTERVAL_MS / 1000} seconds`
  );
  setInterval(saveHourlyLog, CONFIG.HOURLY_INTERVAL_MS);
}

// ── HTTP Server (health check + ESP32 endpoint) ──────────────

function parseBody(req) {
  return new Promise((resolve, reject) => {
    let body = "";
    req.on("data", (chunk) => { body += chunk; });
    req.on("end", () => {
      try {
        resolve(JSON.parse(body));
      } catch (e) {
        reject(new Error("Invalid JSON"));
      }
    });
  });
}

const PORT = process.env.PORT || 3000;

const server = http.createServer(async (req, res) => {
  // ── Health check (GET /) ──
  if (req.method === "GET" && req.url === "/") {
    res.writeHead(200, { "Content-Type": "application/json" });                               //Success 
    return res.end(JSON.stringify({
      status: "running",
      meter: CONFIG.METER_ID,
      latestReading: latestReading,
    }));
  }

  // ── ESP32 reading endpoint (POST /reading) ──
  if (req.method === "POST" && req.url === "/reading") {
    // Check API key
    const apiKey = req.headers["x-api-key"];
    if (apiKey !== CONFIG.ESP32_API_KEY) {
      res.writeHead(401, { "Content-Type": "application/json" });                         //Unauthorized
      return res.end(JSON.stringify({ error: "Unauthorized: invalid API key" }));
    }

    try {
      const data = await parseBody(req);

      if (data.energy_kwh === undefined) {
        res.writeHead(400, { "Content-Type": "application/json" });                         //Bad Request
        return res.end(JSON.stringify({ error: "Missing energy_kwh field" }));
      }

      await processReading(data.energy_kwh, data.ts);

      res.writeHead(200, { "Content-Type": "application/json" });                         //Success
      return res.end(JSON.stringify({ status: "ok", received: latestReading }));
    } catch (err) {
      console.error("Error on /reading:", err.message);
      res.writeHead(500, { "Content-Type": "application/json" });                         //Internal Server Error
      return res.end(JSON.stringify({ error: err.message }));
    }
  }

  // ── 404 for everything else ──
  res.writeHead(404, { "Content-Type": "application/json" });                             //Not Found
  res.end(JSON.stringify({ error: "Not found" }));
});

// ── Start ────────────────────────────────────────────────────
console.log("╔══════════════════════════════════════════════════╗");
console.log("║   Smart Electric Meter — Backend Server          ║");
console.log("╠══════════════════════════════════════════════════╣");
console.log(`║  RTDB URL  : ${CONFIG.DATABASE_URL}`);
console.log(`║  Meter ID  : ${CONFIG.METER_ID}`);
console.log(`║  Endpoint  : POST /reading (API key required)`);
console.log("╚══════════════════════════════════════════════════╝");
console.log("");

startHourlyTimer();

server.listen(PORT, () => {
  console.log(`[${new Date().toLocaleTimeString()}] Server running on port ${PORT}`);
  console.log(`[${new Date().toLocaleTimeString()}] Waiting for ESP32 readings on POST /reading\n`);
});
