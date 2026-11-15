const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');const root=path.resolve(__dirname,'..');process.chdir(root);const stress=process.argv.includes('--stress'),edge=process.argv.includes('--edge'),prepare=process.argv.includes('--prepare-only'),disturbance=process.argv.includes('--disturbance'),launch=process.argv.includes('--launch'),launch50=process.argv.includes('--launch50');const pushScale=Number(process.env.TWIP_PUSH_SCALE||1);const maxSpeed=Number(process.env.TWIP_SPEED_MAX||0.80);assert(Number.isFinite(maxSpeed)&&maxSpeed>=0.5&&maxSpeed<=1.05,'Invalid TWIP_SPEED_MAX');assert(Number.isFinite(pushScale)&&pushScale>0&&pushScale<=5);const caseIds=process.env.TWIP_CASE_IDS?process.env.TWIP_CASE_IDS.split(',').map(Number):Array.from({length:stress?12:6},(_,i)=>i+1);assert(caseIds.length>0&&new Set(caseIds).size===caseIds.length&&caseIds.every(n=>Number.isInteger(n)&&n>=1&&n<=12),'Invalid case IDs');const label=process.env.TWIP_SIM_LABEL||'r12_current';if(!/^[a-zA-Z0-9_]+$/.test(label))throw Error('Invalid label');const dir='.build/governor_sim_'+label,headerRoot=process.env.TWIP_HEADER_DIR||'Xe_Can_Bang/Core/Inc',headers=path.join(dir,'headers');fs.mkdirSync(headers,{recursive:true});for(const h of fs.readdirSync(headerRoot).filter(x=>x.endsWith('.h')))fs.copyFileSync(path.join(headerRoot,h),path.join(headers,h));fs.copyFileSync(process.env.TWIP_GOVERNOR_HEADER||path.join(headerRoot,'balance_governor.h'),dir+'/balance_governor.h');fs.copyFileSync(dir+'/balance_governor.h',path.join(headers,'balance_governor.h'));let source=fs.readFileSync('Simulation/current/motion_native.c','utf8');source='#define TWIP_RECOVERY_EXTRA 1\n#define TWIP_RUNTIME_SPEED_CEILING (study_speed>.72f?1.12f:.8f)\n#include "balance_governor.h"\nstatic BalanceGovernor bal_governor;\nstatic int previous_l,previous_r;\nstatic int reset_yaw;\n#define BALANCE_MOTION_FILTER(o,t,r,w,p,g) BalanceGovernor_Step(&bal_governor,o,t,r,w,p,g)\n#define BALANCE_MOTION_ANCHOR(r,p,d) BalanceGovernor_Anchor(&bal_governor,&r,p,d)\n'+source;source=source.replace('s.speed_preset=study_speed>.50f;','s.speed_preset=study_speed>.80f?2U:study_speed>.50f;');source=source.replace(' int valid=BalanceMotion8_Update',' BalanceGovernor_Observe(&bal_governor,previous_l,previous_r,in[5]);int old_turn=t.requested;\n#ifdef BALANCE_GOVERNOR_EQUILIBRIUM\n BalanceGovernor_ObserveRest(&bal_governor,in[0]==1 && !((unsigned)in[1]&3U) && in[7]==0 && !s.braking && !t.braking && !s.cancel_flags && fabsf(s.v_ref)<.01f && fabsf(t.w_ref)<.01f && in[10]>.72 && in[10]<1.32,in[3],in[4],in[5],in[9]);\n#endif\n int valid=BalanceMotion8_Update');source=source.replace(' double a[29]=',' if(old_turn*t.requested<0)reset_yaw=1;\n if(bal_governor.limited)s.flags|=16384U;\n double a[29]=');source=source.replace(' int valid=motion_cat_step8_mode',' if(reset_yaw){s[CAT_N]=0;reset_yaw=0;}\n int valid=motion_cat_step8_mode');source=source.replace('out[CAT_N+3]=o[0];','previous_l=(int)o[0];previous_r=(int)o[1];out[CAT_N+3]=o[0];');source+='\nAPI void twip_motion_begin(double *in,double *out){BalanceGovernor_Reset(&bal_governor);previous_l=previous_r=reset_yaw=motion_previous_recovery_active=0;study_speed=(float)in[0];out[0]=study_speed;}\n';source+='\nAPI void twip_motion_governor(double *out){out[0]=bal_governor.acceleration;out[1]=bal_governor.duty;out[2]=0;out[3]=bal_governor.pressure;out[4]=0;\n#ifdef BALANCE_GOVERNOR_EQUILIBRIUM\n out[2]=bal_governor.equilibrium;out[4]=bal_governor.equilibrium_ready;\n#endif\n}\n';source+='\n'+fs.readFileSync('Simulation/current/recovery_native.c','utf8').replace('    BalanceRecovery_Reserve(&s,','    if(BalanceGovernor_ReserveNeeded(in[1],in[2])&&(study_speed<=.8f||fabs(in[7])>1.05))BalanceRecovery_Reserve(&s,').replace('BalanceRecovery_Speed(&s,.8f)','(study_speed>.8f?1.12f:BalanceRecovery_Speed(&s,.8f))');fs.writeFileSync(dir+'/native.c',source);let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-O2','-shared','-static-libgcc','-I'+headers,dir+'/native.c','-o',dir+'/native.dll','-lm'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);const p=root.replaceAll('\\','/');fs.writeFileSync(dir+'/run.sce',`
here="${p}/Simulation/current/";exec(here+"cases.sce",-1);
link("${p}/${dir}/native.dll",["twip_motion_reference";"twip_motion_control";"twip_motion_set_speed";"twip_motion_begin";"twip_motion_impact";"twip_motion_cancel_fault";"twip_push_guard";"twip_recovery_drive_mode";"twip_motion_governor"],"c");
link(here+"contact_native.dll","twip_pair_catalog_contact","c");
motor_cases=catalog_nominal_cases(ones(1,${stress?12:6}),:);motor_cases(:,14)=11.7;motor_cases(6,14)=11.2;
${stress?"motor_cases(7:8,14)=11.2;motor_cases(9,15)=.8;motor_cases(10,1:2)=[.95 1.05];motor_cases(11,5:7)=[.006 .006 4];":""}
${edge?"motor_cases(3:4,13)=2;motor_cases(5:6,13)=-2;motor_cases(2,20)=.003;":""}
${launch?"motor_cases(:,13)=[2;-2;3;-3;4;-4;2;-2;3;-3;4;-4];": ""}
${disturbance?`motor_cases(:,11)=[1;-1;2;-2;3;-3;4;-4;2;-2;3;-3]*${pushScale};motor_cases(:,12)=.08;`:""}
function [Pc,Sc]=motor_override(Pc,Sc,ic)
speed=${maxSpeed};if ic==1 then speed=.5;end;if ic==2 then speed=.72;end
r=call("twip_motion_begin",speed,1,"d","out",[1,1],2,"d");Sc.trial_speed_ceiling=0.8;if speed>0.72 then Sc.trial_speed_ceiling=1.12;end;Sc.governor_trace=%t;Sc.live_recovery_commands=%t;Sc.supervisor_count_position=%t;Sc.push_recovery=%t;Sc.push_trace_path=out+msprintf("/guard%02d.csv",ic);Sc.motion_plan=[3 15 1 0];
if ic==4 | ic==8 then Sc.motion_plan=[3 15 -1 0];end
if ic==5 then Sc.motion_plan=[3 7 1 0;7 11 -1 0;11 15 1 0];end
if ic==6 | ic==12 then Sc.motion_plan=[3 15 1 1];end
${edge?`if ic==1 | ic==2 then Sc.motion_plan=[];end
if ic>=3 & ic<=6 then
 speed=.5;Sc.motion_plan=[3 12 1 0];if modulo(ic,2)==0 then Sc.motion_plan=[3 12 -1 0];end
end
if ic==7 then Sc.motion_plan=[3 12 0 1];end
if ic==8 then Sc.motion_plan=[3 12 0 -1];end
if ic==9 then Sc.motion_plan=[3 12 -1 1];end
if ic==10 then Sc.motion_plan=[3 12 -1 -1];end
if ic==11 | ic==12 then
 d=1;if ic==12 then d=-1;end
 Sc.motion_plan=[3 3.3 d 0;4 4.3 d 0;5 6 d 0;7 8 -d 0;9 10 d 0];
end
r=call("twip_motion_begin",speed,1,"d","out",[1,1],2,"d");Sc.trial_speed_ceiling=0.8;if speed>0.72 then Sc.trial_speed_ceiling=1.12;end;`:""}
${launch?`speed=.5;if modulo(ic,3)==2 then speed=.72;end;if modulo(ic,3)==0 then speed=.80;end
d=1;if modulo(ic,2)==0 then d=-1;end
${launch50?"speed=.5;":""}
Sc.motion_plan=[6 6.8 d 0;7.5 8.3 d 0;9 12 d 0;13 16 -d 0];
r=call("twip_motion_begin",speed,1,"d","out",[1,1],2,"d");Sc.trial_speed_ceiling=0.8;if speed>0.72 then Sc.trial_speed_ceiling=1.12;end;`: ""}
${disturbance?`if ic==5 | ic==6 then Sc.motion_plan=[];end
if ic==3 | ic==4 | ic==9 | ic==10 then Sc.motion_plan=[3 15 -1 0];end
if ic==11 then Sc.motion_plan=[3 15 1 1];end
if ic==12 then Sc.motion_plan=[3 15 -1 -1];end`:""}
endfunction
motor_case_ids=[${caseIds.join(" ")}];motor_native=%t;motor_cli=%f;motor_duration=20;motor_substeps=128;motor_settings=[500 1 150 .1 5 .006 3 .01 .5 1 .25 1];motor_stage="${label}";
exec(here+"study_run.sce",-1);mputl(out,"${p}/${dir}/path.txt");
${prepare?`for h=["balance_motion.h" "balance_turn.h" "balance_governor.h" "balance_recovery.h" "balance_profile.h" "phase2_motion_control.h"]
 copyfile("${p}/${dir}/headers/"+h,out+"/inputs/"+h);
end
rt16_summary=csvRead(out+"/summary.csv",",");
if or(rt16_summary(:,3)<19.995) then error("R12: a trajectory stopped early; inspect guard CSV");end
for ic=1:12
 rt16_guard=csvRead(out+msprintf("/guard%02d.csv",ic),",");
 if or(rt16_guard(:,6)<>0) then error("R12: a protection triggered; inspect guard CSV");end
end
if exists("scf")<>0 then
for ic=[1 3 4 5 6]
 a=csvRead(out+msprintf("/case%02d_policy1.csv",ic),",");b=csvRead(out+msprintf("/guard%02d.csv.motion.csv",ic),",");f=scf();clf(f);
 subplot(3,1,1);plot(a(:,1),a(:,7)*180/%pi);xgrid();ylabel("Pitch CF deg");title(msprintf("R12 RT18 case%d",ic));
 subplot(3,1,2);plot(a(:,1),a(:,11),b(:,1),b(:,3));xgrid();ylabel("Speed m/s");legend("Encoder","Reference");
 subplot(3,1,3);plot(a(:,1),a(:,12),b(:,1),b(:,2));xgrid();ylabel("Count position m");xlabel("Time s");legend("Encoder","Reference");
end
end
clear motor_override;
mprintf("RT18 model results: %s\\n",out);`:"quit;"}

