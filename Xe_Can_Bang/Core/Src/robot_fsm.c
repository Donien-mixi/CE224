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

/* IRQ produces fixed-size samples; main sends them using UART DMA. */
#include "wheel_measurement.h"
#define TEST_QUEUE_SIZE 64
static HardwareTestFrame test_queue[TEST_QUEUE_SIZE];
static volatile uint8_t test_head, test_tail;
static uint32_t test_start_ms, test_stage_ms, test_last_cycle, test_seq;
static uint32_t test_last_meta_ms;
static uint16_t test_dropped, test_deadband[4];
static uint8_t test_stage, test_flags, test_config[6], test_config_ok;
static float test_bias[3], test_pitch_reference;
static uint8_t test_locked;
static uint16_t test_visited;
static uint8_t test_abort_from = 255;
static struct {
    uint32_t t_ms;
    float delta, gx, gy, gz, norm;
    int16_t pwm_l, pwm_r;
    uint8_t stage, reason;
} test_stop;
static uint32_t test_last_stop_ms;
static int32_t test_window_ticks;
static uint32_t test_window_ms;
static uint32_t test_angle_since;
static uint8_t test_angle_active;
static int32_t test_step_left, test_step_right;
/* Manual jobs: 1 static,2 known pose,3 slow tilt,4 hand encoder,5/6 motor steps,
 * 7 vibration comparison,8 brake pulse,9 loaded fixture pulse,10 fine deadband,
 * 11 coast pulse (compare with kind8 to separate passive drag from braking).
 * No calibration constants are changed by collecting a job. */
typedef struct { uint32_t token, duration; int pwm, reference, tag, kind; } TestRequest;
static TestRequest manual_request, manual_job;
static volatile uint8_t manual_pending;
static volatile uint32_t manual_heartbeat;
static volatile uint8_t manual_phase; /* 0 legacy boot,1 recording,2 done,3 aborted */
static volatile uint8_t manual_announced;
static uint8_t manual_trial, manual_published_phase;
static uint32_t manual_last_meta;
static uint32_t manual_leg_ms, manual_tick_ms;
static int32_t manual_ticks;
static int manual_level;
static uint16_t manual_db[6];

bool HardwareTest_Command(const char *packet) {
    if (!strncmp(packet,"$TPING",6)) {
        if (test_locked) manual_heartbeat=HAL_GetTick();
        return true;
    }
    if (strncmp(packet,"$TEST,",6)) return false;
    if (!test_locked || test_stage<11 || test_stage==13 || manual_pending || !test_config_ok ||
        test_config[1]!=0xd1 || test_config[2]!=0x28 || test_config[3]!=8 ||
        test_config[4]!=0x28 || test_config[5]!=0) return true;
    long values[6]; const char *p=packet+6; char *end;
    for (unsigned i=0;i<6;i++) {
        values[i]=strtol(p,&end,10);
        if (end==p || (i<5 && *end!=',')) return true;
        p=end+ (i<5 ? 1 : 0);
    }
    if (*p || values[0]<=0 || values[0]>2000000000L || values[1]<1 || values[1]>14 ||
        values[2]<100 || values[2]>60000 || values[3]<-1800 || values[3]>1800 ||
        values[4]<-180000 || values[4]>180000 || values[5]<0 || values[5]>8) return true;
    int kind=(int)values[1];
    if (((kind==8 || kind==11) && (values[2]>700 || values[5]>2)) ||
        (kind==9 && (values[2]>200 || values[3]<-800 || values[3]>800 || values[5]>2)) ||
        (kind==10 && values[5]>1)) return true;
    if(kind>=12 && !WheelMeasurementValid(kind,(uint32_t)values[2],(int)values[3],(int)values[5],(int)values[4]))return true;
    if(ESP32_Comm_GetCommand()->emergency_stop)return true;
    manual_request=(TestRequest){(uint32_t)values[0],(uint32_t)values[2],
        (int)values[3],(int)values[4],(int)values[5],kind};
    manual_heartbeat=HAL_GetTick();__DMB();manual_pending=1;
    return true;
}

static Robot_Data_t s_robot;
static bool bal_config_ok;
static void TestSetStage(uint8_t stage,uint32_t now);
static float TestPitchDelta(float pitch,float reference);
static void ManualFinish(bool aborted,uint32_t now) {
    manual_phase=aborted?3:2;manual_announced=0;
    TestSetStage(aborted?12:11,now);
}

