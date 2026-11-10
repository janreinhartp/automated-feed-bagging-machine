cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(feed_bagging)

idf_build_get_property(idf_ver IDF_VER)
if(NOT idf_ver MATCHES "^5\\.5\\.")
    message(WARNING "feed_bagging was validated on ESP-IDF 5.5.x. Detected: ${idf_ver}")
endif()
