# Adds no_zstd flag for mcap

EXTRA_CXXFLAGS+=-DMCAP_COMPRESSION_NO_ZSTD
EXTRA_CFLAGS+=-DMCAP_COMPRESSION_NO_ZSTD