static void ManualRun(uint32_t now,int32_t left,int32_t right,const BMI160_Data_t *imu) {
    uint32_t elapsed=now-test_start_ms;
    int kind=manual_job.kind, l=0,r=0;
    if (kind>=5 && now-manual_heartbeat>1500) {
        test_flags|=4;ManualFinish(true,now);return;
    }
    if (elapsed<5000) {
        test_pitch_reference=s_robot.pitch; /* each job may use a different fixture orientation */
        Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;return;
    } /* setup/settling is logged separately */
    uint32_t age=elapsed-5000;
    if (kind<=2 && (fabsf(imu->gx)>3 || fabsf(imu->gy)>3 || fabsf(imu->gz)>3 ||
        ((kind==1 || manual_job.tag==0) && fabsf(TestPitchDelta(s_robot.pitch,test_pitch_reference))>3) ||
        imu->ax*imu->ax+imu->ay*imu->ay+imu->az*imu->az<.72f ||
        imu->ax*imu->ax+imu->ay*imu->ay+imu->az*imu->az>1.32f)) test_flags|=128;
    if (kind>=5 && (fabsf(TestPitchDelta(s_robot.pitch,test_pitch_reference))>25 ||
        fabsf(imu->gx)>120 || fabsf(imu->gy)>120 || fabsf(imu->gz)>120)) {
        test_stop.t_ms=elapsed;test_stop.stage=13;
        test_stop.delta=TestPitchDelta(s_robot.pitch,test_pitch_reference);
        test_stop.gx=imu->gx;test_stop.gy=imu->gy;test_stop.gz=imu->gz;
        test_stop.norm=sqrtf(imu->ax*imu->ax+imu->ay*imu->ay+imu->az*imu->az);
        test_stop.pwm_l=s_robot.pwm_left;test_stop.pwm_r=s_robot.pwm_right;
        test_stop.reason=fabsf(test_stop.delta)>25?1:2;
        test_flags|=32;ManualFinish(true,now);return;
    }
    if (kind<=4) {
        if (age>=manual_job.duration) {ManualFinish(false,now);return;}
        if (kind==4) {Motor_Coast();s_robot.pwm_left=s_robot.pwm_right=0;return;}
    } else if (kind==5 || kind==6) {
        /* 3 repetitions, each: four positive and four negative levels.
         * 1000ms excitation +1000ms braking, never a direct reversal. */
        unsigned segment=age/2000;
        if (segment>=24) {ManualFinish(false,now);return;}
        const int levels[4]={600,1000,1400,1800};
        int pwm=age%2000<1000 ? levels[segment%4]*(segment%8<4?1:-1):0;
        int previous=kind==5?s_robot.pwm_left:s_robot.pwm_right;
        if(pwm && !previous) {test_step_left=left;test_step_right=right;}
        if(!pwm && previous && abs(kind==5?left-test_step_left:right-test_step_right)<4) {
            test_flags|=16;ManualFinish(true,now);return;
        }
        if(kind==5)l=pwm;else r=pwm;
    } else if (kind==7) {
        unsigned segment=age/2000;
        if(segment>=18) {ManualFinish(false,now);return;}
        int pwm=age%2000<1000 ? abs(manual_job.pwm)*(segment%6<3?1:-1):0;
        if(segment%3!=1)l=pwm;
        if(segment%3!=0)r=pwm;
    } else if (kind==8 || kind==9 || kind==11) {
        if(age>=manual_job.duration+(kind==11?3000:2000)) {ManualFinish(false,now);return;}
        int pwm=age<manual_job.duration?manual_job.pwm:0;
        if(manual_job.tag!=1)l=pwm;
        if(manual_job.tag!=0)r=pwm;
    } else if (kind==10) {
        int32_t ticks=manual_job.tag?right:left;
        if(manual_trial>=6) {ManualFinish(false,now);return;}
        uint32_t leg_age=now-manual_leg_ms;
        if(leg_age<500) {manual_ticks=ticks;manual_tick_ms=now;manual_level=20;}
        else {
            if(now-manual_tick_ms>=200) {
                if(abs(ticks-manual_ticks)>=4) {
                    manual_db[manual_trial++]=(uint16_t)manual_level;
                    manual_leg_ms=now;Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;return;
                }
                manual_ticks=ticks;manual_tick_ms=now;manual_level+=20;
                if(manual_level>600) {test_flags|=16;ManualFinish(true,now);return;}
            }
            int pwm=manual_level*(manual_trial%2?-1:1);
            if(manual_job.tag)r=pwm;else l=pwm;
        }
    }
    if(kind>=12) {
        if(age>=manual_job.duration) {ManualFinish(false,now);return;}
        int pwm=WheelMeasurementPWM(kind,age,manual_job.pwm);
        if(manual_job.tag!=1)l=pwm;
        if(manual_job.tag!=0)r=pwm;
    }
    if((kind>=7 && kind<=9) || kind==11 || kind==12) {
        if((l && !s_robot.pwm_left) || (r && !s_robot.pwm_right)) {test_step_left=left;test_step_right=right;}
        if((!l && s_robot.pwm_left && abs(left-test_step_left)<4) ||
           (!r && s_robot.pwm_right && abs(right-test_step_right)<4)) {
            test_flags|=16;ManualFinish(true,now);return;
        }
    }
    if(kind==11 && age>=manual_job.duration) {
        Motor_Coast();s_robot.pwm_left=s_robot.pwm_right=0;return;
    }
    if(elapsed>70000) {test_flags|=2;ManualFinish(true,now);return;}
    Motor_SetDuty(l,r);s_robot.pwm_left=l;s_robot.pwm_right=r;
}

