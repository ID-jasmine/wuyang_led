#include "bsp_time_capture.h"
#include "sysctrl.h"
#include "timer3.h"

#define BSP_TIM3_CAPTURE_CLOCK_HZ	  (250000u)
#define BSP_TIM3_CAPTURE_PERIOD_TICKS (0x10000u)
#define BSP_TIM3_CAPTURE_HALF_PERIOD  (0x8000u)

static bsp_tim3_capture_t g_tim3_cap;
static volatile uint32_t s_u32Tim3BaseTick = 0u;

static const bsp_tim3_cap_pin_t s_atim3_cap_pins[] = {
	[BSP_TIM3id_speed] = {BSP_TIM3_CAP_CH2A, GpioPortA, GpioPin10, GpioAf2},
	[BSP_TIM3id_rpm] = {BSP_TIM3_CAP_CH1A, GpioPortA, GpioPin9, GpioAf2},
};

static en_tim3_channel_t Bsp_Tim3Capture_GetTim3Channel(bsp_tim3_cap_ch_t ch)
{
	switch (ch)
	{
	case BSP_TIM3_CAP_CH1A:
		return Tim3CH1;

	case BSP_TIM3_CAP_CH2A:
		return Tim3CH2;

	default:
		return Tim3CH0;
	}
}

static en_tim3_irq_type_t Bsp_Tim3Capture_GetIrqType(bsp_tim3_cap_ch_t ch)
{
	switch (ch)
	{
	case BSP_TIM3_CAP_CH1A:
		return Tim3CA1Irq;

	case BSP_TIM3_CAP_CH2A:
		return Tim3CA2Irq;

	default:
		return Tim3CA0Irq;
	}
}

static en_tim3_m23_ccrx_t Bsp_Tim3Capture_GetCcrSel(bsp_tim3_cap_ch_t ch)
{
	switch (ch)
	{
	case BSP_TIM3_CAP_CH1A:
		return Tim3CCR1A;

	case BSP_TIM3_CAP_CH2A:
		return Tim3CCR2A;

	default:
		return Tim3CCR0A;
	}
}

static uint32_t Bsp_Tim3Capture_ExtendTimestamp(uint16_t cap_value,
											   boolean_t overflow_pending)
{
	uint32_t timestamp;

	timestamp = s_u32Tim3BaseTick + cap_value;
	if ((TRUE == overflow_pending) && (cap_value < BSP_TIM3_CAPTURE_HALF_PERIOD))
	{
		timestamp += BSP_TIM3_CAPTURE_PERIOD_TICKS;
	}

	return timestamp;
}

static void Bsp_Tim3Capture_PortInit(void)
{
	uint8_t i;
	stc_gpio_cfg_t gpio_cfg;

	DDL_ZERO_STRUCT(gpio_cfg);

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	gpio_cfg.enDir = GpioDirIn;
	gpio_cfg.enDrv = GpioDrvH;
	gpio_cfg.enPu = GpioPuDisable;
	gpio_cfg.enPd = GpioPdEnable;
	gpio_cfg.enOD = GpioOdDisable;
	gpio_cfg.enCtrlMode = GpioFastIO;

	for (i = 0; i < BSP_TIM3id_COUNT; i++)
	{
		(void)Gpio_Init(s_atim3_cap_pins[i].port, s_atim3_cap_pins[i].pin, &gpio_cfg);
		(void)Gpio_SetAfMode(s_atim3_cap_pins[i].port, s_atim3_cap_pins[i].pin,
							 s_atim3_cap_pins[i].af);
	}
}

