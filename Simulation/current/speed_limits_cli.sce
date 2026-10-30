// Requested-speed feasibility only; never changes firmware speed presets.
here=get_absolute_file_path("speed_limits_cli.sce");root=here+"../";
exec(here+"cases.sce",-1);
link(here+"motion_rt5.dll",["twip_motion_reference";"twip_motion_control";"twip_motion_set_speed";"twip_motion_impact";"twip_motion_cancel_fault"],"c");
link(here+"contact_native.dll","twip_pair_catalog_contact","c");
link(here+"recovery_rt5.dll","twip_push_guard","c");
motor_cases=catalog_nominal_cases(ones(1,8),:);
function [Pc,Sc]=motor_override(Pc,Sc,ic)
 speeds=[.72 .80 .85 .90];speed=speeds(ceil(ic/2));direction=1;if modulo(ic,2)==0 then direction=-1;end
 r=call("twip_motion_set_speed",speed,1,"d","out",[1,1],2,"d");
 Sc.push_recovery=%t;Sc.push_trace_path=out+msprintf("/guard%02d.csv",ic);
 Sc.motion_plan=[3 13 direction];
endfunction
motor_case_ids=1:8;motor_native=%t;motor_cli=%f;motor_duration=18;motor_substeps=128;
motor_settings=[500 1 150 .1 5 .006 3 .01 .5 1 .25 1];motor_stage="rt5_speed_feasibility";
exec(here+"study_run.sce",-1);
mputl(out,here+"speed_limits_path.txt");
mprintf("Requested speeds +/- .72,.80,.85,.90 m/s. Inspect cancellation/REC flags and achieved reference, not only final pitch. Results:%s\n",out);
quit;
