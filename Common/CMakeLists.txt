cmake_minimum_required(VERSION 3.22)

project(CommonLibs LANGUAGES CXX)

# ------------------------------------------------------------
# Common (substrate)
# ------------------------------------------------------------

add_library(Common STATIC
    # Public headers (explicit)
    include/ImmutableByteView.h
    include/MutableByteView.h
    include/Counters.h

    include/Status.h

    include/SourceId.h
    include/MeasurementHeaderV1.h
    include/MeasurementTypes.h
    include/MeasurementTypesIO.h

    include/MeasurementNameOf.h
    include/MeasurementTypeStrings.h

    include/AnyMeasurement.h

    # Sources
    src/SourceId.cpp
    src/MeasurementTypesStrings.cpp
)

target_include_directories(Common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(Common PUBLIC cxx_std_17)

set_target_properties(Common PROPERTIES
    FOLDER "Common"
)

# ------------------------------------------------------------
# CommonBDS
# ------------------------------------------------------------

add_library(CommonBDS STATIC
    include/BdsCommon.h
    include/BinaryReadStream.h
    include/BinaryWriteStream.h
    include/MessageHeader.h

    src/BdsCommon.cpp
    src/BinaryReadStream.cpp
    src/BinaryWriteStream.cpp
)

target_include_directories(CommonBDS
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CommonBDS
    PUBLIC
        Common
)

target_compile_features(CommonBDS PUBLIC cxx_std_17)

set_target_properties(CommonBDS PROPERTIES
    FOLDER "CommonBDS"
)

# ------------------------------------------------------------
# CommonNMEA
# ------------------------------------------------------------

add_library(CommonNMEA STATIC
    include/NMEAParseStatus.h
    include/NMEAExtractionStream.h
    include/NMEAInsertionStream.h
    include/NMEATokenizer.h

    src/NMEAExtractionStream.cpp
    src/NMEAInsertionStream.cpp
    src/NMEATokenizer.cpp
)

target_include_directories(CommonNMEA
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CommonNMEA
    PUBLIC
        Common
)

target_compile_features(CommonNMEA PUBLIC cxx_std_17)

set_target_properties(CommonNMEA PROPERTIES
    FOLDER "CommonNMEA"
)

# ------------------------------------------------------------
# Measurement codecs (BDS)
# ------------------------------------------------------------

add_library(CommonBdsMeasurementCodecs STATIC
    include/BdsMeasurementCodecs.h
    src/BdsMeasurementCodecs.cpp
)

target_include_directories(CommonBdsMeasurementCodecs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CommonBdsMeasurementCodecs
    PUBLIC
        Common
        CommonBDS
)

target_compile_features(CommonBdsMeasurementCodecs PUBLIC cxx_std_17)

set_target_properties(CommonBdsMeasurementCodecs PROPERTIES
    FOLDER "CommonBDS"
)



include(CTest)


add_executable(CommonBDS_Tests
    tests/BdsTestMain.cpp
)

target_link_libraries(CommonBDS_Tests
    PRIVATE
        CommonBDS
)

set_target_properties(CommonBDS_Tests PROPERTIES
    FOLDER "CommonTests"
)

if (BUILD_TESTING)
    add_test(NAME CommonBDS_Tests COMMAND CommonBDS_Tests)
endif()



add_executable(CommonNmea_Tests
    tests/NmeaTestMain.cpp
)

target_link_libraries(CommonNmea_Tests
    PRIVATE
        CommonNMEA
)

set_target_properties(CommonNmea_Tests PROPERTIES
    FOLDER "CommonTests"
)

if (BUILD_TESTING)
    add_test(NAME CommonNmea_Tests COMMAND CommonNmea_Tests)
endif()