static void Bsp_Tim3Capture_TimerInit(void)
{
	uint8_t i;
	stc_tim3_mode23_cfg_t base_cfg;
	stc_tim3_m23_input_cfg_t cap_cfg;

	DDL_ZERO_STRUCT(base_cfg);
	DDL_ZERO_STRUCT(cap_cfg);

	Sysctrl_SetPeripheralGate(SysctrlPeripheralTim3, TRUE);

	base_cfg.enWorkMode = Tim3WorkMode3;
	base_cfg.enCT = Tim3Timer;
	base_cfg.enPRS = Tim3PCLKDiv64; // 16MHz / 64 = 250kHz
	base_cfg.enCntDir = Tim3CntUp;

	(void)Tim3_Mode23_Init(&base_cfg);

	cap_cfg.enCHxACmpCap = Tim3CHxCapMode;
	cap_cfg.enCHxACapSel = Tim3CHxCapRise;
	cap_cfg.enCHxAInFlt = Tim3FltPCLKDiv16Cnt3;
	cap_cfg.enCHxAPolarity = Tim3PortPositive;

	for (i = 0; i < BSP_TIM3id_COUNT; i++)
	{
		(void)Tim3_M23_PortInput_Cfg(
			Bsp_Tim3Capture_GetTim3Channel(s_atim3_cap_pins[i].ch), &cap_cfg);
	}

	(void)Tim3_M23_ARRSet(0xFFFFu, TRUE); // arr
	(void)Tim3_M23_Cnt16Set(0u);		  // arr初值

	(void)Tim3_ClearAllIntFlag(); // 清楚中断

	(void)Tim3_Mode23_EnableIrq(Tim3UevIrq);
	for (i = 0; i < BSP_TIM3id_COUNT; i++)
	{
		(void)Tim3_Mode23_EnableIrq(Bsp_Tim3Capture_GetIrqType(s_atim3_cap_pins[i].ch));
	}
	EnableNvic(TIM3_IRQn, IrqLevel2, TRUE);
	(void)Tim3_M23_Run();
}

/**
 * @brief  初始化TIM3捕获模块
 * @details 初始化捕获输入引脚、TIM3硬件配置，并设置基础计时参数。
 * @retval Ok: 初始化成功
 */
en_result_t BSP_TimeCapture_Init(void)
{
	g_tim3_cap.timer_clk_hz = BSP_TIM3_CAPTURE_CLOCK_HZ;
	s_u32Tim3BaseTick = 0u;

	Bsp_Tim3Capture_PortInit();
	Bsp_Tim3Capture_TimerInit();

	return Ok;
}

/**
 * @brief  注册TIM3捕获回调函数
 * @param  [in] callback 捕获事件回调函数
 * @param  [in] user_data 用户自定义上下文指针
 * @retval Ok: 注册成功
 */
en_result_t BSP_TimeCapture_RegisterCallback(bsp_tim3_cap_cb_t callback, void *user_data)
{
	g_tim3_cap.callback = callback;
	g_tim3_cap.user_data = user_data;

	return Ok;
}

/**
 * @brief  获取TIM3捕获定时器时钟频率
 * @return 当前捕获定时器时钟频率，单位Hz
 */
uint32_t BSP_TimeCapture_GetTimerClockHz(void)
{
	return g_tim3_cap.timer_clk_hz;
}

/**
 * @brief  TIM3捕获中断处理函数
 * @details 轮询各捕获通道中断标志，计算时间戳后通过回调上报。
 */
void BSP_TimeCapture_IRQHandler(void)
{
	uint8_t i;
	uint16_t cap_value;
	uint32_t timestamp;
	en_tim3_irq_type_t irq_type;
	boolean_t overflow_pending;

	overflow_pending = Tim3_GetIntFlag(Tim3UevIrq);

	for (i = 0; i < BSP_TIM3id_COUNT; i++)
	{
		irq_type = Bsp_Tim3Capture_GetIrqType(s_atim3_cap_pins[i].ch);
		if (TRUE == Tim3_GetIntFlag(irq_type))
		{
			cap_value =
				Tim3_M23_CCR_Get(Bsp_Tim3Capture_GetCcrSel(s_atim3_cap_pins[i].ch));
			(void)Tim3_ClearIntFlag(irq_type);

			timestamp = Bsp_Tim3Capture_ExtendTimestamp(cap_value, overflow_pending);

			if (NULL != g_tim3_cap.callback)
			{
				g_tim3_cap.callback(s_atim3_cap_pins[i].ch, timestamp,
									g_tim3_cap.user_data);
			}
		}
	}

	if (TRUE == overflow_pending)
	{
		(void)Tim3_ClearIntFlag(Tim3UevIrq);
		s_u32Tim3BaseTick += BSP_TIM3_CAPTURE_PERIOD_TICKS;
	}
}
