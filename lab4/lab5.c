// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>
#include "vbe.h"
#include "keyboard.h"
#include "i8042.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <math.h>

// Any header files included below this line should have been created by you

int main(int argc, char *argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need it]
    lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab5/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
    return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}

int (video_test_init)(uint16_t mode, uint8_t delay) {

    if(set_video_mode(mode) != OK)
        return 1;

    sleep(delay);
    vg_exit();

	return 0;
}

int (video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height, uint32_t color) {

    /* Initialize graphics mode */
    vg_init(mode);

    if (vg_draw_rectangle(x, y, width, height, color) != OK) {
      printf("(%s) There was a problem drawing the rectangle\n", __func__);
      vg_exit();
      return 1;
    }

    // Subscribe KBC Interrupts
    uint8_t bitNum;
    if(keyboard_subscribe_int(&bitNum) != OK) {
      vg_exit();
      return 1; } 
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

                    kbc_ih();
                    r = opcode_available(scancodes);
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
    if(keyboard_unsubscribe_int() != OK) {
      vg_exit();
      return 1;
    }

    vg_exit();
    
    return 0;
}

int (video_test_pattern)(uint16_t mode, uint8_t no_rectangles, uint32_t first, uint8_t step) {

  /* Initialize graphics mode */
  vg_init(mode);

  uint16_t width = (uint16_t) floor((float) get_x_res() / no_rectangles);
  uint16_t height = (uint16_t) floor((float) get_y_res() / no_rectangles);

  for (int row = 0; row < no_rectangles; row++) {
    for (int col = 0; col < no_rectangles; col++) {
      /* Direct color mode */
      if (get_memory_model() == DIRECT_COLOR_MODE) {
        uint32_t orig_red = (first >> get_red_field_position()) & set_bits_mask(get_red_mask_size());
        uint32_t orig_green = (first >> get_green_field_position()) & set_bits_mask(get_green_mask_size());
        uint32_t orig_blue = (first >> get_blue_field_position()) & set_bits_mask(get_blue_mask_size());
        uint32_t orig_rsvd = (first >> get_rsvd_field_position()) & set_bits_mask(get_rsvd_mask_size());

        uint32_t red = (orig_red + col * step) % (1 << get_red_mask_size());
        uint32_t green = (orig_green + row * step) % (1 << get_green_mask_size());
        uint32_t blue = (orig_blue + (col + row) * step) % (1 << get_blue_mask_size());

        uint32_t color = (orig_rsvd << get_rsvd_field_position()) | (red << get_red_field_position()) | (green << get_green_field_position()) | (blue << get_blue_field_position());

        /* Draw rectangle */
        if (vg_draw_rectangle(col * width, row * height, width, height, color) != OK) {
          printf("(%s) There was a problem drawing the rectangle\n", __func__);
          vg_exit();
          return 1;
        }

      }
      /* Indexed color mode */
      else if (get_memory_model() == INDEXED_COLOR_MODE) {
        /* Calculate index color */
        uint32_t index_color = (first + (row * no_rectangles + col) * step) % (1 << get_bits_per_pixel());

        /* Draw rectangle */
        if (vg_draw_rectangle(col * width, row * height, width, height, index_color) != OK) {
          printf("(%s) There was a problem drawing the rectangle\n", __func__);
          vg_exit();
          return 1;
        }
      }
      else {
        printf("(%s) Unsuported color mode, __func__\n");
        return 1;
      }
    }
  }

  // Subscribe KBC Interrupts
  uint8_t bitNum;
  if(keyboard_subscribe_int(&bitNum) != OK) {
    vg_exit();
    return 1;
  }

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

                  kbc_ih();
                  r = opcode_available(scancodes);
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
  if(keyboard_unsubscribe_int() != OK) {
    vg_exit();
    return 1;
  }

  vg_exit();

	return 0;
}

int (video_test_xpm)(const char *xpm[], uint16_t x, uint16_t y){

    /* Initialize graphics mode */
    vg_init(0x105);

    int width, height;
    char *pixmap = read_xpm(xpm, &width, &height);
    if(pixmap == NULL)
      return 1;

        
    draw_pixmap(pixmap, x, y, width, height);

    // Subscribe KBC Interrupts
    uint8_t bitNum;
    if(keyboard_subscribe_int(&bitNum) != OK) {
      vg_exit();
      return 1;
    }

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

                    kbc_ih();
                    r = opcode_available(scancodes);
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
    if(keyboard_unsubscribe_int() != OK) {
      vg_exit();
      return 1;
    }

    vg_exit();
    
    return 0;

}

int video_test_move( const char *xpm[],uint16_t xi, uint16_t yi,uint16_t xf, uint16_t yf, int8_t speed, uint8_t frame_rate) {

	Sprite spr;
	// Initialize sprite

	Quantos interrupts para mudar de frame ?
	N = (N interrupts por segundo = 60?) / frame_rate

	Se positivo
	A cada N interrupts, dar shift da imagem SPEED pixeis

	Se negativo
	Esperar SPEED frames para dar shift de 1 pixel

	Verificar se xi >= xf e yi >= yf; 
	Se sim, fazer xi == xf e yi == yf e acabar





}


int (video_test_controller)() {
  struct reg86u r;
  memset(&r, 0, sizeof(r));
  vg_vbe_contr_info_t contr_info;
  mmap_t mmap;

  // Call lm_init
  if(lm_init(true) == NULL){
      printf("(%s) I couldnt init lm\n", __func__);
      return 1;
  }

  // Allocate memory block in low memory area
  if (lm_alloc(sizeof(vg_vbe_contr_info_t), &mmap) == NULL) {
    printf("(%s): lm_alloc() failed\n", __func__);
    return 1;
  }

  // Build the struct
  r.u.b.ah = VBE_FUNC; 
  r.u.b.al = RETURN_VBE_CONTROLLER_INFO;
  r.u.w.es = PB2BASE(mmap.phys);
  r.u.w.di = PB2OFF(mmap.phys);
  r.u.b.intno = VIDEO_CARD_SRV;

  // BIOS Call
  if( sys_int86(&r) != FUNC_SUCCESS ) {
      printf("(%s): sys_int86() failed \n", __func__);
      return 1;
  }

  // Verify the return for errors
  if (r.u.w.ax != FUNC_RETURN_OK) {
      printf("(%s): sys_int86() return in ax was different from OK \n", __func__);
      return 1;     
  }

  // Copy the requested info to vbe_mode_info
  memcpy(&contr_info, mmap.virt, sizeof(vg_vbe_contr_info_t));

  if (vg_display_vbe_contr_info(&contr_info) != OK) {
    printf("(%s): vg_display_vbe_contr_info returned with an error\n", __func__);
    return 1;
  }

  // Free allocated memory
  lm_free(&mmap);

  return OK;
}

