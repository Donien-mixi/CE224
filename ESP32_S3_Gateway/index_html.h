#ifndef __INDEX_HTML_H__
#define __INDEX_HTML_H__
#include "firmware_build.h"

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
    #measurement-panel p {margin:10px 0;}
    #measurement-panel label {display:block;margin:10px 0;}
    #measurement-panel input[type=number],#measurement-panel select,#measurement-panel textarea {
      background:#111827;color:#e2e8f0;border:1px solid #64748b;border-radius:6px;padding:10px;font:inherit;
      -webkit-user-select:text;user-select:text;
    }
    #measurement-panel button {padding:12px;cursor:pointer;white-space:normal;}
    #measurement-panel button:disabled {opacity:.45;cursor:default;}
      .card:has(#btn-bench), .card:has(#speed-slider), .card:has(#btn-fwd),
    details:has(#kp1), .card:has(#btn-record-toggle) {display:none;}
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
    <div class="card" style="border:2px solid #00f0ff">
      <div class="card-title">HIỆU CHUẨN NÂNG CAO · 56 BÀI</div>
      <p style="line-height:1.6">Firmware: <b>)rawliteral" TWIP_WEB_BUILD R"rawliteral(</b></p>
      <p style="line-height:1.6"><a href="#measurement-panel" style="color:#00f0ff">MỞ BẢNG BƯỚC ĐO ĐÃ CHUẨN BỊ SẴN</a> · <a href="/firmware-info" style="color:#00f0ff">KIỂM TRA PHIÊN BẢN ESP32</a></p>
    </div>

    <div class="card"><a href="/">← TRỞ VỀ TRANG VẬN HÀNH XE</a><p>Kê chắc xe, bánh quay tự do. Chỉ mở chế độ này khi cần đo linh kiện; muốn trở lại vận hành phải khởi động lại STM32.</p><label><input id="advanced-fixture" type="checkbox"> Tôi đã kê chắc xe, hai bánh không chạm đất.</label><button class="action" onclick="if(document.getElementById('advanced-fixture').checked)sendRaw('$DIAG*')">MỞ CHẾ ĐỘ HIỆU CHUẨN</button><p>Chuỗi kiểm tra nhanh sẽ chờ ổn định trước khi quay motor, sau đó mới chọn được các bài đo bên dưới.</p></div>
    <div class="card" id="balance-panel" style="display:none">
      <div class="card-title">THỬ CÂN BẰNG · CHỌN R12</div>
      <p>Vận hành, chọn R12 và tải log tại trang chính <a href="/ui/r12">R12</a>. Giữ yên 0,2 giây: R12 cho phép ±60° trước khi bật.</p>
      <p>Khởi động: giữ xe yên 5 giây để đo gyro. Không tự quay bánh, không tự bật cân bằng. Không điều khiển tiến/lùi ở bản thử này.</p>
      <button class="action" id="balance-arm" disabled onclick="balanceArm()">MỞ TRANG CHỌN R12</button>
      <button class="action btn-estop" onclick="sendRaw('$STOP*')">DỪNG MOTOR</button>
      <button class="action" id="balance-calib" onclick="sendRaw('$CALIB*')">ĐO LẠI GYRO · GIỮ XE YÊN 5 GIÂY</button>
      <pre id="balance-live" style="white-space:pre-wrap;font-size:13px;line-height:1.7">Chờ STM32 BAL-20261024-R12-8DIR-RT8…</pre>
      <canvas id="balance-chart" width="700" height="180" style="width:100%;background:#0d1424"></canvas>
      <p>Đồ thị: pitch điều khiển (xanh, ±10°) và quãng đường encoder (cam, ±10 cm), 60 giây gần nhất.</p>
      <button class="action" onclick="location.href='/balance-log'">TẢI LOG CÂN BẰNG 100 Hz</button>
      <button class="action" onclick="balanceClear()">XÓA LOG CŨ · CHUẨN BỊ LẦN MỚI</button>
      <p>Log lưu trên ESP32 từ khi bật, thêm 2 giây sau dừng; tối đa 18.000 mẫu, đầy thì giữ phần đã thu và báo thiếu. Bấm DỪNG, đợi 2 giây rồi tải. Tải xong mới xóa log cũ. Mất nguồn ESP32 sẽ mất log chưa tải.</p>
      <details><summary>Chuyển sang đo linh kiện / hiệu chuẩn 56 lượt</summary>
        <p>Chỉ bật khi đã kê chắc xe, hai bánh quay tự do. Chế độ này chạy chuỗi kiểm tra rồi cho chọn bài đo; muốn trở lại cân bằng phải khởi động lại STM32.</p>
        <label><input type="checkbox" id="balance-fixture"> Tôi đã kê chắc xe, hai bánh không chạm đất.</label>
        <button class="action" onclick="if(document.getElementById('balance-fixture').checked)sendRaw('$DIAG*')">BẬT CHẾ ĐỘ ĐO LINH KIỆN</button>
      </details>
    </div>
    <!-- Diagnosis: biet xe co hoat dong dung khong -->
    <div id="diag" class="diag info"><span class="dot"></span><span id="diag-text">Dang ket noi toi STM32...</span></div>

    <!-- Bench Test toggle card -->
    <div class="card" style="border-color: rgba(217, 119, 6, 0.45); background: linear-gradient(180deg, #181928, #131724);">
      <button id="btn-bench" class="btn-bench" onclick="toggleBenchTest()">
        <span id="bench-icon">🧪</span>
        <span id="bench-txt">BẬT CHẾ ĐỘ TEST BÀN (QUAY BÁNH TRỰC TIẾP)</span>
      </button>
      <div style="font-size:11px; color:var(--dim); margin-top:8px; line-height:1.4;">
        Chế độ Test Bàn dành cho firmware điều khiển. Khi đang dùng firmware đo linh kiện, nút này bị khóa; sử dụng bảng ĐO HIỆU CHUẨN THEO TỪNG BÀI bên dưới.
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
        <span class="label">Pitch điều khiển (+ về trước)</span>
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

    <div class="card" id="measurement-panel">
      <div class="card-title">ĐO HIỆU CHUẨN THEO TỪNG BÀI</div>
      <label>Bước đo đã chuẩn bị sẵn <select id="measure-plan" onchange="selectMeasurementPreset()" style="width:100%"></select></label>
      <div class="btn-row"><button id="measure-next" onclick="nextMeasurementPreset()">CHỌN BƯỚC TIẾP THEO</button>
        <button id="measure-mode" onclick="toggleMeasurementMode()">CHỈNH TAY NÂNG CAO</button></div>
      <p id="measure-preparation" style="white-space:pre-wrap;color:#fde68a;line-height:1.6"></p>
      <p>Chế độ chuẩn bị sẵn tự điền và khóa thông số, ghi chú. Bấm bắt đầu xác nhận bạn đã làm đúng hướng dẫn chuẩn bị. Hệ thống không tự biết tư thế, số vòng hay điện áp thực tế.</p>
      <p style="line-height:1.6">Giữ nguồn LiPo. Chờ chuỗi khởi động kết thúc rồi chọn bài đo. Mỗi bài có 5 giây ổn định trước khi đo; xe không tự cân bằng trong chế độ này.</p>
      <label>Bài đo <select id="measure-kind" onchange="measurementGuide()" style="width:100%;padding:12px">
        <option value="1">IMU đứng yên / đo lại sau khi máy ấm</option>
        <option value="2">IMU tư thế tham chiếu / 6 mặt</option>
        <option value="3">Nghiêng tay chậm để kiểm tra dấu</option>
        <option value="4">Encoder: quay bánh bằng tay nhiều vòng</option>
        <option value="10">Ngưỡng quay motor: 3 lần mỗi chiều</option>
        <option value="5">Motor trái: 4 mức × 2 chiều × 3 lần</option>
        <option value="6">Motor phải: 4 mức × 2 chiều × 3 lần</option>
        <option value="7">Rung IMU: trái / phải / hai motor</option>
        <option value="8">Xung motor tùy chỉnh, có giới hạn</option>
        <option value="11">Xung motor rồi thả trôi (đối chiếu phanh)</option>
        <option value="9">Xung có tải trên giá thử riêng</option>
      </select></label>
      <p id="measure-guide" style="white-space:pre-wrap;line-height:1.6"></p>
      <div style="display:flex;flex-wrap:wrap;gap:12px">
        <label>Thời gian đo/xung (ms)<input id="measure-duration" type="number" value="30000" min="100" max="60000" style="width:100%"></label>
        <label>PWM xung / rung<input id="measure-pwm" type="number" value="600" min="-1800" max="1800" style="width:100%"></label>
        <label>Góc tham chiếu (°) hoặc số vòng bánh<input id="measure-reference" type="number" value="0" step="0.1" style="width:100%"></label>
        <label>Motor / tư thế<select id="measure-tag" style="width:100%;padding:10px"></select></label>
      </div>
      <label>Ghi chú phép đo (điện áp đo bằng đồng hồ, giá kê, chiều tiến…)<textarea id="measure-note" maxlength="180" rows="3" style="width:100%"></textarea></label>
      <p id="measure-ready-row"><label><input type="checkbox" id="measure-ready"> Tôi đã kê chắc / chuẩn bị giá thử phù hợp; bánh và dây không vướng. Bài có tải cần giá chống đổ riêng.</label></p>
      <p><label><input type="checkbox" id="measure-auto" checked> Yêu cầu tự tải log khi bài đo kết thúc</label></p>
      <div class="btn-row">
        <button class="action" id="measure-start" onclick="startMeasurement()">BẮT ĐẦU BÀI ĐÃ CHỌN</button>
        <button class="action btn-estop" onclick="stopMeasurement()">DỪNG / GIỮ PHANH</button>
      </div>
      <p id="measure-status" style="white-space:pre-wrap;line-height:1.6">Đang kiểm tra bộ nhớ log…</p>
      <pre id="measure-live" style="white-space:pre-wrap;font-size:13px;line-height:1.6"></pre>
      <div id="measure-files"></div>
      <p style="font-size:12px;line-height:1.6">Giữ trang ở phía trước khi motor chạy. Mất nhịp điều khiển quá 1,5 giây sẽ dừng motor. Tự tải phụ thuộc trình duyệt; nếu iPhone/laptop chặn, bấm TẢI TXT ở từng bài. Log chỉ mất khi bạn xóa hoặc tắt/reset ESP32. Hệ thống không tự xóa sau tải, không ghi đè khi đầy; giữ tối đa 4 bài trong RAM.</p>
    </div>

    <div class="card">
      <div class="card-title">CHUỖI KIỂM TRA NHANH KHI KHỞI ĐỘNG</div>
      <p style="font-size:13px;line-height:1.5">Kê xe chắc chắn, hai bánh quay tự do trước khi cắm pin.
      Bấm BẬT CHẾ ĐỘ ĐO LINH KIỆN ở bảng cân bằng để chạy chuỗi test. Kết thúc giữ phanh, không tự cân bằng.</p>
      <div id="hardware-status" style="white-space:pre-wrap;font-size:13px;line-height:1.6">Đang lấy trạng thái từ ESP32…</div>
      <pre id="hardware-values" style="white-space:pre-wrap;font-size:12px;line-height:1.5"></pre>
      <pre id="hardware-history" style="white-space:pre-wrap;font-size:12px;line-height:1.7"></pre>
      <div class="btn-row">
        <a class="action btn-export" href="/hardware-log">📥 TẢI LOG LINH KIỆN</a>
        <button class="action" onclick="sendRaw('$STOP*')">DỪNG TEST</button>
      </div>
      <p style="font-size:12px;line-height:1.5">ESP32 giữ dữ liệu dù chưa mở web. Có thể tải khi đang chạy;
      tải lại sau khi báo hoàn tất để có đủ mẫu. Đọc encoder 200 Hz, BMI160 thực tế 100 Hz.
      LED/còi chỉ kiểm tra lệnh xuất; không có cảm biến xác nhận sáng/kêu.
      Chưa đo điện áp/dòng pin, mô-men tải hoặc chiều tiến vật lý.</p>
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
          <input type="number" id="kp1" value="140" step="10">
        </div>
        <div class="tune-field">
          <label>Kd Goc (Giam chan)</label>
          <input type="number" id="kd1" value="20" step="0.1">
        </div>
        <div class="tune-field">
          <label>Kp Van Toc</label>
          <input type="number" id="kp2" value="48" step="0.1">
        </div>
        <div class="tune-field">
          <label>Ki Van Toc</label>
          <input type="number" id="ki2" value="0.15" step="0.02">
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
    var hardwareLocked = false;
    var hardwareLastReceived = 0;
    var hardwareNames = ['Chờ ổn định (10 s)', 'BMI160 đứng yên (5 s)', 'LED và còi',
      'Deadband trái +', 'Deadband trái −', 'Deadband phải +', 'Deadband phải −',
      'Đáp ứng bậc thang trái', 'Đáp ứng bậc thang phải', 'Hai động cơ cùng chạy',
      'BMI160 sau test', 'HOÀN TẤT — GIỮ PHANH', 'ĐÃ DỪNG — GIỮ PHANH', 'ĐANG ĐO BÀI HIỆU CHUẨN RIÊNG'];
    async function refreshHardwareTest() {
      try {
        var response = await fetch('/hardware-status', {cache:'no-store'});
        if (!response.ok) throw new Error('HTTP ' + response.status);
        var data = await response.json();
        hardwareLocked = (data.seen || Boolean(data.meta)) && !data.balance_live;
        hardwareLastReceived = Date.now() - data.age_ms;
        var status = document.getElementById('hardware-status');
        var stale = data.age_ms > 3000;
        status.textContent = (hardwareLocked ? hardwareNames[data.stage] || 'Không rõ giai đoạn' : 'Chưa nhận dữ liệu STM32') +
          '\nLog khởi động đã lưu: ' + data.samples + ' mẫu · thời gian mẫu mới nhất: ' + (data.t_ms / 1000).toFixed(2) + ' s' +
          '\nLỗi CRC: ' + data.crc_errors + ' · mẫu thiếu: ' + data.gaps + ' · tràn bộ nhớ: ' + data.overflow +
          '\nBộ nhớ log: ' + (data.capacity / 1048576).toFixed(2) + ' MB' +
          (data.stage === 11 ? '\nMotor giữ phanh. Để đo nghiêng IMU có log riêng, chọn bài Nghiêng tay chậm ở bảng hiệu chuẩn.' : '') +
          (data.capacity < 1048576 ? ' — cần bật OPI PSRAM để giữ đủ phiên test' : '') +
          (stale && hardwareLocked ? '\nKhông có dữ liệu mới: kiểm tra nguồn/kết nối STM32.' : '');
        if (data.overflow > 0) status.textContent =
          'LOG KHÔNG ĐẦY ĐỦ: đã bỏ ' + data.overflow + ' mẫu vì tràn bộ nhớ.\n' + status.textContent;
        else if (data.capacity < 1048576) status.textContent =
          'BỘ NHỚ KHÔNG ĐỦ CHO TOÀN PHIÊN: cần nạp ESP32 với PSRAM = OPI PSRAM.\n' + status.textContent;
        if (data.gaps > 0 || data.crc_errors > 0) status.textContent =
          'LOG BỊ MẤT/LỖI GÓI UART: cần kiểm tra dữ liệu trước khi tính thông số motor.\n' + status.textContent;
        var row = data.latest.split(',').map(Number);
        var meta = data.meta.split(',');
        var causes = [];
        [[2,'trễ lấy mẫu'],[4,'lệnh STOP'],[8,'lỗi đọc I2C'],[16,'không thấy xung encoder'],
         [32,'xe/giá kê bị chuyển động'],[64,'cấu hình BMI160 không đúng'],[128,'xe chưa đứng yên']].forEach(function(item) {
          if (data.flags & item[0]) causes.push(item[1]);
        });
        var details = causes.length ? 'Nguyên nhân dừng: ' + causes.join('; ') + '\n' : '';
        if (row.length === 16) {
          details += 'PWM trái/phải: ' + row[5] + ' / ' + row[6] + '\n' +
            'Encoder thô trái/phải: ' + row[7] + ' / ' + row[8] + ' ticks\n' +
            'Gyro X/Y/Z: ' + row.slice(9,12).map(function(x){return (x/16.4).toFixed(3);}).join(' / ') + ' °/s (chưa trừ bias)\n' +
            'Accel X/Y/Z: ' + row.slice(12,15).map(function(x){return (x/4096).toFixed(4);}).join(' / ') + ' g\n' +
            'Chu kỳ thực: ' + row[2] + ' µs · mất mẫu STM32: ' + row[15] + '\n';
        }
        if (meta.length >= 20) {
          details += 'Bản test STM32: ' + meta[1] + ' (bản mới = 3; thêm 30 s ghi IMU sau test)\n' +
            'BMI160 addr/ID: 0x' + Number(meta[6]).toString(16) + ' / 0x' + Number(meta[7]).toString(16) +
            '\nThanh ghi ACC_CONF/RANGE, GYR_CONF/RANGE: ' + meta.slice(8,12).map(function(x){return '0x'+Number(x).toString(16);}).join(' / ') +
            '\nBias gyro X/Y/Z: ' + meta.slice(12,15).join(' / ') + ' °/s\n' +
            'Ngưỡng quay (PWM), L+ L− R+ R−: ' + meta.slice(15,19).join(' / ') + ' (0 = chưa xác định)';
        }
        var history = [];
        var visited = meta.length >= 22 ? Number(meta[20]) : 0;
        var abortFrom = meta.length >= 22 ? Number(meta[21]) : 255;
        if (data.stage === 12 && abortFrom < 11) {
          details += '\nDừng tại ID ' + abortFrom + ': ' + hardwareNames[abortFrom];
        } else if (data.stage === 12 && abortFrom === 255) {
          details += '\nDừng khi khởi tạo, trước giai đoạn chờ ổn định.';
        }
        for (var i = 0; i <= 10; i++) {
          var entered = (visited & (1 << i)) !== 0;
          var label = 'Chưa chạy';
          if (entered) {
            if (data.stage === 12 && i === abortFrom) label = 'DỪNG TẠI ĐÂY';
            else if (i === data.stage) label = 'Đang chạy';
            else if (data.stage === 11 || i < data.stage) label = 'Đã kết thúc bước';
            else label = 'Đã vào bước';
          }
          history.push('ID ' + i + ' · ' + hardwareNames[i] + ': ' + label);
        }
        document.getElementById('hardware-history').textContent = meta.length >= 22 ?
          'LỊCH SỬ DO STM32 GHI NHẬN (giữ lại khi mở web muộn)\n' + history.join('\n') +
          '\nID 11 = hoàn tất; ID 12 = dừng lỗi/STOP, không phải bước test bắt buộc.' :
          'Chưa có lịch sử giai đoạn. Cần nạp bản STM32 mới để ghi lịch sử và vị trí dừng.';
        document.getElementById('hardware-values').textContent = details;
        if (data.stop) {
          var trigger = data.stop.split(',');
          if (trigger.length === 11) {
            document.getElementById('hardware-values').textContent +=
              '\nTẠI LÚC DỪNG: t=' + trigger[2] + ' ms, ID ' + trigger[1] +
              '\nGóc accel lệch: ' + trigger[3] + '°; norm accel: ' + trigger[7] + ' g' +
              '\nGyro đã trừ bias X/Y/Z: ' + trigger.slice(4,7).join(' / ') + ' °/s' +
              '\nPWM trước phanh L/R: ' + trigger[8] + ' / ' + trigger[9] +
              '\nNgưỡng kích hoạt: ' + ((Number(trigger[10]) & 1) ?
                (Number(meta[1]) >= 2 ? 'góc accel >8° liên tục 250 ms hoặc >25° tức thời; ' : 'góc từ gia tốc >8° ') : '') +
              ((Number(trigger[10]) & 2) ? 'gyro >120°/s' : '');
          }
        }
        if (hardwareLocked) document.getElementById('btn-export-txt').disabled = false;
        if (hardwareLocked) {
          document.getElementById('btn-bench').disabled=true;
          document.getElementById('bench-txt').textContent='ĐANG DÙNG FIRMWARE ĐO — CHỌN BÀI Ở BẢNG HIỆU CHUẨN';
          document.getElementById('speed-slider').disabled=true;
          var state = data.stage === 11 ? 9 : (data.stage === 12 ? 10 : 8);
          lastState = state;
          document.getElementById('state-badge').textContent = hardwareNames[data.stage];
          document.getElementById('tel-state').textContent = hardwareNames[data.stage];
          document.getElementById('tel-batt').textContent = 'Chưa đo';
          if (row.length === 16) {
            document.getElementById('tel-pwml').textContent = row[5];
            document.getElementById('tel-pwmr').textContent = row[6];
            document.getElementById('tel-pitch').textContent = (Math.atan2(row[12],row[14])*180/Math.PI).toFixed(2) + '° (accel)';
            document.getElementById('tel-gyro').textContent = (row[10]/16.4).toFixed(3) + ' °/s (raw)';
          }
        }
      } catch (error) {
        document.getElementById('hardware-status').textContent = 'Chưa lấy được trạng thái test: ' + error.message;
      }
    }
    refreshHardwareTest();
    setInterval(refreshHardwareTest, 1000);

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
      if (hardwareLocked && txt !== '$STOP*' && txt !== '$BACKGROUND*' && txt !== '$LEAVE*') return;
      if (ws && ws.readyState === WebSocket.OPEN) ws.send(txt);
    }

    /* ---------------- Gui lenh 20Hz ---------------- */
    setInterval(function () {
      if (!hardwareLocked && !document.hidden && ws && ws.readyState === WebSocket.OPEN) {
        ws.send("$PING*");
      }
    }, 50);

    /* ---------------- Che do Test Ban (Bench Test) ---------------- */
    function toggleBenchTest() {
      if(hardwareLocked)return;
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
      alert("Hai chế độ R12 dùng bộ thông số cố định. Chọn chế độ và chạy thử ở trang vận hành."); return;
      var kp1 = parseFloat(document.getElementById("kp1").value);
      var kd1 = parseFloat(document.getElementById("kd1").value);
      var kp2 = parseFloat(document.getElementById("kp2").value);
      var ki2 = parseFloat(document.getElementById("ki2").value);
      sendRaw("$PID," + kp1 + "," + kd1 + "," + kp2 + "," + ki2 + "*");
    }

    function adjustTrim(delta) {
      alert("Hai chế độ R12 giữ mốc góc cố định; không chỉnh pitch trim từ giao diện cũ."); return;
      var el = document.getElementById("pitch-trim");
      var val = (parseFloat(el.value || "0") + delta).toFixed(2);
      el.value = val;
      sendRaw("$TRIM," + val + "*");
    }

    function sendCalib() {
      if (confirm("Dat xe NAM YEN tren mat phang trong ~5 giay de hieu chuan lai Gyro. Tiep tuc?")) {
        sendRaw("$CALIB*");
      }
    }

    /* ---------------- Data Logger & Export TXT ---------------- */
    var isRecording = false;
    var logBuffer = [];
    var logStartTime = 0;
    var maxPitchRecorded = 0.0;
    var autoRecordEnabled = false; // Authoritative 100Hz log resides on ESP32.

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
      if(!hardwareLocked){location.href="/balance-log";return;}
      if (hardwareLocked) { location.href = '/hardware-log'; return; }
      if (logBuffer.length === 0) {
        alert("Chưa có dữ liệu nào được ghi! Hãy cho xe chạy cân bằng trước.");
        return;
      }

      var kp1 = document.getElementById("kp1").value || "54.1";
      var kd1 = document.getElementById("kd1").value || "4.02";
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
      document.getElementById("tel-batt").textContent = "Chưa đo";
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
      var age = Date.now() - (hardwareLocked ? hardwareLastReceived : lastTel);
      var fresh = age < (hardwareLocked ? 3000 : 800);

      var link = document.getElementById("link-badge");
      if (fresh) { link.textContent = "KET NOI OK"; link.className = "badge link-ok"; }
      else { link.textContent = "MAT TIN HIEU"; link.className = "badge link-bad"; }

      document.getElementById("tel-rate").innerHTML = hardwareLocked ? '200<small> Hz đọc</small>' : (fresh ? telCount + '<small> /s</small>' : '--<small> /s</small>');
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
          case 8: cls = 'info'; txt = 'Đang kiểm tra linh kiện tự động. Xe giữ trên giá kê; tải bằng nút TẢI LOG LINH KIỆN.'; break;
          case 9: cls = 'ok'; txt = 'Hoàn tất thu dữ liệu, giữ phanh. Đây chưa phải xác nhận xe cân bằng được. Tải log linh kiện để phân tích.'; break;
          case 10: cls = 'err'; txt = 'Test đã dừng và giữ phanh. Xem nguyên nhân ở bảng linh kiện rồi tải log.'; break;
          case 0: cls = "info"; txt = "STM32 đang khởi động..."; break;
          case 1: cls = "warn"; txt = "Đang hiệu chuẩn IMU (500 mẫu). Hãy giữ xe đứng yên..."; break;
          case 2:
            cls = "warn";
            txt = 'Đang chờ. Giữ xe đứng thẳng và yên, rồi bấm BẬT CÂN BẰNG.';
            break;
          case 3:
            cls = "ok";
            txt = "Đang chạy bộ điều khiển cân bằng. Quan sát góc, quãng đường và tải log để đánh giá kết quả.";
            break;
          case 4:
            cls = "ok";
            txt = "RACING: Chế độ đua tốc độ cao - đang tự cân bằng.";
            break;
          case 5:
            cls = "warn";
            txt = "Đã dừng motor. Xem nguyên nhân tại bảng cân bằng. Xe chỉ chạy lại khi bấm BẬT CÂN BẰNG.";
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
  <script>
    // Independent calibration captures. Never erase data after a browser download.
    var measurementNames={1:'BMI160 đứng yên / sau làm ấm',2:'BMI160 tư thế chuẩn / sáu mặt',3:'BMI160 nghiêng chậm',4:'Encoder quay tay',5:'Đáp ứng motor trái',6:'Đáp ứng motor phải',7:'So sánh rung motor',8:'Xung PWM rồi phanh',9:'Xung có tải trên đồ gá',10:'Deadband bước nhỏ',11:'Xung PWM rồi thả trôi'};
    var measurementActive=0, measurementOwned=0, measurementPolling=false, measurementPinging=false, measurementStarting=false;
    var measurementAutoAttempted={};
    var measurementOwner=Date.now().toString(36)+Math.random().toString(36).slice(2);
    try {measurementOwner=sessionStorage.getItem('measure-owner')||measurementOwner;sessionStorage.setItem('measure-owner',measurementOwner);measurementOwned=Number(sessionStorage.getItem('measure-owned')||0);}catch(e){}
    function measureEl(id){return document.getElementById('measure-'+id);}
    var measurementPresetMode=true,measurementPlan=[];
    function buildMeasurementPlan(){
      measurementPlan=[];
      function add(title,kind,duration,pwm,reference,tag,code,prepare){measurementPlan.push({title:title,kind:kind,duration:duration,pwm:pwm,reference:reference,tag:tag,code:code,prepare:prepare});}
      var fixed='Cố định thân xe trên giá chắc; hai bánh rời đất, dây không vướng. Không chạm xe khi motor chạy. Giữ web phía trước màn hình.';
      add('IMU đứng yên ban đầu',1,30000,600,0,0,'STATIC_INITIAL','Giữ khung bất động ở tư thế thẳng. Thu ngay đầu phiên; nếu máy đã chạy lâu, gửi kèm thông tin đó.');
      ['+X','-X','+Y','-Y','+Z','-Z'].forEach(function(axis,i){add('IMU mặt '+axis,2,20000,600,0,i+1,'FACE_'+axis.replace('+','P').replace('-','N'),'Đặt trục dương '+axis.slice(1)+' của BMI160 hướng thẳng '+(i%2?'xuống':'lên')+'; trục đó cần gần '+(i%2?'-1':'+1')+' g. Giữ cố định trước khi bấm. Nếu chưa biết trục hoặc không gá được, bỏ qua bước và báo lại; không đoán tư thế.');});
      [0,-5,5,-10,10].forEach(function(angle){add('Góc chuẩn '+angle+'°',2,20000,600,angle,0,'ANGLE_'+angle,'Dùng thước góc độc lập đặt '+angle+'° theo mốc trục cảm biến; trước là âm, sau là dương. Không lấy góc trên web làm chuẩn. Nếu chỉ đo góc khung và chưa biết độ lệch lắp, chưa chạy preset góc này; gửi mốc đo cho mình.');});
      add('Nghiêng chậm trước / sau',3,30000,600,0,0,'TILT_SIGN','Sau 5 s đầu: nghiêng CHẬM về WAGO pin 5–10°, giữ 2 s, về giữa; nghiêng về WAGO 5 V, giữ 2 s, về giữa. Lặp 3 lần; bánh rời đất, motor không chạy.');
      ['trái','phải'].forEach(function(wheel,w){[1,-1].forEach(function(sign){for(var rep=1;rep<=3;rep++)add('Encoder '+wheel+' · '+(sign>0?'trước':'sau')+' · lần '+rep,4,45000,600,10,w,'ENC_'+(w?'R':'L')+'_'+(sign>0?'F':'B')+'_R'+rep,'Đánh dấu bánh '+wheel+'. Chờ 5 s đầu rồi quay CHÍNH XÁC 10 vòng '+(sign>0?'về trước (đỉnh bánh về WAGO pin)':'về sau (đỉnh bánh về WAGO 5 V)')+', dừng đúng vạch và giữ yên tới hết bài. Không ép hộp số; quay sai thì báo lượt không hợp lệ, không coi log là đủ 10 vòng.');});});
      ['trái','phải'].forEach(function(wheel,w){add('Deadband '+wheel+' · tự lặp 3 lần/chiều',10,45000,600,0,w,'DB_'+(w?'R':'L'),fixed);});
      [5,6].forEach(function(kind){add('Dãy motor '+(kind===5?'trái':'phải')+' · 53 giây',kind,48000,600,0,0,'STEPS_'+(kind===5?'L':'R'),fixed);});
      add('So sánh rung · PWM600',7,36000,600,0,0,'VIBRATION_600',fixed);
      ['trái','phải'].forEach(function(wheel,w){[600,-600].forEach(function(pwm){for(var rep=1;rep<=3;rep++)[8,11].forEach(function(kind){add('Xung '+wheel+' '+pwm+' · '+(kind===8?'phanh':'thả trôi')+' · lần '+rep,kind,200,pwm,0,w,'PULSE_'+(w?'R':'L')+'_'+pwm+'_'+(kind===8?'BRAKE':'COAST')+'_R'+rep,fixed+' Một xung 200 ms sau 5 s đầu; không chạm bánh để giúp dừng.');});});});
      add('IMU đứng yên sau làm ấm',1,30000,600,0,0,'STATIC_AFTER_WARMUP','Giữ nguyên tư thế IMU ban đầu. Chỉ chạy sau khi điện tử đã hoạt động ít nhất khoảng 5 phút; nếu vừa reset, chờ đủ rồi đo. Không tự coi đây là nhiệt độ chip đã đo.');
      add('TÙY CHỌN · Xung có tải trên đồ gá riêng',9,100,300,0,0,'LOAD_FIXTURE_OPTIONAL','CHỈ chạy khi có đồ gá thử tải chống đổ/chạy đi và cho phép bánh trái quay dưới tải xác định. Không ghì tay, không khóa rotor. Không có đồ gá thì bỏ qua; tải/cách gá thực cần gửi bổ sung, hệ thống không tự đo được.');
    }
    function initializeMeasurementPlan(){
      buildMeasurementPlan();var select=measureEl('plan');select.innerHTML='';
      measurementPlan.forEach(function(p,i){var o=document.createElement('option');o.value=i+1;o.textContent=String(i+1).padStart(2,'0')+' / '+measurementPlan.length+' · '+p.title;select.appendChild(o);});
      var saved=1;try{saved=Number(sessionStorage.getItem('measure-plan-v1')||1);}catch(e){}
      select.value=Math.min(Math.max(saved,1),measurementPlan.length);selectMeasurementPreset();
    }
    function selectMeasurementPreset(){
      if(!measurementPresetMode)return;
      var index=Number(measureEl('plan').value)-1,p=measurementPlan[index];if(!p)return;
      measureEl('kind').value=p.kind;measurementGuide();
      ['duration','pwm','reference','tag'].forEach(function(key){measureEl(key).value=p[key];});
      measureEl('note').value='plan=v1;step='+String(index+1).padStart(2,'0')+';requested='+p.code+';actual_conditions_unverified';
      ['kind','duration','pwm','reference','tag','note'].forEach(function(key){measureEl(key).disabled=true;});
      measureEl('ready-row').style.display='none';
      measureEl('start').textContent='ĐÃ CHUẨN BỊ ĐÚNG — BẮT ĐẦU BƯỚC '+(index+1);
      measureEl('preparation').textContent='BƯỚC '+(index+1)+' / '+measurementPlan.length+' · '+p.title+'\n'+p.prepare+'\nGhi chú tự điền mô tả bài yêu cầu, không xác nhận thao tác thực tế đã đúng.';
      try{sessionStorage.setItem('measure-plan-v1',String(index+1));}catch(e){}
    }
    function nextMeasurementPreset(){
      if(measurementActive||measurementStarting)return;
      measureEl('plan').value=Math.min(Number(measureEl('plan').value)+1,measurementPlan.length);selectMeasurementPreset();
    }
    function toggleMeasurementMode(){
      if(measurementActive||measurementStarting)return;
      measurementPresetMode=!measurementPresetMode;
      measureEl('plan').disabled=!measurementPresetMode;measureEl('next').disabled=!measurementPresetMode;
      measureEl('mode').textContent=measurementPresetMode?'CHỈNH TAY NÂNG CAO':'DÙNG LẠI BƯỚC CHUẨN BỊ SẴN';
      measureEl('ready-row').style.display=measurementPresetMode?'none':'';
      if(measurementPresetMode)selectMeasurementPreset();
      else{measureEl('kind').disabled=measureEl('note').disabled=false;measurementGuide();measureEl('note').value='';measureEl('preparation').textContent='Chế độ chỉnh tay: bạn tự chịu trách nhiệm thông số và ghi chú thực tế.';measureEl('start').textContent='BẮT ĐẦU BÀI ĐÃ CHỌN';}
    }
    function measurementGuide(){
      var k=Number(measureEl('kind').value), d=measureEl('duration'),p=measureEl('pwm'),r=measureEl('reference'),t=measureEl('tag');
      var guides={
        1:'Cố định thân xe, không chạm xe. Thu 30–60 s để đo bias/nhiễu gyro và accel; lặp lại sau khi điện tử hoạt động 5 phút. Không tự sửa bias.',
        2:'Chọn lần lượt +X, −X, +Y, −Y, +Z, −Z; đặt cảm biến đúng mặt và giữ yên bằng gá. Mỗi mặt là một log. Hoặc chọn tư thế pitch rồi nhập góc đo độc lập bằng thước góc. Không dùng chính góc trên web làm chuẩn.',
        3:'Motor tắt. Sau 5 s, nghiêng CHẬM 5–10° về phía WAGO pin, giữ 2 s, về giữa, rồi phía WAGO 5 V; lặp 3 lần. Ghi rõ chiều trong ghi chú để kiểm tra dấu pitch và gyro.',
        4:'Hai bánh rời đất; driver thả trôi sau 5 s. Đánh dấu bánh, quay đúng 10 vòng về phía trước, dừng đúng vạch rồi giữ yên trước khi hết giờ. Làm riêng mỗi bánh; nhập số vòng thực sự đã quay vào ô tham chiếu. Không tự suy ra chiều tiến từ encoder.',
        5:'Kê CHẮC thân xe, bánh trái quay tự do. Tự chạy ±600/1000/1400/1800; 1 s quay + 1 s phanh, mỗi mức lặp 3 lần. Tổng 53 s gồm 5 s chuẩn bị. Ghi điện áp driver đo bằng đồng hồ trước/sau nếu có.',
        6:'Kê CHẮC thân xe, bánh phải quay tự do. Tự chạy ±600/1000/1400/1800; 1 s quay + 1 s phanh, mỗi mức lặp 3 lần. Tổng 53 s gồm 5 s chuẩn bị.',
        7:'So sánh motor trái / phải / cả hai, hai chiều, mỗi trường hợp lặp 3 lần: 1 s quay + 1 s phanh. Tổng 41 s. Kê xe chắc và giữ nguyên cách gá IMU.',
        8:'Một xung PWM có dấu, tối đa 700 ms, rồi phanh 2 s. Chọn bánh cần đo; có 5 s chuẩn bị. Dùng để khảo sát mức PWM khác và tốc độ tăng/giảm; bánh phải rời đất.',
        9:'CHỈ dành cho đồ gá thử tải chống đổ và chống xe chạy đi, có thể quay bánh an toàn. Không giữ bằng tay, không khóa cứng rotor. Xung tối đa 200 ms và |PWM|≤800. Không có cảm biến lực/dòng nên bài này chưa đo được mô-men.',
        10:'Kê chắc, bánh được chọn quay tự do. Tăng PWM từng 20, giữ 200 ms/mức, đến khi encoder đổi ≥4 ticks; đo +/− luân phiên 3 lần, có phanh giữa lần. Giới hạn 600; không đáp ứng sẽ dừng và giữ log.',
        11:'Cùng xung và cách gá như bài Xung rồi phanh, nhưng sau xung driver thả trôi 3 s rồi giữ phanh. Dùng đối chiếu suy giảm tốc do ma sát với phanh điện. Bánh phải rời đất; tối đa 700 ms/xung.'
      };
      measureEl('guide').textContent=guides[k]+' Mỗi bài có 5 giây ổn định đầu tiên; vẫn lưu đầy đủ các mẫu này.';
      d.value=({1:30000,2:20000,3:30000,4:45000,5:48000,6:48000,7:36000,8:200,9:100,10:45000,11:200})[k];
      d.max=[8,11].includes(k)?700:k==9?200:60000;d.disabled=[5,6,7,10].includes(k);
      p.value=k==9?300:600;p.max=k==9?800:1800;p.min=-(Number(p.max));p.disabled=![7,8,9,11].includes(k);
      r.value=k==4?10:0;r.disabled=![2,4].includes(k);
      t.innerHTML='';
      var options=k==2?[[0,'Pitch theo góc tham chiếu (độ)'],[1,'X gần +1 g'],[2,'X gần −1 g'],[3,'Y gần +1 g'],[4,'Y gần −1 g'],[5,'Z gần +1 g'],[6,'Z gần −1 g']]:[[0,'Bánh trái'],[1,'Bánh phải']];
      if([4,8,9,11].includes(k))options.push([2,'Cả hai bánh']);
      options.forEach(function(o){var x=document.createElement('option');x.value=o[0];x.textContent=o[1];t.appendChild(x);});
      t.disabled=![2,4,8,9,10,11].includes(k);measureEl('ready').checked=false;
    }
    async function measurementPost(url,values){
      var response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(values).toString()});
      if(!response.ok)throw Error(await response.text());return response;
    }
    async function startMeasurement(){
      if(measurementActive||measurementStarting)return;
      if(measurementPresetMode)selectMeasurementPreset(); // always restore the reviewed preset before sending
      var k=Number(measureEl('kind').value),duration=Number(measureEl('duration').value),pwm=Number(measureEl('pwm').value),reference=Number(measureEl('reference').value),tag=Number(measureEl('tag').value);
      if(!Number.isInteger(duration)||duration<100||duration>60000||!Number.isInteger(pwm)||Math.abs(pwm)>1800||!Number.isFinite(reference)) {measureEl('status').textContent='Thông số chưa hợp lệ.';return;}
      if(k>=5&&!measurementPresetMode&&!measureEl('ready').checked){measureEl('status').textContent='Chưa xác nhận đồ gá và bánh quay an toàn.';return;}
      if(k==4&&(!Number.isInteger(reference)||reference<1||reference>100)){measureEl('status').textContent='Nhập số vòng nguyên 1–100 và quay đúng số vòng đó.';return;}
      if(k==2)reference=Math.round(reference*1000);
      if(![2,4].includes(k))reference=0;
      measurementStarting=true;measureEl('start').disabled=true;
      measureEl('plan').disabled=measureEl('next').disabled=measureEl('mode').disabled=true;
      try {
        var res=await measurementPost('/measurement-start',{owner:measurementOwner,kind:k,duration:duration,pwm:pwm,reference:reference,tag:tag,note:measureEl('note').value,plan:measurementPresetMode?Number(measureEl('plan').value):0,ready:measurementPresetMode||measureEl('ready').checked?'yes':'no'});
        var data=await res.json();measurementOwned=measurementActive=data.id;
        try{sessionStorage.setItem('measure-owned',String(data.id));}catch(e){}
        measureEl('status').textContent='Đã yêu cầu bài '+data.id+'. Chờ STM32 xác nhận; 5 s đầu ổn định.';
        await measurementPing();
      }catch(e){measureEl('status').textContent='Không bắt đầu: '+e.message;}
      finally{
        measurementStarting=false;measureEl('start').disabled=!!measurementActive;
        measureEl('plan').disabled=measureEl('next').disabled=!!measurementActive||!measurementPresetMode;
        measureEl('mode').disabled=!!measurementActive;
      }
    }
    async function measurementPing(){
      if(!measurementOwned||measurementActive!==measurementOwned||measurementPinging)return;
      measurementPinging=true;
      try{await measurementPost('/measurement-ping',{owner:measurementOwner});}catch(e){}
      finally{measurementPinging=false;}
    }
    async function stopMeasurement(){
      measurementOwned=0;
      try{await measurementPost('/measurement-stop',{});measureEl('status').textContent='Đã gửi STOP, chờ STM32 chốt phần log đã thu.';}
      catch(e){measureEl('status').textContent='Chưa gửi được STOP qua web. Nhịp xác nhận đã ngắt; motor sẽ dừng sau tối đa 1,5 s nếu đang chạy bài riêng.';}
    }
    function measurementDownload(id){
      var a=document.createElement('a');a.href='/measurement-log?id='+Number(id);a.download='twip_test_'+Number(id)+'.txt';
      document.body.appendChild(a);a.click();a.remove();
    }
    async function measurementDelete(id){
      if(!confirm('Chỉ xóa log '+id+' sau khi đã kiểm tra file TXT tải về đầy đủ. Xóa khỏi RAM ESP32?'))return;
      try{await measurementPost('/measurement-delete',{id:id,confirm:'delete'});await refreshMeasurements();}
      catch(e){measureEl('status').textContent='Chưa xóa: '+e.message;}
    }
    async function refreshMeasurements(){
      if(measurementPolling)return;measurementPolling=true;
      try{
        var response=await fetch('/measurement-status',{cache:'no-store'});if(!response.ok)throw Error('HTTP '+response.status);
        var data=await response.json();measurementActive=data.active;
        measureEl('plan').disabled=!!data.active||measurementStarting||!measurementPresetMode;
        measureEl('next').disabled=!!data.active||measurementStarting||!measurementPresetMode;
        measureEl('mode').disabled=!!data.active||measurementStarting;
        measureEl('start').disabled=!data.available||!!data.active||measurementStarting||data.jobs.length>=4;
        if(measurementOwned&&!data.jobs.some(function(j){return j.id===measurementOwned&&!j.sealed;}))measurementOwned=0;
        var active=data.jobs.find(function(j){return j.id===data.active;});
        var raw=(data.latest||'').split(',').map(Number);
        measureEl('live').textContent=active&&active.phase===1&&raw.length===16&&raw[3]===13?
          'Hiển thị khoảng 1 Hz; TXT giữ mẫu đọc 200 Hz.\nPWM L/R: '+raw[5]+' / '+raw[6]+'; encoder thô L/R: '+raw[7]+' / '+raw[8]+' ticks\n'+
          'Gyro RAW X/Y/Z: '+raw.slice(9,12).map(function(v){return (v/16.4).toFixed(3);}).join(' / ')+' °/s (chưa trừ bias)\n'+
          'Accel X/Y/Z: '+raw.slice(12,15).map(function(v){return (v/4096).toFixed(4);}).join(' / ')+' g\n'+
          'Pitch accel thô: '+(Math.atan2(raw[12],raw[14])*180/Math.PI).toFixed(3)+'°; chu kỳ: '+raw[2]+' µs; seq: '+raw[0]:'';
        measureEl('status').textContent=!data.available?'Không đủ PSRAM: bật OPI PSRAM và nạp lại ESP32.':active?('Bài '+active.id+' · '+measurementNames[active.kind]+' · '+(active.phase===0?'CHỜ STM32':'ĐANG THU')+' · '+(active.t_ms/1000).toFixed(2)+' s · '+active.samples+' mẫu. Giữ web ở phía trước màn hình.'):data.jobs.length>=4?'ĐÃ ĐẦY 4 LOG: tải, kiểm tra file rồi xác nhận xóa log cũ bên dưới để đo tiếp. Không tự ghi đè.':'Đang rảnh. Chọn bước, chuẩn bị tư thế/đồ gá rồi bắt đầu. Log RAM: '+data.jobs.length+'/4; khoảng '+(data.records_per_slot/200).toFixed(0)+' s/chỗ.';
        var list=measureEl('files');list.innerHTML='';
        data.jobs.forEach(function(j){
          var row=document.createElement('div');row.style.cssText='border-top:1px solid #475569;padding:12px 0;line-height:1.6';
          var text=document.createElement('p');
          var preset=j.plan&&measurementPlan[j.plan-1];
          text.textContent='#'+j.id+' · '+(preset?'Bước '+j.plan+' · '+preset.title:measurementNames[j.kind])+' · '+(['CHỜ','ĐANG THU','THU XONG','ĐÃ DỪNG'][j.phase]||'KHÔNG RÕ')+' · '+j.samples+' mẫu / '+(j.t_ms/1000).toFixed(2)+' s. Flags='+j.flags+'; thiếu='+j.gaps+'; CRC='+j.crc+'; tràn='+j.overflow+'; STM32 mất='+j.stm_drops+'.';
          row.appendChild(text);
          var quality=document.createElement('p');quality.textContent=!j.sealed?'Đang ghi; chưa tải để tránh ảnh hưởng đo.':(j.phase!==2||j.flags||j.gaps||j.crc||j.overflow||j.stm_drops)?'Dữ liệu cần kiểm tra lỗi. Giữ và gửi cả log này; chưa dùng để hiệu chuẩn tự động.':'Đã kết thúc thu, chưa phải xác nhận linh kiện đạt hoặc PID đạt.';row.appendChild(quality);
          if(j.sealed){
            var a=document.createElement('a');a.textContent='TẢI / TẢI LẠI TXT';a.href='/measurement-log?id='+j.id;a.download='twip_test_'+j.id+'.txt';a.style.cssText='display:inline-block;margin:8px;padding:10px;background:#0f766e;color:white';
            if(data.active){a.removeAttribute('href');a.textContent='CHỜ BÀI ĐANG CHẠY XONG ĐỂ TẢI';}row.appendChild(a);
            var button=document.createElement('button');button.textContent='ĐÃ LƯU FILE — XÓA KHỎI ESP32';button.onclick=function(){measurementDelete(j.id);};row.appendChild(button);
            if(!data.active&&j.samples&&measureEl('auto').checked&&!measurementAutoAttempted[j.id]){
              var attempted=false;try{attempted=!!sessionStorage.getItem('measure-download-'+j.id);}catch(e){}
              measurementAutoAttempted[j.id]=true;
              if(!attempted){try{sessionStorage.setItem('measure-download-'+j.id,'attempted');}catch(e){}measurementDownload(j.id);}
            }
          }
          list.appendChild(row);
        });
      }catch(e){measureEl('status').textContent='Mất kết nối web: '+e.message+'. Motor tự dừng khi mất nhịp xác nhận 1,5 s. Kết nối lại để tải phần log còn giữ.';}
      finally{measurementPolling=false;}
    }
    window.addEventListener('load',function(){initializeMeasurementPlan();refreshMeasurements();setInterval(refreshMeasurements,1000);setInterval(measurementPing,500);});
  </script>

  <script>
    var balanceLastOK=0,balanceBusy=false,balanceData=null,balanceHistory=[];
    function balanceArm() {
      if(!balanceData||Date.now()-balanceLastOK>500||Number(balanceData.row.split(',')[5])!==1||document.hidden)return;
      location.href='/ui/r12';
    }
    async function balanceClear() {
      try {var r=await fetch('/balance-clear',{method:'POST'});if(!r.ok)throw Error(await r.text());balanceHistory=[];}
      catch(e){alert('Chưa xóa được log: '+e.message);}
    }
    function balanceFault(f) {
      var names=[[1,'STOP'],[2,'mất nhịp web >1s'],[4,'góc >25°'],[8,'vượt ngân sách quãng trôi (trần1m)'],
        [16,'tốc độ >0,8m/s'],[32,'lỗi I2C'],[64,'chu kỳ điều khiển bất thường'],
        [128,'hiệu chuẩn gyro không đạt: giữ xe yên, hiệu chuẩn lại'],[256,'cấu hình BMI160 sai'],[512,'giá trị số không hợp lệ'],[2048,'bắt thăng bằng quá3giây']];
      return names.filter(function(n){return f&n[0];}).map(function(n){return n[1];}).join('; ')||'không';
    }
    function balanceDraw() {
      var c=document.getElementById('balance-chart'),ctx=c.getContext('2d');ctx.clearRect(0,0,c.width,c.height);
      ctx.strokeStyle='#475569';ctx.beginPath();ctx.moveTo(0,90);ctx.lineTo(700,90);ctx.stroke();
      [0,1].forEach(function(k){ctx.strokeStyle=k?'#fb923c':'#00f0ff';ctx.beginPath();
        balanceHistory.forEach(function(v,i){var x=i*700/599,y=90-Math.max(-1,Math.min(1,v[k]))*80;
          if(!i)ctx.moveTo(x,y);else ctx.lineTo(x,y);});ctx.stroke();});
    }
    async function refreshBalance() {
      if(balanceBusy)return;balanceBusy=true;
      try {
        var r=await fetch('/balance-status',{cache:'no-store'});if(!r.ok)throw Error('HTTP');var d=await r.json();
        if(!d.seen)return;
        var a=d.row.split(',').map(Number);if(a.length!==30||!a.every(Number.isFinite))throw Error('Khung dữ liệu sai');
        balanceData=d;balanceLastOK=Date.now()-d.age_ms;
        var m=d.meta.split(',');
        var acc=a.slice(14,17).map(function(v,i){return(v/4096-[-.0416836098593,-.105884356548,.004738195334][i])/[.997272837552,1.00758977736,1.00929388014][i];});
        if(d.age_ms<500 && !hardwareLocked) {
          applyTelemetry('$TEL,'+[a[17],a[18],(a[19]+a[20])/2,0,a[7],a[8],a[4],0,a[19],a[20]].join(','));
          document.getElementById('tel-rate').textContent='100 Hz log / 200 Hz điều khiển';
          balanceHistory.push([a[17]/10,a[21]/.1]);if(balanceHistory.length>600)balanceHistory.shift();balanceDraw();
        }
        document.getElementById('balance-live').textContent=
          'STM32: '+(m[1]||'Chờ thông tin cấu hình')+' · XYZ hiệu chuẩn tạm, chưa xác nhận tải thật\n'+
          ('R12 Kx/Kv/Kθ/Kω/blend: ')+m.slice(2,7).join(' / ')+' · PPR: '+(m[7]||'?')+'\n'+
          'Bias gyro XYZ: '+m.slice(12,15).join(' / ')+' °/s · mốc pitch lắp đặt: '+(m[15]||'?')+'°\n'+
          'Trạng thái: '+(a[4]===3?'ĐANG CÂN BẰNG':a[5]?'SẴN SÀNG BẤM BẬT':'ĐÃ DỪNG / CHỜ DỰNG XE')+
          (d.age_ms>=500?' · MẤT DỮ LIỆU MỚI':'')+'\n'+
          'Lý do dừng / kiểm tra: '+balanceFault(a[6])+'\n'+
          'Pitch điều khiển: '+a[17].toFixed(3)+'° · gyro Y đã trừ bias: '+a[18].toFixed(3)+' °/s\n'+
          'Vận tốc L/R: '+a[19].toFixed(4)+' / '+a[20].toFixed(4)+' m/s · quãng đường encoder: '+(a[21]*100).toFixed(2)+' cm\n'+
          (a[27]===7?'Vận tốc đặt / góc đặt: ':'Vận tốc thân / góc thân ước lượng: ')+a[22].toFixed(4)+' m/s / '+a[23].toFixed(4)+'°\n'+
          (a[27]===7?'PWM cơ sở: ':'PWM trung bình: ')+a[24].toFixed(2)+' · PWM L/R: '+a[7]+' / '+a[8]+(a[27]===7?' · tích phân vận tốc: ':' · tích phân hướng: ')+a[25].toFixed(4)+(a[27]===7?' độ\n':' N·m\n')+
          'Encoder thô L/R: '+a[9]+' / '+a[10]+' ticks (R chưa đảo dấu)\n'+
          'Accel thô XYZ (g): '+a.slice(14,17).map(function(v){return(v/4096).toFixed(4);}).join(' / ')+'\n'+
          'Accel đã hiệu chuẩn XYZ (g): '+acc.map(function(v){return v.toFixed(4);}).join(' / ')+'\n'+
          'Gyro thô XYZ (°/s): '+a.slice(11,14).map(function(v){return(v/16.4).toFixed(3);}).join(' / ')+'\n'+
          'Pitch thô atan2(ax,az): '+(Math.atan2(a[14],a[16])*180/Math.PI).toFixed(3)+'° (chưa hiệu chuẩn, dấu khác pitch điều khiển)\n'+
          'Chu kỳ: '+a[2]+' µs · thời gian xử lý: '+a[3]+' µs · mất mẫu STM32: '+a[26]+'\n'+
          'Log: '+d.samples+'/'+d.capacity+' mẫu · CRC: '+d.crc+' · thiếu gói: '+d.gaps+' · bỏ do đầy: '+d.overflow+' · STM32 khởi động lại: '+d.resets+'\n'+
          'Điện áp pin, dòng, mô-men: CHƯA ĐO. Trọng tâm/độ bám/ma sát thực chưa xác nhận.';
      } catch(e) {document.getElementById('balance-live').textContent='Không lấy được dữ liệu cân bằng. Kiểm tra nguồn STM32 và kết nối Wi-Fi.';}
      finally {balanceBusy=false;}
    }
    setInterval(refreshBalance,100);
    setInterval(function(){var d=balanceData,a=d?d.row.split(',').map(Number):[];
      document.getElementById('balance-calib').disabled=!d||Date.now()-balanceLastOK>500||a[4]===3||hardwareLocked;
      document.getElementById('balance-arm').disabled=!d||Date.now()-balanceLastOK>500||!a[5]||a[4]===3||!d.capacity||d.samples>=d.capacity||hardwareLocked;
    },100);
    document.addEventListener('visibilitychange',function(){if(document.hidden)sendRaw('$BACKGROUND*');});
    window.addEventListener('pagehide',function(){sendRaw('$LEAVE*');});
  </script>

</body>
</html>
)rawliteral";

#endif /* __INDEX_HTML_H__ */
