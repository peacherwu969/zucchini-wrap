#include "zucchini_standalone.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "components/zucchini/patch_reader.h"
#include "components/zucchini/patch_writer.h"
#include "components/zucchini/zucchini.h"

namespace {

thread_local std::string g_last_error;

using ByteVector = std::vector<uint8_t>;

void SetError(std::string message) {
  g_last_error = std::move(message);
}

zucchini::ConstBufferView ConstView(const ByteVector& data) {
  static const uint8_t empty_byte = 0;
  const uint8_t* pointer = data.empty() ? &empty_byte : data.data();
  return zucchini::ConstBufferView(pointer, data.size());
}

zucchini::MutableBufferView MutableView(ByteVector& data) {
  static uint8_t empty_byte = 0;
  uint8_t* pointer = data.empty() ? &empty_byte : data.data();
  return zucchini::MutableBufferView(pointer, data.size());
}

bool IsValidPath(const char* path) {
  return path != nullptr && path[0] != '\0';
}

int ReadFile(const char* path, ByteVector* output) {
  if (!IsValidPath(path) || output == nullptr) {
    SetError("invalid file path");
    return ZUCCHINI_STATUS_INVALID_PARAM;
  }

  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    SetError(std::string("cannot open for reading: ") + path);
    return ZUCCHINI_STATUS_FILE_READ_ERROR;
  }

  const std::streamoff end = input.tellg();
  if (end < 0) {
    SetError(std::string("cannot determine file size: ") + path);
    return ZUCCHINI_STATUS_FILE_READ_ERROR;
  }

  const auto size = static_cast<uint64_t>(end);
  if (size > std::numeric_limits<uint32_t>::max()) {
    SetError(std::string("file exceeds Zucchini's 32-bit format limit: ") + path);
    return ZUCCHINI_STATUS_INVALID_PARAM;
  }

  output->resize(static_cast<size_t>(size));
  input.seekg(0, std::ios::beg);
  if (size != 0 &&
      !input.read(reinterpret_cast<char*>(output->data()),
                  static_cast<std::streamsize>(size))) {
    SetError(std::string("short read: ") + path);
    return ZUCCHINI_STATUS_FILE_READ_ERROR;
  }
  return ZUCCHINI_STATUS_SUCCESS;
}

int WriteFile(const char* path, const ByteVector& data) {
  if (!IsValidPath(path)) {
    SetError("invalid output file path");
    return ZUCCHINI_STATUS_INVALID_PARAM;
  }

  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    SetError(std::string("cannot open for writing: ") + path);
    return ZUCCHINI_STATUS_FILE_WRITE_ERROR;
  }

  if (!data.empty()) {
    output.write(reinterpret_cast<const char*>(data.data()),
                 static_cast<std::streamsize>(data.size()));
  }
  if (!output) {
    SetError(std::string("write failed: ") + path);
    return ZUCCHINI_STATUS_FILE_WRITE_ERROR;
  }
  return ZUCCHINI_STATUS_SUCCESS;
}

int Generate(const char* old_path,
             const char* new_path,
             const char* patch_path,
             bool raw,
             uint32_t equivalence_limit,
             uint32_t fragmentation_limit,
             uint32_t equivalence_iterations,
             bool adaptive_equivalence) {
  g_last_error.clear();

  ByteVector old_image;
  ByteVector new_image;
  int status = ReadFile(old_path, &old_image);
  if (status != ZUCCHINI_STATUS_SUCCESS)
    return status;
  status = ReadFile(new_path, &new_image);
  if (status != ZUCCHINI_STATUS_SUCCESS)
    return status;

  zucchini::EnsemblePatchWriter writer(ConstView(old_image),
                                        ConstView(new_image));
  zucchini::status::Code code =
      raw ? zucchini::GenerateBufferRaw(ConstView(old_image),
                                        ConstView(new_image), &writer)
          : zucchini::GenerateBuffer(ConstView(old_image),
                                     ConstView(new_image), equivalence_limit,
                                     fragmentation_limit,
                                     equivalence_iterations,
                                     adaptive_equivalence, &writer);
  if (code != zucchini::status::kStatusSuccess) {
    SetError(raw ? "raw patch generation failed"
                 : "executable-aware patch generation failed");
    return static_cast<int>(code);
  }

  ByteVector patch(writer.SerializedSize());
  if (!writer.SerializeInto(MutableView(patch))) {
    SetError("patch serialization failed");
    return ZUCCHINI_STATUS_PATCH_WRITE_ERROR;
  }

  return WriteFile(patch_path, patch);
}

