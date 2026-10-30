// Same integer PWM rule as the proposed firmware. state=[filteredRate;lastCorrection].
// Positive delta=right distance-left distance. Preserve L+R EXACTLY.
function [left,right,state,ff]=wheel_pair_r8(left,right,delta,rate,state,cfg)
    state(1)=cfg(3)*state(1)+(1-cfg(3))*rate;
    ff=0;
    if left*right>0 then
        amp=(abs(left)+abs(right))/2;
        core=max(0,amp-170);A=cfg(8);B=cfg(9);
        if left>0 then core=max(0,amp-180);A=cfg(6);B=cfg(7);end
        ff=cfg(5)*(A+B*min(core,400))*max(0,min(1,(800-amp)/200));
    end
    cap=max(0,min([cfg(4),2499-abs(left),2499-abs(right),min(abs(left),abs(right))-1]));
    target=max(-cap,min(cap,cfg(1)*delta+cfg(2)*state(1)+ff));
    target=max(state(2)-cfg(10),min(state(2)+cfg(10),target));
    u=round(max(-cap,min(cap,target)));
    if left*right<=0 then u=0;end
    left=left+u;right=right-u;state(2)=u;
endfunction
