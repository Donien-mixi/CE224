/**
  ******************************************************************************
  * @file    encoder.c
  * @brief   Hiện thực hóa Driver đọc Encoder 2 bánh xe GA25.
  ******************************************************************************
  */

#include "encoder.h"

/* Biến lưu trữ dữ liệu Encoder 2 bánh */
static Encoder_Data_t enc_left  = {0};
static Encoder_Data_t enc_right = {0};

static uint16_t prev_cnt_left  = 0;
static uint16_t prev_cnt_right = 0;

void Encoder_Init(void)
{
    // Bắt đầu chế độ đếm Encoder phần cứng trên cả 2 Timer
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL); // Bánh Trái (PA0, PA1)
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); // Bánh Phải (PB4, PB5)

    Encoder_Reset();
}

void Encoder_Reset(void)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);

    prev_cnt_left  = 0;
    prev_cnt_right = 0;

    enc_left.total_ticks        = 0;
    enc_left.delta_ticks        = 0;
    enc_left.velocity_raw       = 0.0f;
    enc_left.velocity_filtered  = 0.0f;

    enc_right.total_ticks       = 0;
    enc_right.delta_ticks       = 0;
    enc_right.velocity_raw      = 0.0f;
    enc_right.velocity_filtered = 0.0f;
}

void Encoder_Update(float dt)
{
    if (dt <= 0.0001f) return;

    // 1. Đọc thanh ghi bộ đếm Timer hiện tại
    uint16_t curr_cnt_left  = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
    uint16_t curr_cnt_right = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);

    // 2. Tính số xung biến thiên (Ép kiểu int16_t để xử lý tràn số quay vòng tự động)
    // Lưu ý: Bánh Phải quay ngược chiều cơ học so với bánh Trái nên đảo dấu (-)
    int16_t delta_l = (int16_t)(curr_cnt_left - prev_cnt_left);
    int16_t delta_r = -(int16_t)(curr_cnt_right - prev_cnt_right);

    prev_cnt_left  = curr_cnt_left;
    prev_cnt_right = curr_cnt_right;

    enc_left.delta_ticks  = delta_l;
    enc_right.delta_ticks = delta_r;

    enc_left.total_ticks  += delta_l;
    enc_right.total_ticks += delta_r;

    // 3. Tính vận tốc tịnh tiến tức thời v = (delta_ticks * distance_per_tick) / dt (m/s)
    enc_left.velocity_raw  = ((float)delta_l * METERS_PER_TICK) / dt;
    enc_right.velocity_raw = ((float)delta_r * METERS_PER_TICK) / dt;

    // 4. Lọc thông thấp bậc 1 (Low-Pass Filter) để khử nhiễu lượng tử hóa ở tốc độ chậm
    enc_left.velocity_filtered = (1.0f - ENCODER_LPF_BETA) * enc_left.velocity_raw 
                               + ENCODER_LPF_BETA * enc_left.velocity_filtered;

    enc_right.velocity_filtered = (1.0f - ENCODER_LPF_BETA) * enc_right.velocity_raw 
                                + ENCODER_LPF_BETA * enc_right.velocity_filtered;
}

float Encoder_GetLeftVelocity(void)
{
    return enc_left.velocity_filtered;
}

float Encoder_GetRightVelocity(void)
{
    return enc_right.velocity_filtered;
}

float Encoder_GetAverageVelocity(void)
{
    return (enc_left.velocity_filtered + enc_right.velocity_filtered) * 0.5f;
}
