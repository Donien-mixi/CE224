function folder=simulation_new_run(simroot,label)
    d=getdate();
    stamp=msprintf("%04d%02d%02d_%02d%02d%02d_%03d",d(1),d(2),d(6),d(7),d(8),d(9),d(10));
    base=simroot+"results/runs/"+stamp+"_"+label;
    folder=base; n=0;
    while isdir(folder)
        n=n+1; folder=base+"_"+string(n);
    end
    mkdir(folder);
    mkdir(folder+"/inputs");
    copyfile(simroot+"model/robot_can_bang_pid_1.sce",folder+"/inputs/model.sce");
    copyfile(simroot+"config/run_settings.sce",folder+"/inputs/run_settings.sce");
    extra=["config/load_native_kernel.sce";"config/vehicle_baseline.sce";"config/team_run.sce"; ..
           "config/startup_r10s1.sce";"scripts/run_team.sce";"model/capture_authority.sci"; ..
           "model/pair_coupled.sci";"tools/twip_integrate.c";"tools/twip_pair_integrate.c"; ..
           "tools/build_native_kernel.ps1";"tools/build_pair_kernel.ps1"; ..
           "../Xe_Can_Bang/Core/Inc/balance_profile.h";"../Xe_Can_Bang/Core/Inc/balance_capture.h"; ..
           "../Xe_Can_Bang/Core/Inc/balance_assist.h";"../Xe_Can_Bang/Core/Src/pid.c"];
    for j=1:size(extra,"*")
        if isfile(simroot+extra(j)) then copyfile(simroot+extra(j),folder+"/inputs/"+basename(extra(j))+fileext(extra(j)));end
    end
    specs=["thông số cần thiết.txt";"thông số cần thiết 1.txt";"thông số chi tiết linh kiện.txt";"thông số động cơ.txt"];
    for i=1:size(specs,"*")
        if isfile(simroot+"../"+specs(i)) then copyfile(simroot+"../"+specs(i),folder+"/inputs/"+specs(i));end
    end
    mputl(["created="+stamp;"purpose="+label;"status=started"],folder+"/run_status.txt");
endfunction
