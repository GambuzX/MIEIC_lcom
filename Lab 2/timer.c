#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  if (timer > 2) {
    printf("Timer range must be between 0 and 2! %d given\n", timer);
  }

  uint8_t previousConfig;
  timer_get_conf(timer, &previousConfig);

  uint8_t countingModeMask = BIT(0);
  uint8_t operatingModeMask = (BIT(1) | BIT(2) | BIT(3));

  uint8_t countingMode = countingModeMask & previousConfig;
  uint8_t operatingMode = operatingModeMask & previousConfig;
  uint8_t counterSelection;
  uint8_t selectedTimer;
  switch (timer) {
    case 0:
      counterSelection = TIMER_SEL0;
      selectedTimer = TIMER_0;
      break;
    case 1:
      counterSelection = TIMER_SEL1;
      selectedTimer = TIMER_1;
      break;
    case 2:
      counterSelection = TIMER_SEL2;
      selectedTimer = TIMER_2;
      break;
  }

  uint8_t controlWord = (counterSelection | TIMER_LSB_MSB | operatingMode | countingMode);

  if(sys_outb(TIMER_CTRL, controlWord)) {
    //TODO Handle error
  }

  uint16_t initialValue = TIMER_FREQ / freq;
  uint8_t lsb, msb;
  util_get_LSB(initialValue, &lsb);
  util_get_MSB(initialValue, &msb);

  // Initialize registers
  if (sys_outb(selectedTimer, lsb)) {
    //TODO Handle error
  }

  if (sys_outb(selectedTimer, msb)) {
    //TODO Handle error
  }

  return 0;
}

int (timer_subscribe_int)(uint8_t *UNUSED(bit_no)) {
  /* To be completed by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
}

int (timer_unsubscribe_int)() {
  /* To be completed by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
}

void (timer_int_handler)() {
  /* To be completed by the students */
  printf("%s is not yet implemented!\n", __func__);
}

int (timer_get_conf)(uint8_t timer, uint8_t *st) {
	if (timer > 2) {
		printf("Timer range must be between 0 and 2! %d given\n", timer);
	}

	// Initialize read back command
	uint32_t read_back_c = 0;

	// Counter selection
	read_back_c |= TIMER_RB_SEL(timer);

	// Do not read counter value
	read_back_c |= TIMER_RB_COUNT_;

	// Set read back command bits 
	read_back_c |= TIMER_RB_CMD;

	// Write Read-Back command to Control Register
	sys_outb(TIMER_CTRL, read_back_c);

	uint32_t status = 0;
	switch (timer) {
	case 0:
		sys_inb(TIMER_0, &status);
		*st = (uint8_t) status;
		break;
	case 1:
		sys_inb(TIMER_1, &status);
		*st = (uint8_t) status;
		break;
	case 2:
		sys_inb(TIMER_2, &status);
		*st = (uint8_t) status;
		break;
	default:
		printf("Could not read from the Control Register.");
		return 1;
	}
	return 0;
}

int (timer_display_conf)(uint8_t timer, uint8_t st,
                        enum timer_status_field field) {

	//timer is unsigned so eveyrthing is greater than 0
	if( timer > 2 ){
		printf("Timer range must be between 0 and 2! %d given\n", timer);
		return 1;
	}
	
	union timer_status_field_val timer_status;

	switch(field){
		case all:
			timer_status.byte = st;
			break;
		case initial:
			timer_status.in_mode = (st & (BIT(5) | BIT(4))) >> 4;
			break;
		case mode:
			timer_status.count_mode = (st & (BIT(3) & BIT(2) & BIT(1))) >> 1;
			break;
		case base:
			timer_status.bcd = (st & BIT(0));	
			break;
	}

	if(timer_print_config(timer, field, timer_status))
		return 1;

	return 0;
}
