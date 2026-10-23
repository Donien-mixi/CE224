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
static void gatewayRecordStop(const char *record){
 xSemaphoreTake(captureMutex,portMAX_DELAY);snprintf(gatewayStopRecord,sizeof(gatewayStopRecord),"%s",record);xSemaphoreGive(captureMutex);
}
