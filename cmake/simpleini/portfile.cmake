# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO alandtse/simpleini
    REF 55f28458df411b3573be624a99f9aa54d8977e5f
    SHA512 0ea40296b1b9b861fd3ccd9198ae9a7654b16160aad10ebc643c55996445ead683775e4c6fe7e5a241fec37e3be2200c8f8527d4ef0bbaee767f7a45390dfda1
    HEAD_REF allow_empty_values
)

# Install codes
set(SIMPLEINI_SOURCE ${SOURCE_PATH}/SimpleIni.h
                     ${SOURCE_PATH}/ConvertUTF.h
                     ${SOURCE_PATH}/ConvertUTF.c
)

file(INSTALL ${SIMPLEINI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
# Install sample
#file(INSTALL ${SOURCE_PATH}/snippets.cpp DESTINATION ${CURRENT_PACKAGES_DIR}/share/sample)

file(INSTALL ${SOURCE_PATH}/LICENCE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
