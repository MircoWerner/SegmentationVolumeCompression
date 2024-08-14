#pragma once

#include "../Raystructs.h"

#include <string>

namespace raven {
    enum VolumeLODType {
        LOD_TYPE_SVO = 0,
        LOD_TYPE_SVDAG = 1,
        LOD_TYPE_SVDAG_OCCUPANCY_FIELD = 2,
    };

    class VolumeLOD {
    public:
        VolumeLOD(std::string dataPath, std::string folder, std::string name, std::string key, const VolumeLODType type) : m_dataPath(std::move(dataPath)), m_folder(std::move(folder)), m_name(std::move(name)), m_key(std::move(key)), m_type(type) {}

        static std::shared_ptr<VolumeLOD> create(const std::string &dataPath, const std::string &folder, const std::string &name, const std::string &key, const std::string &type) {
            if (!std::filesystem::exists(dataPath + "/" + folder + "/lod/" + name + ".bin")) {
                std::cout << name << ": LOD not found." << std::endl;
                return nullptr;
            }

            VolumeLODType lodType;
            if (type == "svo") {
                lodType = LOD_TYPE_SVO;
            } else if (type == "svdag") {
                lodType = LOD_TYPE_SVDAG;
            } else if (type == "svdag_occupancy_field") {
                lodType = LOD_TYPE_SVDAG_OCCUPANCY_FIELD;
            } else {
                std::cout << name << ": Unknown LOD type " << type << "." << std::endl;
                return nullptr;
            }

            return std::make_shared<VolumeLOD>(dataPath, folder, name, key, lodType);
        }

        [[nodiscard]] uint64_t loadDataSize() const {
            return std::filesystem::file_size(m_dataPath + "/" + m_folder + "/lod/" + m_name + ".bin");
        }

        void loadData(std::vector<char> &lodRaw) const {
            const uint64_t bytesLOD = std::filesystem::file_size(m_dataPath + "/" + m_folder + "/lod/" + m_name + ".bin");
            std::ifstream(m_dataPath + "/" + m_folder + "/lod/" + m_name + ".bin", std::ios::binary).read(lodRaw.data() + m_lodOffset, static_cast<std::streamsize>(bytesLOD));

            if ((m_type == LOD_TYPE_SVDAG || m_type == LOD_TYPE_SVDAG_OCCUPANCY_FIELD) && m_lodOffset > 0) {
                const uint32_t offset = m_lodOffset / getSizeofLOD();
                const uint32_t numLODs = bytesLOD / getSizeofLOD();
                auto *lod = reinterpret_cast<SVDAG *>(lodRaw.data() + m_lodOffset);
                for (uint32_t i = 0; i < numLODs; i++) {
                    if (lod[i].isLeaf()) {
                        continue;
                    }
                    lod[i].child0 += offset;
                    lod[i].child1 += offset;
                    lod[i].child2 += offset;
                    lod[i].child3 += offset;
                    lod[i].child4 += offset;
                    lod[i].child5 += offset;
                    lod[i].child6 += offset;
                    lod[i].child7 += offset;
                }
            }
        }

        [[nodiscard]] const std::string &getKey() const { return m_key; }
        [[nodiscard]] const std::string &getName() const { return m_name; }
        [[nodiscard]] VolumeLODType getType() const { return m_type; }
        [[nodiscard]] const std::string &getFolder() const { return m_folder; }
        [[nodiscard]] uint64_t getLODOffset() const { return m_lodOffset; }
        [[nodiscard]] uint64_t getSizeofLOD() const {
            switch (m_type) {
                case LOD_TYPE_SVO:
                    return 2;
                case LOD_TYPE_SVDAG:
                case LOD_TYPE_SVDAG_OCCUPANCY_FIELD:
                    return 32;
            }
            throw std::runtime_error("Unknown LOD type.");
        }

        void setLODOffset(const uint64_t offset) { m_lodOffset = offset; }

    private:
        std::string m_dataPath;
        std::string m_folder;
        std::string m_name;

        std::string m_key;
        VolumeLODType m_type;

        uint64_t m_lodOffset = 0;
    };
} // namespace raven