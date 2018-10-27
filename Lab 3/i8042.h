#ifndef _LCOM_I8042_H_
#define _LCOM_I8042_H_

#define BIT(n) (0x01<<(n))

#define KEYBOARD_IRQ				1

#define DELAY_US					20000
#define DELAY_TRIES                 10

/* I/O port addresses */

#define KEYBOARD_CTRL				0x64 /* Control register */
#define KEYBOARD_IN_CMD				0x64 /* Input buffer to issue commands to the KBC */
#define KEYBOARD_IN_ARG				0x60 /* Input buffer to write arguments of KBC commands */
#define KEYBOARD_OUT 				0x60 /* Output buffer */

/* Status Register */

#define KEYBOARD_OBF				BIT(0) /* Check if output buffer is full */
#define KEYBOARD_IBF				BIT(1) /* Check if input buffer is full */
#define KEYBOARD_A2					BIT(3) /* 0 - data byte; 1 - command byte */
#define KEYBOARD_AUX				BIT(5) /* Mouse data */
#define KEYBOARD_TIMEOUT_ERR		BIT(6) /* Timeout error */
#define KEYBOARD_PARITY_ERR			BIT(7) /* Parity error */

/* KBC Command Byte */

#define KBC_CB_INT					BIT(0) /* Enable interrupt on OBF, from keyboard */
#define KBC_CB_INT2					BIT(1) /* Enable interrupt on OBF, from mouse */
#define KBC_CB_DIS					BIT(4) /* Disable keyboard interface */
#define KBC_CB_DIS2					BIT(5) /* Disable mouse */

/* Keys scancodes */

#define ESC_MAKE					0x01 /* Esc key make code */
#define ESC_BREAK					0x81 /* Esc key break code */

/* KBC Commands */
#define KBC_READ_CMD_BYTE           0x20 /* Read command byte */
#define KBC_WRITE_CMD_BYTE          0x60 /* Write command byte */
#define KEYBOARD_CHECK_KBC			0xAA /* Returns 0x55 if OK; 0xFC if error */
#define KEYBOARD_CHECK_INTERFACE	0xAB /* Retuns 0 if OK */
#define KEYBOARD_DIS_KBD			0xAD /* Disables the KBD Interface */
#define KEYBOARD_EN_KBD				0xAE /* Enables the KBD Interface */

#endif /* _LCOM_I8042_H */
