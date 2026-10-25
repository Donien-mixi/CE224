// Experimental encoder yaw PI, active only after quiet capture. NOT released.
// cfg=[totalCap,Ki,slewPWMperSecond,quietStartSeconds,quietHoldSeconds,Kp,Kd,Icap,(optional)catchCap].
// No no-load feedforward: real loaded differential response is unidentified.
function [left,right,state,limit]=guarded_heading(left,right,delta,rate,pitch,gyro,target,speed,t,state,cfg)
 if size(cfg,"*")==0 then cfg=[24 100 1 3 .5];end
 kp=500;kd=40;icap=16;
 if size(cfg,"*")>=8 then kp=cfg(6);kd=cfg(7);icap=cfg(8);end
 if size(state,"*")<7 then state(7)=0;end
 state(1)=.95*state(1)+.05*rate;
 state(5)=.995*state(5)+.005*(pitch-target)^2;state(6)=.995*state(6)+.005*gyro^2;
 quiet=t>=cfg(4) & abs(pitch)<2 & abs(pitch-target)<3 & abs(gyro)<60 & abs(speed)<.08 & state(5)<1.5^2 & state(6)<25^2;
 if quiet then state(3)=state(3)+.005;else state(3)=0;end
 if state(3)>=cfg(5) then state(4)=min(cfg(1)-8,state(4)+cfg(3)*.005);else state(4)=max(0,state(4)-.1);end
 // Immediately yield extra yaw authority when the pitch controller is catching.
 disturbed=abs(pitch)>=3 | abs(gyro)>=80 | abs(speed)>=.15;
 if disturbed then state(4)=0;end
 limit=8+state(4);
 // Explicit diagnostic tradeoff; defaults still restore the original8PWM.
 if disturbed & size(cfg,"*")>=9 then limit=cfg(9);end
 cap=max(0,min([limit,2499-abs(left),2499-abs(right),min(abs(left),abs(right))-1]));
 valid=left*right>0;
 candidate=state(7);
 if quiet & state(3)>=cfg(5) & valid then
  candidate=max(-icap,min(icap,state(7)+cfg(2)*delta*.005));
  request=kp*delta+kd*state(1)+candidate;
  if abs(request)<=cap | request*delta<0 then state(7)=candidate;end
 end
 // Preserve the learned integral during a push, but gate its OUTPUT. It cannot
 // add yaw effort beyond R7's original8PWM during the recovery phase.
 request=kp*delta+kd*state(1)+state(7);
 u=round(max(-cap,min(cap,request)));if ~valid then u=0;end
 left=left+u;right=right-u;state(2)=u;
endfunction
