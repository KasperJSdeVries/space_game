#include "core/logger.h"

#include "core/asserts.h"
#include "core/filesystem.h"
#include "core/sstring.h"
#include "platform/platform.h"

// TODO: temporary
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// NOTE: ~32k character limit
#define MESSAGE_LENGTH 32000
#define LEVEL_STRING_MAX_LENGTH 12
#define FORMAT_STING_LENGTH MESSAGE_LENGTH - LEVEL_STRING_MAX_LENGTH

typedef struct logger_system_state {
	file_handle log_file_handle;
} logger_system_state;

static logger_system_state *state_ptr;

void append_to_log_file(const char *message);

b8 logging_initialize(u64 *memory_requirement, void *state) {
	*memory_requirement = sizeof(logger_system_state);
	if (state == 0) { return true; }

	state_ptr = state;

	if (!filesystem_open("console.log", FILE_MODE_WRITE, false, &state_ptr->log_file_handle)) {
		platform_console_write_error("ERROR: Unable to open console.log for writing.", LOG_LEVEL_ERROR);
		return false;
	}

	return true;
}

void logging_shutdown() {
	// TODO: cleanup logging/write queued entries.
}

void log_output(log_level level, const char *message, ...) {
	const char *level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};
	b8 is_error                  = level < LOG_LEVEL_WARN;

	// NOTE: ~32k character limit
	char formatted_message[FORMAT_STING_LENGTH];
	memset(formatted_message, 0, sizeof(formatted_message));

	va_list arg_ptr;
	va_start(arg_ptr, message);
	vsnprintf(formatted_message, FORMAT_STING_LENGTH, message, arg_ptr);
	va_end(arg_ptr);

	char out_message[MESSAGE_LENGTH];
	sprintf(out_message, "%s%s", level_strings[level], formatted_message);

	// Platform-specific output.
	if (is_error) {
		platform_console_write_error(out_message, level);
	} else {
		platform_console_write(out_message, level);
	}
}

void report_assertion_failure(const char *expression, const char *message, const char *file, i32 line) {
	log_output(LOG_LEVEL_FATAL,
			   "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n",
			   expression,
			   message,
			   file,
			   line);
}

void append_to_log_file(const char *message) {
	if (state_ptr && state_ptr->log_file_handle.is_valid) {
		u64 length  = string_length(message);
		u64 written = 0;
		if (!filesystem_write(&state_ptr->log_file_handle, length, message, &written)) {
			platform_console_write_error("Error: failed to write to console.log.", LOG_LEVEL_ERROR);
		}
	}
}
