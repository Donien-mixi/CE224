#pragma once
#include "firmware_build.h"
/* UI-only redesign (2026-10-24): markup and CSS replaced. Command, gating,
 * telemetry and logging logic below are unchanged from RT5/NET2. */
const char DASHBOARD_HTML[] PROGMEM=R"dash(
<!doctype html><html lang="vi"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="dark"><meta name="theme-color" content="#070c13">
<title>TWIP</title><style>
:root{color-scheme:dark;--bg:#070c13;--panel:#0e1622;--panel2:#0a121c;--line:#1d2a39;--line2:#2a3b4f;--ink:#e8f0f9;--muted:#8ea3ba;--mint:#5eead4;--amber:#fbbf24;--rose:#fb7185;--blue:#7dd3fc;--r:16px}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%;scroll-padding-top:64px;-webkit-user-select:none;user-select:none;-webkit-touch-callout:none}
body{margin:0;background:radial-gradient(1000px 520px at 50% -14%,#0d1a27 0%,var(--bg) 60%) no-repeat,var(--bg);color:var(--ink);font:16px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;padding-bottom:calc(78px + env(safe-area-inset-bottom));overflow-x:hidden;-webkit-user-select:none;user-select:none;-webkit-touch-callout:none;-webkit-tap-highlight-color:transparent}
body *{-webkit-user-select:none;user-select:none;-webkit-touch-callout:none;-webkit-user-drag:none}
input,textarea,select{-webkit-user-select:text;user-select:text}
::selection{background:transparent;color:inherit}
.skip{position:absolute;left:-9999px;top:0;background:var(--mint);color:#062018;padding:10px 14px;border-radius:0 0 10px 0;font-weight:700;z-index:50}
.skip:focus{left:0}
.top{position:sticky;top:0;z-index:30;display:flex;align-items:center;justify-content:space-between;gap:10px;padding:8px 14px;background:rgba(7,12,19,.86);backdrop-filter:blur(10px);border-bottom:1px solid var(--line)}
.brand{display:flex;align-items:center;gap:8px}
.brand .dot{width:8px;height:8px;border-radius:50%;background:var(--mint);box-shadow:0 0 10px var(--mint)}
h1{font-size:14px;margin:0;letter-spacing:.18em;font-weight:800}
.tag{font-size:11px;font-weight:700;color:var(--muted);border:1px solid var(--line2);border-radius:99px;padding:4px 9px;white-space:nowrap;max-width:58vw;overflow:hidden;text-overflow:ellipsis}
main{display:grid;gap:10px;max-width:1080px;margin:0 auto;padding:10px 12px 6px;grid-template-columns:minmax(0,1fr);grid-template-areas:"status" "gauges" "control" "tools";min-width:0}
.card{background:linear-gradient(180deg,rgba(255,255,255,.022),transparent 40%),var(--panel);border:1px solid var(--line);border-radius:var(--r);padding:13px;min-width:0}
.state{font-size:clamp(20px,5.6vw,25px);font-weight:800;line-height:1.15;margin:0;letter-spacing:-.01em;text-wrap:balance}
#status[data-state="3"]{color:var(--mint)}
#status[data-state="7"]{color:var(--amber)}
#status[data-state="6"]{color:var(--rose)}
.reason{margin:5px 0 0;color:var(--muted);font-size:12.5px;line-height:1.45}
.reason:empty{display:none}
#status[data-state="3"]+#reason,#status[data-state="7"]+#reason{display:none}
main:has(#status:not([data-state="3"])) #drive-status{display:none}
.msg{margin:6px 0 0;color:var(--mint);font-size:12.5px;white-space:pre-line}
.msg:empty{display:none}
.gauges{grid-area:gauges}
.tiles{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
.tile{background:var(--panel2);border:1px solid #1a2735;border-radius:12px;padding:9px 11px;display:flex;flex-direction:column;gap:1px;min-width:0}
.tile .k{font-size:10px;font-weight:700;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.tile .v{font-size:clamp(19px,5vw,25px);font-weight:800;font-variant-numeric:tabular-nums;line-height:1.2;letter-spacing:-.01em;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.tile .u{font-size:10px;color:var(--muted)}
.bar{position:relative;height:3px;background:#12202e;border-radius:99px;margin-top:6px}
.bar .mid{position:absolute;left:50%;top:-2px;width:1px;height:7px;background:#2a3d52}
.bar .marker{position:absolute;left:50%;top:-3px;width:3px;height:9px;border-radius:2px;background:var(--mint);transform:translateX(-50%);transition:left .12s linear}
canvas{width:100%;max-width:100%;height:auto;display:block;margin-top:9px;border-radius:10px;background:#091018;border:1px solid #141f2b}
.control{grid-area:control}
.row{display:flex;align-items:center;gap:8px}
.between{justify-content:space-between}
.seg{display:flex;gap:3px;background:#0a121b;border:1px solid var(--line);border-radius:10px;padding:2px}
.seg button{flex:1;border:0;background:transparent;color:var(--muted);font:inherit;font-size:12px;font-weight:700;padding:6px 10px;border-radius:8px;cursor:pointer;touch-action:manipulation;white-space:nowrap}
.seg button.selected{background:#17303a;color:var(--mint);box-shadow:inset 0 0 0 1px #24515c}
.seg.speeds button{min-width:46px}
.check{display:flex;align-items:center;gap:8px;margin:10px 0 8px;font-size:13px;cursor:pointer}
.check input{appearance:none;-webkit-appearance:none;width:20px;height:20px;flex:0 0 auto;margin:0;border:1.5px solid var(--line2);border-radius:6px;background:#0a121b;display:grid;place-content:center;cursor:pointer}
.check input:checked{border-color:var(--mint);background:#0f2f2b}
.check input:checked::after{content:"";width:9px;height:5px;border-left:2px solid var(--mint);border-bottom:2px solid var(--mint);transform:rotate(-45deg) translateY(-1px)}
.primary{width:100%;border:0;border-radius:12px;padding:13px;font:inherit;font-weight:800;font-size:15px;color:#062018;background:linear-gradient(180deg,#78f2dc,#3ecbb4);cursor:pointer;touch-action:manipulation}
.primary:hover:not(:disabled){background:linear-gradient(180deg,#8ff7e4,#4ad6bf)}
.primary:disabled{background:#1b2937;color:#647689;cursor:default}
.ghost{width:100%;border:1px solid var(--line2);border-radius:11px;padding:10px;font:inherit;font-weight:700;font-size:13px;color:var(--ink);background:#111c29;cursor:pointer;touch-action:manipulation}
.ghost:hover:not(:disabled){background:#16222f;border-color:#33465c}
.ghost:disabled{opacity:.45;cursor:default}
.hint{margin:8px 0 0;font-size:12px;line-height:1.45;color:var(--muted);white-space:pre-line}
#arm:not(:disabled)+.hint{display:none}
main:has(#status[data-state="3"]) #balance{display:none}
.stick{--size:min(78vw,300px);--travel:calc(var(--size)*.26);position:relative;width:var(--size);height:var(--size);margin:12px auto 4px;border-radius:50%;overflow:hidden;touch-action:none;user-select:none;-webkit-user-select:none;-webkit-touch-callout:none;background:radial-gradient(circle at 50% 42%,#1a2a3c 0%,#0f1926 58%,#0a1119 100%);border:1px solid #24344a;box-shadow:inset 0 0 40px rgba(0,0,0,.55)}
.stick::before{content:"";position:absolute;inset:24%;border-radius:50%;border:1px dashed #1f2c3a;pointer-events:none}
.sector{position:absolute;width:30%;height:30%;transform:translate(-50%,-50%);appearance:none;border:0;border-radius:50%;background:transparent;color:#6f8398;font-size:19px;line-height:1;display:grid;place-items:center;pointer-events:none}
.s-n{left:50%;top:8%}.s-ne{left:79.7%;top:20.3%}.s-e{left:92%;top:50%}.s-se{left:79.7%;top:79.7%}
.s-s{left:50%;top:92%}.s-sw{left:20.3%;top:79.7%}.s-w{left:8%;top:50%}.s-nw{left:20.3%;top:20.3%}
.sector.held{color:var(--mint);text-shadow:0 0 12px rgba(94,234,212,.75)}
.knob{position:absolute;left:50%;top:50%;width:26%;height:26%;border-radius:50%;pointer-events:none;background:radial-gradient(circle at 36% 30%,#3b5269,#18242f 72%);border:1px solid #3d5c78;box-shadow:0 8px 18px rgba(0,0,0,.5),inset 0 1px 0 rgba(255,255,255,.1);transform:translate(calc(-50% + var(--tx,0)*var(--travel)),calc(-50% + var(--ty,0)*var(--travel)));transition:transform .12s cubic-bezier(.2,.7,.3,1)}
.knob.live{transition:none}
.knob::after{content:"";position:absolute;left:50%;top:50%;width:26%;height:26%;border-radius:50%;transform:translate(-50%,-50%);background:rgba(94,234,212,.32)}
.hold{margin-top:8px;border:1px solid #3a4a5e;border-radius:11px;padding:11px;width:100%;font:inherit;font-weight:750;font-size:13px;color:var(--ink);background:#16222f;cursor:pointer;touch-action:manipulation}
.hold:hover:not(:disabled){background:#1c2b3a}
.hold:disabled{opacity:.45;cursor:default}
.tools{grid-area:tools;background:var(--panel);border:1px solid var(--line);border-radius:var(--r)}
.tools>summary{cursor:pointer;padding:12px 13px;font-size:12.5px;font-weight:700;color:var(--muted);list-style:none;display:flex;align-items:center;justify-content:space-between}
.tools>summary::-webkit-details-marker{display:none}
.tools>summary::after{content:"＋";font-weight:400}
.tools[open]>summary::after{content:"－"}
.body{padding:0 13px 13px;border-top:1px solid var(--line)}
.mini{display:grid;grid-template-columns:minmax(0,1fr);gap:0;margin:6px 0 0}
.mini>div{display:flex;align-items:baseline;gap:7px;padding:5px 0;border-bottom:1px dashed #16212e;min-width:0;overflow:hidden}
.mini .k{font-size:11.5px;color:var(--muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;flex:1 1 auto;min-width:0}
.mini .v{font-size:13px;font-weight:800;font-variant-numeric:tabular-nums;margin-left:auto;flex:0 0 auto}
.mini .u{font-size:10px;color:var(--muted);flex:0 0 auto}
.tiny{margin:7px 0 0;font-size:11px;line-height:1.55;color:var(--muted);white-space:pre-line;font-variant-numeric:tabular-nums}
.build{margin:10px 0 0;font-size:10px;color:#5d7085;font-family:ui-monospace,Consolas,monospace;overflow-wrap:anywhere}
.sub{margin-top:8px;border-top:1px solid var(--line);padding-top:6px}
.sub>summary{cursor:pointer;font-size:11.5px;font-weight:700;color:var(--muted);list-style:none;padding:4px 0}
.sub>summary::-webkit-details-marker{display:none}
.sub>summary::after{content:" ＋"}
.sub[open]>summary::after{content:" －"}
table{width:100%;border-collapse:collapse;font-size:12px;margin-top:6px}
td{padding:6px 4px;border-bottom:1px solid #16212e;color:var(--muted);vertical-align:top}
td:last-child{text-align:right;color:var(--ink);font-variant-numeric:tabular-nums;white-space:nowrap}
.link{display:block;color:var(--blue);font-size:12.5px;font-weight:650;text-decoration:none;padding:8px 0;border-bottom:1px dashed #16212e}
.link:last-of-type{border-bottom:0}
.link:hover{color:#bae6fd}
#archive-list p{display:flex;flex-wrap:wrap;align-items:center;gap:8px;margin:6px 0;font-size:11.5px}
#archive-list a{color:var(--blue);font-weight:650}
#archive-list button{font:inherit;font-size:10.5px;font-weight:700;padding:5px 8px;border-radius:7px;border:1px solid var(--line2);background:#111c29;color:var(--ink);cursor:pointer}
footer.bar{position:fixed;left:0;right:0;bottom:0;z-index:25;padding:8px 12px calc(8px + env(safe-area-inset-bottom));background:linear-gradient(180deg,rgba(7,12,19,.5),rgba(7,12,19,.97) 45%);border-top:1px solid var(--line);backdrop-filter:blur(10px)}
.stop{display:block;width:100%;max-width:520px;margin:0 auto;border:1px solid #fb7185;border-radius:12px;padding:12px;font:inherit;font-size:15px;font-weight:800;letter-spacing:.02em;color:#fff;background:linear-gradient(180deg,#e11d48,#9f1239);cursor:pointer;touch-action:manipulation}
.stop:active{transform:translateY(1px)}
:focus-visible{outline:2px solid var(--mint);outline-offset:3px}
button:focus:not(:focus-visible){outline:none}
@media(min-width:560px){.tiles{grid-template-columns:repeat(4,minmax(0,1fr))}.mini{grid-template-columns:repeat(2,minmax(0,1fr));gap:0 16px}}
@media(min-width:920px){
 main{grid-template-columns:minmax(0,1fr) 420px;grid-template-areas:"status control" "gauges control" "tools control";align-items:start;padding-top:14px}
 .control{position:sticky;top:60px}
 .tiles{grid-template-columns:repeat(2,minmax(0,1fr))}
 .tile .v{font-size:25px}
 .mini{grid-template-columns:repeat(2,minmax(0,1fr))}
}
@media(prefers-reduced-motion:reduce){*{transition:none!important;animation:none!important}}
</style></head><body>
<a class="skip" href="#control">Tới điều khiển</a>
<header class="top"><div class="brand"><span class="dot" aria-hidden="true"></span><h1 translate="no">TWIP</h1></div><span class="tag" id="link">Đang kết nối…</span></header>
<main>
<section class="card status" aria-live="polite">
 <p class="state" id="status">Đang chờ STM32</p>
 <p class="reason" id="reason">Giữ xe yên sau khi cấp nguồn.</p>
 <p class="msg" id="message" role="status" aria-live="polite"></p>
</section>
<section class="card gauges" aria-label="Thông số xe">
 <div class="tiles">
  <div class="tile"><span class="k">Góc</span><span class="v" id="pitch">—</span><span class="u">độ</span>
   <div class="bar" aria-hidden="true"><span class="mid"></span><span class="marker" id="tilt-marker"></span></div></div>
  <div class="tile"><span class="k">Tốc độ</span><span class="v" id="speed">—</span><span class="u">m/s</span></div>
  <div class="tile"><span class="k">Lệch</span><span class="v" id="distance">—</span><span class="u">cm</span></div>
  <div class="tile"><span class="k">PWM</span><span class="v" id="pwm">—</span><span class="u">L / R · ±2499</span></div>
 </div>
 <canvas id="chart" width="900" height="170" aria-label="Đồ thị góc và độ lệch 60 giây"></canvas>
</section>
<section class="card control" id="control" tabindex="-1" aria-label="R12 · Cấu hình chính thức">
 <div class="row between">
  <div class="seg" style="min-width:132px">
   <button type="button" id="tab-balance" class="selected" onclick="selectMode('balance')">Nền</button>
   <button type="button" id="tab-bench" onclick="selectMode('bench')">Kê cao</button>
  </div>
  <div class="row" style="gap:5px">
   <span class="u" style="font-size:10px;color:var(--muted)">cm/s</span>
   <div class="seg speeds">
    <button type="button" id="speed-normal" class="selected" onclick="selectDriveSpeed(0)">50</button>
    <button type="button" id="speed-fast" onclick="selectDriveSpeed(1)">72</button>
   </div>
  </div>
 </div>
 <div id="balance">
  <label class="check"><input id="floor" type="checkbox" onchange="updateButtons()"><span>Trên nền</span></label>
  <button type="button" class="primary" id="arm" onclick="beginArm()">BẬT CÂN BẰNG</button>
  <p class="hint" id="arm-reason">Chờ dữ liệu…</p>
 </div>
 <div id="bench" hidden>
  <label class="check"><input id="fixture" type="checkbox" onchange="updateButtons()"><span>Đã kê xe</span></label>
  <div class="row" style="gap:6px">
   <button type="button" class="ghost" id="left" style="flex:1" onclick="operate('pulse',1)">Trái</button>
   <button type="button" class="ghost" id="right" style="flex:1" onclick="operate('pulse',2)">Phải</button>
   <button type="button" class="ghost" id="both" style="flex:1" onclick="operate('pulse',3)">Cả hai</button>
  </div>
  <p class="hint" id="pulse-result"></p>
 </div>
 <div class="stick" id="stick">
  <button type="button" class="sector s-nw" id="drive-forward-left" data-drive="1" data-turn="1" aria-label="Tiến trái">↖</button>
  <button type="button" class="sector s-n" id="drive-forward" data-drive="1" data-turn="0" aria-label="Tiến">↑</button>
  <button type="button" class="sector s-ne" id="drive-forward-right" data-drive="1" data-turn="-1" aria-label="Tiến phải">↗</button>
  <button type="button" class="sector s-w" id="drive-left" data-drive="0" data-turn="1" aria-label="Quay trái tại chỗ">↺</button>
  <button type="button" class="sector s-e" id="drive-right" data-drive="0" data-turn="-1" aria-label="Quay phải tại chỗ">↻</button>
  <button type="button" class="sector s-sw" id="drive-reverse-left" data-drive="-1" data-turn="-1" aria-label="Lùi trái">↙</button>
  <button type="button" class="sector s-s" id="drive-reverse" data-drive="-1" data-turn="0" aria-label="Lùi">↓</button>
  <button type="button" class="sector s-se" id="drive-reverse-right" data-drive="-1" data-turn="1" aria-label="Lùi phải">↘</button>
  <div class="knob" id="stick-knob" aria-hidden="true"></div>
 </div>
 <button type="button" class="hold" id="drive-hold" onclick="releaseDrive()">DỪNG CHẠY</button>
 <p class="hint" id="drive-status">Chờ dữ liệu…</p>
</section>
<details class="tools">
 <summary>Công cụ và chi tiết</summary>
 <div class="body">
  <div class="mini">
   <div><span class="k">Chênh bánh</span><span class="v" id="wheel-difference">—</span><span class="u">cm</span></div>
   <div><span class="k">Bù lái</span><span class="v" id="heading-assist">—</span><span class="u">PWM</span></div>
   <div><span class="k" id="integral-label">Tích phân</span><span class="v" id="integral">—</span><span class="u" id="integral-unit">N·m</span></div>
   <div><span class="k" id="theta-label">Góc ước lượng</span><span class="v" id="theta-target">—</span><span class="u">độ</span></div>
  </div>
  <p class="tiny" id="drive-timing"></p>
  <p class="tiny" id="chart-scale"></p>
  <p class="tiny" id="profile-status"></p>
  <p class="tiny" id="speed-ack"></p>
  <details class="sub"><summary>Chi tiết IMU, encoder, điều khiển</summary><table id="details"></table></details>
  <p class="tiny" id="log-status">Chưa nhận log.</p>
  <div class="row" style="gap:6px;margin-top:6px">
   <button type="button" class="ghost" id="download" style="flex:1" onclick="location.href='/balance-log'">TẢI LOG</button>
   <button type="button" class="ghost" id="clear" style="flex:1" onclick="clearLog()">XÓA LOG</button>
  </div>
  <p class="tiny" id="network-health"></p>
  <button type="button" class="ghost" id="download-network" style="margin-top:6px" onclick="if(!isRunning())location.href='/network-log'">CHẨN ĐOÁN KẾT NỐI</button>
  <div id="archive-list"></div>
  <button type="button" class="ghost" id="calib" style="margin-top:6px" onclick="operate('calib')">ĐO LẠI GYRO</button>
  <a class="link" href="/wheel-tests">Đo hai motor · 12 bài</a>
  <a class="link" href="/calibration">Hiệu chuẩn nâng cao</a>
  <a class="link" href="/firmware-info">Phiên bản</a>
  <p class="build">)dash" TWIP_WEB_BUILD R"dash(</p>
 </div>
</details>
</main>
<footer class="bar"><button type="button" class="stop" id="stop" onclick="operate('stop')">■ DỪNG MOTOR</button></footer>
</body></html>
<script>

'use strict';
var data=null,row=[],mode='balance',ws=null,wsOK=false,busy=false,polling=false,lastFresh=0,lastSeq=-1,plotHistory=[],pending=null,pulse=null,endedAt=0,previousState=-1,session=-1,sessionToken=0,armWaitUntil=0;
var driveTurn=0;
var driveHeld=0,drivePointer=null,driveSequence=0,driveEpoch=0,drivePreset=0;
var driveSent=new Map(),driveAckMs=null,driveAckSequence=0,driveLastNeutral=true;
var networkPolling=false,networkPollCount=0,networkBoot=null,detailUpdated=0,detailKey='';
var padId=null,padRect=null,padActive=false,padSector=-1,padKx=0,padKy=0;
var PAD_ENABLE=16,PAD_RELEASE=9,PAD_HYST=27;
var PAD_DIRS=[[0,-1],[-1,1],[-1,0],[-1,-1],[0,1],[1,1],[1,0],[1,-1]];
function syncStick(){
 var k=el('stick-knob');
 if(!k||!k.style||!k.style.setProperty)return;
 if(k.classList&&k.classList.toggle)k.classList.toggle('live',padActive);
 if(padActive){k.style.setProperty('--tx',String(padKx));k.style.setProperty('--ty',String(padKy));}
 else{k.style.setProperty('--tx',String(driveTurn?-driveTurn*.7:0));k.style.setProperty('--ty',String(driveHeld?-driveHeld*.7:0));}
}
function padClamp(v){return v>1?1:(v<-1?-1:v);}
function padApply(dx,dy){
 var r=Math.sqrt(dx*dx+dy*dy);
 if(r>padRect.width*.62){padEnd();return;}
 var travel=padRect.width*.26;
 padKx=padClamp(dx/travel);padKy=padClamp(dy/travel);
 var next=padSector;
 if(r<PAD_RELEASE)next=-1;
 else if(r>=PAD_ENABLE||padSector>=0){
  var ang=Math.atan2(dy,dx)*180/Math.PI;if(ang<0)ang+=360;
  var cand=Math.round(ang/45)%8;
  if(padSector>=0&&cand!==padSector){
   var diff=Math.abs(ang-padSector*45);if(diff>180)diff=360-diff;
   if(diff<PAD_HYST)cand=padSector;
  }
  next=cand;
 }
 padSector=next;
 var d=padSector<0?0:PAD_DIRS[padSector][0],t=padSector<0?0:PAD_DIRS[padSector][1];
 if(d!==driveHeld||t!==driveTurn){driveHeld=d;driveTurn=t;driveTick();updateButtons();}
 else syncStick();
}
function padEnd(e){
 if(padId===null)return;
 if(e&&e.pointerId!==undefined&&e.pointerId!==padId)return;
 padId=null;padActive=false;padSector=-1;padKx=padKy=0;
 releaseDrive();
}
(function(){
 var pad=el('stick');
 if(!pad||!pad.addEventListener)return;
 pad.addEventListener('pointerdown',function(e){
  if(e.button!==undefined&&e.button!==0)return;
  if(padId!==null)return;
  if(!canDrive()){if(el('message'))el('message').textContent='Chưa sẵn sàng điều khiển.';return;}
  if(drivePointer!==null)releaseDrive();
  padRect=pad.getBoundingClientRect();
  padId=e.pointerId;padActive=true;padSector=-1;padKx=padKy=0;drivePointer=e.pointerId;
  try{if(e.currentTarget&&e.currentTarget.setPointerCapture)e.currentTarget.setPointerCapture(e.pointerId);}catch(err){}
  if(e.preventDefault)e.preventDefault();
  padApply(e.clientX-(padRect.left+padRect.width/2),e.clientY-(padRect.top+padRect.height/2));
 },{passive:false});
 pad.addEventListener('pointermove',function(e){
  if(e.pointerId!==padId)return;
  if(e.pointerType==='mouse'&&e.buttons===0){padEnd(e);return;}
  padApply(e.clientX-(padRect.left+padRect.width/2),e.clientY-(padRect.top+padRect.height/2));
 },{passive:true});
 ['pointerup','pointercancel','lostpointercapture'].forEach(function(type){
  pad.addEventListener(type,function(e){padEnd(e);},{passive:true});
 });
 window.addEventListener('blur',function(){padEnd();});
 document.addEventListener('visibilitychange',function(){if(document.hidden)padEnd();});
})();
document.addEventListener('selectstart',function(e){e.preventDefault();});
document.addEventListener('dragstart',function(e){e.preventDefault();});
document.addEventListener('contextmenu',function(e){e.preventDefault();});
function selectDriveSpeed(preset){if(preset!==0&&preset!==1)return;releaseDrive();drivePreset=preset;driveTick();updateButtons();}
function el(id){return document.getElementById(id);}
function ownsControl(){return data&&data.owner===session&&data.owner_token===sessionToken&&sessionToken>0;}
function canDrive(){return fresh()&&matched()&&wsOK&&ownsControl()&&!document.hidden&&!busy&&!pending&&!armWaitUntil&&mode==='balance'&&el('floor').checked&&row[4]===3&&row[28]!==1&&row[6]===0&&(row[33]&1)&&data.drive_epoch>0&&row[32]===data.drive_epoch&&blockers(true).length===0;}
function driveTick(forceNeutral){
 if((driveHeld||driveTurn)&&!canDrive()){driveHeld=0;driveTurn=0;drivePointer=null;forceNeutral=true;}
 var neutral=driveHeld===0&&driveTurn===0;
 if(!wsOK||!ownsControl()||!data.drive_epoch)return;
 // A real release must not wait for the next status poll or freshness check.
 if(!(forceNeutral===true&&neutral)&&(!fresh()||!matched()||row[4]!==3||document.hidden||row[32]!==data.drive_epoch))return;
 if(driveEpoch!==data.drive_epoch){driveEpoch=data.drive_epoch;driveSequence=Math.max(0,Math.floor(Number(row[38])||0));driveHeld=0;driveTurn=0;drivePointer=null;drivePreset=0;driveSent.clear();driveAckMs=null;driveAckSequence=0;}
 neutral=driveHeld===0&&driveTurn===0;
 if(driveSequence>=1000000000){driveHeld=0;driveTurn=0;return;}
 // Do not enqueue repeated held commands behind a slow WebSocket transport.
 // Append one neutral after the last held packet; lease expiry remains on STM32.
 if(ws.bufferedAmount>0&&(!neutral||driveLastNeutral))return;
 ++driveSequence;driveSent.set(driveSequence,Date.now());
 if(driveSent.size>100)driveSent.delete(driveSent.keys().next().value);
 ws.send('$MOVE8,'+driveHeld+','+driveTurn+','+driveEpoch+','+driveSequence+','+drivePreset+'*');
 driveLastNeutral=neutral;
}
function releaseDrive(){driveHeld=0;driveTurn=0;drivePointer=null;driveTick(true);updateButtons();}
var driveButtons=Array.from(document.querySelectorAll('[data-drive]'));
function pressDrive(direction,turn,event){
 if(event){if(event.button!==undefined&&event.button!==0)return;if(event.isPrimary===false)return;event.preventDefault();}
 if(!canDrive()){
  el('message').textContent=row[4]===3&&row[28]===2?'Xe đang lấy lại thăng bằng; giữ nút để tiếp tục khi ổn định.':'Chưa nhận lệnh chạy. Chờ xe sẵn sàng và giữ trang điều khiển mở rồi nhấn lại.';
  return;
 }
 // A new primary physical down is a NEW gesture, including after an OS lost
 // pointerup. Never let a stale pointer silently block the next press.
 if(drivePointer!==null){
  if(!event||!Number.isInteger(event.pointerId))return;
  releaseDrive();
 }
 if(driveEpoch!==data.drive_epoch){driveEpoch=data.drive_epoch;driveSequence=Math.max(0,Math.floor(Number(row[38])||0));driveSent.clear();driveAckMs=null;driveAckSequence=0;}
 driveHeld=direction;driveTurn=turn;drivePointer=event&&event.pointerId!==undefined?event.pointerId:'keyboard';
 if(event&&Number.isInteger(event.pointerId)&&event.currentTarget&&event.currentTarget.setPointerCapture)event.currentTarget.setPointerCapture(event.pointerId);
 driveTick();updateButtons();
}
driveButtons.forEach(function(b){var d=Number(b.dataset.drive),t=Number(b.dataset.turn);
 b.style.touchAction='none';b.style.userSelect='none';b.draggable=false;
 b.onpointerdown=function(e){pressDrive(d,t,e);};
 b.onpointermove=function(e){
  if(drivePointer!==e.pointerId||!canDrive())return;
  if(e.pointerType==='mouse'&&e.buttons===0){releaseDrive();return;}
  var hit=document.elementFromPoint(e.clientX,e.clientY),next=hit&&hit.closest('[data-drive]');
  if(!next){releaseDrive();return;}
  var nd=Number(next.dataset.drive),nt=Number(next.dataset.turn);
  if(nd!==driveHeld||nt!==driveTurn){driveHeld=nd;driveTurn=nt;driveTick();updateButtons();}
 };
 ['pointerup','pointercancel','lostpointercapture'].forEach(function(type){b.addEventListener(type,function(e){if(drivePointer===e.pointerId)releaseDrive();});});
 b.oncontextmenu=function(e){e.preventDefault();};
 b.onkeydown=function(e){if((e.key===' '||e.key==='Enter')&&!e.repeat)pressDrive(d,t,e);};
 b.onkeyup=function(e){if(e.key===' '||e.key==='Enter'){e.preventDefault();releaseDrive();}};
});
function releasePointer(e){if(!e||drivePointer===e.pointerId)releaseDrive();}
document.addEventListener('pointerup',releasePointer,true);document.addEventListener('pointercancel',releasePointer,true);window.addEventListener('blur',releaseDrive);
// iOS may deliver touch cancellation instead of the expected pointer release.
// Only all-fingers-up releases the owner; another finger cannot take control.
['touchend','touchcancel'].forEach(function(type){document.addEventListener(type,function(e){if(!e.touches||e.touches.length===0)releaseDrive();},true);});
document.addEventListener('keyup',function(e){if(drivePointer==='keyboard'&&(e.key===' '||e.key==='Enter'))releaseDrive();},true);
setInterval(driveTick,40);
function stopSource(){return ({controller_background:'Trang điều khiển bị ẩn (chuyển ứng dụng hoặc khóa màn hình)',controller_page_left:'Rời trang điều khiển',controller_disconnected:'Trang điều khiển mất kết nối',stop_button_ws:'Nút DỪNG trên web',stop_button_http:'Nút DỪNG trên web'})[data&&data.stop_source]||'Lệnh STOP; chưa biết nguồn';}
function isRunning(){return row[4]===3||row[4]===7;}
function fresh(){return data&&data.seen&&data.age_ms<500&&Date.now()-lastFresh<500;}
function activeController(){return row[27];}
function armAngleLimit(){var m=data?data.meta.split(','):[],a=Number(m[19]);return (activeController()===12&&a===60)?60:0;}
function matched(){return data&&data.meta.indexOf('$BINFO,BAL-20261024-R12-8DIR-RT8,')===0&&activeController()===12&&Number(data.meta.split(',')[17])===12;}
function selectMode(next){releaseDrive();if(next!=='balance')cancelArmWait('Đã hủy chờ vì đổi chế độ.');mode=next;el('bench').hidden=next!=='bench';el('balance').hidden=next!=='balance';el('tab-bench').className=next==='bench'?'selected':'';el('tab-balance').className=next==='balance'?'selected':'';updateButtons();}
function blockers(forMotion){
 if(!fresh())return ['Chưa nhận dữ liệu mới từ STM32. Kiểm tra nguồn STM32 và dây UART; Wi-Fi kết nối chưa đủ.'];
 if(!matched())return ['Chờ thông tin phiên bản STM32 BAL-20261024-R12-8DIR-RT8. Nạp cả STM32 và ESP32 bản R12 chính thức; chờ cấu hình xác nhận.'];
 if(!wsOK)return ['Mất kênh lệnh từ trình duyệt. Kết nối lại Wi-Fi rồi mở lại trang.'];
 if(!data.capacity)return ['ESP32 chưa cấp được bộ nhớ log. Kiểm tra cấu hình OPI PSRAM khi nạp ESP32.'];
 if(!forMotion&&data.samples>=data.capacity)return ['Log đã đầy. Dừng, tải log rồi xóa log cũ trước lần chạy mới.'];
 var f=row[6],out=[];
 if(row[4]===6||f&32)out.push('Lỗi đọc BMI160. Kiểm tra dây/nguồn cảm biến, sau đó đo lại gyro hoặc khởi động lại STM32.');
 if(f&256)out.push('Cấu hình BMI160 không đúng; chưa được phép bật motor.');
 return out;
}
function armBlockers(){
 var out=blockers();if(out.length)return out;
 if(row[6]&128)out.push('Hiệu chuẩn gyro chưa đạt. Cố định xe rồi bấm ĐO LẠI GYRO.');
 var limit=armAngleLimit();if(Math.abs(row[17])>limit)out.push('Góc hiện tại '+row[17].toFixed(2)+'°; R'+activeController()+' cho phép bật trong ±'+limit+'°. Dựng bớt xe rồi giữ yên.');
 var m=data.meta.split(','),rates=row.slice(11,14).map(function(v,i){return v/16.4-Number(m[12+i]);});
 if(rates.some(function(v){return Math.abs(v)>=6;}))out.push('Thân xe còn chuyển động: gyro cần dưới 6°/s mỗi trục.');
 if(Math.abs(row[19])>=.05||Math.abs(row[20])>=.05)out.push('Bánh còn quay. Giữ xe yên và chờ bánh dừng.');
 var norm=row.slice(14,17).reduce(function(s,v){return s+(v/4096)*(v/4096);},0);
 if(norm<=.72||norm>=1.32)out.push('Gia tốc bất thường hoặc giá kê đang rung. Cố định xe trước khi bật.');
 if(row[2]<4000||row[2]>6000)out.push('Chu kỳ điều khiển bất thường: '+row[2]+' µs.');
 if(!out.length&&!row[5])out.push('Giữ thân xe yên trong ±60° liên tục 0,2 giây để STM32 cho phép bật.');
 return out;
}
function armSafetyBlockers(){var out=blockers();if(row[6]&128)out.push('Hiệu chuẩn gyro chưa đạt. Cố định xe rồi đo lại gyro.');if(row[6]&512)out.push('Dữ liệu điều khiển không hợp lệ.');return out;}
function cancelArmWait(message){if(armWaitUntil){armWaitUntil=0;el('message').textContent=message;}}
function beginArm(){
 if(busy||pending||armWaitUntil||isRunning()||document.hidden||mode!=='balance'||!el('floor').checked||armSafetyBlockers().length)return;
 armWaitUntil=Date.now()+5000;
 el('message').textContent='Đã nhận yêu cầu. Giữ xe yên trong ±60°; motor chưa chạy. Web chờ tối đa 5 giây rồi tự gửi lệnh khi STM32 cho phép. Bấm DỪNG để hủy.';
 serviceArmWait();updateButtons();
}
function serviceArmWait(){
 if(!armWaitUntil)return;
 if(document.hidden||!el('floor').checked||armSafetyBlockers().length||isRunning()||mode!=='balance'){
  cancelArmWait('Đã hủy chờ bật do điều kiện/kết nối thay đổi. Motor không được tự bật lại.');return;
 }
 if(Date.now()>=armWaitUntil){cancelArmWait('Hết 5 giây chờ; chưa bật motor. Giữ xe yên trong ±60° rồi bấm lại.');return;}
 if(row[5]){armWaitUntil=0;operate('arm');}
}
function wheelAssist(a,m){
 if(a[4]!==3)return null;
 return {delta:(-a[10]-a[9])*0.204203522/Number(m[7]),pwm:(a[7]-a[8])/2};
}
function faultText(f){return [[1,stopSource()],[2,'Mất nhịp điều khiển web quá 1 giây'],[4,'Góc thân vượt giới hạn pha đang chạy'],[8,'Sai lệch vượt ngân sách hoặc trôi quá1m từ lúc hủy chuyển động'],[16,'Vượt tốc độ của pha đang chạy (bắt: 1,0/1,1/1,2 theo góc bật≤48/≤58/>58°; thường: 0,8 m/s)'],[32,'Lỗi đọc BMI160'],[64,'Chu kỳ điều khiển bất thường'],[128,'Hiệu chuẩn gyro chưa đạt'],[256,'Sai cấu hình BMI160'],[512,'Giá trị số không hợp lệ'],[1024,'Thân xe/giá kê chuyển động quá mức khi thử bánh'],[2048,'Hết thời gian bắt thăng bằng: 3 giây; góc bật trên58°:4 giây'],[4096,'Phục hồi/hủy chuyển động quá4 giây; chưa lấy lại cân bằng ổn'],[8192,'Nghi va chạm mạnh: đã DỪNG MOTOR. Kiểm tra và dựng xe, bấm BẬT lại; xe không tự chạy tiếp.']].filter(function(x){return f&x[0];}).map(function(x){return x[1];}).join(' · ');}
function updateButtons(){
 el('download-network').disabled=isRunning();
 el('drive-timing').textContent=fresh()&&row.length===40?'Nút trên web: '+((driveHeld||driveTurn)?'ĐANG GIỮ':'ĐÃ NHẢ')+' · STM32: '+((row[34]||row[37])?'đang nhận hướng chạy':'đã nhận trung tính')+'\nLệnh STM32 #'+row[38]+' · tuổi lệnh '+(row[38]?((row[1]-row[39])>>>0)+' ms':'—')+'\nXác nhận qua web: '+(driveAckMs===null?'chưa đo':driveAckMs+' ms')+' (gồm truyền và chu kỳ hiển thị, không phải độ trễ một chiều).\nBánh trái / phải: '+(100*(row[19]||0)).toFixed(1)+' / '+(100*(row[20]||0)).toFixed(1)+' cm/s · trôi tâm: '+(50*((row[19]||0)+(row[20]||0))).toFixed(1)+' cm/s\nQuay đặt / thực: '+((row[36]||0)*180/Math.PI).toFixed(1)+' / '+(((row[20]||0)-(row[19]||0))/.262*180/Math.PI).toFixed(1)+' °/s':'Chờ dữ liệu mới từ STM32…';
 el('speed-normal').className=drivePreset===0?'selected':'';el('speed-fast').className=drivePreset===1?'selected':'';
 el('speed-ack').textContent='Đã chọn '+(drivePreset?72:50)+' cm/s. STM32 '+(row[4]===3?'đang nhận mức '+((row[33]&256)?72:50)+' cm/s.':'sẽ xác nhận sau khi bật cân bằng.')+' Đổi mức sẽ nhả lệnh cũ, cần giữ nút lại.';
 var blocked=blockers().length||busy||pending||armWaitUntil||isRunning()||document.hidden;
 el('profile-status').textContent=matched()?'STM32 ĐÃ XÁC NHẬN R12 · CF 0,998 · khởi động ±60°':'Chờ đúng firmware STM32 R12 chính thức.';
 el('arm').textContent='BẬT CÂN BẰNG R'+(activeController()||'…');
 ['left','right','both'].forEach(function(id){el(id).disabled=!!blocked||!el('fixture').checked;});
 el('arm').disabled=!!blocked||armSafetyBlockers().length>0||!el('floor').checked;
 el('calib').disabled=!fresh()||busy||!!pending||!!armWaitUntil||isRunning();
 el('download').disabled=!fresh()||isRunning()||!!armWaitUntil||!!pending||!data.samples||Date.now()-endedAt<2200;
 el('clear').disabled=!fresh()||isRunning()||!!armWaitUntil||!!pending||busy||Date.now()-endedAt<2200;
 driveButtons.forEach(function(b){b.disabled=!canDrive();b.classList.toggle('held',!!(driveHeld||driveTurn)&&Number(b.dataset.drive)===driveHeld&&Number(b.dataset.turn)===driveTurn);});
 el('drive-hold').disabled=!fresh()||row[4]!==3||!ownsControl();
 el('drive-status').textContent=row[4]!==3?'Bật cân bằng trên nền trước khi điều khiển.':!ownsControl()?'Trang này chỉ xem; điều khiển trên trang đã bấm BẬT.':row[28]===1?'Chờ xe hoàn tất dựng thăng bằng.':row[28]===2?'Đang lấy lại thăng bằng; giữ nút để chạy tiếp.':(row[33]&64)?((row[33]&1)?'Đã bỏ điểm đặt cũ và cân bằng ổn. Giữ nút mới để chạy tiếp.':'Đã hủy chuyển động cũ. Nhả nút, chờ xe cân bằng; không tự chạy lại.'):(row[33]&512)?'Đang giảm tốc quay theo khả năng bám; giữ nút để tiếp tục quẹo.':(row[33]&8)?'Xe đang theo chưa kịp điểm đặt; tạm giảm tốc để giữ thăng bằng.':(row[33]&4)?'ĐÃ NHẢ NÚT · đang hãm và lấy lại tư thế cân bằng tại điểm mới.':Math.abs(row[31]||0)>.0001?'Đang '+(row[31]>0?'TIẾN':'LÙI')+' · vận tốc đặt '+(100*row[31]).toFixed(1)+' cm/s':canDrive()?'Sẵn sàng. Giữ nút để thực hiện liên tục; nhả để hãm rồi giữ cân bằng tại điểm mới.':'Chờ xe cân bằng ổn định; nhả nút rồi bấm lại.';
 if(row[4]===3&&ownsControl()&&row[28]===0&&!(row[33]&64)&&(Math.abs(row[36]||0)>.0001))el('drive-status').textContent='Đang quay '+(row[36]>0?'trái':'phải')+' · '+row[36].toFixed(2)+' rad/s · vận tốc đặt '+(100*row[31]).toFixed(1)+' cm/s';
 // Show the finger's release immediately, rather than presenting the previous
 //100ms telemetry row as if the user were still holding a run command.
 if(row[4]===3&&ownsControl()&&!driveHeld&&!driveTurn&&
    (row[34]||row[37]||Math.abs(row[31]||0)>.0001||Math.abs(row[36]||0)>.0001||(row[33]&4)))
  el('drive-status').textContent='ĐÃ NHẢ NÚT · đã yêu cầu dừng chạy; xe đang hãm để giữ thăng bằng. Không giữ hoặc tự phát lại lệnh cũ.';
 el('arm-reason').textContent=isRunning()?'Motor đang chạy. Bấm DỪNG trước khi đổi bài.':(armWaitUntil?'ĐANG CHỜ DỰNG XE · còn '+Math.max(0,Math.ceil((armWaitUntil-Date.now())/1000))+' giây\n'+armBlockers().join('\n'):(armSafetyBlockers().join('\n')||(!el('floor').checked?'Đánh dấu xác nhận xe đã đặt trên nền.':'Có thể bấm BẬT trước, rồi giữ xe yên trong ±60°.\n'+armBlockers().join('\n'))));
 syncStick();
}
async function operate(action,wheel){
 if(!['stop','arm','pulse','calib'].includes(action))return;
 if(action!=='stop'&&(busy||pending||armWaitUntil))return;
 if(action==='stop'){releaseDrive();pending=null;cancelArmWait('Đã hủy chờ bật.');}
 if(action==='stop'&&wsOK)ws.send('$STOP*');
 var requestedController=12;
 busy=true;el('message').textContent='Đang gửi lệnh…';updateButtons();
 try{
  var body=new URLSearchParams({action:action,controller:String(requestedController||0),wheel:String(wheel||0),direction:'1',fixture:el('fixture').checked?'yes':'no',surface:el('floor').checked?'floor':'no',session:String(session),token:String(sessionToken),stop_revision:String(data?data.control_revision:-1)});
  var r=await fetch('/operate',{method:'POST',body:body,signal:AbortSignal.timeout(3000)}),answer=await r.json();
  if(!r.ok)throw Error(answer.message);
  el('message').textContent=answer.message;
  if(action==='arm'||action==='pulse'){
   pending={state:action==='arm'?3:7,until:Date.now()+2000};
   if(action==='pulse')pulse={wheel:wheel,startL:row[9],startR:row[10],peak:0,observed:false};
  }
  if(action==='calib')pending={calibration:true,startedMs:row[1],until:Date.now()+9000};
 }catch(e){el('message').textContent='Chưa thực hiện được: '+e.message;}
 finally{busy=false;updateButtons();}
}
async function clearLog(){if(!confirm('Chỉ xóa khi bạn đã lưu file TXT. Xóa log cũ?'))return;try{var r=await fetch('/balance-clear',{method:'POST'});if(!r.ok)throw Error(await r.text());plotHistory=[];el('message').textContent='Đã xóa log. Sẵn sàng lần thử mới.';}catch(e){el('message').textContent=e.message;}}
function connect(){
 var socket=new WebSocket('ws://'+location.hostname+':81/');ws=socket;
 var deadline=setTimeout(function(){if(ws===socket&&!wsOK)socket.close();},2000);
 socket.onopen=function(){if(ws===socket)wsOK=false;};
 socket.onmessage=function(e){if(ws!==socket)return;try{var h=JSON.parse(e.data);if(Number.isInteger(h.session)&&Number.isInteger(h.token)&&h.token>0){clearTimeout(deadline);session=h.session;sessionToken=h.token;wsOK=true;if(!document.hidden)socket.send('$PING*');}}catch(ignore){}};
 socket.onclose=function(){if(ws!==socket)return;clearTimeout(deadline);driveHeld=0;driveTurn=0;drivePointer=null;cancelArmWait('Đã hủy chờ vì mất kênh lệnh.');wsOK=false;session=-1;sessionToken=0;setTimeout(connect,1000);};socket.onerror=function(){};
}
setInterval(function(){if(wsOK&&!document.hidden&&!(ws.bufferedAmount>0))ws.send('$PING*');updateButtons();},200);
document.addEventListener('visibilitychange',function(){if(document.hidden){releaseDrive();cancelArmWait('Đã hủy chờ vì trang bị ẩn.');if(wsOK)ws.send('$BACKGROUND*');}});
window.addEventListener('pagehide',function(){releaseDrive();cancelArmWait('Đã hủy chờ vì rời trang.');if(wsOK)ws.send('$LEAVE*');});
function detailRows(items){var key=[row[4],row[6],row[28],data.meta].join('/');if(key===detailKey&&Date.now()-detailUpdated<500)return;detailKey=key;detailUpdated=Date.now();var table=el('details');table.replaceChildren();items.forEach(function(item){var tr=document.createElement('tr');item.forEach(function(value){var td=document.createElement('td');td.textContent=value;tr.appendChild(td);});table.appendChild(tr);});}
function draw(){var c=el('chart'),g=c.getContext('2d');g.clearRect(0,0,c.width,c.height);g.strokeStyle='#334155';g.beginPath();g.moveTo(0,85);g.lineTo(c.width,85);g.stroke();[0,1].forEach(function(k){g.strokeStyle=k?'#fb923c':'#70edb3';g.beginPath();plotHistory.forEach(function(v,i){var x=i*c.width/299,y=85-75*Math.max(-1,Math.min(1,v[k]));if(i)g.lineTo(x,y);else g.moveTo(x,y);});g.stroke();});}
async function poll(){
 if(polling)return;polling=true;
 try{
  var r=await fetch('/balance-status',{cache:'no-store',signal:AbortSignal.timeout(2000)});if(!r.ok)throw Error('HTTP');var d=await r.json();
  data=d;
 if(!d.seen||d.age_ms>=500){
   cancelArmWait('Đã hủy chờ vì mất dữ liệu mới từ STM32.');
   el('status').textContent='Chưa nhận STM32 đang hoạt động';el('link').textContent=wsOK?'Wi-Fi OK · chờ STM32':'Mất kết nối web';
   el('reason').textContent='Giữ xe yên 5 giây sau cấp nguồn. Nếu chờ quá 10 giây: kiểm tra nguồn STM32, dây UART hoặc phiên bản đã nạp.';
   var h=await fetch('/hardware-status',{cache:'no-store',signal:AbortSignal.timeout(1500)}),hd=await h.json();
   if(hd.balance_live!==true&&hd.seen&&hd.age_ms<3000){el('status').textContent='STM32 đang ở chế độ đo linh kiện';el('reason').textContent='Muốn chạy cân bằng/thử bánh tại trang này: khởi động lại STM32. Hoặc mở bảng hiệu chuẩn nâng cao ở cuối trang.';}
   updateButtons();return;
  }
  var a=d.row.split(',').map(Number);if(a.length!==40||!a.every(Number.isFinite)||a[27]!==12)throw Error('Dữ liệu STM32 không hợp lệ');row=a;lastFresh=Date.now()-d.age_ms;
  if(a[32]===driveEpoch&&a[38]>driveAckSequence&&driveSent.has(a[38])){driveAckSequence=a[38];driveAckMs=Date.now()-driveSent.get(a[38]);}
  el('link').textContent=wsOK?'Đã kết nối STM32 + web':'STM32 OK · mất kênh lệnh';
  el('status').textContent=a[4]===3?(a[28]===2?'ĐANG LẤY LẠI THĂNG BẰNG R':a[28]===1?'ĐANG BẮT THĂNG BẰNG R':'ĐANG CHẠY CÂN BẰNG R')+activeController():a[4]===7?'ĐANG THỬ BÁNH · 1,5 GIÂY':'MOTOR ĐANG DỪNG';
  el('reason').textContent=isRunning()?(ownsControl()?'Trang này đang điều khiển xe. Giữ trang sáng và ở phía trước; chuyển ứng dụng sẽ DỪNG.':'Trang chỉ xem. Việc rời tab này không dừng xe; nút DỪNG vẫn dùng được.'):(blockers().join(' ')||faultText(a[6])||'');
  if(pending&&pending.calibration&&((a[1]-pending.startedMs)>>>0)>=4500&&((a[1]-pending.startedMs)>>>0)<2147483648&&a[4]!==1){el('message').textContent=a[6]?'Đo gyro kết thúc nhưng còn lỗi: '+faultText(a[6]):'Đã đo lại gyro. Giữ xe yên, chờ điều kiện bật cân bằng được xác nhận.';pending=null;}
  if(pending&&!pending.calibration&&a[4]===pending.state){el('message').textContent='STM32 đã nhận chế độ và đang điều khiển motor.';pending=null;}
  if(pending&&Date.now()>pending.until){el('message').textContent='Đã gửi lệnh nhưng chưa thấy STM32 chuyển chế độ. '+(armBlockers().join(' ')||'Kiểm tra dây UART từ ESP32 TX đến STM32 RX.');pending=null;}
  if(pulse&&a[4]===7){pulse.observed=true;pulse.peak=Math.max(pulse.peak,Math.abs(a[7]),Math.abs(a[8]));el('pulse-result').textContent='STM32 đã xuất PWM '+a[7]+' / '+a[8]+'. Encoder đang thay đổi: '+Math.abs(a[9]-pulse.startL)+' / '+Math.abs(a[10]-pulse.startR)+' ticks.';}
  if(previousState===7&&a[4]!==7&&pulse){var dl=Math.abs(a[9]-pulse.startL),dr=Math.abs(a[10]-pulse.startR);var moved=(pulse.wheel===1?dl>=4:pulse.wheel===2?dr>=4:dl>=4&&dr>=4);
   el('pulse-result').textContent=(a[6]&~128?'Bài thử đã dừng: '+faultText(a[6]):'Đã kết thúc bài thử.')+'\nPWM lớn nhất đã quan sát: '+pulse.peak+'; encoder L/R: '+dl+' / '+dr+' ticks.\n'+(moved?'Encoder đã ghi nhận bánh quay. Đối chiếu chiều quay bằng mắt và tải log.':'Chưa thấy đủ xung encoder. Nếu PWM đã có mà bánh không quay: kiểm tra nguồn motor/driver và dây motor; nếu bánh quay nhưng ticks không đổi: kiểm tra encoder.');pulse=null;}
  if((previousState===3||previousState===7)&&!isRunning()){endedAt=Date.now();el('message').textContent='Motor đã dừng theo dữ liệu STM32. Chờ 2 giây rồi tải log.';}previousState=a[4];
  el('pitch').textContent=a[17].toFixed(2);
  var tm=el('tilt-marker');if(tm&&tm.style){tm.style.left=(50+Math.max(-1,Math.min(1,a[17]/60))*50)+'%';tm.style.background=Math.abs(a[17])>25?'#fb7185':'#5eead4';}
  var sb=el('status');if(sb&&sb.dataset)sb.dataset.state=String(a[4]);
  el('speed').textContent=((a[19]+a[20])/2).toFixed(3);el('distance').textContent=((a[21]-a[30])*100).toFixed(1)+' / '+(a[29]*100).toFixed(0);el('pwm').textContent=a[7]+' / '+a[8];
  if(lastSeq!==a[0]){plotHistory.push([a[17]/Math.max(10,armAngleLimit()),(a[21]-a[30])/Math.max(.1,a[29])]);if(plotHistory.length>300)plotHistory.shift();lastSeq=a[0];draw();}
  el('chart-scale').textContent='60 giây gần nhất: góc xanh ±'+Math.max(10,armAngleLimit())+'°, sai lệch điểm giữ cam ±'+(100*a[29]).toFixed(0)+' cm. Trần quãng trôi 1 m.';
  var m=d.meta.split(','),assist=wheelAssist(a,m);
  el('wheel-difference').textContent=assist?(100*assist.delta).toFixed(2):'—';
  el('heading-assist').textContent=assist?assist.pwm.toFixed(0):'—';
  el('integral').textContent=a[25].toFixed(3);el('theta-target').textContent=a[23].toFixed(2);
  el('integral-label').textContent='Tích phân giữ hướng';
  el('integral-unit').textContent='N·m · giới hạn ±0,04';
  el('theta-label').textContent='Góc thân ước lượng động học';
  detailRows([
   ['STM32',m[1]||'chờ thông tin phiên bản'],['Gyro Y đã trừ bias',a[18].toFixed(3)+' °/s'],['Vận tốc L / R',a[19].toFixed(4)+' / '+a[20].toFixed(4)+' m/s'],['Encoder thô L / R',a[9]+' / '+a[10]+' ticks'],
   ['Accel thô XYZ',a.slice(14,17).map(function(v){return(v/4096).toFixed(4);}).join(' / ')+' g'],['Gyro thô XYZ',a.slice(11,14).map(function(v){return(v/16.4).toFixed(3);}).join(' / ')+' °/s'],
   ['Điều khiển','R12 · phản hồi trạng thái + bộ ước lượng'],['Gain lực Kx / Kv',m.slice(2,4).join(' / ')],['Gain lực Kθ / Kω',m.slice(4,6).join(' / ')],['Tỉ lệ ước lượng / PPR',m[6]+' / '+m[7]],['Giới hạn tốc độ theo pha',a[28]===2?'1,2 m/s · phục hồi tối đa4s':a[28]===1?'≤48°:1,0; ≤58°:1,1; >58°:1,2 m/s · 3s; >58°:4s':'0,8 m/s · thường'],['Pha / ngân sách trôi',(a[28]===2?'Lấy lại thăng bằng':a[28]===1?'Bắt thăng bằng':'Cân bằng thường')+' / '+(100*a[29]).toFixed(1)+' cm, trần1m'],['CF alpha / giới hạn góc bật',(m[18]||'chờ cấu hình')+' / ±'+armAngleLimit()+'°'],['Vận tốc / góc thân ước lượng',a[22].toFixed(4)+' m/s / '+a[23].toFixed(3)+'°'],['PWM trung bình / tích phân hướng',a[24].toFixed(2)+' / '+a[25].toFixed(4)+(' N·m')],['VM tham chiếu mô hình','11,7 V · không phải số đo trực tiếp'],['Yaw từ chênh encoder',assist?(assist.delta/.262*180/Math.PI).toFixed(2)+'°':'—'],
   ['Vận tốc đặt / điểm dừng mới',a[31].toFixed(3)+' m/s / '+(a[30]*100).toFixed(1)+' cm'],['Quãng đường từ ARM',(a[21]*100).toFixed(1)+' cm'],['Lệnh nút / cờ di chuyển',a[34]+' / '+a[33]],['Chu kỳ / thời gian xử lý',a[2]+' / '+a[3]+' µs'],['Mất mẫu tại STM32',a[26]],['Điện áp / dòng motor','Chưa đo']]);
  serviceArmWait();el('log-status').textContent=d.samples+' / '+d.capacity+' mẫu · ghi 50 Hz khi motor chạy.\nCRC lỗi: '+d.crc+' · thiếu gói: '+d.gaps+' · bỏ do đầy: '+d.overflow+' · STM32 reset: '+d.resets+(d.overflow?' — LOG KHÔNG ĐẦY ĐỦ':'');updateButtons();
 }catch(e){driveHeld=0;driveTurn=0;drivePointer=null;cancelArmWait('Đã hủy chờ vì lỗi kết nối/dữ liệu.');el('link').textContent='Mất dữ liệu mới';el('reason').textContent='Không lấy được dữ liệu: '+e.message;updateButtons();}
 finally{polling=false;}
}
async function deleteArchive(name){if(isRunning())return;if(!confirm('Đã tải và kiểm tra log '+name+'? Xóa bản Flash này?'))return;try{var r=await fetch('/balance-archive-delete',{method:'POST',body:new URLSearchParams({name:name,session:String(session),token:String(sessionToken)})});if(!r.ok)throw Error(await r.text());el('message').textContent='Đã xóa bản Flash được chọn.';networkPollCount=1;}catch(e){el('message').textContent=e.message;}}
async function pollNetwork(){
 if(networkPolling||document.hidden)return;networkPolling=true;
 try{
  var r=await fetch('/network-status',{cache:'no-store',signal:AbortSignal.timeout(1500)});if(!r.ok)throw Error('HTTP');var n=await r.json();
  if(networkBoot!==null&&networkBoot!==n.boot_id){driveHeld=0;driveTurn=0;drivePointer=null;cancelArmWait('ESP32 vừa khởi động lại; không tự bật motor.');el('message').textContent='ESP32 ĐÃ KHỞI ĐỘNG LẠI. Dừng và tải log Flash bên dưới; kiểm tra nguồn5V nếu báo brownout/power_on.';}networkBoot=n.boot_id;
  var reset={power_on:'cấp nguồn/reset nguồn',brownout:'SỤT ÁP NGUỒN',panic:'lỗi phần mềm/panic',task_watchdog:'watchdog tác vụ',interrupt_watchdog:'watchdog ngắt',software_restart:'khởi động lại bằng phần mềm',watchdog:'watchdog',external_reset:'reset ngoài'}[n.reset_reason]||n.reset_reason;
  var archive=n.archive_ready?'Flash đang lưu: '+n.archive_saved+' mẫu':({filesystem_unavailable_preserved:'Flash chưa gắn được; giữ nguyên dữ liệu cũ, chưa có dự phòng',four_archives_full:'Flash đầy4log: tải và xóa bản cũ để lưu lượt mới',flash_space_low:'Flash thiếu chỗ',flash_io_error:'Lỗi ghi Flash; cần tải phần còn đọc được',archive_task_unavailable:'Không khởi tạo được tác vụ lưu Flash'}[n.archive_error]||n.archive_error);
  el('network-health').textContent='ESP32 phiên #'+n.boot_id+' · đã chạy '+Math.floor(n.uptime_ms/1000)+'s · lý do khởi động: '+reset+'\nWi-Fi: '+n.ap_clients+' thiết bị · số lần rời mạng '+n.ap_leaves+' · mã gần nhất '+n.ap_reason+'\nRAM trống/thấp nhất: '+Math.round(n.heap_free/1024)+' / '+Math.round(n.heap_min/1024)+'KB · khoảng ngắt vòng web lớn nhất '+n.loop_gap_max_ms+'ms\n'+archive;
  if(!isRunning()&&++networkPollCount%2===0){
   var f=await fetch('/balance-archives',{cache:'no-store',signal:AbortSignal.timeout(1500)});if(f.ok){var files=await f.json(),box=el('archive-list');box.replaceChildren();
    files.files.forEach(function(x){var p=document.createElement('p'),a=document.createElement('a'),b=document.createElement('button');
     a.href='/balance-archive?name='+encodeURIComponent(x.name);a.onclick=function(e){if(isRunning()){e.preventDefault();el('message').textContent='Dừng motor trước khi tải log Flash.';}};a.textContent='TẢI LOG FLASH · '+x.name+' · '+x.samples+' mẫu'+(x.partial?' · phần cuối chưa trọn':'');p.appendChild(a);
     b.textContent='Xóa bản này';b.disabled=x.current||!wsOK;b.onclick=function(){deleteArchive(x.name);};p.appendChild(b);box.appendChild(p);
    });
   }
  }
 }catch(e){el('network-health').textContent='Chưa liên lạc được ESP32. Nếu Wi-Fi biến mất rồi log RAM về0, kiểm tra mục lý do khởi động sau khi kết nối lại. Không rút pin trước khi tải log còn lại.';}
 finally{networkPolling=false;}
}
connect();poll();setInterval(poll,200);setInterval(pollNetwork,1000);
</script>
</body></html>
)dash";
