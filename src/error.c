#include "retro2d_internal.h"

#include <stdarg.h>
#include <string.h>

static char error_text[512];
static LogCallback logger;
static void *logger_data;

const char *last_error(void)
{
    return error_text;
}

void set_log_callback(LogCallback callback, void *user_data)
{
    logger = callback;
    logger_data = user_data;
}

static void emit(LogLevel level, const char *message)
{
    static const char *names[] = { "info", "warning", "error" };
    if (logger) {
        logger(level, message, logger_data);
    } else {
        fprintf(level == LOG_INFO ? stdout : stderr, "retro2d %s: %s\n",
                names[(int)level], message);
    }
}

void log_message(LogLevel level, const char *format, ...)
{
    char buffer[512];
    va_list arguments;
    va_start(arguments, format);
    (void)vsnprintf(buffer, sizeof buffer, format, arguments);
    va_end(arguments);
    emit(level, buffer);
}

void r2d_set_error(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    (void)vsnprintf(error_text, sizeof error_text, format, arguments);
    va_end(arguments);
    emit(LOG_ERROR, error_text);
}
