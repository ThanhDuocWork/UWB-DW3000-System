#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UWB_LOG_LEVEL_ERROR = 0,
    UWB_LOG_LEVEL_WARN,
    UWB_LOG_LEVEL_INFO,
    UWB_LOG_LEVEL_DEBUG,
} uwb_log_level_t;

#ifdef __cplusplus
extern "C" {
#endif

bool uwb_log_set_tag_flags(const char *tag, uint32_t flags);
uint32_t uwb_log_get_tag_flags(const char *tag);
bool uwb_log_enable_tag_flags(const char *tag, uint32_t flags);
bool uwb_log_disable_tag_flags(const char *tag, uint32_t flags);
bool uwb_log_is_enabled(uwb_log_level_t level, const char *tag, uint32_t flag);
void uwb_log_write(uwb_log_level_t level, uint32_t flag, const char *tag, const char *fmt, ...);
void uwb_log_vwrite(uwb_log_level_t level, uint32_t flag, const char *tag, const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif

#ifndef UWB_LOG_TAG
#define UWB_LOG_TAG "UWB"
#endif

#define UWB_LOGE(tag, fmt, ...)       uwb_log_write(UWB_LOG_LEVEL_ERROR, 0U, tag, fmt, ##__VA_ARGS__)
#define UWB_LOGW(tag, flag, fmt, ...) uwb_log_write(UWB_LOG_LEVEL_WARN,  (uint32_t)(flag), tag, fmt, ##__VA_ARGS__)
#define UWB_LOGI(tag, flag, fmt, ...) uwb_log_write(UWB_LOG_LEVEL_INFO,  (uint32_t)(flag), tag, fmt, ##__VA_ARGS__)
#define UWB_LOGD(tag, flag, fmt, ...) uwb_log_write(UWB_LOG_LEVEL_DEBUG, (uint32_t)(flag), tag, fmt, ##__VA_ARGS__)

#define LOG_E(fmt, ...)             UWB_LOGE(UWB_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_W(flag, fmt, ...)       UWB_LOGW(UWB_LOG_TAG, flag, fmt, ##__VA_ARGS__)
#define LOG_I(flag, fmt, ...)       UWB_LOGI(UWB_LOG_TAG, flag, fmt, ##__VA_ARGS__)
#define LOG_D(flag, fmt, ...)       UWB_LOGD(UWB_LOG_TAG, flag, fmt, ##__VA_ARGS__)

#define UWB_LOG_SET_FLAGS(tag, flags)     uwb_log_set_tag_flags(tag, (uint32_t)(flags))
#define UWB_LOG_ENABLE_FLAGS(tag, flags)  uwb_log_enable_tag_flags(tag, (uint32_t)(flags))
#define UWB_LOG_DISABLE_FLAGS(tag, flags) uwb_log_disable_tag_flags(tag, (uint32_t)(flags))
