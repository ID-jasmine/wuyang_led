#include "bsp_tm3100_led.h"
#include "gpio.h"

#define TM3100_SDI_PORT  GpioPortA
#define TM3100_SDI_PIN   GpioPin8
#define TM3100_CLK_PORT  GpioPortB
#define TM3100_CLK_PIN   GpioPin15
#define TM3100_LE_PORT   GpioPortB
#define TM3100_LE_PIN    GpioPin14
#define TM3100_OE_PORT   GpioPortB
#define TM3100_OE_PIN    GpioPin13

static uint16_t s_au16Tm3100Buffer[BSP_TM3100_CHIP_COUNT];
static boolean_t s_bTm3100Inited = FALSE;

static void Tm3100_SetPin(en_gpio_port_t port, en_gpio_pin_t pin, boolean_t level)
{
    if (level)
    {
        (void)Gpio_SetIO(port, pin);
    }
    else
    {
        (void)Gpio_ClrIO(port, pin);
    }
}

static void Tm3100_SetSdi(boolean_t level)
{
    Tm3100_SetPin(TM3100_SDI_PORT, TM3100_SDI_PIN, level);
}

static void Tm3100_SetClk(boolean_t level)
{
    Tm3100_SetPin(TM3100_CLK_PORT, TM3100_CLK_PIN, level);
}

static void Tm3100_SetLe(boolean_t level)
{
    Tm3100_SetPin(TM3100_LE_PORT, TM3100_LE_PIN, level);
}

static void Tm3100_SetOe(boolean_t level)
{
    Tm3100_SetPin(TM3100_OE_PORT, TM3100_OE_PIN, level);
}

static void Tm3100_Delay(void)
{
    volatile uint8_t i;

    for (i = 0u; i < 8u; i++)
    {
        __NOP();
    }
}

static const stc_bsp_tm3100_ops_t s_stcTm3100GpioOps =
{
    Tm3100_SetSdi,
    Tm3100_SetClk,
    Tm3100_SetLe,
    Tm3100_SetOe,
    Tm3100_Delay,
};

stc_bsp_tm3100_t g_stcTm3100Led =
{
    &s_stcTm3100GpioOps,
    s_au16Tm3100Buffer,
    BSP_TM3100_CHIP_COUNT,
    BspTm3100BitMsbFirst,
    BspTm3100ChipLastFirst,
    FALSE,
};

static en_result_t Tm3100_InitOutputPin(en_gpio_port_t port, en_gpio_pin_t pin)
{
    stc_gpio_cfg_t stcGpioCfg;

    stcGpioCfg.enDir = GpioDirOut;
    stcGpioCfg.enDrv = GpioDrvH;
    stcGpioCfg.enPu = GpioPuDisable;
    stcGpioCfg.enPd = GpioPdDisable;
    stcGpioCfg.enOD = GpioOdDisable;
    stcGpioCfg.enCtrlMode = GpioFastIO;

    return Gpio_Init(port, pin, &stcGpioCfg);
}

static en_result_t Tm3100_CheckReady(void)
{
    if ((FALSE == s_bTm3100Inited) || (NULL == g_stcTm3100Led.ops) || (NULL == g_stcTm3100Led.buffer))
    {
        return ErrorUninitialized;
    }

    return Ok;
}

static void Tm3100_OutputEnableRaw(boolean_t enable)
{
    boolean_t level;

    level = (enable) ? g_stcTm3100Led.oe_active_level : (boolean_t)!g_stcTm3100Led.oe_active_level;
    g_stcTm3100Led.ops->set_oe(level);
}

static void Tm3100_SendBit(boolean_t bit)
{
    g_stcTm3100Led.ops->set_sdi(bit);
    g_stcTm3100Led.ops->set_clk(FALSE);
    g_stcTm3100Led.ops->delay();
    g_stcTm3100Led.ops->set_clk(TRUE);
    g_stcTm3100Led.ops->delay();
}

static void Tm3100_SendWord(uint16_t data)
{
    int8_t bit;

    if (BspTm3100BitMsbFirst == g_stcTm3100Led.bit_order)
    {
        for (bit = 15; bit >= 0; bit--)
        {
            Tm3100_SendBit((boolean_t)((data >> bit) & 0x0001u));
        }
    }
    else
    {
        for (bit = 0; bit < 16; bit++)
        {
            Tm3100_SendBit((boolean_t)((data >> bit) & 0x0001u));
        }
    }
}

static void Tm3100_Latch(void)
{
    g_stcTm3100Led.ops->set_le(FALSE);
    g_stcTm3100Led.ops->delay();
    g_stcTm3100Led.ops->set_le(TRUE);
    g_stcTm3100Led.ops->delay();
    g_stcTm3100Led.ops->set_le(FALSE);
}

