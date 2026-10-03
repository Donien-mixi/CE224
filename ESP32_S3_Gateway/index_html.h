#ifndef __INDEX_HTML_H__
#define __INDEX_HTML_H__

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>TWIP RACER - DIEU KHIEN XE</title>
  <style>
    :root {
      --bg: #0b0f19;
      --card: #151d2f;
      --border: #232f48;
      --cyan: #00f0ff;
      --green: #00ff88;
      --red: #ff3366;
      --orange: #ff9900;
      --text: #f0f4fc;
      --dim: #7f91b3;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-user-select: none; user-select: none; -webkit-tap-highlight-color: transparent; }
    body { background: var(--bg); color: var(--text); font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; padding-bottom: 24px; overflow-x: hidden; }

    header {
      position: sticky; top: 0; z-index: 100;
      display: flex; justify-content: space-between; align-items: center; gap: 8px;
      padding: 12px 14px;
      background: rgba(21,29,47,.92); backdrop-filter: blur(10px);
      border-bottom: 1px solid var(--border);
    }
    .brand { font-size: 15px; font-weight: 800; letter-spacing: .5px; color: var(--cyan); }
    .badges { display: flex; gap: 6px; }
    .badge { font-size: 11px; font-weight: 700; padding: 4px 10px; border-radius: 20px; background: #333; text-transform: uppercase; white-space: nowrap; }
    .state-init { background: #3a3f58; color: #b0b8d8; }
    .state-standby { background: #3a3f58; color: #b0b8d8; }
    .state-balancing { background: #004d40; color: var(--green); }
    .state-racing { background: #660022; color: var(--red); }
    .state-fallen { background: #553300; color: var(--orange); }
    .state-emergency { background: var(--red); color: #fff; }
    .state-bench { background: #d97706; color: #fff; font-weight: 800; }
    .link-ok { background: #004d40; color: var(--green); }
    .link-bad { background: #553300; color: var(--orange); }

    .container { max-width: 560px; margin: 0 auto; padding: 12px; display: flex; flex-direction: column; gap: 12px; }

    .card { background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 14px; box-shadow: 0 4px 15px rgba(0,0,0,.3); }
    .card-title { font-size: 11px; letter-spacing: 1px; color: var(--dim); text-transform: uppercase; }

    .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
    .cell { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 12px; display: flex; flex-direction: column; gap: 4px; }
    .cell .label { font-size: 10px; letter-spacing: .5px; color: var(--dim); text-transform: uppercase; }
    .cell .value { font-size: 20px; font-weight: 800; font-family: ui-monospace, Consolas, monospace; color: #fff; }
    .cell .value small { font-size: 12px; font-weight: 600; color: var(--dim); }

    /* Bench test button */
    .btn-bench {
      width: 100%; border: 1px solid #d97706; border-radius: 12px;
      background: linear-gradient(180deg, #2b1d07, #1c1305);
      color: #fbbf24; font-weight: 800; font-size: 13px; padding: 13px;
      cursor: pointer; transition: all .15s; display: flex; align-items: center; justify-content: center; gap: 8px;
    }
    .btn-bench.active {
      background: linear-gradient(180deg, #f59e0b, #d97706);
      color: #000; box-shadow: 0 0 20px rgba(245,158,11,.6);
    }
    .mode-btn {
      padding: 5px 9px; border-radius: 6px; border: 1px solid var(--border);
      background: #101625; color: var(--dim); font-size: 11px; font-weight: 700; cursor: pointer;
    }
    .mode-btn.active {
      background: #1b2844; color: var(--cyan); border-color: var(--cyan);
    }

    /* Speed control */
    .speed-head { display: flex; align-items: baseline; justify-content: space-between; }
    .speed-big { font-size: 40px; font-weight: 800; font-family: ui-monospace, Consolas, monospace; color: var(--cyan); line-height: 1; }
    .speed-big small { font-size: 15px; color: var(--dim); font-weight: 600; }
    input[type=range] {
      -webkit-appearance: none; appearance: none; width: 100%; height: 10px; border-radius: 6px;
      margin: 16px 0 6px; background: linear-gradient(90deg, var(--cyan), #0088cc); outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none; appearance: none; width: 28px; height: 28px; border-radius: 50%;
      background: #fff; border: 3px solid var(--cyan); box-shadow: 0 0 12px rgba(0,240,255,.7); cursor: pointer;
    }
    input[type=range]::-moz-range-thumb { width: 24px; height: 24px; border-radius: 50%; background: #fff; border: 3px solid var(--cyan); cursor: pointer; }
    .range-labels { display: flex; justify-content: space-between; font-size: 11px; color: var(--dim); }

    /* Direction pad */
    .pad { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 4px; }
    .pad-btn {
      touch-action: none; border: 1px solid var(--border); border-radius: 14px;
      background: linear-gradient(180deg, #1b2740, #121a2c);
      color: var(--text); font-weight: 800; font-size: 16px; padding: 22px 8px;
      display: flex; flex-direction: column; align-items: center; gap: 6px; cursor: pointer;
      transition: transform .05s, box-shadow .1s, background .1s;
    }
    .pad-btn .arrow { font-size: 30px; line-height: 1; }
    .pad-btn.active { background: linear-gradient(180deg, #00f0ff, #0088cc); color: #001018; box-shadow: 0 0 18px rgba(0,240,255,.6); transform: scale(.97); }
    .pad-btn.active .arrow { color: #001018; }

    .btn-row { display: flex; gap: 10px; }
    button.action {
      flex: 1; border: none; border-radius: 10px; padding: 14px; font-weight: 800; font-size: 14px; cursor: pointer;
    }
    .btn-stop { background: #26314a; color: #fff; }
    .btn-estop { background: var(--red); color: #fff; }

    /* Pitch bar */
    .bar-wrap { height: 8px; background: #0a0d14; border-radius: 4px; overflow: hidden; position: relative; margin-top: 6px; }
    .bar-mid { position: absolute; left: 50%; top: 0; width: 1px; height: 100%; background: #33405e; transform: translateX(-50%); }
    .bar-ind { position: absolute; top: 0; height: 100%; width: 6px; border-radius: 3px; background: var(--cyan); transform: translateX(-50%); left: 50%; transition: left .06s linear, background .2s; }

    /* Diagnosis line */
    .diag { display: flex; align-items: center; gap: 10px; border-radius: 12px; padding: 12px 14px; font-size: 13px; font-weight: 600; border: 1px solid var(--border); background: var(--card); }
    .dot { width: 10px; height: 10px; border-radius: 50%; flex: 0 0 auto; background: #444; box-shadow: 0 0 8px currentColor; }
    .diag.ok { color: var(--green); border-color: rgba(0,255,136,.35); }
    .diag.ok .dot { background: var(--green); }
    .diag.warn { color: var(--orange); border-color: rgba(255,153,0,.35); }
    .diag.warn .dot { background: var(--orange); }
    .diag.err { color: var(--red); border-color: rgba(255,51,102,.4); }
    .diag.err .dot { background: var(--red); }
    .diag.info { color: #b0b8d8; }
    .diag.info .dot { background: #7f91b3; }

    /* Tuning Accordion */
    details { background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 12px 14px; }
    summary { font-size: 13px; font-weight: 700; color: var(--cyan); cursor: pointer; outline: none; }
    .tune-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 12px; }
    .tune-field { display: flex; flex-direction: column; gap: 4px; }
    .tune-field label { font-size: 11px; color: var(--dim); font-weight: 600; }
    .tune-field input { background: #0d1424; border: 1px solid var(--border); border-radius: 8px; padding: 8px; color: #fff; font-family: ui-monospace, Consolas, monospace; font-size: 14px; font-weight: 700; outline: none; }
    .trim-bar { display: flex; gap: 6px; align-items: center; margin-top: 4px; }
    .trim-btn { flex: 1; padding: 8px 4px; background: #1b2740; border: 1px solid var(--border); border-radius: 8px; color: var(--cyan); font-weight: 800; font-size: 12px; cursor: pointer; }
    .trim-input { width: 75px; text-align: center; color: var(--orange) !important; }
    .btn-apply-pid { margin-top: 12px; width: 100%; padding: 12px; background: var(--cyan); color: #001018; border: none; border-radius: 8px; font-weight: 800; font-size: 13px; cursor: pointer; }

    /* Data Logger styles */
    .dot-record { width: 9px; height: 9px; border-radius: 50%; background: #555; display: inline-block; transition: all .2s; }
    .dot-record.recording { background: var(--red); box-shadow: 0 0 10px var(--red); animation: pulse-red 1s infinite alternate; }
    @keyframes pulse-red { from { transform: scale(0.85); opacity: 0.7; } to { transform: scale(1.25); opacity: 1; } }
    .btn-record { background: linear-gradient(180deg, #1b2740, #131c2d); color: var(--cyan); border: 1px solid var(--border); transition: all .2s; }
    .btn-record.recording { background: linear-gradient(180deg, #7a1228, #4d0b19); border-color: var(--red); color: #fff; box-shadow: 0 0 15px rgba(255, 51, 102, 0.4); }
    .btn-export { background: linear-gradient(180deg, #00874e, #005a34); color: #fff; border: 1px solid #00aa60; transition: all .2s; }
    .btn-export:disabled { background: #1b2333; color: var(--dim); border-color: var(--border); opacity: 0.5; cursor: not-allowed; }
    .btn-export:not(:disabled):hover { background: linear-gradient(180deg, #00b066, #007a46); box-shadow: 0 0 16px rgba(0, 255, 136, 0.4); }
    .btn-clear { max-width: 48px; background: #1b2333; color: var(--dim); border: 1px solid var(--border); }
    .btn-clear:hover { color: var(--red); border-color: var(--red); }

    .foot { text-align: center; font-size: 11px; color: var(--dim); padding: 4px 0 0; }
  </style>
</head>
<body>
  <header>
    <div class="brand">TWIP RACER</div>
    <div class="badges">
      <div id="state-badge" class="badge state-standby">STANDBY</div>
      <div id="link-badge" class="badge link-bad">MAT TIN HIEU</div>
    </div>
  </header>

  <div class="container">

    <!-- Diagnosis: biet xe co hoat dong dung khong -->
    <div id="diag" class="diag info"><span class="dot"></span><span id="diag-text">Dang ket noi toi STM32...</span></div>

    <!-- Bench Test toggle card -->
    <div class="card" style="border-color: rgba(217, 119, 6, 0.45); background: linear-gradient(180deg, #181928, #131724);">
      <button id="btn-bench" class="btn-bench" onclick="toggleBenchTest()">
        <span id="bench-icon">🧪</span>
        <span id="bench-txt">BẬT CHẾ ĐỘ TEST BÀN (QUAY BÁNH TRỰC TIẾP)</span>
      </button>
      <div style="font-size:11px; color:var(--dim); margin-top:8px; line-height:1.4;">
        💡 <b>Chế độ Test Bàn</b>: Bỏ qua cảm biến cân bằng, cho phép bấm TIẾN/LÙI để quay bánh trực tiếp ngay khi xe đang đặt nằm trên bàn. Rất an toàn để kiểm tra chiều quay động cơ và kết nối web!
      </div>
    </div>

    <!-- Speed control -->
    <div class="card">
      <div class="speed-head">
        <span class="card-title">Toc do dat</span>
        <span class="speed-big"><span id="speed-val">0.60</span><small> m/s</small></span>
      </div>
      <input id="speed-slider" type="range" min="0.10" max="1.50" step="0.05" value="0.60">
      <div class="range-labels"><span>0.10 m/s</span><span>1.50 m/s</span></div>
    </div>

    <!-- Direction controls -->
    <div class="card">
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
        <span class="card-title" style="margin:0;">Dieu khien huong</span>
        <div style="display:flex; gap:6px;">
          <button id="mode-hold" class="mode-btn active" onclick="setPressMode('hold')">Giữ nút</button>
          <button id="mode-toggle" class="mode-btn" onclick="setPressMode('toggle')">Bấm 1 lần</button>
        </div>
      </div>
      <div style="margin-bottom:8px;">
        <span id="cmd-status" style="font-size:12px; font-weight:800; color:var(--cyan);">LENH: DUNG (0.00 m/s)</span>
      </div>
      <div class="pad">
        <div id="btn-fwd" class="pad-btn"><span class="arrow">&#9650;</span>TIEN (W / &uarr;)</div>
        <div id="btn-back" class="pad-btn"><span class="arrow">&#9660;</span>LUI (S / &darr;)</div>
      </div>
      <div class="btn-row" style="margin-top:10px;">
        <button class="action btn-stop" onclick="sendStop()">DUNG (Space)</button>
        <button class="action btn-estop" onclick="sendEmergency()">DUNG KHAN CAP</button>
      </div>
    </div>
    <!-- Telemetry -->
    <div class="grid-2">
      <div class="cell">
        <span class="label">Goc nghieng Pitch</span>
        <span class="value" id="tel-pitch">--.-<small>&deg;</small></span>
        <div class="bar-wrap"><div class="bar-mid"></div><div id="pitch-ind" class="bar-ind"></div></div>
      </div>
      <div class="cell">
        <span class="label">Van toc thuc / dat</span>
        <span class="value"><span id="tel-vact">--.--</span><small> m/s</small></span>
        <span class="label" id="tel-vtgt" style="font-size:9.5px;">đặt: --.-- | L: <span id="tel-vl">--</span> | R: <span id="tel-vr">--</span></span>
      </div>
    </div>

    <div class="grid-3">
      <div class="cell"><span class="label">PWM L</span><span class="value" id="tel-pwml">--</span></div>
      <div class="cell"><span class="label">PWM R</span><span class="value" id="tel-pwmr">--</span></div>
      <div class="cell"><span class="label">Gyro</span><span class="value" id="tel-gyro">--.-<small> dps</small></span></div>
    </div>

    <div class="grid-3">
      <div class="cell"><span class="label">Pin</span><span class="value" id="tel-batt">--.-<small> V</small></span></div>
      <div class="cell"><span class="label">Goi tin</span><span class="value" id="tel-rate">--<small> /s</small></span></div>
      <div class="cell"><span class="label">Chan doan</span><span class="value" id="tel-state" style="font-size:14px;">--</span></div>
    </div>

    <!-- Data Logger Card -->
    <div class="card" style="border-color: rgba(0, 240, 255, 0.35); background: linear-gradient(180deg, #12192c, #0d1322);">
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:10px;">
        <span class="card-title" style="color:var(--cyan); font-weight:800; display:flex; align-items:center; gap:6px;">
          <span id="log-status-dot" class="dot-record"></span>
          BỘ GHI DỮ LIỆU ĐO ĐẠC (TUNING LOG)
        </span>
        <label style="display:flex; align-items:center; gap:6px; font-size:11px; color:var(--dim); cursor:pointer;">
          <input type="checkbox" id="auto-record-chk" checked style="accent-color:var(--cyan); cursor:pointer;">
          Tự động ghi khi xe chạy
        </label>
      </div>

      <div class="grid-3" style="margin-bottom:10px;">
        <div class="cell" style="padding:8px 10px; background:rgba(11,15,25,0.7);">
          <span class="label">Trạng thái</span>
          <span id="log-status-text" style="font-size:13px; font-weight:800; color:var(--dim);">CHỜ GHI</span>
        </div>
        <div class="cell" style="padding:8px 10px; background:rgba(11,15,25,0.7);">
          <span class="label">Số mẫu / Thời gian</span>
          <span id="log-count-text" style="font-size:13px; font-weight:800; color:#fff;">0 mẫu <small>(0.0s)</small></span>
        </div>
        <div class="cell" style="padding:8px 10px; background:rgba(11,15,25,0.7);">
          <span class="label">Nghiêng lớn nhất</span>
          <span id="log-max-pitch" style="font-size:13px; font-weight:800; color:var(--orange);">0.0&deg;</span>
        </div>
      </div>

      <div class="btn-row">
        <button id="btn-record-toggle" class="action btn-record" onclick="toggleRecordManual()">
          <span id="record-icon">⏺️</span> <span id="record-text">BẮT ĐẦU GHI</span>
        </button>
        <button id="btn-export-txt" class="action btn-export" onclick="exportDataLog()" disabled>
          📥 XUẤT FILE TXT
        </button>
        <button class="action btn-clear" onclick="clearDataLog()" title="Xoá bộ đệm">
          🗑️
        </button>
      </div>
      <div style="font-size:11px; color:var(--dim); margin-top:8px; line-height:1.4;">
        💾 <b>Ghi &amp; Xuất dữ liệu</b>: Tự động gom thông số BMI160, 2 động cơ PWM, 2 encoder ở tần số 20Hz. Sau khi test xe xong, bấm <b>XUẤT FILE TXT</b> để tải file log chứa đầy đủ thông số kèm cấu hình PID để phân tích đồ thị hoặc cân chỉnh.
      </div>
    </div>

    <!-- Tuning PID & Pitch Trim Accordion -->
    <details>
      <summary>⚙️ Tinh Chinh PID &amp; Bu Trong Tam Pitch Trim</summary>
      <div class="tune-grid">
        <div class="tune-field">
          <label>Kp Goc (Lo xo)</label>
          <input type="number" id="kp1" value="98.2" step="10">
        </div>
        <div class="tune-field">
          <label>Kd Goc (Giam chan)</label>
          <input type="number" id="kd1" value="6.82" step="0.1">
        </div>
        <div class="tune-field">
          <label>Kp Van Toc</label>
          <input type="number" id="kp2" value="4.0" step="0.1">
        </div>
        <div class="tune-field">
          <label>Ki Van Toc</label>
          <input type="number" id="ki2" value="0.80" step="0.02">
        </div>
        <div class="tune-field" style="grid-column: span 2;">
          <label>Bu Trong Tam Co Khi Pitch Trim (&deg;)</label>
          <div class="trim-bar">
            <button type="button" class="trim-btn" onclick="adjustTrim(-0.1)">-0.1&deg;</button>
            <button type="button" class="trim-btn" onclick="adjustTrim(-0.02)">-0.02&deg;</button>
            <input type="number" id="pitch-trim" class="trim-input" value="0.00" step="0.02">
            <button type="button" class="trim-btn" onclick="adjustTrim(0.02)">+0.02&deg;</button>
            <button type="button" class="trim-btn" onclick="adjustTrim(0.1)">+0.1&deg;</button>
          </div>
        </div>
      </div>
      <button type="button" class="btn-apply-pid" onclick="sendPID()">Cap Nhat Tham So Xuong STM32</button>
      <button type="button" class="btn-apply-pid" style="background:#ff9900; color:#001018; margin-top:8px;" onclick="sendCalib()">HIEU CHUAN LAI IMU ($CALIB)</button>
    </details>

    <div class="foot">ESP32-S3 Gateway &middot; UART 115200 (GPIO18 RX / GPIO17 TX)</div>
  </div>

  <script>
    var ws = null;
    var speed = 0.60;          // m/s, lay tu thanh truot
    var desiredV = 0.0;        // van toc gui xuong STM32
    var connected = false;
    var lastTel = 0;           // thoi diem goi telemetry cuoi (ms)
    var telCount = 0;          // dem goi trong 1 giay
    var lastState = -1;
    var lastPitch = 0.0;
    var lastVTgtAck = 0.0;
    var benchMode = false;
    var toggleMode = false;

    var stateLabels = ["INIT", "CALIB", "STANDBY", "BALANCING", "RACING", "FALLEN", "EMERGENCY", "TEST BÀN"];
    var stateClasses = ["state-init", "state-standby", "state-standby", "state-balancing", "state-racing", "state-fallen", "state-emergency", "state-bench"];

    /* ---------------- WebSocket ---------------- */
    function initWS() {
      var proto = (location.protocol === "https:") ? "wss:" : "ws:";
      ws = new WebSocket(proto + "//" + location.host + ":81/");

      ws.onopen = function () { connected = true; };

      ws.onclose = function () {
        connected = false;
        desiredV = 0.0;
        setTimeout(initWS, 1500);
      };

      ws.onerror = function () {};
      ws.onmessage = function (evt) {
        var msg = String(evt.data).trim();
        if (msg.indexOf("$TEL,") === 0) applyTelemetry(msg);
      };
    }

    function sendRaw(txt) {
      if (ws && ws.readyState === WebSocket.OPEN) ws.send(txt);
    }

    /* ---------------- Gui lenh 20Hz ---------------- */
    setInterval(function () {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send("$CMD," + desiredV.toFixed(2) + ",0.0*");
      }
    }, 50);

    /* ---------------- Che do Test Ban (Bench Test) ---------------- */
    function toggleBenchTest() {
      benchMode = !benchMode;
      sendRaw("$BENCH," + (benchMode ? "1" : "0") + "*");
      updateBenchUI();
    }

    function updateBenchUI() {
      var b = document.getElementById("btn-bench");
      var txt = document.getElementById("bench-txt");
      var ico = document.getElementById("bench-icon");
      if (!b) return;
      if (benchMode) {
        b.classList.add("active");
        if (ico) ico.textContent = "⚖️";
        if (txt) txt.textContent = "ĐANG BẬT TEST BÀN - BẤM ĐỂ VỀ CÂN BẰNG";
      } else {
        b.classList.remove("active");
        if (ico) ico.textContent = "🧪";
        if (txt) txt.textContent = "BẬT CHẾ ĐỘ TEST BÀN (QUAY BÁNH TRỰC TIẾP)";
      }
    }

    /* ---------------- Che do Nhan Nut (Hold vs Toggle) ---------------- */
    function setPressMode(m) {
      toggleMode = (m === "toggle");
      document.getElementById("mode-hold").className = "mode-btn " + (toggleMode ? "" : "active");
      document.getElementById("mode-toggle").className = "mode-btn " + (toggleMode ? "active" : "");
      desiredV = 0.0;
      document.getElementById("btn-fwd").classList.remove("active");
      document.getElementById("btn-back").classList.remove("active");
      updateCommandBadge();
    }

    /* ---------------- Thanh truot toc do ---------------- */
    var slider = document.getElementById("speed-slider");
    slider.addEventListener("input", function () {
      speed = parseFloat(slider.value);
      document.getElementById("speed-val").textContent = speed.toFixed(2);
      if (desiredV > 0) desiredV = speed;
      if (desiredV < 0) desiredV = -speed;
      updateCommandBadge();
    });

    function updateCommandBadge() {
      var el = document.getElementById("cmd-status");
      if (!el) return;
      var ackOk = (Math.abs(desiredV - lastVTgtAck) < 0.05);

      if (desiredV > 0.001) {
        el.innerHTML = '<span style="color:#00ff88;">DANG PHAT: TIEN (+' + desiredV.toFixed(2) + ' m/s)</span>' +
                       (ackOk ? ' <span style="color:#00ff88; font-size:11px;">[STM32 ✓]</span>' : ' <span style="color:#ff9900; font-size:11px;">[Dang gui...]</span>');
      } else if (desiredV < -0.001) {
        el.innerHTML = '<span style="color:#ff9900;">DANG PHAT: LUI (' + desiredV.toFixed(2) + ' m/s)</span>' +
                       (ackOk ? ' <span style="color:#00ff88; font-size:11px;">[STM32 ✓]</span>' : ' <span style="color:#ff9900; font-size:11px;">[Dang gui...]</span>');
      } else {
        el.textContent = "LENH: DUNG (0.00 m/s)";
        el.style.color = "var(--cyan)";
      }
    }

    /* ---------------- Nut TIEN / LUI (ho tro pointerCapture & Toggle) ---------------- */
    function bindHold(el, dir) {
      function down(e) {
        e.preventDefault();
        if (toggleMode) {
          if (Math.abs(desiredV - dir * speed) < 0.01) {
            desiredV = 0.0;
            el.classList.remove("active");
          } else {
            desiredV = dir * speed;
            document.getElementById("btn-fwd").classList.remove("active");
            document.getElementById("btn-back").classList.remove("active");
            el.classList.add("active");
          }
        } else {
          try { e.target.setPointerCapture(e.pointerId); } catch(err) {}
          desiredV = dir * speed;
          el.classList.add("active");
        }
        updateCommandBadge();
      }

      function up(e) {
        if (!toggleMode) {
          desiredV = 0.0;
          el.classList.remove("active");
          try { e.target.releasePointerCapture(e.pointerId); } catch(err) {}
          updateCommandBadge();
        }
      }

      el.addEventListener("pointerdown", down);
      el.addEventListener("pointerup", up);
      el.addEventListener("pointercancel", up);
      el.addEventListener("contextmenu", function (e) { e.preventDefault(); });
    }
    bindHold(document.getElementById("btn-fwd"), +1);
    bindHold(document.getElementById("btn-back"), -1);

    window.addEventListener("blur", function () {
      desiredV = 0.0;
      document.getElementById("btn-fwd").classList.remove("active");
      document.getElementById("btn-back").classList.remove("active");
      updateCommandBadge();
    });

    /* ---------------- Phim bam ban phim (W/S hoac Mui Ten) ---------------- */
    window.addEventListener("keydown", function(e) {
      if (e.target && (e.target.tagName === "INPUT" || e.target.tagName === "TEXTAREA")) return;
      if (e.key === "ArrowUp" || e.key === "w" || e.key === "W") {
        e.preventDefault();
        desiredV = speed;
        document.getElementById("btn-fwd").classList.add("active");
        updateCommandBadge();
      } else if (e.key === "ArrowDown" || e.key === "s" || e.key === "S") {
        e.preventDefault();
        desiredV = -speed;
        document.getElementById("btn-back").classList.add("active");
        updateCommandBadge();
      } else if (e.key === " " || e.key === "Escape") {
        e.preventDefault();
        sendStop();
      }
    });

    window.addEventListener("keyup", function(e) {
      if (e.target && (e.target.tagName === "INPUT" || e.target.tagName === "TEXTAREA")) return;
      if (!toggleMode && (e.key === "ArrowUp" || e.key === "w" || e.key === "W" ||
          e.key === "ArrowDown" || e.key === "s" || e.key === "S")) {
        desiredV = 0.0;
        document.getElementById("btn-fwd").classList.remove("active");
        document.getElementById("btn-back").classList.remove("active");
        updateCommandBadge();
      }
    });

    function sendStop() {
      desiredV = 0.0;
      document.getElementById("btn-fwd").classList.remove("active");
      document.getElementById("btn-back").classList.remove("active");
      updateCommandBadge();
      sendRaw("$CMD,0.00,0.0*");
    }

    function sendEmergency() {
      desiredV = 0.0;
      benchMode = false;
      updateBenchUI();
      document.getElementById("btn-fwd").classList.remove("active");
      document.getElementById("btn-back").classList.remove("active");
      updateCommandBadge();
      sendRaw("$STOP*");
    }

    function sendPID() {
      var kp1 = parseFloat(document.getElementById("kp1").value);
      var kd1 = parseFloat(document.getElementById("kd1").value);
      var kp2 = parseFloat(document.getElementById("kp2").value);
      var ki2 = parseFloat(document.getElementById("ki2").value);
      sendRaw("$PID," + kp1 + "," + kd1 + "," + kp2 + "," + ki2 + "*");
    }

    function adjustTrim(delta) {
      var el = document.getElementById("pitch-trim");
      var val = (parseFloat(el.value || "0") + delta).toFixed(2);
      el.value = val;
      sendRaw("$TRIM," + val + "*");
    }

    function sendCalib() {
      if (confirm("Dat xe NAM YEN tren mat phang trong ~2.5 giay de hieu chuan lai Gyro. Tiep tuc?")) {
        sendRaw("$CALIB*");
      }
    }

    /* ---------------- Data Logger & Export TXT ---------------- */
    var isRecording = false;
    var logBuffer = [];
    var logStartTime = 0;
    var maxPitchRecorded = 0.0;
    var autoRecordEnabled = true;

    function toggleRecordManual() {
      if (isRecording) {
        stopRecording();
      } else {
        startRecording();
      }
    }

    function startRecording() {
      isRecording = true;
      logBuffer = []; // Khởi tạo phiên đo mới
      logStartTime = Date.now();
      maxPitchRecorded = 0.0;
      updateLoggerUI();
    }

    function stopRecording() {
      if (!isRecording) return;
      isRecording = false;
      updateLoggerUI();
    }

    function clearDataLog() {
      if (isRecording) stopRecording();
      logBuffer = [];
      maxPitchRecorded = 0.0;
      updateLoggerUI();
    }

    function updateLoggerUI() {
      var dot = document.getElementById("log-status-dot");
      var txt = document.getElementById("log-status-text");
      var cnt = document.getElementById("log-count-text");
      var maxP = document.getElementById("log-max-pitch");
      var recBtn = document.getElementById("btn-record-toggle");
      var recIco = document.getElementById("record-icon");
      var recTxt = document.getElementById("record-text");
      var expBtn = document.getElementById("btn-export-txt");

      if (isRecording) {
        if (dot) dot.className = "dot-record recording";
        if (txt) { txt.textContent = "ĐANG GHI..."; txt.style.color = "var(--red)"; }
        if (recBtn) recBtn.className = "action btn-record recording";
        if (recIco) recIco.textContent = "⏹️";
        if (recTxt) recTxt.textContent = "DỪNG GHI";
      } else {
        if (dot) dot.className = "dot-record";
        if (txt) {
          if (logBuffer.length > 0) {
            txt.textContent = "ĐÃ LƯU TẠM";
            txt.style.color = "var(--green)";
          } else {
            txt.textContent = "CHỜ GHI";
            txt.style.color = "var(--dim)";
          }
        }
        if (recBtn) recBtn.className = "action btn-record";
        if (recIco) recIco.textContent = "⏺️";
        if (recTxt) recTxt.textContent = "BẮT ĐẦU GHI";
      }

      var durSec = (logBuffer.length * 0.05).toFixed(1);
      if (cnt) cnt.innerHTML = logBuffer.length + " mẫu <small>(" + durSec + "s)</small>";
      if (maxP) maxP.innerHTML = maxPitchRecorded.toFixed(1) + "&deg;";
      if (expBtn) expBtn.disabled = (logBuffer.length === 0);
    }

    function exportDataLog() {
      if (logBuffer.length === 0) {
        alert("Chưa có dữ liệu nào được ghi! Hãy cho xe chạy cân bằng trước.");
        return;
      }

      var kp1 = document.getElementById("kp1").value || "98.2";
      var kd1 = document.getElementById("kd1").value || "6.82";
      var kp2 = document.getElementById("kp2").value || "4.0";
      var ki2 = document.getElementById("ki2").value || "0.80";
      var trim = document.getElementById("pitch-trim").value || "0.00";

      var now = new Date();
      var dateStr = now.getFullYear() + "-" +
                    String(now.getMonth()+1).padStart(2,'0') + "-" +
                    String(now.getDate()).padStart(2,'0') + " " +
                    String(now.getHours()).padStart(2,'0') + ":" +
                    String(now.getMinutes()).padStart(2,'0') + ":" +
                    String(now.getSeconds()).padStart(2,'0');

      var fileTimeTag = now.getFullYear() +
                        String(now.getMonth()+1).padStart(2,'0') +
                        String(now.getDate()).padStart(2,'0') + "_" +
                        String(now.getHours()).padStart(2,'0') +
                        String(now.getMinutes()).padStart(2,'0') +
                        String(now.getSeconds()).padStart(2,'0');

      var totalSec = (logBuffer.length * 0.05).toFixed(2);

      // Metadata Header
      var out = "# ==============================================================================\n";
      out += "# NHOM DO AN CE224 - XE HAI BANH TU CAN BANG (TWIP RACER)\n";
      out += "# NHAT KY DO DAC VA TINH CHINH THONG SO (TUNING TELEMETRY LOG)\n";
      out += "# ==============================================================================\n";
      out += "# Thoi gian ghi nhan  : " + dateStr + "\n";
      out += "# Cau hinh PID hien tai:\n";
      out += "#   - Kp1 (Do nhay goc nghieng): " + kp1 + "\n";
      out += "#   - Kd1 (Giam chan Gyro)     : " + kd1 + "\n";
      out += "#   - Kp2 (Van toc xe)         : " + kp2 + "\n";
      out += "#   - Ki2 (Tich phan van toc)  : " + ki2 + "\n";
      out += "#   - Pitch Trim (Bu trong tam): " + trim + " deg\n";
      out += "# Tan so lay mau       : 20 Hz (Chu ky 50 ms)\n";
      out += "# Tong so mau du lieu  : " + logBuffer.length + " mau (" + totalSec + " giay)\n";
      out += "# Do nghieng Max (|P|) : " + maxPitchRecorded.toFixed(2) + " deg\n";
      out += "# ==============================================================================\n";
      out += "# Du lieu dang bang (Cac cot phan tach bang ky tu TAB):\n";
      out += "Time_ms\tPitch_deg\tGyro_dps\tV_Act_mps\tV_Left_mps\tV_Right_mps\tV_Tgt_mps\tPWM_Left\tPWM_Right\tState\tBatt_Volt\n";

      for (var i = 0; i < logBuffer.length; i++) {
        var row = logBuffer[i];
        out += row.t_ms + "\t" +
               row.pitch.toFixed(2) + "\t" +
               row.gyro.toFixed(2) + "\t" +
               row.v_act.toFixed(3) + "\t" +
               row.v_l.toFixed(3) + "\t" +
               row.v_r.toFixed(3) + "\t" +
               row.v_tgt.toFixed(3) + "\t" +
               row.pwm_l + "\t" +
               row.pwm_r + "\t" +
               row.state + "\t" +
               row.batt.toFixed(2) + "\n";
      }

      var blob = new Blob([out], { type: "text/plain;charset=utf-8" });
      var url = URL.createObjectURL(blob);
      var a = document.createElement("a");
      a.href = url;
      a.download = "twip_tuning_log_" + fileTimeTag + ".txt";
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }

    /* ---------------- Cap nhat Telemetry ---------------- */
    function applyTelemetry(msg) {
      if (msg.endsWith("*")) msg = msg.slice(0, -1);
      var p = msg.substring(5).split(",");
      if (p.length < 8) return;

      var pitch = parseFloat(p[0]);
      var gyro  = parseFloat(p[1]);
      var vAct  = parseFloat(p[2]);
      var vTgt  = parseFloat(p[3]);
      var pwmL  = parseInt(p[4], 10);
      var pwmR  = parseInt(p[5], 10);
      var state = parseInt(p[6], 10);
      var batt  = parseFloat(p[7]);
      var vL    = (p.length > 8) ? parseFloat(p[8]) : vAct;
      var vR    = (p.length > 9) ? parseFloat(p[9]) : vAct;

      lastTel = Date.now();
      telCount++;
      lastPitch = pitch;
      lastVTgtAck = vTgt;

      document.getElementById("tel-pitch").innerHTML = pitch.toFixed(1) + '<small>&deg;</small>';
      document.getElementById("tel-gyro").innerHTML  = gyro.toFixed(1) + '<small> dps</small>';
      document.getElementById("tel-vact").textContent = vAct.toFixed(2);
      var tgtEl = document.getElementById("tel-vtgt");
      if (tgtEl) {
        tgtEl.innerHTML = "đặt: " + vTgt.toFixed(2) + " | L: <span id='tel-vl'>" + vL.toFixed(2) + "</span> | R: <span id='tel-vr'>" + vR.toFixed(2) + "</span>";
      }
      document.getElementById("tel-pwml").textContent = pwmL;
      document.getElementById("tel-pwmr").textContent = pwmR;
      document.getElementById("tel-batt").innerHTML = batt.toFixed(1) + '<small> V</small>';
      document.getElementById("tel-state").textContent = (stateLabels[state] || "?");

      // Dong bo nut Bench Test tren web theo FSM thuc te cua STM32
      if (state === 7 && !benchMode) {
        benchMode = true;
        updateBenchUI();
      } else if (state !== 7 && benchMode) {
        benchMode = false;
        updateBenchUI();
      }

      // Thanh goc nghieng (-30 -> +30 do)
      var pct = 50 + (pitch / 30.0) * 50;
      if (pct < 3) pct = 3;
      if (pct > 97) pct = 97;
      var ind = document.getElementById("pitch-ind");
      ind.style.left = pct + "%";
      ind.style.background = (Math.abs(pitch) > 20) ? "#ff3366" : "#00f0ff";

      var badge = document.getElementById("state-badge");
      badge.textContent = stateLabels[state] || "UNKNOWN";
      badge.className = "badge " + (stateClasses[state] || "state-standby");

      // Tu dong bat/tat ghi log theo trang thai xe (Auto-record)
      if (autoRecordEnabled) {
        var isBalancingState = (state === 3 || state === 4 || state === 7);
        if (isBalancingState && !isRecording) {
          startRecording();
        } else if (!isBalancingState && isRecording) {
          stopRecording();
        }
      }

      // Luu mau neu dang trong che do ghi
      if (isRecording) {
        var t_ms = Date.now() - logStartTime;
        var pAbs = Math.abs(pitch);
        if (pAbs > maxPitchRecorded) maxPitchRecorded = pAbs;
        logBuffer.push({
          t_ms: t_ms,
          pitch: pitch,
          gyro: gyro,
          v_act: vAct,
          v_l: vL,
          v_r: vR,
          v_tgt: vTgt,
          pwm_l: pwmL,
          pwm_r: pwmR,
          state: (stateLabels[state] || ("STATE_" + state)),
          batt: batt
        });
        if (logBuffer.length % 5 === 0) {
          updateLoggerUI();
        }
      }

      lastState = state;
      updateCommandBadge();
    }

    /* ---------------- Kiem tra link + toc do goi tin (1s) ---------------- */
    setInterval(function () {
      var age = Date.now() - lastTel;
      var fresh = (age < 800);

      var link = document.getElementById("link-badge");
      if (fresh) { link.textContent = "KET NOI OK"; link.className = "badge link-ok"; }
      else { link.textContent = "MAT TIN HIEU"; link.className = "badge link-bad"; }

      document.getElementById("tel-rate").innerHTML = fresh ? telCount + '<small> /s</small>' : '--<small> /s</small>';
      telCount = 0;

      updateDiagnosis(fresh);
    }, 1000);

    /* ---------------- Chan doan trang thai FSM ---------------- */
    function updateDiagnosis(fresh) {
      var d = document.getElementById("diag");
      var t = document.getElementById("diag-text");
      var cls = "info", txt = "";

      if (!fresh) {
        cls = "err";
        txt = "MẤT TÍN HIỆU TỪ STM32! Kiểm tra dây: PB6(TX)->GPIO18, PB7(RX)->GPIO17 và WAGO 2 GND chung.";
      } else {
        switch (lastState) {
          case 0: cls = "info"; txt = "STM32 đang khởi động..."; break;
          case 1: cls = "warn"; txt = "Đang hiệu chuẩn IMU (500 mẫu). Hãy giữ xe đứng yên..."; break;
          case 2:
            cls = "warn";
            txt = "STANDBY: Xe đang chờ dựng thẳng (|Pitch| < 15°) để tự cân bằng. (Hoặc bấm 'BẬT CHẾ ĐỘ TEST BÀN' để quay thử động cơ ngay trên bàn!)";
            break;
          case 3:
            cls = "ok";
            txt = "BALANCING: Xe đang tự cân bằng tốt! Nhấn giữ hoặc bấm nút TIẾN/LÙI để di chuyển.";
            break;
          case 4:
            cls = "ok";
            txt = "RACING: Chế độ đua tốc độ cao - đang tự cân bằng.";
            break;
          case 5:
            cls = "warn";
            txt = "FALLEN: Xe bị ngã quá 45° -> Động cơ đã tự ngắt an toàn! Hãy dựng thẳng xe (|Pitch| < 15°) để cân bằng, hoặc bấm 'BẬT CHẾ ĐỘ TEST BÀN'.";
            break;
          case 6:
            cls = "err";
            txt = "EMERGENCY: Lỗi khẩn cấp phần cứng hoặc mất kết nối cảm biến BMI160.";
            break;
          case 7:
            cls = "ok";
            txt = "🧪 CHẾ ĐỘ TEST BÀN ĐANG BẬT: Đang điều khiển trực tiếp 2 bánh xe. Bấm TIẾN để quay tới, LÙI để quay lui (bỏ qua cảm biến cân bằng)!";
            break;
          default:
            cls = "info"; txt = "Đang nhận dữ liệu trạng thái..."; break;
        }
      }

      d.className = "diag " + cls;
      t.textContent = txt;
    }

    window.addEventListener("load", function () {
      document.getElementById("speed-val").textContent = speed.toFixed(2);
      updateCommandBadge();
      updateLoggerUI();
      var chk = document.getElementById("auto-record-chk");
      if (chk) {
        chk.addEventListener("change", function() {
          autoRecordEnabled = chk.checked;
        });
      }
      initWS();
    });
  </script>
</body>
</html>
)rawliteral";

#endif /* __INDEX_HTML_H__ */
