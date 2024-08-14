#pragma once
#include "../Raystructs.h"
#include "builder/DAG.h"
#include "builder/Octree.h"
#include "raven/util/AABB.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <regex>
#include <string>

#include <omp.h>

namespace raven {
    class SegmentationVolumeConverter {
    public:
        SegmentationVolumeConverter(std::string prefix, std::string prefixPlural, std::string data, std::string scene, const bool svdagOccupancyField) : m_prefix(std::move(prefix)), m_prefixPlural(std::move(prefixPlural)), m_data(std::move(data)), m_scene(std::move(scene)), m_svdagOccupancyField(svdagOccupancyField) {}

        virtual ~SegmentationVolumeConverter() = default;

        virtual void rawDataToVoxelTypes() = 0;

        void voxelTypesToAABBsAndOctrees() const {
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + m_stringSVO + "/aabb");
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod");

            uint32_t types = 0;

            // omp_set_dynamic(0);
            // omp_set_num_threads(4);
            // std::cout << "Using " << omp_get_max_threads() << " threads." << std::endl;

            std::vector<std::filesystem::directory_entry> files;
            for (const auto &type: std::filesystem::directory_iterator(m_data + "/" + m_scene + "/" + m_stringVoxels)) {
                files.push_back(type);
            }

            double totalTime = 0;

            // #pragma omp parallel for
            for (uint32_t i = 0; i < files.size(); i++) {
                const auto &type = files[i];
                std::string filename = type.path().filename().string();
                const std::regex rgx("[" + m_prefix + "]?([0-9]+)\\.[bin|idx]");
                std::smatch matches;
                std::regex_search(filename, matches, rgx);
                if (matches.size() != 2) {
                    std::cout << "Skipping " << filename << "." << std::endl;
                    continue;
                }
                const uint32_t typeId = static_cast<uint32_t>(std::stoul(matches[1]));
                if (std::filesystem::exists(m_data + "/" + m_scene + "/" + m_stringSVO + "/aabb/" + m_prefix + std::to_string(typeId) + ".bin") && std::filesystem::exists(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod/" + m_prefix + std::to_string(typeId) + ".bin")) {
                    std::cout << "Skipping " << filename << ". SVO already exists." << std::endl;
                    continue;
                }

                std::cout << "[" << filename << "]" << std::endl;

                uint32_t numVoxels;
                std::map<uint32_t, std::pair<iAABB, std::vector<glm::ivec3>>> voxels;
                loadVoxels(type, numVoxels, voxels);

                std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

                // subdivide
                std::vector<Octree::OctreeBuildInfo> octreeBuildInfos;
                std::cout << "[" << filename << "] Subdividing " << voxels.size() << " id(s)." << std::endl;
                size_t instance = 0;
                for (auto &[id, voxel]: voxels) {
                    subdivide(voxel.second, 0, voxel.second.size(), voxel.first, toLabelId(typeId, instance), octreeBuildInfos);
                    instance++;
                }
                std::cout << "[" << filename << "] Subdivided." << std::endl;

                // build octrees
                Octree octreeBuilder;
                octreeBuilder.buildOctrees(octreeBuildInfos);
                std::cout << "[" << filename << "] SVOs built." << std::endl;

                std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                double cpuTime = (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) * std::pow(10, -3));
                totalTime += cpuTime;
                std::cout << "[SVO] " << cpuTime << "[ms]" << std::endl;

                // write
                std::vector<VoxelAABB> aabbs;
                for (uint32_t j = 0; j < octreeBuildInfos.size(); j++) {
                    const auto &octreeBuildInfo = octreeBuildInfos[j];
                    aabbs.push_back(VoxelAABB{octreeBuildInfo.aabb.m_min.x, octreeBuildInfo.aabb.m_min.y, octreeBuildInfo.aabb.m_min.z,
                                              octreeBuildInfo.aabb.m_max.x, octreeBuildInfo.aabb.m_max.y, octreeBuildInfo.aabb.m_max.z,
                                              octreeBuildInfo.labelId, octreeBuilder.m_octreeIndices[j]});
                }

                std::ofstream(m_data + "/" + m_scene + "/" + m_stringSVO + "/aabb/" + m_prefix + std::to_string(typeId) + ".bin", std::ios::binary)
                        .write(reinterpret_cast<char *>(aabbs.data()), static_cast<std::streamsize>(aabbs.size() * sizeof(VoxelAABB)));
                std::ofstream(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod/" + m_prefix + std::to_string(typeId) + ".bin", std::ios::binary)
                        .write(reinterpret_cast<char *>(octreeBuilder.m_octrees.data()), static_cast<std::streamsize>(octreeBuilder.m_octrees.size() * sizeof(Octree::OctreeNode)));

                std::cout << "Processed " << ++types << "." << std::endl;
            }

            std::cout << "[SVO] Total time: " << totalTime << "[ms]" << std::endl;
        }

