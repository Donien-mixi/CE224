// DIAGNOSTIC: separately actuated pitch/yaw approximation, no roll or slip.
// c=[track,Iyaw_effective,yaw_visc,forceGainL,forceGainR,frictionL,frictionR,
//    torqueTauL,torqueTauR,frictionChangeTime,multiplierL,multiplierR].
// Track and Iyaw remain geometry assumptions; loaded gains/friction are NOT identified.
pair_library=simulation_root+"tools/twip_pair_integrate.dll";
if ~c_link("twip_pair_integrate") then link(pair_library,"twip_pair_integrate","c");end
function [y,pair,la,lg,lz,fsum]=pair_interval(y,pair,la,lg,lz,Ppacked,S,t,drive,old_drive)
 pulse=zeros(3,1);np=size(S.pulses,"*");if np>0 then pulse(1:np)=matrix(S.pulses,-1,1);end
 ss=[t;S.Ts;S.nsub;S.latency;S.acc_fc;S.gyr_fc;S.raw_acc_sign;np;pulse;S.wp;S.Fp];
 result=call("twip_pair_integrate",[y;pair;la;lg;lz],1,"d",Ppacked,2,"d",ss,3,"d", ..
   matrix(S.coupled_pair,-1,1),4,"d",[drive;old_drive],5,"d","out",[14,1],6,"d");
 y=result(1:5);pair=result(6:9);la=result(10:11);lg=result(12);lz=result(13);fsum=result(14);
endfunction
function [left,right,state,limit]=pair_heading(left,right,delta,rate,pitch,gyro,target,speed,t,state,policy)
 // state=[filteredRate,correction,quietTime,extraCap,error2,gyro2].
 state(1)=.95*state(1)+.05*rate;limit=8;
 if policy==1 then
  ff=0;amp=(abs(left)+abs(right))/2;
  if left*right>0 then
   core=max(0,amp-170);A=17.7724344155;B=-.02190686891;
   if left>0 then core=max(0,amp-180);A=23.2200000778;B=-.01271658079;end
   ff=.5*(A+B*min(core,400))*max(0,min(1,(800-amp)/200));
  end
  targetU=500*delta+40*state(1)+ff;limit=24;
 elseif policy>=2 then
  state(5)=.995*state(5)+.005*(pitch-target)^2;state(6)=.995*state(6)+.005*gyro^2;
  startAfter=3;if policy==3 then startAfter=8;end
  good=t>=startAfter & abs(pitch)<2.5 & abs(pitch-target)<3 & abs(gyro)<60 & abs(speed)<.08 & state(5)<1.5^2 & state(6)<25^2;
  if good then state(3)=state(3)+.005;else state(3)=0;end
  extraMax=4;
  if state(3)>=.5 then state(4)=min(extraMax,state(4)+.005);else state(4)=max(0,state(4)-.1);end
  if abs(pitch)>=3 | abs(gyro)>=80 | abs(speed)>=.15 then state(4)=0;end
  limit=8+state(4);targetU=500*delta+40*state(1);
 else targetU=500*delta+40*state(1);end
 cap=max(0,min([limit,2499-abs(left),2499-abs(right),min(abs(left),abs(right))-1]));
 u=max(-cap,min(cap,targetU));
 if policy==1 then u=max(state(2)-2,min(state(2)+2,u));end
 u=round(max(-cap,min(cap,u)));if left*right<=0 then u=0;end
 left=left+u;right=right-u;state(2)=u;
endfunction
