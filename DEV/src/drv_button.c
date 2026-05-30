#include "drv_button.h"
#include "bsp_gpio.h"
#include "dev_speed_rpm.h"

#define BTN_DEBOUNCE_CNT   (20u)
#define BTN_SHORT_PRESS_MS (1000u)
#define BTN_LONG_PRESS_MS  (3000u)
#define BTN_TIMEOUT_MS	   (10000u)

typedef struct
{
	uint16_t k1_press_time;
	uint16_t k2_press_time;
	uint16_t both_press_time;
	uint16_t idle_time;
	en_drv_button_event_t events[DrvButtonIdMax];

	boolean_t k1_dbc;
	boolean_t k2_dbc;
	uint8_t k1_dbc_cnt;
	uint8_t k2_dbc_cnt;

	boolean_t k1_long_fired;
	boolean_t k2_long_fired;
	boolean_t both_long_fired;
	boolean_t k1_part_of_both;
	boolean_t k2_part_of_both;
} stc_drv_button_t;

static stc_drv_button_t s_stcBtn;

void DRV_Button_Init(void)
{
	uint8_t i;

	s_stcBtn.k1_press_time = 0u;
	s_stcBtn.k2_press_time = 0u;
	s_stcBtn.both_press_time = 0u;
	s_stcBtn.idle_time = 0u;

	s_stcBtn.k1_dbc = FALSE;
	s_stcBtn.k2_dbc = FALSE;
	s_stcBtn.k1_dbc_cnt = 0u;
	s_stcBtn.k2_dbc_cnt = 0u;

	s_stcBtn.k1_long_fired = FALSE;
	s_stcBtn.k2_long_fired = FALSE;
	s_stcBtn.both_long_fired = FALSE;
	s_stcBtn.k1_part_of_both = FALSE;
	s_stcBtn.k2_part_of_both = FALSE;

	for (i = 0u; i < DrvButtonIdMax; i++)
	{
		s_stcBtn.events[i] = DrvButtonEventNone;
	}
}

