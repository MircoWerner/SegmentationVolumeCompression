#pragma once

#include "NastjaConverter.h"

namespace raven {
    class CellsConverter final : public NastjaConverter {
    public:
        CellsConverter(std::string data, std::string scene, const bool svdagOccupancyField) : NastjaConverter(std::move(data), std::move(scene), 5, {2, 3, 4, 5}, svdagOccupancyField) {}

    protected:
        [[nodiscard]] uint32_t csvType(const std::string &field) const override {
            return static_cast<uint32_t>(std::stoll(field));
        }
    };
} // namespace raven