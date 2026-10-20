# The bundled dini is installed only with dspxmodel. A project that deploys the binaries instead,
# such as an application bundle, would otherwise receive the library and its development files
# outside its own layout.
set(DINI_INSTALL ${DSPXMODEL_INSTALL})

add_subdirectory(dini)
