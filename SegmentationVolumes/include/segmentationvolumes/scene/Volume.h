#pragma once

#include "VolumeAABB.h"
#include "VolumeLOD.h"
#include "imgui.h"
#include "pugixml/src/pugixml.hpp"
#include "raven/core/AccelerationStructure.h"

#include <string>
#include <utility>

namespace raven {
    enum VolumeType {
        VOLUME_TYPE_NASTJA,
        VOLUME_TYPE_MOUSE_CORTEX,
        VOLUME_TYPE_WORM,
    };

    class Volume {
    public:
        Volume(std::string dataPath, std::string folder, std::string name, const glm::vec3 translate, const glm::vec3 scale, const VolumeType type) : m_dataPath(std::move(dataPath)), m_folder(std::move(folder)), m_name(std::move(name)), m_translate(translate), m_scale(scale), m_type(type) {}

        static std::shared_ptr<Volume> create(const std::string &dataPath, const std::string &name, const std::string &translate, const std::string &scale) {
            const std::string volumePath = Paths::m_resourceDirectoryPath + "/scenes/volumes/" + name + ".xml";
            if (!std::filesystem::exists(volumePath)) {
                std::cout << name << ": Volume not found... " << volumePath << std::endl;
                return nullptr;
            }

            glm::vec3 translateVec;
            if (!vec3FromString(translate, &translateVec)) {
                std::cout << name << ": Cannot parse translate " << translate << "." << std::endl;
                return nullptr;
            }
            glm::vec3 scaleVec;
            if (!vec3FromString(scale, &scaleVec)) {
                std::cout << name << ": Cannot parse scale " << scale << "." << std::endl;
                return nullptr;
            }

            // load rest
            pugi::xml_document doc;
            if (const pugi::xml_parse_result result = doc.load_file(volumePath.c_str()); !result) {
                std::cerr << "XML [" << volumePath << "] parsed with errors." << std::endl;
                std::cerr << "Error description: " << result.description() << std::endl;
                std::cerr << "Error offset: " << result.offset << std::endl;
                return nullptr;
            }

            std::shared_ptr<Volume> volumeObject;
            {
                const auto &info = doc.child("volume").child("info");
                if (info.empty() || info.attribute("type").empty() || info.attribute("folder").empty()) {
                    std::cout << name << ": Info not found or incomplete." << std::endl;
                    return nullptr;
                }

                const std::string type = info.attribute("type").as_string();
                VolumeType volumeType;
                if (type == "nastja") {
                    volumeType = VOLUME_TYPE_NASTJA;
                } else if (type == "mouse") {
                    volumeType = VOLUME_TYPE_MOUSE_CORTEX;
                } else if (type == "celegans") {
                    volumeType = VOLUME_TYPE_WORM;
                } else {
                    std::cout << name << ": Unknown volume type " << type << "." << std::endl;
                    return nullptr;
                }

                volumeObject = std::make_shared<Volume>(dataPath, info.attribute("folder").as_string(), name, translateVec, scaleVec, volumeType);
            }

            // load AABBs + LODs
            for (const auto &volume: doc.child("volume").child("aabbs")) {
                std::string lodKey = volume.attribute("lod").value();
                bool enabled = true;
                if (!volume.attribute("enabled").empty()) {
                    enabled = volume.attribute("enabled").as_bool();
                }

                auto aabb = VolumeAABB::create(dataPath, volumeObject->m_folder,
                                               volume.attribute("name").value(),
                                               volume.attribute("type").value(),
                                               lodKey,
                                               enabled);
                if (!aabb) {
                    continue;
                }
                volumeObject->m_aabbs.push_back(aabb);

                // LOD
                if (!volumeObject->m_lods.contains(lodKey)) {
                    std::string query = "/volume/lods/lod[@key=" + lodKey + "]";
                    if (auto lodNode = doc.select_node(query.c_str())) {
                        const auto lod = VolumeLOD::create(dataPath, volumeObject->m_folder,
                                                           lodNode.node().attribute("name").value(),
                                                           lodKey,
                                                           lodNode.node().attribute("type").value());
                        if (!lod) {
                            continue;
                        }
                        volumeObject->m_lods[lodKey] = lod;
                    } else {
                        std::cout << name << ": LOD key not found in scene file " << lodKey << "." << std::endl;
                    }
                }
            }

            for (const auto &[_, lod]: volumeObject->m_lods) {
                if (volumeObject->m_lodType < 0) {
                    volumeObject->m_lodType = lod->getType();
                }
                if (volumeObject->m_lodType != lod->getType()) {
                    std::cout << name << ": LOD types do not match." << std::endl;
                    return nullptr;
                }
            }

            return volumeObject;
        }

