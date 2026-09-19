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
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "index_html.h"

/* Cấu hình Wi-Fi SoftAP */
const char* AP_SSID = "TWIP_RACER_S3";
const char* AP_PASS = "12345678";

/* Cấu hình chân UART giao tiếp với STM32F411CEU6 */
#define STM32_RX_PIN  18  // Nối với STM32 PB6 (USART1_TX)
#define STM32_TX_PIN  17  // Nối với STM32 PB7 (USART1_RX)
#define STM32_BAUD    115200

/* Khởi tạo Web Server (Port 80) và WebSockets Server (Port 81) */
WebServer server(80);
WebSocketsServer webSocket(81);

/* Bộ đệm nhận dữ liệu từ STM32 */
#define STM32_LINE_MAX 128
char stm32LineBuf[STM32_LINE_MAX];
uint8_t stm32LineIdx = 0;

/* Xử lý sự kiện WebSockets từ trình duyệt client */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client #%u ngat ket noi\n", num);
      // Gửi lệnh phanh an toàn xuống STM32 khi client mất kết nối
      Serial1.print("$CMD,0.00,0.0*\r\n");
      break;

    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] Client #%u ket noi tu IP: %s\n", num, ip.toString().c_str());
      break;
    }

    case WStype_TEXT: {
      // Nhận gói tin điều khiển từ Web UI (ví dụ: $CMD,v,steer* hoặc $PID,...)
      // Chuyển tiếp ngay lập tức xuống STM32F411 qua Serial1
      Serial1.write(payload, length);
      Serial1.print("\r\n");
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
  Serial1.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  Serial.printf("[UART] Da mo ket noi voi STM32 tren RX: GPIO%d, TX: GPIO%d @ %d bps\n",
                STM32_RX_PIN, STM32_TX_PIN, STM32_BAUD);

  /* Bật Wi-Fi SoftAP độc lập tốc độ cao */
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress myIP = WiFi.softAPIP();

  Serial.print("[Wi-Fi] SoftAP SSID : "); Serial.println(AP_SSID);
  Serial.print("[Wi-Fi] Mat khau    : "); Serial.println(AP_PASS);
  Serial.print("[Wi-Fi] Dia chi IP  : http://"); Serial.println(myIP);

  /* Cấu hình Web Server phục vụ giao diện HUD */
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.begin();
  Serial.println("[HTTP] Web Server port 80 da khoi dong");

  /* Khởi động WebSockets Server */
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WS] WebSockets Server port 81 da khoi dong");
  Serial.println("------------------------------------------");
  Serial.println("HE THONG SAN SANG! HAY KET NOI WI-FI & MO TRINH DUYET!");
}

void loop() {
  /* 1. Xử lý các request HTTP và WebSockets */
  server.handleClient();
  webSocket.loop();

  /* 2. Đọc luồng gói tin Telemetry từ STM32F411 và bắn lên Web UI */
  while (Serial1.available()) {
    char c = (char)Serial1.read();

    if (c == '\n' || c == '\r') {
      if (stm32LineIdx > 0) {
        stm32LineBuf[stm32LineIdx] = '\0';
        
        // Phát quảng bá gói $TEL xuống tất cả Web clients đang mở
        webSocket.broadcastTXT(stm32LineBuf);

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
}
