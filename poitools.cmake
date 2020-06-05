# module class must reside in sample/samplemodule.h + sample/samplemodule.cpp
SET(MOD_CORE_MODULECLASS PoiTools)
 
# module's core source files, path relative to module dir
SET(MOD_CORE_SOURCES
    ${MOD_DIR}/processors/pointfitting.cpp
    ${MOD_DIR}/processors/surfacemeasure.cpp
)
 
# module's core header files, path relative to module dir
SET(MOD_CORE_HEADERS
    ${MOD_DIR}/processors/pointfitting.h
    ${MOD_DIR}/processors/surfacemeasure.h
)
