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

#include "firmware_build.h"
#include "dashboard_html_gz.h"
/* ETag trang: tang khi sua HTML de dien thoai lay ban moi. */
#define DASHBOARD_ETAG "rt47-20261128a"

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

#include "network_health.h"
#include "balance_archive.h"

/* Cấu hình Wi-Fi SoftAP */
const char* AP_SSID = "TWIP_RACER_S3";
const char* AP_PASS = "12345678";
/* RT41: kênh AP. Mặc định quét một lần lúc khởi động để chọn kênh ít nhiễu nhất
 * trong1/6/11; đặt GATEWAY_AP_CHANNEL_SCAN=0 để luôn dùng GATEWAY_AP_CHANNEL. */
#ifndef GATEWAY_AP_CHANNEL_SCAN
#define GATEWAY_AP_CHANNEL_SCAN 1
#endif
#ifndef GATEWAY_AP_CHANNEL
#define GATEWAY_AP_CHANNEL 6
#endif
#ifndef GATEWAY_AP_SCAN_MS_PER_CHANNEL
#define GATEWAY_AP_SCAN_MS_PER_CHANNEL 60
#endif
static uint8_t gatewayApChannel=GATEWAY_AP_CHANNEL;

/* Cấu hình chân UART giao tiếp với STM32F411CEU6 */
#define STM32_RX_PIN  18  // Nối với STM32 PB6 (USART1_TX)
#define STM32_TX_PIN  17  // Nối với STM32 PB7 (USART1_RX)
/* RT41: 115200 -> 921600, PHẢI đổi cùng lúc với USART1 trên STM32 (usart.c và
 * Xe_Can_Bang.ioc). Lý do: ở115200 một khung telemetry122B giữ TX DMA10,6ms
 * trong mỗi20ms (53% thời gian), nên35 byte lệnh phải xếp sau nó và khung BINFO
 * hay gặp kênh bận; ở921600 khung đó chỉ1,3ms (6,7%) và lệnh0,38ms.
 * Không đổi luật điều khiển, khung truyền hay vòng200Hz. */
#ifndef STM32_BAUD
#define STM32_BAUD    921600
#endif

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
  /* No STOP on boot: if this gateway restarts while the STM32 is balancing, the
   * vehicle must keep balancing and wait for the next page, not fall over. */
  gatewayHealthBegin();
  balanceCapacity=30000;
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

  /* Wi-Fi first: after a reset the page must be able to re-associate while the
   * flash log is being prepared. The archive mount no longer scans the whole
   * partition on the healthy path, so both steps are now sub-second. */
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  uint8_t apChannel=GATEWAY_AP_CHANNEL;
#if GATEWAY_AP_CHANNEL_SCAN
  /* RT41: channel6 was hard-coded, so every neighbour AP on6 shares our airtime
   * and the command stream pays for it in retransmits. Scan once, bounded (the
   * ESP32 scan has no timeout argument, so this is per-channel instead of
   * unlimited), and take the quietest of1/6/11. A failed/empty scan keeps6.
   * Cost: at most13*GATEWAY_AP_SCAN_MS_PER_CHANNEL, measured ~0.3-0.8s, once at
   * boot before the AP exists - the page retries for20s (NET21), so this cannot
   * turn into a dead page. */
  {
    int found=WiFi.scanNetworks(false,false,false,GATEWAY_AP_SCAN_MS_PER_CHANNEL);
    static const uint8_t candidate[3]={1,6,11};
    int cost[3]={0,0,0};
    if(found>0){
      for(int n=0;n<found;n++){
        int ch=WiFi.channel(n);
        for(int k=0;k<3;k++){
          int d=ch-(int)candidate[k];if(d<0)d=-d;
          if(d==0)cost[k]+=3;          /* same channel: full airtime share */
          else if(d==1)cost[k]+=1;     /* adjacent: still overlaps at HT20 */
        }
      }
      int best=0;for(int k=1;k<3;k++)if(cost[k]<cost[best])best=k;
      apChannel=candidate[best];
    }
    gatewayApChannel=apChannel;
    Serial.printf("[NET41] scan=%d ch1=%d ch6=%d ch11=%d -> AP ch%u\n",
                  found,cost[0],cost[1],cost[2],(unsigned)apChannel);
  }
