#include "animation.h"



// Apply brightness to a color. Brightness is between 0 and 255
static Color apply_brightness(Color color, uint16_t brightness) {
    return (Color) {
            color.r * brightness / 255,
            color.g * brightness / 255,
            color.b * brightness / 255
    };
}

static Color apply_brightness_small(Color color, uint8_t brightness) {
    return (Color) {
            color.r * brightness / 255,
            color.g * brightness / 255,
            color.b * brightness / 255
    };
}

// Apply brightness to a color depending on the ratio of position to max
// Useful for turning a linear increase(1,2,3.../20) into a color pulse
static Color brightness_from_position(Color color, uint8_t position, uint8_t max) {
    return apply_brightness(color, ((max - position) % (max + 1)) * 255 / max);
}


static void reset_animation(State *state) {
    state->action = SOLID;
    state->color1 = COLOR_WHITE;
    state->color2 = COLOR_NONE;
    state->dir = D_NONE;
    state->interruptable = true;
}

//todo use this pulse when changing modes
static void setup_pulse(State *state) {
    state->action = PULSE;
    state->color1 = COLOR_WHITE;
    state->color2 = COLOR_NONE;
    state->dir= D_NONE;
    state->timer = 0;
    state->interruptable = true;
    state->timeout = 20;
    state->pulse_length = 20;
    state->echo = false;
}

State *init_animation(State *state) {
    reset_animation(state);
    state->brightness = 255;
    return state;
}

//Edits
static Color get_color(uint8_t val){
    switch(COLOR_INT){
        case(0):
            return apply_brightness_small(COLOR_RED, val);
        case(1):
            return apply_brightness_small(COLOR_BLUE, val);
        case(2):
            return apply_brightness_small(COLOR_GREEN, val);
        case(3):
            return apply_brightness_small(COLOR_YELLOW, val);
        case(4):
            return apply_brightness_small(COLOR_PURPLE, val);
        case(5):
            return apply_brightness_small(COLOR_PINK, val);
        case(6):
            return apply_brightness_small(COLOR_WHITE, val);
        case(7):
            return apply_brightness_small(COLOR_LIGHT_BLUE, val);
        default:
            return COLOR_NONE;
    }
}

Action get_action(State *state){
    if(state->brightness >= 0xff){ //highest level - time to breathe
        return BREATHE;
    } else { //not highest level
        return SOLID;
    }
}

void next_frame(State *state, Controller *controller) {
    // Update the animation state machine
    state->timer++;

    // Test if the current animation has timed out, if it is, then get back to breathing / solid
    if (state->timer >= state->timeout) {
        reset_animation(state);
        state->action = get_action(state); //on init this should return BREATHE
    }
/*
 * 0 - Off
 * 51 - Dim
 * 102 - Brighter-dim
 * 153 - Bright
 * 204 - Brighter
 * 255 - Brightest (time for breathing)
 */
    if (state->interruptable) { //we aren't pulsing

        if ((CONTROLLER_D_DOWN(*controller))) { //Is brightness being changed?
            //Edits - changed 32 to 51 for 5 settings

            if (state->brightness >= (255 - 51)) { //whoops! went over 255, time to go to zero
                state->brightness = 0;
            } else {
                state->brightness += 51;
            }
            setup_pulse(state);
            state->color1 = get_color(state->brightness);
            state->interruptable = false;
        }

        //change color settings - decrement
        if ((CONTROLLER_D_LEFT(*controller))) {
            if (COLOR_INT != 0) {
                COLOR_INT--;
            }
            setup_pulse(state);
            state->color1 = get_color(state->brightness);

        }

        //change color settings - increment
        if ((CONTROLLER_D_RIGHT(*controller))) {
            if (COLOR_INT < 7) {
                COLOR_INT++;
            }
            setup_pulse(state);
            state->color1 = get_color(state->brightness);
        }
    }


    if (state->action == BREATHE) {
        Color c;
        if (UP_BOOL) {
            c = get_color(state->timer);
        } else {
            c = get_color(255 - state->timer);
        }
        showColor(c);

        if (state->timer == 0xff) { //timer is at 255, time to start going down or up
            UP_BOOL = !UP_BOOL;
        }
    } else if (state->action == SOLID) {
        showColor(get_color(state->brightness));
    } else if (state->action == PULSE) {
        uint8_t position = state->timer;
        //uint8_t position = 0;

        Color colors[5];
        for (uint8_t i = 0; i < 5; i++) {
            if ((position >= (PULSE_DELAY * i)) && (position < state->pulse_length + (PULSE_DELAY * i))) {
                colors[i] = brightness_from_position(state->color1, position - (PULSE_DELAY * i), state->pulse_length);
                colors[i] = apply_brightness(colors[i], state->brightness);
            } else if ((state->echo) &&
                       (position >= (PULSE_DELAY * (i + 1))) &&
                       (position < state->pulse_length + (PULSE_DELAY * (i + 1)))) {
                colors[i] = brightness_from_position(state->color2, position - (PULSE_DELAY * (i + 1)),
                                                     state->pulse_length);
                colors[i] = apply_brightness(colors[i], state->brightness);
            } else {
                colors[i] = COLOR_NONE;
            }
        }
        showColor(colors[0]);
    }
}
