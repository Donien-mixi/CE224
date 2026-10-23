/**
  ******************************************************************************
  * @file    ESP32_S3_Gateway.ino    
  * @brief   Firmware Cầu nối Không dây Tốc độ cao ESP32-S3 N16R8 (Arduino IDE)
  *          Tạo Wi-Fi SoftAP + WebSockets Server + Cầu truyền thông USART1 với STM32
  ******************************************************************************
  * CẤU HÌNH TRÊN ARDUINO IDE:  
  *   - Board: "ESP32S3 Dev Module"
  *   - USB CDC On Boot: "Enabled"
  *   - Flash Size: "16MB (128Mb)"
  *   - Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"
  *   - PSRAM: "OPI PSRAM"
  *   - Thư viện cần cài đặt: "WebSockets" (tác giả Markus Sattler) qua Library Manager
  ******************************************************************************
  */

#include <WiFi.h>
#ifndef BOARD_HAS_PSRAM
#error "Hardware logging needs PSRAM: choose ESP32S3 Dev Module, PSRAM = OPI PSRAM (N16R8), then rebuild."
#endif
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "index_html.h"
#include "dashboard_html.h"
#include "wheel_tests_html.h"
#include <esp_heap_caps.h>
#include <lwip/sockets.h>
#include <errno.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
WebServer server(80);
WebSocketsServer webSocket(81);

struct __attribute__((packed)) TestFrame {
  uint16_t sync;
  uint32_t sequence, t_ms;
  uint16_t dt_us;
  int16_t gx, gy, gz, ax, ay, az;
  int32_t ticks_l, ticks_r;
  int16_t pwm_l, pwm_r;
  uint8_t stage, flags;
  uint16_t dropped, crc;
};
static_assert(sizeof(TestFrame) == 42, "STM32 test protocol v1");
#include "balance_wire.h"
static BalanceFrame balanceLatest,*balanceFrames;
static size_t balanceCount,balanceCapacity;
static uint32_t balanceReceived,balanceCRC,balanceGaps,balanceOverflow,balanceResets,balancePostUntil;
static bool balanceSeen,balanceDownloading;
static char balanceMeta[256]={};
void setupBalanceRoutes();
static void setupNetworkRoutes();
#include "job_log_store.h"
JobLogStore jobLogs;
uint32_t nextJobId=100,jobRequestedAt=0,jobLastRx=0;
bool jobDownloadBusy=false;
bool jobProtocolStop=false;
bool legacyFrozen=false;
char legacyMeta[256]={},legacyStop[192]={};
uint32_t legacyCRC=0;
char *testLog = nullptr;
size_t testCapacity = 0, testUsed = 0;
uint32_t testSamples = 0, testCRCError = 0, testSequenceGaps = 0, testOverflow = 0;
uint32_t testLastSequence = 0, testLastMs = 0, testLastReceived = 0;
uint8_t testStage = 0, testFlags = 0;
bool testSeen = false;
char testMeta[256] = "", testLatest[192] = "";
char testStop[192] = "";
uint8_t testPacket[sizeof(BalanceFrame)];
unsigned packetLength=42;
unsigned testPacketUsed = 0;
void receiveSTM32();
#ifdef ARDUINO
SemaphoreHandle_t captureMutex;
char pendingTelemetry[256] = "";
static void captureTask(void *) {
  for (;;) { receiveSTM32(); vTaskDelay(pdMS_TO_TICKS(1)); }
}
#endif

