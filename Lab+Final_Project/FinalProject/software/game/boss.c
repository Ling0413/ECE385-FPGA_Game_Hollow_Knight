/*
    Functions for boss.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "boss.h"
#include "helper.h"


// Manage the movement of character
void movement_b(
    int state_b[N_state_b],
    long long unsigned int event_b[N1_event_b][N2_event_b],
    int prohibit_b[N_prohibit_b],
    int clock,
    int system_flage[N_system_flage]
)
{
    // Update Vx
    // if (prohibit_b[Vx] == 0) {
    //     state_b[Vx] = 0;
    // }

    // Update Vy
    if (state_b[Vy] <= v_fall && prohibit_b[Vy] == 0) {
        state_b[Vy] += g;
    }
    if (state_b[Vy] >= v_fall && prohibit_b[Vy] == 0) {
        state_b[Vy] = v_fall;
    }

    // Correct Cx, Cy
    if (state_b[Ori] == 0) {
        state_b[Cx] = state_b[Cenx] - state_b[CenOffx];
        state_b[Cy] = state_b[Ceny] - state_b[CenOffy];
    } else {
        state_b[Cx] = state_b[Cenx] - state_b[Offx] + state_b[CenOffx];
        state_b[Cy] = state_b[Ceny] - state_b[CenOffy];
    }
            
    // Update x coordinate
    state_b[Cx] += state_b[Vx];
    if (state_b[Cx] <= Bound_left) {
        state_b[Cx] = Bound_left;
        if (state_b[Vx] < 0) {
        	state_b[Vx] = 0;
        }
    }
    if (state_b[Cx] + state_b[Offx] >= Bound_right) {
        state_b[Cx] = Bound_right - state_b[Offx] - 1;
        if (state_b[Vx] > 0) {
        	state_b[Vx] = 0;
        }
    }

    // Update y coordinate
    state_b[Cy] += state_b[Vy];
    if (state_b[Cy] <= Bound_up) {
//    	printf("%d\n", state_b[Cy]);
//    	printf("1");
        state_b[Cy] = Bound_down - state_b[Offy];
//    	state_b[Cy] = Bound_up;
        state_b[Vy] = 0;
    }
    if (state_b[Cy] + state_b[Offy] >= Bound_down) {
        state_b[Cy] = Bound_down - state_b[Offy];
        state_b[Vy] = 0;
    }

    // Update center coordinate
    if (state_b[Ori] == 0) {
        state_b[Cenx] = state_b[Cx] + state_b[CenOffx];
        state_b[Ceny] = state_b[Cy] + state_b[CenOffy];
    } else {
        state_b[Cenx] = state_b[Cx] + state_b[Offx] - state_b[CenOffx];
        state_b[Ceny] = state_b[Cy] + state_b[CenOffy];
    }

}


/* Update current event list, do following things:
 * 1. If frame counter is larger than number of frames, free prohibition list and reset event list, return
 * 2. If counter of current frame is zero, increment the frame counter by 1, update state accordingly, return
 * 3. Subtract the counter of current frame by 1
 */
void update_event_b(
    int state_b[N_state_b],
    int state_c[N_state_c],
    long long unsigned int event_b[N1_event_b][N2_event_b],
    int prohibit_b[N_prohibit_b],
    int clock,
    int system_flage[N_system_flage],
    int cooling_c[N_cooling_c]
)
{
    int index = event_b[0][fcount];

    // If counter of current frame is zero
    if (event_b[index][0] == 0){
        // If frame counter is larger than or equal to number of frames
        if (index >= event_b[0][fnum]) {
            // Special end
            if (event_b[0][id] == 11) {     // Spark
                cooling_c[c_stiff] = 5;
                free_prohibition(prohibit_b, event_b[0][pvector]);
            } else if (event_b[0][id] == 8) {
                cooling_c[c_stiff] = 60;
                free_prohibition(prohibit_b, event_b[0][pvector]);
            } else if (event_b[0][id] == 9) {
                cooling_c[c_stiff] = 140;
                system_flage[f_spell] = 1;
                free_prohibition(prohibit_b, event_b[0][pvector]);
            } else {
                free_prohibition(prohibit_b, event_b[0][pvector]);
            }
            return;
        } else {
            // Special case
            if (event_b[0][id] == 11) {     // Spark
                if (event_b[0][fcount] == 4) {
                    // Set the orientation and position
                    if (system_flage[f_spark_ori] == 0) {
                        state_b[Ori] = 1;
                        state_b[Cx] = 100;
                        state_b[Cenx] = state_b[Cx] + event_b[index+1][2] - event_b[index+1][4];
                        state_b[Ceny] = state_b[Cy] + event_b[index+1][5];
                    } else {
                        state_b[Ori] = 0;
                        state_b[Cx] = 350;
                        state_b[Cenx] = state_b[Cx] + event_b[index+1][4];
                        state_b[Ceny] = state_b[Cy] + event_b[index+1][5];
                    }
                }
            } else if (event_b[0][id] == 8) {   // Dash
                // Transform
                if (event_b[0][fcount] == 2 || event_b[0][fcount] == 5) {
                    if (state_b[Ori] == 0) {
                        state_b[Cenx] = state_b[Cx] + event_b[index+1][4];
                        state_b[Ceny] = state_b[Cy] + event_b[index+1][5];
                    } else {
                        state_b[Cenx] = state_b[Cx] + event_b[index+1][2] - event_b[index+1][4];
                        state_b[Ceny] = state_b[Cy] + event_b[index+1][5];
                    }
                } else if (event_b[0][fcount] == 3) {
                    if (state_b[Ori] == 0) {
                        state_b[Vx] = -30;
                        prohibit_b[Vy] = 1;
                        state_b[Ceny] += 10;
                    } else {
                        state_b[Vx] = 30;
                        prohibit_b[Vy] = 1;
                        state_b[Ceny] += 10;
                    }
                } else if (event_b[0][fcount] == 4) {
                    state_b[Vx] = 0;
                    prohibit_b[Vy] = 0;
                    state_b[Ceny] -= 10;
                } else {}
            } else if (event_b[0][id] == 9) {   // Spine
                if (event_b[0][fcount] == 4) {      // Spark to the position above character
                    state_b[Ori] = 0;
                    state_b[Cx] = state_c[Cx];
                    state_b[Cy] = 10;
                    state_b[Cenx] = state_b[Cx] + event_b[index+1][4];
                    state_b[Ceny] = state_b[Cy] + event_b[index+1][5];
                } else if (event_b[0][fcount] == 8) {
                    state_b[Vy] = 30;
                } else if (event_b[0][fcount] == 9) {
                    state_b[Cenx] = state_b[Cx] + event_b[index+1][4];
                    state_b[Ceny] = state_b[Cy] + event_b[index+1][5] + 20;
                } else {}
            } else {}
            
            // Current frame ended
            event_b[0][fcount] += 1;
            state_b[Offx] = event_b[index+1][2];
            state_b[Offy] = event_b[index+1][3];
            state_b[CenOffx] = event_b[index+1][4];
            state_b[CenOffy] = event_b[index+1][5];
        }
    } 

    // Subtract the counter of current frame by 1
    if ((index != event_b[0][condition_1]) && (index != event_b[0][condition_2])){
        event_b[index][0] -= 1;
    } else {
        if (state_b[Vy] == 0) {
            event_b[index][0] -= 1;
        }
    }
}


