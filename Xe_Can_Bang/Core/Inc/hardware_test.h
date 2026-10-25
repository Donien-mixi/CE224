#ifndef HARDWARE_TEST_H
#define HARDWARE_TEST_H
#include <stdint.h>
#include <stdbool.h>
/* Set to 0 only after finishing hardware identification. No auto-arm in test mode. */
#define HARDWARE_TEST_ON_BOOT 0 /* Default: supervised balance trial; $DIAG starts retained test suite. */
/* Wire format v1: little endian, CRC16-CCITT (initial FFFF) over all but crc.
 * Encoder totals are BEFORE the firmware's right-wheel sign inversion.
 * PWM is the command applied BEFORE this sample. dt_us is actual loop spacing.
 * IMU remains configured at 100 Hz; reading at 200 Hz does not create new IMU data. */
typedef struct __attribute__((packed)) {
    uint16_t sync;
    uint32_t sequence, t_ms;
    uint16_t dt_us;
    int16_t gx, gy, gz, ax, ay, az;
    int32_t ticks_l, ticks_r;
    int16_t pwm_l, pwm_r;
    uint8_t stage, flags;
    uint16_t dropped;
    uint16_t crc;
} HardwareTestFrame;
_Static_assert(sizeof(HardwareTestFrame) == 42, "test wire layout");
bool HardwareTest_IsLocked(void);
void HardwareTest_Service(void); /* main loop only: UART DMA, no printf in IRQ */
/* Web measurement jobs stay locked out of balancing; commands copied to IRQ mailbox. */
bool HardwareTest_Command(const char *packet);
#endif
