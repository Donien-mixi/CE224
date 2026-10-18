/* Experimental fork: directional running-friction ratios; original kernel unchanged. */
/* Diagnostic pitch + yaw approximation. No roll/slip/contact identification.
 * Separate forces/back-EMF/stiction expose coupling hidden by mean PWM.
 * Endpoint implicit integration; exact two-variable bounded friction solve.
 * Indices of p match native_parameters; c is documented in pair_coupled.sci. */
#include <math.h>
#define API __declspec(dllexport)
static double cap(double a,double b){return fmax(-b,fmin(b,a));}
static double dot(const double*a,const double*b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void solve(double H[3][3],const double*b,double*x){
 double a[3][4];for(int i=0;i<3;i++){for(int j=0;j<3;j++)a[i][j]=H[i][j];a[i][3]=b[i];}
 for(int i=0;i<3;i++){double v=a[i][i];for(int j=i;j<4;j++)a[i][j]/=v;
  for(int k=0;k<3;k++)if(k!=i){v=a[k][i];for(int j=i;j<4;j++)a[k][j]-=v*a[i][j];}}
 for(int i=0;i<3;i++)x[i]=a[i][3];
}
static double step(double*y,double*force,double*command,double fd,double h,double t,double*p,double*c){
 double r=p[6],B=c[0],m=p[0],l=p[1],xc=p[2];
 double q[2][3]={{1/r,-1,-B/(2*r)},{1/r,-1,B/(2*r)}};
 double old[3]={y[1],y[3],y[6]},v[3]={old[0],old[1],old[2]},fe[2]={force[0],force[1]},fr[2]={0,0};
 for(int iter=0;iter<3;iter++){
  double th=y[2]+h*v[1],co=cos(th),sn=sin(th),sc=xc*co+l*sn;
  double M[3][3]={{m+p[4],m*(l*co-xc*sn),0},{m*(l*co-xc*sn),p[3]+m*(l*l+xc*xc),0},{0,0,c[1]}};
  double H[3][3],b[3]={fd+m*sc*v[1]*v[1],m*p[5]*sc+fd*(p[17]*co-p[18]*sn),0};
  for(int i=0;i<3;i++)for(int j=0;j<3;j++){H[i][j]=M[i][j]/h;b[i]+=M[i][j]*old[j]/h;}
  H[0][0]+=p[7];H[2][2]+=c[2];
  double af[2],bf[2],limit[2],visc[2];
  for(int k=0;k<2;k++){
   double gain=c[3+k],wr=dot(q[k],old),u=command[k],vn=p[16];
   if(p[19]){int di=u<0?2:0;if(fabs(u)<1e-10)di=wr<0?2:0;vn=p[21]*p[22+di+k];}
   double fs=p[12]*.5*gain,ce=fs/vn,tau=c[7+k],rho=h/tau;
   double raw=fs*p[14]*u-ce*r*dot(q[k],v),fm=p[13]*.5*gain;
   af[k]=(force[k]+rho*cap(raw,fm))/(1+rho);bf[k]=0;
   if(fabs(raw)<=fm){af[k]=(force[k]+rho*fs*p[14]*u)/(1+rho);bf[k]=rho*ce/(1+rho);}
   int fdirection=wr<0?1:0;if(fabs(wr)<=p[11])fdirection=u<0?1:0;double friction=c[5+k]*c[(fabs(wr)<=p[11]?16:12)+2*fdirection+k];if(c[9]>=0&&t>=c[9])friction*=c[10+k];
   limit[k]=.5*friction*(fabs(wr)<=p[11]?p[9]:p[10]);visc[k]=.5*p[8];
   for(int i=0;i<3;i++){b[i]+=r*q[k][i]*af[k];for(int j=0;j<3;j++)H[i][j]+=(visc[k]+bf[k]*r*r)*q[k][i]*q[k][j];}
  }
  solve(H,b,v);double invq[2][3];for(int k=0;k<2;k++)solve(H,q[k],invq[k]);
  double g0=dot(q[0],invq[0]),g1=dot(q[1],invq[1]),g01=dot(q[0],invq[1]);
  double d0=dot(q[0],v),d1=dot(q[1],v),det=g0*g1-g01*g01;
  double cand[5][2]={{(-g1*d0+g01*d1)/det,(g01*d0-g0*d1)/det},
   {-limit[0],cap((-d1+g01*limit[0])/g1,limit[1])},
   { limit[0],cap((-d1-g01*limit[0])/g1,limit[1])},
   {cap((-d0+g01*limit[1])/g0,limit[0]),-limit[1]},
   {cap((-d0-g01*limit[1])/g0,limit[0]), limit[1]}};
  double best=0;fr[0]=fr[1]=0;
  for(int k=0;k<5;k++)if(fabs(cand[k][0])<=limit[0]&&fabs(cand[k][1])<=limit[1]){
   double a=cand[k][0],bb=cand[k][1],cost=.5*(g0*a*a+2*g01*a*bb+g1*bb*bb)+d0*a+d1*bb;
   if(cost<best){best=cost;fr[0]=a;fr[1]=bb;}
  }
  for(int i=0;i<3;i++)v[i]+=invq[0][i]*fr[0]+invq[1][i]*fr[1];
  for(int k=0;k<2;k++)fe[k]=af[k]-bf[k]*r*dot(q[k],v);
 }
 y[0]+=h*v[0];y[1]=v[0];y[2]+=h*v[1];y[3]=v[1];y[5]+=h*v[2];y[6]=v[2];
 force[0]=fe[0];force[1]=fe[1];y[4]=fe[0]+fe[1];
 double net=fe[0]+fe[1];for(int k=0;k<2;k++)net+=(-.5*p[8]*dot(q[k],v)+fr[k])/r;
 return net;
}
API void twip_pair_catalog_contact(double*state,double*p,double*s,double*c,double*u,double*out){
 double y[7];for(int i=0;i<7;i++)y[i]=state[i];double force[2]={state[7],state[8]};
 double la0=state[9],la1=state[10],lg=state[11],lz=state[12],fsum=0,h=s[1]/s[2];
 for(int j=0;j<(int)s[2];j++){
  double ta=j*h,tb=(j+1)*h,cuts[3]={ta,tb,0};int nc=2;
  if(ta<s[3]&&tb>s[3]){cuts[1]=s[3];cuts[2]=tb;nc=3;}
  for(int part=0;part<nc-1;part++){
   double dt=cuts[part+1]-cuts[part],t=s[0]+cuts[part],fd=0;
   for(int k=0;k<(int)s[7];k++)if(t>=s[8+k]&&t<s[8+k]+s[11])fd+=s[12];
   double command[2]={u[0],u[1]};if(cuts[part]<s[3]){command[0]=u[2];command[1]=u[3];}
   double prev[7];for(int i=0;i<7;i++)prev[i]=y[i];
   double pp[29];for(int i=0;i<29;i++)pp[i]=p[i];
   if(s[15]!=0 && t>=s[13] && t<s[14]){
    double penetration=s[15]*(y[0]+s[19]*sin(y[2]))-s[16];
    double approach=s[15]*(y[1]+s[19]*cos(y[2])*y[3]);
    if(penetration>0){fd-=s[15]*fmin(s[20],fmax(0,s[17]*penetration+s[18]*approach));pp[17]=s[19];pp[18]=0;}
   }
   fsum+=step(y,force,command,fd,dt,t,pp,c)*dt;
   double th=.5*(y[2]+prev[2]),w=.5*(y[3]+prev[3]),yawrate=.5*(y[6]+prev[6]);
   double co=cos(th),sn=sin(th),a=(y[1]-prev[1])/dt,aw=(y[3]-prev[3])/dt,az=(y[6]-prev[6])/dt;
   double xi=p[27]*co+p[28]*sn,zi=p[28]*co-p[27]*sn;
   /* BMI160 is ~10mm off the longitudinal center plane; assumed rigid mount. */
   double aix=a+zi*aw-xi*w*w-xi*yawrate*yawrate-.010*az,aiz=-xi*aw-zi*w*w;
   double raw0=-s[6]*(co*aix-sn*(aiz+p[5])),raw1=sn*aix+co*(aiz+p[5]);
   double ba=exp(-2*3.141592653589793*s[4]*dt),bg=exp(-2*3.141592653589793*s[5]*dt);
   la0=ba*la0+(1-ba)*raw0;la1=ba*la1+(1-ba)*raw1;lg=bg*lg-s[6]*(1-bg)*w;lz=bg*lz+(1-bg)*yawrate*co;
  }
 }
 for(int i=0;i<7;i++)out[i]=y[i];
 out[7]=force[0];out[8]=force[1];out[9]=la0;out[10]=la1;out[11]=lg;out[12]=lz;out[13]=fsum;
}