        void AABBsAndOctreesToAABBsAndDAGs() const {
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/aabb");
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/lod");
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/lod_data");

            uint32_t types = 0;

            std::vector<std::filesystem::directory_entry> files;
            for (const auto &type: std::filesystem::directory_iterator(m_data + "/" + m_scene + "/" + m_stringSVO + "/aabb")) {
                files.push_back(type);
            }

            double totalTime = 0;

            for (uint32_t i = 0; i < files.size(); i++) {
                const auto &type = files[i];
                std::string filename = type.path().filename().string();
                if (!std::filesystem::exists(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod/" + filename)) {
                    std::cout << "Skipping " << filename << ": Missing LOD file." << std::endl;
                    continue;
                }
                const std::regex rgx(m_prefix + "([0-9]+)\\.bin");
                std::smatch matches;
                std::regex_search(filename, matches, rgx);
                if (matches.size() != 2) {
                    std::cout << "Skipping " << filename << "." << std::endl;
                    continue;
                }
                const uint32_t typeId = static_cast<uint32_t>(std::stoul(matches[1]));
                if (std::filesystem::exists(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/aabb/" + m_prefix + std::to_string(typeId) + ".bin") && std::filesystem::exists(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/lod/" + m_prefix + std::to_string(typeId) + ".bin") && std::filesystem::exists(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/lod_data/" + m_prefix + std::to_string(typeId) + ".bin")) {
                    std::cout << "Skipping " << filename << ". SVDAG already exists." << std::endl;
                    continue;
                }

                std::cout << "[" << filename << "]" << std::endl;

                std::ifstream file(type.path(), std::ios::binary);

                std::vector<char> aabbRaw;
                std::vector<char> lodRaw;

                uint32_t numAABB;
                {
                    uint32_t bytesAABB = std::filesystem::file_size(type.path());
                    uint32_t bytesPerAABB = sizeof(VoxelAABB);
                    numAABB = bytesAABB / bytesPerAABB;
                    aabbRaw.resize(bytesAABB);
                    std::ifstream(type.path(), std::ios::binary).read(aabbRaw.data(), bytesAABB);

                    uint32_t bytesLOD = std::filesystem::file_size(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod/" + filename);
                    uint32_t bytesPerLOD = sizeof(Octree::OctreeNode);
                    lodRaw.resize(bytesLOD);
                    std::ifstream(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod/" + filename, std::ios::binary).read(lodRaw.data(), bytesLOD);
                }

                std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

                // dag
                std::vector<DAG::DAGRoot> dagRoot(numAABB);
                uint32_t dagRootCount = numAABB;
                std::vector<DAG::DAGNode> dag;
                std::vector<DAG::DAGLevel> dagLevels;

                // initialize
                if (m_svdagOccupancyField) {
                    svdagOccupancyField_fromOctree(reinterpret_cast<VoxelAABB *>(aabbRaw.data()), reinterpret_cast<Octree::OctreeNode *>(lodRaw.data()), dagRoot.data(), dagRootCount, dag, dagLevels);
                } else {
                    svdag_fromOctree(reinterpret_cast<VoxelAABB *>(aabbRaw.data()), reinterpret_cast<Octree::OctreeNode *>(lodRaw.data()), dagRoot.data(), dagRootCount, dag, dagLevels);
                }
                uint32_t dagCount = dagLevels[dagLevels.size() - 1].index + dagLevels[dagLevels.size() - 1].count;

                // construct
                DAG dagConstruct(dagRoot.data(), dagRootCount, dag.data(), dagCount, dagLevels);
                // dagConstruct.verify();
                std::cout << "[" << filename << "] Start reduce." << std::endl;
                uint32_t outDAGCount;
                std::vector<DAG::DAGLevel> outDAGLevels(dagLevels.size());
                dagConstruct.reduce(&outDAGCount, outDAGLevels);
                std::cout << "[" << filename << "] End reduce." << std::endl;
                // DAG dagVerify(dagRoot.data(), dagRootCount, dag.data(), outDAGCount, outDAGLevels);
                // dagVerify.verify();

                // update aabb pointers
                auto aabbVec = reinterpret_cast<VoxelAABB *>(aabbRaw.data());
                for (uint32_t j = 0; j < numAABB; j++) {
                    auto &aabb = aabbVec[j];
                    aabb.lod = dagRoot[j];
                }

                std::cout << "[" << filename << "] DAG built." << std::endl;

                std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                double cpuTime = (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) * std::pow(10, -3));
                totalTime += cpuTime;
                std::cout << "[SVDAG] " << cpuTime << "[ms]" << std::endl;

                // write
                std::ofstream(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/aabb/" + m_prefix + std::to_string(typeId) + ".bin", std::ios::binary)
                        .write(aabbRaw.data(), static_cast<std::streamsize>(numAABB * sizeof(VoxelAABB)));
                std::ofstream(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/lod/" + m_prefix + std::to_string(typeId) + ".bin", std::ios::binary)
                        .write(reinterpret_cast<char *>(dag.data()), static_cast<std::streamsize>(outDAGCount * sizeof(DAG::DAGNode)));
                std::ofstream(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/lod_data/" + m_prefix + std::to_string(typeId) + ".bin", std::ios::binary)
                        .write(reinterpret_cast<char *>(outDAGLevels.data()), static_cast<std::streamsize>(outDAGLevels.size() * sizeof(DAG::DAGLevel)));

                std::cout << "Processed " << ++types << "." << std::endl;
            }

            std::cout << "[SVDAG] Total time: " << totalTime << "[ms]" << std::endl;
        }

        struct DAGFileInfo {
            std::string m_folder;

            std::vector<std::string> m_aabbs;
            std::string m_lod;
            std::string m_lodData;

            [[nodiscard]] bool exists(const std::string &data, const std::string &scene) const {
                for (const auto &aabb: m_aabbs) {
                    if (!std::filesystem::exists(data + "/" + scene + "/" + m_folder + "/aabb/" + aabb + ".bin")) {
                        return false;
                    }
                }
                return std::filesystem::exists(data + "/" + scene + "/" + m_folder + "/lod/" + m_lod + ".bin") &&
                       std::filesystem::exists(data + "/" + scene + "/" + m_folder + "/lod_data/" + m_lodData + ".bin");
            }
        };

        void mergeDAGs(const std::vector<DAGFileInfo> &dagFileInfos) const {
            std::vector<DAG::DAGRoot> dagRoot;
            std::vector<DAG::DAGNode> dag;
            std::vector<DAG::DAGLevel> dagLevels;

            loadDAGsCombine(m_data, m_scene, dagFileInfos, dagRoot, dag, dagLevels);

            DAG dagConstruct(dagRoot.data(), dagRoot.size(), dag.data(), dag.size(), dagLevels);
            std::cout << "[SVDAG] Start verification." << std::endl;
            dagConstruct.verify();
            std::cout << "[SVDAG] End verification." << std::endl;

            std::cout << "[SVDAG] Start reduce." << std::endl;
            uint32_t outDAGCount;
            std::vector<DAG::DAGLevel> outDAGLevels(dagLevels.size());
            dagConstruct.reduce(&outDAGCount, outDAGLevels);
            std::cout << "[SVDAG] End reduce." << std::endl;

            std::cout << "[SVDAG] Start verification." << std::endl;
            DAG dagVerify(dagRoot.data(), dagRoot.size(), dag.data(), outDAGCount, outDAGLevels);
            dagVerify.verify();
            std::cout << "[SVDAG] End verification." << std::endl;

            // write
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/aabb");
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod");
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod_data");

            uint64_t c = 0;
            for (uint64_t vol = 0; vol < dagFileInfos.size(); vol++) {
                const auto &dagFileInfo = dagFileInfos[vol];

                for (uint64_t aabb = 0; aabb < dagFileInfo.m_aabbs.size(); aabb++) {
                    const auto &aabbFile = dagFileInfo.m_aabbs[aabb];

                    uint64_t bytesAABB = std::filesystem::file_size(m_data + "/" + m_scene + "/" + dagFileInfo.m_folder + "/aabb/" + aabbFile + ".bin");
                    uint64_t bytesPerAABB = sizeof(VoxelAABB);
                    uint64_t numAABB = bytesAABB / bytesPerAABB;
                    std::vector<VoxelAABB> inAABB(numAABB);
                    std::ifstream(m_data + "/" + m_scene + "/" + dagFileInfo.m_folder + "/aabb/" + aabbFile + ".bin", std::ios::binary).read(reinterpret_cast<char *>(inAABB.data()), static_cast<std::streamsize>(bytesAABB));

                    for (uint64_t i = 0; i < numAABB; i++) {
                        inAABB[i].lod = dagRoot[c];
                        c++;
                    }

                    std::ofstream(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/aabb/" + aabbFile + ".bin", std::ios::binary).write(reinterpret_cast<char *>(inAABB.data()), static_cast<std::streamsize>(bytesAABB));
                }
            }

            std::ofstream(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod/" + m_prefixPlural + ".bin", std::ios::binary).write(reinterpret_cast<char *>(dag.data()), static_cast<std::streamsize>(outDAGCount * sizeof(DAG::DAGNode)));
            std::ofstream(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod_data/" + m_prefixPlural + ".bin", std::ios::binary).write(reinterpret_cast<char *>(outDAGLevels.data()), static_cast<std::streamsize>(outDAGLevels.size() * sizeof(DAG::DAGLevel)));
        }

        static void loadDAGsCombine(const std::string &data, const std::string &scene, const std::vector<DAGFileInfo> &dagFileInfos,
                                    std::vector<DAG::DAGRoot> &dagRoot,
                                    std::vector<DAG::DAGNode> &dag,
                                    std::vector<DAG::DAGLevel> &dagLevels) {
            uint64_t totalNumAABB = 0;
            uint64_t totalNumLOD = 0;
            std::vector<uint64_t> inNumDagLevels;

            for (const auto &dagFileInfo: dagFileInfos) {
                for (const auto &file: dagFileInfo.m_aabbs) {
                    const uint64_t bytesAABB = std::filesystem::file_size(data + "/" + scene + "/" + dagFileInfo.m_folder + "/aabb/" + file + ".bin");
                    constexpr uint64_t bytesPerAABB = sizeof(VoxelAABB);
                    totalNumAABB += bytesAABB / bytesPerAABB;
                }

                const uint64_t bytesLOD = std::filesystem::file_size(data + "/" + scene + "/" + dagFileInfo.m_folder + "/lod/" + dagFileInfo.m_lod + ".bin");
                constexpr uint64_t bytesPerLOD = sizeof(DAG::DAGNode);
                totalNumLOD += bytesLOD / bytesPerLOD;

                const uint64_t bytesLODData = std::filesystem::file_size(data + "/" + scene + "/" + dagFileInfo.m_folder + "/lod_data/" + dagFileInfo.m_lodData + ".bin");
                constexpr uint64_t bytesPerLODData = sizeof(DAG::DAGLevel);
                inNumDagLevels.push_back(bytesLODData / bytesPerLODData);
            }

            // load DAG levels (lod_data)
            std::vector<std::vector<DAG::DAGLevel>> inDAGLevels;                          // from input
            std::vector<std::vector<DAG::DAGLevel>> offsetDAGLevels(dagFileInfos.size()); // offset when combining
            uint64_t numLevels = 0;
            {
                for (const auto &dagFileInfo: dagFileInfos) {
                    const uint64_t bytesLODData = std::filesystem::file_size(data + "/" + scene + "/" + dagFileInfo.m_folder + "/lod_data/" + dagFileInfo.m_lodData + ".bin");
                    constexpr uint64_t bytesPerLODData = sizeof(DAG::DAGLevel);
                    const uint64_t numLevel = bytesLODData / bytesPerLODData;
                    numLevels = glm::max(numLevels, numLevel);
                    inDAGLevels.emplace_back();
                    inDAGLevels.back().resize(numLevel);
                    std::ifstream(data + "/" + scene + "/" + dagFileInfo.m_folder + "/lod_data/" + dagFileInfo.m_lodData + ".bin", std::ios::binary).read(reinterpret_cast<char *>(inDAGLevels.back().data()), static_cast<std::streamsize>(bytesLODData));
                }

                dagLevels.resize(numLevels);

                uint64_t globalOffset = 0;
                for (uint64_t level = 0; level < numLevels; level++) {
                    auto &dagLevel = dagLevels[level];

                    uint64_t levelOffset = 0;
                    for (uint64_t vol = 0; vol < dagFileInfos.size(); vol++) {
                        if (inNumDagLevels[vol] <= level) {
                            continue;
                        }
                        auto &inDagLevel = inDAGLevels[vol][level];

                        if (offsetDAGLevels[vol].size() < level + 1) {
                            offsetDAGLevels[vol].resize(level + 1);
                        }

                        auto &offsetDagLevel = offsetDAGLevels[vol][level];

                        offsetDagLevel.index = levelOffset;
                        offsetDagLevel.count = inDagLevel.count;

                        levelOffset += inDagLevel.count;
                    }

                    dagLevel.index = globalOffset;
                    dagLevel.count = levelOffset;

                    globalOffset += levelOffset;
                }
            }

            // load DAG (lod)
            dag.resize(totalNumLOD);
            {
                // copy lod
                for (uint64_t vol = 0; vol < dagFileInfos.size(); vol++) {
                    const auto &dagFileInfo = dagFileInfos[vol];

                    uint64_t bytesLOD = std::filesystem::file_size(data + "/" + scene + "/" + dagFileInfo.m_folder + "/lod/" + dagFileInfo.m_lod + ".bin");
                    uint64_t bytesPerLOD = sizeof(DAG::DAGNode);
                    uint64_t numLOD = bytesLOD / bytesPerLOD;
                    std::vector<DAG::DAGNode> inLOD(numLOD);
                    std::ifstream(data + "/" + scene + "/" + dagFileInfo.m_folder + "/lod/" + dagFileInfo.m_lod + ".bin", std::ios::binary).read(reinterpret_cast<char *>(inLOD.data()), static_cast<std::streamsize>(bytesLOD));

                    for (uint64_t level = 0; level < numLevels; level++) {
                        if (inNumDagLevels[vol] <= level) {
                            continue;
                        }
                        const auto &globalLevelOffset = dagLevels[level];
                        const auto &levelOffset = offsetDAGLevels[vol][level];
                        const auto &inDAGLevel = inDAGLevels[vol][level];

                        if (levelOffset.count != inDAGLevel.count) {
                            throw std::runtime_error("Count mismatch.");
                        }

                        std::copy(inLOD.data() + inDAGLevel.index, inLOD.data() + inDAGLevel.index + inDAGLevel.count, dag.begin() + globalLevelOffset.index + levelOffset.index);

                        // update pointers
                        for (uint64_t i = 0; i < inDAGLevel.count; i++) {
                            auto &node = dag[globalLevelOffset.index + levelOffset.index + i];
                            if (node.isLeaf()) {
                                continue;
                            }
                            if (node.child0 == DAG::invalidPointer() || node.child0 >= totalNumLOD ||
                                node.child1 == DAG::invalidPointer() || node.child1 >= totalNumLOD ||
                                node.child2 == DAG::invalidPointer() || node.child2 >= totalNumLOD ||
                                node.child3 == DAG::invalidPointer() || node.child3 >= totalNumLOD ||
                                node.child4 == DAG::invalidPointer() || node.child4 >= totalNumLOD ||
                                node.child5 == DAG::invalidPointer() || node.child5 >= totalNumLOD ||
                                node.child6 == DAG::invalidPointer() || node.child6 >= totalNumLOD ||
                                node.child7 == DAG::invalidPointer() || node.child7 >= totalNumLOD) {
                                throw std::runtime_error("Invalid pointer.");
                            }
                            node.child0 = sectionUpdatePointer(vol, node.child0, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                            node.child1 = sectionUpdatePointer(vol, node.child1, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                            node.child2 = sectionUpdatePointer(vol, node.child2, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                            node.child3 = sectionUpdatePointer(vol, node.child3, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                            node.child4 = sectionUpdatePointer(vol, node.child4, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                            node.child5 = sectionUpdatePointer(vol, node.child5, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                            node.child6 = sectionUpdatePointer(vol, node.child6, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                            node.child7 = sectionUpdatePointer(vol, node.child7, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                        }
                    }

                    std::cout << "[DAG] SVDAG copied: " << dagFileInfo.m_folder << "/lod/" << dagFileInfo.m_lod << ".bin" << std::endl;
                }
            }

            // copy roots
            dagRoot.resize(totalNumAABB);
            uint64_t c = 0;
            for (uint64_t vol = 0; vol < dagFileInfos.size(); vol++) {
                const auto &dagFileInfo = dagFileInfos[vol];

                for (uint64_t aabb = 0; aabb < dagFileInfo.m_aabbs.size(); aabb++) {
                    const auto &aabbFile = dagFileInfo.m_aabbs[aabb];

                    uint64_t bytesAABB = std::filesystem::file_size(data + "/" + scene + "/" + dagFileInfo.m_folder + "/aabb/" + aabbFile + ".bin");
                    uint64_t bytesPerAABB = sizeof(VoxelAABB);
                    uint64_t numAABB = bytesAABB / bytesPerAABB;
                    std::vector<VoxelAABB> inAABB(numAABB);
                    std::ifstream(data + "/" + scene + "/" + dagFileInfo.m_folder + "/aabb/" + aabbFile + ".bin", std::ios::binary).read(reinterpret_cast<char *>(inAABB.data()), static_cast<std::streamsize>(bytesAABB));

                    for (uint64_t i = 0; i < numAABB; i++) {
                        dagRoot[c] = inAABB[i].lod == DAG::invalidPointer() ? DAG::invalidPointer() : sectionUpdatePointer(vol, inAABB[i].lod, dagLevels, offsetDAGLevels, inDAGLevels[vol].data());
                        c++;
                    }
                }
            }
        }

        [[nodiscard]] std::string stringSVDAG(const bool merged) const {
            return m_stringSVDAG + (m_svdagOccupancyField ? "_" + m_stringSVDAGOccupancyField : "") + (merged ? "_" + m_stringSVDAGMerged : "");
        }

        void nodeInfo() {
            std::ofstream csv;
            csv.open(m_data + "/" + m_scene + "/" + m_scene + "_nodeinfo.txt");

            // svo
            {
                csv << "--------------------------" << std::endl;
                csv << "svo" << std::endl;
                std::vector<uint64_t> sumLevels;

                for (const auto &type: std::filesystem::directory_iterator(m_data + "/" + m_scene + "/" + m_stringSVO + "/aabb")) {
                    csv << type.path().filename() << std::endl;

                    std::vector<char> aabbRaw;
                    std::vector<char> lodRaw;

                    uint32_t numAABB;
                    {
                        uint32_t bytesAABB = std::filesystem::file_size(type.path());
                        uint32_t bytesPerAABB = sizeof(VoxelAABB);
                        numAABB = bytesAABB / bytesPerAABB;
                        aabbRaw.resize(bytesAABB);
                        std::ifstream(type.path(), std::ios::binary).read(aabbRaw.data(), bytesAABB);

                        std::string filename = type.path().filename();
                        uint32_t bytesLOD = std::filesystem::file_size(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod/" + filename);
                        lodRaw.resize(bytesLOD);
                        std::ifstream(m_data + "/" + m_scene + "/" + m_stringSVO + "/lod/" + filename, std::ios::binary).read(lodRaw.data(), bytesLOD);
                    }

                    // traverse SVO
                    auto *aabb = reinterpret_cast<VoxelAABB *>(aabbRaw.data());
                    std::vector<uint64_t> levels;
                    for (uint32_t i = 0; i < numAABB; i++) {
                        std::vector<uint64_t> svoLevels;
                        uint32_t level;
                        svo_traverseOctreeRecordNodeInfo(16, svoLevels, reinterpret_cast<Octree::OctreeNode *>(lodRaw.data()), aabb[i].lod, 0, &level);
                        if (svoLevels.size() > levels.size()) {
                            levels.resize(svoLevels.size());
                        }
                        for (uint32_t j = 0; j < svoLevels.size(); j++) {
                            levels[j] += svoLevels[j];
                        }
                    }

                    uint64_t nodesTotal = 0;
                    for (uint32_t i = 0; i < levels.size(); i++) {
                        const auto &level = levels[i];
                        csv << "level " << i << ": " << level << std::endl;
                        nodesTotal += level;
                        if (levels.size() > sumLevels.size()) {
                            sumLevels.resize(levels.size());
                        }
                        sumLevels[i] += level;
                    }
                    csv << "= " << nodesTotal << std::endl;
                }

                csv << "sum" << std::endl;
                uint64_t nodesTotal = 0;
                for (uint32_t i = 0; i < sumLevels.size(); i++) {
                    const auto &level = sumLevels[i];
                    csv << "level " << i << ": " << level << std::endl;
                    nodesTotal += level;
                }
                csv << "= " << nodesTotal << std::endl;
                csv << "--------------------------" << std::endl;
            }

            // svdag_occupancy_field
            {
                csv << "--------------------------" << std::endl;
                csv << "svdag_occupancy_field" << std::endl;
                std::vector<DAG::DAGLevel> sumLevels;
                for (const auto &type: std::filesystem::directory_iterator(m_data + "/" + m_scene + "/" + stringSVDAG(false) + "/lod_data")) {
                    csv << type.path().filename() << std::endl;
                    std::vector<DAG::DAGLevel> levels;
                    uint64_t bytesLODData = std::filesystem::file_size(type.path());
                    uint64_t bytesPerLODData = sizeof(DAG::DAGLevel);
                    uint64_t numLevel = bytesLODData / bytesPerLODData;
                    levels.resize(numLevel);
                    std::ifstream(type.path(), std::ios::binary).read(reinterpret_cast<char *>(levels.data()), static_cast<std::streamsize>(bytesLODData));

                    uint64_t nodesTotal = 0;
                    for (uint32_t i = 0; i < levels.size(); i++) {
                        const auto &level = levels[i];
                        csv << "level " << i << ": " << level.count << std::endl;
                        nodesTotal += level.count;
                        if (numLevel > sumLevels.size()) {
                            sumLevels.resize(numLevel);
                        }
                        sumLevels[i].count += level.count;
                    }
                    csv << "= " << nodesTotal << std::endl;
                }
                csv << "sum" << std::endl;
                uint64_t nodesTotal = 0;
                for (uint32_t i = 0; i < sumLevels.size(); i++) {
                    const auto &level = sumLevels[i];
                    csv << "level " << i << ": " << level.count << std::endl;
                    nodesTotal += level.count;
                }
                csv << "= " << nodesTotal << std::endl;
                csv << "--------------------------" << std::endl;
            }

            // svdag_occupancy_field_merged
            {
                csv << "--------------------------" << std::endl;
                csv << "svdag_occupancy_field_merged" << std::endl;
                csv << "types.bin" << std::endl;
                std::vector<DAG::DAGLevel> levels;
                uint64_t bytesLODData = std::filesystem::file_size(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod_data/" + m_prefixPlural + ".bin");
                uint64_t bytesPerLODData = sizeof(DAG::DAGLevel);
                uint64_t numLevel = bytesLODData / bytesPerLODData;
                levels.resize(numLevel);
                std::ifstream(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod_data/" + m_prefixPlural + ".bin", std::ios::binary).read(reinterpret_cast<char *>(levels.data()), static_cast<std::streamsize>(bytesLODData));

                uint64_t nodesTotal = 0;
                for (uint32_t i = 0; i < levels.size(); i++) {
                    const auto &level = levels[i];
                    csv << "level " << i << ": " << level.count << std::endl;
                    nodesTotal += level.count;
                }
                csv << "= " << nodesTotal << std::endl;
                csv << "--------------------------" << std::endl;
            }

            csv.close();
        }

        void nodeDegree() {
            std::ofstream csv;
            csv.open(m_data + "/" + m_scene + "/" + m_scene + "_nodedegree.txt");

            // svdag_occupancy_field_merged
            {
                csv << "--------------------------" << std::endl;
                csv << "svdag_occupancy_field_merged" << std::endl;
                std::vector<uint64_t> sumLevels;

                std::vector<std::vector<VoxelAABB>> aabbs;
                std::vector<DAG::DAGNode> lod;
                std::vector<DAG::DAGLevel> level;
                uint32_t numAABB;
                uint32_t numLOD;
                uint32_t numLevel;

                for (const auto &type: std::filesystem::directory_iterator(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/aabb")) {
                    uint64_t bytesAABB = std::filesystem::file_size(type.path());
                    uint64_t bytesPerAABB = sizeof(VoxelAABB);
                    numAABB = bytesAABB / bytesPerAABB;
                    aabbs.emplace_back();
                    aabbs.back().resize(numAABB);
                    std::ifstream(type.path(), std::ios::binary).read(reinterpret_cast<char *>(aabbs.back().data()), static_cast<std::streamsize>(bytesAABB));
                }

                {
                    const std::string filename = m_prefixPlural + ".bin";
                    uint64_t bytesLOD = std::filesystem::file_size(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod/" + filename);
                    uint64_t bytesPerLOD = sizeof(DAG::DAGNode);
                    numLOD = bytesLOD / bytesPerLOD;
                    lod.resize(numLOD);
                    std::ifstream(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod/" + filename, std::ios::binary).read(reinterpret_cast<char *>(lod.data()), static_cast<std::streamsize>(bytesLOD));

                    uint64_t bytesLODData = std::filesystem::file_size(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod_data/" + filename);
                    uint64_t bytesPerLODData = sizeof(DAG::DAGLevel);
                    numLevel = bytesLODData / bytesPerLODData;
                    level.resize(numLevel);
                    std::ifstream(m_data + "/" + m_scene + "/" + stringSVDAG(true) + "/lod_data/" + filename, std::ios::binary).read(reinterpret_cast<char *>(level.data()), static_cast<std::streamsize>(bytesLODData));
                }

                std::vector<uint64_t> visits(numLOD, 0);
                for (const auto &type: aabbs) {
                    for (const auto &aabb: type) {
                        std::vector<uint32_t> stack;
                        stack.push_back(aabb.lod);
                        while (!stack.empty()) {
                            const auto node = stack.back();
                            stack.pop_back();
                            if (node == DAG::invalidPointer()) {
                                continue;
                            }
                            visits[node]++;
                            const auto &n = lod[node];
                            if (n.isLeaf()) {
                                continue;
                            }
                            stack.push_back(n.child0);
                            stack.push_back(n.child1);
                            stack.push_back(n.child2);
                            stack.push_back(n.child3);
                            stack.push_back(n.child4);
                            stack.push_back(n.child5);
                            stack.push_back(n.child6);
                            stack.push_back(n.child7);
                        }
                    }
                }

                for (const auto &l: level) {
                    std::cout << "level " << l.index << ": " << l.count << std::endl;
                    std::sort(visits.begin() + l.index, visits.begin() + l.index + l.count);
                }

                std::vector<std::vector<std::pair<uint64_t, uint64_t>>> degree(numLevel); // visit count, number of nodes with that count
                for (uint32_t l = 0; l < level.size(); l++) {
                    const auto &lev = level[l];
                    uint64_t numVisits = UINT64_MAX;
                    for (uint32_t i = lev.index; i < lev.index + lev.count; i++) {
                        if (numVisits == UINT64_MAX || numVisits != visits[i]) {
                            numVisits = visits[i];
                            degree[l].emplace_back(numVisits, 1);
                            continue;
                        }
                        degree[l].back().second++;
                    }
                }

                for (uint32_t l = 0; l < degree.size(); l++) {
                    std::cout << "level " << l << std::endl;
                    std::cout << "node degree => #unique nodes with that degree" << std::endl;
                    for (const auto &d: degree[l]) {
                        std::cout << "(" << d.first << "," << d.second << ") ";
                    }
                    std::cout << std::endl;
                }

                std::cout << "--------------------------" << std::endl;
            }

            csv.close();
        }

    protected:
        std::string m_prefix;
        std::string m_prefixPlural;
        std::string m_data;
        std::string m_scene;

        bool m_svdagOccupancyField;
        std::string m_stringVoxels = "voxels";
        std::string m_stringSVO = "svo";
        std::string m_stringSVDAG = "svdag";
        std::string m_stringSVDAGOccupancyField = "occupancy_field";
        std::string m_stringSVDAGMerged = "merged";

        template<class T>
        static inline void hash_combine(std::size_t &seed, const T &v) {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        void createVoxelDirectories() const {
            std::filesystem::create_directories(m_data + "/" + m_scene + "/" + m_stringVoxels);
        }

        static void insertVoxel(std::map<uint32_t, std::vector<glm::ivec3>> &voxels, uint32_t typeId, const glm::ivec3 offset, const int32_t vx, const int32_t vy, const int32_t vz) {
            if (voxels.contains(typeId)) {
                voxels[typeId].emplace_back(offset.x + vx, offset.y + vy, offset.z + vz);
            } else {
                voxels.insert({typeId, {{offset.x + vx, offset.y + vy, offset.z + vz}}});
            }
        }
        static void insertVoxel(std::map<uint32_t, std::vector<glm::ivec4>> &voxels, uint32_t typeId, const glm::ivec3 offset, const int32_t vx, const int32_t vy, const int32_t vz, const uint32_t id) {
            if (voxels.contains(typeId)) {
                voxels[typeId].emplace_back(offset.x + vx, offset.y + vy, offset.z + vz, id);
            } else {
                voxels.insert({typeId, {{offset.x + vx, offset.y + vy, offset.z + vz, id}}});
            }
        }

        void writeVoxels(std::map<uint32_t, std::vector<glm::ivec3>> &voxels) const {
            for (auto &type: voxels) {
                std::ofstream(m_data + "/" + m_scene + "/" + m_stringVoxels + "/" + m_prefix + std::to_string(type.first) + ".bin", std::ios::binary | std::ios::app)
                        .write(reinterpret_cast<char *>(type.second.data()), static_cast<std::streamsize>(type.second.size() * sizeof(glm::ivec3)));

                std::cout << m_prefix << type.first << " has " << type.second.size() << " voxels." << std::endl;
            }
        }

        void writeVoxels(std::map<uint32_t, std::vector<glm::ivec4>> &voxels) const {
            for (auto &type: voxels) {
                std::ofstream(m_data + "/" + m_scene + "/" + m_stringVoxels + "/" + m_prefix + std::to_string(type.first) + ".bin", std::ios::binary | std::ios::app)
                        .write(reinterpret_cast<char *>(type.second.data()), static_cast<std::streamsize>(type.second.size() * sizeof(glm::ivec4)));

                std::cout << m_prefix << type.first << " has " << type.second.size() << " voxels." << std::endl;
            }
        }

        virtual void loadVoxels(const std::filesystem::directory_entry &type, uint32_t &numVoxels, std::map<uint32_t, std::pair<iAABB, std::vector<glm::ivec3>>> &voxels) const {
            std::vector<char> voxelsRaw;

            const uint64_t bytesVoxels = std::filesystem::file_size(type.path());
            if (bytesVoxels / sizeof(glm::ivec3) > UINT32_MAX) {
                throw std::runtime_error("numVoxels > UINT32_MAX");
            }
            numVoxels = bytesVoxels / sizeof(glm::ivec3);
            voxelsRaw.resize(bytesVoxels);
            std::ifstream(type.path(), std::ios::binary).read(voxelsRaw.data(), static_cast<std::streamsize>(bytesVoxels));

            auto *voxelsVec = reinterpret_cast<glm::ivec3 *>(voxelsRaw.data());

            // std::map<int32_t, std::map<int32_t, std::map<int32_t, bool>>> voxelsMap3D;
            // uint64_t duplicateVoxels = 0;

            for (uint32_t i = 0; i < numVoxels; i++) {
                auto voxel = voxelsVec[i];
                voxels[0].second.push_back(voxel);
                voxels[0].first.expand(voxel);
                voxels[0].first.expand(voxel + glm::ivec3(1, 1, 1));

                // if (voxelsMap3D.contains(voxel.x)) {
                //     if (voxelsMap3D[voxel.x].contains(voxel.y)) {
                //         if (voxelsMap3D[voxel.x][voxel.y].contains(voxel.z)) {
                //             // std::cout << "duplicate voxel: " << voxel.x << "," << voxel.y << "," << voxel.z << std::endl;
                //             duplicateVoxels++;
                //         } else {
                //             voxelsMap3D[voxel.x][voxel.y][voxel.z] = true;
                //         }
                //     } else {
                //         voxelsMap3D[voxel.x][voxel.y][voxel.z] = true;
                //     }
                // } else {
                //     voxelsMap3D[voxel.x][voxel.y][voxel.z] = true;
                // }
            }
            // std::cout << "duplicate voxels: " << duplicateVoxels << " total voxels: " << numVoxels << std::endl;
        }

        [[nodiscard]] virtual uint32_t toLabelId(const uint32_t typeId, const size_t instance) const {
            return typeId;
        }

        static void subdivide(std::vector<glm::ivec3> &voxels, uint32_t voxelIdx, uint32_t numVoxels, iAABB aabb, uint32_t labelId, std::vector<Octree::OctreeBuildInfo> &outOctreeBuildInfos) {
            if (uint32_t maxExtent = aabb.maxExtent(); maxExtent <= 16) {
                Octree::OctreeBuildInfo octreeBuildInfo{.labelId = labelId, .aabb = aabb};
                if (numVoxels > 16 * 16 * 16) {
                    throw std::runtime_error("numVoxels > 16 * 16 * 16");
                }
                octreeBuildInfo.voxels.insert(octreeBuildInfo.voxels.begin(), voxels.begin() + voxelIdx, voxels.begin() + voxelIdx + numVoxels);
                outOctreeBuildInfos.push_back(octreeBuildInfo);
                return;
            }

            int axis = aabb.maxExtentAxis(); // axis with the longest extent
            uint32_t numVoxelsHalf = numVoxels / 2;

            std::nth_element(voxels.begin() + voxelIdx, voxels.begin() + voxelIdx + numVoxelsHalf, voxels.begin() + voxelIdx + numVoxels,
                             [&axis](const glm::ivec3 &a, const glm::ivec3 &b) -> bool { return a[axis] < b[axis]; });

            iAABB firstAABB{};
            for (uint32_t i = voxelIdx; i < voxelIdx + numVoxelsHalf; i++) {
                firstAABB.expand(voxels[i]);
                firstAABB.expand(voxels[i] + glm::ivec3(1, 1, 1));
            }
            iAABB secondAABB{};
            for (uint32_t i = voxelIdx + numVoxelsHalf; i < voxelIdx + numVoxels; i++) {
                secondAABB.expand(voxels[i]);
                secondAABB.expand(voxels[i] + glm::ivec3(1, 1, 1));
            }

            subdivide(voxels, voxelIdx, numVoxelsHalf, firstAABB, labelId, outOctreeBuildInfos);
            subdivide(voxels, voxelIdx + numVoxelsHalf, numVoxels - numVoxelsHalf, secondAABB, labelId, outOctreeBuildInfos);
        }

        //  === FROM OCTREE ===
        typedef struct __attribute__((packed)) {
            uint8_t level;  // = 0xFF;// DAG::invalidPointer();
            uint32_t index; // = DAG::invalidPointer();
        } OctreeLevelIndex;

        struct OctreeLI {
            OctreeLevelIndex child0{};
            OctreeLevelIndex child1{};
            OctreeLevelIndex child2{};
            OctreeLevelIndex child3{};
            OctreeLevelIndex child4{};
            OctreeLevelIndex child5{};
            OctreeLevelIndex child6{};
            OctreeLevelIndex child7{};
        };

        static void svdagOccupancyField_fromOctree(const VoxelAABB *cells, Octree::OctreeNode *octrees, DAG::DAGRoot *dagRoot, const uint32_t dagRootCount, std::vector<DAG::DAGNode> &dag, std::vector<DAG::DAGLevel> &dagLevels) {
            std::vector<std::vector<OctreeLI>> dagHierarchy;
            std::vector<OctreeLevelIndex> rootIndex(dagRootCount);
            uint32_t maxLevel = 0;

            for (uint32_t i = 0; i < dagRootCount; i++) {
                const auto &cell = cells[i];
                uint32_t level;
                uint32_t index;
                svdagOccupancyField_traverseOctree(16, dagHierarchy, octrees, cell.lod, 0, &level, &index);
                rootIndex[i] = {static_cast<uint8_t>(level), index};
                maxLevel = glm::max(level, maxLevel);
            }

            dagLevels.resize(maxLevel + 1);
            uint32_t num = 0;
            for (const auto &level: dagHierarchy) {
                num += level.size();
            }
            dag.resize(num);
            svdagOccupancyField_encodeDAG(dagHierarchy, dag.data(), dagLevels);

            for (uint32_t i = 0; i < dagRootCount; i++) {
                const auto &root = rootIndex[i];
                dagRoot[i] = dagLevels[root.level].index + root.index;
            }
        }

        static void svdagOccupancyField_encodeDAG(const std::vector<std::vector<OctreeLI>> &dagHierarchy, DAG::DAGNode *dag, std::vector<DAG::DAGLevel> &dagLevels) {
            uint32_t globalIndex = 0;
            for (int32_t level = 0; level < dagHierarchy.size(); level++) {
                dagLevels[level].index = globalIndex;
                dagLevels[level].count = dagHierarchy[level].size();
                for (uint32_t i = 0; i < dagHierarchy[level].size(); i++) {
                    const auto &nodeHierarchy = dagHierarchy[level][i];

                    auto &node = dag[globalIndex];
                    const bool isLeaf = nodeHierarchy.child0.index == DAG::invalidPointer();
                    node.child0 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child0.level].index + (nodeHierarchy.child0.index);
                    node.child1 = isLeaf ? nodeHierarchy.child1.index : dagLevels[nodeHierarchy.child1.level].index + (nodeHierarchy.child1.index);
                    node.child2 = isLeaf ? nodeHierarchy.child2.index : dagLevels[nodeHierarchy.child2.level].index + (nodeHierarchy.child2.index);
                    node.child3 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child3.level].index + (nodeHierarchy.child3.index);
                    node.child4 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child4.level].index + (nodeHierarchy.child4.index);
                    node.child5 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child5.level].index + (nodeHierarchy.child5.index);
                    node.child6 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child6.level].index + (nodeHierarchy.child6.index);
                    node.child7 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child7.level].index + (nodeHierarchy.child7.index);

                    globalIndex++;
                }
            }
        }

        static uint32_t svdagOccupancyField_cellIndexToLinearIndex(const glm::ivec3 cell) {
            // cell \in [0, 3]^3
            // ZYX (4x4x4)
            if (cell.x < 0 || cell.x >= 4 || cell.y < 0 || cell.y >= 4 || cell.z < 0 || cell.z >= 4) {
                throw std::runtime_error("cell index out of bounds");
            }
            return cell.z * 16 + cell.y * 4 + cell.x;
        }

        static glm::ivec3 svdagOccupancyField_linearChildToVector(const uint32_t child) {
            // child \in [0, 7]
            if (child < 0 || child >= 8) {
                throw std::runtime_error("child index out of bounds");
            }
            return {child & 1, (child >> 1) & 1, (child >> 2) & 1};
        }

        static void svdagOccupancyField_setBitsInField(uint64_t &field, const uint32_t extent, const glm::ivec3 anchor) {
            for (int z = 0; z < extent; z++) {
                for (int y = 0; y < extent; y++) {
                    for (int x = 0; x < extent; x++) {
                        const uint32_t index = svdagOccupancyField_cellIndexToLinearIndex(anchor + glm::ivec3(x, y, z));
                        if (index >= 64) {
                            throw std::runtime_error("index out of bounds");
                        }
                        if ((field & (1ull << index)) != 0) {
                            throw std::runtime_error("bit already set");
                        }
                        field |= (1ull << index);
                    }
                }
            }
        }

        static void svdagOccupancyField_encodeBitField(uint64_t &field, const uint32_t extent, const glm::ivec3 anchor, Octree::OctreeNode *octrees, const uint32_t rootIndex, const uint32_t index) {
            if (extent < 1 || extent > 4) {
                throw std::runtime_error("extent out of bounds");
            }

            // ZYX
            const auto &octree = octrees[rootIndex + index];

            const uint16_t solid = OCTREE_NODE_SOLID(octree);
            const uint16_t child = OCTREE_NODE_CHILD(octree);

            if (child == OCTREE_NODE_INVALID_CHILD) {
                if (solid != 0) {
                    svdagOccupancyField_setBitsInField(field, extent, anchor);
                }
                return;
            }

            const uint32_t nextExtent = extent >> 1;

            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(0), octrees, rootIndex, child + 0);
            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(1), octrees, rootIndex, child + 1);
            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(2), octrees, rootIndex, child + 2);
            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(3), octrees, rootIndex, child + 3);
            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(4), octrees, rootIndex, child + 4);
            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(5), octrees, rootIndex, child + 5);
            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(6), octrees, rootIndex, child + 6);
            svdagOccupancyField_encodeBitField(field, nextExtent, anchor + static_cast<int32_t>(nextExtent) * svdagOccupancyField_linearChildToVector(7), octrees, rootIndex, child + 7);
        }

        static void svdagOccupancyField_traverseOctree(uint32_t extent, std::vector<std::vector<OctreeLI>> &dag, Octree::OctreeNode *octrees, uint32_t rootIndex, uint32_t index, uint32_t *childLevel,
                                                       uint32_t *childIndex) {
            auto &octree = octrees[rootIndex + index];

            uint16_t solid = OCTREE_NODE_SOLID(octree);
            uint16_t child = OCTREE_NODE_CHILD(octree);

            if (extent == 4) {
                // early exit for DDA bitfield encoding (4x4x4)
                uint64_t encoding = 0;
                svdagOccupancyField_encodeBitField(encoding, extent, glm::ivec3(0), octrees, rootIndex, index);
                uint32_t bitFieldUpperBits = (encoding >> 32) & 0xFFFFFFFF;
                uint32_t bitFieldLowerBits = encoding & 0xFFFFFFFF;
                OctreeLI node{.child0{0xFF, DAG::invalidPointer()},
                              .child1{0xFF, bitFieldUpperBits},
                              .child2{0xFF, bitFieldLowerBits},
                              .child3{0xFF, DAG::invalidPointer()},
                              .child4{0xFF, DAG::invalidPointer()},
                              .child5{0xFF, DAG::invalidPointer()},
                              .child6{0xFF, DAG::invalidPointer()},
                              .child7{0xFF, DAG::invalidPointer()}};
                if (dag.empty()) {
                    dag.resize(1);
                }
                dag[0].push_back(node);
                *childLevel = 0;
                *childIndex = dag[0].size() - 1;
                return;
            }
            if (extent <= 4) {
                throw std::runtime_error("extent <= 4");
            }

            if (child == OCTREE_NODE_INVALID_CHILD) {
                OctreeLI node{.child0{0xFF, DAG::invalidPointer()},
                              .child1{0xFF, solid == 1 ? 0xFFFFFFFF : 0x00000000},
                              .child2{0xFF, solid == 1 ? 0xFFFFFFFF : 0x00000000},
                              .child3{0xFF, DAG::invalidPointer()},
                              .child4{0xFF, DAG::invalidPointer()},
                              .child5{0xFF, DAG::invalidPointer()},
                              .child6{0xFF, DAG::invalidPointer()},
                              .child7{0xFF, DAG::invalidPointer()}};
                if (dag.empty()) {
                    dag.resize(1);
                }
                dag[0].push_back(node);
                *childLevel = 0;
                *childIndex = dag[0].size() - 1;
                return;
            }

            uint32_t nextExtent = extent >> 1;

            uint32_t child0Level;
            uint32_t child0Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 0, &child0Level, &child0Index);
            uint32_t child1Level;
            uint32_t child1Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 1, &child1Level, &child1Index);
            uint32_t child2Level;
            uint32_t child2Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 2, &child2Level, &child2Index);
            uint32_t child3Level;
            uint32_t child3Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 3, &child3Level, &child3Index);
            uint32_t child4Level;
            uint32_t child4Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 4, &child4Level, &child4Index);
            uint32_t child5Level;
            uint32_t child5Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 5, &child5Level, &child5Index);
            uint32_t child6Level;
            uint32_t child6Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 6, &child6Level, &child6Index);
            uint32_t child7Level;
            uint32_t child7Index;
            svdagOccupancyField_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 7, &child7Level, &child7Index);

            uint32_t maxChildLevel =
                    std::max(child0Level, std::max(child1Level, std::max(child2Level, std::max(child3Level, std::max(child4Level, std::max(child5Level, std::max(child6Level, child7Level)))))));
            *childLevel = maxChildLevel + 1;

            OctreeLI node{.child0 = {static_cast<uint8_t>(child0Level), child0Index},
                          .child1 = {static_cast<uint8_t>(child1Level), child1Index},
                          .child2 = {static_cast<uint8_t>(child2Level), child2Index},
                          .child3 = {static_cast<uint8_t>(child3Level), child3Index},
                          .child4 = {static_cast<uint8_t>(child4Level), child4Index},
                          .child5 = {static_cast<uint8_t>(child5Level), child5Index},
                          .child6 = {static_cast<uint8_t>(child6Level), child6Index},
                          .child7 = {static_cast<uint8_t>(child7Level), child7Index}};
            if (dag.size() < maxChildLevel + 2) {
                dag.resize(maxChildLevel + 2);
            }
            dag[*childLevel].push_back(node);

            *childIndex = dag[*childLevel].size() - 1;
        }

        static void svdag_fromOctree(const VoxelAABB *cells, Octree::OctreeNode *octrees, DAG::DAGRoot *dagRoot, const uint32_t dagRootCount, std::vector<DAG::DAGNode> &dag, std::vector<DAG::DAGLevel> &dagLevels) {
            std::vector<std::vector<OctreeLI>> dagHierarchy;
            std::vector<OctreeLevelIndex> rootIndex(dagRootCount);
            uint32_t maxLevel = 0;

            for (uint32_t i = 0; i < dagRootCount; i++) {
                const auto &cell = cells[i];
                uint32_t level;
                uint32_t index;
                svdag_traverseOctree(16, dagHierarchy, octrees, cell.lod, 0, &level, &index);
                rootIndex[i] = {static_cast<uint8_t>(level), index};
                maxLevel = glm::max(level, maxLevel);
            }

            dagLevels.resize(maxLevel + 1);
            uint32_t num = 0;
            for (const auto &level: dagHierarchy) {
                num += level.size();
            }
            dag.resize(num);
            svdag_encodeDAG(dagHierarchy, dag.data(), dagLevels);

            for (uint32_t i = 0; i < dagRootCount; i++) {
                const auto &root = rootIndex[i];
                dagRoot[i] = dagLevels[root.level].index + root.index;
            }
        }

        static void svdag_encodeDAG(const std::vector<std::vector<OctreeLI>> &dagHierarchy, DAG::DAGNode *dag, std::vector<DAG::DAGLevel> &dagLevels) {
            uint32_t globalIndex = 0;
            for (int32_t level = 0; level < dagHierarchy.size(); level++) {
                dagLevels[level].index = globalIndex;
                dagLevels[level].count = dagHierarchy[level].size();
                for (uint32_t i = 0; i < dagHierarchy[level].size(); i++) {
                    const auto &nodeHierarchy = dagHierarchy[level][i];

                    auto &node = dag[globalIndex];
                    const bool isLeaf = nodeHierarchy.child0.index == DAG::invalidPointer();
                    node.child0 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child0.level].index + (nodeHierarchy.child0.index);
                    node.child1 = isLeaf ? nodeHierarchy.child1.index : dagLevels[nodeHierarchy.child1.level].index + (nodeHierarchy.child1.index);
                    node.child2 = isLeaf ? nodeHierarchy.child2.index : dagLevels[nodeHierarchy.child2.level].index + (nodeHierarchy.child2.index);
                    node.child3 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child3.level].index + (nodeHierarchy.child3.index);
                    node.child4 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child4.level].index + (nodeHierarchy.child4.index);
                    node.child5 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child5.level].index + (nodeHierarchy.child5.index);
                    node.child6 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child6.level].index + (nodeHierarchy.child6.index);
                    node.child7 = isLeaf ? DAG::invalidPointer() : dagLevels[nodeHierarchy.child7.level].index + (nodeHierarchy.child7.index);

                    globalIndex++;
                }
            }
        }

