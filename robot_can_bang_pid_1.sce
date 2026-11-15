// Current R12 firmware headers, including RT18 admission, retained RT17 governor and count anchors.
// Node + GCC build the native wrapper; Execute keeps Scilab open with plots.
twip_root=get_absolute_file_path("robot_can_bang_pid_1.sce");
setenv("TWIP_SIM_LABEL","r12_current");
[twip_loaded,twip_library]=c_link("twip_motion_reference");if twip_loaded then ulink(twip_library);end
[twip_loaded,twip_library]=c_link("twip_push_guard");if twip_loaded then ulink(twip_library);end
[twip_status,twip_build,twip_error]=host("node """+twip_root+"tools/run_governor_sim.cjs"" --stress --prepare-only");
if twip_status<>0 then error(strcat(twip_error,ascii(10)));end
exec(twip_root+".build/governor_sim_r12_current/run.sce",-1);
