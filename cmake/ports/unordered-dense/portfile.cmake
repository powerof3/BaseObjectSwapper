vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO martinus/unordered_dense
    REF a570a0e87f95027e8544c72295236df5d84c3dcc
    SHA512 3f06daf92b68ef7e1ec0c7bbd7070aac8d0bf7ce57bfc6408770ab001f4c3c5efe084b01405a65d6b9e3d99eb0f16c95dd6c5dfc88387d246552f850a9ec6cad
    HEAD_REF main
)

vcpkg_cmake_configure(
	SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(
	PACKAGE_NAME unordered_dense
	CONFIG_PATH lib/cmake/unordered_dense
)

file(REMOVE_RECURSE
	"${CURRENT_PACKAGES_DIR}/debug"
	"${CURRENT_PACKAGES_DIR}/lib"
)

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
