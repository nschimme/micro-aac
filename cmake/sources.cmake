# cmake/sources.cmake

function(micro_aac_get_sources FAAC_DIR)
    set(FAAC_SOURCES
        "${FAAC_DIR}/common/fft.c"
        "${FAAC_DIR}/common/sbr_tables.c"
        "${FAAC_DIR}/common/sfb_tables.c"
        "${FAAC_DIR}/libfaac/bitstream.c"
        "${FAAC_DIR}/libfaac/blockswitch.c"
        "${FAAC_DIR}/libfaac/channels.c"
        "${FAAC_DIR}/libfaac/cpu_compute.c"
        "${FAAC_DIR}/libfaac/faac.c"
        "${FAAC_DIR}/libfaac/filtbank.c"
        "${FAAC_DIR}/libfaac/frame.c"
        "${FAAC_DIR}/libfaac/huff2.c"
        "${FAAC_DIR}/libfaac/huffdata.c"
        "${FAAC_DIR}/libfaac/quantize.c"
        "${FAAC_DIR}/libfaac/ratecontrol.c"
        "${FAAC_DIR}/libfaac/sbr.c"
        "${FAAC_DIR}/libfaac/sbr_bitstream.c"
        "${FAAC_DIR}/libfaac/sbr_analysis.c"
        "${FAAC_DIR}/libfaac/sbr_huff_tables.c"
        "${FAAC_DIR}/libfaac/resample.c"
        "${FAAC_DIR}/libfaac/stereo.c"
        "${FAAC_DIR}/libfaac/tns.c"
        "${FAAC_DIR}/libfaac/util.c"
        PARENT_SCOPE
    )

    # Use forwarder files in src/ for files that collide with libfaac filenames under PlatformIO ESP-IDF
    set(FAAD_SOURCES
        "${FAAC_DIR}/libfaad/decoder.c"
        "${FAAC_DIR}/libfaad/bits.c"
        "${FAAC_DIR}/libfaad/asc.c"
        "${FAAC_DIR}/libfaad/huffman.c"
        "${FAAC_DIR}/libfaad/syntax.c"
        "${FAAC_DIR}/libfaad/dequant.c"
        "${FAAC_DIR}/libfaad/imdct.c"
        "${FAAC_DIR}/libfaad/sbr_dec_tables.c"
        "${FAAC_DIR}/libfaad/ps.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/faad_stereo.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/faad_tns.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/faad_sbr.c"
        PARENT_SCOPE
    )

    set(WRAPPER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/aac_encoder.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/aac_decoder.cpp"
        PARENT_SCOPE
    )
endfunction()
