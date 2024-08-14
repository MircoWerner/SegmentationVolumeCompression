#pragma once

#include "../Raystructs.h"
#include "imgui.h"

#include <string>
#include <utility>
#include <filesystem>

namespace raven {
    enum VolumeAABBType {
        AABB_TYPE_DEFAULT,
    };

    class VolumeAABB {
    public:
        VolumeAABB(std::string dataPath, std::string folder, std::string name, const VolumeAABBType type, std::string lodKey, const bool enabled) : m_dataPath(std::move(dataPath)), m_folder(std::move(folder)), m_name(std::move(name)), m_type(type), m_lodKey(std::move(lodKey)), m_enabled(enabled) {}

        static std::shared_ptr<VolumeAABB> create(const std::string &dataPath, const std::string &folder, const std::string &name, const std::string &type, const std::string &lodKey, const bool enabled) {
            if (!std::filesystem::exists(dataPath + "/" + folder + "/aabb/" + name + ".bin")) {
                std::cout << name << ": AABB not found." << std::endl;
                return nullptr;
            }

            VolumeAABBType aabbType;
            if (type == "default") {
                aabbType = AABB_TYPE_DEFAULT;
            } else {
                std::cout << name << ": Unknown AABB type " << type << "." << std::endl;
                return nullptr;
            }

            return std::make_shared<VolumeAABB>(dataPath, folder, name, aabbType, lodKey, enabled);
        }

        void loadData() {
            std::vector<char> aabbsRaw;

            const uint32_t bytesAABBs = std::filesystem::file_size(m_dataPath + "/" + m_folder + "/aabb/" + m_name + ".bin");
            const uint32_t numAABBs = bytesAABBs / sizeof(VoxelAABB);
            aabbsRaw.resize(bytesAABBs);
            std::ifstream(m_dataPath + "/" + m_folder + "/aabb/" + m_name + ".bin", std::ios::binary).read(aabbsRaw.data(), bytesAABBs);

            const auto *aabbs = reinterpret_cast<VoxelAABB *>(aabbsRaw.data());

            for (uint32_t i = 0; i < numAABBs; i++) {
                // if (aabbs[i].materialId < 128) {
                // continue;
                // }
                m_aabbs.push_back(aabbs[i]);
            }
        }

        void recordHierarchyGUI(const std::function<void()> &rebuildTLASFunction, bool *updateTLAS) {
            const std::string label = m_name;
            if (ImGui::Checkbox(label.c_str(), &m_enabled)) {
                rebuildTLASFunction();
                *updateTLAS = true;
            }
        }

        void recordAABBs(std::vector<VoxelAABB> &aabbs, std::vector<vk::AabbPositionsKHR> &blasAABBs, const uint32_t lodOffset) {
            if (!m_enabled) {
                return;
            }

            for (const auto &aabb : m_aabbs) {
                aabbs.push_back({.minX = aabb.minX, .minY = aabb.minY, .minZ = aabb.minZ, .maxX = aabb.maxX, .maxY = aabb.maxY, .maxZ = aabb.maxZ, .labelId = aabb.labelId, .lod = aabb.lod + lodOffset});
                blasAABBs.push_back(aabb.toVkAABBPosition());
            }
        }

        [[nodiscard]] const std::string &getName() const { return m_name; }
        [[nodiscard]] VolumeAABBType getType() const { return m_type; }
        [[nodiscard]] const std::string &getFolder() const { return m_folder; }
        [[nodiscard]] const std::string &getLodKey() const { return m_lodKey; }

        [[nodiscard]] bool isEnabled() const { return m_enabled; }
        void setEnabled(const bool enabled) { m_enabled = enabled; }

    private:
        std::string m_dataPath;
        std::string m_folder;
        std::string m_name;

        VolumeAABBType m_type;
        std::string m_lodKey;
        bool m_enabled;

        std::vector<VoxelAABB> m_aabbs;
    };
} // namespace raven