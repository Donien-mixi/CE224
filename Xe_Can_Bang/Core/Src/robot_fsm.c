/**
 ******************************************************************************
 * @file    robot_fsm.c
 * @brief   Hiện thực Máy trạng thái FSM và vòng lặp điều khiển thời gian thực
 * 200Hz
 ******************************************************************************
 */

#include "robot_fsm.h"
#include "bmi160.h"
#include "buzzer_led.h"
#include "encoder.h"
#include "esp32_comm.h"
#include "filter.h"
#include "motor.h"
#include "pid.h"
#include "tim.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware_test.h"
#include "balance_profile.h"
#include "balance_wire.h"
#include "phase1_catalog_control.h"
#include "balance_startup.h"
#include "balance_recovery.h"
#include "balance_governor.h"
static BalanceGovernor bal_governor;
#include "balance_motion.h"
#include "balance_turn.h"

/* Hook governor cho quỹ đạo dọc: balance_motion.c có bản yếu, ở đây cài bản mạnh
 * để nối ramp/neo vào bộ trần tốc độ-lực duy nhất của xe. */
float BalanceMotion_FilterHook(float old,float target,float rate,int wanted,float pitch,float gyro){
    return BalanceGovernor_Step(&bal_governor,old,target,rate,wanted,pitch,gyro);
}
void BalanceMotion_AnchorHook(float *ref,float position,int wanted){
    BalanceGovernor_Anchor(&bal_governor,ref,position,wanted);
}
int BalanceMotion_GovernorActive(void){ return 1; }

#include "balance_interlock.h"
#include "balance_trim.h"

#include "phase2_motion_control.h"
static BalanceStartup bal_startup={.budget=.30f,.angle_limit=25.0f};
static BalanceRecovery bal_recovery;
static BalanceMotion bal_motion;
static BalanceTurn bal_turn;
static uint32_t bal_last_cycle;
static uint16_t bal_still;
static const uint8_t bal_controller=BALANCE_DEFAULT_CONTROLLER;

static Robot_Data_t s_robot = {.state = ROBOT_STATE_INIT,
                               .pitch = 0.0f,
                               .gyro_rate = 0.0f,
                               .x_actual = 0.0f,
                               .x_target = 0.0f,
                               .v_actual = 0.0f,
                               .v_left = 0.0f,
                               .v_right = 0.0f,
                               .v_target = 0.0f,
                               .steer_cmd = 0.0f,
                               .pwm_left = 0,
                               .pwm_right = 0,
                               .batt_voltage = 0.0f, // Not measured: never present a guessed voltage as ADC data
                               .loop_count = 0};
