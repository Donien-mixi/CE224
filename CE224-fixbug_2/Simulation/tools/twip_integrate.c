/* Native implementation of ONE control interval, identical to the Scilab
 * implicit endpoint/midpoint loops. Physical parameters and control stay in
 * the .sce model. No tuning, random numbers or hidden parameter changes here.
 * Build: gcc -O2 -shared -static-libgcc -o twip_integrate.dll twip_integrate.c
 * Deliberately no -ffast-math: numerical comparisons are part of validation.
 */
#include <math.h>
#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API
#endif
static double clamp(double x,double b){return fmax(-b,fmin(b,x));}
static double vnl(double *p,double u,double relative){
    if(!p[19])return p[16];
    double e=u*p[20];if(fabs(u)<1e-10)e=relative*p[20];
    int c=e<0?2:0;
    return p[21]*2/(1/p[22+c]+1/p[23+c]);
}
static void step(double *y,double u,double fd,double h,double *p,double *fn){
    double m=p[0],l=p[1],xc=p[2],I=p[3],Meq=p[4],g=p[5],r=p[6];
    double bx=p[7],bg=p[8],ts=p[9],tk=p[10],eps=p[11],fs=p[12],fm=p[13],vol=p[14],tau=p[15];
    int be=p[26]!=0;
    double v0=y[1],w0=y[3],fa0=y[4],vn=v0,wn=w0,fe=fa0,tfr=0;
    double wr0=v0/r-w0,limit=fabs(wr0)<=eps?ts:tk;
    double ce=fs/vnl(p,u,v0-r*w0),rho=h/(tau*(be?1:2));
    for(int it=0;it<3;it++){
        double th=y[2]+(be?h*wn:h*(w0+wn)/4),wm=be?wn:(w0+wn)/2;
        double a11=Meq+m,a12=m*(l*cos(th)-xc*sin(th)),a22=I+m*(l*l+xc*xc);
        double sc=xc*cos(th)+l*sin(th),d11=bx+bg/(r*r),d12=-bg/r,d22=bg;
        double raw=fs*vol*u-ce*(be?vn-r*wn:(v0+vn)/2-r*(w0+wn)/2),af,bf;
        if(fabs(raw)>fm){af=(fa0+rho*clamp(raw,fm))/(1+rho);bf=0;}
        else if(be){af=(fa0+rho*fs*vol*u)/(1+rho);bf=rho*ce/(1+rho);}
        else{af=(fa0+rho*fs*vol*u-rho*ce*(v0-r*w0)/2)/(1+rho);bf=rho*ce/(2*(1+rho));}
        double fac=be?1:.5,H11=a11/h+fac*d11+bf,H12=a12/h+fac*d12-bf*r,H22=a22/h+fac*d22+bf*r*r;
        double dh=H11*H22-H12*H12;
        double b1=a11/h*v0+a12/h*w0+af+fd+m*sc*wm*wm;
        double b2=a12/h*v0+a22/h*w0-r*af+m*g*sc+fd*(p[17]*cos(th)-p[18]*sin(th));
        if(!be){b1-=(d11*v0+d12*w0)/2;b2-=(d12*v0+d22*w0)/2;}
        double vb=(H22*b1-H12*b2)/dh,wb=(H11*b2-H12*b1)/dh;
        double iv=(H22/r+H12)/dh,iw=(-H12/r-H11)/dh,gamma=iv/r-iw;
        tfr=clamp(-(vb/r-wb)/gamma,limit);vn=vb+iv*tfr;wn=wb+iw*tfr;
        fe=af-bf*(vn-r*wn);
    }
    y[0]+=h*(be?vn:(v0+vn)/2);y[1]=vn;y[2]+=h*(be?wn:(w0+wn)/2);y[3]=wn;y[4]=be?fe:2*fe-fa0;
    double tv=-bg*(be?vn/r-wn:(v0+vn)/(2*r)-(w0+wn)/2);
    *fn=fe+(tv+tfr)/r;
}
API void twip_integrate(double *state,double *p,double *s,double *u,double *out){
    double y[5];for(int i=0;i<5;i++)y[i]=state[i];
    double la0=state[5],la1=state[6],lg=state[7],fsum=0;
    double h=s[1]/s[2],r=p[6];(void)r;
    for(int j=0;j<(int)s[2];j++){
        double ta=j*h,tb=(j+1)*h,cuts[3]={ta,tb,0};int nc=2;
        if(ta<s[3] && tb>s[3]){cuts[1]=s[3];cuts[2]=tb;nc=3;}
        for(int ic=0;ic<nc-1;ic++){
            double dt=cuts[ic+1]-cuts[ic],tj=s[0]+cuts[ic],fd=0;
            for(int k=0;k<(int)s[7];k++)if(tj>=s[8+k] && tj<s[8+k]+s[11])fd+=s[12];
            double command=cuts[ic]<s[3]?u[1]:u[0],old[5];for(int i=0;i<5;i++)old[i]=y[i];
            double fn;step(y,command,fd,dt,p,&fn);fsum+=fn*dt;
            double th=(y[2]+old[2])/2,w=(y[3]+old[3])/2,c=cos(th),sn=sin(th);
            double aa=(y[1]-old[1])/dt,aw=(y[3]-old[3])/dt;
            double xi=p[27]*c+p[28]*sn,zi=p[28]*c-p[27]*sn;
            double aix=aa+zi*aw-xi*w*w,aiz=-xi*aw-zi*w*w;
            double raw0=-s[6]*(c*aix-sn*(aiz+p[5])),raw1=sn*aix+c*(aiz+p[5]);
            double ba=exp(-2*3.14159265358979323846*s[4]*dt),bgy=exp(-2*3.14159265358979323846*s[5]*dt);
            la0=ba*la0+(1-ba)*raw0;la1=ba*la1+(1-ba)*raw1;lg=bgy*lg-s[6]*(1-bgy)*w;
        }
    }
    for(int i=0;i<5;i++)out[i]=y[i];out[5]=la0;out[6]=la1;out[7]=lg;out[8]=fsum;
}