#endif
  bool apOK=WiFi.softAP(AP_SSID,AP_PASS,apChannel,false,4);
  esp_err_t bwOK=esp_wifi_set_bandwidth(WIFI_IF_AP,WIFI_BW_HT20);
  /* P1.5: DTIM=1, beacon 100 ms -> dien thoai thuc day thuong xuyen hon, giam duoi tre Wi-Fi. */
  {
    wifi_config_t apcfg;
    if(esp_wifi_get_config(WIFI_IF_AP,&apcfg)==ESP_OK){
      apcfg.ap.beacon_interval=100;
      apcfg.ap.dtim_period=1;
      esp_wifi_set_config(WIFI_IF_AP,&apcfg);
    }
  }
  /* NET34: ha cong suat phat tu 17 dBm xuong 11 dBm.
   * Log RT32 (ba file (3)(5)/(4)(5)/(5)(5)) cho thay ESP32 reset do SUT AP NGUON
   * (reset=brownout) moi khi motor dao chieu; dinh dong WiFi la mot phan cua xung
   * do. 11 dBm giam dinh khoang 100 mA, van du tam phong/trong nha. Neu can xa hon
   * thi doi lai 17 dBm va tang tu nguon (xem Docs/RT33_On_Dinh_Ket_Noi.md). */
  bool powerOK=WiFi.setTxPower(WIFI_POWER_11dBm);
  Serial.printf("[NET1] AP=%d HT20=%d TX11=%d boot=%lu reset=%s\n",apOK,(int)bwOK,powerOK,(unsigned long)gatewayBootId,gatewayResetText(gatewayResetReason));

  archiveBegin();
  Serial.printf("[LOG] archive_mounted=%d error=%lu saved=%lu\n",archiveMounted?1:0,(unsigned long)archiveError,(unsigned long)archiveSaved);
  IPAddress myIP = WiFi.softAPIP();

  Serial.print("[Wi-Fi] SoftAP SSID : "); Serial.println(AP_SSID);
  Serial.print("[Wi-Fi] Mat khau    : "); Serial.println(AP_PASS);
  Serial.print("[Wi-Fi] Dia chi IP  : http://"); Serial.println(myIP);

  /* Cấu hình Web Server phục vụ giao diện HUD */
  auto serveDashboard=[](){
    /* P2.1: trang nen gzip (54,8 KB -> 18,1 KB) + ETag; 304 khi chua doi. */
    String inm=server.header("If-None-Match");
    if(inm.length()&&inm==String("\"" DASHBOARD_ETAG "\"")){server.send(304);return;}
    server.sendHeader("ETag","\"" DASHBOARD_ETAG "\"");
    server.sendHeader("Cache-Control","no-cache");
    server.sendHeader("X-TWIP-Build",TWIP_WEB_BUILD);
    server.sendHeader("Content-Encoding","gzip");
    server.send_P(200,"text/html",(PGM_P)DASHBOARD_GZ,(size_t)DASHBOARD_GZ_LEN);
  };
  server.on("/ui/bal-r6",HTTP_GET,serveDashboard);

  /* Chan doan: /ping tra "pong" (server song?), /diag tra do dai HTML + heap + reset. */
  server.on("/ping",HTTP_GET,[](){server.send(200,"text/plain","pong");});
  server.on("/diag",HTTP_GET,[](){
    char b[256];
    snprintf(b,sizeof b,"{\"gz_len\":%u,\"heap\":%u,\"min_heap\":%u,\"uptime\":%lu,\"boot\":%lu,\"reset\":\"%s\"}",
      (unsigned)DASHBOARD_GZ_LEN,(unsigned)ESP.getFreeHeap(),(unsigned)ESP.getMinFreeHeap(),
      (unsigned long)millis(),(unsigned long)gatewayBootId,gatewayResetText(gatewayResetReason));
    server.send(200,"application/json",b);
  });
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
      "\",\"compiled\":\""+__DATE__+" "+__TIME__+"\",\"speed_presets_cmps\":[50,72,80],\"ui\":\""+TWIP_WEB_PATH+"\"}");
  });

  server.begin();
  setupBalanceRoutes();
  setupNetworkRoutes();
  Serial.println("[HTTP] Web Server port 80 da khoi dong");

  /* Khởi động WebSockets Server */  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  /* Backstop for a client that vanished without FIN (phone Wi-Fi re-association
   * after a fall). The page has its own 3 s silence watchdog. */
  webSocket.enableHeartbeat(20000,8000,2);
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

  /* Gateway heartbeat: a closed browser must not cut the motor. While the STM32
   * reports balancing, keep its command watchdog fresh even with no page open;
   * the 350 ms move lease still brakes any held gesture when commands stop. */
  static uint32_t lastKeepalive=0,lastWsBeat=0;
  if(tick-lastKeepalive>=400){
    bool balancing;
    xSemaphoreTake(captureMutex, portMAX_DELAY);
    balancing=balanceSeen && (uint32_t)(millis()-balanceReceived)<=1000 && balanceLatest.state==3;
    xSemaphoreGive(captureMutex);
    if(balancing){Serial1.print("$PING*\r\n");lastKeepalive=tick;}
  }
  /* RT42: the phone's RSSI/PHY mode is the only way to tell interference from
   * distance. Refresh it at 0.5 Hz; it is reported by /network-status and in the
   * log header, so every future log carries the radio evidence. */
  static uint32_t lastRadio=0;
  if(tick-lastRadio>=2000){lastRadio=tick;gatewayPollClient();}
  /* 1 Hz liveness frame: a phone whose Wi-Fi silently died can no longer keep a
   * half-open command channel that still looks connected to the page. */
  if(tick-lastWsBeat>=1000){
    lastWsBeat=tick;
    if(webSocket.connectedClients())webSocket.broadcastTXT("{\"hb\":1}");
  }
}

static bool strictNumber(const String &s,long &value) {
  if(!s.length() || s.length()>11)return false;
  char *end;errno=0;value=strtol(s.c_str(),&end,10);return *end==0 && errno!=ERANGE;
}
static bool sendMeasurementBytes(WiFiClient &client,const char *bytes,size_t n) {
  uint32_t progress=millis();
  for(size_t sent=0;sent<n && client.connected();) {
    int written=::send(client.fd(),bytes+sent,n-sent,MSG_DONTWAIT);
    if(written>0){sent+=written;progress=millis();}
    else if(written<0 && errno!=EAGAIN && errno!=EWOULDBLOCK)return false;
    if(millis()-progress>5000)return false;
    webSocket.loop();yield();
    if(sent==n)return true;
  }
  return false;
}

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
        bool ok=testPacket[1]==0x60?acceptBalanceFrame():false;
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
        if (!strncmp(stm32LineBuf,"$BINFO,",7)) {
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
