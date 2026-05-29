#include "bsp_tim1_oe.h"

#include "bt.h"
#include "gpio.h"
#include "sysctrl.h"

#define BSP_TIM1_OE_UNIT		 TIM1
#define BSP_TIM1_OE_PORT		 GpioPortB
#define BSP_TIM1_OE_PIN			 GpioPin13
#define BSP_TIM1_OE_AF			 GpioAf5
#define BSP_TIM1_OE_PERIOD_TICKS (999u)
#define BSP_TIM1_OE_MAX_PERCENT	 (100u)

static uint8_t s_u8Tim1OeBrightness = 0u;

static uint16_t BspTim1Oe_PercentToCompare(uint8_t brightness_percent)
{
	uint32_t compare;

	if (brightness_percent > BSP_TIM1_OE_MAX_PERCENT)
	{
		brightness_percent = BSP_TIM1_OE_MAX_PERCENT;
	}

	compare = ((uint32_t)(BSP_TIM1_OE_PERIOD_TICKS + 1u) * brightness_percent) /
			  BSP_TIM1_OE_MAX_PERCENT;

	if (compare > BSP_TIM1_OE_PERIOD_TICKS + 1)
	{
		compare = BSP_TIM1_OE_PERIOD_TICKS + 1;
	}

	return (uint16_t)compare;
}

static en_result_t BspTim1Oe_PortInit(void)
{
	stc_gpio_cfg_t gpio_cfg;
	en_result_t ret;

	DDL_ZERO_STRUCT(gpio_cfg);

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	gpio_cfg.enDir = GpioDirOut;
	gpio_cfg.enDrv = GpioDrvH;
	gpio_cfg.enPu = GpioPuDisable;
	gpio_cfg.enPd = GpioPdDisable;
	gpio_cfg.enOD = GpioOdDisable;
	gpio_cfg.enCtrlMode = GpioFastIO;

	ret = Gpio_Init(BSP_TIM1_OE_PORT, BSP_TIM1_OE_PIN, &gpio_cfg);
	if (Ok != ret)
	{
		return ret;
	}

	return Gpio_SetAfMode(BSP_TIM1_OE_PORT, BSP_TIM1_OE_PIN, BSP_TIM1_OE_AF);
}

static en_result_t BspTim1Oe_TimerInit(void)
{
	stc_bt_mode23_cfg_t base_cfg;
	stc_bt_m23_compare_cfg_t compare_cfg;
	en_result_t ret;

	DDL_ZERO_STRUCT(base_cfg);
	DDL_ZERO_STRUCT(compare_cfg);

	Sysctrl_SetPeripheralGate(SysctrlPeripheralBaseTim, TRUE);

	base_cfg.enWorkMode = BtWorkMode3;
	base_cfg.enCT = BtTimer;
	base_cfg.enPRS = BtPCLKDiv16; // 16MHz / 16 / 1000 = 1kHz PWM
	base_cfg.enCntDir = BtCntUp;
	base_cfg.enPWMTypeSel = BtIndependentPWM;
	base_cfg.enPWM2sSel = BtSinglePointCmp;

	ret = Bt_Mode23_Init(BSP_TIM1_OE_UNIT, &base_cfg);
	if (Ok != ret)
	{
		return ret;
	}

	compare_cfg.enCh0ACmpCap = BtCHxCmpMode;
	compare_cfg.enCH0ACmpCtrl = BtPWMMode1;
	compare_cfg.enCH0APolarity = BtPortOpposite;
	compare_cfg.bCh0ACmpBufEn = TRUE;
	compare_cfg.enCh0ACmpIntSel = BtCmpIntNone;

	compare_cfg.enCh0BCmpCap = BtCHxCmpMode;
	compare_cfg.enCH0BCmpCtrl = BtForceHigh;
	compare_cfg.enCH0BPolarity = BtPortPositive;
	compare_cfg.bCH0BCmpBufEn = TRUE;
	compare_cfg.enCH0BCmpIntSel = BtCmpIntNone;

	ret = Bt_M23_PortOutput_Cfg(BSP_TIM1_OE_UNIT, &compare_cfg);
	if (Ok != ret)
	{
		return ret;
	}

	ret = Bt_M23_ARRSet(BSP_TIM1_OE_UNIT, BSP_TIM1_OE_PERIOD_TICKS, TRUE);
	if (Ok != ret)
	{
		return ret;
	}

	ret = Bt_M23_Cnt16Set(BSP_TIM1_OE_UNIT, 0u);
	if (Ok != ret)
	{
		return ret;
	}

	ret = Bt_ClearAllIntFlag(BSP_TIM1_OE_UNIT);
	if (Ok != ret)
	{
		return ret;
	}

	ret = Bt_M23_EnPWM_Output(BSP_TIM1_OE_UNIT, TRUE, TRUE);
	if (Ok != ret)
	{
		return ret;
	}

	return Bt_M23_Run(BSP_TIM1_OE_UNIT);
}

en_result_t BSP_Tim1Oe_Init(uint8_t brightness_percent)
{
	en_result_t ret;

	ret = BspTim1Oe_PortInit();
	if (Ok != ret)
	{
		return ret;
	}

	ret = BspTim1Oe_TimerInit();
	if (Ok != ret)
	{
		return ret;
	}

	return BSP_Tim1Oe_SetBrightness(brightness_percent);
}

en_result_t BSP_Tim1Oe_SetBrightness(uint8_t brightness_percent)
{
	if (brightness_percent > BSP_TIM1_OE_MAX_PERCENT)
	{
		brightness_percent = BSP_TIM1_OE_MAX_PERCENT;
	}

	s_u8Tim1OeBrightness = brightness_percent;

	return Bt_M23_CCR_Set(BSP_TIM1_OE_UNIT, BtCCR0A,
						  BspTim1Oe_PercentToCompare(brightness_percent));
}

uint8_t BSP_Tim1Oe_GetBrightness(void)
{
	return s_u8Tim1OeBrightness;
}
