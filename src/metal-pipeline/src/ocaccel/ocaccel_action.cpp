#include <metal-pipeline/ocaccel/ocaccel_action.hpp>

#include <unistd.h>

#include <libosnap.h>
#include <osnap_hls_if.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>


#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

std::unique_ptr<FpgaAction> OCAccelActionFactory::createAction() const {
    return std::make_unique<OCAccelAction>(_card, _timeout);
}

OCAccelAction::OCAccelAction(int card, int timeout) : _timeout(timeout) {
  spdlog::trace("Allocating OCXL device /dev/ocxl/IBM,oc-snap.000{}:00:00.1.0", card);

  char device[128];
  snprintf(device, sizeof(device) - 1, "/dev/ocxl/IBM,oc-snap.000%d:00:00.1.0", card);

  _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
  if (!_card) {
    throw std::runtime_error("Failed to open card");
  }

  spdlog::trace("Attaching to Metal FS action...");

  auto action_irq =
      (snap_action_flag_t)(SNAP_ACTION_DONE_IRQ);
  _action = snap_attach_action(_card, fpga::ActionType, action_irq, 10);
  if (!_action) {
    snap_card_free(_card);
    _card = nullptr;
    throw std::runtime_error("Failed to attach action");
  }
  snap_action_assign_irq(_action, ACTION_IRQ_SRC_LO);
}

OCAccelAction::OCAccelAction(OCAccelAction &&other) noexcept
    : _action(other._action), _card(other._card), _timeout(other._timeout) {
  other._action = nullptr;
  other._card = nullptr;
}

OCAccelAction::~OCAccelAction() {
  if (_action) {
    spdlog::trace("Detaching action...");
    snap_detach_action(_action);
    _action = nullptr;
  }

  if (_card) {
    spdlog::trace("Deallocating CXL device...");
    snap_card_free(_card);
    _card = nullptr;
  }

  // Workaround for https://github.com/OpenCAPI/ocse/issues/9
  // Are we running an OCSE simulation?
  // cf. https://github.com/OpenCAPI/ocse/blob/9f0001603c08be058a9ca40344f420bfe0a61908/libocxl/libocxl.c#L1916
  if (getenv("OCSE_SERVER_DAT") || access("ocse_server.dat", F_OK) != -1) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}

void OCAccelAction::executeJob(fpga::JobType jobType, const void *parameters,
                            fpga::Address source, fpga::Address destination,
                            uint64_t directData0, uint64_t directData1,
                            uint64_t *directDataOut0,
                            uint64_t *directDataOut1) {
  spdlog::debug("Starting job {}...", jobTypeToString(jobType));

  fpga::Job mjob{};
  mjob.job_type = jobType;
  mjob.job_address = reinterpret_cast<uint64_t>(parameters);
  mjob.source = source;
  mjob.destination = destination;
  mjob.direct_data[0] = directData0;
  mjob.direct_data[1] = directData1;
  mjob.direct_data[2] = 0;
  mjob.direct_data[3] = 0;

  struct snap_job cjob {};
  snap_job_set(&cjob, &mjob, sizeof(mjob), nullptr, 0);

  int rc = snap_action_sync_execute_job(_action, &cjob, _timeout);

  if (rc != 0) {
    throw std::runtime_error("Error starting job: " +
                             snapReturnCodeToString(rc));
  }

  if (cjob.retc != SNAP_RETC_SUCCESS) {
    throw std::runtime_error("Job was unsuccessful");
  }

  if (directDataOut0) *directDataOut0 = mjob.direct_data[2];
  if (directDataOut1) *directDataOut1 = mjob.direct_data[3];
}

bool OCAccelAction::isNVMeEnabled() {
  unsigned long haveNVMe = 0;
  snap_card_ioctl(_card, GET_NVME_ENABLED, (unsigned long)&haveNVMe);
  return haveNVMe != 0;
}

void *OCAccelAction::allocateMemory(size_t size) { return snap_malloc(size); }

std::string OCAccelAction::jobTypeToString(fpga::JobType job) {
  switch (job) {
    case fpga::JobType::ReadImageInfo:
      return "ReadImageInfo";
    case fpga::JobType::Map:
      return "Map";
    case fpga::JobType::ConfigureStreams:
      return "ConfigureStreams";
    case fpga::JobType::ResetPerfmon:
      return "ResetPerfmon";
    case fpga::JobType::ConfigurePerfmon:
      return "ConfigurePerfmon";
    case fpga::JobType::ReadPerfmonCounters:
      return "ReadPerfmonCounters";
    case fpga::JobType::RunOperators:
      return "RunOperators";
    case fpga::JobType::ConfigureOperator:
      return "ConfigureOperator";
  }

  // The compiler should treat unhandled enum values as an error, so we should
  // never end up here
  return "Unknown";
}

std::string OCAccelAction::addressTypeToString(fpga::AddressType addressType) {
  switch (addressType) {
    case fpga::AddressType::Host:
      return "Host";
    case fpga::AddressType::CardDRAM:
      return "CardDRAM";
    case fpga::AddressType::NVMe:
      return "NVMe";
    case fpga::AddressType::Null:
      return "Null";
    case fpga::AddressType::Random:
      return "Random";
  }

  // The compiler should treat unhandled enum values as an error, so we should
  // never end up here
  return "Unknown";
}

std::string OCAccelAction::mapTypeToString(fpga::MapType mapType) {
  switch (mapType) {
    case fpga::MapType::None:
      return "None";
    case fpga::MapType::DRAM:
      return "DRAM";
    case fpga::MapType::NVMe:
      return "NVMe";
    case fpga::MapType::DRAMAndNVMe:
      return "DRAMAndNVMe";
  }

  // The compiler should treat unhandled enum values as an error, so we should
  // never end up here
  return "Unknown";
}

std::string OCAccelAction::snapReturnCodeToString(int rc) {
  switch (rc) {
    case SNAP_OK:
      return "";
    case SNAP_EBUSY:
      return "Resource is busy";
    case SNAP_ENODEV:
      return "No such device";
    case SNAP_EIO:
      return "Problem accessing the card";
    case SNAP_ENOENT:
      return "Entry not found";
    case SNAP_EFAULT:
      return "Illegal address";
    case SNAP_ETIMEDOUT:
      return "Timeout error";
    case SNAP_EINVAL:
      return "Invalid parameters";
    case SNAP_EATTACH:
      return "Attach error";
    case SNAP_EDETACH:
      return "Detach error";
    default:
      return "Unknown error";
  }
}

}  // namespace metal
