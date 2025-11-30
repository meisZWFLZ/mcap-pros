Pros package for mcap using flatbuffers

Compression algorithms:
- lz4
- Doesn't include zstd

# To Build:
1. Ensure `cmake` and `pros-cli` are installed
2. Run `make gen_schema_code`
  - This will install the `flatc` command in `~/.local/bin` 
3. Run `flatc --binary --schema -o ./static.lib/foxglove -I build/foxglove-sdk/schemas/flatbuffer build/foxglove-sdk/schemas/flatbuffer/*.fbs`
4. Run `pros make template`