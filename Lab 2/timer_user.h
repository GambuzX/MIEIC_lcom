/*
 * This file contains macros and enums we've specified for this lab
 * timer_user because there already exists another timer.h
 */
#ifndef TIMER_USER_H
#define TIMER_USER_H

/*
 * Enumeration that contains possible error codes
 * for the function to ease development and debugging
 */
typedef enum _timer_status{
    TIMER_OK = OK, /*OK is part of the minix macros, if it's not included in the respective source file change this to 0*/
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
 * Reason: Code readability
 * Programmer can use the macros instead of doing the math himself
 */
#define UINT16_T_MAX (uint16_t)0xFFFF

/*
 * In mode 3, that we are using, the timer's frequency is given by the expression TIMER_FREQ/div, and so div
 * is given by TIMER_FREQ/freq. If freq > TIMER_FREQ, the integer division will result in 0, which will
 * lead to a division by 0 in TIMER_FREQ/div.
 * So, the maximum value allowed for freq is equal to TIMER_FREQ.
 */
#define TIMER_MAX_FREQ TIMER_FREQ

/* 
 * LSB and MSB are 8 bits each, so that means the divisor of TIMER_FREQ is 16 bit.
 * The max value a 16 unsigned int can hold is 2^16 - 1 = 65535.
 * The quotient of TIMER_FREQ / 65535 can be either an integer or have decimal places 
 * If it's an integer then that's the value of the minimum frequency
 * If it has decimal places, e.g 18.2, then there's a problem because the result must be an integer.
 * C's integer division truncates the decimal places so 18.2 would become 18, which is lower than actual
 * minimal accepted frequency. Due to that we must add 1 in order to get the ceiling of that actual number,
 * thus 18.2 becomes 19.
 */
#define TIMER_MIN_FREQ (uint16_t)TIMER_FREQ/UINT16_T_MAX + (((uint16_t)TIMER_FREQ % UINT16_T_MAX) ? 1 : 0)

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

#endif