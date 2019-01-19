/* void update_OBF_status_asm(); */
/* using _C_LABEL() from <machine/asm.h> */
#include  <machine/asm.h>
#include "i8042.h"

.intel_syntax
.global _C_LABEL(update_OBF_status_asm)

.text
_C_LABEL(update_OBF_status_asm):
	/* Reset OBF status */
	mov DWORD PTR [OBFStatus], 0

	/* Read keyboard status */
    in eax, KEYBOARD_CTRL

	/* Handle situations where the output buffer is not full */
	test al, KEYBOARD_OBF 
	jz update_OBF_status_asm_end

	/* Dont read mouse data */
	test al, KEYBOARD_AUX
	jnz update_OBF_status_asm_end

	/* Read the keyboard OBF */
	mov edx, eax
	in eax, KEYBOARD_OUT
	mov [outBuffer], al

	/* Ignore OBF value if there is an error */
	test dl, (KEYBOARD_TIMEOUT_ERR | KEYBOARD_PARITY_ERR)
	jnz update_OBF_status_asm_end

	/* Set OBF status as valid */
	mov DWORD PTR [OBFStatus], 1

update_OBF_status_asm_end:
    ret
