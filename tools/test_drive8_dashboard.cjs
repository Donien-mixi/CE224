// Exercise both controller selections in the shipped UI with network/DOM mocks.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync('ESP32_S3_Gateway/dashboard_html.h','utf8');
const elements={},requests=[],events={},sent=[];let hidden=false;
function element(){return {style:{},dataset:{},classList:{toggle(){}},addEventListener(k,f){this['on'+k]=f;},checked:false,disabled:false,hidden:false,textContent:'',children:[],appendChild(x){this.children.push(x);},replaceChildren(){this.children=[];},getContext(){return {clearRect(){},beginPath(){},moveTo(){},lineTo(){},stroke(){}};}};}
const row=[1,1000,5000,500,2,1,0,0,0,0,0,-8,-3,8,0,0,4096,.1,0,0,0,0,0,0,0,0,0,12,0,.3,0,0,1,1,0,0,0,0,0,0];
assert(html.includes('R12 · Cấu hình chính thức'));assert(!html.includes('profile-r7'));assert(!html.includes("operate('profile'"));
const meta='$BINFO,BAL-20261024-R12-8DIR-RT8,9.150813,6.128855,14.327945,1.160484,0.25,898.225,160,200,200,140,-.46,-.18,.46,2.166942649,1,12,.998,60';
const data={seen:true,age_ms:0,row:row.join(','),meta,samples:100,capacity:18000,crc:0,gaps:0,overflow:0,resets:0,owner:0,owner_token:123,stop_source:'none',control_revision:17,drive_epoch:1};
const ctx={document:{querySelectorAll(){return [['drive-forward-left',1,1],['drive-forward',1,0],['drive-forward-right',1,-1],['drive-left',0,1],['drive-right',0,-1],['drive-reverse-left',-1,-1],['drive-reverse',-1,0],['drive-reverse-right',-1,1]].map(([id,d,t])=>{let b=elements[id]||=element();b.dataset={drive:String(d),turn:String(t)};return b;});},get hidden(){return hidden;},getElementById(id){return elements[id]||=element();},createElement:element,addEventListener(k,f){events[k]=f;}},window:{addEventListener(k,f){events[k]=f;}},Date,Math,Number,URLSearchParams,AbortSignal,location:{hostname:'localhost'},console,confirm:()=>true,setInterval(){},setTimeout(){},clearTimeout(){},WebSocket:class{send(s){sent.push(s);}},fetch:async(url,opts)=>{requests.push({url,opts});return {ok:true,json:async()=>url==='/balance-status'?data:{message:'sent'},text:async()=>''};}};
vm.createContext(ctx);vm.runInContext(html.match(/<script>([\s\S]*?)<\/script>/)[1],ctx);
async function run(){
 await new Promise(r=>setImmediate(r));ctx.ws.onmessage({data:JSON.stringify({session:0,token:123})});await ctx.poll();
 elements.floor.checked=true;elements.fixture.checked=true;ctx.updateButtons();assert(!elements.arm.disabled);
 assert.equal(ctx.armAngleLimit(),60);
 row[17]=9.5;data.row=row.join(',');await ctx.poll();assert(!ctx.armBlockers().some(x=>x.includes('Góc hiện tại')));
 row[17]=-60.1;data.row=row.join(',');await ctx.poll();assert(ctx.armBlockers().some(x=>x.includes('±60')));row[17]=.1;data.row=row.join(',');await ctx.poll();
 const a=row.slice();a[4]=3;a[7]=220;a[8]=240;a[9]=100;a[10]=-200;
 assert.equal(ctx.wheelAssist(a,meta.split(',')).pwm,-10);
 assert(Math.abs(ctx.wheelAssist(a,meta.split(',')).delta-100*.204203522/898.225)<1e-10);
 a[4]=7;assert.equal(ctx.wheelAssist(a,meta.split(',')),null);
 data.meta=meta.replace('20261024-R12-8DIR-RT8','20261011-R7');await ctx.poll();assert(elements.arm.disabled&&elements.left.disabled);
 data.meta=meta;data.age_ms=900;await ctx.poll();assert(elements.arm.disabled);data.age_ms=0;
 row[6]=128;data.row=row.join(',');await ctx.poll();assert(elements.arm.disabled);row[6]=0;
 row[4]=3;row[7]=210;row[8]=260;row[23]=1.4;row[25]=.02;data.row=row.join(',');await ctx.poll();
 assert(elements.arm.disabled&&elements.download.disabled);assert.equal(elements.integral.textContent,'0.020');assert.equal(elements['theta-target'].textContent,'1.40');
 row[28]=2;data.row=row.join(',');await ctx.poll();assert.match(elements.status.textContent,/LẤY LẠI THĂNG BẰNG/);
 assert(elements.details.children.some(tr=>tr.children.some(td=>String(td.textContent).includes('phục hồi tối đa4s'))));
 assert.match(ctx.faultText(4096),/Phục hồi\/hủy chuyển động/);
 row[28]=1;data.row=row.join(',');await ctx.poll();assert.match(elements.status.textContent,/BẮT THĂNG BẰNG/);
 row[28]=0;data.row=row.join(',');await ctx.poll();assert.match(elements.status.textContent,/CHẠY CÂN BẰNG/);
 row[4]=2;row[5]=0;row[7]=row[8]=0;data.row=row.join(',');await ctx.poll();ctx.selectMode('balance');
 let before=requests.filter(x=>x.url==='/operate').length;ctx.beginArm();assert(ctx.armWaitUntil);assert.equal(requests.filter(x=>x.url==='/operate').length,before);
 hidden=true;events.visibilitychange();assert.equal(ctx.armWaitUntil,0);assert.equal(sent.at(-1),'$BACKGROUND*');hidden=false;
 ctx.beginArm();row[5]=1;data.row=row.join(',');await ctx.poll();await new Promise(r=>setImmediate(r));
 assert.equal(ctx.armWaitUntil,0);assert(ctx.pending&&ctx.pending.state===3);assert.equal(requests.at(-1).opts.body.get('token'),'123');assert.equal(requests.at(-1).opts.body.get('stop_revision'),'17');
 await ctx.operate('stop');assert.equal(sent.at(-1),'$STOP*');
 let details=elements.details.children.map(tr=>tr.children.map(td=>td.textContent).join(' ')).join('\n');assert.match(details,/Gain lực Kx/);assert.match(details,/N·m/);assert(!details.includes('PID góc'));
 // Obsolete UI calls cannot request switching or ARM R7.
 data.samples=0;ctx.endedAt=0;row[4]=2;data.row=row.join(',');await ctx.poll();
 const oldRequests=requests.length;await ctx.operate('profile',7);assert.equal(requests.length,oldRequests);
 data.meta=meta.replace(',12,.998,60',',7,.980,3');await ctx.poll();assert(elements.arm.disabled&&elements.left.disabled);
 data.meta=meta;await ctx.poll();assert(!elements.arm.disabled);
 assert.match(elements['profile-status'].textContent,/XÁC NHẬN R12/);
 // Actual held-button functions: no ARM side effect, independent0 release.
 ctx.selectMode('balance');row[4]=3;row[28]=0;row[32]=1;row[33]=1;data.row=row.join(',');data.drive_epoch=1;await ctx.poll();
 assert(ctx.canDrive());const arms=requests.filter(x=>x.url==='/operate').length;
 data.samples=data.capacity;await ctx.poll();assert(ctx.canDrive()&&!elements['drive-forward'].disabled);data.samples=100;await ctx.poll();
 ctx.pressDrive(1,0);assert.match(sent.at(-1),/^\$MOVE8,1,0,1,\d+,0\*$/);assert.equal(requests.filter(x=>x.url==='/operate').length,arms);
 ctx.releaseDrive();assert.match(sent.at(-1),/^\$MOVE8,0,0,1,\d+,0\*$/);
 assert.match(elements['drive-timing'].textContent,/ĐÃ NHẢ/);
 assert.match(elements['drive-timing'].textContent,/Bánh trái \/ phải/);
 ctx.pressDrive(-1,0);assert.match(sent.at(-1),/^\$MOVE8,-1,0,1,\d+,0\*$/);events.blur();assert.equal(ctx.driveHeld,0);assert.match(sent.at(-1),/^\$MOVE8,0,0,/);
 ctx.selectDriveSpeed(1);assert.equal(ctx.driveHeld,0);ctx.pressDrive(1,0);assert.match(sent.at(-1),/^\$MOVE8,1,0,1,\d+,1\*$/);
 ctx.selectDriveSpeed(0);assert.equal(ctx.driveHeld,0);assert.match(sent.at(-1),/^\$MOVE8,0,0,1,\d+,0\*$/);
 for(let d=-1;d<=1;d++)for(let turn=-1;turn<=1;turn++){
  if(!d&&!turn)continue;ctx.releaseDrive();ctx.pressDrive(d,turn);
  assert(sent.at(-1).startsWith('$MOVE8,'+d+','+turn+',1,'));ctx.releaseDrive();
  assert.equal(ctx.driveTurn,0);
 }
 assert.match(ctx.faultText(8192),/DỪNG MOTOR/);
 row[33]=64;data.row=row.join(',');await ctx.poll();assert(!ctx.canDrive());assert.match(elements['drive-status'].textContent,/hủy chuyển động cũ/);
 row[33]=65;data.row=row.join(',');await ctx.poll();assert(ctx.canDrive());row[33]=1;data.row=row.join(',');await ctx.poll();
 ctx.pressDrive(1,0);events.pointercancel();assert.equal(ctx.driveHeld,0);
 const keyEvent={key:'Enter',repeat:false,preventDefault(){},currentTarget:{setPointerCapture(){throw Error('Keyboard cannot capture a pointer');}}};
 elements['drive-forward'].onkeydown(keyEvent);assert.equal(ctx.driveHeld,1);elements['drive-forward'].onkeyup(keyEvent);assert.equal(ctx.driveHeld,0);
 ctx.releaseDrive();ctx.pressDrive(1,0);ctx.pressDrive(-1,0);assert.equal(ctx.driveHeld,1);ctx.releaseDrive(); // no conflicting held buttons
 row[28]=1;data.row=row.join(',');await ctx.poll();ctx.pressDrive(1,0);assert.equal(ctx.driveHeld,0);assert(elements['drive-forward'].disabled);
 row[28]=0;row[32]=0;data.row=row.join(',');await ctx.poll();assert(!ctx.canDrive()); // stale epoch
 row[32]=1;data.row=row.join(',');data.owner=1;await ctx.poll();assert(!ctx.canDrive());data.owner=0;await ctx.poll();
 ctx.pressDrive(1,0);hidden=true;events.visibilitychange();assert.equal(ctx.driveHeld,0);assert.equal(sent.at(-1),'$BACKGROUND*');hidden=false;
 ctx.releaseDrive();row[4]=3;row[32]=1;row[33]=1;data.drive_epoch=1;data.row=row.join(',');await ctx.poll();
 const ptr={pointerId:7,button:0,isPrimary:true,preventDefault(){},currentTarget:{setPointerCapture(){}}};
 ctx.pressDrive(1,0,ptr);assert.equal(ctx.driveHeld,1);assert.equal(ctx.drivePointer,7);
 ctx.document.elementFromPoint=()=>({closest:()=>elements['drive-forward-left']});
 elements['drive-forward'].onpointermove({pointerId:7,clientX:20,clientY:20});assert.equal(ctx.driveTurn,1);assert(sent.at(-1).startsWith('$MOVE8,1,1,'));
 events.pointerup({pointerId:8});assert.equal(ctx.drivePointer,7); // another finger cannot release the owner
 row[38]=ctx.driveSequence;row[39]=row[1]-20;data.row=row.join(',');await ctx.poll();assert(ctx.driveAckMs!==null&&ctx.driveAckMs>=0);
 events.pointerup({pointerId:7});assert.equal(ctx.driveHeld,0);assert.equal(ctx.driveTurn,0);
 // Release does not wait for a status poll that has become stale.
 ctx.pressDrive(-1,0,ptr);data.age_ms=900;const staleRelease=sent.length;
 ctx.releaseDrive();assert.equal(sent.length,staleRelease+1);assert.match(sent.at(-1),/^\$MOVE8,0,0,/);
 data.age_ms=0;await ctx.poll();
 // A focused button may change before keyup; the document still releases it.
 elements['drive-left'].onkeydown(keyEvent);assert.equal(ctx.driveTurn,1);
 events.keyup({key:'Enter'});assert.equal(ctx.driveTurn,0);
 // No growing queue of held commands on a slow socket. Append one neutral,
 // then let the350ms STM32 lease guard a transport that cannot make progress.
 ctx.pressDrive(1,0,ptr);ctx.ws.bufferedAmount=200;const queued=sent.length;
 for(let i=0;i<100;i++)ctx.driveTick();assert.equal(sent.length,queued);
 ctx.releaseDrive();assert.equal(sent.length,queued+1);assert.match(sent.at(-1),/^\$MOVE8,0,0,/);
 for(let i=0;i<100;i++)ctx.driveTick();assert.equal(sent.length,queued+1);
 ctx.ws.bufferedAmount=0;await ctx.poll();
 // Lost pointerup cannot leave the next primary physical press blocked.
 const p2={...ptr,pointerId:9};ctx.pressDrive(1,0,ptr);ctx.pressDrive(-1,0,p2);
 assert.equal(ctx.driveHeld,-1);assert.equal(ctx.drivePointer,9);
 // iOS all-touch cancellation releases; a second finger lifting does not.
 events.touchend({touches:[{}]});assert.equal(ctx.driveHeld,-1);
 events.touchcancel({touches:[]});assert.equal(ctx.driveHeld,0);assert.match(sent.at(-1),/^\$MOVE8,0,0,/);
 // Old moving telemetry cannot label a physically released gesture as held.
 ctx.pressDrive(1,0,ptr);row[31]=.2;row[34]=1;data.row=row.join(',');await ctx.poll();
 ctx.releaseDrive();assert.match(elements['drive-status'].textContent,/ĐÃ NHẢ NÚT/);
 row[31]=0;row[34]=0;data.row=row.join(',');await ctx.poll();
 row[4]=2;row[6]=0;row[32]=0;data.row=row.join(',');await ctx.poll();
 await ctx.operate('calib');assert(ctx.pending&&ctx.pending.calibration);
 const beforeCalibArms=requests.length;await ctx.operate('arm');assert.equal(requests.length,beforeCalibArms);
 await ctx.poll();assert(ctx.pending&&ctx.pending.calibration); // pre-calibration standby is not completion
 row[1]+=5000;row[0]++;row[5]=0;data.row=row.join(',');await ctx.poll();assert.equal(ctx.pending,null);
 // NET1: power/reboot evidence and archive links are visible even with RAM0.
 const originalFetch=ctx.fetch;
 const health={boot_id:7,reset_reason:'power_on',uptime_ms:1000,ap_clients:1,ap_leaves:0,ap_reason:0,heap_free:120000,heap_min:110000,loop_gap_max_ms:5,archive_ready:true,archive_saved:50};
 ctx.fetch=async(url,opts)=>url==='/network-status'?{ok:true,json:async()=>health}:url==='/balance-archives'?{ok:true,json:async()=>({files:[{name:'b00000006_g0000.bin',samples:123,partial:true,current:false}]})}:originalFetch(url,opts);
 await ctx.pollNetwork();await ctx.pollNetwork();assert.match(elements['network-health'].textContent,/phiên #7/);assert.equal(elements['archive-list'].children.length,1);
 assert.match(elements['archive-list'].children[0].children[0].textContent,/123 mẫu/);
 const armCount=requests.filter(x=>x.url==='/operate').length;ctx.driveHeld=1;ctx.driveTurn=1;ctx.drivePointer=7;
 health.boot_id=8;health.reset_reason='brownout';await ctx.pollNetwork();assert.equal(ctx.driveHeld,0);assert.equal(ctx.driveTurn,0);assert.match(elements.message.textContent,/ĐÃ KHỞI ĐỘNG LẠI/);assert.match(elements['network-health'].textContent,/SỤT ÁP/);assert.equal(requests.filter(x=>x.url==='/operate').length,armCount);
 ctx.fetch=originalFetch;
 // A hung socket retries without retaining/replaying a held gesture. An old
 // socket closing cannot invalidate the newly opened session.
 const scheduled=[];ctx.setTimeout=(fn,ms)=>{scheduled.push({fn,ms});return scheduled.length;};ctx.clearTimeout=()=>{};
 const previousSocket=ctx.ws;ctx.wsOK=false;ctx.connect();const activeSocket=ctx.ws;
 activeSocket.close=()=>activeSocket.onclose();ctx.driveHeld=1;scheduled[0].fn();assert.equal(ctx.driveHeld,0);assert.equal(scheduled.at(-1).ms,1000);
 activeSocket.onmessage({data:JSON.stringify({session:0,token:123})});assert(ctx.wsOK);previousSocket.onclose();assert(ctx.wsOK&&ctx.ws===activeSocket);
 console.log('PASS 8DIR/NET1 UI: held/release, reconnect without replay, boot/brownout diagnostics, RAM0 archive links, calibration acknowledgment and no automatic ARM.');
}
run().catch(e=>{console.error(e);process.exitCode=1;});
