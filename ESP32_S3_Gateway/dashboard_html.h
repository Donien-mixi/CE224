#pragma once
#include "firmware_build.h"
const char DASHBOARD_HTML[] PROGMEM=R"dash(
<!doctype html><html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TWIP · Vận hành xe</title><style>
:root{color-scheme:dark;--bg:#0b1220;--card:#131f31;--ink:#eaf1fa;--muted:#a6b6cb;--green:#70edb3;--line:#2a3c54;--red:#ff667b}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.5 system-ui,-apple-system,sans-serif}main{max-width:1040px;margin:auto;padding:24px 18px 100px}header{display:flex;align-items:center;justify-content:space-between;gap:16px}h1{font-size:23px;margin:0}h2{font-size:21px;margin:0 0 10px}p{margin:8px 0;color:var(--muted)}small{color:var(--muted)}.tag{font-size:13px;border:1px solid var(--line);padding:6px 12px;border-radius:30px}.card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:22px;margin-top:18px}.hero{border-left:5px solid var(--green)}#status{font-size:25px;font-weight:750;margin:0 0 6px}.tabs,.actions{display:flex;gap:10px;flex-wrap:wrap}button,a.button{font:inherit;font-weight:650;border-radius:11px;border:1px solid var(--line);padding:13px 18px;background:#20324a;color:var(--ink);cursor:pointer;text-decoration:none}button:disabled{opacity:.4;cursor:default}.tabs button{flex:1}.tabs .selected,.primary{background:var(--green);color:#08221b}.stop{background:#9c263e;border-color:#fb7185;font-size:18px}.check{display:flex;gap:10px;align-items:flex-start;margin:15px 0}input[type=checkbox]{width:23px;height:23px;flex-shrink:0}.number{font-size:29px;font-weight:700;font-variant-numeric:tabular-nums}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}.metric{background:#0e1928;padding:14px;border-radius:12px}.metric small{display:block}.hint{background:#0e1928;border-radius:10px;padding:12px;white-space:pre-line}#message{white-space:pre-line;color:var(--green)}details{margin-top:14px}summary{cursor:pointer;color:var(--green);font-weight:600}table{width:100%;border-collapse:collapse;font-size:14px}td{padding:8px 4px;border-bottom:1px solid var(--line)}td:last-child{text-align:right;font-variant-numeric:tabular-nums}canvas{width:100%;height:170px;display:block;margin-top:12px}a{color:var(--green)}footer{position:fixed;bottom:0;left:0;right:0;background:#0b1220ee;border-top:1px solid var(--line);padding:12px;text-align:center;backdrop-filter:blur(10px)}footer button{width:min(100%,560px)}.good{color:var(--green)}.bad{color:var(--red)}[hidden]{display:none!important}@media(max-width:650px){.grid{grid-template-columns:repeat(2,1fr)}.card{padding:16px}.actions button{flex:1}header{align-items:flex-start}.number{font-size:25px}}
</style></head><body><main>
<header><div><h1>TWIP · Vận hành xe</h1><small>)dash" TWIP_WEB_BUILD R"dash(</small></div><span class="tag" id="link">Đang kết nối…</span></header>
<section class="card hero"><div id="status">Đang chờ STM32</div><p id="reason">Sau khi cắm pin, giữ thân xe yên khoảng 5 giây để đo gyro. Xe chưa tự quay bánh.</p><div id="message" role="status" aria-live="polite"></div></section>
<section class="card"><div class="tabs"><button id="tab-bench" class="selected" onclick="selectMode('bench')">1 · Xe đang kê cao</button><button id="tab-balance" onclick="selectMode('balance')">2 · Xe đặt trên nền</button></div>
<div id="bench"><h2 style="margin-top:20px">Thử từng bánh · tự dừng sau 1,5 giây</h2><p>Chọn mục này khi hai bánh đang rời mặt đất. Đây là thử motor và encoder, không chạy PID cân bằng.</p>
<label class="check"><input type="checkbox" id="fixture" onchange="updateButtons()"><span>Tôi đã kê chắc thân xe, hai bánh quay tự do và không có vật chạm vào bánh.</span></label>
<div class="actions"><button id="left" onclick="operate('pulse',1)">Thử bánh TRÁI</button><button id="right" onclick="operate('pulse',2)">Thử bánh PHẢI</button><button id="both" onclick="operate('pulse',3)">Thử CẢ HAI</button></div>
<p><small>PWM cố định +600 / 2499, tối đa 1,5 giây mỗi lần bấm. Chờ lần thử kết thúc rồi chọn bánh tiếp theo.</small></p><div class="hint" id="pulse-result">Bánh sẽ chỉ quay sau khi bạn đánh dấu xác nhận và bấm nút thử.</div></div>
<div id="balance" hidden><h2 style="margin-top:20px">Bấm trước · dựng xe theo thanh chỉ dẫn</h2><p>Đặt hai bánh xuống nền, giữ nhẹ thân xe và bấm BẬT. Bạn có tối đa 20 giây để đưa vạch vào vùng giữa ±2°, rồi giữ yên 0,3 giây. STM32 kiểm tra lại trước khi chạy; không cần đúng tuyệt đối 0°.</p>
<div class="hint"><strong id="start-guide">Chờ số đo để hướng dẫn dựng xe…</strong><input id="start-angle" type="range" min="-8" max="8" step="0.01" value="0" disabled style="width:100%;opacity:1;background:linear-gradient(to right,#334155 37.5%,#70edb3 37.5%,#70edb3 62.5%,#334155 62.5%);height:18px"><small>Âm: nghiêng về WAGO 5 V · 0° · Dương: nghiêng về WAGO nguồn pin</small><p id="start-quality">Motor chưa chạy.</p></div>
<label class="check"><input type="checkbox" id="floor" onchange="updateButtons()"><span>Hai bánh đã trên nền phẳng. Tôi sẽ giữ nhẹ thân xe trong lúc chờ và thả nhẹ khi báo ĐANG CHẠY CÂN BẰNG.</span></label><button class="primary" id="arm" onclick="beginArm()">BẬT CÂN BẰNG · CHỜ DỰNG XE</button><p id="arm-reason" class="hint">Chờ dữ liệu từ STM32…</p><p><small>R10S1 giữ vòng cân bằng và bù hướng của R10; chỉ sửa điều kiện bắt đầu. Không có hẹn giờ dừng cân bằng. Xe ngắt nếu quá góc 25°, quãng encoder 30 cm, tốc độ 0,8 m/s hoặc trang điều khiển bị ẩn/mất kết nối. Muốn quay video: giữ web trên laptop, dùng iPhone quay.</small></p></div>
<details><summary>Đo lại gyro khi cần · không cần làm trước mỗi lượt</summary><p>Dừng motor và cố định thân xe 5 giây, sau đó đợi bộ lọc ổn định thêm 1,5 giây. Đo gyro không đặt góc hiện tại thành 0°; khi đo chỉ cần thân xe đứng yên. Không vừa di chuyển xe vừa đo.</p><button id="calib" onclick="operate('calib')">ĐO LẠI GYRO</button></details></section>
<section class="card"><h2>Xe đang làm gì?</h2><div class="grid"><div class="metric"><small>Góc thân · so với mốc hiệu chuẩn</small><span class="number" id="pitch">—</span><small>độ</small></div><div class="metric"><small>Vận tốc hai bánh</small><span class="number" id="speed">—</span><small>m/s · ước lượng encoder</small></div><div class="metric"><small>Quãng đường từ lần bật cân bằng</small><span class="number" id="distance">—</span><small>cm · ước lượng encoder</small></div><div class="metric"><small>PWM trái / phải</small><span class="number" id="pwm">—</span><small>giới hạn ±2499</small></div></div>
<div class="grid" style="margin-top:12px"><div class="metric"><small>Đường bánh phải − trái</small><span class="number" id="wheel-difference">—</span><small>cm · encoder, khi cân bằng</small></div><div class="metric"><small>Bù lệch hai bánh</small><span class="number" id="heading-assist">—</span><small>PWM · cộng trái / trừ phải, tối đa 8</small></div><div class="metric"><small>Bù chậm độ trôi</small><span class="number" id="integral">—</span><small>độ · mở sau 3 giây, giới hạn ±2°</small></div><div class="metric"><small>Góc mục tiêu</small><span class="number" id="theta-target">—</span><small>độ</small></div></div>
<div class="hint" id="capture-state">CHƯA CHẠY</div><canvas id="chart" width="900" height="170"></canvas><small>60 giây gần nhất: góc thân (xanh, ±10°), quãng đường (cam, ±10 cm). Đường vượt khung được cắt để hiển thị.</small>
<details><summary>Thông số chi tiết · IMU, encoder và PID</summary><table id="details"></table></details></section>
<section class="card"><h2>Lưu dữ liệu để phân tích</h2><p id="log-status">Chưa nhận log.</p><div class="actions"><button id="download" onclick="location.href='/balance-log'">TẢI LOG TXT</button><button id="clear" onclick="clearLog()">XÓA LOG CŨ</button></div><p><small>Log tự ghi trên ESP32 khi motor chạy, thêm 2 giây sau dừng. Dừng trước khi tải. Tải xong mới xóa; mất nguồn ESP32 sẽ mất log chưa tải.</small></p></section>
<p><a class="button" href="/wheel-tests">ĐO HAI MOTOR / CHỐNG XOAY — 12 BÀI</a></p>
<p style="margin-top:22px"><a href="/calibration">Mở bảng hiệu chuẩn nâng cao 56 bài</a> · <a href="/firmware-info">Kiểm tra phiên bản</a></p><p><small>Điện áp pin, dòng motor và mô-men: chưa có phép đo. Web không hiển thị giá trị giả định thay số đo.</small></p>
</main><footer><button class="stop" onclick="operate('stop')">■ DỪNG MOTOR NGAY</button></footer>
<script>
'use strict';
var data=null,row=[],mode='bench',ws=null,wsOK=false,busy=false,polling=false,lastFresh=0,lastSeq=-1,plotHistory=[],pending=null,pulse=null,endedAt=0,previousState=-1,session=-1,sessionToken=0,armWaitUntil=0,calibWaitUntil=0;
function el(id){return document.getElementById(id);}
function ownsControl(){return data&&data.owner===session&&data.owner_token===sessionToken&&sessionToken>0;}
function stopSource(){return ({controller_background:'Trang điều khiển bị ẩn (chuyển ứng dụng hoặc khóa màn hình)',controller_page_left:'Rời trang điều khiển',controller_disconnected:'Trang điều khiển mất kết nối',stop_button_ws:'Nút DỪNG trên web',stop_button_http:'Nút DỪNG trên web'})[data&&data.stop_source]||'Lệnh STOP; chưa biết nguồn';}
function isRunning(){return row[4]===3||row[4]===7;}
function fresh(){return data&&data.seen&&data.age_ms<500&&Date.now()-lastFresh<500;}
function matched(){return data&&data.meta.indexOf('$BINFO,BAL-20261012-R10S1,')===0;}
function selectMode(next){if(next!=='balance')cancelArmWait('Đã hủy chờ vì đổi chế độ.');mode=next;el('bench').hidden=next!=='bench';el('balance').hidden=next!=='balance';el('tab-bench').className=next==='bench'?'selected':'';el('tab-balance').className=next==='balance'?'selected':'';updateButtons();}
function blockers(){
 if(!fresh())return ['Chưa nhận dữ liệu mới từ STM32. Kiểm tra nguồn STM32 và dây UART; Wi-Fi kết nối chưa đủ.'];
 if(!matched())return ['Chờ thông tin phiên bản STM32 BAL-20261012-R10S1. Nếu vẫn hiện R1, cần nạp lại STM32.'];
 if(!wsOK)return ['Mất kênh lệnh từ trình duyệt. Kết nối lại Wi-Fi rồi mở lại trang.'];
 if(!data.capacity)return ['ESP32 chưa cấp được bộ nhớ log. Kiểm tra cấu hình OPI PSRAM khi nạp ESP32.'];
 if(data.samples>=data.capacity)return ['Log đã đầy. Dừng, tải log rồi xóa log cũ trước lần chạy mới.'];
 var f=row[6],out=[];
 if(row[4]===6||f&32)out.push('Lỗi đọc BMI160. Kiểm tra dây/nguồn cảm biến, sau đó đo lại gyro hoặc khởi động lại STM32.');
 if(f&256)out.push('Cấu hình BMI160 không đúng; chưa được phép bật motor.');
 return out;
}
function armBlockers(){
 var out=blockers();if(out.length)return out;
 if(row[6]&128)out.push('Hiệu chuẩn gyro chưa đạt. Cố định xe rồi bấm ĐO LẠI GYRO.');
 if(Math.abs(row[17])>2)out.push('Dựng thân xe gần thẳng: hiện '+row[17].toFixed(2)+'°, đưa vào vùng giữa ±2°.');
 var m=data.meta.split(','),rates=row.slice(11,14).map(function(v,i){return v/16.4-Number(m[12+i]);});
 if(Math.abs(row[17])<=2&&Math.abs(row[17]+.15*row[18])>2)out.push('Xe đang đổ ra ngoài vùng bật. Dựng chậm lại hoặc đưa gần thẳng hơn.');
 if(rates.some(function(v){return Math.abs(v)>=3;}))out.push('Thân xe còn chuyển động: gyro cần dưới 3°/s mỗi trục.');
 if(Math.abs(row[19])>=.02||Math.abs(row[20])>=.02)out.push('Bánh còn quay. Giữ xe yên và chờ bánh dừng.');
 var accPitch=stationaryAccPitch(row);
 if(Math.abs(accPitch-row[17])>.75)out.push('Bộ lọc góc đang theo lại tư thế: chênh '+Math.abs(accPitch-row[17]).toFixed(2)+'°. Giữ thân yên.');
 var norm=row.slice(14,17).reduce(function(s,v){return s+(v/4096)*(v/4096);},0);
 if(norm<=.72||norm>=1.32)out.push('Gia tốc bất thường hoặc giá kê đang rung. Cố định xe trước khi bật.');
 if(row[2]<4000||row[2]>6000)out.push('Chu kỳ điều khiển bất thường: '+row[2]+' µs.');
 if(!out.length&&!row[5])out.push('Giữ điều kiện 0,3 giây; sau đo gyro cần thêm 1,5 giây để bộ lọc ổn định.');
 return out;
}
function armSafetyBlockers(){var out=blockers();if(Date.now()<calibWaitUntil)out.push('Đang đo gyro và chờ bộ lọc. Giữ thân xe yên; chờ khoảng '+Math.ceil((calibWaitUntil-Date.now())/1000)+' giây.');if(row[6]&128)out.push('Hiệu chuẩn gyro chưa đạt. Cố định xe rồi đo lại gyro.');if(row[6]&512)out.push('Dữ liệu điều khiển không hợp lệ.');return out;}
function cancelArmWait(message){if(armWaitUntil){armWaitUntil=0;el('message').textContent=message;}}
function beginArm(){
 if(busy||pending||armWaitUntil||isRunning()||document.hidden||mode!=='balance'||!el('floor').checked||armSafetyBlockers().length)return;
 armWaitUntil=Date.now()+20000;
 el('message').textContent='ĐANG CHỜ DỰNG XE · motor chưa chạy. Đưa vạch vào vùng giữa ±2° và giữ nhẹ thân xe 0,3 giây. Chỉ thả nhẹ khi báo ĐANG CHẠY CÂN BẰNG. Chờ tối đa 20 giây; DỪNG để hủy.';
 serviceArmWait();updateButtons();
}
function serviceArmWait(){
 if(!armWaitUntil)return;
 if(document.hidden||!el('floor').checked||armSafetyBlockers().length||isRunning()||mode!=='balance'){
  cancelArmWait('Đã hủy chờ bật do điều kiện/kết nối thay đổi. Motor không được tự bật lại.');return;
 }
 if(Date.now()>=armWaitUntil){cancelArmWait('Hết 20 giây chờ; chưa bật motor. Dựng xe gần thẳng rồi bấm lại.');return;}
 if(row[5]){armWaitUntil=0;operate('arm');}
}
function wheelAssist(a,m){
 if(a[4]!==3)return null;
 var db=m.slice(8,12).map(Number),u=a[24],l=0,r=0;
 if(u>1){l=Math.trunc(Math.min(2499,u+db[0]));r=Math.trunc(Math.min(2499,u+db[2]));}
 else if(u< -1){l=Math.trunc(Math.max(-2499,u-db[1]));r=Math.trunc(Math.max(-2499,u-db[3]));}
 return {delta:(-a[10]-a[9])*0.204203522/Number(m[7]),pwm:((a[7]-l)+(r-a[8]))/2};
}
function faultText(f){return [[1,stopSource()],[2,'Mất nhịp điều khiển web quá 1 giây'],[4,'Góc thân vượt 25°'],[8,'Quãng đường encoder vượt 30 cm'],[16,'Tốc độ vượt 0,8 m/s'],[32,'Lỗi đọc BMI160'],[64,'Chu kỳ điều khiển bất thường'],[128,'Hiệu chuẩn gyro chưa đạt'],[256,'Sai cấu hình BMI160'],[512,'Giá trị số không hợp lệ'],[1024,'Thân xe/giá kê chuyển động quá mức khi thử bánh']].filter(function(x){return f&x[0];}).map(function(x){return x[1];}).join(' · ');}
function updateButtons(){
 var blocked=blockers().length||busy||pending||armWaitUntil||Date.now()<calibWaitUntil||isRunning()||document.hidden;
 ['left','right','both'].forEach(function(id){el(id).disabled=!!blocked||!el('fixture').checked;});
 el('arm').disabled=!!blocked||armSafetyBlockers().length>0||!el('floor').checked;
 el('calib').disabled=!fresh()||busy||!!pending||!!armWaitUntil||Date.now()<calibWaitUntil||isRunning();
 el('download').disabled=!fresh()||isRunning()||!!armWaitUntil||!!pending||!data.samples||Date.now()-endedAt<2200;
 el('clear').disabled=!fresh()||isRunning()||!!armWaitUntil||!!pending||busy||Date.now()-endedAt<2200;
 el('arm-reason').textContent=isRunning()?'Motor đang chạy. Bấm DỪNG trước khi đổi bài.':(armWaitUntil?'ĐANG CHỜ DỰNG XE · còn '+Math.max(0,Math.ceil((armWaitUntil-Date.now())/1000))+' giây\n'+armBlockers().join('\n'):(armSafetyBlockers().join('\n')||(!el('floor').checked?'Đánh dấu xác nhận xe đã đặt trên nền.':'Bấm BẬT trước rồi dựng theo thanh chỉ dẫn.\n'+armBlockers().join('\n'))));
}
function stationaryAccPitch(a){var ax=(a[14]/4096+.0416836098593)/.997272837552,az=(a[16]/4096-.004738195334)/1.00929388014;return -(Math.atan2(ax,az)*180/Math.PI-2.16694264915);}
function startupGuide(){
 if(!row.length)return;
 el('start-angle').value=Math.max(-8,Math.min(8,row[17]));
 var guide=row[17]>2?'Đưa thân nhẹ về WAGO 5 V (phía sau).':row[17]<-2?'Đưa thân nhẹ về WAGO nguồn pin (phía trước).':'Đã vào vùng giữa · giữ nhẹ thân xe, không đẩy bánh.';
 el('start-guide').textContent=isRunning()?'Motor đang hoạt động.':guide;
 el('start-quality').textContent=isRunning()?'Vòng cân bằng R10 đang chạy.':('Góc '+row[17].toFixed(2)+'° · tốc độ nghiêng '+row[18].toFixed(2)+'°/s · chênh góc lọc/accel '+Math.abs(row[17]-stationaryAccPitch(row)).toFixed(2)+'°\n'+(row[5]?'STM32 ĐÃ SẴN SÀNG — chờ yêu cầu BẬT.':'Chưa bật motor. '+armBlockers().join(' ')));
}
async function operate(action,wheel){
 if(action!=='stop'&&(busy||pending||armWaitUntil))return;
 if(action!=='stop'&&Date.now()<calibWaitUntil)return;
 if(action==='calib')calibWaitUntil=Date.now()+6500;
 if(action==='stop'){pending=null;cancelArmWait('Đã hủy chờ bật.');}
 if(action==='stop'&&wsOK)ws.send('$STOP*');
 busy=true;el('message').textContent='Đang gửi lệnh…';updateButtons();
 try{
  var body=new URLSearchParams({action:action,wheel:String(wheel||0),direction:'1',fixture:el('fixture').checked?'yes':'no',surface:el('floor').checked?'floor':'no',session:String(session),token:String(sessionToken),stop_revision:String(data?data.control_revision:-1)});
  var r=await fetch('/operate',{method:'POST',body:body,signal:AbortSignal.timeout(3000)}),answer=await r.json();
  if(!r.ok)throw Error(answer.message);
  el('message').textContent=answer.message;
  if(action==='arm'||action==='pulse'){
   pending={state:action==='arm'?3:7,until:Date.now()+2000};
   if(action==='pulse')pulse={wheel:wheel,startL:row[9],startR:row[10],peak:0,observed:false};
  }
 }catch(e){if(action==='calib')calibWaitUntil=0;el('message').textContent='Chưa thực hiện được: '+e.message;}
 finally{busy=false;updateButtons();}
}
async function clearLog(){if(!confirm('Chỉ xóa khi bạn đã lưu file TXT. Xóa log cũ?'))return;try{var r=await fetch('/balance-clear',{method:'POST'});if(!r.ok)throw Error(await r.text());plotHistory=[];el('message').textContent='Đã xóa log. Sẵn sàng lần thử mới.';}catch(e){el('message').textContent=e.message;}}
function connect(){ws=new WebSocket('ws://'+location.hostname+':81/');ws.onopen=function(){wsOK=false;};ws.onmessage=function(e){try{var h=JSON.parse(e.data);if(Number.isInteger(h.session)&&Number.isInteger(h.token)&&h.token>0){session=h.session;sessionToken=h.token;wsOK=true;if(!document.hidden)ws.send('$PING*');}}catch(ignore){}};ws.onclose=function(){cancelArmWait('Đã hủy chờ vì mất kênh lệnh.');wsOK=false;session=-1;sessionToken=0;setTimeout(connect,1500);};ws.onerror=function(){};}
setInterval(function(){if(wsOK&&!document.hidden)ws.send('$PING*');updateButtons();},200);
document.addEventListener('visibilitychange',function(){if(document.hidden){cancelArmWait('Đã hủy chờ vì trang bị ẩn.');if(wsOK)ws.send('$BACKGROUND*');}});
window.addEventListener('pagehide',function(){cancelArmWait('Đã hủy chờ vì rời trang.');if(wsOK)ws.send('$LEAVE*');});
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
  var a=d.row.split(',').map(Number);if(a.length!==27||!a.every(Number.isFinite))throw Error('Dữ liệu STM32 không hợp lệ');row=a;lastFresh=Date.now()-d.age_ms;
  el('link').textContent=wsOK?'Đã kết nối STM32 + web':'STM32 OK · mất kênh lệnh';
  el('status').textContent=a[4]===3?'ĐANG CHẠY CÂN BẰNG':a[4]===7?'ĐANG THỬ BÁNH · 1,5 GIÂY':'MOTOR ĐANG DỪNG';
  el('reason').textContent=isRunning()?(ownsControl()?'Trang này đang điều khiển xe. Giữ trang sáng và ở phía trước; chuyển ứng dụng sẽ DỪNG.':'Trang chỉ xem. Việc rời tab này không dừng xe; nút DỪNG vẫn dùng được.'):(blockers().join(' ')||faultText(a[6])||'Xe không tự chạy khi cắm pin. Chọn một trong hai mục bên dưới.');
  if(pending&&a[4]===pending.state){el('message').textContent=a[4]===3?'ĐANG CHẠY CÂN BẰNG · thả nhẹ tay khỏi thân xe; quan sát và bấm DỪNG khi cần.':'STM32 đã nhận chế độ và đang điều khiển motor.';pending=null;}
  if(pending&&Date.now()>pending.until){el('message').textContent='Đã gửi lệnh nhưng chưa thấy STM32 chuyển chế độ. '+(armBlockers().join(' ')||'Kiểm tra dây UART từ ESP32 TX đến STM32 RX.');pending=null;}
  if(pulse&&a[4]===7){pulse.observed=true;pulse.peak=Math.max(pulse.peak,Math.abs(a[7]),Math.abs(a[8]));el('pulse-result').textContent='STM32 đã xuất PWM '+a[7]+' / '+a[8]+'. Encoder đang thay đổi: '+Math.abs(a[9]-pulse.startL)+' / '+Math.abs(a[10]-pulse.startR)+' ticks.';}
  if(previousState===7&&a[4]!==7&&pulse){var dl=Math.abs(a[9]-pulse.startL),dr=Math.abs(a[10]-pulse.startR);var moved=(pulse.wheel===1?dl>=4:pulse.wheel===2?dr>=4:dl>=4&&dr>=4);
   el('pulse-result').textContent=(a[6]&~128?'Bài thử đã dừng: '+faultText(a[6]):'Đã kết thúc bài thử.')+'\nPWM lớn nhất đã quan sát: '+pulse.peak+'; encoder L/R: '+dl+' / '+dr+' ticks.\n'+(moved?'Encoder đã ghi nhận bánh quay. Đối chiếu chiều quay bằng mắt và tải log.':'Chưa thấy đủ xung encoder. Nếu PWM đã có mà bánh không quay: kiểm tra nguồn motor/driver và dây motor; nếu bánh quay nhưng ticks không đổi: kiểm tra encoder.');pulse=null;}
  if((previousState===3||previousState===7)&&!isRunning()){endedAt=Date.now();el('message').textContent='Motor đã dừng theo dữ liệu STM32. Chờ 2 giây rồi tải log.';}previousState=a[4];
  el('pitch').textContent=a[17].toFixed(2);el('speed').textContent=((a[19]+a[20])/2).toFixed(3);el('distance').textContent=(a[21]*100).toFixed(1);el('pwm').textContent=a[7]+' / '+a[8];
  if(lastSeq!==a[0]){plotHistory.push([a[17]/10,a[21]/.1]);if(plotHistory.length>300)plotHistory.shift();lastSeq=a[0];draw();}
  var m=d.meta.split(','),assist=wheelAssist(a,m);
  el('wheel-difference').textContent=assist?(100*assist.delta).toFixed(2):'—';
  el('heading-assist').textContent=assist?assist.pwm.toFixed(0):'—';
  el('integral').textContent=a[25].toFixed(3);el('theta-target').textContent=a[23].toFixed(2);
  var catchGain=1+.75*Math.max(0,Math.min(1,(Math.abs(a[17])-3)/3));
  el('capture-state').textContent=a[4]===3?(catchGain>1?'ĐANG BẮT LẠI · phản hồi góc ×'+catchGain.toFixed(2):'CÂN BẰNG THƯỜNG · điều khiển R7/R9'):'CHƯA CHẠY';
  detailRows([
   ['STM32',m[1]||'chờ thông tin phiên bản'],['Gyro Y đã trừ bias',a[18].toFixed(3)+' °/s'],['Vận tốc L / R',a[19].toFixed(4)+' / '+a[20].toFixed(4)+' m/s'],['Encoder thô L / R',a[9]+' / '+a[10]+' ticks'],
   ['Accel thô XYZ',a.slice(14,17).map(function(v){return(v/4096).toFixed(4);}).join(' / ')+' g'],['Gyro thô XYZ',a.slice(11,14).map(function(v){return(v/16.4).toFixed(3);}).join(' / ')+' °/s'],
   ['PID góc nền Kp / Kd',m.slice(2,4).join(' / ')],['Hệ số bắt lại / Kp / Kd hiệu dụng',catchGain.toFixed(3)+' / '+(Number(m[2])*catchGain).toFixed(2)+' / '+(Number(m[3])*catchGain).toFixed(2)],['PI vận tốc Kp / Ki',m.slice(4,6).join(' / ')],['Kp vị trí / PPR',m[6]+' / '+m[7]],['Vận tốc / góc mục tiêu',a[22].toFixed(4)+' m/s / '+a[23].toFixed(3)+'°'],['PWM cơ sở / tích phân',a[24].toFixed(2)+' / '+a[25].toFixed(4)+'°'],
   ['Chu kỳ / thời gian xử lý',a[2]+' / '+a[3]+' µs'],['Mất mẫu tại STM32',a[26]],['Điện áp / dòng motor','Chưa đo']]);
  startupGuide();serviceArmWait();el('log-status').textContent=d.samples+' / '+d.capacity+' mẫu · ghi 100 Hz: thêm tối đa 1 giây trước bật và 2 giây sau dừng.\nCRC lỗi: '+d.crc+' · thiếu gói: '+d.gaps+' · bỏ do đầy: '+d.overflow+' · STM32 reset: '+d.resets+(d.overflow?' — LOG KHÔNG ĐẦY ĐỦ':'');updateButtons();
 }catch(e){cancelArmWait('Đã hủy chờ vì lỗi kết nối/dữ liệu.');el('link').textContent='Mất dữ liệu mới';el('reason').textContent='Không lấy được dữ liệu: '+e.message;updateButtons();}
 finally{polling=false;}
}
connect();poll();setInterval(poll,200);
</script></body></html>
)dash";
