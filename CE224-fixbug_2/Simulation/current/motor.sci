// Anchors from user's exact supplier sheet; bridge bounds from Allegro A4950.
// Not serial-number-specific identification, and NOT a measured electrical tau.
function [P,catalog]=catalog_motor(P,curves,vm,bridge_r)
 rated=.4*9.80665/100;omega0=280*2*%pi/60;omegar=215*2*%pi/60;
 netstall=rated/(1-omegar/omega0);rm=12/1.8;beta=rm/(rm+bridge_r);
 offset=mean(-curves(:,2)./curves(:,1));frac=offset/2499*(11.1/12);
 emstall=netstall/(1-frac);kin=emstall*frac;
 P.Fstall=2*emstall*beta/P.r;P.Fmax=P.Fstall*vm/12;P.voltage=vm/12;
 P.Tkin=2*kin;P.Tstat=P.Tkin*(175/offset);
 // Retain measured relative direction slopes, scale their arithmetic mean to
 // the catalog's 280rpm no-load point with inferred running friction.
 ratios=curves(:,1)/mean(curves(:,1));base=omega0*P.r*emstall/netstall;
 P.drive_vnl=[ratios(1) ratios(2);ratios(3) ratios(4)]*base;
 P.directional=%t;P.vnl=base;
 catalog=[vm bridge_r rm beta rated netstall emstall kin omega0 omegar offset];
endfunction
function [y,pair,la,lg,lz,fsum]=catalog_interval(y,pair,la,lg,lz,packed,S,t,drive,old)
 pulse=zeros(3,1);np=size(S.pulses,"*");if np>0 then pulse(1:np)=matrix(S.pulses,-1,1);end
 ss=[t;S.Ts;S.nsub;S.latency;S.acc_fc;S.gyr_fc;S.raw_acc_sign;np;pulse;S.wp;S.Fp];
 result=call("twip_pair_catalog_v2",[y;pair;la;lg;lz],1,"d",packed,2,"d",ss,3,"d", ..
  [matrix(S.coupled_pair,-1,1);matrix(S.catalog_friction,-1,1);matrix(S.catalog_start_friction,-1,1)],4,"d",[drive;old],5,"d","out",[14,1],6,"d");
 y=result(1:5);pair=result(6:9);la=result(10:11);lg=result(12);lz=result(13);fsum=result(14);
endfunction
