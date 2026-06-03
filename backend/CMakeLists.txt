cmake_minimum_required(VERSION 3.20)

project(sports_equipment_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(oatpp CONFIG REQUIRED)

find_path(MARIADB_INCLUDE_DIR
  NAMES mysql/mysql.h
  HINTS
    /ucrt64/include
    C:/msys64/ucrt64/include
)

find_library(MARIADB_LIBRARY
  NAMES mariadb mariadbclient
  HINTS
    /ucrt64/lib
    C:/msys64/ucrt64/lib
)

if(NOT MARIADB_INCLUDE_DIR OR NOT MARIADB_LIBRARY)
  message(FATAL_ERROR
    "MariaDB/MySQL client library not found. Install "
    "mingw-w64-ucrt-x86_64-libmariadbclient in MSYS2 UCRT64.")
endif()

set(APP_SOURCES
  src/app/main.cpp
)

set(BUSINESS_SOURCES
  src/dao/EquipmentCategoryDAO.cpp
  src/service/EquipmentCategoryService.cpp
)

add_executable(${PROJECT_NAME}
  ${APP_SOURCES}
  ${BUSINESS_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${MARIADB_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  oatpp::oatpp
  ${MARIADB_LIBRARY}
)
