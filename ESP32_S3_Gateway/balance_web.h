#pragma once
// Included after measurement_web.h, using its bounded socket writer.
static int balanceRow(char *out,size_t n,const BalanceFrame &f) {
  return snprintf(out,n,"%lu,%lu,%u,%u,%u,%u,%u,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u,%u,%u,%.4f,%.6f,%.6f,%lu,%u,%d,%.6f,%.6f,%d",
    (unsigned long)f.sequence,(unsigned long)f.t_ms,f.dt_us,f.execution_us,f.state,f.ready,f.fault,
    f.pwm_l,f.pwm_r,(long)f.ticks_l,(long)f.ticks_r,f.gx,f.gy,f.gz,f.ax,f.ay,f.az,
    f.pitch,f.gyro,f.v_l,f.v_r,f.x,f.v_cmd,f.theta_cmd,f.pwm_base,f.integral,f.dropped,f.controller,f.capture,f.distance_limit,f.x_ref,f.v_ref,(unsigned long)f.drive_epoch,f.motion_flags,f.drive_direction,f.psi_ref,f.w_ref,f.turn_direction);
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
    if(action!="arm" && action!="pulse" && action!="calib") {
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
    bool matched=!strncmp(balanceMeta,"$BINFO,BAL-20261018-R12-SPEED2-8DIR,",sizeof("$BINFO,BAL-20261018-R12-SPEED2-8DIR,")-1);
    bool running=balanceLatest.state==3||balanceLatest.state==7;
    if(action!="calib" && !sessionOK) message="Trang chưa có kênh điều khiển đang hoạt động. Giữ trang này mở và kết nối lại Wi-Fi.";
    else if(action!="calib" && !revisionOK) message="Đã có lệnh DỪNG sau yêu cầu cũ. Chờ thông số mới rồi bấm lại nếu muốn chạy.";
    else if(!fresh) message="Chưa nhận dữ liệu mới từ STM32. Kiểm tra nguồn, UART hoặc chế độ đo linh kiện.";
    else if(!matched) message="STM32 chưa báo BAL-20261018-R12-SPEED2-8DIR. Cần nạp đúng phiên bản STM32.";
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
    status+=",\"owner\":"+String(controlOwner)+",\"owner_token\":"+String(controlOwner>=0?controlTokens[controlOwner]:0)+",\"control_revision\":"+String(controlRevision)+",\"drive_epoch\":"+String(controlDriveEpoch)+",\"stop_source\":\""+String(controlLastStop)+"\"}";
    server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",status);
  });
  server.on("/balance-clear",HTTP_POST,[](){
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool ok=balanceSeen && balanceLatest.state!=3 && balanceLatest.state!=7 && millis()-balanceReceived<500 && !balanceDownloading && (int32_t)(balancePostUntil-millis())<=0;
    if(ok) {controlStopCount=controlStopOverflow=0;strcpy(controlLastStop,"none");balanceCount=balanceCRC=balanceGaps=balanceOverflow=balanceResets=0;balancePostUntil=0;}
    xSemaphoreGive(captureMutex);
    server.send(ok?200:409,"text/plain",ok?"Cleared":"Stop balance and wait for a fresh STM32 sample first.");
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
    String header="# TWIP_BALANCE_8DIR_V1; frame114 sync0x5fa5 50Hz; build BAL-20261018-R12-SPEED2-8DIR\n# last_live_meta (not necessarily the configuration of every stored row): ";
    header+=balanceMeta;
    header+="\n# acc correction (raw/4096-b)/s: b=-0.0416836098593,-0.105884356548,0.004738195334; s=0.997272837552,1.00758977736,1.00929388014; positive Y gain\n";
    header+="# pitch=CF(alpha=0.998, +gyroY, -(atan2(correctedAx,correctedAz)-2.166942649deg)); trim=0; dt=0.005; encoder meanPPR=898.225 beta=0.75; right sign=-1\n";
    header+="# BINFO fields17/18/19=controller/CFalpha/ARM_angle_limit_deg; R12 five values=Kx,Kv,Ktheta,Komega,observer_blend (SI units, force N); NOT PID. Frozen VM reference11.7V, not measured live.\n";
    header+="# R12: nonlinear per-wheel force allocation, directional startup/running friction and back-EMF; NO additional PID deadband; yaw PI/PD with antiwindup, cap0.08Nm; balance-force priority.\n";
    header+="# R12 telemetry: v_control=observer axle speed m/s; theta_control=observer body angle deg; pwm_base=mean final PWM; integral=yaw integral Nm, cap0.04. Brake on first ARM tick.\n";
    header+="# ARM abs(CF pitch)<=60deg for fixed R12 baseline, each gyro<6dps and each wheel<0.05m/s for200ms. Operator reports successful60deg starts; not a guarantee for impacts/slopes. Previous high-angle predictor remains unvalidated. Frozen R12 force law; only ARM>58deg extends startup limits.\n";
    header+="# 50Hz decimated control snapshots; control200Hz IMU100Hz unsynchronised. PWM is command AFTER this sample; raw gyro/16.4 includes bias. No voltage/current/load measurement.\n";
    header+="# MOVE8 direction/turn=-1,0,+1; turn positive=body left. Pure turn cap0.30rad/s; diagonal normal0.20/fast0.15rad/s; yaw accel0.50/decel0.80rad/s2. psi_ref and w_ref are targets, not measured heading. Neutral releases both axes; stale yaw requires neutral before new press.\n";
    header+="# R12 BASE60: distance budget=min(1,max(.3,.3+.014*(abs(ARM_CF_angle)-10)))m. Capture starts above20deg, angle limit=max(25,abs(ARM_angle)+5), ends after200ms abs(pitch)<=8/gyro<25/speed<.15, timeout3s through58deg ARM,4s only for ARM>58deg; fault2048. Capture speed ceiling1m/s through48deg ARM,1.1m/s through58deg ARM,1.2m/s only for ARM>58deg; after capture tilt stop25deg and speed0.8m/s.\n";
    header+="# state3 balance; state7 fixed motor pulse +/-600 for max1500ms, no PID. fault bits: 1 STOP,2 heartbeat timeout,4 tilt exceeds active limit,8 relative tracking error exceeded active budget,16 speed exceeds active ceiling (capture1/1.1/1.2m/s for ARM<=48/<=58/>58deg,normal0.8m/s),32 I2C,64 loop timing,128 gyro calibration,256 IMU config,512 nonfinite,1024 fixture motion during pulse,2048 startup capture timeout3s through58deg ARM,4s above58deg,4096 disturbance recovery timeout4s,8192 suspected severe impact after motion (gyro>900dps or gyro>250dps with accel norm>2g/<0.4g); motor off, manual ARM only\n";
    header+="# REC1 control_phase:0 normal,1 startup,2 disturbance recovery. After500ms steady: abs(pitch)>=8deg or abs(gyro)>=60dps or abs(speed-vref)>=.35m/s may start recovery. Temporary limits50deg/1.2mps/min(1,max(base_budget,abs(entry_position)+.8))m, timeout4s without refresh. Exit300ms quiet: pitch<=6deg, gyro<25dps, abs(speed-vref)<.15mps AND abs(x-xref)<=base_budget. No encoder reset or gain change.\n";
    header+="# SPEED2: hold40/60cmps, default40; speed preset is explicit in each MOVE packet, flag256=fast60 selected. Accel0.20/decel0.30mps2; continuous commanded travel. 350ms MOVE lease independent of PING. Flags:1ready,2moving,4braking,8tracking_limited,16release_required,32has_moved,64cancelled_old_target,128tracking_blockage,256fast_preset. Recovery after motion or sustained tracking failure abandons old position; velocity reference is initialized from measured speed then ramps to zero while xref follows the body until500ms quiet and a release. No automatic resumption; a new press is required. No encoder/observer reset. Cancellation has an independent1m/4s envelope from the first abort sample; re-entry cannot refresh it, even while xref follows the body.\n";
    header+="# log first 180s cumulative active+2s tail; full freezes storage (does not overwrite), live display continues; download requires STOP. Multiple arms require inspecting state/seq boundaries.\n";
    header+="# samples="+String((unsigned)count)+" crc_errors="+String(balanceCRC)+" sequence_gaps="+String(balanceGaps)+" overflow="+String(balanceOverflow)+" STM32_resets="+String(balanceResets)+"\n";
    header+="# GSTOP: gateway_ms,last_seen_STM32_ms,last_seen_seq,source; gateway command evidence, not an STM32 acknowledgment. First STOP per arm/pulse; max32 records.\n";
    for(unsigned i=0;i<controlStopCount;i++)header+=controlStops[i];
    header+="# gateway_stop_events_dropped="+String(controlStopOverflow)+"\n";
    header+="seq,t_ms,dt_us,execution_us,state,ready,fault,pwm_l,pwm_r,ticks_l,ticks_r,gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw,pitch_deg,gyroY_corrected_dps,v_l_mps,v_r_mps,x_encoder_m,control_v_mps,control_theta_deg,control_pwm,control_integral,stm32_drops,controller,control_phase,distance_limit_m,x_ref_m,v_ref_mps,drive_epoch,motion_flags,drive_direction,psi_ref_rad,w_ref_radps,turn_direction\n";
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
