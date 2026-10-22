#pragma once
#include "firmware_build.h"
const char DASHBOARD_HTML[] PROGMEM = R"dash(<!doctype html>
<html lang="vi">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<title>TWIP RACER · Mobile Cockpit R12</title>
<style>
:root {
  color-scheme: dark;
  --bg: #070c14;
  --card: #101929;
  --card-sub: #0a111e;
  --border: #1a273e;
  --border-focus: #00f0ff;
  --ink: #f0f5fc;
  --muted: #6f829c;
  --cyan: #00f0ff;
  --cyan-glow: rgba(0, 240, 255, 0.4);
  --green: #10b981;
  --green-glow: rgba(16, 185, 129, 0.4);
  --amber: #f59e0b;
  --amber-glow: rgba(245, 158, 11, 0.4);
  --red: #ef4444;
  --red-glow: rgba(239, 68, 68, 0.5);
}
* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
  -webkit-tap-highlight-color: transparent;
  -webkit-touch-callout: none;
  -webkit-user-select: none;
  user-select: none;
}
html, body {
  width: 100%;
  height: 100%;
  height: 100dvh;
  overflow: hidden;
  background: var(--bg);
  color: var(--ink);
  font: 14px/1.4 -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
}

/* Khung màn hình Cockpit Mobile 100dvh Zero-Scroll */
.cockpit-container {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  width: 100%;
  height: 100%;
  height: 100dvh;
  max-width: 480px;
  margin: 0 auto;
  padding: 8px 12px calc(8px + env(safe-area-inset-bottom));
  gap: 6px;
}

/* 1. Header Bar */
header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-shrink: 0;
  padding: 2px 0 4px;
}
.brand-group {
  display: flex;
  align-items: baseline;
  gap: 6px;
}
.brand-title {
  font-size: 17px;
  font-weight: 900;
  letter-spacing: 0.5px;
  color: var(--cyan);
}
.header-badges {
  display: flex;
  align-items: center;
  gap: 5px;
}
.btn-detail {
  padding: 5px 9px;
  border-radius: 8px;
  background: #142236;
  color: var(--cyan);
  font-size: 11px;
  white-space: nowrap;
}
.badge {
  font-size: 10px;
  font-weight: 800;
  padding: 3px 8px;
  border-radius: 14px;
  border: 1px solid var(--border);
  background: var(--card-sub);
  color: var(--muted);
  text-transform: uppercase;
  letter-spacing: 0.3px;
}
.badge.ok { border-color: var(--green); color: var(--green); background: rgba(16,185,129,0.12); }
.badge.warn { border-color: var(--amber); color: var(--amber); background: rgba(245,158,11,0.12); }
.badge.bad { border-color: var(--red); color: var(--red); background: rgba(239,68,68,0.15); }

/* 2. Hero & Attitude Horizon */
.hero-card {
  background: var(--card);
  border: 1px solid var(--border);
  border-left: 4px solid var(--green);
  border-radius: 12px;
  padding: 8px 12px;
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  gap: 4px;
  box-shadow: 0 4px 14px rgba(0,0,0,0.3);
  transition: border-color 0.2s, box-shadow 0.2s;
}
.hero-card.stopped { border-left-color: var(--muted); }
.hero-card.running { border-left-color: var(--green); box-shadow: 0 0 20px rgba(16,185,129,0.2); }
.hero-card.bench { border-left-color: var(--cyan); }
.hero-card.fault { border-left-color: var(--red); }

.hero-status-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
#status {
  font-size: 16px;
  font-weight: 850;
  letter-spacing: 0.2px;
  color: var(--ink);
}
#drive-status {
  font-size: 11px;
  font-weight: 700;
  color: var(--muted);
}
.horizon-box {
  background: #060a12;
  border: 1px solid var(--border);
  border-radius: 6px;
  height: 14px;
  position: relative;
  overflow: hidden;
}
.horizon-center {
  position: absolute;
  left: 50%;
  top: 0;
  bottom: 0;
  width: 2px;
  background: #25364f;
  transform: translateX(-50%);
}
.horizon-bar {
  position: absolute;
  top: 2px;
  bottom: 2px;
  width: 14px;
  border-radius: 4px;
  background: var(--cyan);
  left: 50%;
  transform: translateX(-50%);
  box-shadow: 0 0 8px var(--cyan);
  transition: left 0.08s linear;
}
#reason {
  font-size: 11px;
  color: var(--amber);
  font-weight: 700;
  display: none;
}
#message {
  font-size: 11px;
  color: var(--cyan);
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* 3. Telemetry Grid (4 Cột Siêu Gọn) */
.hud-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 6px;
  flex-shrink: 0;
}
.hud-card {
  background: var(--card-sub);
  border: 1px solid var(--border);
  border-radius: 10px;
  padding: 6px 4px;
  text-align: center;
  display: flex;
  flex-direction: column;
  gap: 1px;
}
.hud-card small {
  font-size: 9.5px;
  font-weight: 700;
  color: var(--muted);
  text-transform: uppercase;
  letter-spacing: 0.3px;
}
.hud-card .number {
  font-size: 17px;
  font-weight: 900;
  font-family: ui-monospace, Consolas, monospace;
  font-variant-numeric: tabular-nums;
  color: #fff;
  letter-spacing: -0.3px;
}
.hud-card .unit {
  font-size: 9px;
  color: var(--muted);
}

