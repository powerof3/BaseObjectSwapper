# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/simpleini
    REF f419517a17cd453e644145a5fd82b4479a01f684
    SHA512 f93ac33a1488c315ccfaf4ab2e76b7b3075d817885f9718ddc3cd257a27e114801b92b36818a423af20cdaf511036caea5b8b3c8f3290e1f76352395b8bf37e0
    HEAD_REF master
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