static uint16_t TestCRC(const uint8_t *data, unsigned length) {
    uint16_t crc = 0xffff;
    while (length--) {
        crc ^= (uint16_t)*data++ << 8;
        for (unsigned i = 0; i < 8; i++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}
bool HardwareTest_IsLocked(void) { return test_locked != 0; }
static float TestPitchDelta(float pitch, float reference) {
    float delta = fmodf(pitch - reference, 360.0f);
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    return delta;
}

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

static void TestSetStage(uint8_t stage, uint32_t now) {
    if (stage == 12 && test_stage == 12) return;
    if (stage == 12) test_abort_from = test_visited ? test_stage : 255;
    test_visited |= (uint16_t)(1U << stage);
    test_stage = stage; test_stage_ms = now;
    test_window_ms = now; test_window_ticks = 0;
    test_angle_active = 0;
    Motor_Stop(); s_robot.pwm_left = s_robot.pwm_right = 0;
    if (stage == 11) {
        s_robot.state = ROBOT_STATE_TEST_DONE;
        LED_SetPattern(LED_PATTERN_SLOW_BLINK); Buzzer_BeepAsync(300);
    } else if (stage == 12) {
        s_robot.state = ROBOT_STATE_TEST_ABORTED;
        LED_SetPattern(LED_PATTERN_ALARM); Buzzer_BeepAsync(700);
    }
}

static void TestBegin(bool imu_ok) {
    manual_phase=manual_pending=0;memset(&manual_job,0,sizeof(manual_job));
    test_head = test_tail = 0;
    test_seq = test_last_meta_ms = 0;
    test_flags = test_dropped = test_visited = 0;
    test_stage = 255;
    test_abort_from = 255;
    memset(&test_stop, 0, sizeof(test_stop)); test_last_stop_ms = 0;
    memset(test_deadband, 0, sizeof(test_deadband));
    test_locked = 1;
    test_start_ms = HAL_GetTick();
    test_config_ok = imu_ok && BMI160_GetConfig(test_config);
    BMI160_GetBias(test_bias);
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* HAL I2C timeout uses HAL_GetTick: SysTick must preempt the sampling IRQ. */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    test_last_cycle = DWT->CYCCNT;
    s_robot.state = ROBOT_STATE_DIAGNOSTIC;
    /* Refuse motor excitation when actual registers do not match raw scaling. */
    if (!test_config_ok || test_config[1] != 0xd1 ||
        test_config[2] != 0x28 || test_config[3] != 0x08 ||
        test_config[4] != 0x28 || test_config[5] != 0x00) {
        test_flags |= 0x40;
        TestSetStage(12, test_start_ms);
    } else TestSetStage(0, test_start_ms);
    Encoder_Reset();
}

static void TestSample(void) {
    uint32_t cycle = DWT->CYCCNT;
    uint32_t spacing = (cycle - test_last_cycle) / (SystemCoreClock / 1000000U);
    test_last_cycle = cycle;
    uint32_t now = HAL_GetTick();
    ESP32_Command_t *cmd = ESP32_Comm_GetCommand();
    if(cmd->emergency_stop)manual_pending=0; /* STOP cancels a queued request before promotion. */
    if(manual_pending && test_stage>=11 && test_stage!=13 && test_tail==test_head) {
        __DMB();manual_job=manual_request;manual_pending=0;
        manual_phase=1;manual_announced=0;manual_trial=0;manual_leg_ms=now+5000;
        memset(manual_db,0,sizeof(manual_db));memset(&test_stop,0,sizeof(test_stop));test_last_stop_ms=0;
        test_seq=0;test_dropped=test_flags=0;test_start_ms=now;test_last_meta_ms=0;
        test_pitch_reference=s_robot.pitch;Encoder_Reset();TestSetStage(13,now);
        s_robot.state=ROBOT_STATE_DIAGNOSTIC;
    }
    if (cmd->emergency_stop && (test_stage < 11 || test_stage==13)) {
        test_flags |= 4; cmd->emergency_stop = 0;
        if(manual_phase==1)ManualFinish(true,now);else TestSetStage(12, now);
    }
    cmd->emergency_stop = 0; /* Preserve the original terminal cause. */
    cmd->bench_test = 0; cmd->trigger_calib = 0;
    s_robot.v_target = s_robot.steer_cmd = 0;
    /* A sealed manual job has a fixed attempted-sample count. A new request
     * above can reopen capture, but idle ticks must not inflate its end marker. */
    if(manual_phase>=2) {Motor_Stop();return;}
    if (spacing > 7500 && test_seq > 1 && (test_stage < 11 || test_stage==13)) {
        test_flags |= 2;
        if(manual_phase==1)ManualFinish(true,now);else TestSetStage(12, now);
    }
    BMI160_Data_t imu = {0};
    bool valid = BMI160_Read_All(&imu);
    if (!valid) {
        test_flags |= 8;
        if(manual_phase==1)ManualFinish(true,now);else TestSetStage(12, now);
    }
    if(manual_phase>=2) {Motor_Stop();return;}
    Encoder_Update(spacing > 0 ? spacing * 0.000001f : 0.005f);
    s_robot.v_left = Encoder_GetLeftVelocity();
    s_robot.v_right = Encoder_GetRightVelocity();
    s_robot.v_actual = Encoder_GetAverageVelocity();
    if (valid) {
        s_robot.pitch = atan2f(imu.ax, imu.az) * RAD_TO_DEG; /* measured accel pitch, no forced gyro sign */
        s_robot.gyro_rate = imu.gy;
    }
    int32_t left, right;
    Encoder_GetHardwareTicks(&left, &right);
    HardwareTestFrame frame = {0};
    frame.sync = 0x5aa5; frame.sequence = test_seq++;
    frame.t_ms = now - test_start_ms;
    frame.dt_us = spacing > 65535 ? 65535 : spacing;
    BMI160_RawData_t raw;
    BMI160_GetLastRaw(&raw);
    frame.gx = raw.gx_raw; frame.gy = raw.gy_raw; frame.gz = raw.gz_raw;
    frame.ax = raw.ax_raw; frame.ay = raw.ay_raw; frame.az = raw.az_raw;
    frame.ticks_l = left; frame.ticks_r = right;
    frame.pwm_l = s_robot.pwm_left; frame.pwm_r = s_robot.pwm_right;
    frame.stage = test_stage; frame.flags = test_flags | (valid ? 1 : 0);
    frame.dropped = test_dropped;
    /* Capture prior command/interval before changing the motor input. */
    /* Keep raw IMU/encoder capture for 30 s after completion, PWM stays zero.
     * This permits slow forward/back tilt verification before any PID trial. */
    if (test_stage < 11 || manual_phase==1 || (!manual_phase && now - test_stage_ms < 30000)) {
        uint8_t next = (test_head + 1) % TEST_QUEUE_SIZE;
        if (next == test_tail) { if (test_dropped != 65535) test_dropped++; }
        else {
            frame.crc = TestCRC((const uint8_t *)&frame, sizeof(frame) - 2);
            test_queue[test_head] = frame; __DMB(); test_head = next;
        }
    }
    if(manual_phase==1 && test_stage==13) {ManualRun(now,left,right,&imu);return;}
    if (test_stage >= 11) { Motor_Stop(); return; }
    float norm2 = imu.ax*imu.ax + imu.ay*imu.ay + imu.az*imu.az;
    if ((test_stage == 1 || test_stage == 10) && (norm2 < 0.72f || norm2 > 1.32f ||
        fabsf(imu.gx) > 3 || fabsf(imu.gy) > 3 || fabsf(imu.gz) > 3 ||
        fabsf(TestPitchDelta(s_robot.pitch, test_pitch_reference)) > 3)) test_flags |= 128;
    float pitch_delta = TestPitchDelta(s_robot.pitch, test_pitch_reference);
    /* Accel includes motor vibration/braking acceleration: one 100 Hz sample
     * cannot establish chassis tilt. Require 250 ms continuously beyond 8 deg.
     * Large tilt and fast rotation still brake immediately. */
    if (fabsf(pitch_delta) > 8) {
        if (!test_angle_active) { test_angle_active = 1; test_angle_since = now; }
    } else test_angle_active = 0;
    uint8_t motion_reason = ((fabsf(pitch_delta) > 25 ||
        (test_angle_active && now - test_angle_since >= 250)) ? 1 : 0) |
        ((fabsf(imu.gx) > 120 || fabsf(imu.gy) > 120 || fabsf(imu.gz) > 120) ? 2 : 0);
    if (test_stage >= 3 && test_stage <= 9 && motion_reason) {
        /* Preserve the triggering sample BEFORE braking; subsequent quiet samples
         * do not explain why the test stopped. Accel pitch is not ground truth. */
        test_stop.t_ms = now - test_start_ms; test_stop.stage = test_stage;
        test_stop.delta = pitch_delta;
        test_stop.gx = imu.gx; test_stop.gy = imu.gy; test_stop.gz = imu.gz;
        test_stop.norm = sqrtf(norm2);
        test_stop.pwm_l = s_robot.pwm_left; test_stop.pwm_r = s_robot.pwm_right;
        test_stop.reason = motion_reason;
        test_flags |= 32; TestSetStage(12, now); return;
    }
    uint32_t age = now - test_stage_ms;
    int16_t pwm_l = 0, pwm_r = 0;
    if (test_stage == 0 && age >= 10000) {
        test_pitch_reference = s_robot.pitch; TestSetStage(1, now);
    } else if (test_stage == 1 && age >= 5000) {
        if (test_flags & 128) TestSetStage(12, now);
        else { TestSetStage(2, now); LED_SetPattern(LED_PATTERN_FAST_BLINK); Buzzer_BeepAsync(250); }
    } else if (test_stage == 2 && age >= 2000) TestSetStage(3, now);
    else if (test_stage >= 3 && test_stage <= 6) {
        unsigned wheel = (test_stage - 3) / 2;
        int sign = (test_stage & 1) ? 1 : -1;
        int32_t ticks = wheel ? right : left;
        if (age < 500) { test_window_ticks = ticks; test_window_ms = now; }
        else {
            int magnitude = (int)((age - 500) / 100 + 1) * 60;
            if (now - test_window_ms >= 100) {
                int32_t delta = ticks - test_window_ticks;
                if (delta >= 4 || delta <= -4) {
                    /* Previous plateau PWM, not the next ramp command. Resolution 60 counts. */
                    test_deadband[test_stage - 3] = wheel ? (uint16_t)abs(s_robot.pwm_right) : (uint16_t)abs(s_robot.pwm_left);
                    TestSetStage(test_stage + 1, now); return;
                }
                test_window_ticks = ticks; test_window_ms = now;
            }
            /* Evaluate the completed 1800 plateau BEFORE declaring no response. */
            if (magnitude > 1800) {
                test_flags |= 16; TestSetStage(12, now); return;
            }
            if (wheel) pwm_r = sign * magnitude; else pwm_l = sign * magnitude;
        }
    } else if (test_stage == 7 || test_stage == 8) {
        unsigned window = age / 1200;
        if (window >= 8) { TestSetStage(test_stage + 1, now); return; }
        const int16_t levels[4] = {600, 1000, 1400, 1800};
        int16_t pwm = (age % 1200 < 700) ? levels[window % 4] * (window < 4 ? 1 : -1) : 0;
        int16_t previous = test_stage == 7 ? s_robot.pwm_left : s_robot.pwm_right;
        if (pwm && !previous) { test_step_left = left; test_step_right = right; }
        if (!pwm && previous && abs((test_stage == 7 ? left - test_step_left : right - test_step_right)) < 4) {
            test_flags |= 16; TestSetStage(12, now); return;
        }
        if (test_stage == 7) pwm_l = pwm; else pwm_r = pwm;
    } else if (test_stage == 9) {
        if (age >= 4000) { TestSetStage(10, now); return; }
        pwm_l = pwm_r = (age % 2000 < 700) ? (age < 2000 ? 1000 : -1000) : 0;
        if (pwm_l && !s_robot.pwm_left) { test_step_left = left; test_step_right = right; }
        if (!pwm_l && s_robot.pwm_left &&
            (abs(left - test_step_left) < 4 || abs(right - test_step_right) < 4)) {
            test_flags |= 16; TestSetStage(12, now); return;
        }
    } else if (test_stage == 10 && age >= 3000) {
        TestSetStage((test_flags & 128) ? 12 : 11, now); return;
    }
    Motor_SetDuty(pwm_l, pwm_r);
    s_robot.pwm_left = pwm_l; s_robot.pwm_right = pwm_r;
}

void HardwareTest_Service(void) {
    if (!test_locked) return;
    /* Metadata is retained on ESP32; repeat permits recovery after a gateway restart. */
    uint32_t now = HAL_GetTick();
    /* Snapshot the IRQ-owned phase ONCE. If the IRQ finishes during this call,
     * a running marker is harmless; a final marker must wait for the next call
     * to observe both the terminal phase and its fully published sample queue. */
    uint8_t phase=manual_phase;__DMB();
    /* Drain final samples BEFORE publishing completion; gateway can freeze the log. */
    if(phase>=2 && test_tail!=test_head) {
        if(ESP32_Comm_SendBytes(&test_queue[test_tail],sizeof(HardwareTestFrame))) {
            __DMB();test_tail=(test_tail+1)%TEST_QUEUE_SIZE;
        }
        return;
    }
    if(phase>=2 && test_stop.reason && !test_last_stop_ms) {
        char stop[192];
        int len=snprintf(stop,sizeof(stop),"$TSTOP,%u,%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d,%u\r\n",
            test_stop.stage,(unsigned long)test_stop.t_ms,test_stop.delta,
            test_stop.gx,test_stop.gy,test_stop.gz,test_stop.norm,
            test_stop.pwm_l,test_stop.pwm_r,test_stop.reason);
        if(len>0 && len<(int)sizeof(stop) && ESP32_Comm_SendBytes(stop,len))test_last_stop_ms=now;
        return;
    }
    if(phase && (!manual_announced || phase!=manual_published_phase || now-manual_last_meta>=1000)) {
        char job[192];
        int len=snprintf(job,sizeof(job),"$TJ,1,%lu,%d,%u,%lu,%d,%d,%lu,%u,%d,%u,%u,%u,%u,%u,%u\r\n",
            (unsigned long)manual_job.token,manual_job.kind,phase,
            (unsigned long)manual_job.duration,manual_job.pwm,manual_job.reference,
            (unsigned long)test_seq,test_flags,manual_job.tag,
            manual_db[0],manual_db[1],manual_db[2],manual_db[3],manual_db[4],manual_db[5]);
        if(len>0 && len<(int)sizeof(job) && ESP32_Comm_SendBytes(job,len)) {
            manual_announced=1;manual_published_phase=phase;manual_last_meta=now;
        }
        return;
    }
    if (test_stop.reason && (!test_last_stop_ms || now - test_last_stop_ms >= 1000)) {
        char stop[192];
        int len = snprintf(stop, sizeof(stop), "$TSTOP,%u,%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d,%u\r\n",
            test_stop.stage, (unsigned long)test_stop.t_ms, test_stop.delta,
            test_stop.gx, test_stop.gy, test_stop.gz, test_stop.norm,
            test_stop.pwm_l, test_stop.pwm_r, test_stop.reason);
        if (len > 0 && len < (int)sizeof(stop) && ESP32_Comm_SendBytes(stop, len)) test_last_stop_ms = now;
        else return;
    }
    if (!test_last_meta_ms || now - test_last_meta_ms >= 1000) {
        char meta[256];
        int len = snprintf(meta, sizeof(meta),
            "$TINFO,3,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.6f,%.6f,%.6f,%u,%u,%u,%u,%u,%u,%u\r\n",
            (unsigned long)test_start_ms, test_stage, test_flags, test_config_ok,
            test_config[0], test_config[1], test_config[2], test_config[3], test_config[4], test_config[5],
            test_bias[0], test_bias[1], test_bias[2], test_deadband[0], test_deadband[1],
            test_deadband[2], test_deadband[3], test_dropped, test_visited, test_abort_from);
        if (len > 0 && len < (int)sizeof(meta) && ESP32_Comm_SendBytes(meta, len)) test_last_meta_ms = now;
        else return;
    }
    if(manual_phase==1 && !manual_announced)return; /* start marker must precede frame0 */
    if (test_tail != test_head && ESP32_Comm_SendBytes(&test_queue[test_tail], sizeof(HardwareTestFrame))) {
        __DMB(); test_tail = (test_tail + 1) % TEST_QUEUE_SIZE;
    }
}

void Robot_Init(void) {
  /* 1. Khởi tạo còi, LED, cơ cấu chấp hành */
  BuzzerLED_Init();
  Motor_Init();
  Motor_Stop();
  Encoder_Init();
  PID_Init();

  /* Preserve the existing startup motor exercise: PWM 1600, 600 ms per wheel.
   * This is an output command, not proof of supply voltage or motor health. */
  if (HARDWARE_TEST_ON_BOOT) Motor_SelfTest();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  /* HAL I2C timeout must be able to advance inside TIM4 IRQ. */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
  HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
  ESP32_Comm_Init();

  /* 2. Khởi tạo cảm biến Bosch BMI160 */
  if (!BMI160_Init()) {
    if (HARDWARE_TEST_ON_BOOT) { TestBegin(false); return; }
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
    if (HARDWARE_TEST_ON_BOOT) { TestBegin(false); return; }
    s_robot.state = ROBOT_STATE_EMERGENCY; return;
  }
  uint8_t cfg[6];
  bal_config_ok=BMI160_GetConfig(cfg)&&cfg[1]==0xd1&&cfg[2]==0x28&&cfg[3]==8&&cfg[4]==0x28&&cfg[5]==0;
  if(!bal_config_ok)s_robot.fault|=256;
  float init_pitch = Filter_CalibratedRawPitch(init_imu.ax, init_imu.az);
  Filter_Init(init_pitch);

  /* 5. Chuyển sang trạng thái chờ dựng xe */
  s_robot.state = ROBOT_STATE_STANDBY;
  LED_SetPattern(LED_PATTERN_SLOW_BLINK);
  Buzzer_BeepAsync(200);

  /* Discard commands accumulated during the blocking startup calibration. */
  ESP32_Comm_FlushRx();

  /* Replace the old blind deadband ramp with measured, bounded diagnostics. */
  if (HARDWARE_TEST_ON_BOOT) TestBegin(true);
}

void Robot_RecalibrateIMU(void) {
  if (HardwareTest_IsLocked() || s_robot.state==ROBOT_STATE_BALANCING || s_robot.state==ROBOT_STATE_BENCH_TEST) return;
  /* Tạm dừng vòng điều khiển 200Hz để tránh truy cập I2C chồng chéo trong lúc
   * hiệu chuẩn */
  HAL_TIM_Base_Stop_IT(&htim4);

  Motor_Stop();
  s_robot.pwm_left = 0;
  s_robot.pwm_right = 0;

  s_robot.state = ROBOT_STATE_CALIBRATING;
  LED_SetPattern(LED_PATTERN_FAST_BLINK);
  Buzzer_BeepAsync(100);

  /* 500 read attempts at the configured 100 Hz (~5 seconds). */
  BMI160_Calibrate_Gyro(500);

  /* Khởi tạo lại góc ban đầu cho bộ lọc bù */
  BMI160_Data_t imu;
  if (!BMI160_Read_All(&imu)) {
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
static uint32_t bal_seq,bal_last_cycle,bal_meta_ms;
static uint16_t bal_drops,bal_still;
static uint32_t pulse_started;
static uint8_t pulse_wheel;
static int8_t pulse_dir;
static float pulse_reference;
static float bal_catalog[CAT_N+3];
static void BalanceStop(uint16_t reason) {
    s_robot.fault|=reason; s_robot.state=ROBOT_STATE_FALLEN;pulse_wheel=0;
    Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;
    s_robot.v_control=s_robot.theta_control=s_robot.pwm_base=0;
    PID_Reset_Integral();cat_reset(bal_catalog,0);bal_still=0;
}
void Robot_BalanceService(void) {
    if(test_locked)return;
    if(bal_tail!=bal_head) {
        if(ESP32_Comm_SendBytes(&bal_queue[bal_tail],sizeof(BalanceFrame))) {
            __DMB();bal_tail=(bal_tail+1)%BAL_QUEUE;
        }
        return;
    }
    if(HAL_GetTick()-bal_meta_ms>=1000) {
        float bias[3];BMI160_GetBias(bias);
        char meta[256];
        int n=snprintf(meta,sizeof(meta),"$BINFO,%s,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.9f,1\r\n",
            BALANCE_BUILD,CAT_K[0],CAT_K[1],CAT_K[2],CAT_K[3],CAT_PARAM[11],
            BALANCE_PPR,BALANCE_DB_L_POS,BALANCE_DB_L_NEG,BALANCE_DB_R_POS,BALANCE_DB_R_NEG,
            bias[0],bias[1],bias[2],BALANCE_RAW_ZERO_DEG);
        if(n>0&&n<(int)sizeof(meta)&&ESP32_Comm_SendBytes(meta,n))bal_meta_ms=HAL_GetTick();
    }
}
void Robot_ControlLoop_200Hz(void) {
    if (HardwareTest_IsLocked()) { TestSample(); return; }
    uint32_t begin=DWT->CYCCNT;
    uint32_t spacing=bal_last_cycle?(uint32_t)(begin-bal_last_cycle)/(SystemCoreClock/1000000U):5000;
    bal_last_cycle=begin;s_robot.loop_count++;
    ESP32_Command_t *cmd=ESP32_Comm_GetCommand();
    if(cmd->diag_request) {
        uint8_t diagnostic_mode=cmd->diag_request;
        cmd->diag_request=0;cmd->arm_request=cmd->pulse_request=0;pulse_wheel=0;Motor_Stop();
        TestBegin(bal_config_ok);
        if(diagnostic_mode==2 && test_config_ok && test_stage!=12)TestSetStage(11,HAL_GetTick());
        return;
    }
    BMI160_Data_t imu={0};bool valid=BMI160_Read_All(&imu);
    if(!valid) {BalanceStop(32);s_robot.state=ROBOT_STATE_EMERGENCY;}
    else {
        s_robot.pitch=Filter_Complementary_Update(imu.gy,imu.ax,imu.az,0.005f);
        s_robot.gyro_rate=PITCH_GYRO_SIGN*imu.gy;
        Encoder_Update(0.005f);
        s_robot.v_left=Encoder_GetLeftVelocity();s_robot.v_right=Encoder_GetRightVelocity();
        s_robot.v_actual=Encoder_GetAverageVelocity();
        s_robot.x_actual+=s_robot.v_actual*0.005f;
    }
    /* Verify register setup outside IRQ at initialization/recalibration instead. */
    s_robot.v_target=s_robot.steer_cmd=0;
    if(cmd->emergency_stop) {cmd->emergency_stop=0;cmd->arm_request=cmd->pulse_request=0;BalanceStop(1);}
    if(!BMI160_CalibrationOK()) {s_robot.fault|=128;bal_still=0;}
    float norm=imu.ax*imu.ax+imu.ay*imu.ay+imu.az*imu.az;
    bool finite=isfinite(s_robot.pitch)&&isfinite(s_robot.gyro_rate)&&
        isfinite(s_robot.v_actual)&&isfinite(s_robot.x_actual);
    if(!finite)BalanceStop(512);
    if(valid&&finite&&BMI160_CalibrationOK()&&bal_config_ok&&
       s_robot.state!=ROBOT_STATE_EMERGENCY && fabsf(s_robot.pitch)<=BALANCE_ARM_ANGLE&&
       fabsf(imu.gx)<BALANCE_ARM_GYRO && fabsf(imu.gy)<BALANCE_ARM_GYRO && fabsf(imu.gz)<BALANCE_ARM_GYRO &&
       fabsf(s_robot.v_left)<BALANCE_ARM_WHEEL_SPEED && fabsf(s_robot.v_right)<BALANCE_ARM_WHEEL_SPEED &&
       norm>.72f&&norm<1.32f && spacing>=4000&&spacing<=6000) {
        if(bal_still<BALANCE_ARM_STILL_CYCLES)bal_still++;
    } else bal_still=0;
    s_robot.ready=bal_still>=BALANCE_ARM_STILL_CYCLES;
    bool just_armed=false;
    if(cmd->pulse_request) {
        uint8_t wheel=cmd->pulse_request;cmd->pulse_request=0;
        if(valid && finite && bal_config_ok && s_robot.state!=ROBOT_STATE_EMERGENCY &&
           s_robot.state!=ROBOT_STATE_BALANCING && s_robot.state!=ROBOT_STATE_BENCH_TEST) {
            pulse_wheel=wheel;pulse_dir=cmd->pulse_direction;pulse_started=HAL_GetTick();
            pulse_reference=s_robot.pitch;s_robot.state=ROBOT_STATE_BENCH_TEST;
            s_robot.fault=BMI160_CalibrationOK()?0:128;PID_Reset_Integral();
        }
    }
    if(cmd->arm_request) {
        cmd->arm_request=0;
        if(s_robot.ready && s_robot.state!=ROBOT_STATE_BALANCING && s_robot.state!=ROBOT_STATE_BENCH_TEST) {
            PID_Reset_Integral();Encoder_Reset();s_robot.x_actual=s_robot.x_target=0;
            cat_reset(bal_catalog,s_robot.pitch*DEG_TO_RAD);
            s_robot.v_actual=s_robot.v_left=s_robot.v_right=0;
            s_robot.fault=0;s_robot.state=ROBOT_STATE_BALANCING;just_armed=true;
        }
    }
    if(s_robot.state==ROBOT_STATE_BALANCING) {
        if(HAL_GetTick()-cmd->last_cmd_time>BALANCE_HEARTBEAT_MS)BalanceStop(2);
        if(fabsf(s_robot.pitch)>BALANCE_STOP_ANGLE)BalanceStop(4);
        if(fabsf(s_robot.x_actual)>BALANCE_STOP_DISTANCE)BalanceStop(8);
        if(fabsf(s_robot.v_actual)>BALANCE_STOP_SPEED)BalanceStop(16);
        if(spacing<4000||spacing>6000)BalanceStop(64);
    }
    if(s_robot.state==ROBOT_STATE_BENCH_TEST) {
        if(HAL_GetTick()-cmd->last_cmd_time>BALANCE_HEARTBEAT_MS)BalanceStop(2);
        else if(spacing<4000||spacing>6000)BalanceStop(64);
        else if(fabsf(TestPitchDelta(s_robot.pitch,pulse_reference))>25 || fabsf(imu.gx)>120 || fabsf(imu.gy)>120 || fabsf(imu.gz)>120)BalanceStop(1024);
        else if(HAL_GetTick()-pulse_started>=1500) {
            Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;pulse_wheel=0;
            s_robot.state=ROBOT_STATE_STANDBY;bal_still=0;
        } else {
            s_robot.pwm_left=(pulse_wheel==1||pulse_wheel==3)?600*pulse_dir:0;
            s_robot.pwm_right=(pulse_wheel==2||pulse_wheel==3)?600*pulse_dir:0;
            s_robot.v_control=s_robot.theta_control=s_robot.pwm_base=0;
            Motor_SetDuty(s_robot.pwm_left,s_robot.pwm_right);
        }
    } else if(s_robot.state==ROBOT_STATE_BALANCING) {
        /* Frozen R12 float32 law; same header as Scilab native experiment.
         * Raw ticks are L forward-positive, R forward-negative. No extra DB. */
        int32_t ticks_l,ticks_r;Encoder_GetHardwareTicks(&ticks_l,&ticks_r);
        float input[7]={s_robot.pitch*DEG_TO_RAD,s_robot.gyro_rate*DEG_TO_RAD,
            PITCH_ACC_SIGN*Filter_CalibratedRawPitch(imu.ax,imu.az)*DEG_TO_RAD,
            .5f*((float)ticks_l-(float)ticks_r)*METERS_PER_TICK,
            s_robot.v_left,s_robot.v_right,
            (-(float)ticks_r-(float)ticks_l)*METERS_PER_TICK};
        float output[4];
        if(!cat_step(input,bal_catalog,output))BalanceStop(512);
        else if(just_armed) {
            /* Observer updates once; first ARM tick still holds brake. */
            bal_catalog[CAT_N]=bal_catalog[CAT_N+2]=0;
            Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;
            s_robot.pwm_base=0;
            s_robot.v_control=bal_catalog[1];s_robot.theta_control=bal_catalog[2]*RAD_TO_DEG;
        } else {
            s_robot.pwm_left=(int16_t)output[0];s_robot.pwm_right=(int16_t)output[1];
            s_robot.pwm_base=.5f*(output[0]+output[1]);
            s_robot.v_control=bal_catalog[1];s_robot.theta_control=bal_catalog[2]*RAD_TO_DEG;
            Motor_SetDuty(s_robot.pwm_left,s_robot.pwm_right);
        }
    } else {
        Motor_Stop();s_robot.pwm_left=s_robot.pwm_right=0;
        s_robot.v_control=s_robot.theta_control=s_robot.pwm_base=0;
    }
    uint32_t execution=(uint32_t)(DWT->CYCCNT-begin)/(SystemCoreClock/1000000U);
    if(execution>4000 && (s_robot.state==ROBOT_STATE_BALANCING||s_robot.state==ROBOT_STATE_BENCH_TEST))BalanceStop(64);
    if(s_robot.loop_count%2==0) {
        BalanceFrame f={0};BMI160_RawData_t raw;BMI160_GetLastRaw(&raw);
        f.sync=0x5ba5;f.sequence=bal_seq++;f.t_ms=HAL_GetTick();
        f.dt_us=spacing>65535?65535:spacing;f.execution_us=execution>65535?65535:execution;
        f.gx=raw.gx_raw;f.gy=raw.gy_raw;f.gz=raw.gz_raw;
        f.ax=raw.ax_raw;f.ay=raw.ay_raw;f.az=raw.az_raw;
        int32_t raw_l,raw_r;Encoder_GetHardwareTicks(&raw_l,&raw_r);
        f.ticks_l=raw_l;f.ticks_r=raw_r;
        f.pwm_l=s_robot.pwm_left;f.pwm_r=s_robot.pwm_right;
        f.pitch=s_robot.pitch;f.gyro=s_robot.gyro_rate;
        f.v_l=s_robot.v_left;f.v_r=s_robot.v_right;f.x=s_robot.x_actual;
        f.v_cmd=s_robot.v_control;f.theta_cmd=s_robot.theta_control;f.pwm_base=s_robot.pwm_base;
        f.integral=bal_catalog[CAT_N]; /* R12 yaw integral Nm, NOT velocity integral deg. */f.fault=s_robot.fault;
        f.state=s_robot.state;f.ready=s_robot.ready;f.dropped=bal_drops;
        f.crc=TestCRC((uint8_t*)&f,sizeof(f)-2);
        uint8_t next=(bal_head+1)%BAL_QUEUE;
        if(next==bal_tail) {if(bal_drops<65535)bal_drops++;}
        else {bal_queue[bal_head]=f;__DMB();bal_head=next;}
    }
}

Robot_Data_t *Robot_GetData(void) { return &s_robot; }
