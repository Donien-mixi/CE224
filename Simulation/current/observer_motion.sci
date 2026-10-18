// Discrete observer with actuator, accelerometer specific-force and sensor lag.
// Inputs are realisable IMU/encoder data and previous command, never plant truth.
function c=observer_design(P,Btrack,Iz,q,R,tau,delay_ticks,force_noise,acc_noise_deg)
 c=literature_design(P,Btrack,Iz,q,R,.998,0,1,1);
 M=[P.Meq+P.m,P.m*P.l;P.m*P.l,P.I+P.m*P.l^2];
 grav=M\[0;P.m*P.g*P.l];force=M\[1;-P.r];
 A5=zeros(5,5);A5(1,2)=1;A5(3,4)=1;A5([2 4],3)=grav;A5([2 4],5)=force;
 A5(5,5)=-1/tau;B5=[0;0;0;0;1/tau];
 z=expm([A5 B5;zeros(1,6)]*.005);Ad=z(1:5,1:5);Bd=z(1:5,6);
 [c.K5,c.control_radius,c.riccati_residual]=catalog_lqr(Ad,Bd,[q 0],R);
 z4=expm([c.A c.B;zeros(1,5)]*.005);
 [c.K,c.radius,c.riccati_residual4]=catalog_lqr(z4(1:4,1:4),z4(1:4,5),q,R);
 // Calibrated accel measures gravity AND translation/rotation acceleration.
 ha=[0 0 1-(grav(1)+P.zimu*grav(2))/P.g 0 -(force(1)+P.zimu*force(2))/P.g];
 gain=1.03912069574;ha=ha*gain;c.angle_gain=gain;
 ac=zeros(8,8);ac(1:5,1:5)=A5;bc=zeros(8,1);bc(1:5)=B5;
 ta=1/(2*%pi*40.5);tg=1/(2*%pi*39.9);te=-.005/log(.75);
 ac(6,1:5)=ha/ta;ac(6,6)=-1/ta;ac(7,4)=1/tg;ac(7,7)=-1/tg;
 ac(8,2)=1/te;ac(8,4)=-P.r/te;ac(8,8)=-1/te;
 zz=expm([ac bc;zeros(1,9)]*.005);n=8+2*delay_ticks;
 aa=zeros(n,n);bb=zeros(n,1);aa(1:8,1:8)=zz(1:8,1:8);bb(1:8)=zz(1:8,9);
 hg=7;haidx=6;
 if delay_ticks>0 then
  aa(9,7)=1;aa(9+delay_ticks,6)=1;
  for k=1:delay_ticks-1;aa(9+k,8+k)=1;aa(9+delay_ticks+k,8+delay_ticks+k)=1;end
  hg=8+delay_ticks;haidx=n;
 end
 hh=zeros(4,n);hh(1,1)=1;hh(1,3)=-P.r;hh(2,8)=1;hh(3,hg)=1;hh(4,haidx)=1;
 qq=eye(n,n)*1e-10;qq(1:5,1:5)=diag([1e-9 1e-5 1e-8 1e-4 force_noise]);
 rr=diag([.00012^2,.008^2,(.10*%pi/180)^2,(acc_noise_deg*%pi/180)^2]);
 cov=eye(n,n)*.01;ll=zeros(n,4);
 for k=1:2000
  pr=aa*cov*aa'+qq;ll=(pr*hh')/(hh*pr*hh'+rr);jj=eye(n,n)-ll*hh;
  next=jj*pr*jj'+ll*rr*ll';
  if norm(next-cov,1)<1e-12 then cov=next;break;end;cov=next;
 end
 c.Aobs=aa;c.Bobs=bb;c.Hobs=hh;c.Lobs=ll;c.nobs=n;c.observer=%t;
 c.observer_radius=max(abs(spec((eye(n,n)-ll*hh)*aa)));
 if max(c.control_radius,c.observer_radius)>=1 then error("Unstable LQR or observer design");end
 c.yawKp=Iz*64;c.yawKd=Iz*12.8;c.yawKi=Iz*10;c.yawCap=.08;c.yawIcap=.04;
endfunction
function [K,radius,residual]=catalog_lqr(A,B,q,R)
 // Same physical objective, scaled coordinates to avoid mixed-unit conditioning.
 n=size(A,1);scale=[.1 .1 .1 1 1];D=diag(scale(1:n));aa=D\A*D;bb=D\B;
 Q=D'*diag(q)*D;[kk,xx]=lqr(syslin("d",aa,bb,eye(n,n),zeros(n,1)),Q,R);K=kk/D;
 err=aa'*xx*aa-xx-(aa'*xx*bb)*((R+bb'*xx*bb)\(bb'*xx*aa))+Q;
 residual=norm(err,1)/max(1,norm(xx,1));radius=max(abs(spec(A+B*K)));
 if residual>1e-7 | radius>=1 then error("Riccati residual/stability verification failed");end
