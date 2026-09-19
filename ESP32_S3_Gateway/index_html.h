#ifndef __INDEX_HTML_H__
#define __INDEX_HTML_H__

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>TWIP S3 Racer HUD</title>
  <style>
    :root {
      --bg: #0b0f19;
      --card: #151d2f;
      --card-border: #232f48;
      --cyan: #00f0ff;
      --green: #00ff88;
      --red: #ff3366;
      --orange: #ff9900;
      --text: #f0f4fc;
      --text-dim: #7f91b3;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-user-select: none; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      overflow-x: hidden;
      padding-bottom: 20px;
    }
    header {
      background: rgba(21, 29, 47, 0.85);
      backdrop-filter: blur(10px);
      border-bottom: 1px solid var(--card-border);
      padding: 12px 16px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .brand { font-size: 16px; font-weight: 800; letter-spacing: 1px; color: var(--cyan); }
    .status-badge {
      font-size: 12px;
      padding: 4px 10px;
      border-radius: 20px;
      font-weight: 700;
      background: #333;
      text-transform: uppercase;
    }
    .state-standby { background: #3a3f58; color: #b0b8d8; }
    .state-balancing { background: #004d40; color: var(--green); }
    .state-racing { background: #660022; color: var(--red); }
    .state-fallen { background: #553300; color: var(--orange); }
    .state-emergency { background: var(--red); color: #fff; }

    .container { padding: 12px; max-width: 600px; margin: 0 auto; display: flex; flex-direction: column; gap: 12px; }

    .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }

    .card {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: 12px;
      padding: 14px;
      display: flex;
      flex-direction: column;
      gap: 6px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.3);
    }
    .card-title { font-size: 11px; text-transform: uppercase; letter-spacing: 1px; color: var(--text-dim); }
    .card-val { font-size: 24px; font-weight: 800; font-family: monospace; color: #fff; }
    .card-sub { font-size: 11px; color: var(--text-dim); }

    /* Horizon Bar */
    .pitch-bar-wrap {
      width: 100%;
      height: 8px;
      background: #0a0d14;
      border-radius: 4px;
      overflow: hidden;
      position: relative;
      margin-top: 4px;
    }
    .pitch-indicator {
      position: absolute;
      top: 0;
      height: 100%;
      width: 6px;
      background: var(--cyan);
      transform: translateX(-50%);
      left: 50%;
      transition: left 0.05s ease-out;
    }

    /* Joystick Area */
    .control-card {
      align-items: center;
      padding: 20px 10px;
    }
    #joystick-zone {
      width: 220px;
      height: 220px;
      background: radial-gradient(circle, #1a253d 0%, #0d1424 100%);
      border: 2px solid var(--card-border);
      border-radius: 50%;
      position: relative;
      touch-action: none;
      box-shadow: inset 0 0 20px rgba(0,0,0,0.5);
    }
    #joystick-thumb {
      width: 70px;
      height: 70px;
      background: radial-gradient(circle, var(--cyan) 0%, #0088cc 100%);
      border-radius: 50%;
      position: absolute;
      top: 75px;
      left: 75px;
      box-shadow: 0 0 15px rgba(0,240,255,0.6);
      transform: translate3d(0,0,0);
    }

    /* Switches and buttons */
    .btn-row { display: flex; gap: 8px; width: 100%; }
    button {
      flex: 1;
      padding: 12px;
      border: none;
      border-radius: 8px;
      font-weight: 700;
      cursor: pointer;
      font-size: 13px;
      transition: opacity 0.2s;
    }
    button:active { opacity: 0.7; }
    .btn-race { background: #ff2255; color: #fff; }
    .btn-calib { background: #2b3954; color: var(--cyan); }
    .btn-stop { background: #cc0000; color: #fff; }

    /* Tune Accordion */
    details {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: 12px;
      padding: 12px;
    }
    summary { font-weight: 700; font-size: 13px; cursor: pointer; color: var(--cyan); }
    .tune-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      margin-top: 10px;
    }
    .tune-field label { font-size: 11px; color: var(--text-dim); display: block; }
    .tune-field input {
      width: 100%;
      padding: 8px;
      background: #0d1424;
      border: 1px solid var(--card-border);
      border-radius: 6px;
      color: #fff;
      font-weight: 700;
      font-family: monospace;
    }
    .btn-send-pid {
      margin-top: 10px;
      width: 100%;
      background: #00d2ff;
      color: #000;
    }
  </style>
</head>
<body>
  <header>
    <div class="brand">⚡ TWIP RACER S3</div>
    <div id="state-badge" class="status-badge state-standby">STANDBY</div>
  </header>

  <div class="container">
    <!-- Telemetry Row 1 -->
    <div class="grid-2">
      <div class="card">
        <span class="card-title">Góc Nghiêng Pitch</span>
        <span class="card-val" id="val-pitch">0.0°</span>
        <div class="pitch-bar-wrap">
          <div id="pitch-marker" class="pitch-indicator"></div>
        </div>
        <span class="card-sub" id="val-gyro">Gyro: 0.0 dps</span>
      </div>

      <div class="card">
        <span class="card-title">Vận Tốc Tịnh Tiến</span>
        <span class="card-val" id="val-vel">0.00 <small style="font-size:14px">m/s</small></span>
        <span class="card-sub" id="val-tgt-vel">Đặt: 0.00 m/s</span>
      </div>
    </div>

    <!-- Telemetry Row 2 -->
    <div class="grid-2">
      <div class="card">
        <span class="card-title">Công Suất PWM L/R</span>
        <span class="card-val" id="val-pwm" style="font-size:18px">0 | 0</span>
        <span class="card-sub">Max: +-2499</span>
      </div>

      <div class="card">
        <span class="card-title">Điện Áp Pin</span>
        <span class="card-val" id="val-batt" style="color:var(--green)">12.0 <small style="font-size:14px">V</small></span>
        <span class="card-sub">3S LiPo 11.1V</span>
      </div>
    </div>

    <!-- Joystick Card -->
    <div class="card control-card">
      <div id="joystick-zone">
        <div id="joystick-thumb"></div>
      </div>
      <div style="margin-top: 10px; font-size: 12px; color: var(--text-dim);">
        Kéo cần để Tiến / Lùi / Bẻ Lái Thích Ứng
      </div>
    </div>

    <!-- Action Buttons -->
    <div class="btn-row">
      <button id="btn-race-toggle" class="btn-race" onclick="toggleRaceMode()">Đua Tốc Độ (Race)</button>
      <button class="btn-calib" onclick="sendCalib()">Lấy Zero IMU</button>
      <button class="btn-stop" onclick="sendStop()">DỪNG GẤP</button>
    </div>

    <!-- Tuning Parameters Panel -->
    <details>
      <summary>⚙️ Tinh Chỉnh Tham Số PID Thời Gian Thực</summary>
      <div class="tune-grid">
        <div class="tune-field">
          <label>Kp Góc (Lò xo)</label>
          <input type="number" id="kp1" value="350.0" step="10">
        </div>
        <div class="tune-field">
          <label>Kd Góc (Giảm chấn)</label>
          <input type="number" id="kd1" value="8.5" step="0.5">
        </div>
        <div class="tune-field">
          <label>Kp Vận Tốc</label>
          <input type="number" id="kp2" value="1.2" step="0.1">
        </div>
        <div class="tune-field">
          <label>Ki Vận Tốc</label>
          <input type="number" id="ki2" value="0.15" step="0.02">
        </div>
      </div>
      <button class="btn-send-pid" onclick="sendPID()">Cập Nhật Tham Số Xuống STM32</button>
    </details>
  </div>

  <script>
    let ws;
    let isRacing = false;
    let joyActive = false;
    let v_target = 0.0;
    let steer_cmd = 0.0;
    let sendInterval = null;

    const stateLabels = ["INIT", "CALIB", "STANDBY", "BALANCING", "RACING", "FALLEN", "EMERGENCY"];
    const stateClasses = ["state-standby", "state-standby", "state-standby", "state-balancing", "state-racing", "state-fallen", "state-emergency"];

    function initWebSocket() {
      const loc = window.location;
      let wsUri = (loc.protocol === "https:" ? "wss:" : "ws:") + "//" + loc.host + ":81/";
      ws = new WebSocket(wsUri);

      ws.onopen = () => console.log("WebSocket connected");
      ws.onclose = () => {
        console.log("WebSocket disconnected, retrying...");
        setTimeout(initWebSocket, 2000);
      };

      ws.onmessage = (evt) => {
        const msg = evt.data.trim();
        if (msg.startsWith("$TEL,")) {
          // Format: $TEL,pitch,gyro,v_act,v_tgt,pwm_l,pwm_r,state,batt
          const p = msg.substring(5).split(",");
          if (p.length >= 8) {
            const pitch = parseFloat(p[0]);
            const gyro = parseFloat(p[1]);
            const v_act = parseFloat(p[2]);
            const v_tgt = parseFloat(p[3]);
            const pwm_l = parseInt(p[4]);
            const pwm_r = parseInt(p[5]);
            const state = parseInt(p[6]);
            const batt = parseFloat(p[7]);

            document.getElementById("val-pitch").innerText = pitch.toFixed(1) + "°";
            document.getElementById("val-gyro").innerText = "Gyro: " + gyro.toFixed(1) + " dps";
            document.getElementById("val-vel").innerHTML = v_act.toFixed(2) + ' <small style="font-size:14px">m/s</small>';
            document.getElementById("val-tgt-vel").innerText = "Đặt: " + v_tgt.toFixed(2) + " m/s";
            document.getElementById("val-pwm").innerText = pwm_l + " | " + pwm_r;
            document.getElementById("val-batt").innerHTML = batt.toFixed(1) + ' <small style="font-size:14px">V</small>';

            // Horizon bar update (-30 to +30 deg mapped to 0% - 100%)
            let pct = 50 + (pitch / 30.0) * 50;
            if (pct < 5) pct = 5; if (pct > 95) pct = 95;
            document.getElementById("pitch-marker").style.left = pct + "%";

            // State Badge
            const badge = document.getElementById("state-badge");
            badge.innerText = stateLabels[state] || "UNKNOWN";
            badge.className = "status-badge " + (stateClasses[state] || "state-standby");
          }
        }
      };
    }

    // Virtual Joystick logic
    const zone = document.getElementById("joystick-zone");
    const thumb = document.getElementById("joystick-thumb");
    const maxR = 60; // Max radius in px

    function handleJoystick(x, y) {
      let dx = x - zoneRect.left - 110;
      let dy = y - zoneRect.top - 110;
      let dist = Math.sqrt(dx * dx + dy * dy);
      if (dist > maxR) {
        dx = (dx / dist) * maxR;
        dy = (dy / dist) * maxR;
      }
      thumb.style.transform = `translate3d(${dx}px, ${dy}px, 0)`;

      // Up is forward (v_target > 0)
      const maxSpeed = isRacing ? 2.5 : 1.2; // m/s
      v_target = (-dy / maxR) * maxSpeed;

      // Right is steer positive
      const maxSteer = 600.0;
      steer_cmd = (dx / maxR) * maxSteer;
    }

    let zoneRect;
    function updateRect() { zoneRect = zone.getBoundingClientRect(); }
    window.addEventListener("resize", updateRect);
    window.addEventListener("scroll", updateRect);

    zone.addEventListener("pointerdown", (e) => {
      joyActive = true;
      zone.setPointerCapture(e.pointerId);
      updateRect();
      handleJoystick(e.clientX, e.clientY);
    });

    zone.addEventListener("pointermove", (e) => {
      if (joyActive) handleJoystick(e.clientX, e.clientY);
    });

    function resetJoystick() {
      joyActive = false;
      thumb.style.transform = `translate3d(0, 0, 0)`;
      v_target = 0.0;
      steer_cmd = 0.0;
    }

    zone.addEventListener("pointerup", resetJoystick);
    zone.addEventListener("pointercancel", resetJoystick);

    // Periodic send commands to ESP32 -> STM32 (20Hz)
    setInterval(() => {
      if (ws && ws.readyState === WebSocket.OPEN) {
        const cmd = `$CMD,${v_target.toFixed(2)},${steer_cmd.toFixed(1)}*`;
        ws.send(cmd);
      }
    }, 50);

    function toggleRaceMode() {
      isRacing = !isRacing;
      const btn = document.getElementById("btn-race-toggle");
      if (isRacing) {
        btn.innerText = "Chế Độ Thường (Norm)";
        btn.style.background = "#00ff88";
        btn.style.color = "#000";
        if (ws && ws.readyState === WebSocket.OPEN) ws.send("$RACE,1*");
      } else {
        btn.innerText = "Đua Tốc Độ (Race)";
        btn.style.background = "#ff2255";
        btn.style.color = "#fff";
        if (ws && ws.readyState === WebSocket.OPEN) ws.send("$RACE,0*");
      }
    }

    function sendCalib() {
      if (confirm("Đặt xe đứng thẳng yên tĩnh rồi bấm OK để hiệu chuẩn!")) {
        if (ws && ws.readyState === WebSocket.OPEN) ws.send("$CALIB*");
      }
    }

    function sendStop() {
      if (ws && ws.readyState === WebSocket.OPEN) ws.send("$STOP*");
      resetJoystick();
    }

    function sendPID() {
      const kp1 = parseFloat(document.getElementById("kp1").value);
      const kd1 = parseFloat(document.getElementById("kd1").value);
      const kp2 = parseFloat(document.getElementById("kp2").value);
      const ki2 = parseFloat(document.getElementById("ki2").value);
      const pidPacket = `$PID,${kp1},${kd1},${kp2},${ki2}*`;
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(pidPacket);
        alert("Đã gửi thông số PID xuống xe!");
      }
    }

    window.onload = () => {
      updateRect();
      initWebSocket();
    };
  </script>
</body>
</html>
)rawliteral";

#endif /* __INDEX_HTML_H__ */
