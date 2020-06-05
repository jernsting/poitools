//incude header file
#include "poitools.h"
 
// include classes to be registered
#include "processors/pointfitting.h"
#include "processors/surfacemeasure.h"
 
//use voreen namespace
namespace voreen {
 
PoiTools::PoiTools(const std::string& modulePath)
    : VoreenModule(modulePath)
{
    // module name to be used internally
    setID("POI Tools");
 
    // module name to be used in the GUI
    setGuiName("POI Tools");
 
    // each module processor needs to be registered
    registerProcessor(new PointFitting());
    registerProcessor(new SurfaceMeasure());
 
    // adds a glsl dir to the shader search path (if shaders are needed in the module)
    addShaderPath(getModulePath("glsl"));
}
 
std::string PoiTools::getDescription() const {
    return "This module is a toolset for handling points of interest.";
}
 
} // namespace
