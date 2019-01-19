#ifndef STATE_MACH_H
#define STATE_MACH_H
#include <lcom/lcf.h>

/* Minimum slope acceptable for drawing the inverted V */
#define MIN_SLOPE 1

/**
 * @brief Initializes the state machine
 *
 * This function should be called once per packet byte
 * 
 * @param x_dis_exp minimum valuable for the x displacement
 * @param tolerance acceptable displacement along both x and y
 */
void state_machine_init(int32_t x_dis_exp, uint32_t tolerance);

/**
 * @brief Handles a given packet and updates the state machine accordingly
 *
 * @param pp Address of memory that contains the packet to handle
 */
void state_machine_handle_packet(struct packet *pp);

/**
 * @brief Verifies if state machine in is end state
 *
 * @return Return true if state machine is in end state, false otherwise
 */
bool state_machine_ended();

/*
 * Enumeration of possible states.
 * Undefined states are used for testing purposes only.
 * If a state given any input goes to an undefined state, then there's an obvious problem.
 */
typedef enum{
    INITIAL=0, DRAWING_UP, V_VERTEX, DRAWING_DOWN, FINAL,
    UNDEFINED
} State;

/*
 * Enumeration of possible state transitions.
 * Since with wrong inputs they always go to the initial state we can define INVALID as any invalid input.
 */
typedef enum{
    L_CLICK=0, R_CLICK, L_UP, R_UP,
    NO_TRANS, INVALID 
} Transition;

/* Struct for the State Machine */
typedef struct{
    State state;
    int32_t x_dis, x_dis_exp;
    uint32_t tolerance;
    bool initRequirements;
    struct packet packet;
} StateMachine;

#endif
