#include "drive8_protocol.h"

/**
  ******************************************************************************
  * @file    esp32_comm.c
  * @brief   Hiện thực giao tiếp UART1 DMA 2 chiều với ESP32-S3
  ******************************************************************************
  */

#include "esp32_comm.h"
#include "usart.h"
#include "hardware_test.h"
#include "robot_fsm.h"
#include <math.h>
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
    if(strncmp(packet,"$ARM,",5)&&strncmp(packet,"$MOVE8,",7)&&strncmp(packet,"$PULSE,",7)&&strcmp(packet,"$PING")&&strcmp(packet,"$STOP")&&strcmp(packet,"$CALIB"))return;

    /* Diagnostic mode accepts STOP only, including after finishing or aborting. */
    if (HardwareTest_IsLocked() && strncmp(packet, "$STOP", 5) != 0) return;
    if (s_command.trigger_calib && strcmp(packet,"$PING") && strncmp(packet,"$STOP",5)) return;
    if (!strncmp(packet,"$ARM,",5)) {
        int profile=0;unsigned epoch=0;char tail,canonical[48];
        bool arm_valid=!strcmp(packet,"$ARM,12");
        if(sscanf(packet,"$ARM,%d,%u%c",&profile,&epoch,&tail)==2 && profile==12 && epoch>0 && epoch<=1000000000U){
            snprintf(canonical,sizeof(canonical),"$ARM,12,%u",epoch);
            arm_valid=!strcmp(packet,canonical);
        }
        if(arm_valid &&
           !s_command.emergency_stop && !s_command.profile_request && !s_command.trigger_calib &&
           !s_command.pulse_request && !s_command.diag_request && Robot_GetData()->state!=ROBOT_STATE_CALIBRATING &&
           Robot_GetData()->state!=ROBOT_STATE_BENCH_TEST && Robot_GetData()->state!=ROBOT_STATE_BALANCING) {
            s_command.arm_drive_epoch=epoch;
            s_command.arm_profile=12;s_command.arm_request=1;
            s_command.last_cmd_time=HAL_GetTick();
        }
    } else if (!strncmp(packet,"$MOVE8,",7)) {
        Drive8Packet p;
        if(!Drive8_Parse(packet,&p) || p.epoch!=s_command.drive_epoch ||
           p.sequence<=(s_command.drive_word>>2) || s_command.emergency_stop || s_command.arm_request ||
           Robot_GetData()->state!=ROBOT_STATE_BALANCING ||
           ((p.direction!=0||p.turn!=0)&&!Robot_GetData()->motion_ready))return;
        s_command.drive_rx_ms=HAL_GetTick();s_command.drive_turn=(int8_t)p.turn;s_command.drive_speed_preset=(uint8_t)p.preset;
        s_command.drive_word=(p.sequence<<2)|(p.direction>0?1U:p.direction<0?2U:0U);
        s_command.last_cmd_time=HAL_GetTick();
    } else if (!strncmp(packet,"$MOVE,",6)) {
        return; // SPEED2-only command cannot preserve an old turn by accident.
    } else if (!strcmp(packet,"$PING")) {
        s_command.last_cmd_time=HAL_GetTick();
    } else if (strncmp(packet, "$CALIB", 6) == 0) {
        if (!strcmp(packet,"$CALIB") && !s_command.emergency_stop &&
            Robot_GetData()->state!=ROBOT_STATE_BALANCING && Robot_GetData()->state!=ROBOT_STATE_BENCH_TEST &&
            Robot_GetData()->state!=ROBOT_STATE_CALIBRATING) {
            s_command.arm_request=s_command.pulse_request=s_command.diag_request=0;
            s_command.trigger_calib=1;
        }
    } else if (!strncmp(packet,"$PULSE,",7)) {
        /* Thử bánh trên bàn: chỉ nhận khi xe đang dừng (STANDBY), không ARM/không PID. */
        int wheel=0,direction=0;char tail,canonical[24];
        if(sscanf(packet,"$PULSE,%d,%d%c",&wheel,&direction,&tail)==2 &&
           wheel>=1 && wheel<=3 && (direction==1||direction==-1) &&
           !s_command.emergency_stop && !s_command.trigger_calib &&
           Robot_GetData()->state!=ROBOT_STATE_BALANCING &&
           Robot_GetData()->state!=ROBOT_STATE_BENCH_TEST &&
           Robot_GetData()->state!=ROBOT_STATE_CALIBRATING) {
            snprintf(canonical,sizeof(canonical),"$PULSE,%d,%d",wheel,direction);
            if(!strcmp(packet,canonical)) {
                s_command.bench_test=1;
                s_command.pulse_wheel=(uint8_t)wheel;
                s_command.pulse_direction=(int8_t)direction;
                s_command.arm_request=0;
                s_command.last_cmd_time=HAL_GetTick();
            }
        }
    } else if (strncmp(packet, "$STOP", 5) == 0) {
        s_command.emergency_stop = 1;
        s_command.arm_request=0;s_command.pulse_request=0;s_command.diag_request=0;s_command.trigger_calib=0;
        s_command.profile_request=0;s_command.arm_profile=0;
        s_command.drive_epoch=s_command.drive_word=s_command.drive_rx_ms=0;s_command.drive_speed_preset=0;s_command.drive_turn=0;
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
    unsigned byte_budget=64; /* Bound work per foreground pass; preserve partial packets. */
    while (s_rx_read_idx != write_idx && byte_budget--) {
        uint8_t b = s_rx_dma_buf[s_rx_read_idx];
        s_rx_read_idx = (s_rx_read_idx + 1) % ESP32_RX_BUF_SIZE;

        if (b == '$') {
            s_in_packet = 1;
            s_packet_idx = 0;
            s_packet_buf[s_packet_idx++] = (char)b;
        } else if (s_in_packet) {
            if (b == '*' || b == '\r' || b == '\n') {
                s_packet_buf[s_packet_idx] = '\0';
                /* Keep command fields coherent to the control ISR. SysTick and
                 * DMA stay alive, including HAL timeouts in the recovery path. */
                uint32_t timer_enabled=NVIC_GetEnableIRQ(TIM4_IRQn);
                HAL_NVIC_DisableIRQ(TIM4_IRQn);
                __DSB(); __ISB();
                Parse_Packet(s_packet_buf);
                __DMB();
                if(timer_enabled)HAL_NVIC_EnableIRQ(TIM4_IRQn);
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

void ESP32_Comm_FlushRx(void)
{
    uint16_t dma_counter = __HAL_DMA_GET_COUNTER(huart1.hdmarx);
    uint16_t write_idx = (dma_counter <= ESP32_RX_BUF_SIZE) ? (ESP32_RX_BUF_SIZE - dma_counter) : 0;
    if (write_idx >= ESP32_RX_BUF_SIZE) {
        write_idx = 0;
    }

    /* Bỏ qua toàn bộ byte cũ, đồng bộ con trỏ đọc với vị trí DMA hiện tại */
    s_rx_read_idx = write_idx;
    s_packet_idx  = 0;
    s_in_packet   = 0;
}

ESP32_Command_t* ESP32_Comm_GetCommand(void)
{
    return &s_command;
}

bool ESP32_Comm_SendBytes(const void *data, uint16_t length)
{
    if (length > sizeof(s_tx_dma_buf) || huart1.gState != HAL_UART_STATE_READY)
        return false;
    memcpy(s_tx_dma_buf, data, length);
    return HAL_UART_Transmit_DMA(&huart1, s_tx_dma_buf, length) == HAL_OK;
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
