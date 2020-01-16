#pragma once

#include <metal-pipeline/metal-pipeline_api.h>
#include <metal-pipeline/abstract_operator.hpp>

#include <string>

#include <snap_action_metal.h>

namespace metal {

class METAL_PIPELINE_API DataSink : public AbstractOperator {
 public:
  explicit DataSink(uint64_t address, size_t size = 0)
      : _address(address), _size(size) {}

  std::string id() const override { return ""; }
  uint8_t internal_id() const override { return 0; }

  virtual void prepareForTotalProcessingSize(size_t size) { (void)size; }
  virtual void setSize(size_t size) { _size = size; }

  fpga::Address address() {
    return fpga::Address{_address, static_cast<uint32_t>(_size), addressType(),
                         mapType()};
  }

 protected:
  virtual fpga::AddressType addressType() = 0;
  virtual fpga::MapType mapType() { return fpga::MapType::None; }
  uint64_t _address;
  size_t _size;
};

class METAL_PIPELINE_API HostMemoryDataSink : public DataSink {
 public:
  explicit HostMemoryDataSink(void *destination, size_t size = 0)
      : DataSink(reinterpret_cast<uint64_t>(destination), size) {}
  void setDestination(void *destination) {
    _address = reinterpret_cast<uint64_t>(destination);
  }

 protected:
  fpga::AddressType addressType() override { return fpga::AddressType::Host; }
};

class METAL_PIPELINE_API CardMemoryDataSink : public DataSink {
 public:
  explicit CardMemoryDataSink(uint64_t address, size_t size = 0)
      : DataSink(address, size) {}
  void setAddress(uint64_t address) { _address = address; }

 protected:
  fpga::AddressType addressType() override {
    return fpga::AddressType::CardDRAM;
  }
};

class METAL_PIPELINE_API NullDataSink : public DataSink {
 public:
  explicit NullDataSink(size_t size = 0) : DataSink(0, size) {}

 protected:
  fpga::AddressType addressType() override { return fpga::AddressType::Null; }
};

}  // namespace metal
