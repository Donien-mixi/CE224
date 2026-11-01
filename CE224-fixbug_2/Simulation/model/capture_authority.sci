// Limit only OUTER velocity-loop lean request. Inner angle PD is never faded.
// cfg=[outer_limit,fade_start_deg,fade_end_deg]; fade_start=end disables fade.
function cap=capture_tilt_limit(pitch,cfg)
 cap=cfg(1);
 if cfg(3)>cfg(2) then cap=cap*max(0,min(1,(cfg(3)-abs(pitch))/(cfg(3)-cfg(2))));end
endfunction
function gain=capture_angle_gain(pitch,cfg)
 fraction=max(0,min(1,(abs(pitch)-cfg(1))/(cfg(2)-cfg(1))));
 gain=1+(cfg(3)-1)*fraction;
endfunction
