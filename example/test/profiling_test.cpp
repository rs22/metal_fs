#include <gtest/gtest.h>
#include <malloc.h>
#include <snap_action_metal.h>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_registry.hpp>
#include <metal-pipeline/pipeline_definition.hpp>
#include <metal-pipeline/pipeline_runner.hpp>
#include "base_test.hpp"

namespace metal {

using ProfilingPipeline = PipelineTest;

TEST_F(ProfilingPipeline, ProfileDecryptInMultistagePipeline) {
  uint64_t n_pages = 1;

  uint64_t n_bytes = n_pages * 4096;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  auto decrypt = try_get_operator("blowfish_decrypt");
  auto change_case = try_get_operator("changecase");
  auto encrypt = try_get_operator("blowfish_encrypt");
  if (!decrypt || !change_case || !encrypt) {
    GTEST_SKIP();
    return;
  }

  auto keyBuffer = std::make_shared<std::vector<char>>(16);
  {
    std::vector<char> key(
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF});
    memcpy(keyBuffer->data(), key.data(), key.size());
  }

  decrypt->setOption("key", keyBuffer);
  encrypt->setOption("key", keyBuffer);

  UserOperatorRuntimeContext decryptContext(std::move(*decrypt)),
      changeCaseContext(std::move(*change_case)),
      encryptContext(std::move(*encrypt));

  decryptContext.set_profiling_enabled(true);

  ProfilingPipelineRunner runner(0, std::move(decryptContext),
                                 std::move(changeCaseContext),
                                 std::move(encryptContext));
  ASSERT_NO_THROW(
      runner.run(DataSource(src, n_bytes), DataSink(dest, n_bytes)));
  //   std::cout << runner.formatProfilingResults();
}

TEST_F(ProfilingPipeline, ProfileChangecaseInMultistagePipeline) {
  uint64_t n_pages = 1;

  uint64_t n_bytes = n_pages * 4096;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  auto decrypt = try_get_operator("blowfish_decrypt");
  auto change_case = try_get_operator("changecase");
  auto encrypt = try_get_operator("blowfish_encrypt");
  if (!decrypt || !change_case || !encrypt) {
    GTEST_SKIP();
    return;
  }

  auto keyBuffer = std::make_shared<std::vector<char>>(16);
  {
    std::vector<char> key(
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF});
    memcpy(keyBuffer->data(), key.data(), key.size());
  }

  decrypt->setOption("key", keyBuffer);
  encrypt->setOption("key", keyBuffer);

  UserOperatorRuntimeContext decryptContext(std::move(*decrypt)),
      changeCaseContext(std::move(*change_case)),
      encryptContext(std::move(*encrypt));

  changeCaseContext.set_profiling_enabled(true);

  ProfilingPipelineRunner runner(0, std::move(decryptContext),
                                 std::move(changeCaseContext),
                                 std::move(encryptContext));
  ASSERT_NO_THROW(
      runner.run(DataSource(src, n_bytes), DataSink(dest, n_bytes)));
  //   std::cout << runner.formatProfilingResults();
}

TEST_F(ProfilingPipeline, ProfileEncryptInMultistagePipeline) {
  uint64_t n_pages = 1;

  uint64_t n_bytes = n_pages * 4096;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  auto decrypt = try_get_operator("blowfish_decrypt");
  auto change_case = try_get_operator("changecase");
  auto encrypt = try_get_operator("blowfish_encrypt");
  if (!decrypt || !change_case || !encrypt) {
    GTEST_SKIP();
    return;
  }

  auto keyBuffer = std::make_shared<std::vector<char>>(16);
  {
    std::vector<char> key(
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF});
    memcpy(keyBuffer->data(), key.data(), key.size());
  }

  decrypt->setOption("key", keyBuffer);
  encrypt->setOption("key", keyBuffer);

  UserOperatorRuntimeContext decryptContext(std::move(*decrypt)),
      changeCaseContext(std::move(*change_case)),
      encryptContext(std::move(*encrypt));

  encryptContext.set_profiling_enabled(true);

  ProfilingPipelineRunner runner(0, std::move(decryptContext),
                                 std::move(changeCaseContext),
                                 std::move(encryptContext));
  ASSERT_NO_THROW(
      runner.run(DataSource(src, n_bytes), DataSink(dest, n_bytes)));
  //   std::cout << runner.formatProfilingResults();
}

TEST_F(ProfilingPipeline, BenchmarkChangecase) {
  uint64_t n_pages = 1;
  uint64_t n_bytes = n_pages * 128;

  auto change_case = try_get_operator("changecase");
  if (!change_case) {
    GTEST_SKIP();
    return;
  }

  change_case->setOption("lowercase", false);

  UserOperatorRuntimeContext changeCaseContext(std::move(*change_case));
  changeCaseContext.set_profiling_enabled(true);

  ProfilingPipelineRunner runner(0, std::move(changeCaseContext));
  ASSERT_NO_THROW(runner.run(DataSource(0, n_bytes, fpga::AddressType::Random),
                             DataSink(0, n_bytes, fpga::AddressType::Null)));
  //   std::cout << runner.formatProfilingResults();
}

}  // namespace metal
