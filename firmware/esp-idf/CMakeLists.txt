# ESP-IDF project: Dipper timing-leakage test for ESP32 and ESP32-C6.
# Build with PlatformIO (platformio.ini) or plain idf.py.
cmake_minimum_required(VERSION 3.16)
set(PROJECT_VER "dipper-timing")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(dipper_timing)
