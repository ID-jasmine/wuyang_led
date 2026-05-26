#include "led_panel.h"

int32_t main(void)
{
    (void)LedPanel_Init();

    (void)LedPanel_Clear();
    (void)LedPanel_SetChipChannel(0u, 0u, TRUE);
    (void)LedPanel_SetChipChannel(14u, 15u, TRUE);
    (void)LedPanel_Refresh();

    while(1)
    {

    }
}



