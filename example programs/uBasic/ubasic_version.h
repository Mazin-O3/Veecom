#ifndef UBASIC_VERSION_H
#define UBASIC_VERSION_H

#include "ubasic_config.h"

#define UBASIC_VERSION "1.0"


#define STR(s) #s
#define STRINGIFY(s) STR(s)

#define UBASIC_STARTUP_MESSAGE "VEECOM uBASIC "UBASIC_VERSION"\n"\
STRINGIFY(UBASIC_FREE_BYTES)" uBASIC BYTES FREE\nREADY.\n"

#endif