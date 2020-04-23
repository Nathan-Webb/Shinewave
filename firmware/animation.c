#include "animation.h"



// Apply brightness to a color. Brightness is between 0 and 255
static Color apply_brightness(Color color, uint16_t brightness) {
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

static Direction get_analog_direction(Controller *controller) {
    if(ANALOG_UP(*controller)) {
        return D_UP;
    } else if(ANALOG_DOWN(*controller)) {
        return D_DOWN;
    } else if(ANALOG_LEFT(*controller)) {
        return D_LEFT;
    } else if(ANALOG_RIGHT(*controller)) {
        return D_RIGHT;
    }
    return D_NONE;
}

static Direction get_c_direction(Controller *controller) {
    if(C_UP(*controller)) {
        return D_UP;
    } else if(C_DOWN(*controller)) {
        return D_DOWN;
    } else if(C_LEFT(*controller)) {
        return D_LEFT;
    } else if(C_RIGHT(*controller)) {
        return D_RIGHT;
    }
    return D_NONE;
}

static void reset_animation(State *state) {
    state->action = IDLE;
    state->color1 = COLOR_WHITE;
    state->color2 = COLOR_NONE;
    state->dir = D_NONE;
    state->interruptable = true;
    state->idle_counter = 0;
}

//edits - hopefully we dont go out of bounds with this one
static void increment_character() {
    if(char_index < requested_characters){
        char_index++;
    } else {
        char_index = 0;
    }
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

void next_frame(State *state, Controller *controller) {
    // Update the animation state machine
    state->timer++;

    // Test if the current animation has timed out
    if(state->timer >= state->timeout && state->action != IDLE) {
        reset_animation(state);
        state->action = BLANK;
    }

    // Test if the controller is idle
    if(state->action == BLANK && state->timer >= 0xff) {
        state->action = IDLE;
        state->timer = 0;
    }

    Direction analog_direction = get_analog_direction(controller);

    // If we're in Side-B, make sure B is released before repeating
    if((!state->interruptable) && (state->action == SIDEB) && (!CONTROLLER_B(*controller))) {
        state->interruptable = true;
    } // If the state can be interrupted, check all of the possible outcomes
    else if(state->interruptable) {
        Direction c_direction = get_c_direction(controller);
        // Test if brightness is being changed
        if((CONTROLLER_D_DOWN(*controller))) {
            //Edits
            if(state->brightness >= (255 - 32)) {
                state->brightness = 0;
            } else {
                state->brightness += 32;
            }
            setup_pulse(state);
            state->interruptable = false;
        }

        //change color settings - decrement
        if((CONTROLLER_D_LEFT(*controller))) {

        }

        //change breathing settings - increment
        if((CONTROLLER_D_RIGHT(*controller))) {

        }
    }


    if(state->action == BREATHE) {
        if(state->timer == 0){ //timer is at 0, we need to go up
            UP_BOOL = true;
        } else if(state->timer == 0xff){ //timer is at 255, time to start going down
            UP_BOOL = false;
        }
    }
    // Push the animations to the LEDs
    if(state->action == IDLE) { //start the idle color fade
        //Edits for LED breathing here
        switch(state->idle_counter) {
            case(0):
                showColor(apply_brightness((Color) {255, state->timer, 0}, state->brightness));
                break;
            case(1):
                showColor(apply_brightness((Color) {255 - state->timer, 255, 0}, state->brightness));
                break;
            case(2):
                showColor(apply_brightness((Color) {0, 255, state->timer}, state->brightness));
                break;
            case(3):
                showColor(apply_brightness((Color) {0, 255 - state->timer, 255}, state->brightness));
                break;
            case(4):
                showColor(apply_brightness((Color) {state->timer, 0, 255}, state->brightness));
                break;
            case(5):
                showColor(apply_brightness((Color) {255, 0, 255 - state->timer}, state->brightness));
                break;
            default:
                state->timer = 0;
                state->idle_counter = 0;
                break;
        }
        if(state->timer == 0xff) { //255 - sets the glow color to something different
            state->idle_counter = state->idle_counter + 1;
        }
    } else if(state->action == BLANK) {
        showColor(COLOR_NONE);
    }
}