        void release() const {
            RAVEN_BUFFER_RELEASE(m_aabbBuffer);
            RAVEN_AS_RELEASE(m_blas);
            RAVEN_BUFFER_RELEASE(m_lodBuffer);
        }

        void enableAllAABBs() {
            for (const auto &aabb: m_aabbs) {
                aabb->setEnabled(true);
            }
        }

        void disableAllAABBs() {
            for (const auto &aabb: m_aabbs) {
                aabb->setEnabled(false);
            }
        }

        void loadData(GPUContext *gpuContext) {
            // aabb
            for (const auto &aabb: m_aabbs) {
                aabb->loadData();
            }

            // lod
            if (!m_lods.empty()) {
                uint64_t lodSize = 0;
                for (const auto &[key, lod]: m_lods) {
                    lod->setLODOffset(lodSize);
                    lodSize += lod->loadDataSize();
                }

                std::vector<char> lodRaw(lodSize);
                for (const auto &[key, lod]: m_lods) {
                    lod->loadData(lodRaw);
                }

                // LOD buffer
                const std::string bufferName = "LODBuffer[" + m_name + "]";
                const auto settings = Buffer::BufferSettings{.m_sizeBytes = lodSize,
                                                             .m_bufferUsages = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                             .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                             .m_memoryAllocateFlagBits = vk::MemoryAllocateFlagBits::eDeviceAddress,
                                                             .m_name = bufferName};
                m_lodBuffer = Buffer::fillDeviceWithStagingBuffer(gpuContext, settings, lodRaw.data());
                m_lodBufferAddress = m_lodBuffer->getDeviceAddress();
                // m_lodBufferSize = lodSize;
            }
        }

        void buildBLAS(GPUContext *gpuContext) {
            RAVEN_BUFFER_RELEASE(m_aabbBuffer);
            m_aabbBuffer = nullptr;
            RAVEN_AS_RELEASE(m_blas);
            m_blas = nullptr;

            std::vector<VoxelAABB> aabbs;
            std::vector<vk::AabbPositionsKHR> blasAABBs;
            for (const auto &aabb: m_aabbs) {
                uint32_t lodOffset = 0;
                if (m_lods.contains(aabb->getLodKey())) {
                    const uint64_t res = m_lods[aabb->getLodKey()]->getLODOffset() / m_lods[aabb->getLodKey()]->getSizeofLOD();
                    if (res > 0xFFFFFFFF) {
                        throw std::runtime_error("LOD offset too large for 32bit pointer.");
                    }
                    lodOffset = static_cast<uint32_t>(res);
                }
                aabb->recordAABBs(aabbs, blasAABBs, lodOffset);
            }

            // iAABB tempAABB{};
            // for (const auto &aabb : aabbs) {
            //     tempAABB.expand({aabb.minX, aabb.minY, aabb.minZ});
            //     tempAABB.expand({aabb.maxX, aabb.maxY, aabb.maxZ});
            // }
            // std::cout << "Volume " << m_name << " AABB: " << tempAABB << std::endl;
            // const uint64_t xExtent = tempAABB.m_max.x - tempAABB.m_min.x;
            // const uint64_t yExtent = tempAABB.m_max.y - tempAABB.m_min.y;
            // const uint64_t zExtent = tempAABB.m_max.z - tempAABB.m_min.z;
            // std::cout << "Volume " << m_name << " AABB extent: " << xExtent << " " << yExtent << " " << zExtent << std::endl;
            // std::cout << "Volume " << m_name << " AABB volume: " << xExtent * yExtent * zExtent << std::endl;
            // std::cout << "Volume " << m_name << " AABB volume (4 bytes per label): " << xExtent * yExtent * zExtent * 4 << std::endl;
            // std::cout << "Volume " << m_name << " AABB volume (4 bytes per label) [MB]: " << static_cast<float>(xExtent * yExtent * zExtent * 4) * glm::pow(10, -6) << std::endl;

            if (aabbs.empty() || blasAABBs.empty()) {
                return;
            }

            {
                // AABB buffer
                const std::string bufferName = "AABBBuffer[" + m_name + "]";
                const auto settings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(aabbs.size() * sizeof(VoxelAABB)),
                                                             .m_bufferUsages = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                             .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                             .m_memoryAllocateFlagBits = vk::MemoryAllocateFlagBits::eDeviceAddress,
                                                             .m_name = bufferName};
                m_aabbBuffer = Buffer::fillDeviceWithStagingBuffer(gpuContext, settings, aabbs.data());
            }

