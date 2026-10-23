// ISOLATED DIAGNOSTIC: no firmware deployment.
function [T,Y,F,U,E]=mo_phong(P,K,S,kf)
    // R8 floor logs invalidate the claim that preserved PWM sum validates balance.
    // Explicit acknowledgement is required for any pitch-only differential diagnostic.
    if isfield(S,"wheel_pair_r8") then
        if ~isfield(S,"allow_unvalidated_pair_diagnostic") then
            error("R8 differential model rejected by floor logs: no yaw/load state. Use explicit diagnostic profile; not hardware validation.");
        end
        if ~S.allow_unvalidated_pair_diagnostic then error("Unvalidated pair diagnostic not enabled");end
    end
    P.emf_sign=S.motor_sign;
    if isfield(S,"solver") then P.solver=S.solver; end
    use_native=native_available;
    if isfield(S,"native") then use_native=S.native & native_available;end
    if use_native then Ppacked=native_parameters(P);end
    // Diagnostic only. Normal tuning always keeps real sensor quantization.
    quant_sensor=%t;
    if isfield(S,"quant_sensor") then quant_sensor=S.quant_sensor; end
    rad=%pi/180; N=round(S.tend/S.Ts); h=S.Ts/S.nsub;
    initial_omega=0;if isfield(S,"initial_omega") then initial_omega=S.initial_omega;end
    initial_velocity=0;if isfield(S,"initial_velocity") then initial_velocity=S.initial_velocity;end
    y=[0;initial_velocity;S.th0;initial_omega;0]; Ii=0; vf=0; xhat=0;
    // Optional restart diagnostics: initial conditions only, never future log
    // states. Force and absolute attitude remain unknown without extra sensors.
    if isfield(S,"initial_force_N") then y(5)=S.initial_force_N;end
    if isfield(S,"initial_integral_deg") then Ii=S.initial_integral_deg;end
    if isfield(S,"initial_encoder_speed") then vf=S.initial_encoder_speed;end
    if isfield(S,"initial_encoder_position") then xhat=S.initial_encoder_position;end
    pair_state=[0;0]; // Optional historical R8 command.
    coupled=%f;if isfield(S,"coupled_pair") then coupled=%t;end
    pair=[0;0;0;0];lz=0;old_drive=[0;0];vfLR=[vf;vf];heading_state=zeros(7,1);heading_limit=8;
    if coupled then
        if ~isfield(S,"allow_unvalidated_pair_diagnostic") then error("Coupled diagnostic requires explicit acknowledgement: loaded model is NOT identified.");end
        if ~S.allow_unvalidated_pair_diagnostic then error("Coupled diagnostic not enabled");end
        Ppacked=native_parameters(P);
    end
    thhat=S.acc_mult*S.raw_acc_sign*S.th0;
    if isfield(S,"pitch_est0") then thhat=S.pitch_est0;end
    // Isolated literature experiment. Inputs are sensor/encoder estimates only.
    // Absent by default: the restored R7 path remains numerically unchanged.
    research=%f;research_state=[0;thhat;0;0];research_irow=1;
    if isfield(S,"research_control") then research=%t;end
    if research & ~coupled then error("Literature control requires the explicitly unvalidated two-wheel plant, not a mean-PWM proxy");end
    if research then
        if isfield(S.research_control,"observer") then
            research_state=observer_initial(thhat,S.research_control);research_irow=S.research_control.nobs+1;
        end
    end
    phase=[0.25;-0.25]; // GIA DINH pha encoder hai banh
    actual_ppr=[S.ppr;S.ppr];
    if isfield(S,"actual_ppr") then actual_ppr=matrix(S.actual_ppr,-1,1); end
    // Canonical two-wheel column. A row silently broadcasts against travel,
    // creating four counts/velocities and corrupting the native input packet.
    if size(actual_ppr,"*")<>2 | or(actual_ppr<=0) then error("Expected two positive per-wheel PPR values");end
    cnt_prev=round(-y(3)*actual_ppr/(2*%pi)+phase);
    if ~quant_sensor then cnt_prev=-y(3)*actual_ppr/(2*%pi)+phase; end
    cnt_zero=cnt_prev;
    u_prev=0; n=N+1;
    if isfield(S,"initial_drive") then u_prev=S.initial_drive;end
    T=zeros(1,n); Y=zeros(5,n); F=zeros(1,n); U=zeros(1,n); Y(:,1)=y;
    // Raw accel pitch = raw_acc_sign*theta; raw gyroY = -raw_acc_sign*omega.
    // Run7 slow tilts verify raw_acc_sign=-1, raw gyroY positive on forward tilt.
    // BMI160 0x28: ODR100Hz; BWP normal: acc40.5Hz/gyro39.9Hz.
    // LPF bac 1 khop cutoff la XAP XI, khong phai ham truyen Bosch da do.
    la=[S.raw_acc_sign*P.g*sin(y(3));P.g*cos(y(3))]; lg=0;
    acc_hold=atan(la(1),la(2)); gyro_hold=0;
    if isfield(S,"initialize_sensor_rate") then
        if S.initialize_sensor_rate then lg=-S.raw_acc_sign*initial_omega;gyro_hold=lg;end
    end
    // Explicit observation delay envelope. This is NOT a measured Bosch delay.
    // Floor logs reject the previous zero-extra-delay nominal model; fit/stress
    // scripts vary this parameter without changing geometry or sensor signs.
    sensor_delay_ticks=0;
    if isfield(S,"sensor_delay_ticks") then sensor_delay_ticks=S.sensor_delay_ticks;end
    imu_norm2=1;norm_queue=ones(1,sensor_delay_ticks+1);
    acc_queue=ones(1,sensor_delay_ticks+1)*acc_hold;
    gyro_queue=ones(1,sensor_delay_ticks+1)*gyro_hold;
    // Datasheet scales and ODR tolerance; controller still assumes nominal scales.
    acc_gain=[1;1]; gyro_gain=1; imu_odr=100; acc_bias_g=[0;0];
    if isfield(S,"acc_gain") then acc_gain=S.acc_gain;end
    if isfield(S,"gyro_gain") then gyro_gain=S.gyro_gain;end
    if isfield(S,"imu_odr") then imu_odr=S.imu_odr;end
    if isfield(S,"acc_bias_g") then acc_bias_g=S.acc_bias_g;end
    // Sensor quantization occurs BEFORE subtracting the calibrated gyro bias.
    // Keep its ADC phase, rather than re-quantizing a zero-mean log at zero bias.
    gyro_adc_bias=0;
    if isfield(S,"gyro_adc_bias") then gyro_adc_bias=S.gyro_adc_bias;end
    imu_period=1/imu_odr;
    imu_next=modulo(-S.imu_phase,2)*S.Ts;
    E=zeros(4,n); E(:,1)=[thhat;0;S.acc_mult*acc_hold;0];
    audit_control=%f;
    if isfield(S,"audit_control") then audit_control=S.audit_control;end
    if audit_control then
        E=[E;zeros(6,n)];E(5:6,1)=[vf;xhat];E(10,1)=Ii;
    end
    if coupled then
        if ~audit_control then error("Coupled diagnostic requires audit_control=true");end
        E=[E;zeros(5,n)];
    end
    recovering=%f;recovery_quiet=0;recovery_vmean=0;recovery_enter_time=0;audit_recovery=%f;
    if isfield(S,"audit_recovery") then audit_recovery=S.audit_recovery;end
    if audit_recovery then E=[E;zeros(1,n)];end
    audit_motor=%f;if isfield(S,"audit_motor") then audit_motor=S.audit_motor;end
    if audit_motor then
        if ~coupled | audit_recovery then error("Motor force audit requires coupled plant without recovery-mode audit");end
        E=[E;zeros(2,n)];
    end
    audit_observer=%f;if isfield(S,"audit_observer") then audit_observer=S.audit_observer;end
    if audit_observer then
        if ~audit_motor then error("Observer audit requires motor audit");end
        E=[E;zeros(5,n)];
    end
    push_state=zeros(7,1);motion_state=zeros(28,1);motion_word=0;push_fault=0;push_events=[];motion_events=[];driver_cancelled=%f;last_plan_wanted=0;last_plan_turn=0;sent_turn=0;
    external_stopped=%f; // Latched gateway/operator STOP, independent of PID stability.
    for k=1:N
        t=(k-1)*S.Ts;
        if t+1e-10>=imu_next then
            // Polling at200Hz: read the latest sample; hold it until next update.
            while imu_next<=t+1e-10
                imu_next=imu_next+imu_period;
            end
            // Nhieu chi tao mot lan tren mau MOI, mau cu giu nguyen.
            ca=cos(S.n_th(k)); sa=sin(S.n_th(k));
            a_sensor=acc_gain.*[ca*la(1)+sa*la(2);-sa*la(1)+ca*la(2)]+acc_bias_g*P.g;
            raw_g=a_sensor/P.g;
            if isfield(S,"accel_calibration") then
                ac=S.accel_calibration;
                raw_g=ac(2,:)'.*raw_g+ac(1,:)';
            end
            imu_norm2=sum(raw_g.^2);
            aq=raw_g*4096;
            aq=max(min(round(aq),32767),-32768);
            decoded_g=aq/4096;
            if isfield(S,"accel_calibration") then
                decoded_g=(decoded_g-ac(1,:)')./ac(2,:)';
            end
            acc_hold=atan(decoded_g(1),decoded_g(2));
            gq=round((gyro_gain*lg+S.n_g(k)+(S.gyro_bias+gyro_adc_bias)*rad)/rad*16.4);
            gyro_hold=(max(min(gq,32767),-32768)/16.4-gyro_adc_bias)*rad;
            if ~quant_sensor then
                acc_hold=atan(a_sensor(1),a_sensor(2));
                gyro_hold=gyro_gain*lg+S.n_g(k)+S.gyro_bias*rad;
            end
            // Empirical installed-angle error, not an inverse firmware correction.
            // Restrict gain error to measured +/-10deg; hold error outside the range.
            // This local observation model is superseded once XYZ calibration is valid.
            if isfield(S,"angle_model") then
                am=S.angle_model;
                raw_local=max(min(acc_hold/rad,am(5)),am(4));
                acc_hold=acc_hold+((am(1)-1)*raw_local+am(2))*rad;
            end
            // Optional slow effective zero/load-bias uncertainty. This is not
            // a new BMI160 calibration inferred from a closed-loop capture.
            if isfield(S,"angle_bias_drift_deg") then
                drift_fraction=max(0,min(1,(t-S.angle_bias_drift_start)/S.angle_bias_drift_ramp));
                acc_hold=acc_hold+S.angle_bias_drift_deg*drift_fraction*rad;
            end
        end
        norm_queue=[norm_queue(2:$),imu_norm2];
        acc_queue=[acc_queue(2:$),acc_hold];gyro_queue=[gyro_queue(2:$),gyro_hold];
        th_acc=S.acc_mult*acc_queue(1); gyro=S.gyro_sign*gyro_queue(1);
        alpha=S.imu_alpha;
        if recovering & isfield(S,"recovery_alpha") then alpha=S.recovery_alpha;end
        // Diagnostic adaptive weighting: reduce accel correction during rotation
        // or large innovation. No signal is replaced by recorded future angles.
        if isfield(S,"adaptive_imu") then
            cfg=S.adaptive_imu;pred=thhat+gyro*S.Ts;
            enable_adaptive=%t;
            // Optional catch-only gate preserves fixed R7 filtering near upright.
            if size(cfg,"*")>=4 then
                risk=abs(thhat/rad);
                if size(cfg,"*")>=5 & thhat*gyro>0 then risk=max(risk,abs(thhat/rad+cfg(5)*gyro/rad));end
                enable_adaptive=risk>cfg(4);
            end
            if enable_adaptive & (abs(gyro/rad)>cfg(1) | abs((th_acc-pred)/rad)>cfg(2)) then alpha=cfg(3);end
        end
        thhat=alpha*(thhat+gyro*S.Ts)+(1-alpha)*th_acc;
        // Cat theo GOC UOC LUONG nhu FSM; 45do that la gioi han mo hinh.
        if abs(thhat)>=65*rad then n=k; break; end
        travel=[y(1);y(1)];
        if coupled then travel=travel+[-1;1]*S.coupled_pair(1)*pair(1)/2;end
        count=round((travel/P.r-y(3)).*actual_ppr/(2*%pi)+phase);
        if ~quant_sensor then count=(travel/P.r-y(3)).*actual_ppr/(2*%pi)+phase; end
        vrLR=(count-cnt_prev)*(2*%pi*P.r/S.ppr)/S.Ts;
        vfLR=(1-S.enc_beta)*vrLR+S.enc_beta*vfLR;
        delta=((count(2)-cnt_zero(2))-(count(1)-cnt_zero(1)))*(2*%pi*P.r/S.ppr);
        vraw=mean(vrLR);cnt_prev=count;
        vf=(1-S.enc_beta)*vraw+S.enc_beta*vf; xhat=xhat+vf*S.Ts;
        if isfield(S,"recovery_controller") then
            rc=S.recovery_controller;risk=abs(thhat/rad);
            recovery_vmean=.95*recovery_vmean+.05*vf;
            if thhat*gyro>0 then risk=max(risk,abs(thhat/rad+rc(4)*gyro/rad));end
            if ~recovering & (risk>rc(1) | abs(vf)>rc(2) | abs(gyro/rad)>rc(3)) then recovering=%t;recovery_quiet=0;recovery_enter_time=t;end
            if recovering then
                quiet=abs(thhat/rad)<rc(7) & abs(recovery_vmean)<rc(8) & abs(gyro/rad)<rc(9);
                if quiet then recovery_quiet=recovery_quiet+S.Ts;else recovery_quiet=0;end
                if recovery_quiet>=rc(10) then recovering=%f;recovery_quiet=0;end
            end
        end
        if isfield(S,"push_recovery") then
            impact=call("twip_motion_impact",[1;gyro/rad;norm_queue(1)],1,"d",motion_state,2,"d","out",[1,1],3,"d");
            if impact<>0 then push_fault=impact;push_events=[push_events;t,0,25,.8,.3,impact];n=k;break;end
            maneuver=motion_state(6)<>0 & motion_state(13)==0 & motion_state(7)==0 & motion_state(21)==0 & t*1000-motion_state(4)<350 & (motion_state(11)<>0 | motion_state(19)<>0 | motion_state(24)<>0 | motion_state(26)<>0);
            push_result=call("twip_push_guard",[1;thhat/rad;gyro/rad;vf-motion_state(2);xhat-motion_state(1);.3;t*1000;vf;motion_state(2);bool2s(maneuver)],1,"d",push_state,2,"d","out",[11,1],3,"d");
            push_state=push_result(1:7);push_fault=push_result(11);
            wanted=0;wanted_turn=0;
            for j=1:size(S.motion_plan,1)
                if t>=S.motion_plan(j,1) & t<S.motion_plan(j,2) then wanted=S.motion_plan(j,3);if size(S.motion_plan,2)>=4 then wanted_turn=S.motion_plan(j,4);end;end
            end
            publish=%t;if isfield(S,"motion_freeze_at") then publish=t<S.motion_freeze_at;end
            // Browser hold state, not plant truth: on a displayed inhibit the
            // actual UI clears driveHeld and never resumes a held gesture.
            changed=wanted<>last_plan_wanted | wanted_turn<>last_plan_turn;
            if changed then driver_cancelled=%f;end
            last_plan_wanted=wanted;last_plan_turn=wanted_turn;
            // RT2 matches the browser: immediate direction/release events and40ms refresh.
            if publish & (modulo(k-1,8)==0 | changed) then
                if (wanted<>0 | wanted_turn<>0) & (push_state(6)<>0 | bitand(uint16(motion_state(10)),uint16(1))==0) then driver_cancelled=%t;end
                sent_direction=wanted;sent_turn=wanted_turn;if driver_cancelled then sent_direction=0;sent_turn=0;end
                motion_word=4*(floor(motion_word/4)+1);if sent_direction==1 then motion_word=motion_word+1;end;if sent_direction==-1 then motion_word=motion_word+2;end
            end
            if isfield(S,"motion_speed_plan") then
                selected_speed=S.motion_speed_plan(1,2);
                for j=1:size(S.motion_speed_plan,1)
                    if t>=S.motion_speed_plan(j,1) then selected_speed=S.motion_speed_plan(j,2);end
                end
                ack=call("twip_motion_set_speed",selected_speed,1,"d","out",[1,1],2,"d");
            end
            motion_result=call("twip_motion_reference",[1-push_state(6);motion_word;xhat;thhat/rad;gyro/rad;vf;t*1000;sent_turn;delta/S.coupled_pair(1);(vfLR(2)-vfLR(1))/S.coupled_pair(1)],1,"d",motion_state,2,"d","out",[29,1],3,"d");
            motion_state=motion_result(1:28);
            if motion_result(29)<>1 then error("Invalid motion reference");end
            cancel_fault=call("twip_motion_cancel_fault",[xhat;t*1000],1,"d",motion_state,2,"d","out",[1,1],3,"d");
            if cancel_fault<>0 then push_fault=cancel_fault;push_events=[push_events;t,0,25,.8,.3,cancel_fault];n=k;break;end
            S.research_control.motion_ref=motion_state([1 2 17 18 28]);
            motion_events=[motion_events;t,motion_state(1:2)',motion_state([10 11 12 13 14 17 18 19 21 22 24 26 28])'];
            S.push_diagnostic_limits=push_result([8 10 9]); // angle, distance, speed
            push_events=[push_events;t,push_state(6),push_result(8:11)'];
            if push_fault<>0 then n=k;break;end
        end
        if isfield(S,"external_stop_at") then
            if t+1e-10>=S.external_stop_at then external_stopped=%t;end
        end
        if isfield(S,"trial_interlocks") & ~external_stopped then
            if isfield(S,"push_diagnostic_limits") then
                lim=S.push_diagnostic_limits;
                if S.trial_interlocks & (abs(thhat/rad)>lim(1) | abs(xhat-motion_state(1))>lim(2) | abs(vf)>lim(3)) then n=k;break;end
            else
                if S.trial_interlocks & (abs(thhat/rad)>25 | abs(xhat)>.30 | abs(vf)>.80) then n=k;break;end
            end
        end
        if S.mode==1 then v_cmd=max(min(K(5)*(S.xt-xhat),1),-1); else v_cmd=S.vt; end
        ev=v_cmd-vf;
        outer_limit=S.tilt_max;
        if isfield(S,"capture_authority") then outer_limit=capture_tilt_limit(thhat/rad,S.capture_authority);end
        // Optional floor revision: slow integral only after capture, with bounded
        // angle authority and conditional integration at theta saturation.
        integral_limit=10;integrate=%t;
        if isfield(S,"integral_limit") then integral_limit=S.integral_limit;end
        if isfield(S,"integral_start") then integrate=t>=S.integral_start;end
        if isfield(S,"integral_gate_angle") then integrate=integrate & abs(thhat/rad)<S.integral_gate_angle;end
        if isfield(S,"integral_gate_speed") then integrate=integrate & abs(vf)<S.integral_gate_speed;end
        if integrate then
            candidate_I=max(min(Ii+K(4)*ev*S.Ts,integral_limit),-integral_limit);
            pending_theta=K(3)*ev+candidate_I;
            if ~isfield(S,"integral_start") | abs(pending_theta)<=outer_limit | pending_theta*ev<0 then Ii=candidate_I;end
        end
        theta_t=max(min(K(3)*ev+Ii,outer_limit),-outer_limit);
        pwm=S.pid_sign*(K(1)*(theta_t+S.trim-thhat/rad)-K(2)*gyro/rad);
        if recovering then
            blend=1;
            if isfield(S,"recovery_blend") then
                // Return toward the R7 gains once mean drift disappears, rather
                // than requiring a high-gain oscillation to satisfy quiet gates.
                blend=min(1,abs(recovery_vmean)/.08);
                if t-recovery_enter_time<.15 then blend=1;end
            end
            pwm=S.pid_sign*(K(1)*(1+blend*(rc(5)-1))*(theta_t+S.trim-thhat/rad)-K(2)*(1+blend*(rc(6)-1))*gyro/rad);
        end
        if isfield(S,"capture_angle_gain") then
            catch_angle=thhat/rad;
            if isfield(S,"capture_angle_prediction_s") & thhat*gyro>0 then
                catch_angle=catch_angle+S.capture_angle_prediction_s*gyro/rad;
            end
            pwm=pwm*capture_angle_gain(catch_angle,S.capture_angle_gain);
        end
        if isfield(S,"first_armed_zero") then
            if S.first_armed_zero & k==1 then pwm=0;Ii=0;theta_t=0;end
        end
        dbL=S.DB_L;dbR=S.DB_R;
        if isfield(S,"DB_direction") then
            dc=1;if pwm<0 then dc=2;end
            dbL=S.DB_direction(1,dc);dbR=S.DB_direction(2,dc);
        end
        outL=pid_deadband(pwm,dbL,2499); outR=pid_deadband(pwm,dbR,2499);
        if isfield(S,"wheel_pair_r8") & ~external_stopped then
            [outL,outR,pair_state,pair_ff]=wheel_pair_r8(outL,outR,0,0,pair_state,S.wheel_pair_r8);
        end
        if external_stopped then
            // Mirror firmware Motor_Stop: both PWM=0 selects electrical brake.
            // No modeled hand catch; post-STOP falling cannot identify the PID.
            outL=0;outR=0;pwm=0;Ii=0;theta_t=0;v_cmd=0;
            if research then research_state(research_irow)=0;heading_state(2)=0;end
        end
        if research & ~external_stopped then
            if isfield(S.research_control,"observer") then
                xcounter=mean(count-cnt_zero)*(2*%pi*P.r/S.ppr);
                [outL,outR,research_state]=observer_control(thhat,gyro,th_acc,xcounter,vfLR,delta,research_state,S.research_control,S.Ts);
            else
                [outL,outR,research_state]=literature_control(thhat,gyro,xhat,vfLR,delta,research_state,S.research_control,S.Ts);
            end
            pwm=(outL+outR)/2;theta_t=0;Ii=research_state(research_irow);
            heading_state(2)=(outL-outR)/2;heading_limit=2499;
            if isfield(S,"first_armed_zero") then
                if S.first_armed_zero & k==1 then
                    outL=0;outR=0;pwm=0;research_state(research_irow)=0;
                    if isfield(S.research_control,"observer") then research_state($)=0;end
                end
            end
        end
        if coupled & ~research then
            if S.heading_policy==4 then
                hc=[];if isfield(S,"guarded_heading_cfg") then hc=S.guarded_heading_cfg;end
                [outL,outR,heading_state,heading_limit]=guarded_heading(outL,outR,delta,vfLR(2)-vfLR(1),thhat/rad,gyro/rad,theta_t,vf,t,heading_state,hc);
            else
                [outL,outR,heading_state,heading_limit]=pair_heading(outL,outR,delta,vfLR(2)-vfLR(1),thhat/rad,gyro/rad,theta_t,vf,t,heading_state,S.heading_policy);
            end
        end
        command=S.motor_sign*(outL+outR)/(2*2499);
        // Optional per-wheel loaded-effort sensitivity, NOT a fitted torque model.
        // Unlike a pure PWM average, unequal gains/losses expose pitch/yaw coupling.
        // Additional losses are residual uncertainties; existing P.Tstat/Tkin remain.
        if isfield(S,"pair_effort_probe") then
            command=S.motor_sign*pair_effort_probe(outL,outR,S.pair_effort_probe)/2499;
        end
        // Probe coupling from differential assist with unequal wheel force gains.
        // Common equivalent PWM is a scenario input, not an identified force bound.
        if isfield(S,"mean_pwm_bias") & ~external_stopped then
            if outL<>0 | outR<>0 then command=command+S.motor_sign*S.mean_pwm_bias/2499;end
        end
        fsum=0;
        if coupled then
            drive=S.motor_sign*[outL;outR]/2499;
            if isfield(S,"catalog_friction") then
                [y,pair,la,lg,lz,fsum]=catalog_interval(y,pair,la,lg,lz,Ppacked,S,t,drive,old_drive);
            else
                [y,pair,la,lg,lz,fsum]=pair_interval(y,pair,la,lg,lz,Ppacked,S,t,drive,old_drive);
            end
            old_drive=drive;
        elseif use_native then
            [y,la,lg,fsum]=native_interval(y,la,lg,Ppacked,S,t,command,u_prev);
        else
        for j=1:S.nsub
            // Tach tai thoi diem PWM cap nhat: khong lam tron 0.35ms thanh h.
            ta=(j-1)*h; tb=j*h; cuts=[ta tb];
            if ta<S.latency & S.latency<tb then cuts=[ta S.latency tb]; end
            for ic=1:length(cuts)-1
                dt=cuts(ic+1)-cuts(ic); tj=t+cuts(ic); Fd=0;
                for ip=1:size(S.pulses,"*")
                    if tj+1e-10>=S.pulses(ip) & tj+1e-10<S.pulses(ip)+S.wp then Fd=Fd+S.Fp; end
                end
                u=command; if cuts(ic)+1e-10<S.latency then u=u_prev; end
                y0=y; [y,Fn]=buoc_co_he(y,u,Fd,dt,P); fsum=fsum+Fn*dt;
                ym=(y0+y)/2; c=cos(ym(3)); s=sin(ym(3));
                aa=(y(2)-y0(2))/dt; aw=(y(4)-y0(4))/dt;
                xi=P.ximu*c+P.zimu*s; zi=P.zimu*c-P.ximu*s;
                aix=aa+zi*aw-xi*ym(4)^2; aiz=-xi*aw-zi*ym(4)^2;
                raw_a=[-S.raw_acc_sign*(c*aix-s*(aiz+P.g));s*aix+c*(aiz+P.g)];
                ba=exp(-2*%pi*S.acc_fc*dt); bg=exp(-2*%pi*S.gyr_fc*dt);
                la=ba*la+(1-ba)*raw_a; lg=bg*lg-S.raw_acc_sign*(1-bg)*ym(4);
            end
        end
        end
        U(k+1)=(S.latency*u_prev+(S.Ts-S.latency)*command)/S.Ts; u_prev=command;
        T(k+1)=k*S.Ts; Y(:,k+1)=y; F(k+1)=fsum/S.Ts;
        E(1:4,k+1)=[thhat;gyro;th_acc;pwm];
        if audit_control then E(5:10,k+1)=[vf;xhat;theta_t;outL;outR;Ii];end
        if coupled then E(11:15,k+1)=[pair(1);pair(2);delta;heading_state(2);heading_limit];end
        if audit_recovery then E($,k+1)=bool2s(recovering);end
        if audit_motor then E(16:17,k+1)=pair(3:4);end
        if audit_observer then
            if research then
                if isfield(S.research_control,"observer") then E(18:22,k+1)=research_state(1:5);end
            end
        end
        if abs(y(3))>=65*rad | or(isnan(y)) | or(isinf(y)) then n=k+1; break; end
    end
    if isfield(S,"push_trace_path") then csvWrite(push_events,S.push_trace_path);csvWrite(motion_events,S.push_trace_path+".motion.csv");end
    T=T(1:n); Y=Y(:,1:n); F=F(1:n); U=U(1:n); E=E(:,1:n);
endfunction
