#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

/*
 * Reason: Less loc(prevents switch cases and ambiguous code)
 * Used to select the I/O port of timer N
 * NOTE: developer must ensure 0 < n < 2
 */
#define TIMER_N(n) (TIMER_0 + n)

/*
 * Reason: Less loc(prevents switch cases and ambiguous code)
 * Used to get the correct the selection bits
 * of timer N
 * NOTE: as TIMER_N it doesn't check if the timer
 * id is correct
 */
#define TIMER_SELN(n) (n << 6)

/*
 * Reason: Code reusability
 * Checks if the given timer is between 0 and 2
 * returns TIMER_OUT_RANGE if requirements not met
 */
#define CHECK_TIMER_RANGE(timer_id) \
    if(timer_id > 2) { \
        printf("(%s) Timer range must be between 0 and 2! %d given\n", __func__, timer_id); \
        return TIMER_OUT_RANGE; \
    }

/*
 * Reason: Code reusability
 * Performs sys_outb in a safe manner
 * prints a message in case of error and return TIMER_OUTB_FAILED
 */
#define SYS_OUTB_SAFE(res, message, arg1, arg2) \
    if((res = sys_outb(arg1, arg2)) != OK) { \
        printf("(%s) sys_outb failed (%s): %d\n", __func__, message, res); \
        return TIMER_OUTB_FAILED; \
    }

/*
 * Reason: Code reusability
 * Performs sys_inb in a safe manner
 * prints a message in case of error and return TIMER_INB_FAILED
 */
#define SYS_INB_SAFE(res, message, arg1, arg2) \
    if((res = sys_inb(arg1, arg2)) != OK) { \
        printf("(%s) sys_inb failed (%s): %d\n", __func__, message, res); \
        return TIMER_INB_FAILED; \
    }

#define UINT16_T_MAX (uint16_t)0xFFFF
#define TIMER_MAX_FREQ TIMER_FREQ
#define TIMER_MIN_FREQ (uint16_t)TIMER_FREQ/UINT16_T_MAX + (((uint16_t)TIMER_FREQ % UINT16_T_MAX) ? 1 : 0)

typedef enum _timer_status{
    TIMER_OK = OK,
    TIMER_OUT_RANGE,

    /*out of bounds error*/
    TIMER_FREQ_TOO_LOW,
    TIMER_FREQ_TOO_HIGH,

    /*sys_* failed*/
    TIMER_OUTB_FAILED,
    TIMER_INB_FAILED,

    /*util_* failed*/
    TIMER_UTIL_FAILED,

    /*arguments (timer not included) are not valid*/
    TIMER_INVALID_ARGS,

    /*lcf returned an error*/
    TIMER_LCF_ERROR,

    /*kernel call functions failed*/
    TIMER_INT_SUB_FAILED,
    TIMER_INT_UNSUB_FAILED

} timer_status;


int timerHookID = 31;
unsigned int timerIntCounter = 0;

int (timer_set_frequency)(uint8_t timer, uint32_t freq) {

    CHECK_TIMER_RANGE(timer);
    uint8_t res = 0;

    /*
     * In order to set the frequency to the value *freq* we must write to 
     * the LSB and MSB the divisor *div* that follows the following rule
     * TIMER_FREQ/divisisor = freq, this means that
     * divisor = TIMER_FREQ/freq
     */

    //When freq > TIMER_FREQ then TIMER_FREQ/freq = 0 because we're performing integer division, lets avoid that case
    if (freq > TIMER_MAX_FREQ){
        printf("(%s) freq is higher than TIMER_FREQ\n", __func__);
        return TIMER_FREQ_TOO_HIGH;
    }

    //LSB and MSB are 8bits each, so that means the divisor is 16 bit, the maximum value that an unsigned 16 bit int
    //can hold is 2^16 - 1 = 65535. TIMER_FREQ / 65535 = 18.2067902647. Since we're performing integer division
    //the minimum is obtained by rounding up, 19.
    if (freq < TIMER_MIN_FREQ){
        printf("(%s) freq cant be lower than 19", __func__);
        return TIMER_FREQ_TOO_LOW;
    }

    /*
     * Since we're changing the frequency, we must make sure the counting mode
     * and the counting base are preserved, thus we are preserving the last 4 bits
     * of the previous timer configuration
     */

    // Get previous timer configuration
    uint8_t previousConfig;
    if( (res = timer_get_conf(timer, &previousConfig)) != TIMER_OK)
        return res;

    //Build the control word, maintaining the 4 last bits
    uint8_t controlWord = 0;
    controlWord |= TIMER_SELN(timer) | TIMER_LSB_MSB | (previousConfig & (BIT(3) | BIT(2) | BIT(1) | BIT(0)));

    // Write control word to control register
    SYS_OUTB_SAFE(res, "writting control word", TIMER_CTRL, controlWord);

    // Calculate initial value to be loaded to get the given frequency
    uint16_t divisor = TIMER_FREQ / freq;
    uint8_t lsb = 0 , msb = 0;
    
    /*
     * Since the timer uses two 8 bit registers, we need to separate the 16 bit
     * value in two, thus the separation in LSB and MSB
     */

    if(util_get_LSB(divisor, &lsb) != OK){
        printf("(%s) util_get_LSB: failed reading lsb\n", __func__);
        return TIMER_UTIL_FAILED;
    }

    if(util_get_MSB(divisor, &msb) != OK){
        printf("(%s) util_get_MSB: failed reading msb\n", __func__);
        return TIMER_UTIL_FAILED;
    }

    uint8_t selectedTimer = TIMER_N(timer);
    
    // Write the lsb and msb to the respective timer registers
    SYS_OUTB_SAFE(res, "writting lsb", selectedTimer, lsb);
    SYS_OUTB_SAFE(res, "writting msb", selectedTimer, msb);

    return TIMER_OK;
}

