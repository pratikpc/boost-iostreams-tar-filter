#include <boost-iostreams-tar-filter/tar-filter.hxx>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <ios>
#include <istream>
#include <picosha2.h>
#include <string>
#include <vector>

namespace io = boost::iostreams;
namespace fs = std::filesystem;

/**
 * @brief Compute the SHA-256 digest of data read from a stream.
 *
 * The function reads from the current stream position until EOF and computes
 * the SHA-256 hash. The stream state will be advanced to EOF.
 *
 * @param stream Input stream to hash (read until EOF).
 * @return std::string Hex-encoded SHA-256 digest.
 */
static std::string sha256sum(std::istream &stream) {
  std::vector<std::uint8_t> hash(picosha2::k_digest_size);
  picosha2::hash256(std::istreambuf_iterator<char>(stream),
                    std::istreambuf_iterator<char>{}, hash.begin(), hash.end());
  return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

/**
 * @brief Parameters for a single TAR test case.
 *
 * - name: relative path to the test archive under tests/assets
 * - hash: expected SHA-256 digest of the extracted tarred data after
 * decompression
 * - tar_filter_buffer_sizes: sizes to instantiate the TarFilter with (exercises
 * buffer behavior)
 */
struct TarHashTestCase {
  std::string name;
  std::string hash;
  std::vector<std::streamsize> tar_filter_buffer_sizes;
};

/**
 * @brief Parameterized test fixture that computes hashes for various TAR
 * archives.
 *
 * The fixture reads a compressed TAR archive through TarFilter and
 * gzip_decompressor and compares the SHA-256 of the expanded stream against the
 * expected digest.
 */
class TarFilterHashTest : public ::testing::TestWithParam<TarHashTestCase> {};

/**
 * @brief Verify the SHA-256 of the decompressed TAR contents matches the
 * expected value.
 *
 * The test iterates over each buffer size specified in the test case to
 * exercise different internal buffer sizes in the TarFilter implementation.
 */
TEST_P(TarFilterHashTest, MatchesExpectedSHA256) {
  const auto &[relative_path, expected_hash, buffer_sizes] = GetParam();

  const auto file_path =
      fs::path(__FILE__).parent_path() / "assets" / relative_path;
  ASSERT_TRUE(fs::exists(file_path));
  for (const auto buffer_size : buffer_sizes) {
    io::filtering_istream in;
    in.push(boost_iostreams_tar_filter::TarFilter<>(buffer_size));
    in.push(io::gzip_decompressor());
    in.push(io::file_source(file_path, std::ios::binary));

    const auto hash = sha256sum(in);
    EXPECT_EQ(hash, expected_hash)
        << "Expected " << expected_hash << " but got " << hash;
  }
}

// -----------------------------
// 🧪 Test Case Definitions
// -----------------------------

INSTANTIATE_TEST_SUITE_P(
    TarFilterTests, TarFilterHashTest,
    ::testing::Values(
        TarHashTestCase{
            .name = "single-file.tar.gz",
            .hash = "1287bc72267f1a15a010b654ee725e52df1ef866fdaf056"
                    "748f7251845af832e",
            .tar_filter_buffer_sizes =
                {boost::iostreams::default_device_buffer_size, 16'384, 1}},
        TarHashTestCase{
            .name = "multi-file-multi-level.tar.gz",
            .hash = "654f82d44cf8b33242a34f8d03f4e68fca0859993259a0a9c000e30b52"
                    "d05b86",
            .tar_filter_buffer_sizes = {
                boost::iostreams::default_device_buffer_size, 16'384, 1}}));
