#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <metal-filesystem/metal.h>
#include <metal-pipeline/data_source_context.hpp>

namespace metal {

class FilesystemContext;

class METAL_FILESYSTEM_PIPELINE_API FileDataSourceContext
    : public DefaultDataSourceContext {
  // Common API
 public:
  bool endOfInput() const override;

 protected:
  void configure(SnapAction &action, bool initial) override;
  void finalize(SnapAction &action) override;

  uint64_t _fileLength;
  std::vector<mtl_file_extent> _extents;

  // API to be used from PipelineStorage (extent list-based)
 public:
  explicit FileDataSourceContext(fpga::AddressType resource, fpga::MapType map,
                                 std::vector<mtl_file_extent> &extents,
                                 uint64_t offset, uint64_t size);

  // API to be used when building file pipelines (filename-based)
 public:
  explicit FileDataSourceContext(std::shared_ptr<FilesystemContext> filesystem,
                                 std::string filename, uint64_t offset,
                                 uint64_t size = 0);
  uint64_t reportTotalSize();

 protected:
  uint64_t loadExtents();

  std::string _filename;
  std::shared_ptr<FilesystemContext> _filesystem;
};

}  // namespace metal