uint16_t testCRC(const uint8_t *bytes, unsigned length) {
  uint16_t crc = 0xffff;
  while (length--) {
    crc ^= (uint16_t)*bytes++ << 8;
    for (unsigned i = 0; i < 8; i++)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}

bool acceptBalanceFrame() {
  BalanceFrame f;memcpy(&f,testPacket,sizeof(f));
  if(f.sync!=0x60a5 || f.controller!=12 || testCRC(testPacket,sizeof(f)-2)!=f.crc){balanceCRC++;return false;}
  if(balanceSeen) {
    if(f.sequence>balanceLatest.sequence+1)balanceGaps+=f.sequence-balanceLatest.sequence-1;
    if(f.sequence<=balanceLatest.sequence)balanceResets++;
  }
  balanceLatest=f;balanceSeen=true;balanceReceived=millis();
  if(f.state==3 || f.state==7)balancePostUntil=millis()+2000;
  if((f.state==3 || f.state==7 || (balancePostUntil && (int32_t)(balancePostUntil-millis())>0))) {
    if(balanceFrames && balanceCount<balanceCapacity)balanceFrames[balanceCount++]=f;
    else balanceOverflow++;
  }
  return true;
}

bool acceptTestFrame() {
  TestFrame f; memcpy(&f, testPacket, sizeof(f));
  if (testCRC(testPacket, sizeof(f) - 2) != f.crc) {
    testCRCError++;if(jobLogs.active>=0)jobLogs.jobs[jobLogs.active].crcErrors++;return false;
  }
  bool manual=jobLogs.active>=0;
  bool legacyCapture=!manual && f.stage!=13;
  if (legacyCapture && testSeen && f.sequence <= testLastSequence && f.t_ms < testLastMs) {
    /* STM32 power/reset begins a new session; never silently mix two runs. */
    testUsed = testSamples = testSequenceGaps = testOverflow = 0;
    testMeta[0] = testStop[0] = 0; testSeen = false;
    legacyFrozen=false;
  }
  if(legacyCapture) {
    if (!testSeen) testSequenceGaps += f.sequence;
    else if (f.sequence > testLastSequence + 1) testSequenceGaps += f.sequence - testLastSequence - 1;
  } else if(manual) {
    int before=jobLogs.active;jobLogs.append(f);jobLastRx=millis();
    if(jobLogs.active<0 && (jobLogs.jobs[before].flags&256))jobProtocolStop=true;
  }
  testSeen = true; testLastSequence = f.sequence; testLastMs = f.t_ms;
  testLastReceived = millis(); testStage = f.stage; testFlags = f.flags;
  int len = snprintf(testLatest, sizeof(testLatest),
    "%lu,%lu,%u,%u,%u,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%u\n",
    (unsigned long)f.sequence, (unsigned long)f.t_ms, f.dt_us, f.stage, f.flags,
    f.pwm_l, f.pwm_r, (long)f.ticks_l, (long)f.ticks_r,
    f.gx, f.gy, f.gz, f.ax, f.ay, f.az, f.dropped);
  if (len > 0 && len < (int)sizeof(testLatest)) {
    if(!legacyCapture) { /* Late/aborted manual frames must never contaminate the boot log. */ }
    else if (testLog && testUsed + len <= testCapacity) {
      memcpy(testLog + testUsed, testLatest, len); testUsed += len; testSamples++;
    } else testOverflow++;
    testLatest[len - 1] = '\0'; /* keep stored CSV newline, make JSON preview valid */
  }
  return true;
}

#include "network_health.h"
#include "balance_archive.h"

void setupMeasurementRoutes();
void measurementWatchdog();

void downloadHardwareLog() {
  if(jobLogs.active>=0){server.send(409,"text/plain","Finish the active measurement before downloading.");return;}
  xSemaphoreTake(captureMutex, portMAX_DELAY);
  /* Snapshot length: downloading while tests run never alters the capture buffer. */
  size_t limit = testUsed;
  String header = "# TWIP_HARDWARE_TEST_V1\n# Read 200Hz; BMI160 ODR100Hz, gyro raw/16.4 dps; accel raw/4096 g\n"
    "# ticks_l/r: hardware quadrature totals; right sign NOT inverted. Nominal PPR=937.2; wheel D=0.065m\n"
    "# pwm: applied BEFORE sample; encoder interval ends at t_ms; dt_us measured by DWT, timestamp resolution 1ms\n"
    "# stages: 0 wait,1 staticIMU,2 LED_buzzer,3 L+,4 L-,5 R+,6 R-,7 stepsL,8 stepsR,9 both,10 postStatic,11 done,12 aborted\n"
    "# flags: 1 IMUvalid,2 timingFault,4 STOP,8 I2Cfault,16 noEncoderResponse,32 supportMoved,64 configFault,128 notStationary\n"
    "# No voltage/current sensors: battery_voltage, stall_torque, absolute forward direction and IMU tilt sign NOT VERIFIED\n"
    "# TINFO: version,STM32_start_ms,stage,flags,configOK,I2C_addr,chipID,ACC_CONF,ACC_RANGE,GYR_CONF,GYR_RANGE,biasX,biasY,biasZ,DB_L+,DB_L-,DB_R+,DB_R-,STM32drops,visited_mask,abort_from\n"
    "# visited_mask is recorded on STM32 even if the web misses a short stage; abort_from=255 means startup failure\n";
  header += "# " + String(legacyFrozen?legacyMeta:testMeta) + "\n# CRC_errors=" + String(legacyFrozen?legacyCRC:testCRCError) +
    " sequence_gaps=" + String(testSequenceGaps) + " buffer_drops=" + String(testOverflow) +
    " captured_samples=" + String(testSamples) + "\n";
  header += "# log_capacity_bytes=" + String(testCapacity) +
    " psram_size_bytes=" + String(ESP.getPsramSize()) +
    " psram_free_bytes=" + String(ESP.getFreePsram()) + "\n";
  header += "# TSTOP: stage,t_ms,accel_pitch_delta_deg,gx_dps,gy_dps,gz_dps,accel_norm_g,pwmL,pwmR,reason_mask(1=angle,2=gyro)\n# " + String(legacyFrozen?legacyStop:testStop) + "\n";
  header += "seq,t_ms,dt_us,stage,flags,pwm_l,pwm_r,ticks_l,ticks_r,gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw,stm32_drops\n";
  xSemaphoreGive(captureMutex);
  server.sendHeader("Content-Disposition", "attachment; filename=twip_hardware_test.txt");
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(header.length() + limit);
  server.send(200, "text/plain; charset=utf-8", header);
  auto client = server.client();
  uint32_t lastProgress = millis();
  for (size_t sent = 0; sent < limit && client.connected();) {
    receiveSTM32(); /* keep sampling even when the user downloads mid-test */
    webSocket.loop(); /* STOP remains usable from another connected page. */
    size_t block = limit - sent; if (block > 512) block = 512;
    /* NetworkClient::write may wait seconds on a stalled browser. Never block UART capture. */
    int written = ::send(client.fd(), testLog + sent, block, MSG_DONTWAIT);
    if (written > 0) { sent += written; lastProgress = millis(); }
    else if (written < 0 && errno != EWOULDBLOCK && errno != EAGAIN) break;
    if (millis() - lastProgress > 5000) { client.stop(); break; }
    yield();
  }
}

/* Cấu hình Wi-Fi SoftAP */
const char* AP_SSID = "TWIP_RACER_S3";
const char* AP_PASS = "12345678";

/* Cấu hình chân UART giao tiếp với STM32F411CEU6 */
#define STM32_RX_PIN  18  // Nối với STM32 PB6 (USART1_TX)
#define STM32_TX_PIN  17  // Nối với STM32 PB7 (USART1_RX)
#define STM32_BAUD    115200

/* Khởi tạo Web Server (Port 80) và WebSockets Server (Port 81) */
/* Bộ đệm nhận dữ liệu từ STM32 */
#define STM32_LINE_MAX 256
char stm32LineBuf[STM32_LINE_MAX];
uint8_t stm32LineIdx = 0;

#include "control_session.h"

/* Xử lý sự kiện WebSockets từ trình duyệt client */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      gatewayWsDisconnected++;
      Serial.printf("[WS] Client #%u ngat ket noi\n", num);
      // Only the controlling session can stop on disconnect.
      controlDisconnected(num);
      break;

    case WStype_CONNECTED: {
      gatewayWsConnected++;
      controlConnected(num);
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] Client #%u ket noi tu IP: %s\n", num, ip.toString().c_str());
      break;
    }

    case WStype_TEXT: {
      // Test jobs must reserve storage via HTTP; raw WS cannot bypass that handshake.
      for(size_t i=0;i+6<=length;i++)
        if(!memcmp(payload+i,"$TEST,",6) || !memcmp(payload+i,"$TPING",6))return;
      // Nhận gói tin điều khiển từ Web UI (ví dụ: $CMD,v,steer* hoặc $PID,...)
      // Chuyển tiếp ngay lập tức xuống STM32F411 qua Serial1
      if(length>256)return;
      String command;command.reserve(length);for(size_t i=0;i<length;i++)command+=(char)payload[i];
      if(!controlMessage(num,command))return;
      Serial1.write(payload, length);
      Serial1.print("\r\n");
      // Debug log để người dùng quan sát trên Arduino IDE Serial Monitor
      // Per-command USB printing is disabled: never block the command path.
      break;
    }

    default:
      break;
  }
}