void DRV_Button_Task1ms(void)
{
	boolean_t k1_raw = !Bsp_Gpio_Read(BspGpioIdSwK1);
	boolean_t k2_raw = !Bsp_Gpio_Read(BspGpioIdSwK2);
	boolean_t has_speed = FALSE;

	if (DEV_SpeedRpm_GetFreqMilliHz(DevSpeedRpmIdSpeed) > 0u)
	{
		has_speed = TRUE;
	}

	/* k1 debounce */
	if (k1_raw != s_stcBtn.k1_dbc)
	{
		s_stcBtn.k1_dbc_cnt++;
		if (s_stcBtn.k1_dbc_cnt >= BTN_DEBOUNCE_CNT)
		{
			s_stcBtn.k1_dbc = k1_raw;
			s_stcBtn.k1_dbc_cnt = 0u;
		}
	}
	else
	{
		s_stcBtn.k1_dbc_cnt = 0u;
	}

	/* k2 debounce */
	if (k2_raw != s_stcBtn.k2_dbc)
	{
		s_stcBtn.k2_dbc_cnt++;
		if (s_stcBtn.k2_dbc_cnt >= BTN_DEBOUNCE_CNT)
		{
			s_stcBtn.k2_dbc = k2_raw;
			s_stcBtn.k2_dbc_cnt = 0u;
		}
	}
	else
	{
		s_stcBtn.k2_dbc_cnt = 0u;
	}

	/* Idle timeout logic */
	if (s_stcBtn.k1_dbc || s_stcBtn.k2_dbc)
	{
		s_stcBtn.idle_time = 0u;
	}
	else
	{
		if (s_stcBtn.idle_time < BTN_TIMEOUT_MS)
		{
			s_stcBtn.idle_time++;
		}
	}

	/* Evaluate key lengths */
	if (s_stcBtn.k1_dbc && s_stcBtn.k2_dbc)
	{
		s_stcBtn.both_press_time++;
		s_stcBtn.k1_press_time = 0u;
		s_stcBtn.k2_press_time = 0u;
		s_stcBtn.k1_part_of_both = TRUE;
		s_stcBtn.k2_part_of_both = TRUE;

		if (s_stcBtn.both_press_time == BTN_LONG_PRESS_MS)
		{
			if (!has_speed)
			{
				s_stcBtn.events[DrvButtonIdBoth] = DrvButtonEventLongPress;
			}
			s_stcBtn.both_long_fired = TRUE;
		}
	}
	else if (s_stcBtn.k1_dbc)
	{
		if (s_stcBtn.both_press_time > 0u)
		{
			s_stcBtn.both_press_time = 0u;
			s_stcBtn.both_long_fired = FALSE;
		}

		if (!s_stcBtn.k1_part_of_both)
		{
			s_stcBtn.k1_press_time++;
			if (s_stcBtn.k1_press_time == BTN_LONG_PRESS_MS)
			{
				if (!has_speed)
				{
					s_stcBtn.events[DrvButtonIdK1] = DrvButtonEventLongPress;
				}
				s_stcBtn.k1_long_fired = TRUE;
			}
		}
		s_stcBtn.k2_press_time = 0u;
		s_stcBtn.k2_long_fired = FALSE;
		s_stcBtn.k2_part_of_both = FALSE;
	}
	else if (s_stcBtn.k2_dbc)
	{
		if (s_stcBtn.both_press_time > 0u)
		{
			s_stcBtn.both_press_time = 0u;
			s_stcBtn.both_long_fired = FALSE;
		}

		if (!s_stcBtn.k2_part_of_both)
		{
			s_stcBtn.k2_press_time++;
			if (s_stcBtn.k2_press_time == BTN_LONG_PRESS_MS)
			{
				if (!has_speed)
				{
					s_stcBtn.events[DrvButtonIdK2] = DrvButtonEventLongPress;
				}
				s_stcBtn.k2_long_fired = TRUE;
			}
		}
		s_stcBtn.k1_press_time = 0u;
		s_stcBtn.k1_long_fired = FALSE;
		s_stcBtn.k1_part_of_both = FALSE;
	}
	else
	{
		if (s_stcBtn.k1_press_time > 0u)
		{
			if ((!s_stcBtn.k1_long_fired) &&
				(s_stcBtn.k1_press_time < BTN_SHORT_PRESS_MS))
			{
				s_stcBtn.events[DrvButtonIdK1] = DrvButtonEventShortPress;
			}
			s_stcBtn.k1_press_time = 0u;
		}

		if (s_stcBtn.k2_press_time > 0u)
		{
			if ((!s_stcBtn.k2_long_fired) &&
				(s_stcBtn.k2_press_time < BTN_SHORT_PRESS_MS))
			{
				s_stcBtn.events[DrvButtonIdK2] = DrvButtonEventShortPress;
			}
			s_stcBtn.k2_press_time = 0u;
		}

		s_stcBtn.both_press_time = 0u;
		s_stcBtn.k1_long_fired = FALSE;
		s_stcBtn.k2_long_fired = FALSE;
		s_stcBtn.both_long_fired = FALSE;
		s_stcBtn.k1_part_of_both = FALSE;
		s_stcBtn.k2_part_of_both = FALSE;
	}
}

en_drv_button_event_t DRV_Button_GetEvent(en_drv_button_id_t id)
{
	en_drv_button_event_t event = DrvButtonEventNone;

	if (id < DrvButtonIdMax)
	{
		event = s_stcBtn.events[id];
		s_stcBtn.events[id] = DrvButtonEventNone;
	}

	return event;
}

boolean_t DRV_Button_IsTimeout10s(void)
{
	if (s_stcBtn.idle_time >= BTN_TIMEOUT_MS)
	{
		return TRUE;
	}
	return FALSE;
}

void DRV_Button_ClearTimeout(void)
{
	s_stcBtn.idle_time = 0u;
}