int (timer_subscribe_int)(uint8_t *bit_no) {
    
    if(bit_no == NULL){
        printf("(%s) bit_no is NULL\n", __func__);
        return TIMER_INVALID_ARGS;
    }

    //Return, via the argument, the bit number that will be set in msg.m_notify.interrupts
    *bit_no = timerHookID;

    // Subscribe a notification on every interrupt in the specified timer's IRQ line
    if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &timerHookID) != TIMER_OK) {
        printf("(%s) sys_irqsetpolicy: failed subscribing timer interrupts\n", __func__);
        return TIMER_INT_SUB_FAILED;
    }

    // Since we use the IRQ_REENABLE policy, we do not have to use sys_irqenable
    return TIMER_OK;
}

int (timer_unsubscribe_int)() {
    
    // Since we use the IRQ_REENABLE policy, we do not have to use sys_irqdisable

    // Unsubscribe the interrupt notifications associated with hook id
    if (sys_irqrmpolicy(&timerHookID) != TIMER_OK) {
        printf("(%s) sys_irqrmpolicy: failed unsubscribing timer interrupts\n", __func__);
        return TIMER_INT_UNSUB_FAILED;
    }

    return TIMER_OK;
}

/*
 * Although the current implementation works with uint8_t which max value is 255
 * timerIntCounter can easily hold 60 times that. The problem arises when bigger
 * values are used, they can cause a overflow, thus by limiting the counter to 0-59
 * we guarantee it works for any period of time
 */
void (timer_int_handler)() {
    //Increment the timer interruption counter global variable
    timerIntCounter = (timerIntCounter+1) % 60;
}

int (timer_get_conf)(uint8_t timer, uint8_t *st) {
    
    CHECK_TIMER_RANGE(timer);

    //Used to hold the result of any operation
    uint8_t res = 0;

    if(st == NULL){
        printf("(%s) st is NULL\n", __func__);
        return TIMER_INVALID_ARGS;
    }

    uint8_t readbackC = 0;
    //Define the control word has a read back command
    //Set the appropriate timer and set not to read the count
    readbackC |= (TIMER_RB_CMD | TIMER_RB_SEL(timer) | TIMER_RB_COUNT_);

    // Write Read-Back command to Control Register
    SYS_OUTB_SAFE(res, "writting readback", TIMER_CTRL, readbackC);

    // Read configuration from given timer and write it to the location specified
    uint32_t status = 0;
    SYS_INB_SAFE(res, "reading status", TIMER_N(timer), &status);
    
    // Return the read configuration via the argument 
    *st = (uint8_t) status;

    return TIMER_OK;
}

int (timer_display_conf)(uint8_t timer, uint8_t st,
                        enum timer_status_field field) {
    	
    CHECK_TIMER_RANGE(timer);

    //Used to hold the result of any operation
    uint8_t res = 0;

    union timer_status_field_val timerStatus;

    switch(field){
        case all:
            timerStatus.byte = st;
            break;
        case initial:
            //Gets the register selection bits and converts them to timer_init by shifting them to the least
            //significant bits
            timerStatus.in_mode = (st & TIMER_LSB_MSB) >> 4;
            break;
        case mode:
            timerStatus.count_mode = (st & (BIT(3) | BIT(2) | BIT(1))) >> 1;
            break;
        case base:
            timerStatus.bcd = (st & BIT(0));	
            break;
    }

    if((res = timer_print_config(timer, field, timerStatus)) != TIMER_OK){
        printf("(%s) timer_print_config returned: %d\n", __func__, res);
        return TIMER_LCF_ERROR;
    }

    return TIMER_OK;
}
