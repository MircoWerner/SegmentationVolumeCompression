#pragma once

#include "raven/core/HeadlessApplication.h"
#include "segmentationvolumes/SegmentationVolumes.h"

#include <filesystem>
#include <raven/core/TimingHeadlessApplication.h>
#include <utility>

namespace raven {
    class SegmentationVolumesEvaluation {
    public:
        SegmentationVolumesEvaluation(std::string data, std::string scene,
                                      const std::shared_ptr<SegmentationVolumes> &renderer) : m_data(std::move(data)),
                                                                                              m_scene(std::move(scene)),
                                                                                              m_renderer(renderer) {}

        void init() {
            const auto t = std::time(nullptr);
            const auto tm = *std::localtime(&t);
            const std::string str = "evaluation/%Y-%m-%d-%H-%M-%S-" + m_scene;
            std::ostringstream oss;
            oss << std::put_time(&tm, str.c_str());
            m_directory = oss.str();

            if (!std::filesystem::create_directories(m_directory)) {
                throw std::runtime_error("Failed to create directory for evaluation.");
            }

            const std::string scenePath = Paths::m_resourceDirectoryPath + "/scenes/" + m_scene + ".xml";

            pugi::xml_document doc;
            if (const pugi::xml_parse_result result = doc.load_file(scenePath.c_str()); !result) {
                std::cerr << "XML [" << scenePath << "] parsed with errors." << std::endl;
                std::cerr << "Error description: " << result.description() << std::endl;
                std::cerr << "Error offset: " << result.offset << std::endl;
                throw std::runtime_error("Cannot parse scene file.");
            }

            if (const auto &renderer = doc.child("scene").child("renderer"); !renderer.empty()) {
                if (!renderer.attribute("maxFrames").empty()) {
                    m_frames = renderer.attribute("maxFrames").as_uint();
                } else {
                    throw std::runtime_error("Missing maxFrames attribute in renderer node.");
                }
            } else {
                throw std::runtime_error("Missing renderer node in scene file.");
            }

            if (const auto &window = doc.child("scene").child("window"); !window.empty()) {
                if (!window.attribute("width").empty() && !window.attribute("height").empty()) {
                    m_width = window.attribute("width").as_int();
                    m_height = window.attribute("height").as_int();
                } else {
                    throw std::runtime_error("Missing width or height attribute in window node.");
                }
            } else {
                throw std::runtime_error("Missing window node in scene file.");
            }
        }

        void evaluate() {
            std::vector<float> frameTimes(m_frames, -1.f);

            // create headless application
            const auto settings = raven::HeadlessApplication::ApplicationSettings{
                    .m_msaaSamples = vk::SampleCountFlagBits::e1,
                    .m_deviceExtensions = {
                            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME,
                            VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
                            VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME},
                    .m_width = m_width,
                    .m_height = m_height,
                    .m_rendererExecutions = m_frames,
                    .m_directory = m_directory,
                    .m_name = m_scene,
                    .m_executeAfterInit = [&](GPUContext *gpuContext) {
                        std::ofstream stream;
                        stream.open(m_directory + "/" + m_scene + "_memory.csv");
                        stream << "bytes_aabb,bytes_lod,bytes_blas,bytes_tlas,bytes_total" << std::endl;
                        stream << m_renderer->getAABBBuffersSize() << "," << m_renderer->getLODBuffersSize() << ","
                               << m_renderer->getBLASBuffersSize() << "," << m_renderer->getTLASBufferSize() << ","
                               << m_renderer->getAABBBuffersSize() + m_renderer->getLODBuffersSize() +
                                          m_renderer->getBLASBuffersSize() + m_renderer->getTLASBufferSize()
                               << std::endl;
                        stream.close();
                    },
                    .m_png = true,
            };
            const auto timingSettings = TimingHeadlessApplication::TimingApplicationSettings{
                    .m_timingExecutions = 1,
                    .m_startupExecutions = 256,
                    .m_executeAfterStartupExecutions = [&](GPUContext *gpuContext) { m_renderer->resetFrame(gpuContext); },
                    .m_executeQueryTiming = [&](std::ofstream &stream) {
                        const auto pt = std::isnan(m_renderer->getPathtraceTimeAveraged()) ? 0
                                                                                           : m_renderer->getPathtraceTimeAveraged();
                        stream << std::to_string(pt);
                        std::cout << "Frame time (exponential moving average): " << pt << "ms" << std::endl; },
                    .m_executeEachFrame = [&](const uint32_t execution) {
                        if (!std::isnan(m_renderer->getPathtraceTime())) {
                            frameTimes[execution] = m_renderer->getPathtraceTime();
                        } }};
            TimingHeadlessApplication app(m_scene, settings, timingSettings, m_renderer,
                                          Queues::QueueFamilies::COMPUTE_FAMILY |
                                                  Queues::QueueFamilies::GRAPHICS_FAMILY |
                                                  Queues::TRANSFER_FAMILY);

            try {
                app.run();
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
                return;
            }

            // save frame times to file
            std::ofstream stream;
            stream.open(m_directory + "/" + m_scene + "_frametimes.csv");
            stream << "frame,time" << std::endl;
            for (uint32_t frame = 0; frame < m_frames; frame++) {
                stream << frame << "," << frameTimes[frame] << std::endl;
            }
            stream.close();
        }

    private:
        std::string m_data;
        std::string m_scene;

        std::string m_directory;

        std::shared_ptr<SegmentationVolumes> m_renderer;

        uint32_t m_frames = 0;
        int32_t m_width = -1;
        int32_t m_height = -1;
    };
} // namespace raven