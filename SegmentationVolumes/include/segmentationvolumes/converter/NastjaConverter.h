#pragma once

#include <utility>

#include "SegmentationVolumeConverter.h"

namespace raven {
    class NastjaConverter : public SegmentationVolumeConverter {
    public:
        NastjaConverter(std::string data, std::string scene, const uint32_t csvLineSkipFields, std::vector<uint32_t> excludedTypes, const bool svdagOccupancyField) : SegmentationVolumeConverter("type", "types", std::move(data), std::move(scene), svdagOccupancyField), m_csvLineSkipFields(csvLineSkipFields), m_excludedTypes(std::move(excludedTypes)) {}

        void rawDataToVoxelTypes() override {
            createVoxelDirectories();

            // load mapping
            std::unordered_map<uint32_t, uint32_t> cellIdToTypeId; // cellId -> typeId
            loadTypes(m_data + "/" + m_scene + "/cells.csv", cellIdToTypeId);

            // volume path
            const std::string filename = m_data + "/" + m_scene + "/cells.raw";
            std::cout << "[" << filename << "]" << std::endl;

            // load volume
            std::vector<uint32_t> payload;
            glm::ivec3 dimensions;
            {
                std::ifstream nrrd(filename, std::ios_base::in | std::ios_base::binary);
                if (!nrrd.is_open()) {
                    std::ostringstream err;
                    err << "Unable to open cellsinsilico NRRD file at: " << filename << std::endl;
                    throw std::runtime_error(err.str());
                }

                // read header
                std::string line;
                // first line contains space seperated width height depth
                if (!std::getline(nrrd, line)) {
                    nrrd.close();
                    throw std::runtime_error("Unexpected end of file in: " + filename);
                }
                std::istringstream sizes(line);
                sizes >> dimensions[0] >> dimensions[1] >> dimensions[2];
                // second line contains data type
                if (!std::getline(nrrd, line)) {
                    nrrd.close();
                    throw std::runtime_error("Unexpected end of file in: " + filename);
                }
                if (line != "uint32") {
                    nrrd.close();
                    throw std::runtime_error("Data type " + line + " does not equal to requested format uint32.");
                }
                constexpr uint16_t bits_per_sample = 32;

                const auto max_dim = static_cast<float>(std::max(dimensions[0], std::max(dimensions[1], dimensions[2])));
                const auto physical_size_x = static_cast<float>(dimensions[0]) / max_dim;
                const auto physical_size_y = static_cast<float>(dimensions[1]) / max_dim;
                const auto physical_size_z = static_cast<float>(dimensions[2]) / max_dim;

                if (physical_size_x <= 0.f || physical_size_y <= 0.f || physical_size_z <= 0.f || !std::isfinite(physical_size_x) || !std::isfinite(physical_size_y) || !std::isfinite(physical_size_z)) {
                    nrrd.close();
                    throw std::invalid_argument("Invalid NRRD physical volume size.");
                }

                // thats a 8GiB volume for 8bit samples, 16GiB for 16bit samples
                const std::streamsize voxel_count = dimensions[0] * dimensions[1] * dimensions[2];

                std::streamsize byte_size = voxel_count * (bits_per_sample / 8);
                payload.resize(voxel_count);

                // read binary data inline
                nrrd.read(reinterpret_cast<char *>(payload.data()), byte_size);

                if (!nrrd) {
                    nrrd.close();
                    throw std::runtime_error("Only " + std::to_string(nrrd.gcount()) + " bytes of expected " + std::to_string(byte_size) + " bytes could be read from NRRD file.");
                }

                nrrd.close();
            }
            std::cout << "[" << filename << "] Volume loaded." << std::endl;

            // extract voxels
            // std::map<uint32_t, std::vector<glm::ivec3>> voxels;
            std::map<uint32_t, std::vector<glm::ivec4>> voxels;

            for (int32_t vz = 0; vz < dimensions[2]; vz++) {
                for (int32_t vy = 0; vy < dimensions[1]; vy++) {
                    for (int32_t vx = 0; vx < dimensions[0]; vx++) {
                        const uint32_t cellId = payload[vz * dimensions[0] * dimensions[1] + vy * dimensions[0] + vx];

                        const auto typeId = cellIdToTypeId.find(cellId);
                        if (typeId == cellIdToTypeId.end()) {
                            continue;
                        }
                        if (excludeType(typeId->second)) {
                            continue;
                        }

                        insertVoxel(voxels, typeId->second, glm::ivec3(0), vx, vy, vz, cellId); // use cellId for subdivision
                    }
                }
            }

            std::cout << "[" << filename << "] Voxels extracted." << std::endl;

            writeVoxels(voxels);
        }

