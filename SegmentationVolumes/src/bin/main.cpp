#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "argparse/include/argparse/argparse.hpp"
#include "raven/core/RavenVkDynamicLoader.h"

#include "raven/core/Application.h"
#include "raven/core/HeadlessApplication.h"
#include "raven/util/Paths.h"
#include "segmentationvolumes/SegmentationVolumes.h"
#include "segmentationvolumes/converter/CElegansConverter.h"
#include "segmentationvolumes/converter/CellsConverter.h"
#include "segmentationvolumes/converter/MouseConverter.h"
#include "segmentationvolumes/converter/builder/DAGGPUTest.h"
#include "segmentationvolumes/evaluation/SegmentationVolumesEvaluation.h"
#include "segmentationvolumes/test/DAGTraversalTest.h"

int main(int argc, char *argv[]) {
#ifdef RESOURCE_DIRECTORY_PATH
    raven::Paths::m_resourceDirectoryPath = RESOURCE_DIRECTORY_PATH;
#endif

    // raven::DAGTraversalTest::test();
    // return 0;

    std::string name = "SVDAG Compression for Segmentation Volume Path Tracing";

    argparse::ArgumentParser program("segmentationvolumes");
    program.add_argument("data")
            .help("path to the data folder (e.g. /path/to/data/)")
            .required();
    program.add_argument("scene")
            .help("name of the scene (in resources/scenes/<name>.xml)")
            .required();
    program.add_argument("--rayquery")
            .help("use ray query in compute shader instead of ray tracing pipeline")
            .flag();
    program.add_argument("--evaluate")
            .help("perform evaluation on given scene (measure rendering performance and store rendered image)")
            .flag();
    program.add_argument("--convert")
            .help("perform conversion from raw data to compressed format")
            .flag();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    auto rendererSettings = raven::SegmentationVolumes::SegmentationVolumesSettings{
            .m_data = program.get("data"),
            .m_scene = program.get("scene"),
            .m_rayquery = program["--rayquery"] == true};
    const auto renderer = std::make_shared<raven::SegmentationVolumes>(rendererSettings);

    if (program["--evaluate"] == true) {
        auto evaluation = raven::SegmentationVolumesEvaluation(program.get("data"), program.get("scene"), renderer);
        evaluation.init();
        evaluation.evaluate();
        return EXIT_SUCCESS;
    }

    if (program["--convert"] == true) {
        if (program.get("scene") == "cells") {
            const std::string data = program.get("data");
            const std::string scene = "cells";
            raven::CellsConverter converter(data, scene, true);
            // converter.nodeInfo();
            // converter.nodeDegree();
            converter.rawDataToVoxelTypes();
            converter.voxelTypesToAABBsAndOctrees();
            converter.AABBsAndOctreesToAABBsAndDAGs();
            std::vector<raven::SegmentationVolumeConverter::DAGFileInfo> dagFileInfos;
            for (uint32_t i = 1; i <= 27; i++) {
                const std::string str = "type" + std::to_string(i);
                const raven::SegmentationVolumeConverter::DAGFileInfo dagFileInfo(converter.stringSVDAG(false), {str}, str, str);
                if (!dagFileInfo.exists(data, scene)) {
                    std::cout << "File does not exist: " << str << std::endl;
                    continue;
                }
                dagFileInfos.push_back(dagFileInfo);
            }
            converter.mergeDAGs(dagFileInfos);
            // raven::DAGGPUTest::test(data, scene, dagFileInfos, "types", converter.stringSVDAG(true));
            return 0;
        }
        if (program.get("scene") == "celegans") {
            const std::string data = program.get("data");
            const std::string scene = "celegans";
            raven::CElegansConverter converter(data, scene, true);
            // converter.nodeInfo();
            converter.voxelTypesToAABBsAndOctrees();
            converter.AABBsAndOctreesToAABBsAndDAGs();
            std::vector<raven::SegmentationVolumeConverter::DAGFileInfo> dagFileInfos;
            for (uint32_t i = 1; i <= 240; i++) {
                const std::string str = "neuron" + std::to_string(i);
                const raven::SegmentationVolumeConverter::DAGFileInfo dagFileInfo(converter.stringSVDAG(false), {str}, str, str);
                if (!dagFileInfo.exists(data, scene)) {
                    std::cout << "File does not exist: " << str << std::endl;
                    continue;
                }
                dagFileInfos.push_back(dagFileInfo);
            }
            converter.mergeDAGs(dagFileInfos);
            // raven::DAGGPUTest::test(data, scene, dagFileInfos, "neurons", converter.stringSVDAG(true));
            return 0;
        }
        if (program.get("scene") == "mouse") {
            const std::string data = program.get("data");
            const std::string scene = "mouse";
            raven::MouseConverter converter(data, scene, true);
            // converter.nodeInfo();
            // converter.nodeDegree();
            converter.rawDataToVoxelTypes();
            converter.voxelTypesToAABBsAndOctrees();
            converter.AABBsAndOctreesToAABBsAndDAGs();
            std::vector<raven::SegmentationVolumeConverter::DAGFileInfo> dagFileInfos;
            for (uint32_t i = 1; i <= 96; i++) {
                const std::string str = "neuron" + std::to_string(i);
                const raven::SegmentationVolumeConverter::DAGFileInfo dagFileInfo(converter.stringSVDAG(false), {str}, str, str);
                if (!dagFileInfo.exists(data, scene)) {
                    std::cout << "File does not exist: " << str << std::endl;
                    continue;
                }
                dagFileInfos.push_back(dagFileInfo);
            }
            converter.mergeDAGs(dagFileInfos);
            return 0;
        }
    }

#ifdef WIN32
    auto settings = raven::Application::ApplicationSettings{.m_msaaSamples = vk::SampleCountFlagBits::e1,
                                                            .m_deviceExtensions = {VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                                                                   VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME}};
#else
    auto settings = raven::Application::ApplicationSettings{
            .m_msaaSamples = vk::SampleCountFlagBits::e1,
            .m_deviceExtensions = {
                    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME,
                    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
                    VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME}};
#endif
    raven::Application app(name, settings, renderer,
                           raven::Queues::QueueFamilies::COMPUTE_FAMILY | raven::Queues::QueueFamilies::GRAPHICS_FAMILY | raven::Queues::TRANSFER_FAMILY);
    app.setVSync(true);

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
