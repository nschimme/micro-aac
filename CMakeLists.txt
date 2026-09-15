# CMakeLists.txt for micro-aac
# Supports both ESP-IDF component builds and standalone host builds

cmake_minimum_required(VERSION 3.16)

if(DEFINED IDF_TARGET)
    set(ESP_IDF_BUILD TRUE)
else()
    set(ESP_IDF_BUILD FALSE)
endif()

if(ESP_IDF_BUILD)
    set(MICRO_AAC_COMPONENT_DIR ${COMPONENT_DIR})
else()
    set(MICRO_AAC_COMPONENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

include(${MICRO_AAC_COMPONENT_DIR}/cmake/sources.cmake)

# Generate config header
set(CONFIG_H_DIR "${CMAKE_CURRENT_BINARY_DIR}/config_h")
file(MAKE_DIRECTORY "${CONFIG_H_DIR}")

if(ESP_IDF_BUILD)
    if(CONFIG_MICRO_AAC_MAX_CHANNELS)
        set(MAX_CHANNELS ${CONFIG_MICRO_AAC_MAX_CHANNELS})
    else()
        set(MAX_CHANNELS 2)
    endif()
    if(CONFIG_MICRO_AAC_SBR_DECIMATION)
        set(FAAC_SBR_DECIMATION ${CONFIG_MICRO_AAC_SBR_DECIMATION})
    else()
        set(FAAC_SBR_DECIMATION 1)
    endif()
else()
    if(NOT DEFINED MAX_CHANNELS)
        set(MAX_CHANNELS 2)
    endif()
    if(NOT DEFINED FAAC_SBR_DECIMATION)
        set(FAAC_SBR_DECIMATION 1)
    endif()
endif()

configure_file(
    "${MICRO_AAC_COMPONENT_DIR}/cmake/faac_config.h.in"
    "${CONFIG_H_DIR}/config.h"
)

micro_aac_get_sources("${MICRO_AAC_COMPONENT_DIR}/lib/faac")

if(ESP_IDF_BUILD)
    idf_component_register(
        SRCS ${FAAC_SOURCES} ${WRAPPER_SOURCES}
        INCLUDE_DIRS
            "include"
        PRIV_INCLUDE_DIRS
            "${CONFIG_H_DIR}"
            "${MICRO_AAC_COMPONENT_DIR}/lib/faac/include"
            "${MICRO_AAC_COMPONENT_DIR}/lib/faac/libfaac"
    )

    target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_CONFIG_H)
    target_compile_options(${COMPONENT_LIB} PRIVATE -include "${CONFIG_H_DIR}/config.h")

else()
    project(micro_aac VERSION 0.1.0 LANGUAGES C CXX)

    add_library(micro_aac ${FAAC_SOURCES} ${WRAPPER_SOURCES})

    target_include_directories(micro_aac
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            "${CONFIG_H_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/faac/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/faac/libfaac"
    )

    target_compile_definitions(micro_aac PRIVATE HAVE_CONFIG_H)
    target_compile_options(micro_aac PRIVATE -include "${CONFIG_H_DIR}/config.h")

    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        target_link_libraries(micro_aac PRIVATE ${MATH_LIBRARY})
    endif()

    option(BUILD_TESTING "Build host tests" ON)
    if(BUILD_TESTING)
        enable_testing()
        add_executable(test_aac_encoder tests/test_aac_encoder.c)
        target_link_libraries(test_aac_encoder PRIVATE micro_aac)
        if(MATH_LIBRARY)
            target_link_libraries(test_aac_encoder PRIVATE ${MATH_LIBRARY})
        endif()
        add_test(NAME test_aac_encoder COMMAND test_aac_encoder)
    endif()
endif()
