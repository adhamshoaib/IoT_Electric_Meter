const http = require("http");
const https = require("https");

// ── Configuration ───────────────────────────────────────────
// Change SERVER_URL to your Render URL for remote testing
const SERVER_URL = process.env.SERVER_URL || "http://localhost:3000";
const API_KEY = process.env.ESP32_API_KEY || "sem-dev-key-2026";

let energyKwh = 0.0;

function sendReading() {
  energyKwh += 0.0014;
  energyKwh = parseFloat(energyKwh.toFixed(4));

  const payload = JSON.stringify({
    energy_kwh: energyKwh,
    ts: Math.floor(Date.now() / 1000),
  });

  const url = new URL(`${SERVER_URL}/reading`);
  const client = url.protocol === "https:" ? https : http;

  const options = {
    hostname: url.hostname,
    port: url.port || (url.protocol === "https:" ? 443 : 80),
    path: "/reading",
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "x-api-key": API_KEY,
      "Content-Length": Buffer.byteLength(payload),
    },
  };

  const req = client.request(options, (res) => {
    let body = "";
    res.on("data", (chunk) => { body += chunk; });
    res.on("end", () => {
      if (res.statusCode === 200) {
        console.log(
          `[${new Date().toLocaleTimeString()}] Sent: ${energyKwh} kWh → ${body}`
        );
      } else {
        console.error(
          `[${new Date().toLocaleTimeString()}] Error ${res.statusCode}: ${body}`
        );
      }
    });
  });

  req.on("error", (err) => {
    console.error(`[${new Date().toLocaleTimeString()}] Connection error: ${err.message}`);
  });

  req.write(payload);
  req.end();
}

// Push a reading every 2 seconds
setInterval(sendReading, 2000);

console.log(`ESP32 Simulator running`);
console.log(`  Server : ${SERVER_URL}`);
console.log(`  API Key: ${API_KEY.substring(0, 6)}...`);
console.log(`  Sending a reading every 2 seconds...\n`);
