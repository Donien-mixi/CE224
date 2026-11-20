#pragma once
#include <esp_system.h>
#include <esp_random.h>
#include <esp_wifi.h>
#include <Preferences.h>
static uint32_t gatewayBootId,gatewayResetReason,gatewayLoopGapMax;
static uint32_t gatewayWsConnected,gatewayWsDisconnected;
static portMUX_TYPE wifiHealthMux=portMUX_INITIALIZER_UNLOCKED;
static uint32_t gatewayApConnect,gatewayApDisconnect,gatewayApStop,gatewayApReason;
static char gatewayStopRecord[128]={}; // always copied under captureMutex
static const char *gatewayResetText(uint32_t r){
 switch(r){case ESP_RST_POWERON:return "power_on";case ESP_RST_BROWNOUT:return "brownout";
 case ESP_RST_PANIC:return "panic";case ESP_RST_TASK_WDT:return "task_watchdog";
 case ESP_RST_INT_WDT:return "interrupt_watchdog";case ESP_RST_WDT:return "watchdog";
 case ESP_RST_SW:return "software_restart";case ESP_RST_EXT:return "external_reset";
 default:return "other_reset";}
}
static void gatewayHealthBegin(){
 gatewayResetReason=esp_reset_reason();Preferences p;
 if(p.begin("twip-net1",false)){gatewayBootId=p.getUInt("boot",0)+1;if(!gatewayBootId)gatewayBootId=1;p.putUInt("boot",gatewayBootId);p.end();}
 if(!gatewayBootId)gatewayBootId=esp_random();
 WiFi.onEvent([](WiFiEvent_t event,WiFiEventInfo_t info){
  portENTER_CRITICAL(&wifiHealthMux);
  if(event==ARDUINO_EVENT_WIFI_AP_STACONNECTED)gatewayApConnect++;
  if(event==ARDUINO_EVENT_WIFI_AP_STADISCONNECTED){gatewayApDisconnect++;gatewayApReason=info.wifi_ap_stadisconnected.reason;}
  if(event==ARDUINO_EVENT_WIFI_AP_STOP)gatewayApStop++;
  portEXIT_CRITICAL(&wifiHealthMux);
 });
}
/* RT42: chan song va chat luong duong lenh.
 * ESP32-S3 chi co radio 2.4GHz (khong co 5GHz); AP chay HT20 o kenh 1..13 - nen
 * Wi-Fi, Bluetooth/BLE, camera 2.4GHz va lo vi song deu nam cung bang. Hai so do
 * duoi day la cach duy nhat de phan biet "nhieu song" voi "song yeu":
 *   rssi = dBm cua dien thoai dang dieu khien (cang gan 0 cang tot; < -70 la yeu)
 *   phy  = 1/2/3 = dien thoai dang o 11b/11g/11n. Bi rot xuong 11b la dau hieu
 *          song xau hoac nhieu, va moi khung lenh cham di rat nhieu.
 * Cap nhat tu loop() moi ~2s bang esp_wifi_ap_get_sta_list() (re, khong chan). */
static int8_t gatewayClientRssi=0;
static uint8_t gatewayClientPhy=0;
static void gatewayPollClient(){
  wifi_sta_list_t list;
  if(esp_wifi_ap_get_sta_list(&list)!=ESP_OK||list.num<=0){gatewayClientRssi=0;gatewayClientPhy=0;return;}
  wifi_sta_info_t *st=list.sta;
  if(!st){gatewayClientRssi=0;gatewayClientPhy=0;return;}
  gatewayClientRssi=st[0].rssi;
  gatewayClientPhy=(uint8_t)(st[0].phy_11n?3:(st[0].phy_11g?2:(st[0].phy_11b?1:0)));
}
static const char *gatewayPhyText(uint8_t phy){
  switch(phy){case 1:return "11b";case 2:return "11g";case 3:return "11n";default:return "none";}
}
static void gatewayRecordStop(const char *record){
 xSemaphoreTake(captureMutex,portMAX_DELAY);snprintf(gatewayStopRecord,sizeof(gatewayStopRecord),"%s",record);xSemaphoreGive(captureMutex);
}