        static void svdag_traverseOctree(const uint32_t extent, std::vector<std::vector<OctreeLI>> &dag, Octree::OctreeNode *octrees, const uint32_t rootIndex, const uint32_t index, uint32_t *childLevel, uint32_t *childIndex) {
            auto &octree = octrees[rootIndex + index];

            const uint16_t solid = OCTREE_NODE_SOLID(octree);
            const uint16_t child = OCTREE_NODE_CHILD(octree);

            if (extent <= 1 && child != OCTREE_NODE_INVALID_CHILD) {
                throw std::runtime_error("extent <= 1 but child not invalid");
            }

            if (child == OCTREE_NODE_INVALID_CHILD) {
                OctreeLI node{.child0{0xFF, DAG::invalidPointer()},
                              .child1{0xFF, solid == 1 ? 0xFFFFFFFF : 0x00000000},
                              .child2{0xFF, solid == 1 ? 0xFFFFFFFF : 0x00000000},
                              .child3{0xFF, DAG::invalidPointer()},
                              .child4{0xFF, DAG::invalidPointer()},
                              .child5{0xFF, DAG::invalidPointer()},
                              .child6{0xFF, DAG::invalidPointer()},
                              .child7{0xFF, DAG::invalidPointer()}};
                if (dag.empty()) {
                    dag.resize(1);
                }
                dag[0].push_back(node);
                *childLevel = 0;
                *childIndex = dag[0].size() - 1;
                return;
            }

            uint32_t nextExtent = extent >> 1;

            uint32_t child0Level;
            uint32_t child0Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 0, &child0Level, &child0Index);
            uint32_t child1Level;
            uint32_t child1Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 1, &child1Level, &child1Index);
            uint32_t child2Level;
            uint32_t child2Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 2, &child2Level, &child2Index);
            uint32_t child3Level;
            uint32_t child3Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 3, &child3Level, &child3Index);
            uint32_t child4Level;
            uint32_t child4Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 4, &child4Level, &child4Index);
            uint32_t child5Level;
            uint32_t child5Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 5, &child5Level, &child5Index);
            uint32_t child6Level;
            uint32_t child6Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 6, &child6Level, &child6Index);
            uint32_t child7Level;
            uint32_t child7Index;
            svdag_traverseOctree(nextExtent, dag, octrees, rootIndex, child + 7, &child7Level, &child7Index);

            uint32_t maxChildLevel =
                    std::max(child0Level, std::max(child1Level, std::max(child2Level, std::max(child3Level, std::max(child4Level, std::max(child5Level, std::max(child6Level, child7Level)))))));
            *childLevel = maxChildLevel + 1;

            OctreeLI node{.child0 = {static_cast<uint8_t>(child0Level), child0Index},
                          .child1 = {static_cast<uint8_t>(child1Level), child1Index},
                          .child2 = {static_cast<uint8_t>(child2Level), child2Index},
                          .child3 = {static_cast<uint8_t>(child3Level), child3Index},
                          .child4 = {static_cast<uint8_t>(child4Level), child4Index},
                          .child5 = {static_cast<uint8_t>(child5Level), child5Index},
                          .child6 = {static_cast<uint8_t>(child6Level), child6Index},
                          .child7 = {static_cast<uint8_t>(child7Level), child7Index}};
            if (dag.size() < maxChildLevel + 2) {
                dag.resize(maxChildLevel + 2);
            }
            dag[*childLevel].push_back(node);

            *childIndex = dag[*childLevel].size() - 1;
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

        static void svo_traverseOctreeRecordNodeInfo(const uint32_t extent, std::vector<uint64_t> &levels, Octree::OctreeNode *octrees, const uint32_t rootIndex, const uint32_t index, uint32_t *childLevel) {
            auto &octree = octrees[rootIndex + index];

            uint16_t solid = OCTREE_NODE_SOLID(octree);
            uint16_t child = OCTREE_NODE_CHILD(octree);

            if (extent <= 1 && child != OCTREE_NODE_INVALID_CHILD) {
                throw std::runtime_error("extent <= 1 but child not invalid");
            }

            if (child == OCTREE_NODE_INVALID_CHILD) {
                *childLevel = 0;

                if (*childLevel >= levels.size()) {
                    levels.resize(*childLevel + 1);
                }
                levels[*childLevel]++;

                return;
            }

            const uint32_t nextExtent = extent >> 1;

            uint32_t child0Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 0, &child0Level);
            uint32_t child1Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 1, &child1Level);
            uint32_t child2Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 2, &child2Level);
            uint32_t child3Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 3, &child3Level);
            uint32_t child4Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 4, &child4Level);
            uint32_t child5Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 5, &child5Level);
            uint32_t child6Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 6, &child6Level);
            uint32_t child7Level;
            svo_traverseOctreeRecordNodeInfo(nextExtent, levels, octrees, rootIndex, child + 7, &child7Level);

            const uint32_t maxChildLevel =
                    std::max(child0Level, std::max(child1Level, std::max(child2Level, std::max(child3Level, std::max(child4Level, std::max(child5Level, std::max(child6Level, child7Level)))))));
            *childLevel = maxChildLevel + 1;

            if (*childLevel >= levels.size()) {
                levels.resize(*childLevel + 1);
            }
            levels[*childLevel]++;
        }
    };
} // namespace raven