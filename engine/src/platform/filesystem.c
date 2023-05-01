#include "core/filesystem.h"

#include "core/logger.h"
#include "core/smemory.h"

#include <stdio.h>
#include <sys/stat.h>

b8 filesystem_exists(char const *path) {
  struct stat buffer;
  return stat(path, &buffer) == 0;
}

b8 filesystem_open(char const *path, file_modes mode, b8 binary,
                   file_handle *out_handle) {
  out_handle->handle = 0;
  out_handle->is_valid = false;

  const char *mode_str;
  if ((mode & FILE_MODE_WRITE) != 0) {
    if ((mode & FILE_MODE_READ) != 0) {
      mode_str = binary ? "w+b" : "w+";
    } else {
      mode_str = binary ? "wb" : "w";
    }
  } else if ((mode & FILE_MODE_READ) != 0) {
    mode_str = binary ? "rb" : "r";
  } else {
    SERROR("Invalid mode passed while trying to open file: '%s'", path);
    return false;
  }

  // Attempt to open the file
  FILE *file = fopen(path, mode_str);
  if (!file) {
    SERROR("Failed to open file: '%s'", path);
    return false;
  }

  out_handle->handle = file;
  out_handle->is_valid = true;

  return true;
}

void filesystem_close(file_handle *handle) {
  if (handle->handle) {
    fclose((FILE *)handle->handle);
    handle->handle = 0;
    handle->is_valid = false;
  }
}

b8 filesystem_read_line(file_handle *handle, char **line_buf) {
  if (!handle->is_valid || !handle->handle) {
    return false;
  }

  char buffer[32000];
  if (fgets(buffer, 32000, (FILE *)handle->handle) == 0) {
    return false;
  }

  u64 length = strlen(buffer);
  *line_buf = sallocate((sizeof(char) * length) + 1, MEMORY_TAG_STRING);
  strcpy(*line_buf, buffer);
  return true;
}

b8 filesystem_write_line(file_handle *handle, char const *text) {
  if (!handle->is_valid || !handle->handle) {
    return false;
  }

  i32 result = fputs(text, (FILE *)handle->handle);
  if (result != EOF) {
    result = fputc('\n', (FILE *)handle->handle);
  }

  fflush((FILE *)handle->handle);
  return result != EOF;
}

b8 filesystem_read(file_handle *handle, u64 data_size, void *out_data,
                   u64 *out_bytes_read) {
  if (!handle->is_valid || !handle->handle || !out_data) {
    return false;
  }

  *out_bytes_read = fread(out_data, 1, data_size, (FILE *)handle->handle);
  if (*out_bytes_read != data_size) {
    return false;
  }

  return true;
}

b8 filesystem_read_all_bytes(file_handle *handle, u8 **out_bytes,
                             u64 *out_bytes_read) {
  if (!handle->is_valid || !handle->handle) {
    return false;
  }

  fseek((FILE *)handle->handle, 0, SEEK_END);
  u64 size = (u64)ftell((FILE *)handle->handle);
  rewind((FILE *)handle->handle);

  *out_bytes = sallocate(sizeof(u8) * size, MEMORY_TAG_STRING);
  *out_bytes_read = fread(*out_bytes, 1, size, (FILE *)handle->handle);
  if (*out_bytes_read != size) {
    return false;
  }

  return true;
}

b8 filesystem_write(file_handle *handle, u64 data_size, void const *data,
                    u64 *out_bytes_written) {
  if (!handle->is_valid || !handle->handle) {
    return false;
  }

  *out_bytes_written = fwrite(data, 1, data_size, (FILE *)handle->handle);
  if (*out_bytes_written != data_size) {
    return false;
  }

  fflush((FILE *)handle->handle);
  return true;
}
