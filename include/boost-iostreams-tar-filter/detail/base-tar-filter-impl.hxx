#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace boost_iostreams_tar_filter::detail {
/**
 * @class BaseTarFilterImpl
 * @brief Core TAR parsing logic that operates on char buffers.
 *
 * This class implements a small state machine to parse TAR archives streamed
 * in 512-byte blocks. It is intentionally independent of any iostreams
 * interfaces so it can be tested and reused by templated adapter layers.
 */
class BaseTarFilterImpl {
public:
  /** @enum State Parsing states for the internal state machine. */
  enum class State { ReadHeader, ReadFileData, SkipPadding, Done };

  // Public data members are intentionally simple to make the implementation
  // easy to introspect and to allow callers to allocate buffers externally.

  std::vector<char>
      header_buffer; /**< @brief Buffer for accumulating a 512-byte header. */
  std::size_t header_bytes_read =
      0; /**< @brief Number of header bytes currently buffered. */
  std::size_t file_size_ =
      0; /**< @brief Size of the current file entry in bytes. */
  std::size_t file_bytes_read =
      0; /**< @brief Number of bytes of the current file already read. */
  std::size_t padding_bytes =
      0; /**< @brief Number of padding bytes after the file to align to 512. */
  std::size_t padding_bytes_skipped =
      0; /**< @brief Number of padding bytes already skipped. */
  State state = State::ReadHeader; /**< @brief Current state of the parser. */
  std::string current_file_name;   /**< @brief Name of the file currently being
                                      processed. */

  /**
   * @brief Construct a BaseTarFilterImpl and initialize internal state.
   */
  BaseTarFilterImpl();

  /**
   * @brief Process input TAR data and extract file contents to the destination
   * buffer.
   *
   * Implemented as a pull/push style function where both source and destination
   * pointers are advanced as bytes are consumed/produced. The function returns
   * false when there is no further work (archive finished) or when it cannot
   * make progress.
   *
   * @param src_begin Reference to beginning of source buffer; advanced by
   * consumed bytes.
   * @param src_end One-past-end pointer of source buffer.
   * @param dest_begin Reference to beginning of destination buffer; advanced by
   * written bytes.
   * @param dest_end One-past-end pointer of destination buffer.
   * @param flush Ignored; kept for API compatibility with filtering frameworks.
   * @return true when more input/output activity may be possible.
   * @return false when the archive is fully processed or no progress can be
   * made.
   */
  bool filter(const char *&src_begin, const char *const src_end,
              char *&dest_begin, const char *const dest_end, bool flush);

  /**
   * @brief Reset the parser to initial state for reuse.
   */
  void close();
};
} // namespace boost_iostreams_tar_filter::detail