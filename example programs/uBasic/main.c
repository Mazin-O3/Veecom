#include "ubasic.h"

int main(void)
{
    ubasic_init();

    for (;;)
    {
        ubasic_run();
    }
}