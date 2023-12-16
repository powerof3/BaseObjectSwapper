# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF 2378017a6f9a042c9d7d20fcd52b6db38df3c60d
    SHA512 10c10bc798c503f2b62bda0421763d2e465b4009dd4e6a52bff2164356f28bb2aef0d27f8a22dd268d0136c8a0d42a3dba1a9f02165da3d061be92a0d031f6bc
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
