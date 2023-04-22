#include "core/logger.h"
#include "core/asserts.h"

// NOTE: temporary
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

b8_t logging_initialize() {
  // TODO: create log file.
  return true;
}

void logging_shutdown() {
  // TODO: cleanup logging/write queued entries.
}

void log_output(log_level level, const char *message, ...) {
  const char *level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ",
                                  "[INFO]: ",  "[DEBUG]: ", "[TRACE]: "};
  b8_t is_error = level < 2;

  // NOTE: ~32k character limit
  char formatted_message[32000 - 12];
  memset(formatted_message, 0, sizeof(formatted_message));

  __builtin_va_list arg_ptr;
  va_start(arg_ptr, message);
  vsnprintf(formatted_message, 32000 - 12, message, arg_ptr);
  va_end(arg_ptr);

  char out_message[32000];
  sprintf(out_message, "%s%s\n", level_strings[level], formatted_message);

  // TODO: platform-specific output.
  printf("%s", out_message);
}

void report_assertion_failure(const char *expression, const char *message,
                              const char *file, int32_t line) {
  log_output(LOG_LEVEL_FATAL,
             "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n",
             expression, message, file, line);
}
