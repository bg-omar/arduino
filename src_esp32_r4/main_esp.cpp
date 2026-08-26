/*
 * Wall-Z Brain v0.6.0 — onboard ESP32-S3
 *
 * Responsibilities:
 *   - keep UNO R4 USB CDC/CMSIS-DAP bridge active via ESP_UNO_R4
 *   - WiFi + NTP + web dashboard
 *   - read structured telemetry from RA4M1 over the internal UART
 *   - online novelty / Q-learning state with persistent NVS storage
 *   - bounded PS4 imitation learning with persistent NVS storage
 *   - optional, explicitly armed low-authority autonomous suggestions
 *
 * Safety/authority model:
 *   active PS4 input > RA safety > selected RA robot mode / Brain.
 *   PS4 may stay connected permanently: only non-neutral input temporarily
 *   pauses Brain actuation. OPTIONS explicitly stops autonomy and opens the
 *   RA runtime autonomy menu. The RA4M1 remains final safety authority.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_uno_r4.h>
#include <cstdio>
#include <cstring>

#include "brain_core.h"
#include "brain_store.h"
#include "esp_status.h"
#include "imitation_memory.h"
#include "imitation_store.h"
#include "log_buffer.h"
#include "ra_link.h"
#include "real_time.h"
#include "vision_link.h"
#include "visual_memory.h"
#include "visual_store.h"
#include "secrets.h"

namespace {
WebServer server(80);
LogBuffer gatewayLog;
BrainCore brain;
VisualMemory visualMemory;
ImitationMemory imitationMemory;
uint16_t lastObservedGridSeq = 0;
uint32_t lastWifiRetryMs = 0;
uint32_t lastNtpPollMs = 0;
uint32_t lastBrainObserveMs = 0;
uint32_t lastBrainActionMs = 0;
uint32_t lastPersistMs = 0;
uint32_t lastVisionPingMs = 0;
uint32_t lastDemoLearnedMs = 0;
uint32_t lastImitationPersistMs = 0;
bool wifiWasConnected = false;
bool autonomyEnabled = false;
bool visionFsReady = false;
bool imitationPolicyEnabled = false;
bool imitationDirty = false;
bool manualOverrideWasActive = false;
ImitationPrediction lastImitationPrediction{};
BrainAction lastSuggested = BrainAction::Idle;
BrainContext lastContext = BrainContext::Calm;

constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kNtpPollMs = 1000;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kBrainActionPeriodMs = 850;
constexpr uint32_t kPersistPeriodMs = 300000; // reduce NVS wear
constexpr uint32_t kVisionPingPeriodMs = 5000;
constexpr uint32_t kImitationPersistMs = 60000;

const char kIndexHtml[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wall-Z Brain v0.6</title>
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
<h1>Wall-Z Brain v0.6.0</h1><small>RA4M1 owns motors/safety. Fisheye stores raw visual experience locally; ESP32-S3 adds semantic context, learning and policy.</small>
<div class="grid">
<section class="card"><h2>Robot</h2><dl>
<dt>RA link</dt><dd id="ra">-</dd><dt>distance</dt><dd id="dist">-</dd><dt>light L/R</dt><dd id="light">-</dd><dt>mic L/R</dt><dd id="mic">-</dd><dt>gyro mrad/s</dt><dd id="gyro">-</dd><dt>head</dt><dd id="head">-</dd><dt>manual</dt><dd id="manual">-</dd><dt>robot mode</dt><dd id="robotmode">-</dd><dt>brain armed</dt><dd id="armed">-</dd></dl>
<button class="good" onclick="post('/api/brain/arm?on=1')">ARM brain</button><button onclick="post('/api/brain/arm?on=0')">Disarm</button><button class="danger" onclick="post('/api/robot/stop')">STOP</button></section>
<section class="card"><h2>Fisheye vision + memory</h2><dl>
<dt>link</dt><dd id="visionlink">-</dd><dt>motion</dt><dd id="vmotion">-</dd><dt>attention x/y</dt><dd id="vxy">-</dd><dt>brightness</dt><dd id="vbright">-</dd><dt>contrast</dt><dd id="vcontrast">-</dd><dt>fps</dt><dd id="vfps">-</dd><dt>grid</dt><dd id="vgrid">-</dd><dt>concept</dt><dd id="vconcept">unknown</dd><dt>familiarity</dt><dd id="vfamiliarity">0</dd><dt>concept value</dt><dd id="vvalue">0</dd><dt>concepts</dt><dd id="vconcepts">0</dd><dt>camera SD</dt><dd id="vsd">-</dd><dt>SD used</dt><dd id="vsdused">-</dd><dt>stored frames</dt><dd id="vsdframes">0</dd><dt>SD events/errors</dt><dd id="vsdevents">0 / 0</dd></dl>
<button onclick="post('/api/vision/ping')">Ping camera</button><button onclick="post('/api/vision/snapshot')">Grid snapshot</button><button onclick="post('/api/vision/store')">Store raw frame</button><button onclick="post('/api/vision/threshold?v=18')">Default threshold</button><p><button class="good" onclick="post('/api/vision/storage?on=1')">Camera SD auto ON</button><button onclick="post('/api/vision/storage?on=0')">Camera SD auto OFF</button></p>
<p><input id="teachlabel" maxlength="15" placeholder="person / ball / door"><button class="good" onclick="teach()">Teach + store current view</button><button class="danger" onclick="post('/api/vision/concepts/reset')">Forget all</button></p><p><a href="/api/vision/dataset"><button>Download S3 TinyML grid data</button></a><button onclick="post('/api/vision/dataset/reset')">Clear dataset</button></p><small>Raw 160×120 grayscale PGM frames are stored locally on the fisheye microSD. The S3 retains compact grids/labels for learning and sends context back to the camera.</small></section>
<section class="card"><h2>Imitation learning</h2><dl>
<dt>PS4 demo</dt><dd id="idemostate">-</dd><dt>learned samples</dt><dd id="isamples">0</dd><dt>prediction</dt><dd id="ipred">idle</dd><dt>confidence</dt><dd id="iconf">0</dd><dt>nearest</dt><dd id="inear">-</dd><dt>policy</dt><dd id="ipolicy">shadow</dd></dl>
<button class="good" onclick="post('/api/imitation/policy?on=1')">Use imitation</button><button onclick="post('/api/imitation/policy?on=0')">Shadow only</button><button class="danger" onclick="post('/api/imitation/reset')">Forget driving</button><p><a href="/api/imitation/dataset"><button>Download PS4 dataset</button></a><button onclick="post('/api/imitation/dataset/reset')">Clear dataset</button></p><small>Drive Wall-Z normally with PS4. Active PS4 input temporarily overrides an armed Brain; neutral input returns authority to Brain automatically. OPTIONS stops autonomy and opens the RA mode menu.</small></section>
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
async function teach(){const v=document.getElementById('teachlabel').value.trim();if(!v)return;await post('/api/vision/teach?label='+encodeURIComponent(v));}
function bar(id,v){v=Math.max(0,Math.min(1,v));document.getElementById(id).style.width=(100*v).toFixed(0)+'%';document.getElementById(id+'v').textContent=v.toFixed(3)}
async function tick(){try{
 const [s,r,v,b,i,l]=await Promise.all([fetch('/api/status').then(x=>x.json()),fetch('/api/robot').then(x=>x.json()),fetch('/api/vision').then(x=>x.json()),fetch('/api/brain').then(x=>x.json()),fetch('/api/imitation').then(x=>x.json()),fetch('/api/log').then(x=>x.json())]);
 wifi.textContent=s.wifi;ip.textContent=s.ip;ssid.textContent=s.ssid;rssi.textContent=s.rssi+' dBm';heap.textContent=s.heap;ntp.textContent=s.ntp;
 ra.textContent=r.online?'online ('+r.age_ms+' ms)':'offline';dist.textContent=r.distance_mm<0?'n/a':r.distance_mm+' mm';light.textContent=r.light_l+' / '+r.light_r;mic.textContent=r.mic_l+' / '+r.mic_r;gyro.textContent=r.gx+' / '+r.gy+' / '+r.gz;head.textContent=r.head_xy+' / '+r.head_z;manual.textContent=r.manual?'ACTIVE':'no';robotmode.textContent=r.robot_mode?'ACTIVE':'no';armed.textContent=r.brain_armed?'YES':'no';armed.className=r.brain_armed?'armed':'off';ack.textContent=r.ack;
 visionlink.textContent=v.online?'online ('+v.age_ms+' ms)':'offline';vmotion.textContent=v.motion+'/1000';vxy.textContent=v.x+' / '+v.y;vbright.textContent=v.brightness;vcontrast.textContent=v.contrast;vfps.textContent=(v.fps_x10/10).toFixed(1);vgrid.textContent=v.grid_online?'online ('+v.grid_age_ms+' ms)':'offline';vconcept.textContent=v.concept;vfamiliarity.textContent=v.familiarity+'/1000';vvalue.textContent=v.concept_value;vconcepts.textContent=v.concepts;vsd.textContent=v.sd_status?(v.sd_mounted?'mounted':'missing'):'unknown';vsdused.textContent=v.sd_status?(v.sd_used_mb+' / '+v.sd_total_mb+' MB'):'-';vsdframes.textContent=v.sd_frames;vsdevents.textContent=v.sd_events+' / '+v.sd_errors;
 idemostate.textContent=i.demo_online?'active ('+i.demo_age_ms+' ms)':'idle';isamples.textContent=i.samples;ipred.textContent=i.prediction;iconf.textContent=i.confidence+'/1000';inear.textContent=i.nearest;ipolicy.textContent=i.policy?'enabled':'shadow';
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

BrainTelemetry fusedTelemetry(uint32_t now) {
    BrainTelemetry t = ra_link::telemetry();
    if (vision_link::online(now)) {
        const VisionTelemetry& v = vision_link::telemetry();
        t.vision_online = 1;
        t.vision_motion = v.motion;
        t.vision_x = v.x;
        t.vision_y = v.y;
        t.vision_brightness = v.brightness;
        t.vision_contrast = v.contrast;
        const VisualRecognition& vr = visualMemory.current();
        t.vision_familiarity = vr.index >= 0 ? vr.score : 0;
        t.vision_value = vr.index >= 0 ? vr.value_milli : 0;
    }
    return t;
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

void handleVision() {
    const uint32_t now = millis();
    const bool online = vision_link::online(now);
    const VisionTelemetry& v = vision_link::telemetry();
    const VisualRecognition& r = visualMemory.current();
    const bool haveSd = vision_link::hasStorageStatus();
    const CameraStorageStatus& sd = vision_link::storageStatus();
    char json[1024];
    snprintf(json, sizeof(json),
        "{\"online\":%s,\"age_ms\":%lu,\"motion\":%d,\"x\":%d,\"y\":%d,\"brightness\":%d,\"contrast\":%d,\"fps_x10\":%d,\"flags\":%lu,\"grid_online\":%s,\"grid_age_ms\":%lu,\"concept\":\"%s\",\"familiarity\":%d,\"concept_value\":%d,\"concepts\":%d,\"sd_status\":%s,\"sd_status_age_ms\":%lu,\"sd_mounted\":%s,\"sd_total_mb\":%lu,\"sd_used_mb\":%lu,\"sd_frames\":%lu,\"sd_events\":%lu,\"sd_errors\":%lu,\"message\":\"%s\"}",
        online ? "true" : "false", static_cast<unsigned long>(vision_link::telemetryAge(now)),
        v.motion, v.x, v.y, v.brightness, v.contrast, v.fps_x10, static_cast<unsigned long>(v.flags),
        vision_link::gridOnline(now) ? "true" : "false", static_cast<unsigned long>(vision_link::gridAge(now)),
        r.label, r.index >= 0 ? r.score : 0, r.index >= 0 ? r.value_milli : 0,
        visualMemory.conceptCount(), haveSd ? "true" : "false",
        static_cast<unsigned long>(vision_link::storageStatusAge(now)),
        haveSd && sd.mounted ? "true" : "false",
        static_cast<unsigned long>(haveSd ? sd.total_mb : 0),
        static_cast<unsigned long>(haveSd ? sd.used_mb : 0),
        static_cast<unsigned long>(haveSd ? sd.frames : 0),
        static_cast<unsigned long>(haveSd ? sd.events : 0),
        static_cast<unsigned long>(haveSd ? sd.errors : 0), vision_link::lastMessage());
    server.send(200, "application/json", json);
}

void handleVisionPing() { vision_link::ping(); server.send(200,"application/json","{\"ping\":true}"); }
void handleVisionThreshold() {
    const int value = server.hasArg("v") ? server.arg("v").toInt() : 18;
    vision_link::setThreshold(value);
    server.send(200,"application/json","{\"threshold\":true}");
}

void handleVisionSnapshot() {
    vision_link::requestSnapshot();
    server.send(200,"application/json","{\"snapshot\":true}");
}

void handleVisionStore() {
    const VisualRecognition& r = visualMemory.current();
    vision_link::requestStore("manual", r.index >= 0 ? r.label : "unknown");
    server.send(200,"application/json","{\"store\":true}");
}

void handleVisionStorage() {
    const bool on = !server.hasArg("on") || server.arg("on").toInt() != 0;
    vision_link::setStorageEnabled(on);
    vision_link::requestStorageStatus();
    server.send(200,"application/json",on?"{\"storage\":true}":"{\"storage\":false}");
}

bool appendVisionTrainingSample(const char* label) {
    if (!visionFsReady || !label || !vision_link::hasGrid()) return false;
    File f = SPIFFS.open("/vision_dataset.csv", FILE_APPEND);
    if (!f) return false;
    f.print(label); f.print(',');
    static const char hex[] = "0123456789ABCDEF";
    const auto& g = vision_link::grid();
    char pair[2];
    for (int i=0; i<WALLZ_SNAPSHOT_GRID_CELLS; ++i) {
        const uint8_t v = g.grid[i]; pair[0]=hex[v>>4]; pair[1]=hex[v&15]; f.write(reinterpret_cast<const uint8_t*>(pair),2);
    }
    f.println(); f.close(); return true;
}

void handleVisionTeach() {
    if (!server.hasArg("label")) { server.send(400,"application/json","{\"error\":\"label-required\"}"); return; }
    if (!vision_link::gridOnline(millis()) || !visualMemory.haveGrid()) {
        vision_link::requestSnapshot();
        server.send(409,"application/json","{\"error\":\"no-fresh-grid\"}");
        return;
    }
    const String label = server.arg("label");
    if (!visualMemory.teach(label.c_str())) {
        server.send(400,"application/json","{\"error\":\"invalid-label-or-full\"}");
        return;
    }
    visual_store::save(visualMemory);
    if (!appendVisionTrainingSample(label.c_str())) logLine("Visual concept taught; dataset write unavailable");
    else logLine("Visual concept taught + training sample");
    vision_link::sendContext(label.c_str(), 1000, 0, visualMemory.current().value_milli);
    vision_link::requestStore("teach", label.c_str());
    server.send(200,"application/json","{\"teach\":true,\"camera_store\":true}");
}

void handleVisionResetConcepts() {
    visualMemory.reset();
    visual_store::clear();
    logLine("Visual concepts cleared");
    server.send(200,"application/json","{\"reset\":true}");
}

void handleVisionConcepts() {
    String out = "{\"concepts\":[";
    bool first = true;
    for (int i=0; i<WALLZ_VISUAL_MAX_CONCEPTS; ++i) {
        const auto& c = visualMemory.conceptAt(i);
        if (!c.samples || !c.label[0]) continue;
        if (!first) out += ',';
        first = false;
        out += "{\"label\":\""; out += c.label;
        out += "\",\"samples\":"; out += String(c.samples);
        out += ",\"value\":"; out += String(c.value_milli); out += '}';
    }
    out += "]}";
    server.send(200,"application/json",out);
}

void handleVisionDataset() {
    if (!visionFsReady || !SPIFFS.exists("/vision_dataset.csv")) { server.send(404,"text/plain","no dataset"); return; }
    File f = SPIFFS.open("/vision_dataset.csv", FILE_READ);
    if (!f) { server.send(500,"text/plain","dataset open failed"); return; }
    server.sendHeader("Content-Disposition","attachment; filename=wallz_vision_dataset.csv");
    server.streamFile(f,"text/csv");
    f.close();
}
void handleVisionDatasetReset() {
    if (visionFsReady) SPIFFS.remove("/vision_dataset.csv");
    logLine("Visual TinyML dataset cleared");
    server.send(200,"application/json","{\"reset\":true}");
}

uint16_t currentConceptTag() {
    const VisualRecognition& r = visualMemory.current();
    return r.index >= 0 ? ImitationMemory::tagFromLabel(r.label) : 0;
}

bool appendImitationTrainingSample(const ManualDemonstration& d, const BrainTelemetry& t) {
    if (!visionFsReady) return false;
    const bool newFile = !SPIFFS.exists("/imitation_dataset.csv");
    File f = SPIFFS.open("/imitation_dataset.csv", FILE_APPEND);
    if (!f) return false;
    if (newFile) {
        f.println("ms,action,lx,ly,rx,ry,distance_mm,light_l,light_r,mic_l,mic_r,gx,gy,gz,head_xy,head_z,vision_motion,vision_x,vision_y,vision_familiarity,concept");
    }
    const ImitationAction a = ImitationMemory::actionFromManual(d);
    const VisualRecognition& r = visualMemory.current();
    f.print(static_cast<unsigned long>(d.ms)); f.print(',');
    f.print(ImitationMemory::actionName(a)); f.print(',');
    f.print(d.lx); f.print(','); f.print(d.ly); f.print(','); f.print(d.rx); f.print(','); f.print(d.ry); f.print(',');
    f.print(t.distance_mm); f.print(','); f.print(t.light_l); f.print(','); f.print(t.light_r); f.print(',');
    f.print(t.mic_l); f.print(','); f.print(t.mic_r); f.print(',');
    f.print(t.gyro_x_mrad); f.print(','); f.print(t.gyro_y_mrad); f.print(','); f.print(t.gyro_z_mrad); f.print(',');
    f.print(t.head_xy); f.print(','); f.print(t.head_z); f.print(',');
    f.print(t.vision_motion); f.print(','); f.print(t.vision_x); f.print(','); f.print(t.vision_y); f.print(','); f.print(t.vision_familiarity); f.print(',');
    f.println(r.index >= 0 && r.label[0] ? r.label : "unknown");
    f.close();
    return true;
}

void handleImitation() {
    const uint32_t now = millis();
    const BrainTelemetry t = fusedTelemetry(now);
    lastImitationPrediction = imitationMemory.predict(t, currentConceptTag());
    const bool demoOnline = ra_link::hasDemonstration() && ra_link::demonstrationAge(now) <= 600;
    char json[512];
    snprintf(json, sizeof(json),
        "{\"demo_online\":%s,\"demo_age_ms\":%lu,\"samples\":%d,\"accepted\":%lu,\"duplicates\":%lu,\"prediction\":\"%s\",\"confidence\":%d,\"nearest\":%d,\"neighbors\":%d,\"policy\":%s,\"threshold\":%d}",
        demoOnline ? "true" : "false", static_cast<unsigned long>(ra_link::demonstrationAge(now)),
        imitationMemory.count(), static_cast<unsigned long>(imitationMemory.accepted()),
        static_cast<unsigned long>(imitationMemory.rejectedDuplicate()),
        ImitationMemory::actionName(lastImitationPrediction.action), lastImitationPrediction.confidence,
        lastImitationPrediction.nearest_distance, lastImitationPrediction.neighbors,
        imitationPolicyEnabled ? "true" : "false", WALLZ_IMITATION_EXEC_THRESHOLD);
    server.send(200, "application/json", json);
}

void handleImitationPolicy() {
    imitationPolicyEnabled = server.hasArg("on") && server.arg("on").toInt() != 0;
    logLine(imitationPolicyEnabled ? "Imitation policy enabled (still requires Brain ARM)" : "Imitation policy shadow-only");
    server.send(200, "application/json", imitationPolicyEnabled ? "{\"policy\":true}" : "{\"policy\":false}");
}

void handleImitationReset() {
    imitationPolicyEnabled = false;
    imitationMemory.reset();
    imitation_store::clear();
    logLine("PS4 imitation memory cleared");
    server.send(200, "application/json", "{\"reset\":true}");
}

void handleImitationDataset() {
    if (!visionFsReady || !SPIFFS.exists("/imitation_dataset.csv")) { server.send(404,"text/plain","no dataset"); return; }
    File f = SPIFFS.open("/imitation_dataset.csv", FILE_READ);
    if (!f) { server.send(500,"text/plain","dataset open failed"); return; }
    server.sendHeader("Content-Disposition","attachment; filename=wallz_imitation_dataset.csv");
    server.streamFile(f,"text/csv");
    f.close();
}

void handleImitationDatasetReset() {
    if (visionFsReady) SPIFFS.remove("/imitation_dataset.csv");
    logLine("PS4 imitation dataset cleared");
    server.send(200,"application/json","{\"reset\":true}");
}

void handleBrain() {
    BrainTelemetry t = fusedTelemetry(millis());
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
        if (t.robot_mode) { server.send(409,"application/json","{\"error\":\"robot-mode-active\"}"); return; }
    }
    autonomyEnabled=on;
    ra_link::arm(on);
    logLine(on?"Brain autonomy ARM requested (shared PS4 control)":"Brain autonomy disarmed");
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
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    const BrainContext next=brain.classify(fusedTelemetry(millis()));
    brain.reward(v,next);
    visualMemory.rewardCurrent(v);
    brain_store::save(brain);
    visual_store::save(visualMemory);
    const VisualRecognition& vr = visualMemory.current();
    const char* rewardLabel = vr.index >= 0 ? vr.label : "unknown";
    vision_link::sendContext(rewardLabel, vr.index >= 0 ? vr.score : 0,
                             vr.index >= 0 ? 1000 - vr.score : 1000,
                             vr.index >= 0 ? vr.value_milli : 0);
    vision_link::requestStore(v >= 0.0f ? "reward_pos" : "reward_neg", rewardLabel);
    char line[64]; snprintf(line,sizeof(line),"Brain reward %.2f",v); logLine(line);
    server.send(200,"application/json","{\"reward\":true,\"camera_store\":true}");
}

void handleResetBrain() {
    autonomyEnabled=false; ra_link::arm(false);
    brain.reset(); brain_store::clear();
    logLine("Brain learning reset");
    server.send(200,"application/json","{\"reset\":true}");
}

void learnFromManualDemonstration(uint32_t now) {
    if (!ra_link::hasDemonstration() || ra_link::demonstrationAge(now) > 600) return;
    const ManualDemonstration& d = ra_link::demonstration();
    if (d.ms == lastDemoLearnedMs) return;
    lastDemoLearnedMs = d.ms;
    const BrainTelemetry t = fusedTelemetry(now);
    if (t.robot_mode != 0 || !ra_link::online(now)) return;
    if (imitationMemory.learn(d, t, currentConceptTag())) {
        imitationDirty = true;
        appendImitationTrainingSample(d, t);
    }
}

void processRaUserModeRequest() {
    const RaUserModeRequest req = ra_link::takeUserModeRequest();
    switch (req) {
        case RaUserModeRequest::Brain:
            autonomyEnabled = true;
            imitationPolicyEnabled = false;
            ra_link::arm(true);
            logLine("PS4 menu: Brain Auto");
            break;
        case RaUserModeRequest::Imitation:
            autonomyEnabled = true;
            imitationPolicyEnabled = true;
            ra_link::arm(true);
            logLine("PS4 menu: Brain Imitation");
            break;
        case RaUserModeRequest::SenseReact:
            autonomyEnabled = false;
            ra_link::arm(false);
            logLine("PS4 menu: Sense React (RA owns autonomy)");
            break;
        case RaUserModeRequest::FreeRoam:
            autonomyEnabled = false;
            ra_link::arm(false);
            logLine("PS4 menu: Free Roam (RA owns autonomy)");
            break;
        case RaUserModeRequest::Stop:
            autonomyEnabled = false;
            ra_link::arm(false);
            logLine("PS4 menu: autonomy stopped/manual");
            break;
        case RaUserModeRequest::None:
        default:
            break;
    }
}

bool executeImitationAction(ImitationAction action, const BrainTelemetry& t) {
    switch (action) {
        case ImitationAction::Forward: ra_link::move('F', 180); return true;
        case ImitationAction::Backward: ra_link::move('B', 160); return true;
        case ImitationAction::TurnLeft: ra_link::move('L', 150); return true;
        case ImitationAction::TurnRight: ra_link::move('R', 150); return true;
        case ImitationAction::LookLeft: ra_link::head(145, t.head_z); return true;
        case ImitationAction::LookRight: ra_link::head(35, t.head_z); return true;
        case ImitationAction::LookUp: ra_link::head(t.head_xy, 25); return true;
        case ImitationAction::LookDown: ra_link::head(t.head_xy, 140); return true;
        case ImitationAction::Idle:
        default: return false;
    }
}

void executeBrainAction(uint32_t now) {
    if (!autonomyEnabled || !ra_link::online(now) || !ra_link::hasTelemetry()) return;
    const BrainTelemetry t=fusedTelemetry(now);
    if (t.robot_mode) {
        autonomyEnabled=false;
        ra_link::arm(false);
        manualOverrideWasActive=false;
        logLine("Brain auto-disarm: RA robot mode");
        return;
    }
    if (t.manual) {
        // Shared control: learn from the demonstration, but do not actuate.
        // RA4M1 simultaneously cancels any outstanding Brain pulse without
        // overwriting the PS4 command. Brain stays armed.
        manualOverrideWasActive=true;
        return;
    }
    if (manualOverrideWasActive) {
        manualOverrideWasActive=false;
        // Resume on the first neutral telemetry frame rather than waiting for
        // the normal action cadence.
        lastBrainActionMs = now - kBrainActionPeriodMs;
        logLine("Brain resumed after PS4 override");
    }
    if (!t.brain_armed) return; // wait for RA acknowledgement / state update
    if (static_cast<uint32_t>(now-lastBrainActionMs)<kBrainActionPeriodMs) return;
    lastBrainActionMs=now;

    lastContext=brain.classify(t);

    // Obstacle stop has absolute priority over imitation. The RA still performs
    // its own sonar gate as the final authority, so this is defense-in-depth.
    if (lastContext == BrainContext::Obstacle) {
        lastSuggested = BrainAction::Stop;
        brain.markAction(lastContext, lastSuggested);
        ra_link::brake();
        return;
    }

    lastImitationPrediction = imitationMemory.predict(t, currentConceptTag());
    if (imitationPolicyEnabled &&
        lastImitationPrediction.action != ImitationAction::Idle &&
        lastImitationPrediction.confidence >= WALLZ_IMITATION_EXEC_THRESHOLD) {
        if (executeImitationAction(lastImitationPrediction.action, t)) return;
    }

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
    logLine("Wall-Z Brain v0.6.0 ESP32-S3 boot");
    visionFsReady = SPIFFS.begin(true);
    logLine(visionFsReady ? "Vision dataset FS ready" : "Vision dataset FS unavailable");
    ra_link::begin();
    vision_link::begin();
    logLine("Fisheye UART2 S3 GPIO41/42 @230400; camera SD local-first");
    if (brain_store::load(brain)) logLine("Brain memory restored");
    else logLine("Brain memory new");
    if (visual_store::load(visualMemory)) logLine("Visual concept memory restored");
    else logLine("Visual concept memory new");
    if (imitation_store::load(imitationMemory)) logLine("PS4 imitation memory restored");
    else logLine("PS4 imitation memory new");

    connectWifi(kWifiConnectTimeoutMs);

    server.on("/",HTTP_GET,handleRoot);
    server.on("/api/status",HTTP_GET,handleStatus);
    server.on("/api/log",HTTP_GET,handleLog);
    server.on("/api/robot",HTTP_GET,handleRobot);
    server.on("/api/vision",HTTP_GET,handleVision);
    server.on("/api/vision/ping",HTTP_POST,handleVisionPing);
    server.on("/api/vision/threshold",HTTP_POST,handleVisionThreshold);
    server.on("/api/vision/snapshot",HTTP_POST,handleVisionSnapshot);
    server.on("/api/vision/store",HTTP_POST,handleVisionStore);
    server.on("/api/vision/storage",HTTP_POST,handleVisionStorage);
    server.on("/api/vision/teach",HTTP_POST,handleVisionTeach);
    server.on("/api/vision/concepts",HTTP_GET,handleVisionConcepts);
    server.on("/api/vision/concepts/reset",HTTP_POST,handleVisionResetConcepts);
    server.on("/api/vision/dataset",HTTP_GET,handleVisionDataset);
    server.on("/api/vision/dataset/reset",HTTP_POST,handleVisionDatasetReset);
    server.on("/api/imitation",HTTP_GET,handleImitation);
    server.on("/api/imitation/policy",HTTP_POST,handleImitationPolicy);
    server.on("/api/imitation/reset",HTTP_POST,handleImitationReset);
    server.on("/api/imitation/dataset",HTTP_GET,handleImitationDataset);
    server.on("/api/imitation/dataset/reset",HTTP_POST,handleImitationDatasetReset);
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
    processRaUserModeRequest();
    ra_link::heartbeat(now);
    vision_link::poll(now);
    if (!vision_link::online(now) && static_cast<uint32_t>(now-lastVisionPingMs)>=kVisionPingPeriodMs) {
        lastVisionPingMs=now;
        vision_link::ping();
    }

    if (vision_link::hasGrid() && vision_link::grid().seq != lastObservedGridSeq) {
        lastObservedGridSeq = vision_link::grid().seq;
        const VisualRecognition vr = visualMemory.observe(vision_link::grid());
        const int familiarity = vr.index >= 0 ? vr.score : 0;
        const int visualNovelty = vr.index >= 0 ? 1000 - vr.score : 1000;
        int brainNovelty = static_cast<int>(brain.metrics().novelty * 1000.0f + 0.5f);
        if (brainNovelty < 0) brainNovelty = 0;
        if (brainNovelty > 1000) brainNovelty = 1000;
        const int novelty = visualNovelty > brainNovelty ? visualNovelty : brainNovelty;
        vision_link::sendContext(vr.index >= 0 ? vr.label : "unknown", familiarity, novelty,
                                 vr.index >= 0 ? vr.value_milli : 0);
    }

    if (ra_link::hasTelemetry() && static_cast<uint32_t>(now-lastBrainObserveMs)>=100) {
        lastBrainObserveMs=now;
        brain.observe(fusedTelemetry(now));
    }
    learnFromManualDemonstration(now);
    executeBrainAction(now);

    if (imitationDirty && static_cast<uint32_t>(now-lastImitationPersistMs)>=kImitationPersistMs) {
        lastImitationPersistMs=now;
        if (imitation_store::save(imitationMemory)) imitationDirty=false;
    }

    if (static_cast<uint32_t>(now-lastPersistMs)>=kPersistPeriodMs) {
        lastPersistMs=now;
        brain_store::save(brain);
        visual_store::save(visualMemory);
        imitation_store::save(imitationMemory);
        imitationDirty=false;
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
