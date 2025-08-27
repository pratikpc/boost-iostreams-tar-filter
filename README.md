# boost-iostreams-tar-filter

## Usage

```cpp
#include <boost-iostreams-tar-filter/tar-filter.hxx>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

io::filtering_istream in;
in.push(boost_iostreams_tar_filter::TarFilter<>(buffer_size));
in.push(io::gzip_decompressor());
in.push(io::file_source("test.tar.gz", std::ios::binary));
```

As the filters go from a bottom up manner, TarFilter should be placed before decompression.