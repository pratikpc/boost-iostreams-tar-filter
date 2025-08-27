/**
 * @file tar_filter.hpp
 * @brief Defines a TAR filter that extracts file content from a tar stream
 * using Boost.Iostreams.
 */

#pragma once

#include <boost-iostreams-tar-filter/detail/tar-filter-impl.hxx>
#include <boost/iostreams/filter/symmetric.hpp>

namespace boost_iostreams_tar_filter {
/**
 * @brief Boost.Iostreams-compatible symmetric filter that extracts file
 * contents from a TAR archive stream.
 *
 * This filter parses a TAR archive stream and outputs only the raw file data,
 * stripping out TAR headers and padding blocks. It can be composed in a
 * Boost.Iostreams filtering pipeline alongside other filters such as
 * compression decompressors.
 *
 * @tparam Alloc Allocator type for internal buffers (default:
 * std::allocator<char>)
 *
 * @code{.cpp}
 * #include <boost/iostreams/filtering_stream.hpp>
 * #include <boost/iostreams/filter/gzip.hpp>
 * #include "tar_filter.hpp" // This filter's header
 *
 * namespace io = boost::iostreams;
 *
 * int main() {
 *   io::filtering_istream in;
 *   // Push the TarFilter to extract file contents from the tar archive
 *   in.push(TarFilter<>());
 *   // Push gzip decompressor for .tar.gz files
 *   in.push(io::gzip_decompressor());
 *   // Push the source file stream (binary mode)
 *   in.push(io::file_source("a.tar.gz", std::ios::binary));
 *
 *   // Read extracted file contents from 'in' as a normal istream
 *   std::string file_contents((std::istreambuf_iterator<char>(in)),
 *                              std::istreambuf_iterator<char>());
 *
 *   // Process file_contents ...
 * }
 * @endcode
 *
 * @note The filter expects input to be a valid TAR archive stream. It only
 * outputs the concatenated file data, omitting headers and padding blocks.
 *
 * @note The filter is stateful and maintains internal parsing state, suitable
 * for use in streaming decompression pipelines.
 */
template <typename Alloc = std::allocator<char>>
struct TarFilter
    : boost::iostreams::symmetric_filter<detail::TarFilterImpl<Alloc>, Alloc> {
private:
  using impl_type = detail::TarFilterImpl<Alloc>;
  using base_type = boost::iostreams::symmetric_filter<impl_type, Alloc>;

public:
  /// Character type used by the stream.
  using char_type = typename base_type::char_type;
  /// Filter category for Boost.Iostreams.
  using category = typename base_type::category;

  /**
   * @brief Constructs the TAR filter with optional buffer size.
   *
   * @param buffer_size Buffer size used internally (defaults to
   * Boost.Iostreams' default size).
   */
  explicit TarFilter(std::streamsize buffer_size =
                         boost::iostreams::default_device_buffer_size)
      : base_type(buffer_size) {}
};

/// @brief Makes TarFilter pipable in Boost.Iostreams pipelines.
BOOST_IOSTREAMS_PIPABLE(TarFilter<>, 0);
} // namespace boost_iostreams_tar_filter