#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>

// Nama WiFi (SSID), password, dan ThingSpeak API Key
const char* WIFI_SSID          = "ISI DENGAN NAMA WIFI SENDIRI";
const char* WIFI_PASS          = "ISI DENGAN PW WIFI SENDIRI";
const char* THINGSPEAK_API_KEY = "ISI DENGAN API KEY PUNYA SENDIRI";

ESP8266WebServer server(80);
SoftwareSerial unoSerial(D5, D6); // RX = D5 (Pin 12 Uno), TX = D6 (Pin 11 Uno)

// Data terbaru dari Arduino Uno
String statusMode = "MENUNGGU";
int isGelap    = 0;
int gerak      = 0;
int sisaDetik  = 0;
int suaraDB    = 0;
int lampuLDR   = 0;
int lampuPIR   = 0;
int lampuSuara = 0;
int buzzer     = 0;

// Timer pengiriman ke ThingSpeak (minimal 15 detik untuk akun gratis, kita pakai 20 detik)
unsigned long waktuKirimCloud = 0;
const unsigned long INTERVAL_CLOUD = 20000;

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Street Lighting</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; }
  body { background: #0f172a; color: #f8fafc; padding: 16px; min-height: 100vh; }
  .container { max-width: 460px; margin: 0 auto; }
  .header { text-align: center; margin-bottom: 18px; }
  .header h1 { font-size: 1.35rem; font-weight: 700; color: #38bdf8; letter-spacing: 0.5px; }
  .mode-bar { display: inline-flex; align-items: center; gap: 8px; margin-top: 8px; background: #1e293b; padding: 6px 14px; border-radius: 999px; font-size: 0.85rem; border: 1px solid #334155; }
  .dot { width: 8px; height: 8px; border-radius: 50%; background: #22c55e; box-shadow: 0 0 8px #22c55e; }
  .card { background: #1e293b; border: 1px solid #334155; border-radius: 14px; padding: 14px; margin-bottom: 14px; box-shadow: 0 4px 12px rgba(0,0,0,0.25); }
  .card-title { font-size: 0.8rem; text-transform: uppercase; letter-spacing: 1px; color: #94a3b8; margin-bottom: 12px; font-weight: 600; }
  .sensor-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }
  .sensor-item { background: #0f172a; padding: 10px 8px; border-radius: 10px; text-align: center; border: 1px solid #1e293b; }
  .sensor-label { font-size: 0.72rem; color: #94a3b8; margin-bottom: 4px; }
  .sensor-val { font-size: 0.95rem; font-weight: 700; color: #f8fafc; }
  .val-active { color: #facc15; }
  .val-alert { color: #f87171; }
  .db-bar-bg { width: 100%; height: 5px; background: #334155; border-radius: 4px; margin-top: 6px; overflow: hidden; }
  .db-bar-fill { height: 100%; width: 0%; background: #38bdf8; transition: width 0.3s, background 0.3s; }
  .ctrl-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 0; border-bottom: 1px solid #334155; }
  .ctrl-row:last-child { border-bottom: none; padding-bottom: 0; }
  .ctrl-info { display: flex; align-items: center; gap: 10px; }
  .badge { font-size: 0.7rem; font-weight: 700; padding: 3px 8px; border-radius: 6px; background: #334155; color: #94a3b8; min-width: 40px; text-align: center; }
  .badge.on { background: rgba(250, 204, 21, 0.2); color: #facc15; border: 1px solid rgba(250, 204, 21, 0.4); }
  .badge.buz-on { background: rgba(248, 113, 113, 0.2); color: #f87171; border: 1px solid rgba(248, 113, 113, 0.4); }
  .ctrl-name { font-size: 0.92rem; font-weight: 600; }
  .btn-group { display: flex; gap: 6px; }
  button { border: none; border-radius: 8px; padding: 7px 14px; font-size: 0.82rem; font-weight: 600; cursor: pointer; transition: transform 0.1s, opacity 0.2s; }
  button:active { transform: scale(0.95); }
  .btn-on { background: #16a34a; color: #fff; }
  .btn-off { background: #475569; color: #f8fafc; }
  .btn-auto { width: 100%; padding: 11px; background: linear-gradient(135deg, #0284c7, #2563eb); color: #fff; font-size: 0.92rem; margin-bottom: 10px; }
  .btn-all-wrap { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
  .btn-all-on { background: #eab308; color: #0f172a; padding: 9px; }
  .btn-all-off { background: #dc2626; color: #fff; padding: 9px; }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>Smart Street Lighting</h1>
    <div class="mode-bar">
      <span class="dot"></span>
      <span id="mode">MENUNGGU</span> • <span id="timer">0 dtk</span>
    </div>
  </div>

  <div class="card">
    <div class="card-title">Monitoring Sensor</div>
    <div class="sensor-grid">
      <div class="sensor-item">
        <div class="sensor-label">Cahaya (LDR)</div>
        <div class="sensor-val" id="s-ldr">-</div>
      </div>
      <div class="sensor-item">
        <div class="sensor-label">Gerak (PIR)</div>
        <div class="sensor-val" id="s-pir">-</div>
      </div>
      <div class="sensor-item">
        <div class="sensor-label">Suara</div>
        <div class="sensor-val" id="s-db">0 dB</div>
        <div class="db-bar-bg"><div class="db-bar-fill" id="db-bar"></div></div>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="card-title">Kontrol Cepat</div>
    <button class="btn-auto" onclick="cmd('/auto')">Aktifkan Mode OTOMATIS (Sensor)</button>
    <div class="btn-all-wrap">
      <button class="btn-all-on" onclick="cmd('/lampu_on')">Semua Lampu ON</button>
      <button class="btn-all-off" onclick="cmd('/lampu_off')">Semua Lampu OFF</button>
    </div>
  </div>

  <div class="card">
    <div class="card-title">Kontrol Manual Per Perangkat</div>
    <div class="ctrl-row">
      <div class="ctrl-info">
        <span class="badge" id="b-l1">OFF</span>
        <span class="ctrl-name">Lampu 1 (LDR)</span>
      </div>
      <div class="btn-group">
        <button class="btn-on" onclick="cmd('/l1_on')">ON</button>
        <button class="btn-off" onclick="cmd('/l1_off')">OFF</button>
      </div>
    </div>
    <div class="ctrl-row">
      <div class="ctrl-info">
        <span class="badge" id="b-l2">OFF</span>
        <span class="ctrl-name">Lampu 2 (PIR)</span>
      </div>
      <div class="btn-group">
        <button class="btn-on" onclick="cmd('/l2_on')">ON</button>
        <button class="btn-off" onclick="cmd('/l2_off')">OFF</button>
      </div>
    </div>
    <div class="ctrl-row">
      <div class="ctrl-info">
        <span class="badge" id="b-l3">OFF</span>
        <span class="ctrl-name">Lampu 3 (Suara)</span>
      </div>
      <div class="btn-group">
        <button class="btn-on" onclick="cmd('/l3_on')">ON</button>
        <button class="btn-off" onclick="cmd('/l3_off')">OFF</button>
      </div>
    </div>
    <div class="ctrl-row">
      <div class="ctrl-info">
        <span class="badge" id="b-buz">OFF</span>
        <span class="ctrl-name">Buzzer Alarm</span>
      </div>
      <div class="btn-group">
        <button class="btn-on" onclick="cmd('/buzzer_on')">ON</button>
        <button class="btn-off" onclick="cmd('/buzzer_off')">OFF</button>
      </div>
    </div>
  </div>
</div>

<script>
function cmd(u) { fetch(u).then(() => update()); }
function setBadge(id, state, isBuz) {
  const el = document.getElementById(id);
  el.textContent = state ? 'ON' : 'OFF';
  el.className = 'badge' + (state ? (isBuz ? ' buz-on' : ' on') : '');
}
function update() {
  fetch('/data').then(r => r.json()).then(d => {
    document.getElementById('mode').textContent = d.mode;
    document.getElementById('timer').textContent = d.sisaDetik + ' dtk';

    const ldr = document.getElementById('s-ldr');
    ldr.textContent = d.gelap ? 'GELAP' : 'TERANG';
    ldr.className = 'sensor-val' + (d.gelap ? ' val-active' : '');

    const pir = document.getElementById('s-pir');
    pir.textContent = d.gerak ? 'GERAK' : 'SEPI';
    pir.className = 'sensor-val' + (d.gerak ? ' val-active' : '');

    const dbEl = document.getElementById('s-db');
    dbEl.textContent = d.suaraDB + ' dB';
    dbEl.className = 'sensor-val' + (d.suaraDB >= 65 ? ' val-alert' : '');

    const bar = document.getElementById('db-bar');
    bar.style.width = Math.min(100, Math.max(0, d.suaraDB)) + '%';
    bar.style.background = d.suaraDB >= 65 ? '#f87171' : '#38bdf8';

    setBadge('b-l1', d.lampuLDR, false);
    setBadge('b-l2', d.lampuPIR, false);
    setBadge('b-l3', d.lampuSuara, false);
    setBadge('b-buz', d.buzzer, true);
  }).catch(() => {});
}
setInterval(update, 1000);
update();
</script>
</body>
</html>
)rawliteral";

void bacaSerialUno() {
  while (unoSerial.available() > 0) {
    String data = unoSerial.readStringUntil('\n');
    data.trim();
    if (data.length() == 0) continue;

    int idx[8];
    int pos = 0;
    bool valid = true;
    for (int i = 0; i < 8; i++) {
      idx[i] = data.indexOf('|', pos);
      if (idx[i] == -1) {
        valid = false;
        break;
      }
      pos = idx[i] + 1;
    }

    if (valid) {
      statusMode = data.substring(0, idx[0]);
      isGelap    = data.substring(idx[0] + 1, idx[1]).toInt();
      gerak      = data.substring(idx[1] + 1, idx[2]).toInt();
      sisaDetik  = data.substring(idx[2] + 1, idx[3]).toInt();
      suaraDB    = data.substring(idx[3] + 1, idx[4]).toInt();
      lampuLDR   = data.substring(idx[4] + 1, idx[5]).toInt();
      lampuPIR   = data.substring(idx[5] + 1, idx[6]).toInt();
      lampuSuara = data.substring(idx[6] + 1, idx[7]).toInt();
      buzzer     = data.substring(idx[7] + 1).toInt();
    }
  }
}

void kirimKeThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;

  http.begin(client, "http://api.thingspeak.com/update.json");
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = "{";
  jsonPayload += "\"api_key\":\"" + String(THINGSPEAK_API_KEY) + "\",";
  jsonPayload += "\"field1\":" + String(isGelap) + ",";
  jsonPayload += "\"field2\":" + String(gerak) + ",";
  jsonPayload += "\"field3\":" + String(suaraDB) + ",";
  jsonPayload += "\"field4\":" + String(lampuLDR) + ",";
  jsonPayload += "\"field5\":" + String(lampuPIR) + ",";
  jsonPayload += "\"field6\":" + String(lampuSuara) + ",";
  jsonPayload += "\"field7\":" + String(buzzer) + ",";
  jsonPayload += "\"status\":\"mode=" + statusMode + "\"";
  jsonPayload += "}";

  int httpCode = http.POST(jsonPayload);
  Serial.print("ThingSpeak HTTP Code: ");
  Serial.println(httpCode);
  http.end();
}

void handleData() {
  String json = "{";
  json += "\"mode\":\"" + statusMode + "\",";
  json += "\"gelap\":" + String(isGelap) + ",";
  json += "\"gerak\":" + String(gerak) + ",";
  json += "\"sisaDetik\":" + String(sisaDetik) + ",";
  json += "\"suaraDB\":" + String(suaraDB) + ",";
  json += "\"lampuLDR\":" + String(lampuLDR) + ",";
  json += "\"lampuPIR\":" + String(lampuPIR) + ",";
  json += "\"lampuSuara\":" + String(lampuSuara) + ",";
  json += "\"buzzer\":" + String(buzzer);
  json += "}";
  server.send(200, "application/json", json);
}

void kirimPerintahUno(String cmd) {
  unoSerial.println(cmd);
  server.send(200, "text/plain", "OK: " + cmd);
}

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void setup() {
  Serial.begin(115200);
  unoSerial.begin(9600);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTerhubung! IP Address: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/auto", []() { kirimPerintahUno("CMD:AUTO"); });
  server.on("/lampu_on", []() { kirimPerintahUno("CMD:LAMPU_ON"); });
  server.on("/lampu_off", []() { kirimPerintahUno("CMD:LAMPU_OFF"); });
  server.on("/l1_on", []() { kirimPerintahUno("CMD:L1_ON"); });
  server.on("/l1_off", []() { kirimPerintahUno("CMD:L1_OFF"); });
  server.on("/l2_on", []() { kirimPerintahUno("CMD:L2_ON"); });
  server.on("/l2_off", []() { kirimPerintahUno("CMD:L2_OFF"); });
  server.on("/l3_on", []() { kirimPerintahUno("CMD:L3_ON"); });
  server.on("/l3_off", []() { kirimPerintahUno("CMD:L3_OFF"); });
  server.on("/buzzer_on", []() { kirimPerintahUno("CMD:BUZZER_ON"); });
  server.on("/buzzer_off", []() { kirimPerintahUno("CMD:BUZZER_OFF"); });

  server.begin();
}

void loop() {
  bacaSerialUno();
  server.handleClient();

  unsigned long sekarang = millis();
  if (sekarang - waktuKirimCloud >= INTERVAL_CLOUD) {
    waktuKirimCloud = sekarang;
    kirimKeThingSpeak();
  }
}