int ApplyToMemory(const char* old_path,
                  const char* patch_path,
                  ByteVector* new_image) {
  ByteVector old_image;
  ByteVector patch;

  int status = ReadFile(old_path, &old_image);
  if (status != ZUCCHINI_STATUS_SUCCESS)
    return status;
  status = ReadFile(patch_path, &patch);
  if (status != ZUCCHINI_STATUS_SUCCESS)
    return status;

  auto reader = zucchini::EnsemblePatchReader::Create(ConstView(patch));
  if (!reader.has_value()) {
    SetError("invalid or truncated Zucchini patch");
    return ZUCCHINI_STATUS_PATCH_READ_ERROR;
  }

  new_image->assign(reader->header().new_size, 0);
  const zucchini::status::Code code =
      zucchini::ApplyBuffer(ConstView(old_image), *reader,
                            MutableView(*new_image));
  if (code != zucchini::status::kStatusSuccess) {
    SetError("patch application failed");
    return static_cast<int>(code);
  }
  return ZUCCHINI_STATUS_SUCCESS;
}

}  // namespace

extern "C" int zucchini_generate_file(const char* old_path,
                                        const char* new_path,
                                        const char* patch_path) {
  return zucchini_generate_file_with_options(
      old_path, new_path, patch_path, 0, 0, 0);
}

extern "C" int zucchini_generate_file_with_early_finalize_limits(
    const char* old_path,
    const char* new_path,
    const char* patch_path,
    uint32_t equivalence_limit,
    uint32_t fragmentation_limit) {
  return zucchini_generate_file_with_options(
      old_path, new_path, patch_path, equivalence_limit, fragmentation_limit,
      0);
}

extern "C" int zucchini_generate_file_with_options(
    const char* old_path,
    const char* new_path,
    const char* patch_path,
    uint32_t equivalence_limit,
    uint32_t fragmentation_limit,
    uint32_t equivalence_iterations) {
  return Generate(old_path, new_path, patch_path, false, equivalence_limit,
                  fragmentation_limit, equivalence_iterations, false);
}

extern "C" int zucchini_generate_file_with_adaptive_options(
    const char* old_path,
    const char* new_path,
    const char* patch_path,
    uint32_t equivalence_limit,
    uint32_t fragmentation_limit,
    uint32_t equivalence_iterations,
    int adaptive_equivalence) {
  return Generate(old_path, new_path, patch_path, false, equivalence_limit,
                  fragmentation_limit, equivalence_iterations,
                  adaptive_equivalence != 0);
}

extern "C" int zucchini_generate_raw_file(const char* old_path,
                                            const char* new_path,
                                            const char* patch_path) {
  return Generate(old_path, new_path, patch_path, true, 0, 0, 0, false);
}

extern "C" int zucchini_apply_file(const char* old_path,
                                     const char* patch_path,
                                     const char* new_path) {
  g_last_error.clear();
  ByteVector new_image;
  const int status = ApplyToMemory(old_path, patch_path, &new_image);
  if (status != ZUCCHINI_STATUS_SUCCESS)
    return status;
  return WriteFile(new_path, new_image);
}

extern "C" int zucchini_verify_file(const char* old_path,
                                      const char* patch_path,
                                      const char* expected_new_path) {
  g_last_error.clear();
  ByteVector reconstructed;
  int status = ApplyToMemory(old_path, patch_path, &reconstructed);
  if (status != ZUCCHINI_STATUS_SUCCESS)
    return status;

  ByteVector expected;
  status = ReadFile(expected_new_path, &expected);
  if (status != ZUCCHINI_STATUS_SUCCESS)
    return status;

  if (reconstructed != expected) {
    SetError("reconstructed file differs from expected new file");
    return ZUCCHINI_STATUS_VERIFY_MISMATCH;
  }
  return ZUCCHINI_STATUS_SUCCESS;
}

extern "C" const char* zucchini_status_string(int status) {
  switch (status) {
    case ZUCCHINI_STATUS_SUCCESS:
      return "success";
    case ZUCCHINI_STATUS_INVALID_PARAM:
      return "invalid parameter";
    case ZUCCHINI_STATUS_FILE_READ_ERROR:
      return "file read error";
    case ZUCCHINI_STATUS_FILE_WRITE_ERROR:
      return "file write error";
    case ZUCCHINI_STATUS_PATCH_READ_ERROR:
      return "patch read error";
    case ZUCCHINI_STATUS_PATCH_WRITE_ERROR:
      return "patch write error";
    case ZUCCHINI_STATUS_INVALID_OLD_IMAGE:
      return "old image does not match patch";
    case ZUCCHINI_STATUS_INVALID_NEW_IMAGE:
      return "reconstructed image failed CRC/size validation";
    case ZUCCHINI_STATUS_FATAL:
      return "Zucchini internal failure";
    case ZUCCHINI_STATUS_VERIFY_MISMATCH:
      return "verification mismatch";
    default:
      return "unknown status";
  }
}

extern "C" const char* zucchini_last_error(void) {
  return g_last_error.c_str();
}
