### HighFive ###
#add_library(highfive SHARED ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/highfive.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5Attribute.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5DataSet.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5DataSpace.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5DataType.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5File.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5Group.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5PropertyList.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5Reference.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5Selection.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5Utility.hpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive/include/highfive/H5Version.hpp
#        main.cpp)
#set_target_properties(highfive PROPERTIES LINKER_LANGUAGE CXX)
#target_include_directories(highfive
#        PUBLIC
#        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/HighFive/include>
#        ${CMAKE_CURRENT_SOURCE_DIR}/HighFive
#)
#install(TARGETS highfive LIBRARY DESTINATION lib)
add_subdirectory(HighFive)