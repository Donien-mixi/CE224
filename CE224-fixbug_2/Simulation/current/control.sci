// Own implementation of discrete state feedback and balance-priority allocation.
// This is an experiment, not a firmware release. No truth state enters control.
function cfg=literature_design(P,Btrack,Iz,q,R,alpha,lead,friction_fraction,emf_fraction)
 M=[P.Meq+P.m,P.m*P.l;P.m*P.l,P.I+P.m*P.l^2];
 grav=M\[0;P.m*P.g*P.l];force=M\[1;-P.r];
 A=zeros(4,4);A(1,2)=1;A(3,4)=1;A([2 4],3)=grav;
 B=zeros(4,1);B([2 4])=force;
 // Partial cancellation retains passive motor damping. The residual is included
 // in the design, instead of treating delayed encoder feedback as ideal force.
 ce=P.Fstall*mean(1 ./ P.drive_vnl);
 A=A+B*[0,-(1-emf_fraction)*ce,0,(1-emf_fraction)*ce*P.r];
 Z=expm([A B;zeros(1,5)]*.005);Ad=Z(1:4,1:4);Bd=Z(1:4,5);
 plant=syslin("d",Ad,Bd,eye(4,4),zeros(4,1));
 [gain,X]=lqr(plant,diag(q),R);
 radius=max(abs(spec(Ad+Bd*gain)));
 if radius>=1 then error("LQR sign/stability check failed");end
 cfg=struct("K",gain,"radius",radius,"A",A,"B",B,"alpha",alpha,"lead",lead, ..
  "track",Btrack,"r",P.r,"fs",P.Fstall/2,"voltage",P.voltage,"vnl",P.drive_vnl, ..
  "friction",P.Tkin/(2*P.r)*friction_fraction,"emf",emf_fraction,"yawKp",Iz*9,"yawKd",Iz*6, ..
  "yawKi",Iz*3,"yawIcap",.04,"yawCap",.08,"forceCap",P.Fmax);
endfunction
function [L,R,state]=literature_control(phi,gyro,xenc,vLR,delta,state,c,dt)
 // Encoder is wheel rotation relative to body, not axle world displacement.
 theta=phi+c.lead*gyro;
 vx=mean(vLR)+c.r*gyro;x=xenc+c.r*(phi-state(2));
 omega=gyro;
 if isfield(c,"gyro_prediction") then
  state(4)=.75*state(4)+.25*(gyro-state(3))/dt;state(3)=gyro;
  aa=max(-500,min(500,state(4)));omega=gyro+c.gyro_prediction*aa;
  theta=theta+.5*c.lead*c.gyro_prediction*aa;
  vx=mean(vLR)+c.r*omega;x=xenc+c.r*(theta-state(2));
 end
 desired=max(-c.forceCap,min(c.forceCap,c.K*[x;vx;theta;omega]));
 psi=delta/c.track;rate=(vLR(2)-vLR(1))/c.track;
 nextI=max(-c.yawIcap,min(c.yawIcap,state(1)+c.yawKi*psi*dt));
 yaw=max(-c.yawCap,min(c.yawCap,-c.yawKp*psi-c.yawKd*rate-nextI));
 [L,R,actualYaw]=literature_allocate(desired,yaw,vLR,c);
 // Conditional integration: freeze when allocation cannot provide requested turn.
 unsat=-c.yawKp*psi-c.yawKd*rate-nextI;
 if abs(unsat-actualYaw)<.0001 | (unsat-actualYaw)*psi>0 then state(1)=nextI;
 else
  yaw=max(-c.yawCap,min(c.yawCap,-c.yawKp*psi-c.yawKd*rate-state(1)));
  [L,R,actualYaw]=literature_allocate(desired,yaw,vLR,c);
 end
endfunction
function [L,R,actualYaw,actualTotal]=literature_allocate(total,yaw,vLR,c)
 // Nominal inverse actuator, fixed across all perturbed truth-model cases.
 // Only measured no-load slopes are available; friction/loaded force uncertain.
 loss=zeros(2,1);capacity=c.fs*c.voltage;
 for j=1:2
  direction=1;if vLR(j)<0 then direction=2;end
  smooth=abs(vLR(j))/(abs(vLR(j))+.02);
  s=smooth*tanh(vLR(j)/.01)+(1-smooth)*tanh(total/.05);
  fj=c.friction;
  if isfield(c,"frictionLR") then
   request=total/2+(-1)^j*yaw/c.track;
   if abs(vLR(j))<.002 then direction=1;if request<0 then direction=2;end;end
   smooth=abs(vLR(j))/(abs(vLR(j))+.005);
   s=smooth*tanh(vLR(j)/.01)+(1-smooth)*tanh(request/.01);
   fj=c.frictionLR(j,direction);
   if isfield(c,"startFrictionLR") then fj=smooth*fj+(1-smooth)*c.startFrictionLR(j,direction);end
  end
  loss(j)=c.emf*c.fs/c.vnl(j,direction)*vLR(j)+fj*s;
 end
 low=-capacity-loss;high=capacity-loss;
 total=max(sum(low),min(sum(high),total));average=total/2;
 dlo=max(low(1)-average,average-high(2));dhi=min(high(1)-average,average-low(2));
 diff=max(dlo,min(dhi,-yaw/c.track));f=[average+diff;average-diff];
 u=(f+loss)/capacity;L=round(2499*u(1));R=round(2499*u(2));
 actualYaw=-diff*c.track;
 actualTotal=capacity*(L+R)/2499-sum(loss);
endfunction
