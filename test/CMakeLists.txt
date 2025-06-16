cmake_minimum_required(VERSION 3.16.0)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf test/certificates/ca-lan.pem TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf test/certificates/cer-plscount.pem TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf test/certificates/cer-plscount.key TEXT)
