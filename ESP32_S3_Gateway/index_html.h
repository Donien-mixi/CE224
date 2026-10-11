#ifndef __INDEX_HTML_H__
#define __INDEX_HTML_H__

/* Giao diện điều khiển TWIP RACER — thiết kế cho điện thoại nằm ngang.
 * Không dùng thư viện/CDN ngoài (ESP32 chạy SoftAP không có Internet).
 * Giao thức giữ nguyên: $CMD,v,steer* · $PID · $TRIM · $YAW · $RACE · $BENCH · $CALIB · $SAVE · $STOP
 * Telemetry: $TEL,pitch,gyro,v_act,v_tgt,pwm_l,pwm_r,state,batt,v_l,v_r,theta_tgt */
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
<meta name="theme-color" content="#05080f">
<title>TWIP RACER</title>
<style>
  :root {
    --bg:#05080f; --panel:#0b1220; --panel2:#0f1829; --line:#1c2a44; --text:#e6f1ff; --dim:#6f82a3;
    --cyan:#00e5ff; --green:#22e58a; --amber:#ffb020; --red:#ff3860; --violet:#a78bfa;
  }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; }
  html,body { margin:0; height:100%; background:var(--bg); color:var(--text);
    font:14px/1.3 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
    overflow:hidden; overscroll-behavior:none; touch-action:none; user-select:none; -webkit-user-select:none; }
  button,input { font:inherit; color:inherit; }
  small { color:var(--dim); font-weight:600; }

  /* ---------- Khung chính ---------- */
  #app { height:100dvh; display:grid; grid-template-rows:auto auto 1fr;
    padding:env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left); }

  /* ---------- Thanh trên ---------- */
  #top { display:flex; align-items:center; gap:8px; padding:6px 10px; background:var(--panel); border-bottom:1px solid var(--line); }
  .badge { padding:4px 10px; border-radius:999px; font-weight:800; font-size:12px; letter-spacing:.5px; border:1px solid var(--line); white-space:nowrap; }
  .s-init,.s-standby { color:var(--amber); border-color:#5a4210; background:#1a1405; }
  .s-balancing { color:var(--green); border-color:#0d5a38; background:#04170f; }
  .s-racing { color:#ff6b9a; border-color:#6a1d3a; background:#1c0711; }
  .s-fallen { color:var(--amber); border-color:#6a4a10; background:#1d1404; }
  .s-emergency { color:#fff; border-color:var(--red); background:var(--red); animation:blink .6s infinite; }
  .s-bench { color:var(--violet); border-color:#3b2f73; background:#100c24; }
  .link-ok { color:var(--green); } .link-bad { color:var(--red); border-color:var(--red); }
  #top .sp { flex:1; }
  .stat { font-weight:800; font-variant-numeric:tabular-nums; }
  .stat small { font-size:11px; margin-left:2px; }
  .iconbtn { background:var(--panel2); border:1px solid var(--line); border-radius:10px; padding:7px 12px; font-weight:800; cursor:pointer; }
  .iconbtn:active { background:#16233b; }
  #estop { background:var(--red); border:0; color:#fff; border-radius:10px; padding:8px 14px; font-weight:900; letter-spacing:.5px; box-shadow:0 0 14px rgba(255,56,96,.45); cursor:pointer; }
  #estop:active { transform:scale(.96); }
  @keyframes blink { 50% { opacity:.45; } }

  /* ---------- Banner chẩn đoán ---------- */
  #diag { padding:5px 10px; font-size:12px; font-weight:700; border-bottom:1px solid var(--line); }
  #diag.info { background:#0a1424; color:#8fb3ff; } #diag.ok { background:#04170f; color:var(--green); }
  #diag.warn { background:#1a1405; color:var(--amber); } #diag.err { background:#220610; color:#ff7b95; }

  /* ---------- Khu lái xe ---------- */
  #drive { display:grid; grid-template-columns:minmax(150px,1fr) minmax(0,1.5fr) minmax(150px,1fr); gap:10px; padding:10px; min-height:0; }
  .zone { display:flex; flex-direction:column; align-items:center; justify-content:center; gap:8px; min-height:0; }
  .zone .cap { font-size:11px; font-weight:800; letter-spacing:1px; color:var(--dim); }
  .zone .val { font-size:22px; font-weight:900; font-variant-numeric:tabular-nums; }

  .stick { position:relative; touch-action:none; background:radial-gradient(circle at 50% 50%, #0f1b30 0%, #0a1322 70%); border:2px solid var(--line); }
  .stick.v { width:min(34vw,120px); height:min(52vh,220px); border-radius:60px; }
  .stick.h { width:min(34vw,220px); height:min(34vw,120px); max-height:120px; border-radius:60px; }
  .stick::before { content:""; position:absolute; background:#1c2a44; }
  .stick.v::before { left:50%; top:10px; bottom:10px; width:2px; transform:translateX(-1px); }
  .stick.h::before { top:50%; left:10px; right:10px; height:2px; transform:translateY(-1px); }
  .stick.on { border-color:var(--cyan); box-shadow:0 0 18px rgba(0,229,255,.25) inset; }
  .knob { position:absolute; left:50%; top:50%; width:64px; height:64px; margin:-32px 0 0 -32px; border-radius:50%;
    background:radial-gradient(circle at 35% 30%, #5ff3ff, #0090b8); box-shadow:0 4px 14px rgba(0,0,0,.6); pointer-events:none; }
  .stick:not(.on) .knob { transition:transform .12s ease-out; }

  .range { width:min(34vw,220px); }
  input[type=range] { width:100%; accent-color:var(--cyan); touch-action:pan-x; }
  .rowlab { display:flex; justify-content:space-between; width:100%; font-size:11px; font-weight:800; color:var(--dim); }
  .rowlab b { color:var(--text); }

  /* ---------- Trung tâm ---------- */
  #center { display:flex; flex-direction:column; gap:8px; min-height:0; min-width:0; }
  #tiles { display:grid; grid-template-columns:repeat(4,1fr); gap:6px; }
  .tile { background:var(--panel); border:1px solid var(--line); border-radius:10px; padding:6px 8px; min-width:0; }
  .tile .k { font-size:10px; font-weight:800; letter-spacing:.8px; color:var(--dim); }
  .tile .n { font-size:20px; font-weight:900; font-variant-numeric:tabular-nums; white-space:nowrap; }
  #pitchbar { position:relative; height:10px; border-radius:6px; background:linear-gradient(90deg,#3a0f1a,#0b2a1d 35%,#0b2a1d 65%,#3a0f1a); border:1px solid var(--line); }
  #pitchbar i { position:absolute; top:-3px; width:4px; height:16px; border-radius:2px; background:var(--cyan); transform:translateX(-2px); left:50%; }
  #pitchbar::after { content:""; position:absolute; left:50%; top:0; bottom:0; width:1px; background:#2c3f63; }
  #chartwrap { flex:1; min-height:80px; background:var(--panel); border:1px solid var(--line); border-radius:10px; position:relative; }
  #chart { width:100%; height:100%; display:block; }
  #legend { position:absolute; left:8px; top:4px; font-size:10px; font-weight:800; display:flex; gap:10px; pointer-events:none; }
  #modes { display:flex; gap:6px; }
  .mode { flex:1; background:var(--panel2); border:1px solid var(--line); border-radius:10px; padding:9px 4px; font-weight:800; font-size:12px; cursor:pointer; }
  .mode.on { color:#001018; background:var(--cyan); border-color:var(--cyan); }
  #m-race.on { background:#ff6b9a; border-color:#ff6b9a; }
  #m-bench.on { background:var(--violet); border-color:var(--violet); }

  /* ---------- Ngăn kéo Tune / Log ---------- */
  #sheet { position:fixed; inset:0; background:rgba(2,5,10,.82); display:none; z-index:20; touch-action:pan-y; }
  #sheet.open { display:flex; align-items:flex-end; justify-content:center; }
  #panel { width:min(720px,100%); max-height:92dvh; overflow:auto; background:var(--panel); border:1px solid var(--line); border-radius:16px 16px 0 0; padding:12px 14px calc(14px + env(safe-area-inset-bottom)); }
  #panel h3 { margin:12px 0 6px; font-size:12px; letter-spacing:1px; color:var(--dim); }
  .tabs { display:flex; gap:6px; position:sticky; top:-12px; background:var(--panel); padding:4px 0 8px; }
  .tab { flex:1; padding:9px; border-radius:10px; border:1px solid var(--line); background:var(--panel2); font-weight:800; cursor:pointer; }
  .tab.on { background:var(--cyan); color:#001018; border-color:var(--cyan); }
  .grid2 { display:grid; grid-template-columns:1fr 1fr; gap:8px; }
  .fld label { display:block; font-size:11px; font-weight:800; color:var(--dim); margin-bottom:3px; }
  .fld input[type=number] { width:100%; padding:10px; background:#060b14; border:1px solid var(--line); border-radius:8px; font-weight:800; }
  .btn { width:100%; padding:12px; border:0; border-radius:10px; font-weight:900; cursor:pointer; background:var(--panel2); border:1px solid var(--line); }
  .btn.pri { background:var(--cyan); color:#001018; border:0; }
  .btn.ok { background:var(--green); color:#001018; border:0; }
  .btn.warn { background:var(--amber); color:#1a1000; border:0; }
  .btn:disabled { opacity:.4; }
  .btn:active { transform:scale(.98); }
  .stack { display:grid; gap:8px; margin-top:8px; }
  .trim { display:grid; grid-template-columns:auto auto 1fr auto auto; gap:6px; align-items:center; }
  .trim .iconbtn { padding:9px 8px; }
  .note { font-size:11px; color:var(--dim); margin-top:6px; }
  .chk { display:flex; gap:8px; align-items:center; font-weight:700; margin-top:8px; }
  .hidden { display:none !important; }

  /* ---------- Dọc màn hình ---------- */
  @media (orientation:portrait) {
    #drive { grid-template-columns:1fr 1fr; grid-template-rows:auto auto; }
    #center { grid-column:1 / 3; }
    .stick.v { height:min(30vh,200px); }
    .stick.h { width:min(44vw,200px); }
    .range { width:min(44vw,200px); }
    #tiles { grid-template-columns:repeat(2,1fr); }
  }
  @media (max-height:420px) and (orientation:landscape) {
    .tile .n { font-size:17px; } #diag { display:none; }
  }
</style>
</head>
<body>
<div id="app">

  <!-- Thanh trên -->
  <div id="top">
    <span id="state-badge" class="badge s-init">---</span>
    <span id="link-badge" class="badge link-bad">NGAT</span>
    <span class="stat" id="t-batt">--<small>V</small></span>
    <span class="stat" id="t-rate">--<small>/s</small></span>
    <span class="sp"></span>
    <button class="iconbtn" onclick="openSheet()">TUNE &middot; LOG</button>
    <button id="estop" onclick="estop()">DUNG KHAN CAP</button>
  </div>

  <div id="diag" class="info"><span id="diag-text">Dang ket noi toi xe...</span></div>

  <div id="drive">

    <!-- Joystick GA (trái) -->
    <div class="zone">
      <div class="cap">GA &middot; TIEN / LUI</div>
      <div id="stick-t" class="stick v"><div class="knob"></div></div>
      <div class="val" id="v-cmd">0.00<small> m/s</small></div>
      <div class="range">
        <div class="rowlab"><span>TOC DO TOI DA</span><b id="vmax-val">0.60</b></div>
        <input id="vmax" type="range" min="0.10" max="1.50" step="0.05" value="0.60">
      </div>
    </div>

    <!-- Trung tâm -->
    <div id="center">
      <div id="tiles">
        <div class="tile"><div class="k">PITCH</div><div class="n" id="t-pitch">--</div></div>
        <div class="tile"><div class="k">V THUC</div><div class="n" id="t-vact">--</div></div>
        <div class="tile"><div class="k">PWM L / R</div><div class="n" id="t-pwm">--</div></div>
        <div class="tile"><div class="k">GYRO</div><div class="n" id="t-gyro">--</div></div>
      </div>
      <div id="pitchbar"><i id="pitch-ind"></i></div>
      <div id="chartwrap">
        <canvas id="chart"></canvas>
        <div id="legend"><span style="color:#00e5ff">pitch</span><span style="color:#a78bfa">theta dat</span><span style="color:#22e58a">v thuc</span><span style="color:#ffb020">v dat</span></div>
      </div>
      <div id="modes">
        <button id="m-race" class="mode" onclick="toggleRace()">DUA: TAT</button>
        <button id="m-bench" class="mode" onclick="toggleBench()">TEST BAN</button>
        <button class="mode" onclick="preset('smooth')">SMOOTH</button>
        <button class="mode" onclick="preset('race')">RACE</button>
      </div>
    </div>

    <!-- Joystick LÁI (phải) -->
    <div class="zone">
      <div class="cap">LAI &middot; TRAI / PHAI</div>
      <div id="stick-s" class="stick h"><div class="knob"></div></div>
      <div class="val" id="s-cmd">0<small> pwm</small></div>
      <div class="range">
        <div class="rowlab"><span>LUC LAI TOI DA</span><b id="smax-val">300</b></div>
        <input id="smax" type="range" min="100" max="800" step="50" value="300">
      </div>
    </div>
  </div>
</div>

<!-- Ngăn kéo -->
<div id="sheet" onclick="if(event.target===this)closeSheet()">
  <div id="panel">
    <div class="tabs">
      <button class="tab on" id="tab-tune" onclick="showTab('tune')">TUNE</button>
      <button class="tab" id="tab-log" onclick="showTab('log')">LOG</button>
      <button class="tab" onclick="closeSheet()">DONG</button>
    </div>

    <div id="pane-tune">
      <h3>VONG GOC (PD) &amp; VONG VAN TOC (PI)</h3>
      <div class="grid2">
        <div class="fld"><label>Kp1 goc</label><input type="number" id="kp1" value="54.1" step="0.5"></div>
        <div class="fld"><label>Kd1 gyro</label><input type="number" id="kd1" value="4.02" step="0.05"></div>
        <div class="fld"><label>Kp2 van toc</label><input type="number" id="kp2" value="4.0" step="0.1"></div>
        <div class="fld"><label>Ki2 van toc</label><input type="number" id="ki2" value="0.80" step="0.02"></div>
      </div>
      <h3>BU TRONG TAM (PITCH TRIM, DO)</h3>
      <div class="trim">
        <button class="iconbtn" onclick="adjustTrim(-0.1)">-0.1</button>
        <button class="iconbtn" onclick="adjustTrim(-0.02)">-0.02</button>
        <div class="fld"><input type="number" id="trim" value="0.00" step="0.02"></div>
        <button class="iconbtn" onclick="adjustTrim(0.02)">+0.02</button>
        <button class="iconbtn" onclick="adjustTrim(0.1)">+0.1</button>
      </div>
      <h3>YAW LOCK (PWM / deg/s, co dau, 0 = tat)</h3>
      <div class="fld"><input type="number" id="kyaw" value="0" step="0.5"></div>
      <div class="note">Neu bat Yaw lock ma xe xoay nhieu hon khi di thang, doi dau K Yaw.</div>
      <div class="stack">
        <button class="btn pri" onclick="sendTune()">GUI THAM SO XUONG XE</button>
        <button class="btn ok" onclick="sendSave()">LUU VAO FLASH ($SAVE)</button>
        <button class="btn warn" onclick="sendCalib()">HIEU CHUAN LAI IMU ($CALIB)</button>
      </div>
      <div class="note">Luu Flash chi hoat dong khi xe KHONG o BALANCING/RACING (dat xe nam yen). Coi ngan = thanh cong, coi dai = that bai/bi tu choi.</div>
    </div>

    <div id="pane-log" class="hidden">
      <h3>GHI DU LIEU TELEMETRY (20 Hz)</h3>
      <div class="grid2">
        <div class="tile"><div class="k">TRANG THAI</div><div class="n" id="log-state" style="font-size:16px">CHO GHI</div></div>
        <div class="tile"><div class="k">SO MAU</div><div class="n" id="log-count" style="font-size:16px">0</div></div>
        <div class="tile"><div class="k">|PITCH| MAX</div><div class="n" id="log-max" style="font-size:16px">0.0&deg;</div></div>
        <div class="tile"><div class="k">THOI LUONG</div><div class="n" id="log-dur" style="font-size:16px">0.0 s</div></div>
      </div>
      <label class="chk"><input type="checkbox" id="auto-rec" checked> Tu dong ghi khi xe can bang / test ban</label>
      <div class="stack">
        <button class="btn pri" id="btn-rec" onclick="toggleRec()">BAT DAU GHI</button>
        <div class="grid2">
          <button class="btn" id="btn-txt" onclick="exportLog('txt')" disabled>XUAT TXT</button>
          <button class="btn" id="btn-csv" onclick="exportLog('csv')" disabled>XUAT CSV</button>
        </div>
        <button class="btn" onclick="clearLog()">XOA BO DEM</button>
      </div>
    </div>
  </div>
</div>

<script>
(function () {
  "use strict";

  /* ======================= Trạng thái ======================= */
  var ws = null, wsOpen = false;
  var lastTel = 0, telCount = 0, lastState = -1;
  var raceMode = false, benchMode = false;
  var axisT = 0, axisS = 0;               // -1..+1 từ joystick/bàn phím
  var keys = { w:0, s:0, a:0, d:0 };
  var vmax = 0.60, smax = 300;
  var DEADZONE = 0.08, EXPO = 1.6;

  var STATE = ["INIT","CALIB","STANDBY","BALANCING","RACING","FALLEN","EMERGENCY","TEST BAN"];
  var SCLASS = ["s-init","s-init","s-standby","s-balancing","s-racing","s-fallen","s-emergency","s-bench"];

  function $(id) { return document.getElementById(id); }

  /* ======================= WebSocket ======================= */
  function initWS() {
    var proto = (location.protocol === "https:") ? "wss:" : "ws:";
    ws = new WebSocket(proto + "//" + location.hostname + ":81/");
    ws.onopen = function () { wsOpen = true; };
    ws.onclose = function () { wsOpen = false; setTimeout(initWS, 1000); };
    ws.onerror = function () {};
    ws.onmessage = function (e) {
      var m = String(e.data).trim();
      if (m.indexOf("$TEL,") === 0) onTelemetry(m);
    };
  }
  function send(txt) { if (ws && ws.readyState === 1) ws.send(txt); }

  /* Gửi lệnh ga/lái 20 Hz. STM32 tự về 0 sau 1 s nếu mất gói (failsafe). */
  function curV() { return shape(axisT) * vmax; }
  function curS() { return Math.round(shape(axisS) * smax); }
  function shape(a) {            // deadzone + expo để lái mịn ở biên độ nhỏ
    var m = Math.abs(a);
    if (m < DEADZONE) return 0;
    m = (m - DEADZONE) / (1 - DEADZONE);
    return (a < 0 ? -1 : 1) * Math.pow(m, EXPO);
  }
  setInterval(function () {
    var v = curV(), s = curS();
    send("$CMD," + v.toFixed(2) + "," + s + "*");
    $("v-cmd").innerHTML = v.toFixed(2) + "<small> m/s</small>";
    $("s-cmd").innerHTML = s + "<small> pwm</small>";
  }, 50);

  /* ======================= Joystick ảo ======================= */
  function makeStick(el, axis, onChange) {
    var knob = el.querySelector(".knob"), pid = null;
    function limit() {
      var r = el.getBoundingClientRect();
      return axis === "y" ? (r.height - 64) / 2 : (r.width - 64) / 2;
    }
    function move(e) {
      var r = el.getBoundingClientRect(), L = limit();
      var d = axis === "y" ? (e.clientY - (r.top + r.height / 2)) : (e.clientX - (r.left + r.width / 2));
      var n = Math.max(-1, Math.min(1, d / L));
      knob.style.transform = axis === "y" ? "translateY(" + (n * L) + "px)" : "translateX(" + (n * L) + "px)";
      // Kéo lên = tiến (+); kéo phải = lái phải (+)
      onChange(axis === "y" ? -n : n);
    }
    function end() {
      pid = null; el.classList.remove("on");
      knob.style.transform = "translate(0,0)";
      onChange(0);
    }
    el.addEventListener("pointerdown", function (e) {
      e.preventDefault();
      if (pid !== null) return;
      pid = e.pointerId; el.setPointerCapture(pid); el.classList.add("on"); move(e);
    });
    el.addEventListener("pointermove", function (e) { if (e.pointerId === pid) move(e); });
    el.addEventListener("pointerup", function (e) { if (e.pointerId === pid) end(); });
    el.addEventListener("pointercancel", function (e) { if (e.pointerId === pid) end(); });
    el.addEventListener("lostpointercapture", function (e) { if (e.pointerId === pid) end(); });
    el.addEventListener("contextmenu", function (e) { e.preventDefault(); });
    return end;
  }
  var stickAxisT = 0, stickAxisS = 0;
  function recompute() {
    axisT = keys.w - keys.s; if (!axisT) axisT = stickAxisT;
    axisS = keys.d - keys.a; if (!axisS) axisS = stickAxisS;
  }
  var endT = makeStick($("stick-t"), "y", function (v) { stickAxisT = v; recompute(); });
  var endS = makeStick($("stick-s"), "x", function (v) { stickAxisS = v; recompute(); });

  $("vmax").addEventListener("input", function (e) { vmax = parseFloat(e.target.value); $("vmax-val").textContent = vmax.toFixed(2); });
  $("smax").addEventListener("input", function (e) { smax = parseInt(e.target.value, 10); $("smax-val").textContent = smax; });

  /* ======================= Bàn phím (laptop) ======================= */
  var KEYMAP = { w:"w", arrowup:"w", s:"s", arrowdown:"s", a:"a", arrowleft:"a", d:"d", arrowright:"d" };
  function typing(e) { var t = e.target && e.target.tagName; return t === "INPUT" || t === "TEXTAREA"; }
  window.addEventListener("keydown", function (e) {
    if (typing(e)) return;
    var k = KEYMAP[e.key.toLowerCase()];
    if (k) { e.preventDefault(); keys[k] = 1; recompute(); }
    else if (e.key === " ") { e.preventDefault(); stopAll(); }
    else if (e.key === "Escape") { e.preventDefault(); estop(); }
  });
  window.addEventListener("keyup", function (e) {
    var k = KEYMAP[e.key.toLowerCase()];
    if (k) { keys[k] = 0; recompute(); }
  });

  /* ======================= An toàn: thả về 0 ======================= */
  function stopAll() {
    keys = { w:0, s:0, a:0, d:0 };
    stickAxisT = stickAxisS = 0; axisT = axisS = 0;
    endT(); endS();
    send("$CMD,0.00,0*");
  }
  window.addEventListener("blur", stopAll);
  document.addEventListener("visibilitychange", function () { if (document.hidden) stopAll(); });
  window.addEventListener("pagehide", stopAll);

  window.estop = function () {
    stopAll(); benchMode = false; raceMode = false; syncModes();
    send("$STOP*");
  };

  /* ======================= Chế độ ======================= */
  window.toggleRace = function () { raceMode = !raceMode; send("$RACE," + (raceMode ? 1 : 0) + "*"); syncModes(); };
  window.toggleBench = function () { benchMode = !benchMode; send("$BENCH," + (benchMode ? 1 : 0) + "*"); syncModes(); };
  function syncModes() {
    var r = $("m-race"), b = $("m-bench");
    r.textContent = "DUA: " + (raceMode ? "BAT" : "TAT"); r.className = "mode" + (raceMode ? " on" : "");
    b.className = "mode" + (benchMode ? " on" : "");
  }
  /* Preset giao diện: chỉ đổi tốc độ/lực lái tối đa + chế độ đua (KHÔNG đổi PID) */
  window.preset = function (name) {
    var race = (name === "race");
    vmax = race ? 1.50 : 0.60; smax = race ? 500 : 300;
    $("vmax").value = vmax; $("smax").value = smax;
    $("vmax-val").textContent = vmax.toFixed(2); $("smax-val").textContent = smax;
    if (raceMode !== race) toggleRace();
  };

  /* ======================= Tune ======================= */
  function num(id, d) { var v = parseFloat($(id).value); return isNaN(v) ? d : v; }
  window.sendTune = function () {
    send("$PID," + num("kp1",54.1) + "," + num("kd1",4.02) + "," + num("kp2",4.0) + "," + num("ki2",0.8) + "*");
    send("$TRIM," + num("trim",0) + "*");
    send("$YAW," + num("kyaw",0) + "*");
  };
  window.sendSave = function () {
    sendTune();
    setTimeout(function () { send("$SAVE*"); }, 250);   // chờ STM32 nhận xong 3 gói trên
  };
  window.adjustTrim = function (d) {
    var v = (num("trim",0) + d).toFixed(2); $("trim").value = v; send("$TRIM," + v + "*");
  };
  window.sendCalib = function () {
    if (confirm("Dat xe NAM YEN tren mat phang ~2.5 giay de hieu chuan Gyro. Tiep tuc?")) send("$CALIB*");
  };

  /* ======================= Ngăn kéo ======================= */
  window.openSheet = function () { stopAll(); $("sheet").className = "open"; };
  window.closeSheet = function () { $("sheet").className = ""; };
  window.showTab = function (t) {
    $("pane-tune").className = (t === "tune") ? "" : "hidden";
    $("pane-log").className = (t === "log") ? "" : "hidden";
    $("tab-tune").className = "tab" + (t === "tune" ? " on" : "");
    $("tab-log").className = "tab" + (t === "log" ? " on" : "");
  };

  /* ======================= Telemetry ======================= */
  var H = 200, hist = { p:[], v:[], t:[], th:[] };     // 200 mẫu = 10 s
  function onTelemetry(msg) {
    if (msg.charAt(msg.length - 1) === "*") msg = msg.slice(0, -1);
    var p = msg.substring(5).split(",");
    if (p.length < 8) return;
    var pitch = parseFloat(p[0]), gyro = parseFloat(p[1]), vAct = parseFloat(p[2]), vTgt = parseFloat(p[3]);
    var pwmL = parseInt(p[4], 10), pwmR = parseInt(p[5], 10), st = parseInt(p[6], 10), batt = parseFloat(p[7]);
    var vL = p.length > 8 ? parseFloat(p[8]) : vAct, vR = p.length > 9 ? parseFloat(p[9]) : vAct;
    var thT = p.length > 10 ? parseFloat(p[10]) : 0; if (isNaN(thT)) thT = 0;
    if (isNaN(pitch) || isNaN(vAct)) return;

    lastTel = Date.now(); telCount++; lastState = st;

    $("t-pitch").innerHTML = pitch.toFixed(1) + "<small>&deg;</small>";
    $("t-vact").innerHTML = vAct.toFixed(2) + "<small> m/s</small>";
    $("t-pwm").textContent = pwmL + " / " + pwmR;
    $("t-gyro").innerHTML = gyro.toFixed(0) + "<small>&deg;/s</small>";
    $("t-batt").innerHTML = batt.toFixed(1) + "<small>V</small>";

    var pct = Math.max(3, Math.min(97, 50 + pitch / 30 * 50));
    var ind = $("pitch-ind"); ind.style.left = pct + "%"; ind.style.background = Math.abs(pitch) > 20 ? "#ff3860" : "#00e5ff";

    var b = $("state-badge"); b.textContent = STATE[st] || "?"; b.className = "badge " + (SCLASS[st] || "s-standby");

    // Đồng bộ nút theo FSM thật của STM32
    var nb = (st === 7), nr = (st === 4);
    if (nb !== benchMode) { benchMode = nb; syncModes(); }
    if ((st === 3 || st === 4) && nr !== raceMode) { raceMode = nr; syncModes(); }

    push(hist.p, pitch); push(hist.v, vAct); push(hist.t, vTgt); push(hist.th, thT);
    if (!drawPending) { drawPending = true; requestAnimationFrame(draw); }

    // Ghi log
    if (autoRec) {
      var active = (st === 3 || st === 4 || st === 7);
      if (active && !recording) startRec(); else if (!active && recording) stopRec();
    }
    if (recording) {
      var ap = Math.abs(pitch); if (ap > maxPitch) maxPitch = ap;
      logBuf.push({ t: Date.now() - logT0, pitch: pitch, gyro: gyro, vAct: vAct, vL: vL, vR: vR, vTgt: vTgt,
                    pwmL: pwmL, pwmR: pwmR, thT: thT, state: STATE[st] || ("S" + st), batt: batt });
      if (logBuf.length % 5 === 0) logUI();
    }
  }
  function push(a, x) { a.push(x); if (a.length > H) a.shift(); }

  setInterval(function () {
    var fresh = (Date.now() - lastTel) < 800;
    var l = $("link-badge"); l.textContent = fresh ? "OK" : "MAT TIN HIEU"; l.className = "badge " + (fresh ? "link-ok" : "link-bad");
    $("t-rate").innerHTML = fresh ? telCount + "<small>/s</small>" : "--<small>/s</small>";
    telCount = 0;
    diagnose(fresh);
  }, 1000);

  function diagnose(fresh) {
    var cls = "info", txt = "";
    if (!wsOpen) { cls = "err"; txt = "Chua noi WebSocket. Kiem tra Wi-Fi TWIP_RACER_S3."; }
    else if (!fresh) { cls = "err"; txt = "Mat tin hieu tu STM32. Kiem tra day PB6->GPIO18, PB7->GPIO17 va GND chung."; }
    else switch (lastState) {
      case 0: txt = "STM32 dang khoi dong..."; break;
      case 1: cls = "warn"; txt = "Dang hieu chuan IMU. Giu xe dung yen."; break;
      case 2: cls = "warn"; txt = "STANDBY: dung xe thang (|pitch| < 15 do) de tu can bang, hoac bat TEST BAN."; break;
      case 3: cls = "ok"; txt = "Dang can bang. Keo joystick trai de chay, joystick phai de lai."; break;
      case 4: cls = "ok"; txt = "CHE DO DUA: dang can bang."; break;
      case 5: cls = "warn"; txt = "Xe nga (> 45 do), da ngat dong co. Dung xe thang de can bang lai."; break;
      case 6: cls = "err"; txt = "KHAN CAP: mat BMI160 hoac ket banh (PWM cao nhung xe dung). Reset xe de tiep tuc."; break;
      case 7: cls = "ok"; txt = "TEST BAN: quay thang 2 banh, bo qua can bang. Joystick phai = quay vi sai."; break;
      default: txt = "Dang nhan du lieu...";
    }
    var d = $("diag"); d.className = cls; $("diag-text").textContent = txt;
  }

  /* ======================= Biểu đồ realtime ======================= */
  var cv = $("chart"), cx = cv.getContext("2d"), drawPending = false;
  function resize() {
    var r = cv.getBoundingClientRect(), dpr = window.devicePixelRatio || 1;
    cv.width = Math.max(1, Math.round(r.width * dpr)); cv.height = Math.max(1, Math.round(r.height * dpr));
    cx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }
  window.addEventListener("resize", function () { resize(); draw(); });
  function trace(a, ymin, ymax, x0, y0, w, h, color) {
    if (a.length < 2) return;
    cx.beginPath(); cx.strokeStyle = color; cx.lineWidth = 1.6;
    for (var i = 0; i < a.length; i++) {
      var x = x0 + (i + (H - a.length)) / (H - 1) * w;
      var y = y0 + h - (Math.max(ymin, Math.min(ymax, a[i])) - ymin) / (ymax - ymin) * h;
      if (i === 0) cx.moveTo(x, y); else cx.lineTo(x, y);
    }
    cx.stroke();
  }
  function panel(y0, h, ymin, ymax, w) {
    cx.strokeStyle = "#16233b"; cx.lineWidth = 1; cx.strokeRect(6.5, y0 + 0.5, w - 13, h - 1);
    var z = y0 + h - (0 - ymin) / (ymax - ymin) * h;
    cx.strokeStyle = "#2c3f63"; cx.beginPath(); cx.moveTo(6, z); cx.lineTo(w - 6, z); cx.stroke();
  }
  function draw() {
    drawPending = false;
    var w = cv.clientWidth, h = cv.clientHeight;
    if (!w || !h) return;
    cx.clearRect(0, 0, w, h);
    var gap = 6, ph = (h - gap - 16) / 2, y1 = 14, y2 = y1 + ph + gap;
    panel(y1, ph, -25, 25, w);          // pitch ±25°
    panel(y2, ph, -1.6, 1.6, w);        // vận tốc ±1.6 m/s
    trace(hist.th, -25, 25, 8, y1, w - 16, ph, "#a78bfa");
    trace(hist.p, -25, 25, 8, y1, w - 16, ph, "#00e5ff");
    trace(hist.t, -1.6, 1.6, 8, y2, w - 16, ph, "#ffb020");
    trace(hist.v, -1.6, 1.6, 8, y2, w - 16, ph, "#22e58a");
  }

  /* ======================= Data logger ======================= */
  var recording = false, autoRec = true, logBuf = [], logT0 = 0, maxPitch = 0;
  function startRec() { recording = true; logBuf = []; logT0 = Date.now(); maxPitch = 0; logUI(); }
  function stopRec() { recording = false; logUI(); }
  window.toggleRec = function () { if (recording) stopRec(); else startRec(); };
  window.clearLog = function () { recording = false; logBuf = []; maxPitch = 0; logUI(); };
  $("auto-rec").addEventListener("change", function (e) { autoRec = e.target.checked; });
  function logUI() {
    $("log-state").textContent = recording ? "DANG GHI" : (logBuf.length ? "DA LUU TAM" : "CHO GHI");
    $("log-state").style.color = recording ? "#ff3860" : (logBuf.length ? "#22e58a" : "#6f82a3");
    $("log-count").textContent = logBuf.length;
    $("log-max").innerHTML = maxPitch.toFixed(1) + "&deg;";
    $("log-dur").textContent = (logBuf.length * 0.05).toFixed(1) + " s";
    $("btn-rec").textContent = recording ? "DUNG GHI" : "BAT DAU GHI";
    $("btn-txt").disabled = $("btn-csv").disabled = (logBuf.length === 0);
  }
  window.exportLog = function (fmt) {
    if (!logBuf.length) return;
    var sep = (fmt === "csv") ? "," : "\t", out = "";
    if (fmt === "txt") {
      out += "# TWIP RACER - telemetry log (20 Hz) - " + new Date().toISOString() + "\n";
      out += "# Kp1=" + $("kp1").value + " Kd1=" + $("kd1").value + " Kp2=" + $("kp2").value + " Ki2=" + $("ki2").value +
             " Trim=" + $("trim").value + " Kyaw=" + $("kyaw").value + "\n";
      out += "# Mau=" + logBuf.length + " |Pitch|max=" + maxPitch.toFixed(2) + " deg\n";
    }
    out += ["Time_ms","Pitch_deg","Gyro_dps","V_Act_mps","V_Left_mps","V_Right_mps","V_Tgt_mps","PWM_Left","PWM_Right","State","Batt_Volt","Theta_Tgt_deg"].join(sep) + "\n";
    for (var i = 0; i < logBuf.length; i++) {
      var r = logBuf[i];
      out += [r.t, r.pitch.toFixed(2), r.gyro.toFixed(2), r.vAct.toFixed(3), r.vL.toFixed(3), r.vR.toFixed(3),
              r.vTgt.toFixed(3), r.pwmL, r.pwmR, r.state, r.batt.toFixed(2), r.thT.toFixed(2)].join(sep) + "\n";
    }
    var d = new Date(), pad = function (n) { return String(n).padStart(2, "0"); };
    var tag = d.getFullYear() + pad(d.getMonth() + 1) + pad(d.getDate()) + "_" + pad(d.getHours()) + pad(d.getMinutes()) + pad(d.getSeconds());
    var a = document.createElement("a");
    a.href = URL.createObjectURL(new Blob([out], { type: fmt === "csv" ? "text/csv;charset=utf-8" : "text/plain;charset=utf-8" }));
    a.download = "twip_log_" + tag + "." + fmt;
    document.body.appendChild(a); a.click(); document.body.removeChild(a);
    setTimeout(function () { URL.revokeObjectURL(a.href); }, 1000);
  };

  /* ======================= Khởi động ======================= */
  window.addEventListener("load", function () { resize(); draw(); logUI(); syncModes(); diagnose(false); initWS(); });
})();
</script>
</body>
</html>
)rawliteral";

#endif /* __INDEX_HTML_H__ */