    protected:
        [[nodiscard]] virtual uint32_t csvType(const std::string &) const = 0;
        uint32_t m_csvLineSkipFields{};
        std::vector<uint32_t> m_excludedTypes{};

        void loadVoxels(const std::filesystem::directory_entry &type, uint32_t &numVoxels, std::map<uint32_t, std::pair<iAABB, std::vector<glm::ivec3>>> &voxels) const override {
            std::vector<char> voxelsRaw;

            const uint64_t bytesVoxels = std::filesystem::file_size(type.path());
            if (bytesVoxels / sizeof(glm::ivec4) > UINT32_MAX) {
                throw std::runtime_error("numVoxels > UINT32_MAX");
            }
            numVoxels = bytesVoxels / sizeof(glm::ivec4);
            voxelsRaw.resize(bytesVoxels);
            std::ifstream(type.path(), std::ios::binary).read(voxelsRaw.data(), static_cast<std::streamsize>(bytesVoxels));

            const auto *voxelsVec = reinterpret_cast<glm::ivec4 *>(voxelsRaw.data());

            // std::map<int32_t, std::map<int32_t, std::map<int32_t, bool>>> voxelsMap3D;
            // uint64_t duplicateVoxels = 0;

            for (uint32_t i = 0; i < numVoxels; i++) {
                const auto voxel = voxelsVec[i];
                const auto voxelCoordinates = glm::ivec3(voxel.x, voxel.y, voxel.z);

                voxels[voxel.w].second.push_back(voxelCoordinates);
                voxels[voxel.w].first.expand(voxelCoordinates);
                voxels[voxel.w].first.expand(voxelCoordinates + glm::ivec3(1, 1, 1));

                // if (voxelsMap3D.contains(voxel.x)) {
                //     if (voxelsMap3D[voxel.x].contains(voxel.y)) {
                //         if (voxelsMap3D[voxel.x][voxel.y].contains(voxel.z)) {
                //             std::cout << "duplicate voxel: " << voxel.x << "," << voxel.y << "," << voxel.z << std::endl;
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

        [[nodiscard]] uint32_t toLabelId(const uint32_t typeId, const size_t instance) const override {
            if (typeId > 0xFFFF) {
                throw std::runtime_error("Type number too large for 16 bit label id.");
            }
            if (instance > 0xFFFF) {
                throw std::runtime_error("Instance number too large for 16 bit label id.");
            }
            return (static_cast<uint32_t>(instance) << 16u) | typeId;
        }

    private:
        void loadTypes(const std::string &url, std::unordered_map<uint32_t, uint32_t> &cellIdToTypeId) const {
            std::ifstream csv(url);
            if (!csv.is_open()) {
                throw std::runtime_error("Unable to open csv file: " + url);
            }

            csv.ignore(LLONG_MAX, '\n'); // skip first line

            std::string line;
            while (std::getline(csv, line)) {
                std::istringstream iss(line);
                uint32_t cellId;
                uint32_t type;
                if (parseCSVLine(iss, &cellId, &type)) {
                    cellIdToTypeId[cellId] = type;
                } else {
                    std::cerr << "Cannot parse csv line: " << line;
                }
            }

            csv.close();

            std::cout << "Volume csv file parsed successfully. Found " << cellIdToTypeId.size() << " cell ids in file.";
        }

        bool parseCSVLine(std::istringstream &iss, uint32_t *cellId, uint32_t *type) const {
            std::string field;

            if (std::getline(iss, field, ' ')) {
                *cellId = static_cast<uint32_t>(std::stoll(field));
            } else {
                return false;
            }

            for (uint32_t i = 0; i < m_csvLineSkipFields; i++) {
                std::getline(iss, field, ' ');
            }

            if (std::getline(iss, field, ' ')) {
                *type = csvType(field);
            } else {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool excludeType(const uint32_t type) const { return std::ranges::find(m_excludedTypes, type) != m_excludedTypes.end(); }
    };
} // namespace raven