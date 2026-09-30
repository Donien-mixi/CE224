/**
  ******************************************************************************
  * @file    esp32_comm.c
  * @brief   Hiện thực giao tiếp UART1 DMA 2 chiều với ESP32-S3
  ******************************************************************************
  */

#include "esp32_comm.h"
#include "usart.h"
#include "pid.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint8_t s_rx_dma_buf[ESP32_RX_BUF_SIZE];
static uint16_t s_rx_read_idx = 0;

static uint8_t s_tx_dma_buf[ESP32_TX_BUF_SIZE];

static ESP32_Command_t s_command = {
    .v_target = 0.0f,
    .steer_cmd = 0.0f,
    .is_racing = 0,
    .trigger_calib = 0,
    .emergency_stop = 0,
    .bench_test = 0,
    .last_cmd_time = 0
};

#define PARSER_BUF_SIZE 64
static char s_packet_buf[PARSER_BUF_SIZE];
static uint8_t s_packet_idx = 0;
static uint8_t s_in_packet = 0;

void ESP32_Comm_Init(void)
{
    memset(s_rx_dma_buf, 0, sizeof(s_rx_dma_buf));
    s_rx_read_idx = 0;
    s_packet_idx = 0;
    s_in_packet = 0;
    s_command.last_cmd_time = HAL_GetTick();

    /* Xóa sạch các cờ lỗi phần cứng trước khi mở kênh DMA */
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_NEFLAG(&huart1);
    __HAL_UART_CLEAR_FEFLAG(&huart1);
    __HAL_UART_CLEAR_PEFLAG(&huart1);
    huart1.ErrorCode = HAL_UART_ERROR_NONE;

    /* Bật DMA nhận chế độ xoay vòng (Circular Mode) */
    HAL_UART_Receive_DMA(&huart1, s_rx_dma_buf, ESP32_RX_BUF_SIZE);
}

