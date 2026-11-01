function [y,pair,la,lg,lz,fsum]=catalog_interval(y,pair,la,lg,lz,packed,S,t,drive,old)
 pulse=zeros(3,1);np=size(S.pulses,"*");if np>0 then pulse(1:np)=matrix(S.pulses,-1,1);end
 ss=[t;S.Ts;S.nsub;S.latency;S.acc_fc;S.gyr_fc;S.raw_acc_sign;np;pulse;S.wp;S.Fp];
 wall=zeros(8,1);if isfield(S,"wall") then wall=matrix(S.wall,-1,1);end;ss=[ss;wall];
 result=call("twip_pair_catalog_contact",[y;pair;la;lg;lz],1,"d",packed,2,"d",ss,3,"d", ..
  [matrix(S.coupled_pair,-1,1);matrix(S.catalog_friction,-1,1);matrix(S.catalog_start_friction,-1,1)],4,"d",[drive;old],5,"d","out",[14,1],6,"d");
 y=result(1:5);pair=result(6:9);la=result(10:11);lg=result(12);lz=result(13);fsum=result(14);
endfunction