            {
                // build BLAS
                const auto settings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(sizeof(vk::AabbPositionsKHR) * blasAABBs.size()),
                                                             .m_bufferUsages = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                             .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                             .m_memoryAllocateFlagBits = vk::MemoryAllocateFlagBits::eDeviceAddress,
                                                             .m_name = "BLASAABBBuffer"};
                const auto blasAABBBuffer = Buffer::fillDeviceWithStagingBuffer(gpuContext, settings, blasAABBs.data());
                m_blas = std::make_shared<AabbBLAS>(gpuContext, blasAABBBuffer->getDeviceAddress(), sizeof(vk::AabbPositionsKHR), blasAABBs.size());
                m_blas->build();
                blasAABBBuffer->release();
            }

            m_aabbBufferAddress = m_aabbBuffer->getDeviceAddress();
        }

        [[nodiscard]] std::optional<ObjectDescriptor> createObjectDescriptor() const {
            if (!m_aabbBuffer) {
                constexpr std::optional<ObjectDescriptor> objectDescriptor;
                return objectDescriptor;
            }
            auto objectDescriptor = std::make_optional<ObjectDescriptor>();
            objectDescriptor.value().aabbAddress = m_aabbBufferAddress;
            objectDescriptor.value().lodAddress = m_lodBufferAddress;
            return objectDescriptor;
        }

        std::optional<TLAS::BLASHandle> createBLASHandle(uint32_t &instanceCount) const {
            if (m_blas) {
                auto handle = std::make_optional<TLAS::BLASHandle>();
                handle.value().m_blas = m_blas;
                handle.value().m_instanceCustomIndex = instanceCount;
                handle.value().m_transform = std::array<std::array<float, 4>, 3>{m_scale.x, 0.0f, 0.0f, m_translate.x, 0.0f, m_scale.y, 0.0f, m_translate.y, 0.0f, 0.0f, m_scale.z, m_translate.z};
                instanceCount++;
                return handle;
            }
            std::optional<TLAS::BLASHandle> handle;
            return handle;
        }

        void recordHierarchyGUI(const std::function<void()> &rebuildTLASFunction, bool *updateTLAS) {
            if (ImGui::TreeNodeEx(m_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (const auto &aabb: m_aabbs) {
                    aabb->recordHierarchyGUI(rebuildTLASFunction, updateTLAS);
                }
                ImGui::TreePop();
            }
        }

        [[nodiscard]] const std::string &getName() const { return m_name; }
        [[nodiscard]] VolumeType getType() const { return m_type; }
        [[nodiscard]] const std::string &getFolder() const { return m_folder; }

        [[nodiscard]] size_t getAABBCount() const { return m_aabbs.size(); }

        [[nodiscard]] uint64_t getAABBBufferSize() const {
            if (!m_aabbBuffer) {
                return 0;
            }
            return m_aabbBuffer->getSizeBytes();
        }
        [[nodiscard]] uint64_t getBLASSize() const {
            if (!m_blas) {
                return 0;
            }
            return m_blas->getSizeBytes();
        }
        [[nodiscard]] uint64_t getLODBufferSize() const {
            if (!m_lodBuffer) {
                return 0;
            }
            return m_lodBuffer->getSizeBytes();
            // return m_lodBufferSize;
        }

        [[nodiscard]] int32_t getLODType() const { return m_lodType; }

    private:
        std::string m_dataPath;
        std::string m_folder;
        std::string m_name;

        glm::vec3 m_translate;
        glm::vec3 m_scale;
        VolumeType m_type;

        std::vector<std::shared_ptr<VolumeAABB>> m_aabbs;
        std::map<std::string, std::shared_ptr<VolumeLOD>> m_lods;
        int32_t m_lodType = -1;

        std::shared_ptr<Buffer> m_aabbBuffer;
        vk::DeviceAddress m_aabbBufferAddress{};
        std::shared_ptr<AabbBLAS> m_blas;

        std::shared_ptr<Buffer> m_lodBuffer;
        vk::DeviceAddress m_lodBufferAddress{};
        // uint64_t m_lodBufferSize{};

        static bool vec3FromString(const std::string &str, glm::vec3 *vec) {
            const std::regex rgx("([+-]?[0-9]*[.]?[0-9]+) ([+-]?[0-9]*[.]?[0-9]+) ([+-]?[0-9]*[.]?[0-9]+)");
            std::smatch matches;
            std::regex_search(str, matches, rgx);
            if (matches.size() == 4) {
                vec->x = std::stof(matches[1]);
                vec->y = std::stof(matches[2]);
                vec->z = std::stof(matches[3]);
                return true;
            }
            return false;
        }
    };
} // namespace raven