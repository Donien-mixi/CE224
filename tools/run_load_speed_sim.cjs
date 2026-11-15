// Diagnostic plant only: assumptions about loaded torque/contact are unvalidated.
// Builds a temporary adapter from the shipped motion/controller + LOADTEST helper.
// Leaves all production simulation DLLs and vehicle parameters unchanged.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');
const root=path.resolve(__dirname,'..');process.chdir(root);
const dir=path.join(root,'.build/load_speed');fs.mkdirSync(dir,{recursive:true});
const posix=p=>p.replace(/\\/g,'/');
let source=fs.readFileSync('Simulation/current/motion_native.c','utf8');
assert(source.includes(' int valid=BalanceMotion8_Update'));
source='#include <stdio.h>\n#include "load_speed_test.h"\nstatic LoadSpeedTest trial;\nstatic int previous_left,previous_right;\n'+source;
source=source.replace(' int valid=BalanceMotion8_Update',String.raw`
 uint32_t word=(uint32_t)in[1];float left=in[5]-in[9]*.131f,right=in[5]+in[9]*.131f;
 unsigned old_phase=trial.phase;
 LoadTest_Reserve(&trial,previous_left,previous_right,(uint32_t)in[6]);
 LoadTest_Update(&trial,(word&3U)==1,(word>>2)>0U&&(word!=s.last_word||(uint32_t)(in[6]-s.last_ms)<350U),true,in[0]!=0,s.ready,in[3],in[4],left,right,in[2],s.v_ref,s.requested==0&&t.requested==0,(uint32_t)in[6]);
 if(trial.phase!=1)word&=~3U;
 if(old_phase!=trial.phase)fprintf(stderr,"LOADTEST t%.3f phase%u reason%u\n",in[6]/1000,trial.phase,trial.reason);
 int valid=BalanceMotion8_Update`);
source=source.replace('in[0]!=0,(uint32_t)in[1],','in[0]!=0,word,');
source=source.replace(' double a[29]=',' if(trial.phase==1&&(s.cancel_flags||in[0]==0))LoadTest_End(&trial,4,(uint32_t)in[6]);\n s.flags|=LoadTest_Flags(&trial);\n double a[29]=');
source=source.replace('study_speed=(float)in[0];','trial=(LoadSpeedTest){0};previous_left=previous_right=0;study_speed=(float)in[0];');
source=source.replace('out[CAT_N+3]=o[0];','previous_left=(int)o[0];previous_right=(int)o[1];out[CAT_N+3]=o[0];');
source+='\n'+fs.readFileSync('Simulation/current/recovery_native.c','utf8').replace('BalanceRecovery_Speed(&s,.8f)','(LoadTest_Guard(&trial)?LOAD_TEST_SPEED_LIMIT:BalanceRecovery_Speed(&s,.8f))');
fs.writeFileSync(path.join(dir,'sim.c'),source);
const dll=posix(path.join(dir,'sim.dll'));
const cli=String.raw`
here="${posix(path.join(root,'Simulation/current'))}/";
exec(here+"cases.sce",-1);
link("${dll}",["twip_motion_reference";"twip_motion_control";"twip_motion_set_speed";"twip_motion_impact";"twip_motion_cancel_fault";"twip_push_guard"],"c");
link(here+"contact_native.dll","twip_pair_catalog_contact","c");
motor_cases=catalog_nominal_cases(ones(1,3),:);motor_cases(1,14)=12;motor_cases(2,14)=10.8;
function [Pc,Sc]=motor_override(Pc,Sc,ic)
 r=call("twip_motion_set_speed",.95,1,"d","out",[1,1],2,"d");
 Sc.push_recovery=%t;Sc.push_trace_path=out+msprintf("/guard%02d.csv",ic);Sc.motion_plan=[5 13 1];
 if ic==3 then Sc.motion_freeze_at=8;end
endfunction
motor_case_ids=1:3;motor_native=%t;motor_cli=%f;motor_duration=17;motor_substeps=128;
motor_settings=[500 1 150 .1 5 .006 3 .01 .5 1 .25 1];motor_stage="rt9_load1_feasibility";
exec(here+"study_run.sce",-1);
mputl(out,"${posix(path.join(dir,'sim_path.txt'))}");
quit;
`;
fs.writeFileSync(path.join(dir,'run.sce'),cli);
if(!process.argv.includes('--run')){console.log('Prepared diagnostic adapter; pass --run to compile/run.');process.exit(0);}
let r=cp.spawnSync(process.env.CC||'C:/msys64/ucrt64/bin/gcc.exe',['-O2','-shared','-static-libgcc','-IXe_Can_Bang/Core/Inc',path.join(dir,'sim.c'),'-o',dll,'-lm'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(process.env.SCILAB_CLI||'C:/Program Files/scilab-2026.1.0/bin/Scilex.exe',['-nb','-f',path.join(dir,'run.sce')],{encoding:'utf8'});
process.stdout.write(r.stdout);process.stderr.write(r.stderr);assert.equal(r.status,0);
const result=fs.readFileSync(path.join(dir,'sim_path.txt'),'utf8').trim();
assert.equal(fs.readFileSync(path.join(result,'run_status.txt'),'utf8').trim(),'status=completed');
const summary=fs.readFileSync(path.join(result,'summary.csv'),'utf8').trim().split(/\r?\n/).map(l=>l.split(',').map(Number));
assert.equal(summary.length,3);
for(let i=0;i<3;i++){
 assert.equal(summary[i][2],17,'model stopped before17s');
 const rows=fs.readFileSync(path.join(result,`guard0${i+1}.csv.motion.csv`),'utf8').trim().split(/\r?\n/).map(l=>l.split(',').map(Number));
 assert(rows.some(r=>r[3]&4096),'trial never started');
 assert(rows.some(r=>r[3]&8192),'trial never braked');
 assert(!(rows.at(-1)[3]&12288),'trial did not finish braking');
 assert(Math.abs(rows.at(-1)[2])<.01,'reference did not settle');
}
console.log('PASS3 diagnostic cases:12V,10.8V and lost commands; each starts/brakes/settles. Loaded plant is NOT hardware validation. '+result);