endfunction
function s=observer_initial(phi,c)
 n=c.nobs;s=zeros(n+3,1);th=phi/c.angle_gain;
 s(3)=th;s(6)=phi;
 d=(n-8)/2;if d>0 then s(9+d:n)=phi;end
 s(n+2)=th;
endfunction
function [L,R,s]=observer_control(phi,gyro,acc,xenc,vLR,delta,s,c,dt)
 if size(vLR,"*")<>2 then error("Expected exactly two encoder velocities");end
 if isfield(c,"native") then
  result=call("twip_motion_control",[phi;gyro;acc;xenc;matrix(vLR,-1,1);delta;c.motion_ref],1,"d",s,2,"d","out",[c.nobs+6,1],3,"d");
  if result($)<>1 then error("Native controller invalid output");end
  if isfield(c,"verify_native") then
   cr=c;cr=rmfield(cr,"native");cr=rmfield(cr,"verify_native");
   [lref,rref,sref]=observer_control(phi,gyro,acc,xenc,vLR,delta,s,cr,dt);
   if max(abs([result(c.nobs+4)-lref,result(c.nobs+5)-rref]))>1 then
    disp([phi;gyro;acc;xenc;matrix(vLR,-1,1);delta],"native inputs");
    disp([result(c.nobs+4:c.nobs+5)';lref rref],"native/source outputs");error("Transfer mismatch");
   end
  end
  s=result(1:c.nobs+3);L=result(c.nobs+4);R=result(c.nobs+5);return;
 end
 n=c.nobs;z=c.Aobs*s(1:n)+c.Bobs*s(n+3);
 measurement=[xenc-c.r*s(n+2);mean(vLR);gyro;acc];
 z=z+c.Lobs*(measurement-c.Hobs*z);s(1:n)=z;
 request=c.K5*z(1:5);
 if isfield(c,"blend") then
  theta0=phi+.03*gyro;theta=theta0+c.blend*(z(3)-theta0);
  omega=gyro+c.blend*(z(4)-gyro);
  xx=xenc+c.r*(theta-s(n+2));vv=mean(vLR)+c.r*omega;
  request=c.K*[xx;vv;theta;omega];
  if isfield(c,"feedback5") then request=c.K5*[xx;vv;theta;omega;z(5)];end
 end
 desired=max(-c.forceCap,min(c.forceCap,request));
 psi=delta/c.track;rate=(vLR(2)-vLR(1))/c.track;
 ci=max(-c.yawIcap,min(c.yawIcap,s(n+1)+c.yawKi*psi*dt));
 unsat=-c.yawKp*psi-c.yawKd*rate-ci;yaw=max(-c.yawCap,min(c.yawCap,unsat));
 [L,R,actual,total]=literature_allocate(desired,yaw,vLR,c);
 if abs(unsat-actual)<.0001 | (unsat-actual)*psi>0 then s(n+1)=ci;
 else
  yaw=max(-c.yawCap,min(c.yawCap,-c.yawKp*psi-c.yawKd*rate-s(n+1)));
  [L,R,actual,total]=literature_allocate(desired,yaw,vLR,c);
 end
 s(n+3)=total;
endfunction