void setup() {
  /* Khởi tạo USB Serial để Debug */
  Serial.begin(115200);
  delay(500);
  Serial.println("\n==========================================");
  Serial.println("   TWIP RACER S3 — WIRELESS IOT GATEWAY   ");
  Serial.println("==========================================");

  /* Khởi tạo Serial1 giao tiếp phần cứng với STM32F411 */
  Serial1.setRxBufferSize(16384);
  Serial1.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  // An ESP-only restart must never keep an earlier STM32 driving session alive.
  Serial1.print("$STOP*\r\n");
  gatewayHealthBegin();
  testCapacity = 2 * 1024 * 1024;
  testLog = (char *)heap_caps_malloc(testCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!testLog) { testCapacity = 128 * 1024; testLog = (char *)malloc(testCapacity); }
  if (!testLog) testCapacity = 0;
  void *jobMemory=heap_caps_malloc(3*1024*1024,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(jobMemory)jobLogs.attach(jobMemory,3*1024*1024);
  balanceCapacity=9000;
  balanceFrames=(BalanceFrame*)heap_caps_malloc(balanceCapacity*sizeof(BalanceFrame),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(!balanceFrames)balanceCapacity=0;
  captureMutex = xSemaphoreCreateMutex();
  if (!captureMutex || xTaskCreatePinnedToCore(captureTask, "uartCapture", 6144,
      nullptr, 3, nullptr, 1) != pdPASS) {
    Serial.println("ERROR: UART capture task unavailable; restart gateway.");
    while (true) delay(1000);
  }
  Serial.printf("[UART] Da mo ket noi voi STM32 tren RX: GPIO%d, TX: GPIO%d @ %d bps\n",
                STM32_RX_PIN, STM32_TX_PIN, STM32_BAUD);
  archiveBegin();

  /* Bật Wi-Fi SoftAP độc lập tốc độ cao */
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  bool apOK=WiFi.softAP(AP_SSID,AP_PASS,6,false,4);
  esp_err_t bwOK=esp_wifi_set_bandwidth(WIFI_IF_AP,WIFI_BW_HT20);
  bool powerOK=WiFi.setTxPower(WIFI_POWER_17dBm);
  Serial.printf("[NET1] AP=%d HT20=%d TX17=%d boot=%lu reset=%s archive=%lu\n",apOK,(int)bwOK,powerOK,(unsigned long)gatewayBootId,gatewayResetText(gatewayResetReason),(unsigned long)archiveError);
  IPAddress myIP = WiFi.softAPIP();

  Serial.print("[Wi-Fi] SoftAP SSID : "); Serial.println(AP_SSID);
  Serial.print("[Wi-Fi] Mat khau    : "); Serial.println(AP_PASS);
  Serial.print("[Wi-Fi] Dia chi IP  : http://"); Serial.println(myIP);

  /* Cấu hình Web Server phục vụ giao diện HUD */
  auto serveMeasurementPage=[]() {
    server.sendHeader("Cache-Control","no-store");
    server.sendHeader("Pragma","no-cache");
    server.sendHeader("Expires","0");
    server.sendHeader("X-TWIP-Build",TWIP_WEB_BUILD);
    server.send_P(200, "text/html", INDEX_HTML);
  };
  auto serveDashboard=[](){
    server.sendHeader("Cache-Control","no-store");server.sendHeader("X-TWIP-Build",TWIP_WEB_BUILD);
    server.send_P(200,"text/html",DASHBOARD_HTML);
  };
  server.on("/wheel-tests",HTTP_GET,[](){
    server.sendHeader("Cache-Control","no-store");server.sendHeader("X-TWIP-Build",TWIP_WEB_BUILD);
    server.send_P(200,"text/html",WHEEL_TESTS_HTML);
  });
  server.on("/ui/bal-r6",HTTP_GET,serveDashboard);
  server.on("/calibration", HTTP_GET, serveMeasurementPage);
  server.on("/", HTTP_GET, serveDashboard);
  server.on(TWIP_WEB_PATH, HTTP_GET, serveDashboard);
  server.on("/ui/bal-r7-r12",HTTP_GET,serveDashboard); // old bookmark, same R12-only page
  server.on("/ui/bal-r1",HTTP_GET,serveDashboard);
  server.on("/ui/bal-r2",HTTP_GET,serveDashboard);
  server.on("/ui/bal-r3",HTTP_GET,serveDashboard);
  server.on("/ui/bal-r4",HTTP_GET,serveDashboard);
  server.on("/ui/bal-r5",HTTP_GET,serveDashboard);
  server.on("/firmware-info",HTTP_GET,[](){
    server.sendHeader("Cache-Control","no-store");
    server.sendHeader("X-TWIP-Build",TWIP_WEB_BUILD);
    server.send(200,"application/json",String("{\"build\":\"")+TWIP_WEB_BUILD+
      "\",\"compiled\":\""+__DATE__+" "+__TIME__+"\",\"plan_version\":1,\"presets\":56,\"wheel_plan_version\":1,\"wheel_presets\":12,\"wheel_ui\":\"/wheel-tests\",\"ui\":\""+TWIP_WEB_PATH+"\"}");
  });

  server.begin();
  server.on("/hardware-log", HTTP_GET, downloadHardwareLog);
  setupMeasurementRoutes();
  setupBalanceRoutes();
  setupNetworkRoutes();
  server.on("/hardware-status", HTTP_GET, []() {
    char reply[1024];
    xSemaphoreTake(captureMutex, portMAX_DELAY);
    snprintf(reply, sizeof(reply),
      "{\"seen\":%s,\"stage\":%u,\"flags\":%u,\"samples\":%lu,\"t_ms\":%lu,\"crc_errors\":%lu,\"gaps\":%lu,\"overflow\":%lu,\"capacity\":%u,\"age_ms\":%lu,\"meta\":\"%s\",\"latest\":\"%s\",\"stop\":\"%s\"}",
      testSeen ? "true" : "false", testStage, testFlags, (unsigned long)testSamples,
      (unsigned long)testLastMs, (unsigned long)testCRCError, (unsigned long)testSequenceGaps,
      (unsigned long)testOverflow, (unsigned)testCapacity, (unsigned long)(millis()-testLastReceived), testMeta, testLatest, testStop);
    size_t replyLen=strlen(reply);
    if(replyLen && replyLen+40<sizeof(reply))snprintf(reply+replyLen-1,sizeof(reply)-replyLen+1,",\"balance_live\":%s}",
      balanceSeen && millis()-balanceReceived<1000?"true":"false");
    xSemaphoreGive(captureMutex);
    /* CSV latest terminator is omitted so JSON contains no raw newline. */
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", reply);
  });
  Serial.println("[HTTP] Web Server port 80 da khoi dong");

  /* Khởi động WebSockets Server */  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WS] WebSockets Server port 81 da khoi dong");
  Serial.println("------------------------------------------");
  Serial.println("HE THONG SAN SANG! HAY KET NOI WI-FI & MO TRINH DUYET!");
}

void loop() {
  static uint32_t previous=0,lastLegacy=0;uint32_t tick=millis();
  if(previous&&tick-previous>gatewayLoopGapMax)gatewayLoopGapMax=tick-previous;previous=tick;
  char telemetry[256];bool liveBalance;
  xSemaphoreTake(captureMutex, portMAX_DELAY);
  memcpy(telemetry, pendingTelemetry, sizeof(telemetry)); pendingTelemetry[0] = 0;
  liveBalance=balanceSeen&&millis()-balanceReceived<=1000;
  xSemaphoreGive(captureMutex);
  // R12 dashboard consumes balance-status, not legacy $TEL broadcasts. A slow
  // observer must not queue ignored traffic ahead of control on every loop.
  if(telemetry[0]&&!liveBalance&&tick-lastLegacy>=200){webSocket.broadcastTXT(telemetry);lastLegacy=tick;}
  /* 1. Xử lý các request HTTP và WebSockets */
  webSocket.loop();
  server.handleClient();
  webSocket.loop();
  receiveSTM32();
  measurementWatchdog();
}

#include "measurement_web.h"
#include "balance_web.h"
#include "network_routes.h"

void receiveSTM32() {
#ifdef ARDUINO
  if (!captureMutex) return;
  xSemaphoreTake(captureMutex, portMAX_DELAY);
#endif

  /* 2. Đọc luồng gói tin Telemetry từ STM32F411 và bắn lên Web UI */
  while (Serial1.available()) {
    uint8_t b = (uint8_t)Serial1.read();
    if (testPacketUsed || b == 0xa5) {
      if(!testPacketUsed)stm32LineIdx=0;
      testPacket[testPacketUsed++]=b;
      for(;;) {
        if(!testPacketUsed)break;
        if(testPacket[0]!=0xa5 || (testPacketUsed>=2 && testPacket[1]!=0x5a && testPacket[1]!=0x60)) {
          memmove(testPacket,testPacket+1,--testPacketUsed);continue;
        }
        if(testPacketUsed<2)break;
        packetLength=testPacket[1]==0x60?sizeof(BalanceFrame):sizeof(TestFrame);
        if(testPacketUsed<packetLength)break;
        bool ok=testPacket[1]==0x60?acceptBalanceFrame():acceptTestFrame();
        unsigned consumed=ok?packetLength:1;
        testPacketUsed-=consumed;memmove(testPacket,testPacket+consumed,testPacketUsed);
      }
      continue;
    }
    char c = (char)b;

    if (c == '\n' || c == '\r') {
      if (stm32LineIdx > 0) {
        stm32LineBuf[stm32LineIdx] = '\0';
        
        // Phát quảng bá gói $TEL xuống tất cả Web clients đang mở
        if (!strncmp(stm32LineBuf,"$TJ,",4)) {
          jobLogs.marker(stm32LineBuf,testMeta);jobLastRx=millis();
          if(jobLogs.active<0) {
            unsigned version,id,kind,phase;
            if(sscanf(stm32LineBuf,"$TJ,%u,%u,%u,%u",&version,&id,&kind,&phase)==4 && jobLogs.find(id)) {
              testStage=phase==2?11:12;testLastReceived=millis();
            }
          }
        } else if (!strncmp(stm32LineBuf, "$TINFO,", 7)) {
          strncpy(testMeta, stm32LineBuf, sizeof(testMeta)-1);
          testMeta[sizeof(testMeta)-1] = 0;
          unsigned version, start, stage, flags;
          if (sscanf(testMeta, "$TINFO,%u,%u,%u,%u", &version, &start, &stage, &flags) == 4) {
            testStage = stage; testFlags = flags; testLastReceived = millis();
          }
        } else if (!strncmp(stm32LineBuf, "$TSTOP,", 7)) {
          strncpy(testStop, stm32LineBuf, sizeof(testStop)-1);
          testStop[sizeof(testStop)-1] = 0;
          if(jobLogs.active>=0)snprintf(jobLogs.jobs[jobLogs.active].stop,sizeof(testStop),"%s",testStop);
        } else if (!strncmp(stm32LineBuf,"$BINFO,",7)) {
          snprintf(balanceMeta,sizeof(balanceMeta),"%s",stm32LineBuf);
        } else if (!strncmp(stm32LineBuf, "$TEL,", 5)) {
#ifdef ARDUINO
          /* Network calls stay on the web task, never stall UART capture. */
          strncpy(pendingTelemetry, stm32LineBuf, sizeof(pendingTelemetry)-1);
          pendingTelemetry[sizeof(pendingTelemetry)-1] = 0;
#else
          webSocket.broadcastTXT(stm32LineBuf);
#endif
        }

        stm32LineIdx = 0;
      }
    } else {
      if (stm32LineIdx < (STM32_LINE_MAX - 1)) {
        stm32LineBuf[stm32LineIdx++] = c;
      } else {
        stm32LineIdx = 0; // Tràn bộ đệm -> Reset
      }
    }
  }
#ifdef ARDUINO
  xSemaphoreGive(captureMutex);
#endif
}
