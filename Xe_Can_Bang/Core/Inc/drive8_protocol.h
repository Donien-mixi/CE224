#pragma once
#include <stdint.h>
#include <string.h>
/* Body excludes '*'. v and turn: -1,0,+1; left=positive body yaw.
 * MOVE8 has a distinct name so old integer/PWM steer cannot be misread. */
typedef struct {int direction,turn;uint32_t epoch,sequence,preset;} Drive8Packet;
static inline int Drive8_Uint(const char **p,uint32_t *value,int zero_ok){
 const char *s=*p;uint32_t n=0;
 if(*s<'0'||*s>'9')return 0;
 if(*s=='0'&&s[1]>='0'&&s[1]<='9')return 0;
 do{unsigned d=(unsigned)(*s-'0');if(n>(1000000000U-d)/10U)return 0;n=n*10U+d;s++;}while(*s>='0'&&*s<='9');
 if(!zero_ok&&!n)return 0;
 *p=s;*value=n;return 1;
}
static inline int Drive8_Trit(const char **p,int *value){
 const char *s=*p;
 if(*s=='-'&&s[1]=='1'){*value=-1;*p=s+2;return 1;}
 if(*s=='0'||*s=='1'){*value=*s-'0';*p=s+1;return 1;}return 0;
}
static inline int Drive8_Parse(const char *s,Drive8Packet *packet){
 Drive8Packet p;if(strncmp(s,"$MOVE8,",7))return 0;s+=7;
 if(!Drive8_Trit(&s,&p.direction)||*s++!=',')return 0;
 if(!Drive8_Trit(&s,&p.turn)||*s++!=',')return 0;
 if(!Drive8_Uint(&s,&p.epoch,0)||*s++!=',')return 0;
 if(!Drive8_Uint(&s,&p.sequence,0)||*s++!=',')return 0;
 if(!Drive8_Uint(&s,&p.preset,1)||*s||p.preset>1)return 0;
 *packet=p;return 1;
}
