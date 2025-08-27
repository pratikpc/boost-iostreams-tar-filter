#pragma once

#include "base-tar-filter-impl.hxx"
#include <memory>

namespace boost_iostreams_tar_filter::detail {
namespace {
/**
 * @brief TAR file streaming filter adapter templated on allocator/char type.
 *
 * TarFilterImpl is a thin adapter over BaseTarFilterImpl that allows the
 * filter to be used with different char-like types provided by Alloc.
 *
 * @tparam Alloc Allocator type whose value_type defines the char_type (defaults
 * to std::allocator<char>).
 */
template <typename Alloc = std::allocator<char>>
class TarFilterImpl : public BaseTarFilterImpl {
public:
  using char_type = typename Alloc::value_type;

  /**
   * @brief Construct a TarFilterImpl.
   *
   * The constructor forwards to the BaseTarFilterImpl default constructor.
   */
  TarFilterImpl() : BaseTarFilterImpl() {}

  /**
   * @brief Filter data from source to destination performing TAR parsing.
   *
   * This function casts the provided char_type pointers to plain char pointers,
   * delegates processing to BaseTarFilterImpl::filter, and then updates the
   * original pointer arguments to reflect consumed/produced bytes.
   *
   * @param src_begin Reference to the input buffer pointer; advanced as bytes
   * are consumed.
   * @param src_end One-past-end pointer of the input buffer.
   * @param dest_begin Reference to the output buffer pointer; advanced as bytes
   * are written.
   * @param dest_end One-past-end pointer of the output buffer.
   * @param flush Ignored; kept for API compatibility.
   * @return true when more data is expected or more output space may be
   * provided.
   * @return false when no further processing is necessary (archive done) or no
   * progress can be made.
   */
  bool filter(const char_type *&src_begin, const char_type *const src_end,
              char_type *&dest_begin, const char_type *const dest_end,
              bool flush) {
    // Cast pointers to char* and delegate to BaseTarFilterImpl
    auto src_b = reinterpret_cast<const char *>(src_begin);
    auto src_e = reinterpret_cast<const char *>(src_end);
    auto dest_b = reinterpret_cast<char *>(dest_begin);
    auto dest_e = reinterpret_cast<const char *>(dest_end);

    bool result =
        BaseTarFilterImpl::filter(src_b, src_e, dest_b, dest_e, flush);

    // Update pointers back to char_type*
    src_begin = reinterpret_cast<const char_type *>(src_b);
    dest_begin = reinterpret_cast<char_type *>(dest_b);

    return result;
  }

  /**
   * @brief Reset internal state by delegating to BaseTarFilterImpl::close.
   */
  void close() { BaseTarFilterImpl::close(); }
};
} // namespace
} // namespace boost_iostreams_tar_filter::detail