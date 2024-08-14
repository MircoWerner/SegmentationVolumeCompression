cmake_minimum_required(VERSION 3.18)
project(segmentationvolumes VERSION 0.1.0 DESCRIPTION "SVDAG Compression for Segmentation Volume Path Tracing" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)
find_package(TBB REQUIRED)

add_subdirectory(lib)

#set(GCC_COVERAGE_COMPILE_FLAGS "-fpermissive")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

message(STATUS "Compiler Version: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

# VkRaven Core
if(NOT BASE_DIRECTORY)
    find_path(BASE_DIRECTORY
            NAMES VkRaven/cmake/setup.cmake
            PATHS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../..
            REQUIRED
            DOC "Directory containing VkRaven"
    )
endif()
if(EXISTS ${BASE_DIRECTORY}/VkRaven)
    add_subdirectory(${BASE_DIRECTORY}/VkRaven ${CMAKE_CURRENT_BINARY_DIR}/VkRaven)
else()
    message(FATAL_ERROR "Could not find base directory, please set BASE_DIRECTORY to folder containing VkRaven")
endif()
set(RAVEN_CORE_DIR ${BASE_DIRECTORY}/VkRaven)

set(RAVENPROJECT_HEADERS
        include/segmentationvolumes/Raystructs.h
        include/segmentationvolumes/PassTonemapper.h
        include/segmentationvolumes/SegmentationVolumes.h
        include/segmentationvolumes/PassDebug.h
        include/segmentationvolumes/SegmentationVolumesPassPathtrace.h
        include/segmentationvolumes/SegmentationVolumesPassPathtraceRayQuery.h
        include/segmentationvolumes/SegmentationVolumesPassPathtraceRayTracing.h

        include/segmentationvolumes/data/MaterialGenerator.h
        include/segmentationvolumes/data/SegmentationVolumeMaterial.h

        include/segmentationvolumes/scene/SegmentationVolumesScene.h
        include/segmentationvolumes/scene/Volume.h
        include/segmentationvolumes/scene/VolumeAABB.h
        include/segmentationvolumes/scene/VolumeLOD.h

        include/segmentationvolumes/test/DAGTraversalTest.h

        include/segmentationvolumes/converter/SegmentationVolumeConverter.h
        include/segmentationvolumes/converter/NastjaConverter.h
        include/segmentationvolumes/converter/CellsConverter.h
        include/segmentationvolumes/converter/MouseConverter.h
        include/segmentationvolumes/converter/CElegansConverter.h
        include/segmentationvolumes/converter/builder/Octree.h
        include/segmentationvolumes/converter/builder/DAG.h
        include/segmentationvolumes/converter/builder/DAGGPU.h
        include/segmentationvolumes/converter/builder/DAGGPUPassPreparation.h
        include/segmentationvolumes/converter/builder/DAGGPUPassSort.h
        include/segmentationvolumes/converter/builder/DAGGPUPassUnique.h
        include/segmentationvolumes/converter/builder/DAGGPUPassPrefixSum.h
        include/segmentationvolumes/converter/builder/DAGGPUPassPrefixSumOffset.h
        include/segmentationvolumes/converter/builder/DAGGPUPassCompaction.h
        include/segmentationvolumes/converter/builder/DAGGPUPassRoots.h
        include/segmentationvolumes/converter/builder/DAGGPUTest.h

        include/segmentationvolumes/evaluation/SegmentationVolumesEvaluation.h
)

set(RAVENPROJECT_SOURCES
        src/bin/main.cpp
        src/segmentationvolumes/SegmentationVolumes.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${RAVENPROJECT_HEADERS} ${RAVENPROJECT_SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME} glm Vulkan::Vulkan imgui stb tinyobjloader tinygltf spirv-reflect ravencore glfw pugixml argparse HighFive tbb OpenMP::OpenMP_CXX)
if (CUDAToolkit_FOUND)
    target_link_libraries(${CMAKE_PROJECT_NAME} CUDA::cudart CUDA::cuda_driver)
endif ()

target_include_directories(${CMAKE_PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if (OPTIX_FOUND)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OPTIX_INCLUDE_DIR})
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE ${OPTIX_INCLUDE_DIR})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 -DVULKAN_HPP_STORAGE_SHARED=1)

SET(RESOURCE_DIRECTORY_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/resources\")
if (RESOURCE_DIRECTORY_PATH)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE RESOURCE_DIRECTORY_PATH=${RESOURCE_DIRECTORY_PATH})
endif ()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/VkRaven/lib"
        "${PROJECT_BINARY_DIR}")
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/VkRaven/raven"
        "${PROJECT_BINARY_DIR}")