`);if(prepare){console.log(path.resolve(dir+'/run.sce'));process.exit(0);}r=cp.spawnSync('C:/Program Files/scilab-2026.1.0/bin/Scilex.exe',['-nb','-f',path.resolve(dir+'/run.sce')],{encoding:'utf8'});process.stdout.write(r.stdout);process.stderr.write(r.stderr);assert.equal(r.status,0);const out=fs.readFileSync(dir+'/path.txt','utf8').trim();assert.equal(fs.readFileSync(path.join(out,'run_status.txt'),'utf8').trim(),'status=completed');fs.copyFileSync(dir+'/balance_governor.h',path.join(out,'inputs/balance_governor.h'));for(const h of ['balance_motion.h','balance_turn.h','balance_recovery.h','balance_profile.h','phase2_motion_control.h'])fs.copyFileSync(path.join(headers,h),path.join(out,'inputs/'+h));fs.copyFileSync(dir+'/native.c',path.join(out,'inputs/governor_native.c'));fs.copyFileSync(dir+'/run.sce',path.join(out,'inputs/governor_run.sce'));const summary=fs.readFileSync(path.join(out,'summary.csv'),'utf8').trim().split(/\r?\n/).map(l=>l.split(',').map(Number));assert.equal(summary.length,caseIds.length);for(const row of summary){assert(row[2]>=19.995,'Simulation terminated early');const tag=String(row[0]).padStart(2,'0');const guard=fs.readFileSync(path.join(out,'guard'+tag+'.csv'),'utf8').trim().split(/\r?\n/).map(l=>l.split(',').map(Number));assert(guard.every(x=>x[5]===0),'Simulation guard fault');}if(launch){for(const row of summary){const tag=String(row[0]).padStart(2,'0'),direction=row[0]%2?1:-1;const m=fs.readFileSync(path.join(out,'guard'+tag+'.csv.motion.csv'),'utf8').trim().split(/\r?\n/).map(l=>l.split(',').map(Number));assert(m.some(r=>r[0]>=9&&r[0]<=12&&r[4]===direction&&direction*r[2]>.20&&(r[3]&1)),'Launch case completed but did not execute the sustained requested direction');}}console.log(out);
