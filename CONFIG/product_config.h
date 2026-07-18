#ifndef PRODUCT_CONFIG_H
#define PRODUCT_CONFIG_H

/*
 * 产品功能开关。
 *
 * 这里描述“这个产品要什么”，BOARD 描述“这些功能接在哪里”。
 * 当前仪表的核心功能全部启用；派生产品可在复制本文件后按需关闭实例。
 */
#define PRODUCT_FEATURE_EEPROM       (1u)
#define PRODUCT_FEATURE_ADC          (1u)
#define PRODUCT_FEATURE_SPEED_RPM    (1u)
#define PRODUCT_FEATURE_LED_PANEL    (1u)

/* 产品行为配置。 */
#define PRODUCT_NORMAL_FUNCTION      (1u)
#define PRODUCT_DEEP_SLEEP_DELAY_MS  (1500u)

/* 当前 APP 以 ADC 电门和 LED 面板为核心依赖，错误组合在编译期直接阻止。 */
#if ((PRODUCT_NORMAL_FUNCTION != 0u) && (PRODUCT_FEATURE_ADC == 0u))
	#error "Normal vehicle application requires PRODUCT_FEATURE_ADC"
#endif

#if ((PRODUCT_NORMAL_FUNCTION != 0u) && (PRODUCT_FEATURE_LED_PANEL == 0u))
	#error "Normal vehicle application requires PRODUCT_FEATURE_LED_PANEL"
#endif

#endif
