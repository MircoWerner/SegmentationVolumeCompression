#pragma once

#include "DAG.h"
#include "DAGGPUPassCompaction.h"
#include "DAGGPUPassPrefixSum.h"
#include "DAGGPUPassPrefixSumOffset.h"
#include "DAGGPUPassPreparation.h"
#include "DAGGPUPassRoots.h"
#include "DAGGPUPassSort.h"
#include "DAGGPUPassUnique.h"
#include "raven/core/GPUContext.h"
#include "raven/passes/PassCompute.h"

#include <vector>

namespace raven {
    class DAGGPU {
    public:
        DAGGPU(GPUContext *gpuContext, const uint32_t dagRootSize, const std::vector<DAG::DAGLevel> &dagLevels) : m_gpuContext(gpuContext), m_dagRootSize(dagRootSize), m_dagLevels(dagLevels) {
        }

        void create(std::vector<uint32_t> &dagRoot, std::vector<DAG::DAGNode> &dag) {
            assert(dagRoot.size() == m_dagRootSize);

            for (uint32_t level = 0; level < m_dagLevels.size(); level++) {
                if (m_dagLevels[level].count <= 1) {
                    // no sort required
                    continue;
                }

                uint32_t n = nextPowerOfTwo(m_dagLevels[level].count);

                uint32_t workgroup_size_x;
                if (uint32_t max_workgroup_size = 512; n < max_workgroup_size * 2) {
                    workgroup_size_x = n / 2;
                } else {
                    workgroup_size_x = max_workgroup_size;
                }

                const uint32_t workgroup_count = n / (workgroup_size_x * 2);

                m_sortN.push_back(n);
                m_sortWorkGroupSizeX.push_back(workgroup_size_x);
                m_sortWorkGroupCount.push_back(workgroup_count);
            }

            // compute passes
            m_computePass = std::make_shared<PassCompute>(m_gpuContext, Pass::PassSettings{.m_name = "m_computePass"});
            m_computePass->create();

            // compute shaders
            m_passPreparation = std::make_shared<DAGGPUPassPreparation>(m_gpuContext);
            m_passPreparation->create();
            for (uint32_t level = 0; level < m_dagLevels.size(); level++) {
                if (m_dagLevels[level].count <= 1) {
                    // no sort required
                    continue;
                }
                m_passSort.push_back(std::make_shared<DAGGPUPassSort>(m_gpuContext, m_sortWorkGroupSizeX[level]));
                m_passSort.back()->create();
            }
            m_passUnique = std::make_shared<DAGGPUPassUnique>(m_gpuContext);
            m_passUnique->create();
            m_passPrefixSum = std::make_shared<DAGGPUPassPrefixSum>(m_gpuContext);
            m_passPrefixSum->create();
            m_passPrefixSumOffset = std::make_shared<DAGGPUPassPrefixSumOffset>(m_gpuContext);
            m_passPrefixSumOffset->create();
            m_passCompaction = std::make_shared<DAGGPUPassCompaction>(m_gpuContext);
            m_passCompaction->create();
            m_passRoots = std::make_shared<DAGGPUPassRoots>(m_gpuContext);
            m_passRoots->create();

            // buffers
            {
                const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(dag.size() * sizeof(DAG::DAGNode)), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "dag"};
                m_dagBuffer = Buffer::fillDeviceWithStagingBuffer(m_gpuContext, bufferSettings, dag.data());
            }
            {
                const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(m_dagLevels.size() * sizeof(DAG::DAGLevel)), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "dag_levels"};
                m_dagLevelBuffer = std::make_shared<Buffer>(m_gpuContext, bufferSettings);
            }
            {
                const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(dag.size() * sizeof(uint32_t)), .m_bufferUsages = vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "storage[0]"};
                m_buffers.emplace_back(std::make_shared<Buffer>(m_gpuContext, bufferSettings));
            }
            {
                constexpr uint WORKGROUP_SIZE = 256;

                std::vector<uint32_t> numOffsets;

                uint32_t currentNumElements = dag.size();
                uint32_t currentNumOffsets = PassShaderCompute::getDispatchSize(currentNumElements, 1, 1, vk::Extent3D{WORKGROUP_SIZE, 1, 1}).width;
                numOffsets.push_back(currentNumOffsets);

                while (currentNumElements > WORKGROUP_SIZE) {
                    currentNumElements = currentNumOffsets;
                    currentNumOffsets = PassShaderCompute::getDispatchSize(currentNumElements, 1, 1, vk::Extent3D{WORKGROUP_SIZE, 1, 1}).width;
                    numOffsets.push_back(currentNumOffsets);
                }

                for (uint32_t i = 0; i < numOffsets.size(); i++) {
                    std::string name = "storage[" + std::to_string(i + 1) + "]";
                    const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(numOffsets[i] * sizeof(uint32_t)), .m_bufferUsages = vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = name};
                    m_buffers.emplace_back(std::make_shared<Buffer>(m_gpuContext, bufferSettings));
                }
            }
            {
                const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(dag.size() * sizeof(uint32_t)), .m_bufferUsages = vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "indirection"};
                m_indirectionBuffer = std::make_shared<Buffer>(m_gpuContext, bufferSettings);
            }
            {
                const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(dag.size() * sizeof(uint32_t)), .m_bufferUsages = vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "index"};
                m_indexBuffer = std::make_shared<Buffer>(m_gpuContext, bufferSettings);
            }
            {
                const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(dagRoot.size() * sizeof(uint32_t)), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "dag_roots"};
                m_rootsBuffer = Buffer::fillDeviceWithStagingBuffer(m_gpuContext, bufferSettings, dagRoot.data());
            }

            {
                // print buffer sizes
                uint64_t totalSizeBytes = 0;
                std::cout << "dagBuffer.size = " << m_dagBuffer->getSizeBytes() << " bytes (" << static_cast<float>(m_dagBuffer->getSizeBytes()) * glm::pow(10, -6) << " MB)" << std::endl;
                totalSizeBytes += m_dagBuffer->getSizeBytes();
                std::cout << "dagLevelBuffer.size = " << m_dagLevelBuffer->getSizeBytes() << " bytes (" << static_cast<float>(m_dagLevelBuffer->getSizeBytes()) * glm::pow(10, -6) << " MB)" << std::endl;
                totalSizeBytes += m_dagLevelBuffer->getSizeBytes();
                for (uint32_t i = 0; i < m_buffers.size(); i++) {
                    std::cout << "buffers[" << i << "].size = " << m_buffers[i]->getSizeBytes() << " bytes (" << static_cast<float>(m_buffers[i]->getSizeBytes()) * glm::pow(10, -6) << " MB)" << std::endl;
                    totalSizeBytes += m_buffers[i]->getSizeBytes();
                }
                std::cout << "indirectionBuffer.size = " << m_indirectionBuffer->getSizeBytes() << " bytes (" << static_cast<float>(m_indirectionBuffer->getSizeBytes()) * glm::pow(10, -6) << " MB)" << std::endl;
                totalSizeBytes += m_indirectionBuffer->getSizeBytes();
                std::cout << "indexBuffer.size = " << m_indexBuffer->getSizeBytes() << " bytes (" << static_cast<float>(m_indexBuffer->getSizeBytes()) * glm::pow(10, -6) << " MB)" << std::endl;
                totalSizeBytes += m_indexBuffer->getSizeBytes();
                std::cout << "rootsBuffer.size = " << m_rootsBuffer->getSizeBytes() << " bytes (" << static_cast<float>(m_rootsBuffer->getSizeBytes()) * glm::pow(10, -6) << " MB)" << std::endl;
                totalSizeBytes += m_rootsBuffer->getSizeBytes();
                std::cout << "sum = " << totalSizeBytes << " bytes (" << static_cast<float>(totalSizeBytes) * glm::pow(10, -6) << " MB)" << std::endl;
            }

            m_passPreparation->setStorageBuffer(0, 0, m_dagBuffer.get());
            m_passPreparation->setStorageBuffer(0, 1, m_indirectionBuffer.get());
            m_passPreparation->setStorageBuffer(0, 2, m_indexBuffer.get());

            for (uint32_t level = 0; level < m_dagLevels.size(); level++) {
                if (m_dagLevels[level].count <= 1) {
                    // no sort required
                    continue;
                }
                m_passSort[level]->setStorageBuffer(0, 0, m_dagBuffer.get());
                m_passSort[level]->setStorageBuffer(0, 1, m_indexBuffer.get());
            }

            m_passUnique->setStorageBuffer(0, 0, m_dagBuffer.get());
            m_passUnique->setStorageBuffer(0, 1, m_buffers[0].get());

            m_passCompaction->setStorageBuffer(0, 0, m_dagBuffer.get());
            m_passCompaction->setStorageBuffer(0, 1, m_dagLevelBuffer.get());
            m_passCompaction->setStorageBuffer(0, 2, m_buffers[0].get());
            m_passCompaction->setStorageBuffer(0, 3, m_indirectionBuffer.get());
            m_passCompaction->setStorageBuffer(0, 4, m_indexBuffer.get());

            m_passRoots->setStorageBuffer(0, 0, m_indirectionBuffer.get());
            m_passRoots->setStorageBuffer(0, 1, m_rootsBuffer.get());
        }

        void execute() const {
            const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

            vk::Semaphore waitTimelineSemaphore = nullptr;
            uint64_t waitTimelineSemaphoreValue = 0;
            vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eComputeShader;

            uint32_t globalOffset = 0;
            for (uint32_t level = 0; level < m_dagLevels.size(); level++) {
                // 0
                m_passPreparation->setGlobalInvocationSize(m_dagLevels[level].count, 1, 1);
                m_computePass->execute([&] {},
                                       [&] {},
                                       [&](vk::CommandBuffer &commandBuffer) {
                                           {
                                               m_passPreparation->m_pushConstants.g_level = level;
                                               m_passPreparation->m_pushConstants.g_level_index = m_dagLevels[level].index;
                                               m_passPreparation->m_pushConstants.g_level_count = m_dagLevels[level].count;
                                               m_passPreparation->m_pushConstants.g_global_offset = globalOffset;

                                               m_passPreparation->execute(
                                                       [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                           cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassPreparation::PushConstants), &m_passPreparation->m_pushConstants);
                                                       },
                                                       [](const vk::CommandBuffer &cbf) {
                                                           constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                                           cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                                       },
                                                       commandBuffer);
                                           }
                                       },
                                       waitTimelineSemaphore ? 1 : 0, waitTimelineSemaphore ? &waitTimelineSemaphore : nullptr, &waitTimelineSemaphoreValue, &waitDstStageMask);
                waitTimelineSemaphore = m_computePass->getTimelineSemaphore();
                waitTimelineSemaphoreValue = m_computePass->getTimelineSemaphoreValue();

                // 1
                if (m_dagLevels[level].count > 1) {
                    auto sort = [&](const DAGGPUPassSort::PushConstants::eAlgorithmVariant algorithm, const uint32_t h) {
                        m_passSort[level]->setWorkGroupCount(m_sortWorkGroupCount[level], 1, 1);
                        m_computePass->execute([&] {},
                                               [&] {},
                                               [&](vk::CommandBuffer &commandBuffer) {
                                                   {
                                                       m_passSort[level]->m_pushConstants.numElements = m_dagLevels[level].count;  // define range of elements in buffer [globalOffset, globalOffset + numElements)
                                                       m_passSort[level]->m_pushConstants.globalOffset = m_dagLevels[level].index; // define range of elements in buffer [globalOffset, globalOffset + numElements)
                                                       m_passSort[level]->m_pushConstants.algorithm = algorithm;
                                                       m_passSort[level]->m_pushConstants.h = h;

                                                       m_passSort[level]->execute(
                                                               [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                                   cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassSort::PushConstants), &m_passSort[level]->m_pushConstants);
                                                               },
                                                               [](const vk::CommandBuffer &cbf) {
                                                                   constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                                                   cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                                               },
                                                               commandBuffer);
                                                   }
                                               },
                                               waitTimelineSemaphore ? 1 : 0, waitTimelineSemaphore ? &waitTimelineSemaphore : nullptr, &waitTimelineSemaphoreValue, &waitDstStageMask);
                        waitTimelineSemaphore = m_computePass->getTimelineSemaphore();
                        waitTimelineSemaphoreValue = m_computePass->getTimelineSemaphoreValue();
                    };

                    uint32_t h = m_sortWorkGroupSizeX[level] * 2;
                    assert(h <= m_sortN[level]);
                    assert(h % 2 == 0);

                    sort(DAGGPUPassSort::PushConstants::eLocalBitonicMergeSortExample, h);
                    // we must now double h, as this happens before every flip
                    h *= 2;

                    for (; h <= m_sortN[level]; h *= 2) {
                        sort(DAGGPUPassSort::PushConstants::eBigFlip, h);

                        for (uint32_t hh = h / 2; hh > 1; hh /= 2) {
                            if (hh <= m_sortWorkGroupSizeX[level] * 2) {
                                // We can fit all elements for a disperse operation into continuous shader
                                // workgroup local memory, which means we can complete the rest of the
                                // cascade using a single shader invocation.
                                sort(DAGGPUPassSort::PushConstants::eLocalDisperse, hh);
                                break;
                            } else {
                                sort(DAGGPUPassSort::PushConstants::eBigDisperse, hh);
                            }
                        }
                    }
                }


                // 2
                m_passUnique->setGlobalInvocationSize(m_dagLevels[level].count, 1, 1);
                m_computePass->execute([&] {},
                                       [&] {},
                                       [&](vk::CommandBuffer &commandBuffer) {
                                           {
                                               m_passUnique->m_pushConstants.g_level = level;
                                               m_passUnique->m_pushConstants.g_level_index = m_dagLevels[level].index;
                                               m_passUnique->m_pushConstants.g_level_count = m_dagLevels[level].count;
                                               m_passUnique->m_pushConstants.g_global_offset = globalOffset;

                                               m_passUnique->execute(
                                                       [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                           cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassUnique::PushConstants), &m_passUnique->m_pushConstants);
                                                       },
                                                       [](const vk::CommandBuffer &cbf) {
                                                           constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                                           cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                                       },
                                                       commandBuffer);
                                           }
                                       },
                                       waitTimelineSemaphore ? 1 : 0, waitTimelineSemaphore ? &waitTimelineSemaphore : nullptr, &waitTimelineSemaphoreValue, &waitDstStageMask);
                waitTimelineSemaphore = m_computePass->getTimelineSemaphore();
                waitTimelineSemaphoreValue = m_computePass->getTimelineSemaphoreValue();

                // 3
                {
                    constexpr uint WORKGROUP_SIZE = 256;

                    std::vector<uint32_t> numElements;
                    std::vector<uint32_t> numOffsets;

                    uint32_t currentNumElements = m_dagLevels[level].count;
                    numElements.push_back(currentNumElements);
                    uint32_t currentNumOffsets = PassShaderCompute::getDispatchSize(currentNumElements, 1, 1, vk::Extent3D{WORKGROUP_SIZE, 1, 1}).width;
                    numOffsets.push_back(currentNumOffsets);

                    while (currentNumElements > WORKGROUP_SIZE) {
                        currentNumElements = currentNumOffsets;
                        numElements.push_back(currentNumElements);
                        currentNumOffsets = PassShaderCompute::getDispatchSize(currentNumElements, 1, 1, vk::Extent3D{WORKGROUP_SIZE, 1, 1}).width;
                        numOffsets.push_back(currentNumOffsets);
                    }

                    // down
                    for (uint32_t i = 0; i < numElements.size(); i++) {
                        m_passPrefixSum->setGlobalInvocationSize(numElements[i], 1, 1);
                        m_computePass->execute(
                                [&] {
                                    m_passPrefixSum->setStorageBuffer(0, 0, m_buffers[i].get());
                                    m_passPrefixSum->setStorageBuffer(0, 1, m_buffers[i + 1].get());
                                },
                                [&] {},
                                [&](vk::CommandBuffer &commandBuffer) {
                                    {
                                        m_passPrefixSum->m_pushConstants.g_num_elements = numElements[i];

                                        m_passPrefixSum->execute(
                                                [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                    cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassPrefixSum::PushConstants), &m_passPrefixSum->m_pushConstants);
                                                },
                                                [](const vk::CommandBuffer &cbf) {
                                                    constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                                    cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                                },
                                                commandBuffer);
                                    }
                                },
                                waitTimelineSemaphore ? 1 : 0, waitTimelineSemaphore ? &waitTimelineSemaphore : nullptr, &waitTimelineSemaphoreValue, &waitDstStageMask);
                        waitTimelineSemaphore = m_computePass->getTimelineSemaphore();
                        waitTimelineSemaphoreValue = m_computePass->getTimelineSemaphoreValue();
                    }

                    // up
                    for (int32_t i = static_cast<int32_t>(numElements.size()) - 2; i >= 0; i--) {
                        m_passPrefixSumOffset->setGlobalInvocationSize(numElements[i], 1, 1);
                        m_computePass->execute(
                                [&] {
                                    m_passPrefixSumOffset->setStorageBuffer(0, 0, m_buffers[i].get());
                                    m_passPrefixSumOffset->setStorageBuffer(0, 1, m_buffers[i + 1].get());
                                },
                                [&] {},
                                [&](vk::CommandBuffer &commandBuffer) {
                                    {
                                        m_passPrefixSumOffset->m_pushConstants.g_num_elements = numElements[i];

                                        m_passPrefixSumOffset->execute(
                                                [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                    cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassPrefixSumOffset::PushConstants), &m_passPrefixSumOffset->m_pushConstants);
                                                },
                                                [](const vk::CommandBuffer &cbf) {
                                                    constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                                    cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                                },
                                                commandBuffer);
                                    }
                                },
                                waitTimelineSemaphore ? 1 : 0, waitTimelineSemaphore ? &waitTimelineSemaphore : nullptr, &waitTimelineSemaphoreValue, &waitDstStageMask);
                        waitTimelineSemaphore = m_computePass->getTimelineSemaphore();
                        waitTimelineSemaphoreValue = m_computePass->getTimelineSemaphoreValue();
                    }
                }

                // 4
                m_passCompaction->setGlobalInvocationSize(256, 1, 1);
                m_computePass->execute([&] {},
                                       [&] {},
                                       [&](vk::CommandBuffer &commandBuffer) {
                                           {
                                               m_passCompaction->m_pushConstants.g_level = level;
                                               m_passCompaction->m_pushConstants.g_level_index = m_dagLevels[level].index;
                                               m_passCompaction->m_pushConstants.g_level_count = m_dagLevels[level].count;
                                               m_passCompaction->m_pushConstants.g_global_offset = globalOffset;

                                               m_passCompaction->execute(
                                                       [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                           cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassCompaction::PushConstants), &m_passCompaction->m_pushConstants);
                                                       },
                                                       [](const vk::CommandBuffer &cbf) {
                                                           constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                                           cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                                       },
                                                       commandBuffer);
                                           }
                                       },
                                       waitTimelineSemaphore ? 1 : 0, waitTimelineSemaphore ? &waitTimelineSemaphore : nullptr, &waitTimelineSemaphoreValue, &waitDstStageMask);
                waitTimelineSemaphore = m_computePass->getTimelineSemaphore();
                waitTimelineSemaphoreValue = m_computePass->getTimelineSemaphoreValue();

                // get dag data
                {
                    m_gpuContext->m_device.waitIdle();

                    std::vector<DAG::DAGLevel> data(m_dagLevels.size());
                    m_dagLevelBuffer->downloadWithStagingBuffer(data.data());
                    // std::cout << "dagLevels[" << level << "].index = " << data[level].index << std::endl;
                    // std::cout << "dagLevels[" << level << "].count = " << data[level].count << std::endl;

                    globalOffset += data[level].count;
                }
            }

            // 5
            m_passRoots->setGlobalInvocationSize(m_dagRootSize, 1, 1);
            m_computePass->execute([&] {},
                                   [&] {},
                                   [&](vk::CommandBuffer &commandBuffer) {
                                       {
                                           m_passRoots->m_pushConstants.g_num_elements = m_dagRootSize;

                                           m_passRoots->execute(
                                                   [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                       cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassRoots::PushConstants), &m_passRoots->m_pushConstants);
                                                   },
                                                   [](const vk::CommandBuffer &cbf) {
                                                       constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                                       cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                                   },
                                                   commandBuffer);
                                       }
                                   },
                                   waitTimelineSemaphore ? 1 : 0, waitTimelineSemaphore ? &waitTimelineSemaphore : nullptr, &waitTimelineSemaphoreValue, &waitDstStageMask);
            waitTimelineSemaphore = m_computePass->getTimelineSemaphore();
            waitTimelineSemaphoreValue = m_computePass->getTimelineSemaphoreValue();

            m_gpuContext->m_device.waitIdle();

            const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            const double gpuTime = (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) * std::pow(10, -3));

            std::cout << "[DAGGPU] " << gpuTime << "[ms]" << std::endl;
        }

        void download(std::vector<DAG::DAGRoot> &dagRoot, std::vector<DAG::DAGNode> &dag, std::vector<DAG::DAGLevel> &dagLevels) const {
            dagLevels.resize(m_dagLevels.size());
            m_dagLevelBuffer->downloadWithStagingBuffer(dagLevels.data());

            uint32_t numDAGElements = 0;
            for (uint32_t level = 0; level < dagLevels.size(); level++) {
                assert(numDAGElements == dagLevels[level].index);
                numDAGElements += dagLevels[level].count;
            }
            dag.resize(numDAGElements);
            m_dagBuffer->downloadWithStagingBuffer(dag.data(), numDAGElements * sizeof(DAG::DAGNode));

            dagRoot.resize(m_dagRootSize);
            m_rootsBuffer->downloadWithStagingBuffer(dagRoot.data());
        }

        void release() {
            RAVEN_BUFFER_RELEASE(m_dagBuffer);
            RAVEN_BUFFER_RELEASE(m_dagLevelBuffer);
            for (auto &buffer: m_buffers) {
                RAVEN_BUFFER_RELEASE(buffer);
            }
            RAVEN_BUFFER_RELEASE(m_indirectionBuffer);
            RAVEN_BUFFER_RELEASE(m_indexBuffer);
            RAVEN_BUFFER_RELEASE(m_rootsBuffer);

            RAVEN_PASS_RELEASE(m_computePass);
            RAVEN_PASS_SHADER_RELEASE(m_passPreparation);
            for (auto &pass: m_passSort) {
                RAVEN_PASS_SHADER_RELEASE(pass);
            }
            RAVEN_PASS_SHADER_RELEASE(m_passUnique);
            RAVEN_PASS_SHADER_RELEASE(m_passPrefixSum);
            RAVEN_PASS_SHADER_RELEASE(m_passPrefixSumOffset);
            RAVEN_PASS_SHADER_RELEASE(m_passCompaction);
            RAVEN_PASS_SHADER_RELEASE(m_passRoots);
        }

    private:
        GPUContext *m_gpuContext;

        std::shared_ptr<PassCompute> m_computePass;
        std::shared_ptr<DAGGPUPassPreparation> m_passPreparation;
        std::vector<std::shared_ptr<DAGGPUPassSort>> m_passSort;
        std::shared_ptr<DAGGPUPassUnique> m_passUnique;
        std::shared_ptr<DAGGPUPassPrefixSum> m_passPrefixSum;
        std::shared_ptr<DAGGPUPassPrefixSumOffset> m_passPrefixSumOffset;
        std::shared_ptr<DAGGPUPassCompaction> m_passCompaction;
        std::shared_ptr<DAGGPUPassRoots> m_passRoots;

        std::shared_ptr<Buffer> m_dagBuffer;
        std::shared_ptr<Buffer> m_dagLevelBuffer;
        std::vector<std::shared_ptr<Buffer>> m_buffers; // additional buffers
        std::shared_ptr<Buffer> m_indirectionBuffer;
        std::shared_ptr<Buffer> m_indexBuffer;
        std::shared_ptr<Buffer> m_rootsBuffer;

        uint32_t m_dagRootSize;
        std::vector<DAG::DAGLevel> m_dagLevels;

        std::vector<uint32_t> m_sortN;
        std::vector<uint32_t> m_sortWorkGroupSizeX;
        std::vector<uint32_t> m_sortWorkGroupCount;

        static uint32_t nextPowerOfTwo(uint32_t v) {
            v--;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            v++;
            return v;
        }
    };
} // namespace raven