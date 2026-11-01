// Optional accelerator; missing library falls back to the same Scilab equations.
native_available=%f;
native_path=simulation_root+"tools/twip_integrate.dll";
if isfile(native_path) then
 try
  if ~c_link("twip_integrate") then link(native_path,"twip_integrate","c");end
  native_available=%t;
 catch
  mprintf("Native accelerator unavailable: %s. Using Scilab equations.\n",lasterror());
 end
end
function pp=native_parameters(P)
 dv=ones(2,2)*P.vnl;if P.directional then dv=P.drive_vnl;end
 pp=[P.m;P.l;P.xc;P.I;P.Meq;P.g;P.r;P.bx;P.bgear;P.Tstat;P.Tkin;P.weps; ..
 P.Fstall;P.Fmax;P.voltage;P.tau;P.vnl;P.zpush;P.xpush;bool2s(P.directional); ..
 P.emf_sign;P.speed_scale;dv(1,1);dv(2,1);dv(1,2);dv(2,2);P.solver;P.ximu;P.zimu];
endfunction
function [y,la,lg,fsum]=native_interval(y,la,lg,Ppacked,S,t,command,u_prev)
 pulse=zeros(3,1);np=size(S.pulses,"*");
 if np>3 then error("Native interval supports up to3 push events");end
 if np>0 then pulse(1:np)=matrix(S.pulses,-1,1);end
 ss=[t;S.Ts;S.nsub;S.latency;S.acc_fc;S.gyr_fc;S.raw_acc_sign;np;pulse;S.wp;S.Fp];
 result=call("twip_integrate",[y;la;lg],1,"d",Ppacked,2,"d",ss,3,"d", ..
 [command;u_prev],4,"d","out",[9,1],5,"d");
 y=result(1:5);la=result(6:7);lg=result(8);fsum=result(9);
endfunction
