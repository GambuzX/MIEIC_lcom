#include <lcom/lcf.h>

#include <stdbool.h>
#include <stdint.h>

#include "keyboard.h"
#include "i8042.h"

/* LCF already includes timer.h from another folder */
#include "timer_extra.h"

/* sys_inb_counter from the keyboard */
extern unsigned int sys_inb_counter;

/* Interrupt counter from the timer */
extern unsigned int timerIntCounter;

int main(int argc, char *argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need/ it]
    lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab3/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
      return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}

int (kbd_test_scan)(bool assembly) {

    // Subscribe KBC Interrupts
    uint8_t bitNum;
    if(keyboard_subscribe_int(&bitNum) != OK)
      return 1;

    // Variables to hold results
    int ipc_status;
    message msg;
    uint32_t r = 0;
    uint8_t scancodes[SCANCODES_BYTES_LEN];
    
    // Mask used to verify if a caught interrupt should be handled
    uint32_t irq_set = BIT(bitNum);

    // Keep receiving and handling interrupts until the ESC key is released.
    while(!(r == 1 && scancodes[0] == ESC_BREAK)) {
        /* Get a request message.  */
        if( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
            printf("driver_receive failed with: %d", r);
            continue;
        }

        if (is_ipc_notify(ipc_status)) { /* received notification */
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE: /* hardware interrupt notification */
                if ( msg.m_notify.interrupts & irq_set) { /* subscribed interrupt */

                    // Call the appropriate handler
                    if(assembly)
                        kbc_asm_ih();
                    else
                        kbc_ih();

                    // Verify if there is any available opcode for the scancodes read
                    if((r = opcode_available(scancodes)))
                        kbd_print_scancode( is_make_code(r, scancodes), r, scancodes);
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

    // Unsubscribe KBC Interrupts
    if(keyboard_unsubscribe_int() != OK)
      return 1;

#ifdef LAB3
    // Print the number of sys_inb calls that ocurred if not using assembly
    if (!assembly)
      kbd_print_no_sysinb(sys_inb_counter);
#endif

    return 0;
}


int (kbd_test_poll)() {

  // First we need to disable keyboard interrupts. But lcf_start() already handles that

  // Variables to hold results
  uint32_t r = 0;
  uint8_t scancodes[SCANCODES_BYTES_LEN] = {0, 0};

  // Loop until ESC key is released or an invalid scancode is found
  while(!(r == 1 && scancodes[0] == ESC_BREAK)) {

	//First update the status then read the opcode
	update_OBF_status();

    // Verify if there is any available opcode for the scancodes read
    if((r = opcode_available(scancodes)))
      kbd_print_scancode( is_make_code(r, scancodes), r, scancodes);

    tickdelay(micros_to_ticks(DELAY_US));
  }

  // Reenable keyboard interrupts
  if(reenable_keyboard()){
      puts("There was a problem reenabling the keyboard");
      return 1;
  }

#ifdef LAB3
  // Print the number of sys_inb calls that ocurred
  kbd_print_no_sysinb(sys_inb_counter);
#endif
  
  return 0;
}


int (kbd_test_timed_scan)(uint8_t n) {

    // Variables to hold results
    int ipc_status;
    message msg;
    uint32_t r;
    uint8_t scancodes[SCANCODES_BYTES_LEN];

	//Return immediatelly if idle time is 0
    if(n == 0)
        return 0;
    
    // Subscribe the timer interrupts
    uint8_t bit_no = 0;
    if(timer_subscribe_int(&bit_no) != OK){
        printf("(%s) timer_subscribe_int: error while subscribing\n", __func__);
        return 1;
    }
    // Mask used to verify if a caught timer interrupt should be handled
    uint32_t timer_irq_set = BIT(bit_no);

    // Subscribe keyboard interrupts
    if(keyboard_subscribe_int(&bit_no) != OK){
        printf("(%s) keyboard_subscribe_int: error while subscribing\n", __func__);
        return 1;
    }    
    // Mask used to verify if a caught kb interrupt should be handled
    uint32_t kb_irq_set = BIT(bit_no);

    // Variable to keep track of remaining time
    uint32_t remaining_time = n;

    // Keep receiving and handling interrupts until the specified timer has passed
    while(remaining_time && scancodes[0] != ESC_BREAK) {
        /* Get a request message.  */
        if( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
            printf("driver_receive failed with: %d", r);
            continue;
        }

        if (is_ipc_notify(ipc_status)) { /* received notification */
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE: /* hardware interrupt notification */

                    // Keyboard interrupt
                    if ( msg.m_notify.interrupts & kb_irq_set) {

                        // Keyboard interrupt handler
                        kbc_ih();

                        // Verify if there is any available opcode for the scancodes read
                        if((r = opcode_available(scancodes)))
                            kbd_print_scancode( is_make_code(r, scancodes), r, scancodes);

                        // Reset remaining_time
                        remaining_time = n;

                        // Reset timerIntCounter
                        timer_reset_int_counter();
                    }

                    // Timer interrupt
                    if ( msg.m_notify.interrupts & timer_irq_set) {

                        // Timer interrupt handler
                        timer_int_handler();

                        // Check if one second has passed and update remaining_time
                        if(timerIntCounter == 0)
                            remaining_time--;
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

    // Unsubscribe further KBC Interrupts
    if(keyboard_unsubscribe_int() != OK){
      printf("(%s) keyboard_unsubscribe_int: error while unsubscribing\n", __func__);
      return 1;
    }

#ifdef LAB3
    // Print the number of sys_inb calls that ocurred
    kbd_print_no_sysinb(sys_inb_counter);
#endif

    return 0;
}