/* Vùng Nội dung Chính (Flex-grow) */
.main-content-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  min-height: 0;
  gap: 6px;
}
#balance {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  height: 100%;
  gap: 8px;
}
.prep-row {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 6px;
}
.prep-row .check-toggle { min-width: 0; }

/* ========================================================
   HỆ THỐNG BUTTON ĐỒNG NHẤT TOÀN DIỆN (UNIFIED BUTTON SPEC)
   ======================================================== */
button {
  font: inherit;
  font-weight: 800;
  border-radius: 12px;
  border: 1px solid var(--border);
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  user-select: none;
  -webkit-user-select: none;
  touch-action: manipulation;
  transition: transform 0.06s ease, background 0.1s ease, border-color 0.1s ease, box-shadow 0.1s ease;
}
button:active:not(:disabled) {
  transform: scale(0.97);
}
button:disabled {
  opacity: 0.32;
  cursor: not-allowed;
  transform: none !important;
  box-shadow: none !important;
}

/* Checkbox Toggle Gọn Gàng */
.check-toggle {
  display: flex;
  align-items: center;
  gap: 8px;
  background: var(--card-sub);
  border: 1px solid var(--border);
  border-radius: 9px;
  padding: 7px 10px;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}
.check-toggle input[type=checkbox] {
  width: 17px;
  height: 17px;
  accent-color: var(--cyan);
}