// Change the current event
void change_event_b(
    int state_b[N_state_b],
    int state_c[N_state_c],
    long long unsigned int event_b[N1_event_b][N2_event_b],
    int prohibit_b[N_prohibit_b],
    int clock,
    int system_flage[N_system_flage],
    int cooling_c[N_cooling_c]
)
{
    int spark_enable = 0;
    int left = state_b[Cx] - 80;
    int right = state_b[Cx] + state_b[Offx] + 80;

	// Determine whether a spark is needed
	if ((state_c[Cenx] >= left && state_c[Cenx] <= right)) {
		if (state_b[SparkF] == 0) {
			spark_enable = 1;
			if (state_c[Cenx] >= 320) {
				system_flage[f_spark_ori] = 0;
			} else {
				system_flage[f_spark_ori] = 1;
			}
		}
	}

    // Spark
    if ((cooling_c[c_stiff] == 0) && (spark_enable == 1) && (state_b[skill] <= 3) && (prohibit_b[p_spark] == 0)) {
        // Set flag
        state_b[SparkF] = 1;
        // Update event
        clean_event(event_b);
        free_prohibition(prohibit_b, 0xffff);
        set_event(event_b, event_b_spark);
        set_prohibition(prohibit_b, event_b[0][pvector]);
        // Update other state values
        state_b[Offx] = event_b[1][2];
        state_b[Offy] = event_b[1][3];
        state_b[CenOffx] = event_b[1][4];
        state_b[CenOffy] = event_b[1][5];
    }

    // Dash
    if ((cooling_c[c_stiff] == 0) && (state_b[skill] <=3) && (prohibit_b[p_dash] == 0)) {
        // Set flag
        state_b[SparkF] = 0;
        if ((rand()%20) < 9) {
			state_b[skill] = 4;
		} else {
			state_b[skill] += 1;
		}
        // Update event
        clean_event(event_b);
        free_prohibition(prohibit_b, 0xffff);
        set_event(event_b, event_b_dash);
        set_prohibition(prohibit_b, event_b[0][pvector]);
        // Update other state values
        state_b[Offx] = event_b[1][2];
        state_b[Offy] = event_b[1][3];
        state_b[CenOffx] = event_b[1][4];
        state_b[CenOffy] = event_b[1][5];
    }

    // Spine
    if ((cooling_c[c_stiff] == 0) && (state_b[skill] == 4) && (prohibit_b[p_spine] == 0)) {
        // Set flag
        state_b[skill] = 1;
        // Update event
        clean_event(event_b);
        free_prohibition(prohibit_b, 0xffff);
        set_event(event_b, event_b_spine);
        set_prohibition(prohibit_b, event_b[0][pvector]);
        // Update other state values
        state_b[Offx] = event_b[1][2];
        state_b[Offy] = event_b[1][3];
        state_b[CenOffx] = event_b[1][4];
        state_b[CenOffy] = event_b[1][5];
    }
    
    // Defualt: stand
    if (prohibit_b[p_stand] == 0) {
        clean_event(event_b);
        free_prohibition(prohibit_b, 0xffff);
        set_event(event_b, event_b_stand);
        set_prohibition(prohibit_b, event_b[0][pvector]);
        // Set the orientation to character's direction
        if (state_c[Cx] < state_b[Cx]) {
            state_b[Ori] = 0;
        } else {
            state_b[Ori] = 1;
        }
        // Update other state values
        state_b[Offx] = event_b[1][2];
        state_b[Offy] = event_b[1][3];
        state_b[CenOffx] = event_b[1][4];
        state_b[CenOffy] = event_b[1][5];
    }
}
