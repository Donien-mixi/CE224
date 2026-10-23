#pragma once
static const char *archiveErrorText(){
 switch(archiveError){case 0:return "ready";case 1:return "filesystem_unavailable_preserved";
 case 2:return "four_archives_full";case 3:return "flash_space_low";case 4:return "flash_io_error";default:return "archive_task_unavailable";}
}
static bool archiveSafeToRead(){
 xSemaphoreTake(captureMutex,portMAX_DELAY);
 bool safe=(!balanceSeen||(balanceLatest.state!=3&&balanceLatest.state!=7))&&
    !balanceDownloading&&(int32_t)(balancePostUntil-millis())<=0&&jobLogs.active<0;
 xSemaphoreGive(captureMutex);return safe;
}
static int archiveEntryIndex(const String &name){
 if(name.length()>39||name.indexOf('/')>=0||name.indexOf('\\')>=0||name.indexOf("..")>=0)return -1;
 for(int i=0;i<4;i++)if(archiveEntries[i].name[0]&&name==archiveEntries[i].name)return i;
 return -1;
}
static void setupNetworkRoutes(){
 auto status=[](bool download){
  uint32_t joins,leaves,stops,reason;
  portENTER_CRITICAL(&wifiHealthMux);joins=gatewayApConnect;leaves=gatewayApDisconnect;stops=gatewayApStop;reason=gatewayApReason;portEXIT_CRITICAL(&wifiHealthMux);
  char b[1024];snprintf(b,sizeof(b),
   "{\"boot_id\":%lu,\"reset_code\":%lu,\"reset_reason\":\"%s\",\"uptime_ms\":%lu,\"heap_free\":%u,\"heap_min\":%u,\"psram_free\":%u,\"ap_clients\":%u,\"ap_joins\":%lu,\"ap_leaves\":%lu,\"ap_stops\":%lu,\"ap_reason\":%lu,\"ws_joins\":%lu,\"ws_leaves\":%lu,\"loop_gap_max_ms\":%lu,\"channel\":6,\"archive_ready\":%s,\"archive_error\":\"%s\",\"archive_saved\":%lu,\"archives\":%u}",
   (unsigned long)gatewayBootId,(unsigned long)gatewayResetReason,gatewayResetText(gatewayResetReason),(unsigned long)millis(),ESP.getFreeHeap(),ESP.getMinFreeHeap(),ESP.getFreePsram(),WiFi.softAPgetStationNum(),
   (unsigned long)joins,(unsigned long)leaves,(unsigned long)stops,(unsigned long)reason,(unsigned long)gatewayWsConnected,(unsigned long)gatewayWsDisconnected,(unsigned long)gatewayLoopGapMax,
   archiveMounted&&archiveError==0?"true":"false",archiveErrorText(),(unsigned long)archiveSaved,(unsigned)archiveCount);
  if(download)server.sendHeader("Content-Disposition",String("attachment; filename=twip_network_boot_")+String(gatewayBootId)+".json");
  server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",b);
 };
 server.on("/network-status",HTTP_GET,[status](){status(false);});
 server.on("/network-log",HTTP_GET,[status](){status(true);});
 server.on("/balance-archives",HTTP_GET,[](){
  if(!archiveMounted||!archiveMutex||!archiveBuffer){server.send(503,"application/json","{\"files\":[]}");return;}
  if(xSemaphoreTake(archiveMutex,0)!=pdTRUE){server.send(503,"application/json","{\"files\":[]}");return;}
  String b="{\"files\":[";bool comma=false;
  for(int i=0;i<4;i++)if(archiveEntries[i].name[0]){
   if(comma)b+=",";
   comma=true;ArchiveEntry &e=archiveEntries[i];
   b+="{\"name\":\""+String(e.name)+"\",\"samples\":"+String(e.frames)+",\"partial\":"+(e.partial?"true":"false")+",\"current\":"+(!strcmp(e.name,archiveCurrentName)?"true":"false")+"}";
  }
  b+="]}";xSemaphoreGive(archiveMutex);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",b);
 });
 server.on("/balance-archive",HTTP_GET,[](){
  if(!archiveMounted||!archiveMutex||!archiveBuffer||!archiveSafeToRead()){server.send(409,"text/plain","Stop motors and wait2s before archive download.");return;}
  if(xSemaphoreTake(archiveMutex,pdMS_TO_TICKS(250))!=pdTRUE){server.send(503,"text/plain","Archive busy; retry.");return;}
  int slot=archiveEntryIndex(server.arg("name"));
  if(slot<0){xSemaphoreGive(archiveMutex);server.send(404,"text/plain","Archive not found.");return;}
  xSemaphoreTake(captureMutex,portMAX_DELAY);balanceDownloading=true;xSemaphoreGive(captureMutex);
  char path[64];snprintf(path,sizeof(path),"/twip-net1/%s",archiveEntries[slot].name);File f=FFat.open(path,FILE_READ);JournalHeader h;
  if(!f||!archiveOpenHeader(f,h)){
   f.close();xSemaphoreGive(archiveMutex);xSemaphoreTake(captureMutex,portMAX_DELAY);balanceDownloading=false;xSemaphoreGive(captureMutex);server.send(500,"text/plain","Archive header invalid; file preserved.");return;
  }
  h.web_build[sizeof(h.web_build)-1]=0;
  String header="# TWIP_BALANCE_RT4_NET1_ARCHIVE_V1; CRC-validated committed batches only\n# web_build="+String(h.web_build)+" boot_id="+String(h.boot_id)+" boot_reset="+String(gatewayResetText(h.reset_reason))+"\n";
  header+="# gateway_at_export_boot="+String(gatewayBootId)+" reset="+String(gatewayResetText(gatewayResetReason))+"\n";
  header+="# Frames122;50Hz; controller R12 RT4. New BINFO per batch is authoritative for its following rows. Tail after last good CRC is omitted. Flash checkpoint is best effort; final unflushed samples may be missing.\n";
  header+="seq,t_ms,dt_us,execution_us,state,ready,fault,pwm_l,pwm_r,ticks_l,ticks_r,gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw,pitch_deg,gyroY_corrected_dps,v_l_mps,v_r_mps,x_encoder_m,control_v_mps,control_theta_deg,control_pwm,control_integral,stm32_drops,controller,control_phase,distance_limit_m,x_ref_m,v_ref_mps,drive_epoch,motion_flags,drive_direction,psi_ref_rad,w_ref_radps,turn_direction,drive_sequence,drive_rx_ms\n";
  // Two deterministic passes guarantee Content-Length even for a torn tail.
  uint32_t rows=0;size_t size=header.length();bool valid=true;
  char row[520],meta[512],footer[100];
  for(int pass=0;pass<2;pass++){
   if(pass){
    snprintf(footer,sizeof(footer),"# END archive rows=%lu partial_tail=%u\n",(unsigned long)rows,archiveEntries[slot].partial?1:0);
    server.sendHeader("Content-Disposition",String("attachment; filename=twip_balance_saved_")+archiveEntries[slot].name+".txt");
    server.sendHeader("Cache-Control","no-store");server.setContentLength(size+strlen(footer));server.send(200,"text/plain; charset=utf-8",header);
   }
   WiFiClient client=server.client();f.seek(sizeof(JournalHeader));uint32_t index=0;
   while(f.available()){
    JournalChunk c;if(!archiveReadChunk(f,index++,c))break;
    JournalInfo info;memcpy(&info,archiveBuffer,sizeof(info));info.meta[255]=0;info.stop[127]=0;
    int n=snprintf(meta,sizeof(meta),"# BATCH,%lu; BINFO=%s; captured=%lu crc=%lu gaps=%lu overflow=%lu STM32_resets=%lu\n%s",(unsigned long)(index-1),info.meta,(unsigned long)info.captured,(unsigned long)info.crc_errors,(unsigned long)info.gaps,(unsigned long)info.overflow,(unsigned long)info.resets,info.stop);
    if(n<0||(size_t)n>=sizeof(meta)){valid=false;break;}
    if(!pass)size+=(size_t)n;else if(!sendMeasurementBytes(client,meta,n)){valid=false;break;}
    for(unsigned i=0;i<c.frames;i++){
     BalanceFrame frame;memcpy(&frame,archiveBuffer+sizeof(info)+i*sizeof(frame),sizeof(frame));int len=balanceExportRow(row,sizeof(row),frame);
     if(len<0){valid=false;break;}
     if(!pass){size+=len;rows++;}else if(!sendMeasurementBytes(client,row,len)){valid=false;break;}
    }
    if(!valid)break;
    yield();
   }
   if(!valid){if(pass)client.stop();else server.send(500,"text/plain","Export error; archive preserved.");break;}
   if(pass&&!sendMeasurementBytes(client,footer,strlen(footer)))client.stop();
  }
  f.close();xSemaphoreGive(archiveMutex);xSemaphoreTake(captureMutex,portMAX_DELAY);balanceDownloading=false;xSemaphoreGive(captureMutex);
 });
 server.on("/balance-archive-delete",HTTP_POST,[](){
  long client=-1,token=0;
  bool identity=strictNumber(server.arg("session"),client)&&strictNumber(server.arg("token"),token)&&controlIdentity(client,token);
  if(!identity||!archiveMutex||!archiveSafeToRead()){server.send(409,"text/plain","Stop first; deletion needs this page's connected session.");return;}
  if(xSemaphoreTake(archiveMutex,pdMS_TO_TICKS(250))!=pdTRUE){server.send(503,"text/plain","Archive busy; retry.");return;}
  int slot=archiveEntryIndex(server.arg("name"));bool ok=false;
  if(slot>=0&&strcmp(archiveEntries[slot].name,archiveCurrentName)){
   char p[64];snprintf(p,sizeof(p),"/twip-net1/%s",archiveEntries[slot].name);ok=FFat.remove(p);
   if(ok){archiveEntries[slot]={};archiveCount--;if(archiveError==2||archiveError==3)archiveError=0;}
  }
  xSemaphoreGive(archiveMutex);server.send(ok?200:409,"text/plain",ok?"Deleted":"Download first; for the current capture clear RAM log before deleting its archive.");
 });
}
