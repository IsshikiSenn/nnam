#if !defined(_WIN32)
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#elif defined(__linux__)
#define _GNU_SOURCE
#endif
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#endif
#endif

#define PAGE_SIZE (64 * 1024)
#define ENCODED_SIZE(length) (4 * (((length) + 2) / 3) + 1)
#define MAX_READ_SIZE (16 * 1024 * 1024)

typedef struct {
  int64_t size;
  int64_t modified;
  long nanoseconds;
} FileStamp;

typedef struct {
  FILE *file;
  FileStamp stamp;
  int64_t page_start;
  size_t page_length;
  uint8_t page[PAGE_SIZE];
  char encoded[ENCODED_SIZE(PAGE_SIZE)];
  char *large_encoded;
} NnamInput;

typedef struct {
  FILE *file;
} NnamOutput;

#if defined(_WIN32)
static wchar_t *wide_path(const char *path) {
  int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
  if (length == 0) return NULL;
  wchar_t *wide = malloc((size_t)length * sizeof(*wide));
  if (wide && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, length)) {
    free(wide);
    return NULL;
  }
  return wide;
}
#endif

static bool file_stamp(FILE *file, FileStamp *stamp) {
#if defined(_WIN32)
  intptr_t handle = _get_osfhandle(_fileno(file));
  BY_HANDLE_FILE_INFORMATION info;
  if (handle == -1 || GetFileType((HANDLE)handle) != FILE_TYPE_DISK ||
      !GetFileInformationByHandle((HANDLE)handle, &info) ||
      (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return false;
  uint64_t size = ((uint64_t)info.nFileSizeHigh << 32) | info.nFileSizeLow;
  if (size > INT64_MAX) return false;
  stamp->size = (int64_t)size;
  stamp->modified = info.ftLastWriteTime.dwHighDateTime;
  stamp->nanoseconds = info.ftLastWriteTime.dwLowDateTime;
#else
  struct stat info;
  if (fstat(fileno(file), &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 0) return false;
  stamp->size = info.st_size;
#if defined(__APPLE__)
  stamp->modified = info.st_mtimespec.tv_sec;
  stamp->nanoseconds = info.st_mtimespec.tv_nsec;
#else
  stamp->modified = info.st_mtim.tv_sec;
  stamp->nanoseconds = info.st_mtim.tv_nsec;
#endif
#endif
  return true;
}

NnamInput *nnam_input_open(const char *path) {
#if defined(_WIN32)
  wchar_t *wide = wide_path(path);
  FILE *file = wide ? _wfopen(wide, L"rb") : NULL;
  free(wide);
#else
  // Opening non-regular files such as FIFOs must not hold up a worker.
  int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) return NULL;
  FILE *file = fdopen(fd, "rb");
  if (!file) close(fd);
#endif
  if (!file) return NULL;
  FileStamp stamp;
  NnamInput *input = file_stamp(file, &stamp) ? calloc(1, sizeof(*input)) : NULL;
  if (!input) {
    fclose(file);
    return NULL;
  }
  input->file = file;
  input->stamp = stamp;
  return input;
}

void nnam_input_release(NnamInput *input) {
  fclose(input->file);
  free(input->large_encoded);
  free(input);
}

int64_t nnam_input_size(NnamInput *input) {
  return input->stamp.size;
}

bool nnam_input_unchanged(NnamInput *input) {
  FileStamp current;
  return file_stamp(input->file, &current) && current.size == input->stamp.size &&
    current.modified == input->stamp.modified && current.nanoseconds == input->stamp.nanoseconds;
}

static bool read_at(NnamInput *input, int64_t offset, uint8_t *bytes, size_t length) {
#if defined(_WIN32)
  if (_fseeki64(input->file, offset, SEEK_SET) != 0) return false;
#else
  if (fseeko(input->file, (off_t)offset, SEEK_SET) != 0) return false;
#endif
  return fread(bytes, 1, length, input->file) == length;
}

static bool valid_range(NnamInput *input, int64_t offset, int64_t length) {
  return offset >= 0 && offset <= input->stamp.size &&
    length >= 0 && length <= input->stamp.size - offset;
}

static const uint8_t *read_page(NnamInput *input, int64_t offset, size_t length) {
  int64_t index = offset - input->page_start;
  if (index >= 0 && (uint64_t)index <= input->page_length &&
      length <= input->page_length - (size_t)index) return input->page + index;

  int64_t start = offset - offset % PAGE_SIZE;
  if (length > PAGE_SIZE - (size_t)(offset - start)) start = offset;
  int64_t remaining = input->stamp.size - start;
  size_t count = remaining < PAGE_SIZE ? (size_t)remaining : PAGE_SIZE;
  input->page_length = 0;
  if (!read_at(input, start, input->page, count)) return NULL;
  input->page_start = start;
  input->page_length = count;
  return input->page + (offset - start);
}

int32_t nnam_input_byte(NnamInput *input, int64_t offset) {
  if (!valid_range(input, offset, 1)) return -1;
  const uint8_t *byte = read_page(input, offset, 1);
  return byte ? *byte : -1;
}

static void encode_base64(const uint8_t *data, size_t length, char *output) {
  static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (size_t i = 0; i < length; i += 3) {
    size_t remaining = length - i;
    uint32_t value = (uint32_t)data[i] << 16;
    if (remaining > 1) value |= (uint32_t)data[i + 1] << 8;
    if (remaining > 2) value |= data[i + 2];
    *output++ = alphabet[value >> 18];
    *output++ = alphabet[(value >> 12) & 63];
    *output++ = remaining > 1 ? alphabet[(value >> 6) & 63] : '=';
    *output++ = remaining > 2 ? alphabet[value & 63] : '=';
  }
  *output = '\0';
}

void nnam_input_clear_large_read(NnamInput *input) {
  free(input->large_encoded);
  input->large_encoded = NULL;
}

// The extern ABI can return a copied CString but cannot return bytes.
// Base64 carries arbitrary file bytes through that ABI without decoding the audio here.
const char *nnam_input_read(NnamInput *input, int64_t offset, int64_t length) {
  if (!valid_range(input, offset, length) || length > MAX_READ_SIZE) return NULL;
  size_t count = (size_t)length;
  if (count <= PAGE_SIZE) {
    const uint8_t *data = read_page(input, offset, count);
    if (!data) return NULL;
    encode_base64(data, count, input->encoded);
    return input->encoded;
  }
  nnam_input_clear_large_read(input);
  uint8_t *data = malloc(count);
  if (!data) return NULL;
  if (read_at(input, offset, data, count)) {
    input->large_encoded = malloc(ENCODED_SIZE(count));
    if (input->large_encoded) encode_base64(data, count, input->large_encoded);
  }
  free(data);
  return input->large_encoded;
}

NnamOutput *nnam_output_open(const char *path) {
  NnamOutput *output = malloc(sizeof(*output));
  if (!output) return NULL;
#if defined(_WIN32)
  wchar_t *wide = wide_path(path);
  int fd = wide ? _wopen(wide, _O_WRONLY | _O_BINARY | _O_CREAT | _O_EXCL | _O_NOINHERIT,
    _S_IREAD | _S_IWRITE) : -1;
  FILE *file = fd >= 0 ? _fdopen(fd, "wb") : NULL;
  if (!file && fd >= 0) {
    _close(fd);
    _wunlink(wide);
  }
  free(wide);
#else
  int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
  FILE *file = fd >= 0 ? fdopen(fd, "wb") : NULL;
  if (!file && fd >= 0) {
    close(fd);
    unlink(path);
  }
#endif
  if (!file) {
    free(output);
    return NULL;
  }
  output->file = file;
  return output;
}

bool nnam_output_close(NnamOutput *output) {
  FILE *file = output->file;
  output->file = NULL;
  return !file || fclose(file) == 0;
}

void nnam_output_release(NnamOutput *output) {
  nnam_output_close(output);
  free(output);
}

bool nnam_output_write(NnamOutput *output, const uint8_t *data, size_t length) {
  return output->file && (length == 0 || fwrite(data, 1, length, output->file) == length);
}

// Returns 0 on success, 1 when the destination is occupied and -1 on failure.
int32_t nnam_output_publish_new(const char *temporary, const char *destination) {
#if defined(_WIN32)
  wchar_t *from = wide_path(temporary);
  wchar_t *to = wide_path(destination);
  int32_t result = -1;
  if (from && to) {
    if (MoveFileExW(from, to, 0)) {
      result = 0;
    } else {
      DWORD error = GetLastError();
      if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS ||
          (error == ERROR_ACCESS_DENIED && GetFileAttributesW(to) != INVALID_FILE_ATTRIBUTES)) {
        result = 1;
      }
    }
  }
  free(from);
  free(to);
  return result;
#else
#if defined(__APPLE__) && defined(RENAME_EXCL)
  if (renamex_np(temporary, destination, RENAME_EXCL) == 0) return 0;
  if (errno == EEXIST || errno == EISDIR || errno == ENOTEMPTY) return 1;
  if (errno != ENOTSUP && errno != EINVAL) return -1;
#elif defined(SYS_renameat2)
  const unsigned int rename_noreplace = 1;
  if (syscall(SYS_renameat2, AT_FDCWD, temporary, AT_FDCWD, destination, rename_noreplace) == 0) return 0;
  if (errno == EEXIST || errno == EISDIR || errno == ENOTEMPTY) return 1;
  if (errno != ENOSYS && errno != ENOTSUP && errno != EINVAL) return -1;
#endif
  // Both paths are in the same directory. The caller removes the temporary name.
  if (link(temporary, destination) == 0) return 0;
  return errno == EEXIST ? 1 : -1;
#endif
}
