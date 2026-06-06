#include "uwb_log.h"

#include <stdio.h>
#include <string.h>

#include "esp_timer.h"

namespace {

constexpr size_t UWB_LOG_MAX_TAGS = 32;
constexpr size_t UWB_LOG_TAG_NAME_SIZE = 24;

typedef struct {
    bool used;
    char tag[UWB_LOG_TAG_NAME_SIZE];
    uint32_t flags;
} uwb_log_tag_entry_t;

uwb_log_tag_entry_t s_log_tags[UWB_LOG_MAX_TAGS] = {};

const char *level_to_text(uwb_log_level_t level)
{
    switch (level) {
    case UWB_LOG_LEVEL_ERROR:
        return "E";
    case UWB_LOG_LEVEL_WARN:
        return "W";
    case UWB_LOG_LEVEL_INFO:
        return "I";
    case UWB_LOG_LEVEL_DEBUG:
    default:
        return "D";
    }
}

uwb_log_tag_entry_t *find_tag_entry(const char *tag)
{
    if (tag == nullptr || tag[0] == '\0') {
        return nullptr;
    }

    for (size_t i = 0; i < UWB_LOG_MAX_TAGS; ++i) {
        if (s_log_tags[i].used && strncmp(s_log_tags[i].tag, tag, UWB_LOG_TAG_NAME_SIZE) == 0) {
            return &s_log_tags[i];
        }
    }

    return nullptr;
}

uwb_log_tag_entry_t *find_or_create_tag_entry(const char *tag)
{
    uwb_log_tag_entry_t *entry = find_tag_entry(tag);
    if (entry != nullptr) {
        return entry;
    }

    if (tag == nullptr || tag[0] == '\0') {
        return nullptr;
    }

    for (size_t i = 0; i < UWB_LOG_MAX_TAGS; ++i) {
        if (!s_log_tags[i].used) {
            s_log_tags[i].used = true;
            strncpy(s_log_tags[i].tag, tag, UWB_LOG_TAG_NAME_SIZE - 1U);
            s_log_tags[i].tag[UWB_LOG_TAG_NAME_SIZE - 1U] = '\0';
            s_log_tags[i].flags = 0U;
            return &s_log_tags[i];
        }
    }

    return nullptr;
}

}  // namespace

extern "C" bool uwb_log_set_tag_flags(const char *tag, uint32_t flags)
{
    uwb_log_tag_entry_t *entry = find_or_create_tag_entry(tag);
    if (entry == nullptr) {
        return false;
    }

    entry->flags = flags;
    return true;
}

extern "C" uint32_t uwb_log_get_tag_flags(const char *tag)
{
    uwb_log_tag_entry_t *entry = find_tag_entry(tag);
    return (entry != nullptr) ? entry->flags : 0U;
}

extern "C" bool uwb_log_enable_tag_flags(const char *tag, uint32_t flags)
{
    uwb_log_tag_entry_t *entry = find_or_create_tag_entry(tag);
    if (entry == nullptr) {
        return false;
    }

    entry->flags |= flags;
    return true;
}

extern "C" bool uwb_log_disable_tag_flags(const char *tag, uint32_t flags)
{
    uwb_log_tag_entry_t *entry = find_or_create_tag_entry(tag);
    if (entry == nullptr) {
        return false;
    }

    entry->flags &= ~flags;
    return true;
}

extern "C" bool uwb_log_is_enabled(uwb_log_level_t level, const char *tag, uint32_t flag)
{
    if (level == UWB_LOG_LEVEL_ERROR) {
        return true;
    }

    if (flag == 0U) {
        return false;
    }

    return (uwb_log_get_tag_flags(tag) & flag) != 0U;
}

extern "C" void uwb_log_vwrite(uwb_log_level_t level, uint32_t flag, const char *tag, const char *fmt, va_list args)
{
    const unsigned long long timestamp_ms = (unsigned long long)(esp_timer_get_time() / 1000ULL);
    const char *safe_tag = (tag != nullptr && tag[0] != '\0') ? tag : "UWB";

    if (!uwb_log_is_enabled(level, safe_tag, flag)) {
        return;
    }

    printf("[%10llu][%s][%s] ", timestamp_ms, level_to_text(level), safe_tag);
    vprintf(fmt, args);
    printf("\n");
}

extern "C" void uwb_log_write(uwb_log_level_t level, uint32_t flag, const char *tag, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    uwb_log_vwrite(level, flag, tag, fmt, args);
    va_end(args);
}
