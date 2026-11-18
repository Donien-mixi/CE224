#pragma once
// Included after measurement_web.h, using its bounded socket writer.
static int balanceRow(char *out,size_t n,const BalanceFrame &f) {
  return snprintf(out,n,"%lu,%lu,%u,%u,%u,%u,%u,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u,%u,%u,%.4f,%.6f,%.6f,%lu,%u,%d,%.6f,%.6f,%d,%lu,%lu",
    (unsigned long)f.sequence,(unsigned long)f.t_ms,f.dt_us,f.execution_us,f.state,f.ready,f.fault,
    f.pwm_l,f.pwm_r,(long)f.ticks_l,(long)f.ticks_r,f.gx,f.gy,f.gz,f.ax,f.ay,f.az,
    f.pitch,f.gyro,f.v_l,f.v_r,f.x,f.v_cmd,f.theta_cmd,f.pwm_base,f.integral,f.dropped,f.controller,f.capture,f.distance_limit,f.x_ref,f.v_ref,(unsigned long)f.drive_epoch,f.motion_flags,f.drive_direction,f.psi_ref,f.w_ref,f.turn_direction,(unsigned long)f.drive_sequence,(unsigned long)f.drive_rx_ms);
}
static int balanceExportRow(char *out,size_t n,const BalanceFrame &f) {
  if(n<2)return -1;
  int used=balanceRow(out,n-1,f);
  if(used<0 || (size_t)used>=n-1)return -1;
  out[used++]='\n';return used;
}
static uint8_t balanceMetaController() {
  const char *p=balanceMeta;
  for(unsigned i=0;i<17;i++){p=strchr(p,',');if(!p)return 0;p++;}
  char *end;unsigned long mode=strtoul(p,&end,10);
  return (*end==',' && mode==12)?(uint8_t)mode:0;
}
void setupBalanceRoutes() {
  server.on("/operate",HTTP_POST,[](){
    String action=server.arg("action"),command,message;
    if(action=="stop") {
      controlStop("stop_button_http");
      server.send(200,"application/json","{\"message\":\"Đã gửi DỪNG tới STM32. Kiểm tra PWM về 0.\"}");return;
    }
    long wheel=0,direction=0,client=-1,token=0,revision=-1,controller=0;
    bool sessionOK=strictNumber(server.arg("session"),client)&&strictNumber(server.arg("token"),token)&&controlCanClaim(client,token);
    bool revisionOK=strictNumber(server.arg("stop_revision"),revision)&&revision>=0&&(uint32_t)revision==controlRevision;
    if(action!="arm" && action!="calib") {
      server.send(400,"application/json","{\"message\":\"Lệnh không hợp lệ.\"}");return;
    }
    if(action=="arm" && (!strictNumber(server.arg("controller"),controller) || controller!=12)) {
      server.send(400,"application/json","{\"message\":\"Firmware chỉ hỗ trợ R12.\"}");return;
    }
    if(action=="pulse" && (!strictNumber(server.arg("wheel"),wheel)||wheel<1||wheel>3||
       !strictNumber(server.arg("direction"),direction)||(direction!=1&&direction!=-1))) {
      server.send(400,"application/json","{\"message\":\"Bánh hoặc chiều quay không hợp lệ.\"}");return;
    }
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool fresh=balanceSeen && millis()-balanceReceived<500;
    bool matched=!strncmp(balanceMeta,"$BINFO,BAL-20261118-R12-8DIR-RT38-CONT,",sizeof("$BINFO,BAL-20261118-R12-8DIR-RT38-CONT,")-1);
    bool running=balanceLatest.state==3||balanceLatest.state==7;
    if(action!="calib" && !sessionOK) message="Trang chưa có kênh điều khiển đang hoạt động. Giữ trang này mở và kết nối lại Wi-Fi.";
    else if(action!="calib" && !revisionOK) message="Đã có lệnh DỪNG sau yêu cầu cũ. Chờ thông số mới rồi bấm lại nếu muốn chạy.";
    else if(!fresh) message="Chưa nhận dữ liệu mới từ STM32. Kiểm tra nguồn, UART hoặc chế độ đo linh kiện.";
    else if(!matched) message="STM32 chưa báo BAL-20261118-R12-8DIR-RT38-CONT. Cần nạp đúng phiên bản STM32.";
    else if(running) message="Motor đang chạy. Bấm DỪNG trước khi đổi bài.";
    else if(balanceDownloading) message="Đang tải log. Chờ tải xong.";
    else if(action=="arm" && (controller!=balanceLatest.controller || balanceMetaController()!=controller)) message="Chế độ đã thay đổi hoặc STM32 chưa xác nhận cấu hình. Chờ thông số mới rồi chọn/bấm lại.";
    else if(action!="calib" && (!balanceCapacity||balanceCount>=balanceCapacity)) message="Bộ nhớ log chưa sẵn sàng hoặc đã đầy. Tải và xóa log cũ; kiểm tra OPI PSRAM.";
    else if(action!="calib" && (balanceLatest.state==6||(balanceLatest.fault&(32|256|512)))) message="BMI160 hoặc dữ liệu đang lỗi. Kiểm tra cảm biến rồi đo lại gyro/khởi động lại STM32.";
    else if(action=="arm" && server.arg("surface")!="floor") message="Xác nhận hai bánh đã đặt trên nền phẳng trước khi bật cân bằng.";
    else if(action=="arm" && !balanceLatest.ready) message=String("STM32 chưa cho phép cân bằng. R")+String(balanceLatest.controller)+" cần góc trong ±"+"60"+" độ và yên 0,2 giây; xem điều kiện trên web.";
    else if(action=="pulse" && server.arg("fixture")!="yes") message="Xác nhận đã kê chắc thân xe và hai bánh quay tự do.";
    xSemaphoreGive(captureMutex);
    if(message.length()) {server.send(409,"application/json",String("{\"message\":\"")+message+"\"}");return;}
    if(action=="arm" || action=="pulse")controlClaim(client);
    if(action=="arm") {command="$ARM,"+String(controller)+","+String(controlDriveEpoch)+"*";message="Đã gửi BẬT CÂN BẰNG R"+String(controller)+"; đang chờ STM32 xác nhận.";}
    else if(action=="pulse") {command="$PULSE,"+String(wheel)+","+String(direction)+"*";message="Đã gửi bài thử bánh 1,5 giây; đang chờ PWM và encoder từ STM32.";}
    else {command="$CALIB*";message="Đã gửi ĐO LẠI GYRO. Cố định thân xe trong 5 giây.";}
    Serial1.print(command);Serial1.print("\r\n");
    server.send(200,"application/json",String("{\"message\":\"")+message+"\"}");
  });
  server.on("/balance-status",HTTP_GET,[](){
    char row[640],reply[2048];
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    balanceRow(row,sizeof(row),balanceLatest);
    snprintf(reply,sizeof(reply),"{\"seen\":%s,\"age_ms\":%lu,\"row\":\"%s\",\"meta\":\"%s\",\"samples\":%u,\"capacity\":%u,\"crc\":%lu,\"gaps\":%lu,\"overflow\":%lu,\"resets\":%lu}",
      balanceSeen?"true":"false",(unsigned long)(millis()-balanceReceived),row,balanceMeta,
      (unsigned)balanceCount,(unsigned)balanceCapacity,(unsigned long)balanceCRC,
      (unsigned long)balanceGaps,(unsigned long)balanceOverflow,(unsigned long)balanceResets);
    xSemaphoreGive(captureMutex);
    String status=String(reply);status.remove(status.length()-1);
    status+=",\"owner\":"+String(controlOwner)+",\"owner_token\":"+String(controlOwner>=0?controlTokens[controlOwner]:0)+",\"control_revision\":"+String(controlRevision)+",\"drive_epoch\":"+String(controlDriveEpoch)+",\"stop_source\":\""+String(controlLastStop)+"\"";
    /* A page that lost the gateway must be able to say "the ESP32 restarted",
     * not "the STM32 stopped sending". */
    status+=",\"boot_id\":"+String(gatewayBootId)+",\"uptime_ms\":"+String(millis())+",\"reset_reason\":\""+String(gatewayResetText(gatewayResetReason))+"\",\"ws_clients\":"+String(webSocket.connectedClients())+"}";
    server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",status);
  });
  server.on("/balance-clear",HTTP_POST,[](){
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool ok=balanceSeen && balanceLatest.state!=3 && balanceLatest.state!=7 && millis()-balanceReceived<500 && !balanceDownloading && (int32_t)(balancePostUntil-millis())<=0;
    if(ok) {controlStopCount=controlStopOverflow=0;strcpy(controlLastStop,"none");balanceCount=balanceCRC=balanceGaps=balanceOverflow=balanceResets=0;balancePostUntil=0;
#ifdef ARDUINO
      archiveGeneration++;gatewayStopRecord[0]=0;
#endif
    }
    xSemaphoreGive(captureMutex);
    server.send(ok?200:409,"text/plain",ok?"Cleared":"Stop balance and wait for a fresh STM32 sample first.");
  });
  server.on("/archive-format",HTTP_POST,[](){
    String message;
    long client=-1,token=0;
    bool identity=strictNumber(server.arg("session"),client)&&strictNumber(server.arg("token"),token)&&
      controlIdentity(client,token);
    bool confirmed=server.arg("confirm")=="ERASE";
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool idle=balanceSeen&&balanceLatest.state!=3&&balanceLatest.state!=7&&
      millis()-balanceReceived<500&&!balanceDownloading;
    bool usable=archiveMounted&&archiveError==0;
    xSemaphoreGive(captureMutex);
    if(!identity) message="Cần phiên điều khiển đang kết nối trên chính trang này. Giữ trang mở, kết nối lại Wi-Fi rồi thử lại.";
    else if(!confirmed) message="Thiếu xác nhận ERASE; không xóa vùng log.";
    else if(!idle) message="Dừng motor và chờ mẫu STM32 mới rồi thử lại.";
    else if(usable) message="Vùng log Flash đang dùng được; không cần định dạng.";
    if(message.length()) {server.send(409,"application/json",String("{\"message\":\"")+message+"\"}");return;}
    /* RT23: queue the format instead of running9.9MB f_mkfs on the web task. The
     * old call blocked this request for tens of seconds (and an operator retry
     * could start a second format), which is indistinguishable from a dead page
     * and loses the RAM log if the gateway restarts mid-format. */
    archiveFormatRequest=true;archiveFormatState=1;
    server.send(202,"application/json",String("{\"message\":\"")+
      "Đã xếp lịch định dạng lại vùng log Flash. Quá trình chạy trong tác vụ lưu (10-60 giây); giữ trang mở và xem archive_ready trong /status chuyển sang true, archive_format=3 là xong, 4 là lỗi."+"\"}");
  });
  server.on("/balance-log",HTTP_GET,[](){
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool ok=balanceCount && balanceLatest.state!=3 && balanceLatest.state!=7 && millis()-balanceReceived<500 && !balanceDownloading && (int32_t)(balancePostUntil-millis())<=0;
    if(!ok) {xSemaphoreGive(captureMutex);server.send(409,"text/plain","Press STOP, wait 2 seconds, then download.");return;}
    balanceDownloading=true;
    size_t count=balanceCount;
    uint8_t logController=balanceFrames[0].controller;
    for(size_t i=1;i<count;i++)if(balanceFrames[i].controller!=logController){logController=0;break;}
    String filename=logController?String("twip_balance_R")+String(logController)+".txt":"twip_balance_MIXED.txt";
    String header="# TWIP_BALANCE_RT4_V1; frame122 sync0x60a5 50Hz; build BAL-20261118-R12-8DIR-RT38-CONT\n# last_live_meta (not necessarily the configuration of every stored row): ";
    header+=balanceMeta;
#ifdef ARDUINO
    header+="\n# GATEWAY_NET1 build="+String(TWIP_WEB_BUILD)+" boot="+String(gatewayBootId)+" reset="+String(gatewayResetText(gatewayResetReason))+" heap_min="+String(ESP.getMinFreeHeap())+" archive_saved="+String((unsigned long)archiveSaved)+" archive_error="+String((unsigned long)archiveError)+"\n";
#endif
header+="\n# RT19 HOLD80: third preset is80cm/s (was95). Measured floor slope puts PWM2499 at ~0.96m/s, so95 needed97-99%duty and left no balance authority. Post-turn braking now follows the measured yaw for at most0.6s and then freezes the heading anchor; the old rule required 100ms below0.08rad/s with no bound, so a residual rate kept the anchor chasing the body (log(13)(2): +59deg in4.9s on a straight command). After a release the position anchor is captured once more200ms after the brake silence, when the body is genuinely still; log(13)(2) showed a9cm roll-back after the old early capture, which left a permanent offset and a steady +240PWM push. RT18 motion-ready, blockage evidence, governor, R12 gain/observer/allocator, encoder signs and wiring are unchanged.\n";
header+="\n# RT20 POSTREC: the post-REC1 speed softening is bounded to1.5s. Previously a press made while REC1 was still active consumed the new-gesture evidence, so the vehicle stayed capped at0.50mps for the rest of the run (log L2: 7s at0.50 with preset80 after a190dps bump). A direction/turn change still clears it immediately; otherwise the selected preset returns1.5s after recovery ends. No gain/observer/allocator/encoder change.\n";
header+="\n# RT21 ANTISTALL: the headroom shaper no longer treats tracking lag as missing headroom while the reference is still accelerating, and a frozen reference below0.20mps with a stationary vehicle may crawl at0.01mps so static friction can be broken. Log RT20 showed v_ref stuck at0.09-0.14mps for up to1.64s and v_ref<0.60 for51-64% of forward time. Duty>0.90, the lean predictor, retreat, the motion blockage detector and all other limits are unchanged.\n";
header+="\n# RT22 SYM: the headroom governor's lean rule moved from5deg to9deg (11 while launching) and no longer freezes the reference. Under pressure it caps the commanded acceleration at0.06mps2 instead of holding v_ref, so a held command keeps being obeyed; log RT21 showed forward frozen for0.5-4.8s with70% pressure versus3.7% in reverse. The learned neutral posture (0.5s dwell+60 samples, kept across STOP/ARM) now centres the predictor. Steering: pure turn1.3/2.4radps, diagonal0.90/1.15radps, yaw accel4.5radps2, lateral ceiling0.9mps2 (was0.4, which capped every diagonal turn at0.5radps when moving0.8mps). Duty>0.90, the tracking-lag rule, wheel-speed cap, lean taper, blockage detector, REC1 and the R12 force law are unchanged.\n";
header+="\n# RT23 SYM2: two left/right turn scaling bugs, no gain change. (1) The wheel-speed cap evaluated the MEASURED axle speed against the0.80 AXLE ceiling, so at preset72 a diagonal held0.51-0.69radps against1.15 requested (flag512 63-100%) while the same gesture at80 reached1.15. It now uses the commanded speed and a per-wheel envelope (50:0.80, 72:0.98, 80:1.05). (2) The centre-drift taper counted the2-3cm/s creep of a normal pivot (body leans+4.8..+5.9deg turning left, -2.6..-4.2 right; drift plateaus15-17cm), so left turns faded2.40->1.43radps while right held2.40. Knee22cm->45cm, floor40%->55%. The speed interlock no longer cuts the motors on a transient: it needs200ms above a ceiling that follows the command(|v_ref|+0.12), with the hard runaway bound1.45mps. Both RT22 falls at preset72 tripped fault16 one tick after a turn release (yaw brake surged the axle0.74->0.86mps, PWM went0/0 at pitch1.8deg, body fell0->105deg in0.24s). R12 gains, observer, allocator, governor, REC1 and the122-byte frame are unchanged.\n";
    header+="\n# RT24 LEAN: the turn lean taper acted on ABSOLUTE pitch with a6deg knee, but a full2.40radps pivot on the car rides at pitch+6.5deg turning left and -2.2deg turning right (log RT24: BINFO equilibrium+1.074deg qualified, left w_ref median2.18 with58% of samples below2.38radps, right2.40 with18%). The taper now uses |pitch - equilibrium|: full authority up to8deg of lean beyond the neutral posture, linear cut8-to-12deg, zero beyond; a lean of10deg absolute with a0 reference still gives0.50. Nothing else changed: R12 gains/observer/allocator, governor, REC1, wheel envelope, lateral ceiling0.9, centre knee45cm, blockage, 122-byte frame. Replay of the recorded lean trace: left2.40/right2.40 (RT23 rule: left1.95 median,1.37 min).\n";
header+="\n# RT27 RECOVER: push handling is the RT21-ANTISTALL design again. RT25/RT26 added a CATCH that set v_ref:=measured speed and a drive-home assist that moved x_ref; the RT26 logs show the result: with a neutral command v_ref followed the body to-0.87mps, so the R12 law kept driving the vehicle in the direction it was already rolling, the vehicle ran away and fell (pitch-25..-53deg with duty already saturated). Replaying the recorded window before that fall through the R12 request formula, the plain RT21 references brake up to5.3N harder than the catch did. Both blocks are removed: the anchor and the velocity reference stay where the drive command put them, exactly as in RT21-ANTISTALL. Push recovery is REC1 again (50deg,1.2mps,+0.8m travel) plus two guards that only remove spurious motor cuts: the RT23 200ms speed-interlock debounce and REC1 keeping its priming after a settled exit. The RT23/REC1 change in log RT26 was a fault8 tracking cut at |x-xref|=0.378m while the body was still only13.8deg; with the RT21 0.8m allowance that cut no longer fires. Nothing else changed: R12 gains/observer/allocator, motion rate limits, turn law, governor and the122-byte frame.\n";
header+="\n# NET21 LINK: boot no longer scans the9.9MB FAT partition when the volume mounts (that scan cost seconds before the SoftAP existed, so a brownout looked like a dead page); the AP now starts before the flash log. The page closes a silent WebSocket after3s without a gateway frame and retries after0.5s,1s,2s,4s (capped); the gateway sends a1Hz hb frame and a20s/8s ping backstop. An ESP32 restart is reported as a gateway restart with boot_id/reset_reason instead of STM32 silence. An unmountable log volume can be reformatted from the page (/archive-format, explicit ERASE, motor stopped); listed archives use the bare file name. STM32 control law, R12 gains, observer, encoder and the122-byte frame are unchanged.\n";
    header+="\n# RT17 SYNC: governor predictor centered on qualified neutral posture (clamped+-2deg, near-zero deadzone0.5deg); does NOT change CF/force gains. BINFO fields24/25=governor equilibrium_deg/qualified. Abort confirmation200ms completes readiness without a second500ms hurdle; fresh neutral/new gesture still required. REC1 entry always starts the same bounded braking anchor for held or neutral input; anchor stays fixed after vref reaches zero. Latest fresh MOVE8 may run at<=0.25mps/yaw0.4radps only when REC1 pitch<=6deg/gyro<=25dps/speed and ref<=0.35mps; otherwise balance first. Flag32768=REC1/post-REC gesture speed limit. After REC1 a continuing held gesture caps at0.5mps/yaw0.8radps; new normal-state gesture restores selected preset. Neutral cancels current intent immediately; no stale gesture replay. Moderate gyro>250dps+norm>2g/<0.4g requests bounded REC1, not a motor cut; hard gyro>900dps and other hard faults retained. Early speed reserve covers uncommanded overspeed before ordinary ceiling. Failed gyro calibration keeps prior valid bias but locks ARM.\n";
    header+="\n# RT16 STABLE: fixed abort completion after neutral, fresh neutral/new press after abort; quiet brake angle6deg. Ordinary release brake bound1.25m/4s, cancelled/blockage bound1m/4s; no extension of active recovery/startup. Encoder position from counts, same physical frame as force controller. BINFO fields20-23=I2C read_errors/retry_ok/last_error/last_error_ms since boot (counts saturate65535); bit31=BUSY precheck; other error bits=STM32 HAL. One fast NACK retry only, no stale sample. Explicit CALIB while stopped clears bus, verifies configuration/PMU and recalibrates; no automatic motor resume. Low-speed lean hysteresis8deg entry,5.5deg predicted and6deg body exit,100ms all presets; high-speed gates unchanged.\n";
    header+="\n# RT15 reference rate retained: preset72/MAX80 acceleration caps0.50mps2 through0.50mps,0.22mps2 above; preset50 unchanged. Retained jerk2mps3, RT14 neutral monotonicity, pressure/lean/duty gates,300ms high-speed recovery dwell and all fault limits. Reference rates are conditional, not guaranteed vehicle acceleration.\n";
    header+="\n# RT14 release fix retained: neutral cannot increase abs(v_ref). R12 gains/observer/allocator unchanged. Left red M1->AOUT2,white M2->AOUT1; right red->BOUT1,white->BOUT2; encoder signs unchanged.\n";
    header+="\n# RT13 reference shaper retained: flag16384=headroom/lean constrained, target50/80/90 stands selected; braking<=0.65mps2. This is an acceleration limiter, not network delay.\n";
    header+="\n# acc correction (raw/4096-b)/s: b=-0.0416836098593,-0.105884356548,0.004738195334; s=0.997272837552,1.00758977736,1.00929388014; positive Y gain\n";
    header+="# pitch=CF(alpha=0.998, +gyroY, -(atan2(correctedAx,correctedAz)-2.166942649deg)); trim=0; dt=0.005; encoder meanPPR=898.225 beta=0.75; right sign=-1\n";
    header+="# BINFO fields17/18/19=controller/CFalpha/ARM_angle_limit_deg; R12 five values=Kx,Kv,Ktheta,Komega,observer_blend (SI units, force N); NOT PID. Frozen VM reference11.7V, not measured live.\n";
    header+="# R12: nonlinear per-wheel force allocation, directional startup/running friction and back-EMF; NO additional PID deadband; yaw PI/PD with antiwindup, cap0.08Nm; balance-force priority.\n";
    header+="# R12 telemetry: v_control=observer axle speed m/s; theta_control=observer body angle deg; pwm_base=mean final PWM; integral=yaw integral Nm, cap0.04. Brake on first ARM tick.\n";
    header+="# ARM abs(CF pitch)<=60deg for fixed R12 baseline, each gyro<6dps and each wheel<0.05m/s for200ms. Operator reports successful60deg starts; not a guarantee for impacts/slopes. Previous high-angle predictor remains unvalidated. Frozen R12 force law; only ARM>58deg extends startup limits.\n";
    header+="# 50Hz decimated control snapshots; control200Hz IMU100Hz unsynchronised. PWM is command AFTER this sample; raw gyro/16.4 includes bias. No voltage/current/load measurement.\n";
    header+="# MOVE8 direction/turn=-1,0,+1; turn positive=body left. RT23: pure turn normal1.3/fast2.4rad/s; diagonal normal0.90/fast1.15rad/s before wheel-speed/lateral/lean governors (wheel envelope50:0.80,72:0.98,80:1.05m/s per wheel, evaluated on the commanded speed); yaw accel4.5/reversal-decel8rad/s2; lateral ceiling0.9m/s2; RT24: the lean taper acts on the lean BEYOND the learned neutral posture (full authority to8deg of excess, cut8-to-12, zero beyond), so a pivot's own lean no longer reduces its rate; centre-drift taper knee45cm/floor55%. Neutral sets w_ref=0 immediately; bounded rate feedback brakes physical rotation. During a held turn psi_ref follows measured encoder yaw; after release it anchors only after quiet. Neutral releases both axes; stale yaw requires neutral before new press.\n";
    header+="# R12 BASE60: distance budget=min(1,max(.3,.3+.014*(abs(ARM_CF_angle)-10)))m. Capture starts above20deg, angle limit=max(25,abs(ARM_angle)+5), ends after200ms abs(pitch)<=8/gyro<25/speed<.15, timeout3s through58deg ARM,4s only for ARM>58deg; fault2048. Capture speed ceiling1m/s through48deg ARM,1.1m/s through58deg ARM,1.2m/s only for ARM>58deg; after capture tilt stop25deg and speed0.8m/s.\n";
    header+="# state3 balance; state7 fixed motor pulse +/-600 for max1500ms, no PID. fault bits: 1 STOP,2 heartbeat timeout,4 tilt exceeds active limit,8 relative tracking error exceeded active budget,16 speed exceeds active ceiling (capture1/1.1/1.2m/s for ARM<=48/<=58/>58deg,normal0.8m/s, RT23: ceiling=max(preset,abs(v_ref)+0.12), trip needs200ms at200Hz, instant only above1.45m/s),32 I2C,64 loop timing,128 gyro calibration,256 IMU config,512 nonfinite,1024 fixture motion during pulse,2048 startup capture timeout3s through58deg ARM,4s above58deg,4096 disturbance recovery timeout4s,8192 extreme body rate after motion (gyro>900dps); the250dps+norm event now requests REC1; motor off, manual ARM only\n";
    header+="# REC1 control_phase:0 normal,1 startup,2 disturbance recovery. After500ms steady: idle pitch trigger8deg; an acknowledged fresh maneuver/braking uses12deg. RT8: gyro trigger150dps (was60, log27 bumps reached83); speed error.35m/s unchanged. Temporary limits50deg/1.2mps/min(1,max(base_budget,abs(entry_position)+.8))m, timeout4s without refresh. Exit300ms quiet: pitch<=6deg, gyro<25dps, abs(speed-vref)<.15mps AND abs(x-xref)<=base_budget. No encoder reset or gain change.\n";
    header+="# RT11: MAX90 preset2 flag4096, max envelope flag8192; normal/fast50/80 unchanged. Selected MAX envelope mean1.12mps each wheel1.20mps, retained during deceleration to another preset. RT4: hold50/72cmps, default50; speed preset is explicit in each MOVE8 packet, flag256=fast72 selected. Accel0.60mps2 below0.50mps,0.30above; neutral-decel0.90mps2/reverse-decel1.0mps2; continuous commanded travel. 350ms MOVE lease independent of PING. Flags:1ready,2moving,4braking,8tracking_limited,16release_required,32has_moved,64cancelled_old_target,128tracking_blockage,256fast_preset,512yaw_governor_limited,1024linear_release_braking,2048yaw_rate_control. New press/reverse discards old position and velocity-ramp debt, initializing velocity from measured speed; neutral abandons old target, follows axle only during velocity ramp, then freezes a braking anchor; final hold captures after100ms speed<0.06mps/pitch<=6deg/gyro<25dps. Pure turns capture one linear center at gesture entry and retain R12 position feedback; low-speed blockage requires a translation command, pure-turn center drift progressively governs yaw rate instead of falsely classifying translation blockage. The20cm commanded-translation lead and ordinary distance protections remain. Held yaw uses bounded rate PI; release clears the gesture integral and sets zero yaw-rate target, then holds a new heading. Ordinary release braking also has an independent1.25m/4s envelope. drive_sequence/drive_rx_ms acknowledge actual STM32 parsing; PING does not refresh them. Recovery after motion or sustained tracking failure abandons old position; velocity reference is initialized from measured speed then ramps to zero while xref follows the body during active recovery or a nonzero velocity ramp, then fixes the anchor to remove residual drift; blockage abort ends after200ms quiet, requires fresh neutral/new press, without a second readiness dwell. No automatic resumption; a new press is required. High-speed reserve predictor enters the same bounded REC1 only when primed, abs(vref)>0.55mps, abs(vactual)>0.78mps and abs(vactual-vref)>0.04mps in the overspeed direction. The0.8mps ordinary cutoff is unchanged. No encoder/observer reset. An unsettled cancellation has an independent1m/4s envelope from the first abort sample; re-entry cannot refresh it, even while xref follows the body. A200ms quiet body/wheel/yaw confirmation can end a completed abort while still requiring neutral/new press; a PING alone cannot end it.\n";
    header+="# log first 600s cumulative active+2s tail; full freezes storage (does not overwrite), live display continues; download requires STOP. Multiple arms require inspecting state/seq boundaries.\n";
    header+="# samples="+String((unsigned)count)+" crc_errors="+String(balanceCRC)+" sequence_gaps="+String(balanceGaps)+" overflow="+String(balanceOverflow)+" STM32_resets="+String(balanceResets)+"\n";
    header+="# GSTOP: gateway_ms,last_seen_STM32_ms,last_seen_seq,source; gateway command evidence, not an STM32 acknowledgment. First STOP per arm/pulse; max32 records.\n";
    for(unsigned i=0;i<controlStopCount;i++)header+=controlStops[i];
    header+="# gateway_stop_events_dropped="+String(controlStopOverflow)+"\n";
    header+="seq,t_ms,dt_us,execution_us,state,ready,fault,pwm_l,pwm_r,ticks_l,ticks_r,gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw,pitch_deg,gyroY_corrected_dps,v_l_mps,v_r_mps,x_encoder_m,control_v_mps,control_theta_deg,control_pwm,control_integral,stm32_drops,controller,control_phase,distance_limit_m,x_ref_m,v_ref_mps,drive_epoch,motion_flags,drive_direction,psi_ref_rad,w_ref_radps,turn_direction,drive_sequence,drive_rx_ms\n";
    xSemaphoreGive(captureMutex);
    // A raw socket body must have a fixed length, never a chunked HTTP header.
    // Frames are immutable here: motors stopped, tail complete, new ARM/clear locked.
    char footer[80],row[520];
    int footerLength=snprintf(footer,sizeof(footer),"# END balance rows=%u\n",(unsigned)count);
    size_t bytes=header.length()+(size_t)footerLength;
    bool valid=true;
    for(size_t i=0;i<count;i++) {
      int n=balanceExportRow(row,sizeof(row),balanceFrames[i]);
      if(n<0){valid=false;break;}bytes+=(size_t)n;
      if(i%64==0)yield();
    }
    if(!valid) {
      xSemaphoreTake(captureMutex,portMAX_DELAY);balanceDownloading=false;xSemaphoreGive(captureMutex);
      server.send(500,"text/plain","Log row exceeds export buffer; log retained.");return;
    }
    server.sendHeader("Content-Disposition",String("attachment; filename=")+filename);
    server.sendHeader("Cache-Control","no-store");server.setContentLength(bytes);
    server.send(200,"text/plain; charset=utf-8",header);WiFiClient client=server.client();
    bool sent=true;char block[2048];size_t used=0;
    for(size_t i=0;sent&&i<count;i++) {
      int n=balanceExportRow(row,sizeof(row),balanceFrames[i]);
      if(n<0){sent=false;break;}
      if(used+(size_t)n>sizeof(block)){sent=sendMeasurementBytes(client,block,used);used=0;}
      if(sent){memcpy(block+used,row,(size_t)n);used+=(size_t)n;}
    }
    if(sent&&used)sent=sendMeasurementBytes(client,block,used);
    if(sent)sent=sendMeasurementBytes(client,footer,(size_t)footerLength);
    if(!sent)client.stop(); // Preserve stored frames so a failed transfer can be retried.
    xSemaphoreTake(captureMutex,portMAX_DELAY);balanceDownloading=false;xSemaphoreGive(captureMutex);
  });
}
