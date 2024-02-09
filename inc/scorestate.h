#ifndef SCORESTATE_H_INCLUDED
#define SCORESTATE_H_INCLUDED
#include <types.h>

void scorestate_init(void);

void scorestate_update(fix32 dt);

void scorestate_joyevent(u16 joy, u16 changed, u16 state);

void scorestate_stop(void);

#endif