/**
  * @brief  Tự động phục hồi khi UART1 gặp lỗi Overrun (ORE), Framing (FE), Noise (NE)
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_PEFLAG(huart);
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        huart->RxState = HAL_UART_STATE_READY;
        HAL_UART_Receive_DMA(huart, s_rx_dma_buf, ESP32_RX_BUF_SIZE);
    }
}

static void Parse_Packet(const char* packet)
{
    if (strncmp(packet, "$CMD,", 5) == 0) {
        char* endp = NULL;
        float v = strtof(packet + 5, &endp);
        float st = 0.0f;
        if (endp && *endp == ',') {
            st = strtof(endp + 1, NULL);
        }
        s_command.v_target = v;
        s_command.steer_cmd = st;
        s_command.last_cmd_time = HAL_GetTick();
    } else if (strncmp(packet, "$PID,", 5) == 0) {
        char* p = (char*)packet + 5;
        float kp1 = strtof(p, &p); if (*p == ',') p++;
        float kd1 = strtof(p, &p); if (*p == ',') p++;
        float kp2 = strtof(p, &p); if (*p == ',') p++;
        float ki2 = strtof(p, NULL);
        PID_SetParams(kp1, kd1, kp2, ki2);
    } else if (strncmp(packet, "$TRIM,", 6) == 0) {
        float trim = strtof(packet + 6, NULL);
        PID_SetTrim(trim);
    } else if (strncmp(packet, "$RACE,", 6) == 0) {
        s_command.is_racing = (uint8_t)strtol(packet + 6, NULL, 10);
    } else if (strncmp(packet, "$BENCH,", 7) == 0) {
        s_command.bench_test = (uint8_t)strtol(packet + 7, NULL, 10);
        s_command.last_cmd_time = HAL_GetTick();
    } else if (strncmp(packet, "$CALIB", 6) == 0) {
        s_command.trigger_calib = 1;
    } else if (strncmp(packet, "$STOP", 5) == 0) {
        s_command.emergency_stop = 1;
        s_command.v_target = 0.0f;
        s_command.steer_cmd = 0.0f;
        s_command.bench_test = 0;
    }
}

void ESP32_Comm_Process(void)
{
    /* Cơ chế tự phục hồi Watchdog TOÀN DIỆN nếu DMA RX bị chết:
     * CHỈ clear cờ lỗi ORE/FE/NE/PE và đọc SR/DR khi THỰC SỰ có lỗi phần cứng hoặc DMA bị tắt.
     * TUYỆT ĐỐI không đọc DR vô điều kiện trong while(1) vì sẽ cướp byte dữ liệu của DMA! */
    if (huart1.ErrorCode != HAL_UART_ERROR_NONE ||
        huart1.RxState == HAL_UART_STATE_READY ||
        !(huart1.hdmarx->Instance->CR & DMA_SxCR_EN)) {

        __HAL_UART_CLEAR_OREFLAG(&huart1);
        __HAL_UART_CLEAR_NEFLAG(&huart1);
        __HAL_UART_CLEAR_FEFLAG(&huart1);
        __HAL_UART_CLEAR_PEFLAG(&huart1);
        (void)USART1->SR;
        (void)USART1->DR;
        huart1.ErrorCode = HAL_UART_ERROR_NONE;
        huart1.RxState = HAL_UART_STATE_READY;

        /* Reset read pointer để đồng bộ với DMA counter mới */
        s_rx_read_idx = 0;
        s_packet_idx = 0;
        s_in_packet = 0;

        HAL_UART_Receive_DMA(&huart1, s_rx_dma_buf, ESP32_RX_BUF_SIZE);
    }

    /* 1. Tính toán vị trí ghi hiện tại của phần cứng DMA RX */
    uint16_t dma_counter = __HAL_DMA_GET_COUNTER(huart1.hdmarx);
    uint16_t write_idx = (dma_counter <= ESP32_RX_BUF_SIZE) ? (ESP32_RX_BUF_SIZE - dma_counter) : 0;
    if (write_idx >= ESP32_RX_BUF_SIZE) {
        write_idx = 0;
    }

    /* 2. Đọc tuần tự các byte mới từ bộ đệm xoay vòng */
    while (s_rx_read_idx != write_idx) {
        uint8_t b = s_rx_dma_buf[s_rx_read_idx];
        s_rx_read_idx = (s_rx_read_idx + 1) % ESP32_RX_BUF_SIZE;

        if (b == '$') {
            s_in_packet = 1;
            s_packet_idx = 0;
            s_packet_buf[s_packet_idx++] = (char)b;
        } else if (s_in_packet) {
            if (b == '*' || b == '\r' || b == '\n') {
                s_packet_buf[s_packet_idx] = '\0';
                Parse_Packet(s_packet_buf);
                s_in_packet = 0;
                s_packet_idx = 0;
            } else if (s_packet_idx < (PARSER_BUF_SIZE - 1)) {
                s_packet_buf[s_packet_idx++] = (char)b;
            } else {
                /* Tràn bộ đệm gói tin -> Hủy */
                s_in_packet = 0;
                s_packet_idx = 0;
            }
        }
    }

    /* 3. Failsafe Timeout: Mất kết nối điều khiển quá 1000ms -> Dừng ga/lái */
    if (HAL_GetTick() - s_command.last_cmd_time > 1000) {
        s_command.v_target = 0.0f;
        s_command.steer_cmd = 0.0f;
    }
}

ESP32_Command_t* ESP32_Comm_GetCommand(void)
{
    return &s_command;
}

bool ESP32_Comm_SendTelemetry(float pitch, float gyro, float v_act, float v_tgt,
                              int16_t pwm_l, int16_t pwm_r, uint8_t state, float batt_volt,
                              float v_l, float v_r)
{
    /* Nếu kênh TX DMA đang bận phát gói trước thì bỏ qua lượt này */
    if (huart1.gState != HAL_UART_STATE_READY) {
        return false;
    }

    int len = snprintf((char*)s_tx_dma_buf, ESP32_TX_BUF_SIZE,
                       "$TEL,%.2f,%.2f,%.2f,%.2f,%d,%d,%u,%.2f,%.2f,%.2f\r\n",
                       pitch, gyro, v_act, v_tgt, pwm_l, pwm_r, (unsigned int)state, batt_volt, v_l, v_r);

    if (len > 0 && len < ESP32_TX_BUF_SIZE) {
        if (HAL_UART_Transmit_DMA(&huart1, s_tx_dma_buf, (uint16_t)len) == HAL_OK) {
            return true;
        }
    }
    return false;
}