en_result_t Bsp_Tm3100Led_Init(void)
{
    en_result_t enRet;

    enRet = Tm3100_InitOutputPin(TM3100_SDI_PORT, TM3100_SDI_PIN);
    if (Ok != enRet)
    {
        return enRet;
    }

    enRet = Tm3100_InitOutputPin(TM3100_CLK_PORT, TM3100_CLK_PIN);
    if (Ok != enRet)
    {
        return enRet;
    }

    enRet = Tm3100_InitOutputPin(TM3100_LE_PORT, TM3100_LE_PIN);
    if (Ok != enRet)
    {
        return enRet;
    }

    enRet = Tm3100_InitOutputPin(TM3100_OE_PORT, TM3100_OE_PIN);
    if (Ok != enRet)
    {
        return enRet;
    }

    s_bTm3100Inited = TRUE;
    (void)Bsp_Tm3100Led_OutputEnable(FALSE);
    g_stcTm3100Led.ops->set_sdi(FALSE);
    g_stcTm3100Led.ops->set_clk(FALSE);
    g_stcTm3100Led.ops->set_le(FALSE);
    (void)Bsp_Tm3100Led_Clear();
    (void)Bsp_Tm3100Led_Refresh();

    return Ok;
}

en_result_t Bsp_Tm3100Led_SetChipChannel(uint8_t chip, uint8_t channel, boolean_t on)
{
    if ((chip >= g_stcTm3100Led.chip_count) || (channel >= BSP_TM3100_CHANNEL_PER_CHIP))
    {
        return ErrorInvalidParameter;
    }

    if (on)
    {
        g_stcTm3100Led.buffer[chip] |= (uint16_t)(1u << channel);
    }
    else
    {
        g_stcTm3100Led.buffer[chip] &= (uint16_t)~(1u << channel);
    }

    return Ok;
}

en_result_t Bsp_Tm3100Led_SetLinear(uint16_t index, boolean_t on)
{
    uint8_t chip;
    uint8_t channel;

    if (index >= BSP_TM3100_LED_COUNT)
    {
        return ErrorInvalidParameter;
    }

    chip = (uint8_t)(index / BSP_TM3100_CHANNEL_PER_CHIP);
    channel = (uint8_t)(index % BSP_TM3100_CHANNEL_PER_CHIP);

    return Bsp_Tm3100Led_SetChipChannel(chip, channel, on);
}

en_result_t Bsp_Tm3100Led_SetChipData(uint8_t chip, uint16_t data)
{
    if (chip >= g_stcTm3100Led.chip_count)
    {
        return ErrorInvalidParameter;
    }

    g_stcTm3100Led.buffer[chip] = data;

    return Ok;
}

en_result_t Bsp_Tm3100Led_Clear(void)
{
    uint8_t chip;

    for (chip = 0u; chip < g_stcTm3100Led.chip_count; chip++)
    {
        g_stcTm3100Led.buffer[chip] = 0x0000u;
    }

    return Ok;
}

en_result_t Bsp_Tm3100Led_Fill(void)
{
    uint8_t chip;

    for (chip = 0u; chip < g_stcTm3100Led.chip_count; chip++)
    {
        g_stcTm3100Led.buffer[chip] = 0xFFFFu;
    }

    return Ok;
}

en_result_t Bsp_Tm3100Led_Refresh(void)
{
    int8_t chip;
    en_result_t enRet;

    enRet = Tm3100_CheckReady();
    if (Ok != enRet)
    {
        return enRet;
    }

    Tm3100_OutputEnableRaw(FALSE);

    if (BspTm3100ChipLastFirst == g_stcTm3100Led.chip_order)
    {
        for (chip = (int8_t)(g_stcTm3100Led.chip_count - 1u); chip >= 0; chip--)
        {
            Tm3100_SendWord(g_stcTm3100Led.buffer[(uint8_t)chip]);
        }
    }
    else
    {
        for (chip = 0; chip < (int8_t)g_stcTm3100Led.chip_count; chip++)
        {
            Tm3100_SendWord(g_stcTm3100Led.buffer[(uint8_t)chip]);
        }
    }

    Tm3100_Latch();
    Tm3100_OutputEnableRaw(TRUE);

    return Ok;
}

en_result_t Bsp_Tm3100Led_OutputEnable(boolean_t enable)
{
    en_result_t enRet;

    enRet = Tm3100_CheckReady();
    if (Ok != enRet)
    {
        return enRet;
    }

    Tm3100_OutputEnableRaw(enable);

    return Ok;
}

uint16_t Bsp_Tm3100Led_GetChipData(uint8_t chip)
{
    if (chip >= g_stcTm3100Led.chip_count)
    {
        return 0u;
    }

    return g_stcTm3100Led.buffer[chip];
}

uint16_t *Bsp_Tm3100Led_GetBuffer(void)
{
    return g_stcTm3100Led.buffer;
}
