#ifndef SYSTEM_RUNTIME_H
#define SYSTEM_RUNTIME_H

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* 工程中所有模块的统一生命周期入口。 */
en_result_t SystemRuntime_Init(void);
void SystemRuntime_Task1ms(void);
void SystemRuntime_PrepareDeepSleep(void);
void SystemRuntime_RestoreClockAfterWake(void);
void SystemRuntime_WakeupIgnCheck(void);
void SystemRuntime_ResumeAfterIgn(void);
void SystemRuntime_AbortIgnCheck(void);

#ifdef __cplusplus
}
#endif

#endif
