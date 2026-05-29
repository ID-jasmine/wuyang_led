#include "drv_rtc.h"
#include "bsp_rtc.h"

static int RTC_HW_Init(RTC_Device *dev, uint8_t hour, uint8_t minute);
static en_result_t RTC_HW_ReadDateTime(RTC_Device *dev, stc_rtc_time_t *time);
static en_result_t RTC_HW_SetTime(RTC_Device *dev, const stc_rtc_time_t *time);

static const RTC_Ops s_rtc_ops = {
	.init = RTC_HW_Init,
	.read_datetime = RTC_HW_ReadDateTime,
	.set_time = RTC_HW_SetTime,
};

RTC_Device g_rtc_dev = {
	.ops = &s_rtc_ops,
	.context = 0,
};

static int RTC_HW_Init(RTC_Device *dev, uint8_t hour, uint8_t minute)
{
	(void)dev;
	BSP_RTC_Init(hour, minute);
	return 0;
}

static en_result_t RTC_HW_ReadDateTime(RTC_Device *dev, stc_rtc_time_t *time)
{
	(void)dev;
	return BSP_RTC_ReadDateTime(time);
}

static en_result_t RTC_HW_SetTime(RTC_Device *dev, const stc_rtc_time_t *time)
{
	(void)dev;
	return BSP_RTC_SetTime(time);
}

/**
 * @brief 初始化 RTC 驱动。
 * @param hour 初始小时值。
 * @param minute 初始分钟值。
 * @return int `0` 表示成功，负值表示失败。
 */
int DRV_RTC_Init(uint8_t hour, uint8_t minute)
{
	if (g_rtc_dev.ops == 0 || g_rtc_dev.ops->init == 0)
	{
		return -1;
	}
	return g_rtc_dev.ops->init(&g_rtc_dev, hour, minute);
}

/**
 * @brief 读取当前 RTC 日期和时间。
 * @param time 输出参数，保存读取到的时间结构体。
 * @return en_result_t 读取结果。
 */
en_result_t DRV_RTC_ReadDateTime(stc_rtc_time_t *time)
{
	if (g_rtc_dev.ops == 0 || g_rtc_dev.ops->read_datetime == 0)
	{
		return Error;
	}
	return g_rtc_dev.ops->read_datetime(&g_rtc_dev, time);
}

/**
 * @brief 设置 RTC 当前时间。
 * @param time 输入参数，指向待设置的时间结构体。
 * @return en_result_t 设置结果。
 */
en_result_t DRV_RTC_SetTime(const stc_rtc_time_t *time)
{
	if (g_rtc_dev.ops == 0 || g_rtc_dev.ops->set_time == 0)
	{
		return Error;
	}
	return g_rtc_dev.ops->set_time(&g_rtc_dev, time);
}
