// Diagnostic only: gains and residual PWM losses are NOT identified from floor logs.
// A surrogate effective effort, not a no-load velocity or physical torque conversion.
function effort=pair_effort_probe(left,right,cfg)
    if size(cfg,"*")<>4 | or(cfg(1:2)<=0) | or(cfg(3:4)<0) then
        error("Invalid per-wheel effort probe");
    end
    effort=(cfg(1)*sign(left)*max(0,abs(left)-cfg(3))+ ..
            cfg(2)*sign(right)*max(0,abs(right)-cfg(4)))/2;
endfunction
