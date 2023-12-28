include(ExternalProject)
set(OPENSSL_ROOT "${CMAKE_BINARY_DIR}/openssl")
set(OPENSSL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/openssl-1.1.1w")
set(OPENSSL_BINARY_DIR "${CMAKE_BINARY_DIR}/openssl-build")

ExternalProject_Add(build_openssl
  CONFIGURE_COMMAND bash ${OPENSSL_SOURCE_DIR}/config --prefix=${OPENSSL_ROOT}
  BUILD_COMMAND     make -j
  SOURCE_DIR        "${OPENSSL_SOURCE_DIR}"
  BINARY_DIR        "${OPENSSL_BINARY_DIR}"
)

set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT}/include)

add_library(openssl_crypto STATIC IMPORTED)

set_target_properties(openssl_crypto PROPERTIES
  IMPORTED_LOCATION ${OPENSSL_ROOT}/lib/libcrypto.a
  INCLUDE_DIRECTORIES ${OPENSSL_ROOT}/include
)

add_dependencies(openssl_crypto build_openssl)
