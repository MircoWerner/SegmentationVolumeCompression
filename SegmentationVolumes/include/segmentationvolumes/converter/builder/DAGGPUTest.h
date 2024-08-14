#pragma once

#include "../../Raystructs.h"
#include "DAG.h"
#include "DAGGPU.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace raven {
    class DAGGPUTest {
    public:
        static void test(const std::string &data, const std::string &scene, const std::vector<SegmentationVolumeConverter::DAGFileInfo> &dagFileInfos, const std::string &prefixPlural, const std::string &outputFolder) {
            std::vector<DAG::DAGRoot> dagRoot;
            std::vector<DAG::DAGNode> dag;
            std::vector<DAG::DAGLevel> dagLevels;
            SegmentationVolumeConverter::loadDAGsCombine(data, scene, dagFileInfos, dagRoot, dag, dagLevels);

            DAG dagConstruct(dagRoot.data(), dagRoot.size(), dag.data(), dag.size(), dagLevels);
            std::cout << "[DAG] Start verification." << std::endl;
            dagConstruct.verify();
            std::cout << "[DAG] End verification." << std::endl;

            auto gpuContext = std::make_shared<GPUContext>(Queues::QueueFamilies::COMPUTE_FAMILY | Queues::QueueFamilies::GRAPHICS_FAMILY | Queues::TRANSFER_FAMILY);
            gpuContext->init();

            DAGGPU dagGPU(gpuContext.get(), dagRoot.size(), dagLevels);
            dagGPU.create(dagRoot, dag);
            std::cout << "[DAG] Start reduction." << std::endl;
            dagGPU.execute();
            std::cout << "[DAG] End reduction." << std::endl;
            std::vector<DAG::DAGRoot> dagRootNew;
            std::vector<DAG::DAGNode> dagNew;
            std::vector<DAG::DAGLevel> dagLevelsNew;
            dagGPU.download(dagRootNew, dagNew, dagLevelsNew);
            dagGPU.release();

            dagConstruct = DAG(dagRootNew.data(), dagRootNew.size(), dagNew.data(), dagNew.size(), dagLevelsNew);
            std::cout << "[DAG] Start verification." << std::endl;
            dagConstruct.verify();
            std::cout << "[DAG] End verification." << std::endl;

            gpuContext->shutdown();

            // write
            std::filesystem::create_directories(data + "/" + scene + "/" + outputFolder + "/aabb");
            std::filesystem::create_directories(data + "/" + scene + "/" + outputFolder + "/lod");
            std::filesystem::create_directories(data + "/" + scene + "/" + outputFolder + "/lod_data");

            uint64_t c = 0;
            for (uint64_t vol = 0; vol < dagFileInfos.size(); vol++) {
                const auto &dagFileInfo = dagFileInfos[vol];

                for (uint64_t aabb = 0; aabb < dagFileInfo.m_aabbs.size(); aabb++) {
                    const auto &aabbFile = dagFileInfo.m_aabbs[aabb];

                    const uint64_t bytesAABB = std::filesystem::file_size(data + "/" + scene + "/" + dagFileInfo.m_folder + "/aabb/" + aabbFile + ".bin");
                    const uint64_t bytesPerAABB = sizeof(VoxelAABB);
                    const uint64_t numAABB = bytesAABB / bytesPerAABB;
                    std::vector<VoxelAABB> inAABB(numAABB);
                    std::ifstream(data + "/" + scene + "/" + dagFileInfo.m_folder + "/aabb/" + aabbFile + ".bin", std::ios::binary).read(reinterpret_cast<char *>(inAABB.data()), static_cast<std::streamsize>(bytesAABB));

                    for (uint64_t i = 0; i < numAABB; i++) {
                        inAABB[i].lod = dagRootNew[c];
                        c++;
                    }

                    std::ofstream(data + "/" + scene + "/" + outputFolder + "/aabb/" + aabbFile + ".bin", std::ios::binary).write(reinterpret_cast<char *>(inAABB.data()), static_cast<std::streamsize>(bytesAABB));
                }
            }

            std::ofstream(data + "/" + scene + "/" + outputFolder + "/lod/" + prefixPlural + ".bin", std::ios::binary).write(reinterpret_cast<char *>(dagNew.data()), static_cast<std::streamsize>(dagNew.size() * sizeof(DAG::DAGNode)));
            std::ofstream(data + "/" + scene + "/" + outputFolder + "/lod_data/" + prefixPlural + ".bin", std::ios::binary).write(reinterpret_cast<char *>(dagLevelsNew.data()), static_cast<std::streamsize>(dagLevelsNew.size() * sizeof(DAG::DAGLevel)));
        }

        static uint32_t sectionUpdatePointer(const uint64_t volume, const uint64_t pointer, const std::vector<DAG::DAGLevel> &dagLevels, const std::vector<std::vector<DAG::DAGLevel>> &offsetDAGLevels,
                                             const DAG::DAGLevel *inDAGLevels) {
            uint64_t localOffset = 0;

            for (uint64_t level = 0; level < offsetDAGLevels[volume].size(); level++) {
                const auto &globalLevelOffset = dagLevels[level];
                const auto &levelOffset = offsetDAGLevels[volume][level];
                const auto &localLevelOffset = inDAGLevels[level];

                if (pointer < localOffset + localLevelOffset.count) {
                    const uint64_t localOffsetInLevel = pointer - localLevelOffset.index;
                    const uint64_t globalOffsetInLevel = levelOffset.index + localOffsetInLevel;
                    return globalLevelOffset.index + globalOffsetInLevel;
                }

                localOffset += localLevelOffset.count;
            }

            return 0xFFFFFFFF;
        }

        static void testSort() {
            auto gpuContext = std::make_shared<GPUContext>(raven::Queues::QueueFamilies::COMPUTE_FAMILY | raven::Queues::QueueFamilies::GRAPHICS_FAMILY | raven::Queues::TRANSFER_FAMILY);
            gpuContext->init();

            ///////
            std::vector<DAG::DAGNode> dag;
            std::vector<uint32_t> index;
            for (uint32_t i = 0; i < 1024; i++) {
                dag.push_back(
                        {(1024 - i) % 10, DAG::invalidPointer(), DAG::invalidPointer(), DAG::invalidPointer(), DAG::invalidPointer(), DAG::invalidPointer(), DAG::invalidPointer(), DAG::invalidPointer()});
                index.push_back(i);
            }

            std::cout << std::endl;
            for (uint32_t i = 0; i < dag.size(); i++) {
                std::cout << dag[i].child0 << " ";
                if (i % 10 == 0) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            std::cout << std::endl;
            for (uint32_t i = 0; i < index.size(); i++) {
                std::cout << index[i] << " ";
                if (i % 10 == 0) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;

            int a;
            std::cin >> a;

            auto m_gpuContext = gpuContext.get();

            std::vector<uint32_t> m_sortN;
            std::vector<uint32_t> m_sortWorkGroupSizeX;
            std::vector<uint32_t> m_sortWorkGroupCount;
            {
                uint32_t n = nextPowerOfTwo(dag.size());

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

            std::shared_ptr<PassCompute> m_computePass;
            std::shared_ptr<DAGGPUPassSort> m_passSort;
            {
                m_computePass = std::make_shared<PassCompute>(m_gpuContext, Pass::PassSettings{.m_name = "m_computePass"});
                m_computePass->create();

                m_passSort = std::make_shared<DAGGPUPassSort>(m_gpuContext, m_sortWorkGroupSizeX[0]);
                m_passSort->create();
            }

            std::shared_ptr<Buffer> m_dagBuffer;
            std::shared_ptr<Buffer> m_indexBuffer;
            {
                {
                    const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(dag.size() * sizeof(DAG::DAGNode)), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "dag"};
                    m_dagBuffer = Buffer::fillDeviceWithStagingBuffer(m_gpuContext, bufferSettings, dag.data());
                }
                {
                    const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(index.size() * sizeof(uint32_t)), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "index"};
                    m_indexBuffer = Buffer::fillDeviceWithStagingBuffer(m_gpuContext, bufferSettings, index.data());
                }

                m_passSort->setStorageBuffer(0, 0, m_dagBuffer.get());
                m_passSort->setStorageBuffer(0, 1, m_indexBuffer.get());
            }

            // 1
            {
                vk::Semaphore waitTimelineSemaphore = nullptr;
                uint64_t waitTimelineSemaphoreValue = 0;
                vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eComputeShader;

                auto sort = [&](const DAGGPUPassSort::PushConstants::eAlgorithmVariant algorithm, const uint32_t h) {
                    m_passSort->setWorkGroupCount(m_sortWorkGroupCount[0], 1, 1);
                    m_computePass->execute([&] {},
                                           [&] {},
                                           [&](vk::CommandBuffer &commandBuffer) {
                                               {
                                                   m_passSort->m_pushConstants.numElements = dag.size(); // define range of elements in buffer [globalOffset, globalOffset + numElements)
                                                   m_passSort->m_pushConstants.globalOffset = 0;         // define range of elements in buffer [globalOffset, globalOffset + numElements)
                                                   m_passSort->m_pushConstants.algorithm = algorithm;
                                                   m_passSort->m_pushConstants.h = h;

                                                   m_passSort->execute(
                                                           [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                                               cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DAGGPUPassSort::PushConstants), &m_passSort->m_pushConstants);
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
                };

                uint32_t h = m_sortWorkGroupSizeX[0] * 2;
                assert(h <= m_sortN[0]);
                assert(h % 2 == 0);

                sort(DAGGPUPassSort::PushConstants::eLocalBitonicMergeSortExample, h);
                // we must now double h, as this happens before every flip
                h *= 2;

                for (; h <= m_sortN[0]; h *= 2) {
                    sort(DAGGPUPassSort::PushConstants::eBigFlip, h);

                    for (uint32_t hh = h / 2; hh > 1; hh /= 2) {
                        if (hh <= m_sortWorkGroupSizeX[0] * 2) {
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

            m_dagBuffer->downloadWithStagingBuffer(dag.data());
            m_indexBuffer->downloadWithStagingBuffer(index.data());

            std::cout << std::endl;
            for (uint32_t i = 0; i < dag.size(); i++) {
                std::cout << dag[i].child0 << " ";
                if (i % 10 == 0) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            std::cout << std::endl;
            for (uint32_t i = 0; i < index.size(); i++) {
                std::cout << index[i] << " ";
                if (i % 10 == 0) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;

            ////////

            gpuContext->shutdown();
        }

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