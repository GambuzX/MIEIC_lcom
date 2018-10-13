#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"
#include "timer_user.h"

// Hook id to be used when subscribing a timer interrupt
int timerHookID = 1;

// Counts the number of interrupts
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
    
    // Check if freq is within acceptable limits
    if (freq > TIMER_MAX_FREQ){
        printf("(%s) freq is higher than TIMER_FREQ\n", __func__);
        return TIMER_FREQ_TOO_HIGH;
    }

    if (freq < TIMER_MIN_FREQ){
        printf("(%s) freq cant be lower than %d", __func__, TIMER_MIN_FREQ);
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

    // Build the control word, maintaining the 4 last bits
    uint8_t controlWord = TIMER_SELN(timer) | TIMER_LSB_MSB | (previousConfig & (BIT(3) | BIT(2) | BIT(1) | BIT(0)));

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
    
    // Check null pointer
    if(bit_no == NULL){
        printf("(%s) bit_no is NULL\n", __func__);
        return TIMER_INVALID_ARGS;
    }

    // Return, via the argument, the bit number that will be set in msg.m_notify.interrupts
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
 * Although the current implementation works with uint8_t, whose max value is 255,
 * timerIntCounter can easily hold 60 times that. The problem arises when bigger
 * values are used, which can cause an overflow. Thus, by limiting the counter to 0-59,
 * we guarantee it works for any period of time
 */
void (timer_int_handler)() {
    timerIntCounter = (timerIntCounter+1) % 60;
}

int (timer_get_conf)(uint8_t timer, uint8_t *st) {
    
    CHECK_TIMER_RANGE(timer);

    // Used to hold the result of any operation
    uint8_t res = 0;
    
    // Check null pointer
    if(st == NULL){
        printf("(%s) st is NULL\n", __func__);
        return TIMER_INVALID_ARGS;
    }

    uint8_t readbackC = 0;
    // Define the control word has a read back command
    // Set the appropriate timer and set not to read the count
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

    // Used to hold the result of any operation
    uint8_t res = 0;

    union timer_status_field_val timerStatus;

    switch(field){
        case all:
            timerStatus.byte = st;
            break;
        case initial:
            // Gets the register selection bits and converts them to timer_init by shifting them to the least
            // significant bits
            timerStatus.in_mode = (st & TIMER_LSB_MSB) >> 4;
            break;
        case mode:
            timerStatus.count_mode = (st & (BIT(3) | BIT(2) | BIT(1))) >> 1;

            // LCF generates with test 0 generates bogus status bytes
            // and only prints if the dont care bit is 0
            if(timerStatus.count_mode > 5)
                timerStatus.count_mode &= (uint8_t)(~BIT(2));
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
