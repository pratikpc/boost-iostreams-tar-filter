#include <boost-iostreams-tar-filter/detail/base-tar-filter-impl.hxx>
#include <boost-iostreams-tar-filter/detail/tar-header.hxx>
#include <cstdint>
#include <cstring>
#include <string>

namespace boost_iostreams_tar_filter::detail {
namespace {

/**
 * @brief Check whether a 512-byte TAR block is entirely zeros.
 *
 * TAR archives are terminated by at least two consecutive 512-byte blocks
 * of zero. This helper tests a single 512-byte block for that condition.
 *
 * @param block Pointer to a 512-byte memory region to inspect.
 * @return true when every byte in the block is '\0'.
 * @return false otherwise.
 */
bool is_zero_block_impl(const char *block) {
  for (std::size_t i = 0; i < 512; ++i)
    if (block[i] != '\0')
      return false;
  return true;
}

/**
 * @brief Parse a base-256 encoded integer from a TAR header field.
 *
 * GNU tar and newer POSIX extensions allow storing file sizes and other
 * numeric fields using a base-256 (binary) representation when values do
 * not fit into the traditional octal ASCII field. This routine implements
 * the decoding of that representation into a signed 64-bit integer.
 *
 * The function handles variable-length two's complement values encoded in
 * big-endian order and performs overflow checks against the int64_t range.
 *
 * @param _p Pointer to the first byte of the base-256 field.
 * @param char_cnt Number of bytes available in the field.
 * @return int64_t Decoded signed integer; returns INT64_MIN/INT64_MAX on
 * overflow.
 */
int64_t parse_base256_impl(const char *_p, std::size_t char_cnt) {
  uint64_t l;
  auto p = reinterpret_cast<const unsigned char *>(_p);
  auto c = *p;
  unsigned char neg;

  if (c & 0x40) {
    neg = 0xff;
    c |= 0x80;
    l = ~uint64_t(0);
  } else {
    neg = 0;
    c &= 0x7f;
    l = 0;
  }

  while (char_cnt > sizeof(int64_t)) {
    --char_cnt;
    if (c != neg)
      return neg ? INT64_MIN : INT64_MAX;
    c = *++p;
  }

  if ((c ^ neg) & 0x80)
    return neg ? INT64_MIN : INT64_MAX;

  while (--char_cnt > 0) {
    l = (l << 8) | c;
    c = *++p;
  }
  l = (l << 8) | c;
  return static_cast<int64_t>(l);
}

/**
 * @brief Parse an octal ASCII integer from a TAR header field.
 *
 * Traditional TAR headers encode many numeric fields as ASCII octal text.
 * This helper parses such a field stopping at the first NUL or after 'n' bytes.
 *
 * @param p Pointer to the first byte of the octal ASCII field.
 * @param n Number of bytes to inspect.
 * @return std::size_t Parsed (non-negative) integer value.
 */
std::size_t parse_octal_impl(const char *p, std::size_t n) {
  std::size_t result = 0;
  for (std::size_t i = 0; i < n && p[i]; ++i)
    if (p[i] >= '0' && p[i] <= '7')
      result = (result << 3) + (p[i] - '0');
  return result;
}

/**
 * @brief Extract file size from a TarHeader, handling octal and base-256
 * encodings.
 *
 * The function inspects the first byte to determine whether the size uses
 * the base-256 (binary) encoding (high bit set) or the traditional octal text.
 *
 * @param tar Pointer to the parsed TarHeader.
 * @return std::size_t File size in bytes.
 */
std::size_t parse_file_size_impl(const TarHeader *tar) {
  if (static_cast<unsigned char>(tar->size[0]) & 0x80)
    return static_cast<std::size_t>(
        parse_base256_impl(tar->size, sizeof(tar->size)));
  else
    return parse_octal_impl(tar->size, sizeof(tar->size));
}

/**
 * @brief Extract a possibly non-null-terminated name field from the header.
 *
 * File name fields in the TAR header may not occupy all 100 bytes and are
 * not guaranteed to be NUL-terminated if the full length is used. This
 * helper returns a std::string constructed from the name bytes up to the
 * first NUL or the full length.
 *
 * @param tar Pointer to the TarHeader.
 * @return std::string The file name as a std::string.
 */
std::string extract_file_name_impl(const TarHeader *tar) {
  const char *start = tar->name;
  std::size_t len = 0;
  for (; len < sizeof(tar->name); ++len)
    if (start[len] == '\0')
      break;
  return std::string(start, len);
}

/**
 * @brief Check whether a header entry represents a regular file.
 *
 * According to the TAR standard, a typeflag of '0' or a NUL indicates
 * a regular file entry. Other typeflags represent directories, symlinks, etc.
 *
 * @param tar Pointer to the TarHeader.
 * @return true if the entry is a regular file, false otherwise.
 */
inline bool is_regular_file(const TarHeader *tar) {
  return tar->typeflag[0] == '0' || tar->typeflag[0] == '\0';
}

} // unnamed namespace

/**
 * @brief Construct a BaseTarFilterImpl and initialize state.
 *
 * Use this constructor to create or reset the internal state prior to feeding
 * TAR stream data into filter().
 */
BaseTarFilterImpl::BaseTarFilterImpl() {}

/**
 * @brief Main streaming filter: read headers, extract file data and skip
 * padding.
 *
 * This function implements a small state machine that:
 *  - reads 512-byte TAR headers,
 *  - determines file size and type,
 *  - copies file payload bytes into the destination buffer,
 *  - skips file padding to align to 512-byte blocks,
 *  - recognizes archive termination (two zero blocks).
 *
 * The function is designed for use in push-style filtering where src_begin is
 * advanced as bytes are consumed and dest_begin advanced as bytes are produced.
 *
 * @param src_begin Reference to the start pointer of the source buffer;
 * advanced by the number of bytes consumed.
 * @param src_end Pointer to one-past-the-end of the source buffer.
 * @param dest_begin Reference to the start pointer of the destination buffer;
 *                   advanced by the number of bytes written.
 * @param dest_end Pointer to one-past-the-end of the destination buffer.
 * @param flush Ignored; present for API compatibility with Boost.Iostreams
 * filter.
 * @return true when the caller may supply more input or more output space is
 * expected.
 * @return false to indicate either end-of-archive (done) or no progress can be
 * made.
 */
bool BaseTarFilterImpl::filter(const char *&src_begin,
                               const char *const src_end, char *&dest_begin,
                               const char *const dest_end, bool /*flush*/) {
  while (src_begin < src_end && dest_begin < dest_end) {
    switch (state) {
    case State::ReadHeader: {
      auto needed = 512 - header_bytes_read;
      auto available = static_cast<std::size_t>(src_end - src_begin);
      auto to_copy = std::min(needed, available);

      if (header_buffer.size() < 512)
        header_buffer.resize(512);
      std::memcpy(&header_buffer[header_bytes_read], src_begin,
                  to_copy * sizeof(char));
      src_begin += to_copy;
      header_bytes_read += to_copy;

      if (header_bytes_read == 512) {
        if (is_zero_block_impl(header_buffer.data())) {
          state = State::Done;
          return false;
        }
        auto tar = reinterpret_cast<const TarHeader *>(header_buffer.data());
        file_size_ = parse_file_size_impl(tar);
        current_file_name = extract_file_name_impl(tar);
        file_bytes_read = 0;
        padding_bytes = (512 - (file_size_ % 512)) % 512;
        padding_bytes_skipped = 0;

        if (is_regular_file(tar)) {
          state = State::ReadFileData;
        } else {
          state = State::SkipPadding;
          file_size_ = 0;
        }
        header_bytes_read = 0;
      }
      break;
    }

    case State::ReadFileData: {
      auto remaining = file_size_ - file_bytes_read;
      auto src_avail = static_cast<std::size_t>(src_end - src_begin);
      auto dest_space = static_cast<std::size_t>(dest_end - dest_begin);

      auto const to_copy = std::min(remaining, std::min(src_avail, dest_space));
      std::copy(src_begin, src_begin + to_copy, dest_begin);

      src_begin += to_copy;
      dest_begin += to_copy;
      file_bytes_read += to_copy;

      if (file_bytes_read == file_size_)
        state = State::SkipPadding;
      break;
    }

    case State::SkipPadding: {
      auto remaining = padding_bytes - padding_bytes_skipped;
      auto src_avail = static_cast<std::size_t>(src_end - src_begin);
      auto to_skip = std::min(remaining, src_avail);

      src_begin += to_skip;
      padding_bytes_skipped += to_skip;

      if (padding_bytes_skipped == padding_bytes)
        state = State::ReadHeader;
      break;
    }

    case State::Done:
      return false;
    }
  }

  return true;
}

/**
 * @brief Reset internal parser state so the filter can be reused.
 *
 * Clears any buffered partial header data, resets counters and the current
 * filename. After close() the filter behaves as if newly constructed.
 */
void BaseTarFilterImpl::close() {
  state = State::ReadHeader;
  header_bytes_read = 0;
  file_bytes_read = 0;
  padding_bytes_skipped = 0;
  file_size_ = 0;
  header_buffer.clear();
  current_file_name.clear();
}
} // namespace boost_iostreams_tar_filter::detail