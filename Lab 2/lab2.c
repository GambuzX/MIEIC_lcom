#include <lcom/lcf.h>

#include <lcom/lab2.h>
#include <lcom/timer.h>
#include "i8254.h"

#include <stdbool.h>
#include <stdint.h>

extern unsigned int timerIntCounter;

int main(int argc, char *argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need/ it]
    lcf_trace_calls("/home/lcom/labs/lab2/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab2/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
        return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}

int(timer_test_read_config)(uint8_t timer, enum timer_status_field field) {
    
    // Variable used to get and pass the configuration between the two functions
    uint8_t status;

    // Read the timer configuration and write it to the status variable
    if(timer_get_conf(timer, &status)){
        puts("Could not get the configuration/status!");
        return 1;
    }

    // Display the previously read timer configuration
    if(timer_display_conf(timer, status, field)){
        puts("Could not display the configuration/status");
        return 1;
    }
    return 0;
}

int(timer_test_time_base)(uint8_t timer, uint32_t freq) {

    // Change the frequency of the specified timer to freq
    if (timer_set_frequency(timer, freq)) {
        puts("Could not configure the timer's frequency!");
        return 1;
    }
  
    return 0;
}

int(timer_test_int)(uint8_t time) {

    // Variables to hold results
    int ipc_status;
    message msg;
    uint32_t r;
    
    // Subscribe the timer interrupts and store the hook_id used in bit_no
    uint8_t bit_no = 0;
    if((r = timer_subscribe_int(&bit_no)) != OK){
        printf("(%s) Couldnt subscribe interrupt\n", __func__);
        return 1;
    }
    
    // Mask used to verify if a caught interrupt should be handled
    uint32_t irq_set = BIT(bit_no);

    // Keep receiving and handling interrupts until the specified timer has passed
    while(time) {
        /* Get a request message.  */
        if( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
            printf("driver_receive failed with: %d", r);
            continue;
        }

        if (is_ipc_notify(ipc_status)) { /* received notification */
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE: /* hardware interrupt notification */
                if ( msg.m_notify.interrupts & irq_set) { /* subscribed interrupt */

                    // Handle the interrupt
                    timer_int_handler();
                    
                    // Check if one second has passed
                    if(timerIntCounter == 0){
                        time--;
                        timer_print_elapsed_time();
                    }                    
                }
                break;
            default:
                break; /* no other notifications expected: do nothing */
            }
        }
        else { /* received a standard message, not a notification */
        /* no standard messages expected: do nothing */
        }
    }
    
    // Unsubscribe further timer interrupts
    if(timer_unsubscribe_int() != OK){
	    printf("(%s) timer_unsubscribe_int: error while unsubscribing\n", __func__);
	    return 1;
    }

    return 0;
}

int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
    
    if(lsb == NULL)
        return 1;

    *lsb = (uint8_t) (0x00ff & val);
    return OK;
}

int(util_get_MSB)(uint16_t val, uint8_t *msb) {
    
    if(msb == NULL)
        return 1;

    *msb = (uint8_t) ((0xff00 & val)>>8);
    return OK;
}
