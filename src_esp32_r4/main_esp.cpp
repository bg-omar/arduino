/*
 * Wall-Z Brain v0.1.0 — onboard ESP32-S3
 *
 * Responsibilities:
 *   - keep UNO R4 USB CDC/CMSIS-DAP bridge active via ESP_UNO_R4
 *   - WiFi + NTP + web dashboard
 *   - read structured telemetry from RA4M1 over the internal UART
 *   - online novelty / Q-learning state with persistent NVS storage
 *   - optional, explicitly armed low-authority autonomous suggestions
 *
 * Safety/authority model:
 *   PS4/manual + RA safety > existing RA robot modes > Brain.
 *   The RA4M1 enforces manual override, heartbeat timeout, sonar clearance,
 *   short movement deadlines and motor stop independently of this firmware.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_uno_r4.h>
#include <cstdio>
#include <cstring>

#include "brain_core.h"
#include "brain_store.h"
#include "esp_status.h"
#include "log_buffer.h"
#include "ra_link.h"
#include "real_time.h"
#include "secrets.h"

namespace {
WebServer server(80);
LogBuffer gatewayLog;
BrainCore brain;
uint32_t lastWifiRetryMs = 0;
uint32_t lastNtpPollMs = 0;
uint32_t lastBrainObserveMs = 0;
uint32_t lastBrainActionMs = 0;
uint32_t lastPersistMs = 0;
bool wifiWasConnected = false;
bool autonomyEnabled = false;
BrainAction lastSuggested = BrainAction::Idle;
BrainContext lastContext = BrainContext::Calm;

constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kNtpPollMs = 1000;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kBrainActionPeriodMs = 850;
constexpr uint32_t kPersistPeriodMs = 300000; // reduce NVS wear

const char kIndexHtml[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wall-Z Brain v0.1</title>
<style>
:root{color-scheme:dark} body{font-family:system-ui,sans-serif;margin:1rem;background:#101114;color:#eee;max-width:1000px}
h1{font-size:1.35rem;margin-bottom:.3rem} h2{font-size:1rem;margin-top:1.4rem}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:.8rem}.card{background:#1b1d22;border:1px solid #333;border-radius:10px;padding:.8rem}
dl{display:grid;grid-template-columns:8rem 1fr;gap:.2rem .6rem;margin:.2rem 0}dt{opacity:.65}dd{margin:0;font-family:ui-monospace,monospace}
button{margin:.2rem;padding:.45rem .7rem;border:1px solid #555;border-radius:7px;background:#262a31;color:#eee;cursor:pointer}.danger{border-color:#944}.good{border-color:#496}
pre{background:#08090b;padding:.65rem;min-height:7rem;white-space:pre-wrap;overflow:auto}.bar{height:8px;background:#333;border-radius:5px;overflow:hidden}.bar>i{display:block;height:100%;background:#aaa;width:0%}
small{opacity:.65}.armed{color:#8f8}.off{color:#aaa}.warn{color:#f99}
</style>
</head>
<body>
<h1>Wall-Z Brain v0.1.0</h1><small>RA4M1 owns motors/safety. ESP32-S3 observes, remembers and learns.</small>
<div class="grid">
<section class="card"><h2>Robot</h2><dl>
<dt>RA link</dt><dd id="ra">-</dd><dt>distance</dt><dd id="dist">-</dd><dt>light L/R</dt><dd id="light">-</dd><dt>mic L/R</dt><dd id="mic">-</dd><dt>gyro mrad/s</dt><dd id="gyro">-</dd><dt>head</dt><dd id="head">-</dd><dt>manual</dt><dd id="manual">-</dd><dt>robot mode</dt><dd id="robotmode">-</dd><dt>brain armed</dt><dd id="armed">-</dd></dl>
<button class="good" onclick="post('/api/brain/arm?on=1')">ARM brain</button><button onclick="post('/api/brain/arm?on=0')">Disarm</button><button class="danger" onclick="post('/api/robot/stop')">STOP</button></section>
<section class="card"><h2>Cognitive state</h2><dl>
<dt>context</dt><dd id="context">-</dd><dt>suggestion</dt><dd id="action">-</dd><dt>observations</dt><dd id="obs">-</dd><dt>rewards</dt><dd id="rewards">-</dd></dl>
<div>novelty <span id="noveltyv"></span><div class="bar"><i id="novelty"></i></div></div>
<div>curiosity <span id="curiosityv"></span><div class="bar"><i id="curiosity"></i></div></div>
<div>arousal <span id="arousalv"></span><div class="bar"><i id="arousal"></i></div></div>
<div>confidence <span id="confidencev"></span><div class="bar"><i id="confidence"></i></div></div>
<div>valence <span id="valencev"></span></div>
<p><button class="good" onclick="post('/api/brain/reward?v=1')">+ reward</button><button class="danger" onclick="post('/api/brain/reward?v=-1')">- reward</button><button onclick="post('/api/brain/reset')">Reset learning</button></p></section>
<section class="card"><h2>Network</h2><dl><dt>wifi</dt><dd id="wifi">-</dd><dt>ip</dt><dd id="ip">-</dd><dt>ssid</dt><dd id="ssid">-</dd><dt>rssi</dt><dd id="rssi">-</dd><dt>heap</dt><dd id="heap">-</dd><dt>ntp</dt><dd id="ntp">-</dd><dt>RA ack</dt><dd id="ack">-</dd></dl></section>
</div>
<h2>ESP log</h2><pre id="log"></pre>
<script>
async function post(u){try{await fetch(u,{method:'POST'});setTimeout(tick,80)}catch(e){}}
function bar(id,v){v=Math.max(0,Math.min(1,v));document.getElementById(id).style.width=(100*v).toFixed(0)+'%';document.getElementById(id+'v').textContent=v.toFixed(3)}
async function tick(){try{
 const [s,r,b,l]=await Promise.all([fetch('/api/status').then(x=>x.json()),fetch('/api/robot').then(x=>x.json()),fetch('/api/brain').then(x=>x.json()),fetch('/api/log').then(x=>x.json())]);
 wifi.textContent=s.wifi;ip.textContent=s.ip;ssid.textContent=s.ssid;rssi.textContent=s.rssi+' dBm';heap.textContent=s.heap;ntp.textContent=s.ntp;
 ra.textContent=r.online?'online ('+r.age_ms+' ms)':'offline';dist.textContent=r.distance_mm<0?'n/a':r.distance_mm+' mm';light.textContent=r.light_l+' / '+r.light_r;mic.textContent=r.mic_l+' / '+r.mic_r;gyro.textContent=r.gx+' / '+r.gy+' / '+r.gz;head.textContent=r.head_xy+' / '+r.head_z;manual.textContent=r.manual?'ACTIVE':'no';robotmode.textContent=r.robot_mode?'ACTIVE':'no';armed.textContent=r.brain_armed?'YES':'no';armed.className=r.brain_armed?'armed':'off';ack.textContent=r.ack;
 context.textContent=b.context;action.textContent=b.suggestion+(b.autonomy?' [AUTO]':' [shadow]');obs.textContent=b.observations;rewards.textContent=b.rewards;bar('novelty',b.novelty);bar('curiosity',b.curiosity);bar('arousal',b.arousal);bar('confidence',b.confidence);valence.textContent=b.valence.toFixed(3);document.getElementById('log').textContent=(l.lines||[]).join('\n');
}catch(e){}}
tick();setInterval(tick,500);
</script></body></html>)HTML";

void logLine(const char* text) {
    gatewayLog.appendln(text);
    USBSerial.println(text);
}

bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }

void printIpToLog() {
    char line[LogBuffer::kLineLength];
    snprintf(line, sizeof(line), "IP %s", WiFi.localIP().toString().c_str());
    logLine(line);
}

void connectWifi(uint32_t timeoutMs) {
    if (wifiConnected()) return;
    logLine("WiFi connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    const uint32_t start = millis();
    while (!wifiConnected() && (millis() - start) < timeoutMs) {
        delay(250);
        USBSerial.print('.');
    }
    USBSerial.println();
    if (wifiConnected()) {
        logLine("WiFi up");
        printIpToLog();
        wifiWasConnected = true;
        real_time::begin();
    } else {
        logLine("WiFi fail");
        wifiWasConnected = false;
    }
}

void copyBounded(char* dest, size_t cap, const char* src) {
    if (!dest || cap == 0) return;
    if (!src) { dest[0] = '\0'; return; }
    strncpy(dest, src, cap - 1);
    dest[cap - 1] = '\0';
}

void fillStatus(EspStatus& status, char* ipBuf, size_t ipCap, char* ssidBuf, size_t ssidCap, char* ntpBuf, size_t ntpCap) {
    if (wifiConnected()) {
        copyBounded(ipBuf, ipCap, WiFi.localIP().toString().c_str());
        copyBounded(ssidBuf, ssidCap, WiFi.SSID().c_str());
        status.wifi = "connected";
        status.rssi = WiFi.RSSI();
    } else {
        copyBounded(ipBuf, ipCap, "0.0.0.0");
        copyBounded(ssidBuf, ssidCap, SECRET_SSID);
        status.wifi = "disconnected";
        status.rssi = 0;
    }
    status.ip = ipBuf; status.ssid = ssidBuf;
    real_time::format(ntpBuf, ntpCap); status.ntp = ntpBuf;
    status.uptime_ms = millis(); status.heap = ESP.getFreeHeap();
}

void handleRoot() { server.send_P(200, "text/html", kIndexHtml); }

void handleStatus() {
    char ipBuf[20], ssidBuf[33], ntpBuf[16], json[384];
    EspStatus status;
    fillStatus(status, ipBuf, sizeof(ipBuf), ssidBuf, sizeof(ssidBuf), ntpBuf, sizeof(ntpBuf));
    if (!formatEspStatusJson(json, sizeof(json), status)) { server.send(500, "application/json", "{\"error\":\"status\"}"); return; }
    server.send(200, "application/json", json);
}

void handleLog() {
    const char* lines[LogBuffer::kLines];
    const int count = gatewayLog.packedCount();
    for (int i=0;i<count;++i) lines[i]=gatewayLog.packedLine(i);
    char json[768];
    if (!formatEspLogJson(json, sizeof(json), lines, count)) { server.send(500,"application/json","{\"error\":\"log\"}"); return; }
    server.send(200,"application/json",json);
}

void handleRobot() {
    const uint32_t now = millis();
    const bool online = ra_link::online(now);
    const BrainTelemetry& t = ra_link::telemetry();
    char json[640];
    snprintf(json, sizeof(json),
        "{\"online\":%s,\"age_ms\":%lu,\"distance_mm\":%d,\"light_l\":%d,\"light_r\":%d,\"mic_l\":%d,\"mic_r\":%d,\"gx\":%d,\"gy\":%d,\"gz\":%d,\"head_xy\":%d,\"head_z\":%d,\"manual\":%d,\"robot_mode\":%d,\"brain_armed\":%d,\"ack\":\"%s\"}",
        online?"true":"false", static_cast<unsigned long>(ra_link::telemetryAge(now)),
        t.distance_mm,t.light_l,t.light_r,t.mic_l,t.mic_r,t.gyro_x_mrad,t.gyro_y_mrad,t.gyro_z_mrad,
        t.head_xy,t.head_z,t.manual,t.robot_mode,t.brain_armed,ra_link::lastAck());
    server.send(200,"application/json",json);
}

void handleBrain() {
    BrainTelemetry t = ra_link::telemetry();
    const BrainMetrics& m = brain.metrics();
    const BrainContext ctx = brain.classify(t);
    const BrainAction suggestion = brain.suggest(t);
    char json[640];
    snprintf(json,sizeof(json),
        "{\"context\":\"%s\",\"suggestion\":\"%s\",\"autonomy\":%s,\"novelty\":%.5f,\"curiosity\":%.5f,\"arousal\":%.5f,\"confidence\":%.5f,\"valence\":%.5f,\"observations\":%lu,\"rewards\":%lu,\"actions\":%lu}",
        BrainCore::contextName(ctx),BrainCore::actionName(suggestion),autonomyEnabled?"true":"false",
        m.novelty,m.curiosity,m.arousal,m.confidence,m.valence,
        static_cast<unsigned long>(m.observations),static_cast<unsigned long>(m.rewards),static_cast<unsigned long>(m.actions));
    server.send(200,"application/json",json);
}

void handleArm() {
    const bool on = server.hasArg("on") && server.arg("on").toInt()!=0;
    if (on) {
        if (!ra_link::online(millis())) { server.send(409,"application/json","{\"error\":\"ra-offline\"}"); return; }
        const BrainTelemetry& t=ra_link::telemetry();
        if (t.manual || t.robot_mode) { server.send(409,"application/json","{\"error\":\"manual-or-robot-mode\"}"); return; }
    }
    autonomyEnabled=on;
    ra_link::arm(on);
    logLine(on?"Brain autonomy ARM requested":"Brain autonomy disarmed");
    server.send(200,"application/json",on?"{\"autonomy\":true}":"{\"autonomy\":false}");
}

void handleStop() {
    autonomyEnabled=false;
    ra_link::stop();
    logLine("Robot STOP requested");
    server.send(200,"application/json","{\"stop\":true}");
}

void handleReward() {
    float v=server.hasArg("v")?server.arg("v").toFloat():0.0f;
    if (v>1.0f) v=1.0f; if (v<-1.0f) v=-1.0f;
    const BrainContext next=brain.classify(ra_link::telemetry());
    brain.reward(v,next);
    brain_store::save(brain);
    char line[64]; snprintf(line,sizeof(line),"Brain reward %.2f",v); logLine(line);
    server.send(200,"application/json","{\"reward\":true}");
}

void handleResetBrain() {
    autonomyEnabled=false; ra_link::arm(false);
    brain.reset(); brain_store::clear();
    logLine("Brain learning reset");
    server.send(200,"application/json","{\"reset\":true}");
}

void executeBrainAction(uint32_t now) {
    if (!autonomyEnabled || !ra_link::online(now) || !ra_link::hasTelemetry()) return;
    const BrainTelemetry& t=ra_link::telemetry();
    if (t.manual || t.robot_mode) {
        autonomyEnabled=false;
        ra_link::arm(false);
        logLine("Brain auto-disarm: manual/robot mode");
        return;
    }
    if (!t.brain_armed) return; // wait for RA acknowledgement / state update
    if (static_cast<uint32_t>(now-lastBrainActionMs)<kBrainActionPeriodMs) return;
    lastBrainActionMs=now;

    lastContext=brain.classify(t);
    lastSuggested=brain.suggest(t);
    brain.markAction(lastContext,lastSuggested);
    switch(lastSuggested) {
        case BrainAction::Stop:
            ra_link::brake();
            break;
        case BrainAction::LookLeft: ra_link::head(145,t.head_z); break;
        case BrainAction::LookRight: ra_link::head(35,t.head_z); break;
        case BrainAction::CreepForward: ra_link::move('F',180); break;
        case BrainAction::TurnLeft: ra_link::move('L',150); break;
        case BrainAction::TurnRight: ra_link::move('R',150); break;
        case BrainAction::Idle:
        default: break;
    }
}

void handleNotFound(){server.send(404,"text/plain","not found");}
}

void setup() {
    esp_uno_r4_setup();
    delay(50);
    logLine("Wall-Z Brain v0.1.0 ESP32-S3 boot");
    ra_link::begin();
    if (brain_store::load(brain)) logLine("Brain memory restored");
    else logLine("Brain memory new");

    connectWifi(kWifiConnectTimeoutMs);

    server.on("/",HTTP_GET,handleRoot);
    server.on("/api/status",HTTP_GET,handleStatus);
    server.on("/api/log",HTTP_GET,handleLog);
    server.on("/api/robot",HTTP_GET,handleRobot);
    server.on("/api/brain",HTTP_GET,handleBrain);
    server.on("/api/brain/arm",HTTP_POST,handleArm);
    server.on("/api/brain/reward",HTTP_POST,handleReward);
    server.on("/api/brain/reset",HTTP_POST,handleResetBrain);
    server.on("/api/robot/stop",HTTP_POST,handleStop);
    server.onNotFound(handleNotFound);
    server.begin();
    logLine("HTTP :80");
    if (wifiConnected()) { USBSerial.print("Open http://"); USBSerial.println(WiFi.localIP()); }
}

void loop() {
    server.handleClient();
    const uint32_t now=millis();
    ra_link::poll(now);
    ra_link::heartbeat(now);

    if (ra_link::hasTelemetry() && static_cast<uint32_t>(now-lastBrainObserveMs)>=100) {
        lastBrainObserveMs=now;
        brain.observe(ra_link::telemetry());
    }
    executeBrainAction(now);

    if (static_cast<uint32_t>(now-lastPersistMs)>=kPersistPeriodMs) {
        lastPersistMs=now;
        brain_store::save(brain);
    }

    const bool nowUp=wifiConnected();
    if (nowUp && !wifiWasConnected) {logLine("WiFi up");printIpToLog();real_time::begin();}
    wifiWasConnected=nowUp;
    if (!nowUp && static_cast<uint32_t>(now-lastWifiRetryMs)>=kWifiRetryMs) {
        lastWifiRetryMs=now; logLine("WiFi retry"); WiFi.disconnect(); WiFi.mode(WIFI_STA); WiFi.begin(SECRET_SSID,SECRET_PASS);
    }
    if (static_cast<uint32_t>(now-lastNtpPollMs)>=kNtpPollMs) {lastNtpPollMs=now;real_time::poll();}
    delay(1);
}
