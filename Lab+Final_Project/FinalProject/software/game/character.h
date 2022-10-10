#ifndef CHARACTER_H_
#define CHARACTER_H_

#include "data.h"

void movement(
    int state_c[N_state_c],
    long long unsigned int event_c[N1_event_c][N2_event_c],
    int prohibit_c[N_prohibit_c],
    int cooling_c[N_cooling_c],
    char keycodes[N_keycodes],
    int clock,
    int system_flage[N_system_flage]
);

void update_event(
    int state_c[N_state_c],
    long long unsigned int event_c[N1_event_c][N2_event_c],
    int prohibit_c[N_prohibit_c],
    int cooling_c[N_cooling_c],
    char keycodes[N_keycodes],
    int clock,
    int system_flage[N_system_flage]
);

void change_event(
    int state_c[N_state_c],
    long long unsigned int event_c[N1_event_c][N2_event_c],
    int prohibit_c[N_prohibit_c],
    int cooling_c[N_cooling_c],
    char keycodes[N_keycodes],
    int clock,
    int system_flage[N_system_flage]
);



#endif