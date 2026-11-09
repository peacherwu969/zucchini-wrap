#ifndef ZUCCHINI_STANDALONE_H_
#define ZUCCHINI_STANDALONE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * These values intentionally preserve AOSP Zucchini's status codes 0..8.
 * The standalone wrapper adds 9 for an output verification mismatch.
 */
enum zucchini_status {
  ZUCCHINI_STATUS_SUCCESS = 0,
  ZUCCHINI_STATUS_INVALID_PARAM = 1,
  ZUCCHINI_STATUS_FILE_READ_ERROR = 2,
  ZUCCHINI_STATUS_FILE_WRITE_ERROR = 3,
  ZUCCHINI_STATUS_PATCH_READ_ERROR = 4,
  ZUCCHINI_STATUS_PATCH_WRITE_ERROR = 5,
  ZUCCHINI_STATUS_INVALID_OLD_IMAGE = 6,
  ZUCCHINI_STATUS_INVALID_NEW_IMAGE = 7,
  ZUCCHINI_STATUS_FATAL = 8,
  ZUCCHINI_STATUS_VERIFY_MISMATCH = 9
};

/* Generate an executable-aware Zucchini patch. */
int zucchini_generate_file(const char* old_path,
                            const char* new_path,
                            const char* patch_path);

/*
 * Generate an executable-aware patch with optional first-pass early
 * finalization. Early finalization occurs only when both limits are nonzero,
 * the equivalence count reaches |equivalence_limit|, and
 * count * extra_bytes / total_bytes reaches |fragmentation_limit|.
 */
int zucchini_generate_file_with_early_finalize_limits(
    const char* old_path,
    const char* new_path,
    const char* patch_path,
    uint32_t equivalence_limit,
    uint32_t fragmentation_limit);

/*
 * Generate with all optional controls. |equivalence_iterations| is a cap:
 * 0 preserves the executable format's normal refinement count, while a
 * positive value runs at most that many passes. It never forces extra passes.
 */
int zucchini_generate_file_with_options(
    const char* old_path,
    const char* new_path,
    const char* patch_path,
    uint32_t equivalence_limit,
    uint32_t fragmentation_limit,
    uint32_t equivalence_iterations);

/*
 * Same controls as zucchini_generate_file_with_options(), plus the adaptive
 * pass-2 map gate. When enabled on a normal four-pass executable format,
 * pass 2 is emitted when its equivalence count is at least 40000 and its
 * fragmentation score is at least 15000. Two-pass formats are unchanged.
 */
int zucchini_generate_file_with_adaptive_options(
    const char* old_path,
    const char* new_path,
    const char* patch_path,
    uint32_t equivalence_limit,
    uint32_t fragmentation_limit,
    uint32_t equivalence_iterations,
    int adaptive_equivalence);

/* Generate a raw Zucchini patch without executable disassembly. */
int zucchini_generate_raw_file(const char* old_path,
                                const char* new_path,
                                const char* patch_path);

/* Apply a patch and write the reconstructed new file. */
int zucchini_apply_file(const char* old_path,
                         const char* patch_path,
                         const char* new_path);

/* Apply a patch in memory and compare it with an expected new file. */
int zucchini_verify_file(const char* old_path,
                          const char* patch_path,
                          const char* expected_new_path);

/* Human-readable description of a status code. */
const char* zucchini_status_string(int status);

/* More detailed text for the most recent call on the current thread. */
const char* zucchini_last_error(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ZUCCHINI_STANDALONE_H_ */
