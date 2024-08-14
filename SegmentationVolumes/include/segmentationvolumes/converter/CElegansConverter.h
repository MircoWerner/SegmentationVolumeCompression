#pragma once

#include "SegmentationVolumeConverter.h"

namespace raven {
    class CElegansConverter final : public SegmentationVolumeConverter {
    public:
        CElegansConverter(std::string data, std::string scene, const bool svdagOccupancyField) : SegmentationVolumeConverter("neuron", "neurons", std::move(data), std::move(scene), svdagOccupancyField) {}

        void rawDataToVoxelTypes() override {
            createVoxelDirectories();

            throw std::runtime_error("Not implemented.");
        }
    };
} // namespace raven