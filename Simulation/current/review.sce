// Interactive SPEED2 review; no flashing and no quit.
here=get_absolute_file_path("review.sce");root=here+"../";
exec(root+"current/cases.sce",-1);
[loaded,library]=c_link("twip_motion_reference");if loaded then ulink(library);end
link(here+"motion_rt5.dll",["twip_motion_reference";"twip_motion_control";"twip_motion_set_speed";"twip_motion_impact";"twip_motion_cancel_fault"],"c");
if ~c_link("twip_pair_catalog_contact") then link(here+"contact_native.dll","twip_pair_catalog_contact","c");end
[loaded,recovery_library]=c_link("twip_push_guard");if loaded then ulink(recovery_library);end
link(root+"current/recovery_rt5.dll","twip_push_guard","c");
motor_cases=catalog_nominal_cases(ones(1,12),:);
function [Pc,Sc]=motor_override(Pc,Sc,ic)
 speed=.72;if ic<=2 then speed=.5;end;r=call("twip_motion_set_speed",speed,1,"d","out",[1,1],2,"d");
 Sc.push_recovery=%t;Sc.push_trace_path=out+msprintf("/guard%02d.csv",ic);Sc.motion_plan=[];
 if ic==2 | ic==3 then Sc.motion_plan=[3 13 1];end
 if ic==4 then Sc.motion_plan=[3 13 -1];end
 if ic==5 then Sc.motion_plan=[3 10 1;10 17 -1];end
 if ic==6 then Sc.motion_plan=[3 6.5 1];Sc.wall=[0 25 1 1.45 100 4 .10 25];end
 if ic==7 then Sc.motion_plan=[3 13 0 1];end
 if ic==8 then Sc.motion_plan=[3 13 0 -1];end
 if ic==9 then Sc.motion_plan=[3 13 1 1];end
 if ic==10 then Sc.motion_plan=[3 13 1 -1];end
 if ic==11 then Sc.motion_plan=[3 13 -1 -1];end
 if ic==12 then Sc.motion_plan=[3 13 -1 1];end
endfunction
motor_case_ids=1:12;motor_native=%t;motor_cli=%f;motor_duration=25;motor_substeps=128;
motor_settings=[500 1 150 .1 5 .006 3 .01 .5 1 .25 1];motor_stage="speed2_8dir_interactive";
exec(here+"study_run.sce",-1);mputl(out,here+"interactive_path.txt");
show_plots=%t;if isdef("motion_review_plots") then show_plots=motion_review_plots;end
if show_plots then
 for ic=[2 3 5 6]
  a=csvRead(out+msprintf("/case%02d_policy1.csv",ic),",");b=csvRead(out+msprintf("/guard%02d.csv.motion.csv",ic),",");f=scf();clf(f);
  subplot(3,1,1);plot(a(:,1),a(:,7)*180/%pi);xgrid();ylabel("Pitch CF deg");title(msprintf("R12 RT5 hold/release review case%d",ic));
  subplot(3,1,2);plot(a(:,1),a(:,11),b(:,1),b(:,3));xgrid();ylabel("Speed m/s");legend("Encoder","Reference");
  subplot(3,1,3);plot(a(:,1),a(:,12),b(:,1),b(:,2));xgrid();ylabel("Travel m");xlabel("Time s");legend("Encoder","Reference");
 end
 for ic=[7 8 9 11]
  a=csvRead(out+msprintf("/case%02d_policy1.csv",ic),",");b=csvRead(out+msprintf("/guard%02d.csv.motion.csv",ic),",");f=scf();clf(f);
  subplot(3,1,1);plot(a(:,1),a(:,7)*180/%pi);xgrid();ylabel("Pitch deg");title(msprintf("8 directions case%d (conditional flat floor)",ic));
  subplot(3,1,2);plot(a(:,1),a(:,17)*180/%pi,b(:,1),b(:,9)*180/%pi);xgrid();ylabel("Yaw deg");legend("Plant","Reference");
  subplot(3,1,3);plot(b(:,1),b(:,10));xgrid();ylabel("Turn reference rad/s");xlabel("Time s");
 end
end
clear motor_override;
mprintf("RT5 review done. 50cm/s default;72cm/s requires floor testing; collision model is not identified. Results:%s\n",out);
