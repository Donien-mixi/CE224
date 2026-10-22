from pathlib import Path
import shutil
r=Path(__file__).resolve().parents[3];d=r/'.build/rt1_release_check'
for folder in ['Simulation/config','Simulation/model','Simulation/current','Xe_Can_Bang/Core/Inc']:
 for p in (r/folder).iterdir():
  if p.suffix not in ['.sce','.sci','.c','.h']:continue
  q=d/p.relative_to(r);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
for name in ['Simulation/build_native.ps1','Simulation/scripts/new_run.sci','Simulation/tools/twip_integrate.c','Simulation/tools/twip_pair_integrate.c']:
 p=r/name;q=d/name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
launch='''here=get_absolute_file_path("qualify.sce");exec(here+"cases.sce",-1);
link(here+"motion_native.dll",["twip_motion_reference";"twip_motion_control";"twip_motion_set_speed";"twip_motion_impact";"twip_motion_cancel_fault"],"c");link(here+"contact_native.dll","twip_pair_catalog_contact","c");link(here+"recovery_native.dll","twip_push_guard","c");
motor_cases=catalog_nominal_cases(ones(1,76),:);
for ic=1:48
 envelope=modulo(floor((ic-1)/8),3);
 if envelope==1 then motor_cases(ic,1:7)=[.95 1.05 1.05 .95 .006 .006 4];motor_cases(ic,14:16)=[11.2 .95 1.1];end
 if envelope==2 then motor_cases(ic,1:7)=[1.05 .95 .95 1.05 .006 .006 4];motor_cases(ic,14:16)=[11.2 .90 1.1];end
end
function [Pc,Sc]=motor_override(Pc,Sc,ic)
 speed=.5;if (ic>24&&ic<=48)||(ic>56&&ic<=64)||modulo(ic,2)==0&&ic>64 then speed=.72;end
 ack=call("twip_motion_set_speed",speed,1,"d","out",[1,1],2,"d");plans=[1 0;-1 0;0 1;0 -1;1 1;1 -1;-1 -1;-1 1];j=modulo(ic-1,8)+1;
 Sc.push_recovery=%t;Sc.push_trace_path=out+msprintf("/guard%02d.csv",ic);Sc.motion_plan=[3 17 plans(j,:)];
 if ic>=49&&ic<=64 then next=modulo(j,8)+1;Sc.motion_plan=[3 9 plans(j,:);9 17 plans(next,:)];end
 if ic>=65&&ic<=68 then if ic<=66 then Sc.motion_plan=[3 17 1 0];else Sc.motion_plan=[3 17 0 1];end;Sc.motion_freeze_at=8;end
 if ic>=69&&ic<=72 then Sc.tend=60;if ic<=70 then Sc.motion_plan=[3 52 0 1];else Sc.motion_plan=[3 52 1 1];end;end
 if ic>=73 then Sc.motion_plan=[3 17 1 1];Sc.pulses=8;Sc.wp=.1;Sc.Fp=.2;if ic>=75 then Sc.Fp=-.2;end;end
endfunction
motor_case_ids=1:76;motor_native=%t;motor_cli=%t;motor_duration=26;motor_substeps=256;motor_settings=[500 1 150 .1 5 .006 3 .01 .5 1 .25 1];motor_stage="rt1_final";exec(here+"study_run.sce",-1);
'''
(d/'Simulation/current/qualify.sce').write_text(launch)