.btn-arm {
  height: 48px;
  background: linear-gradient(180deg, #10b981, #059669);
  color: #012217;
  border-color: #34d399;
  font-size: 15px;
  box-shadow: 0 0 16px var(--green-glow);
}
.btn-arm:active:not(:disabled) {
  box-shadow: 0 0 25px rgba(16, 185, 129, 0.8);
}
.btn-arm.pending {
  background: linear-gradient(180deg, #f59e0b, #d97706);
  border-color: #fbbf24;
  color: #160c01;
  box-shadow: 0 0 18px var(--amber-glow);
  animation: pulse-btn 1.2s infinite alternate;
}
.btn-calib {
  height: 48px;
  padding: 0 14px;
  background: #142236;
  border-color: #273b57;
  color: var(--cyan);
  font-size: 13px;
}

/* Thanh chọn tốc độ */
.config-strip {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 6px;
}
.config-strip > span { color: var(--muted); font-size: 12px; font-weight: 750; }
.config-strip .mini-segmented { width: min(100%, 220px); }
.mini-segmented {
  display: grid;
  grid-template-columns: 1fr 1fr;
  background: var(--card-sub);
  border: 1px solid var(--border);
  border-radius: 9px;
  padding: 2px;
  gap: 2px;
}
.mini-segmented button {
  height: 32px;
  font-size: 11px;
  font-weight: 750;
  border-radius: 7px;
  border: 1px solid transparent;
  background: transparent;
  color: var(--muted);
}
.mini-segmented button.selected {
  background: #162942;
  color: var(--cyan);
  border-color: rgba(0, 240, 255, 0.4);
}

/* ========================================================
   CỤM LÁI VÙNG NGÓN CÁI (THUMB REACH DRIVE PAD)
   ======================================================== */
.drive-pad {
  display: flex;
  flex-direction: column;
  gap: 7px;
  margin-top: auto;
}
.btn-drive {
  touch-action: none;
  height: 58px;
  font-size: 17px;
  font-weight: 900;
  letter-spacing: 0.5px;
  border-radius: 13px;
  border: 1px solid #1f324d;
  background: linear-gradient(180deg, #16263d, #0d1727);
  color: #fff;
  box-shadow: 0 4px 12px rgba(0,0,0,0.3);
  transition: all 0.08s ease;
}
.btn-drive.active {
  background: linear-gradient(180deg, #00f0ff, #0099cc) !important;
  color: #02121b !important;
  border-color: #00f0ff !important;
  box-shadow: 0 0 25px rgba(0, 240, 255, 0.8) !important;
  transform: scale(0.97);
}
.btn-stop-move {
  height: 40px;
  font-size: 13px;
  font-weight: 800;
  letter-spacing: 0.3px;
  border-radius: 10px;
  border: 1px solid #78350f;
  background: linear-gradient(180deg, #271d10, #171108);
  color: var(--amber);
}
.btn-stop-move:hover:not(:disabled) {
  border-color: var(--amber);
}
.btn-stop-move:active:not(:disabled) {
  background: #f59e0b;
  color: #000;
  box-shadow: 0 0 16px var(--amber-glow);
}

/* 6. Nút Khẩn Cấp Dưới Cùng (E-STOP) */
.btn-estop {
  height: 50px;
  width: 100%;
  flex-shrink: 0;
  background: linear-gradient(180deg, #dc2626, #991b1b);
  border: 1px solid #ef4444;
  color: #fff;
  font-size: 16px;
  font-weight: 900;
  letter-spacing: 0.8px;
  border-radius: 12px;
  box-shadow: 0 0 20px var(--red-glow);
}
.btn-estop:active {
  box-shadow: 0 0 30px rgba(239, 68, 68, 0.95);
  transform: scale(0.97);
}

/* Tab 2: Test & Log Container (Cuộn Được Khi Cần) */
#bench {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8px;
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  padding-right: 2px;
}
.pulse-grid {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 6px;
}
.btn-pulse {
  height: 44px;
  font-size: 12px;
  background: #132034;
  color: var(--ink);
  border-color: var(--border);
}
.btn-pulse.active {
  background: linear-gradient(180deg, #00f0ff, #0088cc);
  color: #000;
  border-color: var(--cyan);
  box-shadow: 0 0 16px var(--cyan-glow);
}
canvas#chart {
  width: 100%;
  height: 110px;
  background: #060a12;
  border: 1px solid var(--border);
  border-radius: 8px;
  display: block;
}
.log-actions {
  display: flex;
  gap: 6px;
}
.btn-log {
  flex: 1;
  height: 38px;
  font-size: 12px;
  background: #141f32;
  color: var(--muted);
}
.btn-log:hover:not(:disabled) {
  color: var(--ink);
  border-color: #2b4060;
}
details {
  background: var(--card-sub);
  border: 1px solid var(--border);
  border-radius: 8px;
  padding: 8px 10px;
  font-size: 12px;
}
summary {
  cursor: pointer;
  color: var(--cyan);
  font-weight: 750;
  outline: none;
}
table#details {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
  margin-top: 6px;
}
td {
  padding: 4px 2px;
  border-bottom: 1px solid rgba(255,255,255,0.05);
}
td:last-child {
  text-align: right;
  font-family: ui-monospace, monospace;
  font-weight: 700;
  color: #fff;
}

@keyframes pulse-btn {
  from { opacity: 0.85; transform: scale(0.99); }
  to { opacity: 1; transform: scale(1.01); }
}

[hidden] { display: none !important; }
@media (max-height: 680px) {
  .main-content-area { overflow-y: auto; }
  #balance { min-height: 380px; }
  .btn-estop { position: sticky; bottom: 0; z-index: 2; }
}
</style>
</head>
<body>

<div class="cockpit-container">
  <!-- 1. Header Bar -->
  <header>
    <div class="brand-group">
      <span class="brand-title">TWIP RACER</span>
    </div>
    <div class="header-badges">
      <span class="badge" id="link">Đang kết nối</span>
      <span class="badge ok" id="profile-status">R12</span>
      <button class="btn-detail" id="detail-toggle" onclick="selectMode(mode==='balance'?'bench':'balance')">Chi tiết</button>
    </div>
  </header>

  <!-- 2. Hero & Attitude Horizon Leveler -->
  <section class="hero-card stopped" id="hero-card">
    <div class="hero-status-row">
      <span id="status">Đang chờ STM32…</span>
      <span id="drive-status">Sẵn sàng</span>
    </div>
    <div class="horizon-box">
      <div class="horizon-center"></div>
      <div class="horizon-bar" id="horizon-bar"></div>
    </div>
    <div id="reason"></div>
    <div id="message" role="status" aria-live="polite"></div>
  </section>

  <!-- 3. HUD Matrix 4 Cột Cốt Lõi -->
  <div class="hud-grid">
    <div class="hud-card">
      <small>Góc Thân</small>
      <span class="number" id="pitch">—</span>
      <span class="unit">độ (Pitch)</span>
    </div>
    <div class="hud-card">
      <small>Vận Tốc</small>
      <span class="number" id="speed">—</span>
      <span class="unit">m/s</span>
    </div>
    <div class="hud-card">
      <small>Độ Lệch</small>
      <span class="number" id="distance">—</span>
      <span class="unit">cm</span>
    </div>
    <div class="hud-card">
      <small>PWM L/R</small>
      <span class="number" id="pwm">—</span>
      <span class="unit">±2499</span>
    </div>
  </div>

  <!-- 4. Điều khiển chính; đo đạc và log mở bằng Chi tiết -->
  <div class="main-content-area">
    <div id="balance">
      <div class="prep-row">
        <label class="check-toggle">
          <input type="checkbox" id="floor" onchange="updateButtons()">
          <span>Xe trên nền</span>
        </label>
        <button class="btn-calib" id="calib" onclick="operate('calib')">ĐO GYRO</button>
      </div>
      <button class="btn-arm" id="arm" onclick="beginArm()">BẬT CÂN BẰNG</button>
      <div id="arm-reason" style="font-size:11px;color:var(--amber);display:none;text-align:center"></div>

      <div class="config-strip">
        <span>Tốc độ</span>
        <div class="mini-segmented">
          <button id="speed-normal" class="selected" onclick="selectDriveSpeed(0)">40 cm/s</button>
          <button id="speed-fast" onclick="selectDriveSpeed(1)">60 cm/s</button>
        </div>
      </div>

      <!-- Cụm Phím Lái Vùng Ngón Cái (Thumb Reach Drive Pad) -->
      <div class="drive-pad">
        <button class="btn-drive" id="drive-forward">▲ GIỮ ĐỂ TIẾN</button>
        <button class="btn-stop-move" id="drive-hold" onclick="releaseDrive()">● DỪNG DI CHUYỂN</button>
        <button class="btn-drive" id="drive-reverse">▼ GIỮ ĐỂ LÙI</button>
      </div>
    </div>

    <!-- PANEL 2: TEST BÀN & LOG (Cuộn mượt mà khi xem chi tiết) -->
    <div id="bench" hidden>
      <label class="check-toggle">
        <input type="checkbox" id="fixture" onchange="updateButtons()">
        <span>Xe đã kê chắc</span>
      </label>

      <div class="pulse-grid">
        <button class="btn-pulse" id="left" onclick="operate('pulse',1)">Bánh Trái</button>
        <button class="btn-pulse" id="both" onclick="operate('pulse',3)">Cả Hai Bánh</button>
        <button class="btn-pulse" id="right" onclick="operate('pulse',2)">Bánh Phải</button>
      </div>
      <div id="pulse-result" style="font-size:11px;color:var(--cyan);font-family:ui-monospace,monospace;min-height:16px"></div>

      <!-- Biểu đồ Sóng Realtime -->
      <canvas id="chart" width="900" height="110"></canvas>
      <div style="display:flex;justify-content:space-between;font-size:10px;color:var(--muted)">
        <span style="color:var(--cyan)">— Pitch</span>
        <span style="color:var(--amber)">— Sai số</span>
        <span id="chart-scale">Tự động</span>
      </div>

      <!-- Bộ Ghi Dữ Liệu -->
      <div style="display:flex;flex-direction:column;gap:4px">
        <div style="font-size:11px;color:var(--muted);font-family:ui-monospace,monospace" id="log-status">Chưa nhận log.</div>
        <div class="log-actions">
          <button class="btn-log" id="download" onclick="location.href='/balance-log'">Tải Log TXT</button>
          <button class="btn-log" id="clear" onclick="clearLog()">Xóa Log</button>
        </div>
      </div>

      <!-- Chi tiết cảm biến -->
      <details>
        <summary>Bảng thanh ghi IMU & Vòng điều khiển</summary>
        <div style="display:grid;grid-template-columns:1fr 1fr;gap:4px;margin-top:4px">
          <div><small style="color:var(--muted)">Lệch bánh:</small> <span id="wheel-difference" style="font-weight:700">—</span> cm</div>
          <div><small style="color:var(--muted)">Bù hướng:</small> <span id="heading-assist" style="font-weight:700">—</span> PWM</div>
          <div><small id="integral-label" style="color:var(--muted)">Tích phân:</small> <span id="integral" style="font-weight:700">—</span> <small id="integral-unit">N·m</small></div>
          <div><small id="theta-label" style="color:var(--muted)">Góc đặt:</small> <span id="theta-target" style="font-weight:700">—</span>°</div>
        </div>
        <table id="details"></table>
      </details>
    </div>
  </div>

  <!-- 6. Phím An Toàn Sát Đáy (E-STOP) -->
  <button class="btn-estop" onclick="operate('stop')">■ DỪNG MOTOR</button>
</div>

<script>
'use strict';
var data=null,row=[],mode='balance',ws=null,wsOK=false,busy=false,polling=false,lastFresh=0,lastSeq=-1,lastDetailAt=0,plotHistory=[],pending=null,pulse=null,endedAt=0,previousState=-1,session=-1,sessionToken=0,armWaitUntil=0;
var driveHeld=0,driveSequence=0,driveEpoch=0,drivePreset=0,lastDriveSend=0;

function el(id){return document.getElementById(id);}

function haptic(ms){
  if(window.navigator&&window.navigator.vibrate){
    try{window.navigator.vibrate(ms||25);}catch(e){}
  }
}

function selectDriveSpeed(preset){
  if(preset!==0&&preset!==1)return;
  haptic(25);
  releaseDrive();
  drivePreset=preset;
  driveTick(true);
  updateButtons();
}

function ownsControl(){
  return data&&data.owner===session&&data.owner_token===sessionToken&&sessionToken>0;
}

function isRunning(){
  return row[4]===3||row[4]===7;
}

function fresh(){
  return data&&data.seen&&data.age_ms<600&&Date.now()-lastFresh<600;
}

function activeController(){
  return row[27]||12;
}

function armAngleLimit(){
  var m=data?data.meta.split(','):[],a=Number(m[19]);
  return (activeController()===12&&a===60)?60:0;
}

function matched(){
  return data&&data.meta.indexOf('$BINFO,BAL-20261018-R12-SPEED2,')===0&&activeController()===12&&Number(data.meta.split(',')[17])===12;
}

function canDrive(){
  return fresh()&&matched()&&wsOK&&ownsControl()&&!document.hidden&&!busy&&!pending&&!armWaitUntil&&mode==='balance'&&el('floor').checked&&row[4]===3&&row[28]===0&&row[6]===0&&(row[33]&1)&&data.drive_epoch>0&&row[32]===data.drive_epoch&&blockers(true).length===0;
}

function driveTick(force){
  if(driveHeld&&!canDrive()){
    driveHeld=0;
    el('drive-forward').classList.remove('active');
    el('drive-reverse').classList.remove('active');
  }
  if(!wsOK||!ws||ws.readyState!==WebSocket.OPEN||!fresh()||!matched()||!ownsControl()||row[4]!==3||document.hidden||row[32]!==data.drive_epoch||!data.drive_epoch)return;
  if(driveEpoch!==data.drive_epoch){driveEpoch=data.drive_epoch;driveSequence=0;driveHeld=0;}
  if(driveSequence>=1000000000){driveHeld=0;return;}
  var now=performance.now();
  if(!force&&now-lastDriveSend<(driveHeld?70:200))return;
  // Four queued MOVE packets can outlive the STM32 350ms motion lease.
  // Request STOP independently and close the session to prevent stale replay.
  if(ws.bufferedAmount>128){
    driveHeld=0;
    el('drive-forward').classList.remove('active');
    el('drive-reverse').classList.remove('active');
    el('message').textContent='Kênh điều khiển chậm. Đã ngắt phiên lái.';
    fetch('/operate',{method:'POST',body:new URLSearchParams({action:'stop'}),keepalive:true}).catch(function(){});
    ws.close();return;
  }
  ws.send('$MOVE,'+driveHeld+','+driveEpoch+','+(++driveSequence)+','+drivePreset+'*');
  lastDriveSend=now;
}

function releaseDrive(){
  if(driveHeld!==0){
    haptic(15);
    driveHeld=0;
    driveTick(true);
  }
  el('drive-forward').classList.remove('active');
  el('drive-reverse').classList.remove('active');
}

function pressDrive(direction,event){
  if(event){
    if(event.button!==undefined&&event.button!==0)return;
    if(event.isPrimary===false)return;
    event.preventDefault();
  }
  if(!canDrive())return;
  haptic(35);

  if(driveHeld&&driveHeld!==direction){releaseDrive();return;}
  if(driveEpoch!==data.drive_epoch){driveEpoch=data.drive_epoch;driveSequence=0;}
  driveHeld=direction;

  if(event&&Number.isInteger(event.pointerId)&&event.currentTarget&&event.currentTarget.setPointerCapture){
    try{event.currentTarget.setPointerCapture(event.pointerId);}catch(e){}
  }

  if(direction===1){
    el('drive-forward').classList.add('active');
    el('drive-reverse').classList.remove('active');
  } else if(direction===-1){
    el('drive-reverse').classList.add('active');
    el('drive-forward').classList.remove('active');
  }
  driveTick(true);
}

// Drive Button Listeners
['drive-forward','drive-reverse'].forEach(function(id,index){
  var b=el(id),direction=index===0?1:-1;
  b.addEventListener('pointerdown',function(e){pressDrive(direction,e);});
  b.addEventListener('pointerup',releaseDrive);
  b.addEventListener('pointercancel',releaseDrive);
  b.addEventListener('lostpointercapture',releaseDrive);
  b.addEventListener('contextmenu',function(e){e.preventDefault();});
});

document.addEventListener('pointerup',releaseDrive);
document.addEventListener('pointercancel',releaseDrive);
window.addEventListener('blur',releaseDrive);

// Keyboard Listeners
window.addEventListener('keydown',function(e){
  if(e.repeat)return;
  if(e.key==='ArrowUp'||e.key==='w'||e.key==='W'){pressDrive(1,e);}
  else if(e.key==='ArrowDown'||e.key==='s'||e.key==='S'){pressDrive(-1,e);}
  else if(e.key===' '||e.key==='Escape'){e.preventDefault();releaseDrive();}
});
window.addEventListener('keyup',function(e){
  if(['ArrowUp','ArrowDown','w','W','s','S'].includes(e.key)){
    e.preventDefault();releaseDrive();
  }
});

// Lease Heartbeat 75ms
setInterval(driveTick,75);

function selectMode(next){
  haptic(20);
  releaseDrive();
  if(next!=='balance')cancelArmWait('Đã đổi chế độ.');
  mode=next;
  el('bench').hidden=next!=='bench';
  el('balance').hidden=next!=='balance';
  el('detail-toggle').textContent=next==='bench'?'← Điều khiển':'Chi tiết';
  if(next==='bench')draw();
  updateButtons();
}

function blockers(forMotion){
  if(!fresh())return ['Chờ tín hiệu STM32'];
  if(!matched())return ['Chờ khớp bản R12'];
  if(!wsOK)return ['Mất kênh WebSocket'];
  if(!data.capacity)return ['Lỗi bộ nhớ ESP32'];
  if(!forMotion&&data.samples>=data.capacity)return ['Bộ nhớ log đầy'];
  var f=row[6],out=[];
  if(row[4]===6||f&32)out.push('Lỗi BMI160');
  if(f&256)out.push('Lỗi cấu hình BMI160');
  return out;
}

function armBlockers(){
  var out=blockers();if(out.length)return out;
  if(row[6]&128)out.push('Cần đo lại Gyro');
  var limit=armAngleLimit();
  if(Math.abs(row[17])>limit)out.push('Góc '+row[17].toFixed(1)+'° > ±'+limit+'°');
  var m=data.meta.split(','),rates=row.slice(11,14).map(function(v,i){return v/16.4-Number(m[12+i]);});
  if(rates.some(function(v){return Math.abs(v)>=6;}))out.push('Rung (Gyro ≥ 6°/s)');
  if(Math.abs(row[19])>=.05||Math.abs(row[20])>=.05)out.push('Bánh chưa dừng');
  var norm=row.slice(14,17).reduce(function(s,v){return s+(v/4096)*(v/4096);},0);
  if(norm<=.72||norm>=1.32)out.push('Gia tốc bất thường');
  if(row[2]<4000||row[2]>6000)out.push('Loop '+row[2]+' µs');
  if(!out.length&&!row[5])out.push('Giữ yên 0,2s');
  return out;
}

function armSafetyBlockers(){
  var out=blockers();
  if(row[6]&128)out.push('Cần đo lại Gyro');
  if(row[6]&512)out.push('Dữ liệu không hợp lệ');
  return out;
}

function cancelArmWait(message){
  if(armWaitUntil){
    armWaitUntil=0;
    el('message').textContent=message;
    el('arm').classList.remove('pending');
  }
}

function beginArm(){
  haptic(40);
  if(busy||pending||armWaitUntil||isRunning()||document.hidden||mode!=='balance'||!el('floor').checked||armSafetyBlockers().length)return;
  armWaitUntil=Date.now()+5000;
  el('message').textContent='Đã nhận lệnh ARM. Giữ yên thân xe…';
  el('arm').classList.add('pending');
  serviceArmWait();
  updateButtons();
}

function serviceArmWait(){
  if(!armWaitUntil)return;
  if(document.hidden||!el('floor').checked||armSafetyBlockers().length||isRunning()||mode!=='balance'){
    cancelArmWait('Đã hủy chờ ARM.');
    return;
  }
  if(Date.now()>=armWaitUntil){
    cancelArmWait('Hết 5s chờ. Thử lại.');
    return;
  }
  if(row[5]){
    armWaitUntil=0;
    el('arm').classList.remove('pending');
    operate('arm');
  }
}

function wheelAssist(a,m){
  if(a[4]!==3)return null;
  return {delta:(-a[10]-a[9])*0.204203522/Number(m[7]),pwm:(a[7]-a[8])/2};
}

function faultText(f){
  return [
    [1,'Dừng lệnh'],[2,'Mất nhịp điều khiển'],[4,'Vượt góc'],[8,'Lệch điểm cân bằng'],
    [16,'Vượt tốc độ'],[32,'Lỗi BMI160'],[64,'Chu kỳ bất thường'],[128,'Cần hiệu chuẩn Gyro'],
    [256,'Sai cấu hình BMI160'],[512,'Số liệu lỗi'],[1024,'Rung khi thử bánh'],
    [2048,'Timeout bắt thăng bằng'],[4096,'Timeout phục hồi'],[8192,'Va chạm: Đã dừng']
  ].filter(function(x){return f&x[0];}).map(function(x){return x[1];}).join(' · ');
}

function updateButtons(){
  el('speed-normal').className=drivePreset===0?'selected':'';
  el('speed-fast').className=drivePreset===1?'selected':'';

  var blocked=blockers().length||busy||pending||armWaitUntil||isRunning()||document.hidden;
  el('profile-status').textContent=matched()?'R12 SPEED2':'CHỜ KẾT NỐI';
  el('profile-status').className='badge '+(matched()?'ok':'warn');
  
  if(armWaitUntil){
    el('arm').textContent='CHỜ YÊN ('+Math.max(0,Math.ceil((armWaitUntil-Date.now())/1000))+'s)…';
  } else if(row[4]===3){
    el('arm').textContent='ĐANG CÂN BẰNG R'+(activeController()||'12');
  } else {
    el('arm').textContent='BẬT CÂN BẰNG R'+(activeController()||'12');
  }

  ['left','right','both'].forEach(function(id){el(id).disabled=!!blocked||!el('fixture').checked;});
  el('arm').disabled=!!blocked||armSafetyBlockers().length>0||!el('floor').checked;
  el('calib').disabled=!fresh()||busy||!!pending||!!armWaitUntil||isRunning();
  el('download').disabled=!fresh()||isRunning()||!!armWaitUntil||!!pending||!data.samples||Date.now()-endedAt<2200;
  el('clear').disabled=!fresh()||isRunning()||!!armWaitUntil||!!pending||busy||Date.now()-endedAt<2200;

  ['drive-forward','drive-reverse'].forEach(function(id){el(id).disabled=!canDrive();});
  el('drive-hold').disabled=!fresh()||row[4]!==3||!ownsControl();

  if(row[4]!==3){
    el('drive-status').textContent='Chờ bật';
  } else if(!ownsControl()){
    el('drive-status').textContent='Chỉ xem';
  } else if(row[28]){
    el('drive-status').textContent='Đang phục hồi…';
  } else if(canDrive()){
    el('drive-status').textContent='Sẵn sàng';
  } else {
    el('drive-status').textContent='Cân bằng ổn định';
  }

  var armReasonEl=el('arm-reason');
  if(isRunning()){
    armReasonEl.style.display='none';
  } else if(armWaitUntil){
    armReasonEl.style.display='block';
    armReasonEl.textContent='CHỜ YÊN: '+armBlockers().join('; ');
  } else if(!el('floor').checked){
    armReasonEl.style.display='none';
  } else {
    var bText=armBlockers().join('; ');
    if(bText){
      armReasonEl.style.display='block';
      armReasonEl.textContent=bText;
    } else {
      armReasonEl.style.display='none';
    }
  }
}

async function operate(action,wheel){
  haptic(30);
  if(!['stop','arm','pulse','calib'].includes(action))return;
  if(action!=='stop'&&(busy||pending||armWaitUntil))return;
  if(action==='stop'){
    releaseDrive();
    pending=null;
    cancelArmWait('Đã gửi DỪNG.');
    ['left','both','right'].forEach(function(id){el(id).classList.remove('active');});
  }
  if(action==='stop'&&wsOK&&ws.readyState===WebSocket.OPEN){
    if(ws.bufferedAmount>128)ws.close();
    else ws.send('$STOP*');
  }
  
  busy=true;
  el('message').textContent='Đang gửi lệnh…';
  if(action==='pulse'){
    var activeId=wheel===1?'left':wheel===2?'right':'both';
    el(activeId).classList.add('active');
  }
  updateButtons();

  try{
    var body=new URLSearchParams({
      action:action,
      controller:'12',
      wheel:String(wheel||0),
      direction:'1',
      fixture:el('fixture').checked?'yes':'no',
      surface:el('floor').checked?'floor':'no',
      session:String(session),
      token:String(sessionToken),
      stop_revision:String(data?data.control_revision:-1)
    });
    var r=await fetch('/operate',{method:'POST',body:body,signal:AbortSignal.timeout(3000)}),answer=await r.json();
    if(!r.ok)throw Error(answer.message);
    el('message').textContent=answer.message;
    if(action==='arm'||action==='pulse'){
      pending={state:action==='arm'?3:7,until:Date.now()+2000};
      if(action==='pulse')pulse={wheel:wheel,startL:row[9],startR:row[10],peak:0,observed:false};
    }
  }catch(e){
    el('message').textContent='Lỗi: '+e.message;
    ['left','both','right'].forEach(function(id){el(id).classList.remove('active');});
  }finally{
    busy=false;
    updateButtons();
  }
}

async function clearLog(){
  haptic(35);
  if(!confirm('Xác nhận xóa log ESP32?'))return;
  try{
    var r=await fetch('/balance-clear',{method:'POST'});
    if(!r.ok)throw Error(await r.text());
    plotHistory=[];
    el('message').textContent='Đã xóa log sạch.';
  }catch(e){
    el('message').textContent=e.message;
  }
}

function connect(){
  var wsPort = (location.port && location.port !== '80') ? '8081' : '81';
  ws=new WebSocket('ws://'+location.hostname+':'+wsPort+'/');
  ws.onopen=function(){wsOK=false;};
  ws.onmessage=function(e){
    try{
      var h=JSON.parse(e.data);
      if(Number.isInteger(h.session)&&Number.isInteger(h.token)&&h.token>0){
        session=h.session;sessionToken=h.token;wsOK=true;
        if(!document.hidden)ws.send('$PING*');
      }
    }catch(ignore){}
  };
  ws.onclose=function(){
    driveHeld=0;
    cancelArmWait('Mất kênh WebSocket.');
    wsOK=false;session=-1;sessionToken=0;
    setTimeout(connect,1200);
  };
  ws.onerror=function(){};
}

setInterval(function(){
  if(wsOK&&!document.hidden)ws.send('$PING*');
  updateButtons();
},200);

document.addEventListener('visibilitychange',function(){
  if(document.hidden){
    releaseDrive();
    cancelArmWait('Trang bị ẩn.');
    if(wsOK)ws.send('$BACKGROUND*');
  } else {
    poll();
  }
});
window.addEventListener('pagehide',function(){
  releaseDrive();
  cancelArmWait('Rời trang.');
  if(wsOK)ws.send('$LEAVE*');
});

function detailRows(items){
  var table=el('details');table.replaceChildren();
  items.forEach(function(item){
    var tr=document.createElement('tr');
    item.forEach(function(value){
      var td=document.createElement('td');td.textContent=value;tr.appendChild(td);
    });
    table.appendChild(tr);
  });
}

function draw(){
  var c=el('chart'),g=c.getContext('2d');
  g.clearRect(0,0,c.width,c.height);
  g.strokeStyle='#17253d';
  g.lineWidth=1;
  g.beginPath();
  g.moveTo(0,55);g.lineTo(c.width,55);
  g.moveTo(0,18);g.lineTo(c.width,18);
  g.moveTo(0,92);g.lineTo(c.width,92);
  g.stroke();

  [0,1].forEach(function(k){
    g.strokeStyle=k?'#f59e0b':'#00f0ff';
    g.lineWidth=1.5;
    g.beginPath();
    plotHistory.forEach(function(v,i){
      var x=i*c.width/299,y=55-45*Math.max(-1,Math.min(1,v[k]));
      if(i)g.lineTo(x,y);else g.moveTo(x,y);
    });
    g.stroke();
  });
}

async function poll(){
  if(polling||document.hidden)return;
  polling=true;
  try{
    var r=await fetch('/balance-status',{cache:'no-store',signal:AbortSignal.timeout(1200)});
    if(!r.ok)throw Error('HTTP');
    var d=await r.json();
    data=d;

    var heroCard=el('hero-card');
    if(!d.seen||d.age_ms>=600){
      cancelArmWait('Mất dữ liệu STM32.');
      el('status').textContent='CHƯA KẾT NỐI STM32';
      el('link').textContent=wsOK?'WS OK · CHỜ STM32':'MẤT KẾT NỐI';
      el('link').className='badge bad';
      heroCard.className='hero-card stopped';
      el('reason').style.display='block';
      el('reason').textContent='Không có dữ liệu mới từ STM32.';
      updateButtons();
      return;
    }

    var a=d.row.split(',').map(Number);
    if(a.length!==35||!a.every(Number.isFinite)||a[27]!==12)throw Error('Dữ liệu không hợp lệ');
    row=a;
    lastFresh=Date.now()-d.age_ms;

    el('link').textContent=wsOK?'KẾT NỐI OK':'MẤT WS';
    el('link').className='badge '+(wsOK?'ok':'warn');

    if(a[4]===3){
      heroCard.className='hero-card running';
      el('status').textContent=a[28]===2?'ĐANG PHỤC HỒI SAU ĐẨY':(a[28]===1?'ĐANG BẮT THĂNG BẰNG':'ĐANG CÂN BẰNG R12');
    } else if(a[4]===7){
      heroCard.className='hero-card bench';
      el('status').textContent='ĐANG THỬ BÁNH (1.5S)';
    } else {
      heroCard.className='hero-card stopped';
      el('status').textContent='MOTOR ĐANG DỪNG';
    }

    var faultMsg=faultText(a[6]);
    var reasonEl=el('reason');
    if(faultMsg&&!isRunning()){
      reasonEl.style.display='block';
      reasonEl.textContent='Cảnh báo: '+faultMsg;
    } else {
      reasonEl.style.display='none';
    }

    if(pending&&a[4]===pending.state){el('message').textContent='STM32 đã nhận lệnh.';pending=null;}
    if(pending&&Date.now()>pending.until){el('message').textContent='Không phản hồi từ STM32.';pending=null;}

    if(pulse&&a[4]===7){
      pulse.observed=true;
      pulse.peak=Math.max(pulse.peak,Math.abs(a[7]),Math.abs(a[8]));
      el('pulse-result').textContent='PWM '+a[7]+'/'+a[8]+' · ΔTicks: '+Math.abs(a[9]-pulse.startL)+'/'+Math.abs(a[10]-pulse.startR);
    }
    if(previousState===7&&a[4]!==7&&pulse){
      var dl=Math.abs(a[9]-pulse.startL),dr=Math.abs(a[10]-pulse.startR);
      el('pulse-result').textContent='Hoàn tất thử: PWM max '+pulse.peak+' | ΔL: '+dl+', ΔR: '+dr;
      ['left','both','right'].forEach(function(id){el(id).classList.remove('active');});
      pulse=null;
    }
    if((previousState===3||previousState===7)&&!isRunning()){
      endedAt=Date.now();
      el('message').textContent='Motor đã dừng.';
      ['left','both','right'].forEach(function(id){el(id).classList.remove('active');});
    }
    previousState=a[4];

    // Cập nhật số HUD
    el('pitch').textContent=a[17].toFixed(2);
    el('speed').textContent=((a[19]+a[20])/2).toFixed(2);
    el('distance').textContent=((a[21]-a[30])*100).toFixed(1);
    el('pwm').textContent=a[7]+'/'+a[8];

    // Thước đo chân trời
    var normPitch=Math.max(-1,Math.min(1,a[17]/25.0));
    el('horizon-bar').style.left=(50+normPitch*44)+'%';

    // Lưu mẫu cho trang chi tiết; chỉ vẽ khi trang đó đang mở.
    if(lastSeq!==a[0]){
      plotHistory.push([a[17]/Math.max(10,armAngleLimit()),(a[21]-a[30])/Math.max(.1,a[29])]);
      if(plotHistory.length>300)plotHistory.shift();
      lastSeq=a[0];
      if(mode==='bench')draw();
    }
    el('chart-scale').textContent='±'+Math.max(10,armAngleLimit())+'° · ±'+(100*a[29]).toFixed(0)+' cm';

    var m=d.meta.split(','),assist=wheelAssist(a,m);
    el('wheel-difference').textContent=assist?(100*assist.delta).toFixed(2):'0.00';
    el('heading-assist').textContent=assist?assist.pwm.toFixed(0):'0';
    el('integral').textContent=a[25].toFixed(3);
    el('theta-target').textContent=a[23].toFixed(2);

    // Bảng chi tiết không chiếm thời gian render của màn hình lái.
    if(mode==='bench'&&Date.now()-lastDetailAt>=500){
      lastDetailAt=Date.now();
      detailRows([
      ['Build STM32',m[1]||'BAL-20261018-R12-SPEED2'],
      ['Vận tốc Gyro Y',a[18].toFixed(3)+' °/s'],
      ['Vận tốc L/R',a[19].toFixed(3)+' / '+a[20].toFixed(3)+' m/s'],
      ['Encoder ticks L/R',a[9]+' / '+a[10]],
      ['Gia tốc XYZ',a.slice(14,17).map(function(v){return(v/4096).toFixed(2);}).join('/')+' g'],
      ['Gyro XYZ',a.slice(11,14).map(function(v){return(v/16.4).toFixed(1);}).join('/')+' °/s'],
      ['Quãng đường tổng',(a[21]*100).toFixed(1)+' cm'],
      ['V_ref / X_ref',a[31].toFixed(2)+' m/s / '+(a[30]*100).toFixed(0)+' cm'],
      ['Chu kỳ loop',a[2]+' µs ('+a[3]+' µs exec)'],
      ['Mất mẫu STM32',a[26]]
      ]);
    }

    serviceArmWait();
    el('log-status').textContent=d.samples+' / '+d.capacity+' mẫu ('+(d.samples/100).toFixed(1)+'s) · Lỗi CRC: '+d.crc;
    updateButtons();
  }catch(e){
    driveHeld=0;
    cancelArmWait('Lỗi kết nối.');
    el('link').textContent='LỖI DỮ LIỆU';
    el('link').className='badge bad';
    updateButtons();
  }finally{
    polling=false;
  }
}

connect();
poll();
setInterval(poll,100);
</script>
</body>
</html>
)dash";
