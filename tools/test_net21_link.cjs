// NET21: a silent (half-open) command channel must be detected, the retry must
// back off instead of hammering, and an ESP32 restart must be reported as such
// rather than as "STM32 stopped sending".
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync('ESP32_S3_Gateway/dashboard_html.h','utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];

let now=100000,seq=1,timers=[],intervals=[];
function setTimeout(fn,ms){const id=seq++;timers.push({id,at:now+ms,fn});return id;}
function clearTimeout(id){timers=timers.filter(t=>t.id!==id);}
function setInterval(fn,ms){const id=seq++;intervals.push({id,fn,ms});return id;}
function advance(ms){const end=now+ms;for(;;){timers.sort((a,b)=>a.at-b.at);const t=timers[0];if(!t||t.at>end)break;timers.shift();now=t.at;t.fn();}now=end;}
const sockets=[];
class FakeWS{
 constructor(url){this.url=url;this.readyState=0;this.bufferedAmount=0;this.sent=[];this.closed=false;sockets.push(this);}
 send(s){this.sent.push(s);}
 close(){if(this.closed)return;this.closed=true;this.readyState=3;if(this.onclose)this.onclose();}
}
const elements={};
function element(){return {style:{},dataset:{},classList:{toggle(){}},addEventListener(k,f){this['on'+k]=f;},checked:false,disabled:false,hidden:false,textContent:'',children:[],appendChild(x){this.children.push(x);},replaceChildren(){this.children=[];},getContext(){return {clearRect(){},beginPath(){},moveTo(){},lineTo(){},stroke(){}};}};}
const row=[1,1000,5000,500,3,1,0,0,0,0,0,-8,-3,8,0,0,4096,.1,0,0,0,0,0,0,0,0,0,12,0,.3,0,.8,1,1,0,0,0,0,0,0];
const meta='$BINFO,BAL-20261120-R12-8DIR-RT43-MAX,9.150813,6.128855,14.327945,1.160484,0.25,898.225,160,200,200,140,-.46,-.18,.46,2.166942649,1,12,.998,60,0,0,0,0,1.940,1';
let data={seen:true,age_ms:0,row:row.join(','),meta,samples:100,capacity:18000,crc:0,gaps:0,overflow:0,resets:0,owner:0,owner_token:7,stop_source:'none',control_revision:17,drive_epoch:1,boot_id:5,uptime_ms:60000};
let net={boot_id:5,reset_reason:'brownout',uptime_ms:60000,heap_free:200000,heap_min:197568,ap_clients:1,ap_leaves:0,ap_reason:0,loop_gap_max_ms:12,archive_ready:false,archive_error:'filesystem_unavailable_preserved',archive_saved:0};
let statusCalls=0,archiveCalls=0;
const ctx={document:{querySelectorAll(){return [['drive-forward',1,0],['drive-left',0,1]].map(([id,d,t])=>{let b=elements[id]||=element();b.dataset={drive:String(d),turn:String(t)};return b;});},get hidden(){return false;},getElementById(id){return elements[id]||=element();},createElement:element,addEventListener(){}},window:{addEventListener(){}},Date:{now(){return now;}},Math,Number,URLSearchParams,AbortSignal,location:{hostname:'localhost'},console,confirm:()=>true,setInterval,setTimeout,clearTimeout,WebSocket:FakeWS,
 fetch:async(url,opts)=>{
  if(url==='/balance-status'){statusCalls++;return {ok:true,json:async()=>data};}
  if(url==='/network-status'){return {ok:true,json:async()=>net};}
  if(url==='/balance-archives'){archiveCalls++;return {ok:true,json:async()=>({files:[]})};}
  if(url==='/hardware-status'){return {ok:true,json:async()=>({balance_live:false,seen:false,age_ms:9999})};}
  return {ok:true,json:async()=>({message:'sent'}),text:async()=>''};
 }};
vm.createContext(ctx);vm.runInContext(script,ctx);

const hello=()=>sockets.at(-1).onmessage({data:JSON.stringify({session:0,token:7})});
(async()=>{
 assert.equal(typeof ctx.wsAlive,'function','page must expose a liveness check');
 assert.equal(sockets.length,1);
 hello();
 assert.equal(ctx.wsOK,true);
 assert.equal(ctx.wsDelay,500,'first retry delay');

 // 2.9 s of silence is tolerated, 3.1 s must tear the socket down.
 advance(2900);ctx.wsAlive();assert.equal(sockets[0].closed,false);
 advance(200);ctx.wsAlive();assert.equal(sockets[0].closed,true,'silent socket must close');
 assert.equal(ctx.wsDelay,1000,'retry delay doubles after a failed attempt');
 const before=statusCalls;
 advance(500);assert.equal(sockets.length,2,'reconnect after 500 ms');
 assert.equal(statusCalls,before,'reconnect alone must not poll; hello triggers the refresh');

 // No hello: the 2 s handshake deadline closes the socket, delay keeps growing.
 advance(2000);assert.equal(sockets[1].closed,true);
 advance(1000);assert.equal(sockets.length,3);assert.equal(ctx.wsDelay,2000);
 advance(2000);assert.equal(sockets[2].closed,true);assert.equal(ctx.wsDelay,4000);
 advance(2000);assert.equal(sockets.length,4,'retry uses the 2 s delay');
 advance(2000);assert.equal(sockets[3].closed,true);
 advance(4000);assert.equal(sockets.length,5,'retry delay is capped at 4 s');
 assert.equal(ctx.wsDelay,4000);
 hello();assert.equal(ctx.wsDelay,500,'a live channel resets the backoff');
 assert.equal(ctx.session,0);

 // A heartbeat frame proves the gateway is still there.
 advance(2000);sockets.at(-1).onmessage({data:'{"hb":23000}'});
 advance(2500);ctx.wsAlive();assert.equal(sockets.at(-1).closed,false,'heartbeat keeps the channel');
 advance(600);ctx.wsAlive();assert.equal(sockets.at(-1).closed,true);

 // An ESP32 restart is reported as a gateway restart, not as a silent STM32.
 await ctx.pollNetwork();
 ctx.driveHeld=1;ctx.driveTurn=0;
 net.boot_id=6;await ctx.pollNetwork();
 assert.match(ctx.el('message').textContent,/khởi động lại/i);
 assert.equal(ctx.driveHeld,0,'a restart must drop the held gesture');
 assert.ok(ctx.rebootAt>0);
 data.seen=false;data.age_ms=5000;data.boot_id=6;
 await ctx.poll();
 assert.match(ctx.el('status').textContent,/khởi động lại/i);
 assert.doesNotMatch(ctx.el('status').textContent,/Chưa nhận STM32 đang hoạt động/);
 advance(13000);await ctx.poll();
 assert.match(ctx.el('status').textContent,/Chưa nhận STM32 đang hoạt động/,'the note expires after 12 s');
 console.log('PASS NET21 link: liveness, capped backoff, hello refresh, restart reported as gateway event.');
})().catch(e=>{console.error(e);process.exitCode=1;});
