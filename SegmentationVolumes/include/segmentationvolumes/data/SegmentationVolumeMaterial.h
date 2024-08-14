#pragma once

#include <string>

namespace raven {
    class SegmentationVolumeMaterial {
    public:
        struct Material {
            glm::vec3 baseColor = glm::vec3(0.9, 0.2, 0.3);
            float specularTransmission = 0.f;
            float metallic = 0.f;
            float subsurface = 0.f;
            float specular = 0.f;
            float roughness = 0.5f;
            float specularTint = 0.f;
            float anisotropic = 0.f;
            float sheen = 0.f;
            float sheenTint = 0.f;
            float clearcoat = 0.f;
            float clearcoatGloss = 0.f;
            float eta = 1.5f;

            glm::vec3 emission = glm::vec3(0);
        };

        std::string m_key{};
        std::string m_name = "SegmentationVolumeMaterial";
        Material m_material{};
        glm::vec3 m_emissiveFactor = glm::vec3(0.f);
        float m_emissiveStrength = 0.f;

        void setEmission(const glm::vec3 emissiveFactor, const float emissiveStrength) {
            m_emissiveFactor = emissiveFactor;
            m_emissiveStrength = emissiveStrength;
            updateEmission();
        }
        void setEmission(const glm::vec3 emissiveFactor) {
            m_emissiveFactor = emissiveFactor;
            updateEmission();
        }
        void setEmission(const float emissiveStrength) {
            m_emissiveStrength = emissiveStrength;
            updateEmission();
        }
        void updateEmission() {
            m_material.emission = m_emissiveFactor * m_emissiveStrength;
        }
    };
} // namespace RavenMaterial