#include "state_machine.h"

/* Transitions for each state */
static State transToState[][5] = {
    /*INITIAL*/
    { DRAWING_UP, UNDEFINED, UNDEFINED, UNDEFINED },
    /*/DRAWING_UP*/
    { UNDEFINED, UNDEFINED, V_VERTEX, UNDEFINED },
    /*V_VERTEX*/
    { DRAWING_UP, DRAWING_DOWN, UNDEFINED, UNDEFINED },
    /*DRAWING_DOWN*/
    { UNDEFINED, UNDEFINED, UNDEFINED, FINAL },
    /*FINAL*/
    { UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED },

};

/* State handlers */
static Transition initial_handler();
static Transition drawing_up_handler();
static Transition v_vertex_handler();
static Transition drawing_down_handler();
static Transition final_handler();

/* Array of pointers to handlers */
static Transition (*stateHandlers[])(void) = { initial_handler, drawing_up_handler, v_vertex_handler, drawing_down_handler, final_handler, NULL, NULL};

/* Instantiate the state machine */
static StateMachine sm = { INITIAL, 0, -1, 0, true,
	/*struct packet*/ { {0,0,0} , false, false, false, 0, 0, false, false } };

void state_machine_init(int32_t x_dis_exp, uint32_t tolerance){
    sm.state = INITIAL;
    sm.x_dis_exp = x_dis_exp;
    sm.tolerance = tolerance;
    sm.initRequirements = true;
    memset(&sm.packet, 0, sizeof(struct packet));
}

/* No button should be pressed when initiating state machine */
static void inline updateInitRequirements() { 
    sm.initRequirements = (!sm.packet.lb && !sm.packet.mb && !sm.packet.rb);
}

static void performStateTransition(Transition trans){

    /* No transition */
    if(trans == NO_TRANS)
        return;

    /* Invalid transition */
    else if(trans == INVALID){
        sm.state = INITIAL;
        updateInitRequirements();
		return;
	}

    /* Test transition to new state and ensure it is defined */
    State newState = transToState[sm.state][trans];
    if(newState == UNDEFINED){
        printf("Transition to undefined state!\n");
        printf("State:%d Trans:%d\n", sm.state, trans);
        return;
    }

    /* Update current state */
    sm.state = newState;
}

void state_machine_handle_packet(struct packet *pp){

    /* Check null pointer */
    if(pp == NULL)
        return;

    /* Copy pp to the state machine packet */
    memcpy(&sm.packet, pp, sizeof(struct packet));

    /* Check if state machine was initialized */
    if(sm.x_dis_exp < 0){
        puts("Please init the state machine before performing any operation");
        return;
    }

    /* Get current handler and verify its validity */
    Transition (*funcHandler)() = stateHandlers[sm.state];
    if(funcHandler == NULL){
        puts("The machine is in an invalid state and can't handle more packets");
        return;
    }
    
    /* Determine next transition */
    Transition trans = funcHandler();

    /* Perform the transition */
    performStateTransition(trans);

}

/* Verify it there was an overflow */
static bool check_ovf(){ return (sm.packet.x_ov || sm.packet.y_ov); }

Transition initial_handler(){

    /* Initial requirements are not met, thus not doing anything */
    if(sm.initRequirements == false)
        return INVALID;

    /* Mouse buttons state alteration */
    if( !(!sm.packet.lb && !sm.packet.mb && !sm.packet.rb)){

        /* LB Pressed and others not */
        if(sm.packet.lb && !sm.packet.mb && !sm.packet.rb)
            return L_CLICK;

        /* Force update the init requirements */
        return INVALID;
    }

    return NO_TRANS;
}

/* Reset state machine X distance */
static Transition resetXDisp(){
	sm.x_dis = 0;
	return INVALID;
}

Transition drawing_up_handler(){
    
    if(check_ovf())
        return resetXDisp();

    /* Change in mouse buttons state */
    if( !(sm.packet.lb && !sm.packet.mb && !sm.packet.rb) ){

        /* L button is up */
        if (!sm.packet.lb && !sm.packet.mb && !sm.packet.rb){
            /* If X distance is above specified minimum */
            if(sm.x_dis >= sm.x_dis_exp){
                sm.x_dis = 0;
                return L_UP;
            }
        }
        return resetXDisp();
    }
	
    /* Negative x input and over the tolerance */
    if(sm.packet.delta_x < 0 && (uint32_t)abs(sm.packet.delta_x) > sm.tolerance)
    	return resetXDisp();
    
    sm.packet.delta_x = abs(sm.packet.delta_x);
    
    /* Negative y input and over the tolerance */
    if(sm.packet.delta_y < 0 && (uint32_t)abs(sm.packet.delta_y) > sm.tolerance)
    	return resetXDisp();
    
    sm.packet.delta_y = abs(sm.packet.delta_y);

    /* Check there was movement */
    if(sm.packet.delta_x == 0)
        return NO_TRANS;

    /* Check slope */
    if ((sm.packet.delta_y/sm.packet.delta_x) <= MIN_SLOPE)
        return resetXDisp();
	
    /* Update current X distance */
    sm.x_dis += sm.packet.delta_x;
    return  NO_TRANS;
}

Transition v_vertex_handler(){

    if(check_ovf())
        return INVALID;

    /* Avoid mouse movement above tolerance */
    if((uint32_t)abs(sm.packet.delta_x) > sm.tolerance || (uint32_t)abs(sm.packet.delta_y) > sm.tolerance)
        return INVALID;

	/* Check for change in mouse buttons */
    if( !(!sm.packet.lb && !sm.packet.mb && !sm.packet.rb) ){

        /* Rb was pressed */
        if(!sm.packet.lb && !sm.packet.mb && sm.packet.rb)
            return R_CLICK;

        /* Lb was pressed, go back to drawing */
        if(sm.packet.lb && !sm.packet.mb && !sm.packet.rb)
            return L_CLICK;

        /* Another button was pressed */
        return INVALID;
    }

    return NO_TRANS;
}

Transition drawing_down_handler(){

    /* Change in mouse button state */
    if( !(!sm.packet.lb && !sm.packet.mb && sm.packet.rb) ){
	/* R button is up */
        if (!sm.packet.lb && !sm.packet.mb && !sm.packet.rb){
            if(sm.x_dis >= sm.x_dis_exp){
                sm.x_dis = 0;
                return R_UP;
            }
        }
        return resetXDisp();
    }
	
    /* Only consider movement after mouse buttons are processed */
    if(check_ovf())
        return resetXDisp();

    /* Negative x input and over the tolerance */
    if(sm.packet.delta_x < 0 && (uint32_t)abs(sm.packet.delta_x) > sm.tolerance)
    	return resetXDisp();
    
    sm.packet.delta_x = abs(sm.packet.delta_x);
    
    /* Positive y input and over the tolerance */
    if(sm.packet.delta_y > 0 && (uint32_t)abs(sm.packet.delta_y) > sm.tolerance)
    	return resetXDisp();
    
    sm.packet.delta_y = abs(sm.packet.delta_y);

    /* Check there was movement */
    if(sm.packet.delta_x == 0)
        return NO_TRANS;

    /* Check slope */
    if ((sm.packet.delta_y/sm.packet.delta_x) <= MIN_SLOPE)
    	return resetXDisp();
    
    sm.x_dis += sm.packet.delta_x;
    return  NO_TRANS;
}

Transition final_handler(){ return NO_TRANS; }

bool state_machine_ended() { return sm.state == FINAL; }
