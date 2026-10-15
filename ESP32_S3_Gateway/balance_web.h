#pragma once
// Included after measurement_web.h, using its bounded socket writer.
static int balanceRow(char *out,size_t n,const BalanceFrame &f) {
  return snprintf(out,n,"%lu,%lu,%u,%u,%u,%u,%u,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u",
    (unsigned long)f.sequence,(unsigned long)f.t_ms,f.dt_us,f.execution_us,f.state,f.ready,f.fault,
    f.pwm_l,f.pwm_r,(long)f.ticks_l,(long)f.ticks_r,f.gx,f.gy,f.gz,f.ax,f.ay,f.az,
    f.pitch,f.gyro,f.v_l,f.v_r,f.x,f.v_cmd,f.theta_cmd,f.pwm_base,f.integral,f.dropped);
}
static int balanceExportRow(char *out,size_t n,const BalanceFrame &f) {
  if(n<2)return -1;
  int used=balanceRow(out,n-1,f);
  if(used<0 || (size_t)used>=n-1)return -1;
  out[used++]='\n';return used;
}
void setupBalanceRoutes() {
  server.on("/operate",HTTP_POST,[](){
    String action=server.arg("action"),command,message;
    if(action=="stop") {
      controlStop("stop_button_http");
      server.send(200,"application/json","{\"message\":\"Đã gửi DỪNG tới STM32. Kiểm tra PWM về 0.\"}");return;
    }
    long wheel=0,direction=0,client=-1,token=0,revision=-1;
    bool sessionOK=strictNumber(server.arg("session"),client)&&strictNumber(server.arg("token"),token)&&controlCanClaim(client,token);
    bool revisionOK=strictNumber(server.arg("stop_revision"),revision)&&revision>=0&&(uint32_t)revision==controlRevision;
    if(action!="arm" && action!="pulse" && action!="calib") {
      server.send(400,"application/json","{\"message\":\"Lệnh không hợp lệ.\"}");return;
    }
    if(action=="pulse" && (!strictNumber(server.arg("wheel"),wheel)||wheel<1||wheel>3||
       !strictNumber(server.arg("direction"),direction)||(direction!=1&&direction!=-1))) {
      server.send(400,"application/json","{\"message\":\"Bánh hoặc chiều quay không hợp lệ.\"}");return;
    }
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool fresh=balanceSeen && millis()-balanceReceived<500;
    bool matched=!strncmp(balanceMeta,"$BINFO,BAL-20261015-R12,",sizeof("$BINFO,BAL-20261015-R12,")-1);
    bool running=balanceLatest.state==3||balanceLatest.state==7;
    if(action!="calib" && !sessionOK) message="Trang chưa có kênh điều khiển đang hoạt động. Giữ trang này mở và kết nối lại Wi-Fi.";
    else if(action!="calib" && !revisionOK) message="Đã có lệnh DỪNG sau yêu cầu cũ. Chờ thông số mới rồi bấm lại nếu muốn chạy.";
    else if(!fresh) message="Chưa nhận dữ liệu mới từ STM32. Kiểm tra nguồn, UART hoặc chế độ đo linh kiện.";
    else if(!matched) message="STM32 chưa báo BAL-20261015-R12. Cần nạp đúng phiên bản STM32.";
    else if(running) message="Motor đang chạy. Bấm DỪNG trước khi đổi bài.";
    else if(balanceDownloading) message="Đang tải log. Chờ tải xong.";
    else if(action!="calib" && (!balanceCapacity||balanceCount>=balanceCapacity)) message="Bộ nhớ log chưa sẵn sàng hoặc đã đầy. Tải và xóa log cũ; kiểm tra OPI PSRAM.";
    else if(action!="calib" && (balanceLatest.state==6||(balanceLatest.fault&(32|256|512)))) message="BMI160 hoặc dữ liệu đang lỗi. Kiểm tra cảm biến rồi đo lại gyro/khởi động lại STM32.";
    else if(action=="arm" && server.arg("surface")!="floor") message="Xác nhận hai bánh đã đặt trên nền phẳng trước khi bật cân bằng.";
    else if(action=="arm" && !balanceLatest.ready) message="STM32 chưa cho phép cân bằng. Giữ thân xe trong ±3 độ và yên 0,2 giây; xem điều kiện còn thiếu trên web.";
    else if(action=="pulse" && server.arg("fixture")!="yes") message="Xác nhận đã kê chắc thân xe và hai bánh quay tự do.";
    xSemaphoreGive(captureMutex);
    if(message.length()) {server.send(409,"application/json",String("{\"message\":\"")+message+"\"}");return;}
    if(action=="arm" || action=="pulse")controlClaim(client);
    if(action=="arm") {command="$ARM*";message="Đã gửi BẬT CÂN BẰNG; đang chờ STM32 xác nhận.";}
    else if(action=="pulse") {command="$PULSE,"+String(wheel)+","+String(direction)+"*";message="Đã gửi bài thử bánh 1,5 giây; đang chờ PWM và encoder từ STM32.";}
    else {command="$CALIB*";message="Đã gửi ĐO LẠI GYRO. Cố định thân xe trong 5 giây.";}
    Serial1.print(command);Serial1.print("\r\n");
    server.send(200,"application/json",String("{\"message\":\"")+message+"\"}");
  });
  server.on("/balance-status",HTTP_GET,[](){
    char row[512],reply[1600];
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    balanceRow(row,sizeof(row),balanceLatest);
    snprintf(reply,sizeof(reply),"{\"seen\":%s,\"age_ms\":%lu,\"row\":\"%s\",\"meta\":\"%s\",\"samples\":%u,\"capacity\":%u,\"crc\":%lu,\"gaps\":%lu,\"overflow\":%lu,\"resets\":%lu}",
      balanceSeen?"true":"false",(unsigned long)(millis()-balanceReceived),row,balanceMeta,
      (unsigned)balanceCount,(unsigned)balanceCapacity,(unsigned long)balanceCRC,
      (unsigned long)balanceGaps,(unsigned long)balanceOverflow,(unsigned long)balanceResets);
    xSemaphoreGive(captureMutex);
    String status=String(reply);status.remove(status.length()-1);
    status+=",\"owner\":"+String(controlOwner)+",\"owner_token\":"+String(controlOwner>=0?controlTokens[controlOwner]:0)+",\"control_revision\":"+String(controlRevision)+",\"stop_source\":\""+String(controlLastStop)+"\"}";
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
    String header="# TWIP_BALANCE_V2; EXPERIMENTAL candidate BAL-20261015-R12, not hardware certified\n# ";
    header+=balanceMeta;
    header+="\n# acc correction (raw/4096-b)/s: b=-0.0416836098593,-0.105884356548,0.004738195334; s=0.997272837552,1.00758977736,1.00929388014; positive Y gain\n";
    header+="# pitch=CF(0.998, +gyroY, -(atan2(correctedAx,correctedAz)-2.166942649deg)); trim=0; dt=0.005; encoder meanPPR=898.225 beta=0.75; right sign=-1\n";
    header+="# R12 STATE4_OBS14: $BINFO five values=Kx,Kv,Ktheta,Komega,observer_blend (SI units, force N); NOT PID. Frozen VM reference11.7V, not measured live.\n";
    header+="# R12: nonlinear per-wheel force allocation, directional startup/running friction and back-EMF; NO additional PID deadband; yaw PI/PD with antiwindup, cap0.08Nm; balance-force priority.\n";
    header+="# R12 telemetry: v_control=observer axle speed m/s; theta_control=observer body angle deg; pwm_base=mean final PWM; integral=yaw integral Nm, cap0.04. Brake on first ARM tick.\n";
    header+="# ARM abs(pitch)<=3deg, each gyro<6dps and each wheel<0.05m/s for200ms. Experimental: weak-motor uncertainty cases still fail simulation; not hardware certified.\n";
    header+="# 100Hz decimated control snapshots; control200Hz IMU100Hz unsynchronised. PWM is command AFTER this sample; raw gyro/16.4 includes bias. No voltage/current/load measurement.\n";
    header+="# state3 balance; state7 fixed motor pulse +/-600 for max1500ms, no PID. fault bits: 1 STOP,2 heartbeat timeout,4 tilt>25deg,8 encoder distance>0.30m,16 speed>0.80m/s,32 I2C,64 loop timing,128 gyro calibration,256 IMU config,512 nonfinite,1024 fixture motion during pulse\n";
    header+="# log first 180s cumulative active+2s tail; full freezes storage (does not overwrite), live display continues; download requires STOP. Multiple arms require inspecting state/seq boundaries.\n";
    header+="# samples="+String((unsigned)count)+" crc_errors="+String(balanceCRC)+" sequence_gaps="+String(balanceGaps)+" overflow="+String(balanceOverflow)+" STM32_resets="+String(balanceResets)+"\n";
    header+="# GSTOP: gateway_ms,last_seen_STM32_ms,last_seen_seq,source; gateway command evidence, not an STM32 acknowledgment. First STOP per arm/pulse; max32 records.\n";
    for(unsigned i=0;i<controlStopCount;i++)header+=controlStops[i];
    header+="# gateway_stop_events_dropped="+String(controlStopOverflow)+"\n";
    header+="seq,t_ms,dt_us,execution_us,state,ready,fault,pwm_l,pwm_r,ticks_l,ticks_r,gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw,pitch_deg,gyroY_corrected_dps,v_l_mps,v_r_mps,x_encoder_m,observer_v_mps,observer_theta_deg,pwm_mean,yaw_integral_Nm,stm32_drops\n";
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
    server.sendHeader("Content-Disposition","attachment; filename=twip_balance_R12.txt");
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
