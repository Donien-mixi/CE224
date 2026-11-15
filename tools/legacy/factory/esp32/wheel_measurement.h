#pragma once
#include <stdint.h>
#include <stdbool.h>
/* Raw diagnostic PWM, not balance deadband compensation. Protocol v1. */
static inline bool WheelMeasurementValid(int kind,uint32_t duration,int pwm,int tag,int ref) {
    if(tag<0 || tag>2 || ref!=0)return false;
    return (kind==12 && duration==18000 && pwm==600) ||
           (kind==13 && duration==36000 && (pwm==400 || pwm==-400)) ||
           (kind==14 && duration==12900 && pwm==300);
}
static inline int WheelMeasurementPWM(int kind,uint32_t age,int sign) {
    if(kind==12)return age%6000<2000?600:(age%6000>=3000 && age%6000<5000?-600:0);
    if(kind==13) {
        const int levels[8]={120,160,180,200,220,260,320,400};
        return age%1500<1000?levels[(age/1500)%8]*(sign<0?-1:1):0;
    }
    return age%4300<1500?300:(age%4300>=1800 && age%4300<3300?-300:0);
}