static bool bal_config_ok;
static volatile bool bal_imu_failed;
static bool bal_max_envelope;
/* Số nhịp 200 Hz liên tiếp vượt trần giám sát tốc độ. */
static BalanceInterlock bal_interlock;
static uint16_t TestCRC(const uint8_t *data, unsigned length) {
    uint16_t crc = 0xffff;
    while (length--) {
        crc ^= (uint16_t)*data++ << 8;
        for (unsigned i = 0; i < 8; i++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}
bool HardwareTest_IsLocked(void){return false;}
void HardwareTest_Service(void){}
void Robot_Init(void) {
  /* 1. Khởi tạo còi, LED, cơ cấu chấp hành */
  BuzzerLED_Init();
  Motor_Init();
  Motor_Stop();
  Encoder_Init();
  PID_Init();
  Filter_SelectController(bal_controller);

  /* Preserve the existing startup motor exercise: PWM 1600, 600 ms per wheel.
   * This is an output command, not proof of supply voltage or motor health. */

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  /* HAL I2C timeout must be able to advance inside TIM4 IRQ. */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
  HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
  ESP32_Comm_Init();

  /* 2. Khởi tạo cảm biến Bosch BMI160 */
  if (!BMI160_Init()) {
    bal_imu_failed=true;

    s_robot.state = ROBOT_STATE_EMERGENCY;
    LED_SetPattern(LED_PATTERN_ALARM);
    Buzzer_On();
    return;
  }

  /* 3. Hiệu chuẩn bù độ trôi Gyro */
  s_robot.state = ROBOT_STATE_CALIBRATING;
  LED_SetPattern(LED_PATTERN_FAST_BLINK);
  Buzzer_BeepAsync(100);

  BMI160_Calibrate_Gyro(500);

  /* 4. Khởi tạo góc ban đầu cho bộ lọc bù */
  BMI160_Data_t init_imu;
  if (!BMI160_Read_All(&init_imu)) {

    s_robot.state = ROBOT_STATE_EMERGENCY; return;
  }
  bal_imu_failed=false;
  uint8_t cfg[6];
  bal_config_ok=BMI160_GetConfig(cfg)&&cfg[1]==0xd1&&cfg[2]==0x28&&cfg[3]==8&&cfg[4]==0x28&&cfg[5]==0;
  if(!bal_config_ok)s_robot.fault|=256;
  float init_pitch = Filter_CalibratedRawPitch(init_imu.ax, init_imu.az);
  Filter_Init(init_pitch);

  /* 5. Chuyển sang trạng thái chờ dựng xe */
  bal_imu_failed=false;
  s_robot.state = ROBOT_STATE_STANDBY;
  LED_SetPattern(LED_PATTERN_SLOW_BLINK);
  Buzzer_BeepAsync(200);

  /* Discard commands accumulated during the blocking startup calibration. */
  ESP32_Comm_FlushRx();

  /* Replace the old blind deadband ramp with measured, bounded diagnostics. */

}

void Robot_RecalibrateIMU(void) {
  if (HardwareTest_IsLocked() || s_robot.state==ROBOT_STATE_BALANCING || s_robot.state==ROBOT_STATE_BENCH_TEST) return;
  /* Tạm dừng vòng điều khiển 200Hz để tránh truy cập I2C chồng chéo trong lúc
   * hiệu chuẩn */
  HAL_TIM_Base_Stop_IT(&htim4);

  /* Calibration invalidates all queued motor actions and previous readiness.
   * A fresh ARM must be requested after a new 200ms stillness dwell. */
  ESP32_Command_t *cmd=ESP32_Comm_GetCommand();
  cmd->arm_request=cmd->pulse_request=cmd->diag_request=cmd->trigger_calib=0;
  cmd->drive_epoch=cmd->drive_word=cmd->drive_rx_ms=0;cmd->drive_speed_preset=0;cmd->drive_turn=0;
  s_robot.ready=s_robot.motion_ready=0;bal_still=0;bal_last_cycle=0;
  BalanceGovernor_Reset(&bal_governor);BalanceMotion_Clear(&bal_motion);BalanceTurn_Clear(&bal_turn);
  BalanceRecovery_Clear(&bal_recovery);BalanceStartup_Stop(&bal_startup);

  Motor_Stop();
  s_robot.pwm_left = 0;
  s_robot.pwm_right = 0;

  s_robot.state = ROBOT_STATE_CALIBRATING;
  LED_SetPattern(LED_PATTERN_FAST_BLINK);
  Buzzer_BeepAsync(100);

  /* Recovery is attempted only in main, with motors stopped and TIM4 paused.
   * Do not re-enter a stuck HAL I2C transaction from every200Hz interrupt. */
  if((bal_imu_failed||(s_robot.fault&(32U|256U)))&&!BMI160_Init()){
    bal_imu_failed=true;s_robot.fault|=32;s_robot.state=ROBOT_STATE_EMERGENCY;
    LED_SetPattern(LED_PATTERN_ALARM);Buzzer_On();
    __HAL_TIM_SET_COUNTER(&htim4,0);HAL_TIM_Base_Start_IT(&htim4);return;
  }
  /* 500 read attempts at the configured 100 Hz (~5 seconds). */
  BMI160_Calibrate_Gyro(500);

  /* Khởi tạo lại góc ban đầu cho bộ lọc bù */
  BMI160_Data_t imu;
  if (!BMI160_Read_All(&imu)) {
    bal_imu_failed=true;
    s_robot.state = ROBOT_STATE_EMERGENCY;
    LED_SetPattern(LED_PATTERN_ALARM);
    Buzzer_On();
    s_robot.fault|=32;s_robot.ready=0;
    __HAL_TIM_SET_COUNTER(&htim4,0);HAL_TIM_Base_Start_IT(&htim4);
    return; /* Keep fault telemetry alive; motor stays stopped. */
  }

  uint8_t cfg[6];
  bal_config_ok=BMI160_GetConfig(cfg)&&cfg[1]==0xd1&&cfg[2]==0x28&&cfg[3]==8&&cfg[4]==0x28&&cfg[5]==0;
  s_robot.fault=(bal_config_ok?0:256)|(BMI160_CalibrationOK()?0:128);
  Filter_Init(Filter_CalibratedRawPitch(imu.ax, imu.az));
  PID_Reset_Integral();
  Encoder_Reset();

  bal_imu_failed=false;
  s_robot.state = ROBOT_STATE_STANDBY;
  LED_SetPattern(LED_PATTERN_SLOW_BLINK);
  Buzzer_BeepAsync(200);

  __HAL_TIM_SET_COUNTER(&htim4, 0);
  HAL_TIM_Base_Start_IT(&htim4);
}


/* SPSC IRQ->main queue; serialization and UART DMA run outside the control IRQ. */
#define BAL_QUEUE 32
static BalanceFrame bal_queue[BAL_QUEUE];
static volatile uint8_t bal_head,bal_tail;
static uint32_t bal_seq,bal_meta_ms;
static uint16_t bal_drops;
static uint8_t pulse_wheel;
static float bal_catalog[CAT_N+3];
static uint32_t bal_armed_ms;
static void BalanceStop(uint16_t reason) {
    bal_max_envelope=false;
    BalanceInterlock_Clear(&bal_interlock);
    BalanceStartup_Stop(&bal_startup);
    BalanceRecovery_Clear(&bal_recovery);
    /* STOP/re-ARM giữ tư thế trung tính đã học (kẹp ±2°); nó chỉ dùng cho bộ dự
 * đoán headroom, không phải điểm đặt điều khiển. */
    BalanceGovernor_ResetKeepEquilibrium(&bal_governor);BalanceMotion_Clear(&bal_motion);BalanceTurn_Clear(&bal_turn);s_robot.motion_ready=0;
    ESP32_Command_t *motion_command=ESP32_Comm_GetCommand();
    motion_command->drive_epoch=motion_command->drive_word=motion_command->drive_rx_ms=0;motion_command->drive_speed_preset=0;motion_command->drive_turn=0;
    s_robot.fault|=reason; s_robot.state=ROBOT_STATE_FALLEN;pulse_wheel=0;
    Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;
    s_robot.v_control=s_robot.theta_control=s_robot.pwm_base=0;
    PID_Reset_Integral();cat_reset(bal_catalog,0);bal_still=0;
}
void Robot_BalanceService(void) {

    if(bal_tail!=bal_head) {
        if(ESP32_Comm_SendBytes(&bal_queue[bal_tail],sizeof(BalanceFrame))) {
            __DMB();bal_tail=(bal_tail+1)%BAL_QUEUE;
        }
        return;
    }
    if(HAL_GetTick()-bal_meta_ms>=1000) {
        float bias[3];BMI160_GetBias(bias);
        char meta[256];uint32_t health[4];BMI160_GetHealth(health);
        uint8_t profile=bal_controller;
        float gains[5];
        for(unsigned i=0;i<5;i++)gains[i]=i<4?CAT_K[i]:CAT_PARAM[11];
        int n=snprintf(meta,sizeof(meta),"$BINFO,%s,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.9f,1,%u,%.3f,%.1f,%lu,%lu,%lu,%lu,%.3f,%u\r\n",
            BALANCE_BUILD,gains[0],gains[1],gains[2],gains[3],gains[4],
            BALANCE_PPR,BALANCE_DB_L_POS,BALANCE_DB_L_NEG,BALANCE_DB_R_POS,BALANCE_DB_R_NEG,
            bias[0],bias[1],bias[2],BALANCE_RAW_ZERO_DEG,profile,
            COMP_FILTER_ALPHA_R12,
            BALANCE_ARM_ANGLE_FOR(profile),(unsigned long)health[0],(unsigned long)health[1],
            (unsigned long)health[2],(unsigned long)health[3],bal_governor.equilibrium,
            (unsigned)bal_governor.equilibrium_ready);
        if(n>0&&n<(int)sizeof(meta)&&ESP32_Comm_SendBytes(meta,n))bal_meta_ms=HAL_GetTick();
    }
}
static float BalanceTurn_Yaw(void){int32_t l,r;Encoder_GetHardwareTicks(&l,&r);return (-(float)r-(float)l)*METERS_PER_TICK/CAT_PARAM[0];}
void Robot_ControlLoop_200Hz(void) {
    uint32_t begin=DWT->CYCCNT;
    uint32_t spacing=bal_last_cycle?(uint32_t)(begin-bal_last_cycle)/(SystemCoreClock/1000000U):5000;
    bal_last_cycle=begin;s_robot.loop_count++;
    ESP32_Command_t *cmd=ESP32_Comm_GetCommand();
    BMI160_Data_t imu={0};bool valid=!bal_imu_failed&&BMI160_Read_All(&imu);
    if(!valid) {bal_imu_failed=true;BalanceStop(32);s_robot.state=ROBOT_STATE_EMERGENCY;}
    else {
        /* No runtime controller switching in the finalized R12 baseline. */
        cmd->profile_request=0;
        s_robot.pitch=Filter_Complementary_Update(imu.gy,imu.ax,imu.az,0.005f);
        s_robot.gyro_rate=PITCH_GYRO_SIGN*imu.gy;
        Encoder_Update(0.005f);
        s_robot.v_left=Encoder_GetLeftVelocity();s_robot.v_right=Encoder_GetRightVelocity();
        s_robot.v_actual=Encoder_GetAverageVelocity();
        /* Same encoder frame as the force controller. Integrating the LPF
         * velocity delayed the supervisory anchor by~1.5cm at MAX95. Counts
         * are never reset by a press/release/recovery, only while stopped/ARM. */
        int32_t ticks_l,ticks_r;Encoder_GetHardwareTicks(&ticks_l,&ticks_r);
        s_robot.x_actual=.5f*((float)ticks_l-(float)ticks_r)*METERS_PER_TICK;
    }
    /* Verify register setup outside IRQ at initialization/recalibration instead. */
    s_robot.v_target=s_robot.steer_cmd=0;
    if(cmd->emergency_stop) {cmd->emergency_stop=0;cmd->arm_request=cmd->pulse_request=cmd->profile_request=0;BalanceStop(1);}
    if(!BMI160_CalibrationOK()) {s_robot.fault|=128;bal_still=0;}
    float norm=imu.ax*imu.ax+imu.ay*imu.ay+imu.az*imu.az;
    bool finite=isfinite(s_robot.pitch)&&isfinite(s_robot.gyro_rate)&&
        isfinite(s_robot.v_actual)&&isfinite(s_robot.x_actual);
    if(!finite)BalanceStop(512);
    if(valid&&finite&&BMI160_CalibrationOK()&&bal_config_ok&&
       s_robot.state!=ROBOT_STATE_EMERGENCY && fabsf(s_robot.pitch)<=BALANCE_ARM_ANGLE_FOR(bal_controller)&&
       fabsf(imu.gx)<BALANCE_ARM_GYRO && fabsf(imu.gy)<BALANCE_ARM_GYRO && fabsf(imu.gz)<BALANCE_ARM_GYRO &&
       fabsf(s_robot.v_left)<BALANCE_ARM_WHEEL_SPEED && fabsf(s_robot.v_right)<BALANCE_ARM_WHEEL_SPEED &&
       norm>.72f&&norm<1.32f && spacing>=4000&&spacing<=6000) {
        if(bal_still<BALANCE_ARM_STILL_CYCLES)bal_still++;
    } else bal_still=0;
    s_robot.ready=bal_still>=BALANCE_ARM_STILL_CYCLES;
    bool just_armed=false;
    if(cmd->arm_request) {
        cmd->arm_request=0;
        if(s_robot.ready && cmd->arm_profile==bal_controller && s_robot.state!=ROBOT_STATE_BALANCING && s_robot.state!=ROBOT_STATE_BENCH_TEST) {
            PID_Reset_Integral();Encoder_Reset();s_robot.x_actual=s_robot.x_target=0;
            cat_reset(bal_catalog,s_robot.pitch*DEG_TO_RAD);
            bal_armed_ms=HAL_GetTick();
            bal_max_envelope=false;BalanceInterlock_Clear(&bal_interlock);
            BalanceStartup_Begin(&bal_startup,bal_controller,s_robot.pitch,bal_armed_ms);
            BalanceRecovery_Clear(&bal_recovery);
            /* Giữ tư thế trung tính qua ARM; sẽ đo lại khi xe đứng yên. */
            BalanceGovernor_ResetKeepEquilibrium(&bal_governor);BalanceMotion_Clear(&bal_motion);BalanceTurn_Clear(&bal_turn);s_robot.motion_ready=0;
            cmd->drive_word=cmd->drive_rx_ms=0;cmd->drive_epoch=cmd->arm_drive_epoch;cmd->drive_speed_preset=0;cmd->drive_turn=0;
            s_robot.v_actual=s_robot.v_left=s_robot.v_right=0;
            s_robot.fault=0;s_robot.state=ROBOT_STATE_BALANCING;just_armed=true;
        }
    }
    if(s_robot.state==ROBOT_STATE_BALANCING) {
        do {
        if(HAL_GetTick()-cmd->last_cmd_time>BALANCE_HEARTBEAT_MS){BalanceStop(2);break;}
        uint16_t impact_fault=BalanceMotion_Impact(&bal_motion,!bal_startup.active&&!just_armed,s_robot.gyro_rate,norm);
        if(impact_fault){BalanceStop(impact_fault);break;}
        uint16_t startup_fault=BalanceStartup_Update(&bal_startup,s_robot.pitch,s_robot.gyro_rate,s_robot.v_actual,HAL_GetTick());
        if(startup_fault){BalanceStop(startup_fault);break;}
        BalanceRecovery_Impact(&bal_recovery,!bal_startup.active&&!just_armed,bal_motion.used,
            s_robot.gyro_rate,norm,s_robot.x_actual-bal_motion.x_ref,bal_startup.budget,HAL_GetTick());
        /* Một trần duy nhất cho "dự phòng vượt tốc", tính TRƯỚC quỹ đạo và interlock để
 * REC1 kịp nâng trần ngay trong cùng nhịp. */
        BalanceRecovery_OverSpeed(&bal_recovery,!bal_startup.active&&!just_armed,
            s_robot.v_actual,bal_motion.v_ref,s_robot.x_actual-bal_motion.x_ref,
            bal_startup.budget,bal_max_envelope?1.12f:BalanceStartup_SpeedLimit(&bal_startup),
            HAL_GetTick());
        uint16_t recovery_fault=BalanceRecovery_UpdateMode(&bal_recovery,
            s_robot.state==ROBOT_STATE_BALANCING&&!bal_startup.active&&!just_armed,
            s_robot.pitch,s_robot.gyro_rate,s_robot.v_actual-bal_motion.v_ref,s_robot.x_actual-bal_motion.x_ref,
            bal_startup.budget,HAL_GetTick(),BalanceMotion8_Maneuver(&bal_motion,&bal_turn,HAL_GetTick()));
        if(recovery_fault){BalanceStop(recovery_fault);break;}
        if(cmd->drive_speed_preset==2&&!bal_startup.active&&!just_armed)bal_max_envelope=true;
        if(cmd->drive_speed_preset!=2&&fabsf(s_robot.v_actual)<.76f&&fabsf(bal_motion.v_ref)<.73f)bal_max_envelope=false;
        bal_motion.speed_preset=cmd->drive_speed_preset;
        BalanceGovernor_Observe(&bal_governor,s_robot.pwm_left,s_robot.pwm_right,s_robot.v_actual);
        BalanceGovernor_ObserveRest(&bal_governor,!bal_startup.active&&!bal_recovery.active&&
            !(cmd->drive_word&3U)&&!cmd->drive_turn&&!bal_motion.braking&&!bal_turn.braking&&
            !bal_motion.cancel_flags&&fabsf(bal_motion.v_ref)<.01f&&fabsf(bal_turn.w_ref)<.01f&&
            norm>.72f&&norm<1.32f,
            s_robot.pitch,s_robot.gyro_rate,s_robot.v_actual,(s_robot.v_right-s_robot.v_left)/CAT_PARAM[0]);
        int8_t previous_turn=bal_turn.requested;
        /* Taper nghiêng tính trên độ nghiêng vượt tư thế trung tính, nên cú quay bình
 * thường không tự bóp tốc độ quay. */
        bal_turn.lean_ref=bal_governor.equilibrium_ready?bal_governor.equilibrium:0.0f;
        /* Đang cua: giữ ít nhất một bánh khỏi trần duty. Trần cua chỉ tác dụng khi có
 * lệnh quay; chạy thẳng không đổi nên vẫn đạt mốc đã chọn. */
        BalanceTurn_headroom(BalanceTurn_headroom_next(
            fmaxf(fabsf((float)s_robot.pwm_left),fabsf((float)s_robot.pwm_right))/2499.0f,
            cmd->drive_turn!=0));
        /* Cùng bằng chứng duty đó cũng hạ trần MAX ở đường thẳng, nhưng chỉ khi KHÔNG
 * cua, để trần cua giữ thang riêng. */
        BalanceMotion_maxhead(BalanceMotion_maxhead_next(
            fmaxf(fabsf((float)s_robot.pwm_left),fabsf((float)s_robot.pwm_right))/2499.0f,
            cmd->drive_turn==0&&cmd->drive_speed_preset==2&&fabsf(bal_motion.v_ref)>.80f));
        unsigned drive_mode=BalanceRecovery_DriveMode(&bal_recovery,!bal_startup.active&&!just_armed,
            s_robot.pitch,s_robot.gyro_rate,s_robot.v_actual,bal_motion.v_ref);
        if(!BalanceMotion8_Update(&bal_motion,&bal_turn,
            drive_mode,
            cmd->drive_word,cmd->drive_turn,s_robot.x_actual,s_robot.pitch,s_robot.gyro_rate,s_robot.v_actual,
            BalanceTurn_Yaw(),(s_robot.v_right-s_robot.v_left)/CAT_PARAM[0],HAL_GetTick())){BalanceStop(512);break;}
        if(bal_governor.limited)bal_motion.flags|=16384U;
        if(previous_turn*bal_turn.requested<0)bal_catalog[CAT_N]=0;
        bal_motion.flags|=(cmd->drive_speed_preset==2?4096U:0U)|(bal_max_envelope?8192U:0U);
        s_robot.motion_ready=(bal_motion.flags&1U)!=0;
        s_robot.x_target=bal_motion.x_ref;s_robot.v_target=bal_motion.v_ref;
        uint16_t motion_fault=BalanceMotion_Fault(&bal_motion,s_robot.x_actual,HAL_GetTick());
        if(motion_fault){BalanceStop(motion_fault);break;}
        if(fabsf(s_robot.pitch)>BalanceRecovery_Angle(&bal_recovery,bal_startup.angle_limit)){BalanceStop(4);break;}
        if(fabsf(s_robot.x_actual-bal_motion.x_ref)>BalanceRecovery_Distance(&bal_recovery,bal_startup.budget)){BalanceStop(8);break;}
        /* Mẫu vượt trần là quán tính khi vòng R12 đang bắt nghiêng, không phải lệnh
 * dừng: cắt motor lúc đó chính là làm xe ngã. Trần đi theo lệnh (|v_ref|+0,12),
 * cần 200 ms vượt liên tục, và mốc 1,45 m/s vẫn cắt ngay. */
        float speed_ceiling=bal_max_envelope?1.12f:BalanceRecovery_Speed(&bal_recovery,BalanceStartup_SpeedLimit(&bal_startup));
        if(BalanceInterlock_Step(&bal_interlock,s_robot.v_actual,s_robot.v_left,s_robot.v_right,
            speed_ceiling,bal_motion.v_ref,bal_max_envelope)){BalanceStop(16);break;}
        if(spacing<4000||spacing>6000)BalanceStop(64);
        } while(0);
    }
    if(s_robot.state==ROBOT_STATE_BALANCING && bal_controller==BALANCE_CONTROLLER_R12) {
        /* Frozen R12 float32 law; same header as Scilab native experiment.
         * Raw ticks are L forward-positive, R forward-negative. No extra DB. */
        int32_t ticks_l,ticks_r;Encoder_GetHardwareTicks(&ticks_l,&ticks_r);
        float input[7]={s_robot.pitch*DEG_TO_RAD,s_robot.gyro_rate*DEG_TO_RAD,
            PITCH_ACC_SIGN*Filter_CalibratedRawPitch(imu.ax,imu.az)*DEG_TO_RAD,
            .5f*((float)ticks_l-(float)ticks_r)*METERS_PER_TICK,
            s_robot.v_left,s_robot.v_right,
            (-(float)ticks_r-(float)ticks_l)*METERS_PER_TICK};
        float output[4];
        if(!motion_cat_step8_mode(input,bal_catalog,output,bal_motion.x_ref,bal_motion.v_ref,bal_turn.psi_ref,bal_turn.w_ref,bal_turn.rate_mode))BalanceStop(512);
        else if(just_armed) {
            /* Observer updates once; first ARM tick still holds brake. */
            bal_catalog[CAT_N]=bal_catalog[CAT_N+2]=0;
            Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;
            s_robot.pwm_base=0;
            s_robot.v_control=bal_catalog[1];s_robot.theta_control=bal_catalog[2]*RAD_TO_DEG;
        } else {
            /* Bù lệch cơ khí bánh trái — xem balance_trim.h. Đặt SAU bộ phân bổ R12,
 * kẹp ±MOTOR_MAX_PWM. */
            float pwm_l=BalanceTrim_Left(output[0],BALANCE_TRIM_LEFT_PWM,(float)MOTOR_MAX_PWM);
            s_robot.pwm_left=(int16_t)pwm_l;s_robot.pwm_right=(int16_t)output[1];
            s_robot.pwm_base=.5f*(pwm_l+output[1]);
            s_robot.v_control=bal_catalog[1];s_robot.theta_control=bal_catalog[2]*RAD_TO_DEG;
            Motor_SetDuty(s_robot.pwm_left,s_robot.pwm_right);
        }
    } else {
        Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;
        s_robot.v_control=s_robot.theta_control=s_robot.pwm_base=0;
    }
    uint32_t execution=(uint32_t)(DWT->CYCCNT-begin)/(SystemCoreClock/1000000U);
    if(execution>4000 && (s_robot.state==ROBOT_STATE_BALANCING||s_robot.state==ROBOT_STATE_BENCH_TEST))BalanceStop(64);
    if(s_robot.loop_count%4==0) {
        BalanceFrame f={0};BMI160_RawData_t raw;BMI160_GetLastRaw(&raw);
        f.sync=0x60a5;f.sequence=bal_seq++;f.t_ms=HAL_GetTick();
        f.dt_us=spacing>65535?65535:spacing;f.execution_us=execution>65535?65535:execution;
        f.gx=raw.gx_raw;f.gy=raw.gy_raw;f.gz=raw.gz_raw;
        f.ax=raw.ax_raw;f.ay=raw.ay_raw;f.az=raw.az_raw;
        int32_t raw_l,raw_r;Encoder_GetHardwareTicks(&raw_l,&raw_r);
        f.ticks_l=raw_l;f.ticks_r=raw_r;
        f.pwm_l=s_robot.pwm_left;f.pwm_r=s_robot.pwm_right;
        f.pitch=s_robot.pitch;f.gyro=s_robot.gyro_rate;
        f.v_l=s_robot.v_left;f.v_r=s_robot.v_right;f.x=s_robot.x_actual;
        f.v_cmd=s_robot.v_control;f.theta_cmd=s_robot.theta_control;f.pwm_base=s_robot.pwm_base;
        f.integral=bal_catalog[CAT_N];
        f.controller=bal_controller;f.capture=bal_startup.active?1U:(bal_recovery.active?2U:0U);
        f.distance_limit=BalanceRecovery_Distance(&bal_recovery,bal_startup.budget);f.fault=s_robot.fault;
        f.x_ref=bal_motion.x_ref;f.v_ref=bal_motion.v_ref;
        f.drive_epoch=cmd->drive_epoch;f.motion_flags=bal_motion.flags;f.drive_direction=bal_motion.requested;
        f.drive_sequence=cmd->drive_word>>2;f.drive_rx_ms=cmd->drive_rx_ms;
        f.psi_ref=bal_turn.psi_ref;f.w_ref=bal_turn.w_ref;f.turn_direction=bal_turn.requested;
        f.state=s_robot.state;f.ready=s_robot.ready;f.dropped=bal_drops;
        f.crc=TestCRC((uint8_t*)&f,sizeof(f)-2);
        uint8_t next=(bal_head+1)%BAL_QUEUE;
        if(next==bal_tail) {if(bal_drops<65535)bal_drops++;}
        else {bal_queue[bal_head]=f;__DMB();bal_head=next;}
    }
}

Robot_Data_t *Robot_GetData(void) { return &s_robot; }
