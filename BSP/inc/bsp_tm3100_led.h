#ifndef __BSP_TM3100_LED_H__
#define __BSP_TM3100_LED_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BSP_TM3100_CHIP_COUNT		(15u)
#define BSP_TM3100_CHANNEL_PER_CHIP (16u)
#define BSP_TM3100_LED_COUNT		(BSP_TM3100_CHIP_COUNT * BSP_TM3100_CHANNEL_PER_CHIP)

	typedef enum en_bsp_tm3100_bit_order
	{
		BspTm3100BitMsbFirst = 0u,
		BspTm3100BitLsbFirst = 1u,
	} en_bsp_tm3100_bit_order_t;

	typedef enum en_bsp_tm3100_chip_order
	{
		BspTm3100ChipLastFirst = 0u,
		BspTm3100ChipFirstFirst = 1u,
	} en_bsp_tm3100_chip_order_t;

	typedef struct stc_bsp_tm3100_ops
	{
		void (*set_sdi)(boolean_t level);
		void (*set_clk)(boolean_t level);
		void (*set_le)(boolean_t level);
		void (*delay)(void);
	} stc_bsp_tm3100_ops_t;

	typedef struct stc_bsp_tm3100
	{
		const stc_bsp_tm3100_ops_t *ops;
		uint16_t *buffer;
		uint8_t chip_count;
		en_bsp_tm3100_bit_order_t bit_order;
		en_bsp_tm3100_chip_order_t chip_order;
		void *reserved;
	} stc_bsp_tm3100_t;

	extern stc_bsp_tm3100_t g_stcTm3100Led;

	en_result_t Bsp_Tm3100Led_Init(void);
	en_result_t Bsp_Tm3100Led_SetChipChannel(uint8_t chip, uint8_t channel, boolean_t on);
	en_result_t Bsp_Tm3100Led_SetLinear(uint16_t index, boolean_t on);
	en_result_t Bsp_Tm3100Led_SetChipData(uint8_t chip, uint16_t data);
	en_result_t Bsp_Tm3100Led_Clear(void);
	en_result_t Bsp_Tm3100Led_Fill(void);
	en_result_t Bsp_Tm3100Led_Refresh(void);
	en_result_t Bsp_Tm3100Led_OutputEnable(boolean_t enable);
	en_result_t Bsp_Tm3100Led_SetBrightness(uint8_t brightness_percent);
	uint8_t Bsp_Tm3100Led_GetBrightness(void);
	uint16_t Bsp_Tm3100Led_GetChipData(uint8_t chip);
	uint16_t *Bsp_Tm3100Led_GetBuffer(void);

#ifdef __cplusplus
}
#endif

#endif
