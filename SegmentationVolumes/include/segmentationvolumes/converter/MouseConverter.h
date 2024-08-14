#pragma once

#include "SegmentationVolumeConverter.h"
#include "highfive/highfive.hpp"

namespace raven {
    class MouseConverter final : public SegmentationVolumeConverter {
    public:
        MouseConverter(std::string data, std::string scene, const bool svdagOccupancyField) : SegmentationVolumeConverter("neuron", "neurons", std::move(data), std::move(scene), svdagOccupancyField) {}

        void rawDataToVoxelTypes() override {
            createVoxelDirectories();

            // load mapping
            std::unordered_map<uint32_t, uint32_t> mappedSegmentIdToAgglomerateId; // mappedSegmentId -> agglomerateId
            std::unordered_map<uint32_t, uint32_t> agglomerateIdToNeuronId;        // agglomerateId -> neuronId
            loadMouseCortexDendrites(m_data + "/" + m_scene + "/dendrites.hdf5", mappedSegmentIdToAgglomerateId, agglomerateIdToNeuronId);

            uint32_t types = 0;
            for (const auto &entry: std::filesystem::directory_iterator(m_data + "/" + m_scene)) {
                // volume path
                const std::regex rgx("x([0-9]+)y([0-9]+)z([0-9]+)\\.hdf5");
                const std::string filename = entry.path().filename().string();
                std::smatch matches;
                std::regex_search(filename, matches, rgx);
                if (matches.size() != 4) {
                    continue;
                }
                auto coordinate = glm::ivec3(std::stoi(matches[1]), std::stoi(matches[2]), std::stoi(matches[3]));
                const auto offset = glm::ivec3(1024) * coordinate;
                std::cout << "[" << filename << "]" << std::endl;

                // load volume
                HighFive::File file(entry.path(), HighFive::File::ReadOnly);
                auto dataset = file.getDataSet(file.getObjectName(0));
                std::cout << "[" << filename << "] Volume loaded." << std::endl;

                // read dimension
                std::vector<size_t> dimensions = dataset.getDimensions();
                auto max_dim = static_cast<float>(std::max(dimensions.at(0), std::max(dimensions.at(1), dimensions.at(2))));
                float physical_size_x = static_cast<float>(dimensions.at(0)) / max_dim;
                float physical_size_y = static_cast<float>(dimensions.at(1)) / max_dim;
                float physical_size_z = static_cast<float>(dimensions.at(2)) / max_dim;
                if (physical_size_x <= 0.f || physical_size_y <= 0.f || physical_size_z <= 0.f || !std::isfinite(physical_size_x) || !std::isfinite(physical_size_y) || !std::isfinite(physical_size_z)) {
                    throw std::invalid_argument("invalid hdf5 physical volume size");
                }

                // allocate a memory region and read hdf5 object to it
                auto payload = std::vector<uint32_t>(dimensions[0] * dimensions[1] * dimensions[2]);
                dataset.read_raw<uint32_t>(payload.data());

                // extract voxels
                std::map<uint32_t, std::vector<glm::ivec3>> voxels;

                for (int32_t vz = 0; vz < dimensions[2]; vz++) {
                    for (int32_t vy = 0; vy < dimensions[1]; vy++) {
                        for (int32_t vx = 0; vx < dimensions[0]; vx++) {
                            const uint32_t mappedSegmentId = payload[vz * dimensions[0] * dimensions[1] + vy * dimensions[0] + vx];

                            const auto agglomerateId = mappedSegmentIdToAgglomerateId.find(mappedSegmentId);
                            if (agglomerateId == mappedSegmentIdToAgglomerateId.end()) {
                                continue;
                            }

                            const auto typeId = agglomerateIdToNeuronId.find(agglomerateId->second);
                            if (typeId == agglomerateIdToNeuronId.end()) {
                                continue;
                            }

                            insertVoxel(voxels, typeId->second, offset, vx, vy, vz);
                        }
                    }
                }

                std::cout << "[" << filename << "] Voxels extracted." << std::endl;

                writeVoxels(voxels);

                std::cout << "Processed " << ++types << "." << std::endl;
            }
        }

    private:
        static void loadMouseCortexDendrites(const std::string &url, std::unordered_map<uint32_t, uint32_t> &mappedSegmentIdToAgglomerateId, std::unordered_map<uint32_t, uint32_t> &agglomerateIdToNeuronId) {
            // "The dendrite reconstructions are stored in dendrites.hdf5. Here, "dendrites" refers to all postsynaptic targets (including, for example, neuronal somata)." from
            // https://l4dense2019.brain.mpg.de/#neurite-sec For all biology laymen (as myself): https://en.wikipedia.org/wiki/Soma_(biology) (the first image "structure of a typical neuron" is very helpful)

            const HighFive::File file(url, HighFive::File::ReadOnly);
            const auto dendrites = file.getGroup("dendrites");

            {
                // dataset mapping: agglomerateId -> mappedSegmentId
                // target mapping: mappedSegmentId -> agglomerateId
                const auto dataset = dendrites.getDataSet("mappedSegIds");
                // read dimension
                const std::vector<size_t> dimensions = dataset.getDimensions();
                assert(dimensions.size() == 1 && "dataset contains more than one dimension");
                // allocate a memory region and read hdf5 object to it
                const auto payload = dataset.read<std::vector<uint32_t>>();
                for (uint32_t i = 0; i < payload.size(); i++) {
                    if (payload[i] != 0) {
                        mappedSegmentIdToAgglomerateId[payload[i]] = i;
                    }
                }
            }
            {
                // dataset mapping: agglomerateId -> neuronId
                // target mapping: agglomerateId -> neuronId
                const auto dataset = dendrites.getDataSet("neuronId");
                // read dimension
                const std::vector<size_t> dimensions = dataset.getDimensions();
                assert(dimensions.size() == 1 && "dataset contains more than one dimension");
                // allocate a memory region and read hdf5 object to it
                const auto payload = dataset.read<std::vector<uint32_t>>();
                for (uint32_t i = 0; i < payload.size(); i++) {
                    if (payload[i] != 0) {
                        agglomerateIdToNeuronId[i] = payload[i];
                    }
                }
            }
        }
    };
} // namespace raven