// One catalog for continuing the two-bug campaign. None is a firmware release.
// Variant: screen/refinement/recovery/settle/blend/combined/validation.
if ~or(bugfix_variant==["screen","refinement","recovery","settle","blend","combined","validation"]) then error("Unknown two-bug experiment variant");end
// Cases: gainsL/R,frictionL/R,tauL/R,delay,loadTime,loadL/R,pushN,width,reference.
bugfix_cases=[1 1 1 1 .003 .003 3 -1 1 1 0 .1 0; ..
 .95 1.05 1 1 .003 .003 3 -1 1 1 0 .1 0;1.05 .95 1 1 .003 .003 3 -1 1 1 0 .1 0; ..
 .9 1.1 1.1 .9 .003 .006 3 8 1.2 1 0 .1 0; ..
 1 1 1 1 .003 .003 3 -1 1 1 .25 .1 0;1 1 1 1 .003 .003 3 -1 1 1 -.25 .1 0; ..
 1 1 1 1 .003 .003 3 -1 1 1 .5 .1 0;1 1 1 1 .003 .003 3 -1 1 1 -.5 .1 0; ..
 .95 1.05 1 1 .003 .003 3 -1 1 1 .5 .1 0;1.05 .95 1 1 .003 .003 3 -1 1 1 -.5 .1 0; ..
 1 1 1 1 .006 .006 4 -1 1 1 1 .1 0;1 1 1 1 .006 .006 4 -1 1 1 -1 .1 .5];
bugfix_heading=[24 100 1 3 .5];bugfix_adaptive=[30 6 .995 2];
bugfix_controllers=[0 2 5 1 0;4 2 5 1 0;4 2 5 1.5 0;4 2 5 1.5 1];
bugfix_recovery_thresholds=[1.5 .06 60 .025 2 1.3 1 .03 20 .4];
if bugfix_variant<>"screen" then
 bugfix_heading=[32 100 8 1 .3 1500 80 28];bugfix_adaptive=[20 6 .999 1.5 .025];
 bugfix_controllers=[0 1.5 4 1 0 0;4 1.5 4 1 0 0;4 1.5 4 2.5 0 .025;4 1.5 4 2.5 1 .025];
end
if or(bugfix_variant==["recovery","settle","blend","combined","validation"]) then
 bugfix_controllers=[0 1.5 4 1 0 0 0 0 0;4 1.5 4 1 0 0 0 0 0;4 1.5 4 1 0 0 2 1.3 0;4 1.5 4 1 0 0 2 1.3 1];
end
if or(bugfix_variant==["settle","blend","combined","validation"]) then
 bugfix_recovery_thresholds=[2 .08 70 .025 2 1.3 1.5 .03 80 .3];
end
if or(bugfix_variant==["blend","validation"]) then bugfix_controllers=bugfix_controllers([1 3],:);end
if bugfix_variant=="combined" then
 bugfix_controllers=[0 1.5 4 1 0 0 0 0 0 8;4 1.5 4 1 0 0 2 1.3 0 8;4 1.5 4 1 0 0 2.5 1.3 0 16];
end
bugfix_case_ids=1:size(bugfix_cases,1);bugfix_profile_ids=1:size(bugfix_controllers,1);
if bugfix_variant=="validation" then
 bugfix_case_ids=[2 5 6 10 2 5 6 10 2 5];bugfix_cases=bugfix_cases(bugfix_case_ids,:);
 bugfix_controllers=bugfix_controllers(2,:);bugfix_profile_ids=2;
 validation_substeps=[256 256 256 256 512 512 512 512 256 256];
 validation_durations=[30 30 30 30 30 30 30 30 120 120];
end
bugfix_push_time=8;bugfix_push_height=.1325;
bugfix_limits=struct("yaw_end_deg",5,"settled_pitch_deg",2,"settled_speed_mps",.05,"travel_m",.15,"recovery_seconds",5);
