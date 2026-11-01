#pragma once
#include "firmware_build.h"
const char DASHBOARD_HTML[] PROGMEM=R"dash(
<!doctype html><html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TWIP · Vận hành xe</title><style>
:root{color-scheme:dark;--bg:#0b1220;--card:#131f31;--ink:#eaf1fa;--muted:#a6b6cb;--green:#70edb3;--line:#2a3c54;--red:#ff667b}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.5 system-ui,-apple-system,sans-serif}main{max-width:1040px;margin:auto;padding:24px 18px 100px}header{display:flex;align-items:center;justify-content:space-between;gap:16px}h1{font-size:23px;margin:0}h2{font-size:21px;margin:0 0 10px}p{margin:8px 0;color:var(--muted)}small{color:var(--muted)}.tag{font-size:13px;border:1px solid var(--line);padding:6px 12px;border-radius:30px}.card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:22px;margin-top:18px}.hero{border-left:5px solid var(--green)}#status{font-size:25px;font-weight:750;margin:0 0 6px}.tabs,.actions{display:flex;gap:10px;flex-wrap:wrap}button,a.button{font:inherit;font-weight:650;border-radius:11px;border:1px solid var(--line);padding:13px 18px;background:#20324a;color:var(--ink);cursor:pointer;text-decoration:none}button:disabled{opacity:.4;cursor:default}.tabs button{flex:1}.tabs .selected,.primary{background:var(--green);color:#08221b}.stop{background:#9c263e;border-color:#fb7185;font-size:18px}.check{display:flex;gap:10px;align-items:flex-start;margin:15px 0}input[type=checkbox]{width:23px;height:23px;flex-shrink:0}.number{font-size:29px;font-weight:700;font-variant-numeric:tabular-nums}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}.metric{background:#0e1928;padding:14px;border-radius:12px}.metric small{display:block}.hint{background:#0e1928;border-radius:10px;padding:12px;white-space:pre-line}#message{white-space:pre-line;color:var(--green)}details{margin-top:14px}summary{cursor:pointer;color:var(--green);font-weight:600}table{width:100%;border-collapse:collapse;font-size:14px}td{padding:8px 4px;border-bottom:1px solid var(--line)}td:last-child{text-align:right;font-variant-numeric:tabular-nums}canvas{width:100%;height:170px;display:block;margin-top:12px}a{color:var(--green)}footer{position:fixed;bottom:0;left:0;right:0;background:#0b1220ee;border-top:1px solid var(--line);padding:12px;text-align:center;backdrop-filter:blur(10px)}footer button{width:min(100%,560px)}.good{color:var(--green)}.bad{color:var(--red)}[hidden]{display:none!important}@media(max-width:650px){.grid{grid-template-columns:repeat(2,1fr)}.card{padding:16px}.actions button{flex:1}header{align-items:flex-start}.number{font-size:25px}}
.drive-pad{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px;max-width:540px;margin:16px auto;touch-action:none;user-select:none}.drive-pad button{min-height:70px;touch-action:none;padding:10px 4px}.drive-pad .held{background:var(--green);color:#08221b}
</style></head><body><main>
<header><div><h1>TWIP · Vận hành xe</h1><small>)dash" TWIP_WEB_BUILD R"dash(</small></div><span class="tag" id="link">Đang kết nối…</span></header>
<section class="card hero"><div id="status">Đang chờ STM32</div><p id="reason">Sau khi cắm pin, giữ thân xe yên khoảng 5 giây để đo gyro. Xe chưa tự quay bánh.</p><div id="message" role="status" aria-live="polite"></div></section>
<section class="card"><h2>R12 · Cấu hình chính thức</h2><p class="hint" id="profile-status">Đang chờ STM32 xác nhận R12…</p><p>Xe luôn dùng R12 khi cấp nguồn. Khởi động nghiêng trước/sau đến ±60°. Motor chỉ chạy sau khi bạn bấm nút bên dưới. REC1 bổ sung phục hồi sau đẩy; giữ nguyên khởi động60° và luật cân bằng.</p></section>
<section class="card"><div class="tabs"><button id="tab-bench" class="selected" onclick="selectMode('bench')">1 · Xe đang kê cao</button><button id="tab-balance" onclick="selectMode('balance')">2 · Xe đặt trên nền</button></div>
<div id="bench"><h2 style="margin-top:20px">Thử từng bánh · tự dừng sau 1,5 giây</h2><p>Chọn mục này khi hai bánh đang rời mặt đất. Đây là thử motor và encoder, không chạy PID cân bằng.</p>
<label class="check"><input type="checkbox" id="fixture" onchange="updateButtons()"><span>Tôi đã kê chắc thân xe, hai bánh quay tự do và không có vật chạm vào bánh.</span></label>
<div class="actions"><button id="left" onclick="operate('pulse',1)">Thử bánh TRÁI</button><button id="right" onclick="operate('pulse',2)">Thử bánh PHẢI</button><button id="both" onclick="operate('pulse',3)">Thử CẢ HAI</button></div>
<p><small>PWM cố định +600 / 2499, tối đa 1,5 giây mỗi lần bấm. Chờ lần thử kết thúc rồi chọn bánh tiếp theo.</small></p><div class="hint" id="pulse-result">Bánh sẽ chỉ quay sau khi bạn đánh dấu xác nhận và bấm nút thử.</div></div>
<div id="balance" hidden><h2 style="margin-top:20px">R12 · Tự cân bằng · khởi động đến ±60°</h2><p>Đặt hai bánh xuống nền phẳng. Giữ thân xe ở góc muốn bắt đầu trong ±60°, yên khoảng 0,2 giây. Bấm BẬT CÂN BẰNG rồi thả tay khi bánh bắt đầu chạy. Chuẩn bị đỡ xe khi cần.</p>
<label class="check"><input type="checkbox" id="floor" onchange="updateButtons()"><span>Tôi đã đặt bánh trên nền phẳng và sẵn sàng đỡ thân xe.</span></label><button class="primary" id="arm" onclick="beginArm()">BẬT CÂN BẰNG</button><p id="arm-reason" class="hint">Chờ dữ liệu từ STM32…</p><p><small>Pha dựng khi góc bật trên20°: tối đa3giây; trên58°: tối đa4giây. Khi đã bắt ổn, xe tiếp tục cân bằng không hẹn giờ dừng. Ngân sách trôi tăng theo góc bật, tối đa1m; xe không cần dùng hết khoảng này. Xe dừng nếu vượt giới hạn hoặc trang bị ẩn/mất kết nối. Khi quay video, giữ web điều khiển mở trên laptop và dùng iPhone quay. Sau khi cân bằng ổn0,5giây, REC1 có thể mở pha phục hồi khi bị đẩy: góc tối đa50°, tốc độ1,2m/s, tối đa4giây; giới hạn sai lệch so với điểm đặt vẫn tối đa1m, không giới hạn đường chạy chủ động. Khi yên và về ngân sách cũ0,3giây, xe trở lại giới hạn thường25°/0,8m/s. Không bảo đảm mọi va đập hoặc dốc60°.</small></p></div>
<details><summary>Giữ xe yên để đo lại gyro</summary><p>Dừng motor và cố định thân xe. Bấm nút rồi giữ yên khoảng 5 giây; không thay đổi góc khi đo.</p><button id="calib" onclick="operate('calib')">ĐO LẠI GYRO</button></details></section>
<section class="card"><h2>Giai đoạn2 · TIẾN / LÙI</h2><p>Bật cân bằng trên nền trước, chờ xe ổn định rồi giữ nút để chạy. Nhả nút: xe giảm tốc và giữ cân bằng tại điểm dừng mới.</p>
<div class="actions"><button id="speed-normal" onclick="selectDriveSpeed(0)">ÊM · 40 cm/s</button><button id="speed-fast" onclick="selectDriveSpeed(1)">NHANH · 60 cm/s · thử nghiệm</button></div><p id="speed-ack">Mặc định 40 cm/s. Đổi mức tốc độ sẽ nhả lệnh cũ; cần giữ nút lại.</p>
<div class="drive-pad"><button id="drive-forward-left" data-drive="1" data-turn="1">↖ Tiến trái</button>
<button id="drive-forward" data-drive="1" data-turn="0">↑ Tiến</button>
<button id="drive-forward-right" data-drive="1" data-turn="-1">↗ Tiến phải</button>
<button id="drive-left" data-drive="0" data-turn="1">↶ Quay trái</button>
<button id="drive-hold" onclick="releaseDrive()">DỪNG CHẠY</button>
<button id="drive-right" data-drive="0" data-turn="-1">↷ Quay phải</button>
<button id="drive-reverse-left" data-drive="-1" data-turn="-1">↙ Lùi trái</button>
<button id="drive-reverse" data-drive="-1" data-turn="0">↓ Lùi</button>
<button id="drive-reverse-right" data-drive="-1" data-turn="1">↘ Lùi phải</button></div><p class="hint" id="drive-status">Bật cân bằng, chờ sẵn sàng rồi giữ nút để chạy.</p><p><small>Nhả để giảm tốc. Trái/phải quay tại chỗ; hướng chéo kết hợp tiến/lùi với quay thân. Hướng chéo tính theo quỹ đạo: lùi trái làm thân quay phải, lùi phải làm thân quay trái. Nút đỏ cắt motor.</small></p></section>
<section class="card"><h2>Xe đang làm gì?</h2><div class="grid"><div class="metric"><small>Góc thân · + về trước</small><span class="number" id="pitch">—</span><small>độ</small></div><div class="metric"><small>Vận tốc hai bánh</small><span class="number" id="speed">—</span><small>m/s · ước lượng encoder</small></div><div class="metric"><small>Độ lệch điểm giữ cân bằng</small><span class="number" id="distance">—</span><small>cm · sai lệch / giới hạn hiện tại</small></div><div class="metric"><small>PWM trái / phải</small><span class="number" id="pwm">—</span><small>giới hạn ±2499</small></div></div>
<div class="grid" style="margin-top:12px"><div class="metric"><small>Đường bánh phải − trái</small><span class="number" id="wheel-difference">—</span><small>cm · encoder, khi cân bằng</small></div><div class="metric"><small>Bù lệch hai bánh</small><span class="number" id="heading-assist">—</span><small>PWM · (trái − phải)/2, gồm bù motor</small></div><div class="metric"><small id="integral-label">Tích phân điều khiển</small><span class="number" id="integral">—</span><small id="integral-unit">—</small></div><div class="metric"><small id="theta-label">Góc điều khiển</small><span class="number" id="theta-target">—</span><small>độ</small></div></div>
<canvas id="chart" width="900" height="170"></canvas><small id="chart-scale">60 giây gần nhất: góc thân xanh, sai lệch điểm giữ cam; thang đo theo chế độ và ngân sách lượt chạy.</small>
<details><summary>Thông số chi tiết · IMU, encoder và điều khiển R12</summary><table id="details"></table></details></section>
<section class="card"><h2>Lưu dữ liệu để phân tích</h2><p id="log-status">Chưa nhận log.</p><div class="actions"><button id="download" onclick="location.href='/balance-log'">TẢI LOG TXT</button><button id="clear" onclick="clearLog()">XÓA LOG CŨ</button></div><p><small>Log tự ghi trên ESP32 khi motor chạy, thêm 2 giây sau dừng. Dừng trước khi tải. Tải xong mới xóa; mất nguồn ESP32 sẽ mất log chưa tải. Bộ nhớ đầy chỉ dừng ghi thêm, xe vẫn điều khiển được; muốn ghi đủ lượt thử nên dừng và tải trước3phút. Bộ nhớ đầy chỉ dừng ghi thêm, xe vẫn điều khiển được; muốn ghi đủ lượt thử nên dừng và tải trước3phút.</small></p></section>
<p><a class="button" href="/wheel-tests">ĐO HAI MOTOR / CHỐNG XOAY — 12 BÀI</a></p>
<p style="margin-top:22px"><a href="/calibration">Mở bảng hiệu chuẩn nâng cao 56 bài</a> · <a href="/firmware-info">Kiểm tra phiên bản</a></p><p><small>Điện áp pin, dòng motor và mô-men: chưa có phép đo. Web không hiển thị giá trị giả định thay số đo.</small></p>
</main><footer><button class="stop" onclick="operate('stop')">■ DỪNG MOTOR NGAY</button></footer>
<script>
'use strict';
var data=null,row=[],mode='bench',ws=null,wsOK=false,busy=false,polling=false,lastFresh=0,lastSeq=-1,plotHistory=[],pending=null,pulse=null,endedAt=0,previousState=-1,session=-1,sessionToken=0,armWaitUntil=0;
var driveTurn=0;
var driveHeld=0,drivePointer=null,driveSequence=0,driveEpoch=0,drivePreset=0;
function selectDriveSpeed(preset){if(preset!==0&&preset!==1)return;releaseDrive();drivePreset=preset;driveTick();updateButtons();}
function el(id){return document.getElementById(id);}
function ownsControl(){return data&&data.owner===session&&data.owner_token===sessionToken&&sessionToken>0;}
function canDrive(){return fresh()&&matched()&&wsOK&&ownsControl()&&!document.hidden&&!busy&&!pending&&!armWaitUntil&&mode==='balance'&&el('floor').checked&&row[4]===3&&row[28]===0&&row[6]===0&&(row[33]&1)&&data.drive_epoch>0&&row[32]===data.drive_epoch&&blockers(true).length===0;}
function driveTick(){
 if((driveHeld||driveTurn)&&!canDrive()){driveHeld=0;driveTurn=0;drivePointer=null;}
 if(!wsOK||!fresh()||!matched()||!ownsControl()||row[4]!==3||document.hidden||row[32]!==data.drive_epoch||!data.drive_epoch)return;
 if(driveEpoch!==data.drive_epoch){driveEpoch=data.drive_epoch;driveSequence=0;driveHeld=0;driveTurn=0;drivePointer=null;drivePreset=0;}
 if(driveSequence>=1000000000){driveHeld=0;driveTurn=0;return;}
 ws.send('$MOVE8,'+driveHeld+','+driveTurn+','+driveEpoch+','+(++driveSequence)+','+drivePreset+'*');
}
function releaseDrive(){driveHeld=0;driveTurn=0;drivePointer=null;driveTick();}
var driveButtons=Array.from(document.querySelectorAll('[data-drive]'));
function pressDrive(direction,turn,event){
 if(event){if(event.button!==undefined&&event.button!==0)return;if(event.isPrimary===false)return;event.preventDefault();}
 if(!canDrive())return;
 if(drivePointer!==null)return;
 if(driveEpoch!==data.drive_epoch){driveEpoch=data.drive_epoch;driveSequence=0;}
 driveHeld=direction;driveTurn=turn;drivePointer=event&&event.pointerId!==undefined?event.pointerId:'keyboard';
 if(event&&Number.isInteger(event.pointerId)&&event.currentTarget&&event.currentTarget.setPointerCapture)event.currentTarget.setPointerCapture(event.pointerId);
 driveTick();
}
driveButtons.forEach(function(b){var d=Number(b.dataset.drive),t=Number(b.dataset.turn);
 b.onpointerdown=function(e){pressDrive(d,t,e);};
 ['pointerup','pointercancel','lostpointercapture'].forEach(function(type){b.addEventListener(type,function(e){if(drivePointer===e.pointerId)releaseDrive();});});
 b.oncontextmenu=function(e){e.preventDefault();};
 b.onkeydown=function(e){if((e.key===' '||e.key==='Enter')&&!e.repeat)pressDrive(d,t,e);};
 b.onkeyup=function(e){if(e.key===' '||e.key==='Enter'){e.preventDefault();releaseDrive();}};
});
document.addEventListener('pointerup',releaseDrive);document.addEventListener('pointercancel',releaseDrive);window.addEventListener('blur',releaseDrive);
setInterval(driveTick,100);
function stopSource(){return ({controller_background:'Trang điều khiển bị ẩn (chuyển ứng dụng hoặc khóa màn hình)',controller_page_left:'Rời trang điều khiển',controller_disconnected:'Trang điều khiển mất kết nối',stop_button_ws:'Nút DỪNG trên web',stop_button_http:'Nút DỪNG trên web'})[data&&data.stop_source]||'Lệnh STOP; chưa biết nguồn';}
function isRunning(){return row[4]===3||row[4]===7;}
function fresh(){return data&&data.seen&&data.age_ms<500&&Date.now()-lastFresh<500;}
function activeController(){return row[27];}
function armAngleLimit(){var m=data?data.meta.split(','):[],a=Number(m[19]);return (activeController()===12&&a===60)?60:0;}
function matched(){return data&&data.meta.indexOf('$BINFO,BAL-20261018-R12-SPEED2-8DIR,')===0&&activeController()===12&&Number(data.meta.split(',')[17])===12;}
function selectMode(next){releaseDrive();if(next!=='balance')cancelArmWait('Đã hủy chờ vì đổi chế độ.');mode=next;el('bench').hidden=next!=='bench';el('balance').hidden=next!=='balance';el('tab-bench').className=next==='bench'?'selected':'';el('tab-balance').className=next==='balance'?'selected':'';updateButtons();}
function blockers(forMotion){
 if(!fresh())return ['Chưa nhận dữ liệu mới từ STM32. Kiểm tra nguồn STM32 và dây UART; Wi-Fi kết nối chưa đủ.'];
 if(!matched())return ['Chờ thông tin phiên bản STM32 BAL-20261018-R12-SPEED2-8DIR. Nạp cả STM32 và ESP32 bản R12 chính thức; chờ cấu hình xác nhận.'];
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
 el('speed-normal').className=drivePreset===0?'selected':'';el('speed-fast').className=drivePreset===1?'selected':'';
 el('speed-ack').textContent='Đã chọn '+(drivePreset?60:40)+' cm/s. STM32 '+(row[4]===3?'đang nhận mức '+((row[33]&256)?60:40)+' cm/s.':'sẽ xác nhận sau khi bật cân bằng.')+' Đổi mức sẽ nhả lệnh cũ, cần giữ nút lại.';
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
 el('drive-status').textContent=row[4]!==3?'Bật cân bằng trên nền trước khi điều khiển.':!ownsControl()?'Trang này chỉ xem; điều khiển trên trang đã bấm BẬT.':row[28]?'Chờ xe hoàn tất dựng/phục hồi.':(row[33]&64)?((row[33]&1)?'Đã bỏ điểm đặt cũ và cân bằng ổn. Giữ nút mới để chạy tiếp.':'Đã hủy chuyển động cũ. Nhả nút, chờ xe cân bằng; không tự chạy lại.'):(row[33]&8)?'Xe đang theo chưa kịp điểm đặt; tạm giảm tốc để giữ thăng bằng.':(row[33]&4)?'Đang giảm tốc — giữ cân bằng.':Math.abs(row[31]||0)>.0001?'Đang '+(row[31]>0?'TIẾN':'LÙI')+' · vận tốc đặt '+(100*row[31]).toFixed(1)+' cm/s':canDrive()?'Sẵn sàng. Giữ một trong 8 nút; nhả để giảm tốc.':'Chờ xe cân bằng ổn định; nhả nút rồi bấm lại.';
 if(row[4]===3&&ownsControl()&&row[28]===0&&!(row[33]&64)&&(Math.abs(row[36]||0)>.0001))el('drive-status').textContent='Đang quay '+(row[36]>0?'trái':'phải')+' · '+row[36].toFixed(2)+' rad/s · vận tốc đặt '+(100*row[31]).toFixed(1)+' cm/s';
 el('arm-reason').textContent=isRunning()?'Motor đang chạy. Bấm DỪNG trước khi đổi bài.':(armWaitUntil?'ĐANG CHỜ DỰNG XE · còn '+Math.max(0,Math.ceil((armWaitUntil-Date.now())/1000))+' giây\n'+armBlockers().join('\n'):(armSafetyBlockers().join('\n')||(!el('floor').checked?'Đánh dấu xác nhận xe đã đặt trên nền.':'Có thể bấm BẬT trước, rồi giữ xe yên trong ±60°.\n'+armBlockers().join('\n'))));
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
 }catch(e){el('message').textContent='Chưa thực hiện được: '+e.message;}
 finally{busy=false;updateButtons();}
}
async function clearLog(){if(!confirm('Chỉ xóa khi bạn đã lưu file TXT. Xóa log cũ?'))return;try{var r=await fetch('/balance-clear',{method:'POST'});if(!r.ok)throw Error(await r.text());plotHistory=[];el('message').textContent='Đã xóa log. Sẵn sàng lần thử mới.';}catch(e){el('message').textContent=e.message;}}
function connect(){ws=new WebSocket('ws://'+location.hostname+':81/');ws.onopen=function(){wsOK=false;};ws.onmessage=function(e){try{var h=JSON.parse(e.data);if(Number.isInteger(h.session)&&Number.isInteger(h.token)&&h.token>0){session=h.session;sessionToken=h.token;wsOK=true;if(!document.hidden)ws.send('$PING*');}}catch(ignore){}};ws.onclose=function(){driveHeld=0;driveTurn=0;drivePointer=null;cancelArmWait('Đã hủy chờ vì mất kênh lệnh.');wsOK=false;session=-1;sessionToken=0;setTimeout(connect,1500);};ws.onerror=function(){};}
setInterval(function(){if(wsOK&&!document.hidden)ws.send('$PING*');updateButtons();},200);
document.addEventListener('visibilitychange',function(){if(document.hidden){releaseDrive();cancelArmWait('Đã hủy chờ vì trang bị ẩn.');if(wsOK)ws.send('$BACKGROUND*');}});
window.addEventListener('pagehide',function(){releaseDrive();cancelArmWait('Đã hủy chờ vì rời trang.');if(wsOK)ws.send('$LEAVE*');});
function detailRows(items){var table=el('details');table.replaceChildren();items.forEach(function(item){var tr=document.createElement('tr');item.forEach(function(value){var td=document.createElement('td');td.textContent=value;tr.appendChild(td);});table.appendChild(tr);});}
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
  var a=d.row.split(',').map(Number);if(a.length!==38||!a.every(Number.isFinite)||a[27]!==12)throw Error('Dữ liệu STM32 không hợp lệ');row=a;lastFresh=Date.now()-d.age_ms;
  el('link').textContent=wsOK?'Đã kết nối STM32 + web':'STM32 OK · mất kênh lệnh';
  el('status').textContent=a[4]===3?(a[28]===2?'ĐANG PHỤC HỒI SAU ĐẨY R':a[28]===1?'ĐANG BẮT THĂNG BẰNG R':'ĐANG CHẠY CÂN BẰNG R')+activeController():a[4]===7?'ĐANG THỬ BÁNH · 1,5 GIÂY':'MOTOR ĐANG DỪNG';
  el('reason').textContent=isRunning()?(ownsControl()?'Trang này đang điều khiển xe. Giữ trang sáng và ở phía trước; chuyển ứng dụng sẽ DỪNG.':'Trang chỉ xem. Việc rời tab này không dừng xe; nút DỪNG vẫn dùng được.'):(blockers().join(' ')||faultText(a[6])||'Xe không tự chạy khi cắm pin. Chọn một trong hai mục bên dưới.');
  if(pending&&a[4]===pending.state){el('message').textContent='STM32 đã nhận chế độ và đang điều khiển motor.';pending=null;}
  if(pending&&Date.now()>pending.until){el('message').textContent='Đã gửi lệnh nhưng chưa thấy STM32 chuyển chế độ. '+(armBlockers().join(' ')||'Kiểm tra dây UART từ ESP32 TX đến STM32 RX.');pending=null;}
  if(pulse&&a[4]===7){pulse.observed=true;pulse.peak=Math.max(pulse.peak,Math.abs(a[7]),Math.abs(a[8]));el('pulse-result').textContent='STM32 đã xuất PWM '+a[7]+' / '+a[8]+'. Encoder đang thay đổi: '+Math.abs(a[9]-pulse.startL)+' / '+Math.abs(a[10]-pulse.startR)+' ticks.';}
  if(previousState===7&&a[4]!==7&&pulse){var dl=Math.abs(a[9]-pulse.startL),dr=Math.abs(a[10]-pulse.startR);var moved=(pulse.wheel===1?dl>=4:pulse.wheel===2?dr>=4:dl>=4&&dr>=4);
   el('pulse-result').textContent=(a[6]&~128?'Bài thử đã dừng: '+faultText(a[6]):'Đã kết thúc bài thử.')+'\nPWM lớn nhất đã quan sát: '+pulse.peak+'; encoder L/R: '+dl+' / '+dr+' ticks.\n'+(moved?'Encoder đã ghi nhận bánh quay. Đối chiếu chiều quay bằng mắt và tải log.':'Chưa thấy đủ xung encoder. Nếu PWM đã có mà bánh không quay: kiểm tra nguồn motor/driver và dây motor; nếu bánh quay nhưng ticks không đổi: kiểm tra encoder.');pulse=null;}
  if((previousState===3||previousState===7)&&!isRunning()){endedAt=Date.now();el('message').textContent='Motor đã dừng theo dữ liệu STM32. Chờ 2 giây rồi tải log.';}previousState=a[4];
  el('pitch').textContent=a[17].toFixed(2);el('speed').textContent=((a[19]+a[20])/2).toFixed(3);el('distance').textContent=((a[21]-a[30])*100).toFixed(1)+' / '+(a[29]*100).toFixed(0);el('pwm').textContent=a[7]+' / '+a[8];
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
   ['Điều khiển','R12 · phản hồi trạng thái + bộ ước lượng'],['Gain lực Kx / Kv',m.slice(2,4).join(' / ')],['Gain lực Kθ / Kω',m.slice(4,6).join(' / ')],['Tỉ lệ ước lượng / PPR',m[6]+' / '+m[7]],['Giới hạn tốc độ theo pha',a[28]===2?'1,2 m/s · phục hồi tối đa4s':a[28]===1?'≤48°:1,0; ≤58°:1,1; >58°:1,2 m/s · 3s; >58°:4s':'0,8 m/s · thường'],['Pha / ngân sách trôi',(a[28]===2?'Phục hồi sau đẩy':a[28]===1?'Bắt thăng bằng':'Cân bằng thường')+' / '+(100*a[29]).toFixed(1)+' cm, trần1m'],['CF alpha / giới hạn góc bật',(m[18]||'chờ cấu hình')+' / ±'+armAngleLimit()+'°'],['Vận tốc / góc thân ước lượng',a[22].toFixed(4)+' m/s / '+a[23].toFixed(3)+'°'],['PWM trung bình / tích phân hướng',a[24].toFixed(2)+' / '+a[25].toFixed(4)+(' N·m')],['VM tham chiếu mô hình','11,7 V · không phải số đo trực tiếp'],['Yaw từ chênh encoder',assist?(assist.delta/.262*180/Math.PI).toFixed(2)+'°':'—'],
   ['Vận tốc đặt / điểm dừng mới',a[31].toFixed(3)+' m/s / '+(a[30]*100).toFixed(1)+' cm'],['Quãng đường từ ARM',(a[21]*100).toFixed(1)+' cm'],['Lệnh nút / cờ di chuyển',a[34]+' / '+a[33]],['Chu kỳ / thời gian xử lý',a[2]+' / '+a[3]+' µs'],['Mất mẫu tại STM32',a[26]],['Điện áp / dòng motor','Chưa đo']]);
  serviceArmWait();el('log-status').textContent=d.samples+' / '+d.capacity+' mẫu · ghi 50 Hz khi motor chạy.\nCRC lỗi: '+d.crc+' · thiếu gói: '+d.gaps+' · bỏ do đầy: '+d.overflow+' · STM32 reset: '+d.resets+(d.overflow?' — LOG KHÔNG ĐẦY ĐỦ':'');updateButtons();
 }catch(e){driveHeld=0;driveTurn=0;drivePointer=null;cancelArmWait('Đã hủy chờ vì lỗi kết nối/dữ liệu.');el('link').textContent='Mất dữ liệu mới';el('reason').textContent='Không lấy được dữ liệu: '+e.message;updateButtons();}
 finally{polling=false;}
}
connect();poll();setInterval(poll,200);
</script></body></html>
)dash";
