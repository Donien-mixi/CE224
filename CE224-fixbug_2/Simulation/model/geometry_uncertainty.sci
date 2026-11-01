// Separate an unknown CG displacement from an unknown IMU angle reference.
// Never infer either parameter from the sign of a fall alone.
function [Pc,Sc,eq_true_deg,eq_observed_deg]=geometry_uncertainty(P,S,cg_x_offset_mm,reference_error_deg)
 if size(cg_x_offset_mm,"*")<>1 | size(reference_error_deg,"*")<>1 then error("Geometry uncertainty must be scalar");end
 if isnan(cg_x_offset_mm) | isinf(cg_x_offset_mm) | isnan(reference_error_deg) | isinf(reference_error_deg) then error("Nonfinite geometry uncertainty");end
 Pc=P;Sc=S;
 Pc.xc=P.xc+cg_x_offset_mm/1000;
 // I is about the body centroid. Preserve it; the mass matrix separately uses
 // I + m*(l^2+xc^2), so an effective centroid translation obeys parallel-axis.
 // This is aggregate uncertainty, not a measured redistribution of components.
 eq_true_deg=-atan(Pc.xc/Pc.l)*180/%pi;
 if ~isfield(Sc,"angle_model") then Sc.angle_model=[1 0 0 -10 10 1];end
 Sc.angle_model(2)=Sc.angle_model(2)+reference_error_deg;
 am=Sc.angle_model;raw=Sc.raw_acc_sign*eq_true_deg;
 eq_observed_deg=Sc.acc_mult*(raw+(am(1)-1)*max(am(4),min(am(5),raw))+am(2));
endfunction
