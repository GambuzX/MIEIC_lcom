#include <lcom/lcf.h>

#include "mouse.h"
#include "timer_user.h"

#include "state_machine.h"


int main(int argc, char *argv[]) {

    /* sets the language of LCF messages (can be either EN-US or PT-PT)*/
    lcf_set_language("EN-US");

    /* enables to log function invocations that are being "wrapped" by LCF
    [comment this out if you don't want/need/ it] */
    lcf_trace_calls("/home/lcom/labs/lab4/trace.txt");

    /* enables to save the output of printf function calls on a file
    [comment this out if you don't want/need it] */
    lcf_log_output("/home/lcom/labs/lab4/output.txt");

    /* handles control over to LCF
    [LCF handles command line arguments and invokes the right function] */
    if (lcf_start(argc, argv))
        return 1;

    /* LCF clean up tasks
    [must be the last statement before return] */
    lcf_cleanup();

    return 0;
}


int (mouse_test_packet)(uint32_t cnt) {

    uint8_t bitNum;

    /* Subscribe mouse interrupts */
    if (mouse_subscribe_int(&bitNum) != OK)
      return 1;

    /* Enable Data Reporting */
    if(mouse_enable_dr() != OK){
  		printf("Error enabling data report\n");
  		return 1;
  	}

    /* Variables to hold results */
    int ipc_status;
    message msg;
    uint32_t r = 0;
    uint8_t mouse_packet[MOUSE_PACKET_SIZE];
    struct packet pp;
    
    /* Mask used to verify if a caught interrupt should be handled */
    uint32_t irq_set = BIT(bitNum);

    /* Loop until processed specified number of packets */
    while(cnt) {
        /* Get a request message.  */
        if( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
            printf("driver_receive failed with: %d", r);
            continue;
        }

        if (is_ipc_notify(ipc_status)) { /* received notification */
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE: /* hardware interrupt notification */
                if ( msg.m_notify.interrupts & irq_set) { /* subscribed interrupt */

                    /* Handle interrupt */
                    mouse_ih();

                    /* Check if we have a full mouse packet */
                    if (assemble_mouse_packet(mouse_packet)) {
                        parse_mouse_packet(mouse_packet, &pp);
                        mouse_print_packet(&pp);
                        cnt--;
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

    /* Disable Data Reporting */
    if(mouse_disable_dr() != OK){
  		printf("Error disabling data report\n");
  		return 1;
  	}

    /* Unsubscribe mouse interrupts */
    if (mouse_unsubscribe_int() != OK)
      return 1;

    return 0;
}

int (mouse_test_remote)(uint16_t period, uint8_t cnt) {

    /*
     * LCF already configures mouse to operate in remote mode and disables mouse interrupts,
     * so the KBC and mouse are ready to respond to read data commands
     */

    uint8_t mouse_packet[MOUSE_PACKET_SIZE];
    struct packet pp;

    while (cnt) {

      /* Read mouse packets using Read Data command (0xEB) */
      if (mouse_read_data_cmd(mouse_packet) != OK)
        return 1;

      /* Read the 3 packet bytes from the OBF. Stop when we have a full packet */
      do {
        mouse_poll_handler();
      } while (!assemble_mouse_packet(mouse_packet));

      parse_mouse_packet(mouse_packet, &pp);
      mouse_print_packet(&pp);
      cnt--;

      /* Wait 'period' miliseconds (must convert to microseconds, hence the *1000) */
      tickdelay(micros_to_ticks(period*1000));
    }

    /* Restore the kbc state to default */
    if (restore_kbc_state(minix_get_dflt_kbc_cmd_byte()) != OK){
      printf("There was an error restoring the keyboard state\n");
		  return 1;
    }

    return 0;
}

int (mouse_test_async)(uint8_t idle_time) {

    /* Variables to hold results */
    int ipc_status;
    message msg;
    uint32_t r;
    struct packet pp;
    uint8_t mouse_packet[MOUSE_PACKET_SIZE];

    /* Return immediatelly if idle time is 0 */
    if(idle_time == 0)
        return 0;
    
    /* Subscribe the timer interrupts */
    uint8_t bit_no = 0;
    if(timer_subscribe_int(&bit_no) != OK){
        printf("(%s) timer_subscribe_int: error while subscribing\n", __func__);
        return 1;
    }
    /* Mask used to verify if a caught timer interrupt should be handled */
    uint32_t timer_irq_set = BIT(bit_no);

    /* Subscribe mouse interrupts */
    if(mouse_subscribe_int(&bit_no) != OK){
        printf("(%s) mouse_subscribe_int: error while subscribing\n", __func__);
        return 1;
    }    

    /* Enable data reporting */
    if(mouse_enable_dr() != OK){
  		printf("Error enabling data report\n");
  		return 1;
  	}

    /* Mask used to verify if a caught mouse interrupt should be handled */
    uint32_t mouse_irq_set = BIT(bit_no);

    /* Variable to keep track of remaining time */
    uint32_t remaining_time = idle_time;

    /* Handle different timer frequencies */
    set_internal_frequency_counter(sys_hz());

    /* Keep receiving and handling interrupts until the specified timer has passed */
    while(remaining_time) {
        /* Get a request message.  */
        if( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
            printf("driver_receive failed with: %d", r);
            continue;
        }

        if (is_ipc_notify(ipc_status)) { /* received notification */
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE: /* hardware interrupt notification */

                    /* Mouse interrupt */
                    if ( msg.m_notify.interrupts & mouse_irq_set) {

                        mouse_ih();

                        if (assemble_mouse_packet(mouse_packet)) {
                            parse_mouse_packet(mouse_packet, &pp);
                            mouse_print_packet(&pp);
                        }

                        /* Reset remaining_time */
                        remaining_time = idle_time;

                        /* Reset timerIntCounter */
                        timer_reset_int_counter();
                    }

                    /* Timer interrupt */
                    if ( msg.m_notify.interrupts & timer_irq_set) {

                        /* Timer interrupt handler */
                        timer_int_handler();

                        /* Check if one second has passed and update remaining_time */
                        if(get_timer_int_counter() == 0)
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
    
    /* Unsubscribe further timer interrupts */
    if(timer_unsubscribe_int() != OK){
      printf("(%s) timer_unsubscribe_int: error while unsubscribing\n", __func__);
      return 1;
    }

    /* Disable data reporting */
    if (mouse_disable_dr() != OK)
      return 1;

    /*  Unsubscribe further Mouse Interrupts */
    if(mouse_unsubscribe_int() != OK){
      printf("(%s) mouse_unsubscribe_int: error while unsubscribing\n", __func__);
      return 1;
    }

    return 0;
}

int (mouse_test_gesture)(uint8_t x_len, uint8_t tolerance) {
    uint8_t bitNum;

    /* Initialize state machine */
    state_machine_init(x_len, tolerance);

    /* Subscribe mouse interrupts */
    if (mouse_subscribe_int(&bitNum) != OK)
      return 1;
    /* Enable Data Reporting */
    if (mouse_enable_dr() != OK){
  		printf("Error enabling data report\n");
  		return 1;
  	}

    /* Variables to hold results */
    int ipc_status;
    message msg;
    uint32_t r = 0;
    uint8_t mouse_packet[MOUSE_PACKET_SIZE];
    struct packet pp;
    
    /* Mask used to verify if a caught interrupt should be handled */
    uint32_t irq_set = BIT(bitNum);

    /* Loop until state machine reaches a final state */
    while(!state_machine_ended()) {
        /* Get a request message.  */
        if( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
            printf("driver_receive failed with: %d", r);
            continue;
        }

        if (is_ipc_notify(ipc_status)) { /* received notification */
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE: /* hardware interrupt notification */
                  if ( msg.m_notify.interrupts & irq_set) { /* subscribed interrupt */

                      /* Handle interrupt */
                      mouse_ih();

                      /* Check if we have a full mouse packet */
                      if (assemble_mouse_packet(mouse_packet)) {
                          parse_mouse_packet(mouse_packet, &pp);
                          mouse_print_packet(&pp);
  					              state_machine_handle_packet(&pp);
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

    /* Disable data reporting */
    if (mouse_disable_dr() != OK)
      return 1;

    /* Unsubscribe mouse interrupts */
    if (mouse_unsubscribe_int() != OK)
      return 1;

    return 0;
}
