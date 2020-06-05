#ifndef VRN_POITOOLS_SURFACEMEASURE_H
#define VRN_POITOOLS_SURFACEMEASURE_H

#include "voreen/core/processors/imageprocessor.h"
#include "voreen/core/properties/eventproperty.h"
#include "voreen/core/properties/cameraproperty.h"
#include "voreen/core/properties/stringproperty.h"
#include "voreen/core/properties/floatproperty.h"
#include "voreen/core/properties/intproperty.h"
#include "voreen/core/properties/boolproperty.h"
#include "voreen/core/utils/stringutils.h"
#include "voreen/core/datastructures/geometry/glmeshgeometry.h"

#include "voreen/core/datastructures/geometry/pointlistgeometry.h"
#include "voreen/core/ports/volumeport.h"
#include "voreen/core/ports/textport.h"

#include "tgt/font.h"
#include "tgt/glmath.h"
#include "tgt/immediatemode/immediatemode.h"

#include <algorithm>

namespace voreen {

class VRN_CORE_API SurfaceMeasure : public ImageProcessor {
public:
    SurfaceMeasure();
    ~SurfaceMeasure();
    virtual Processor* create() const;

    virtual std::string getCategory() const  { return "Utility";         }
    virtual std::string getClassName() const { return "SurfaceMeasure"; }
    virtual CodeState getCodeState() const   { return CODE_STATE_STABLE; }
    virtual bool isUtility() const           { return true; }

    virtual bool isReady() const;

    void measure(tgt::MouseEvent* e);
    void undo(tgt::MouseEvent* e);

protected:
    virtual void setDescriptions() {
        setDescription(
                "Allows to interactively measure distances on the surface rendered volumes. "
                "This processor expects the rendered volume, a rendering of the first hitpoints "
                "(use FHP compositing in a SingleVolumeRaycaster), and the volume that is currently "
                "being rendered (for scaling information) as input."
                );
    }

    void process();

private:
    tgt::ivec2 clampToViewport(tgt::ivec2 mousePos);

    RenderPort imgInport_;
    RenderPort fhpInport_;
    VolumePort refInport_;
    RenderPort outport_;
    GeometryPort outportDistance_;
    TextPort outportDistanceText_;

    std::vector<tgt::vec3> pointsListX_;
    std::vector<tgt::vec3> pointsListY_;

    EventProperty<SurfaceMeasure> mouseEventProp_;
    EventProperty<SurfaceMeasure> mouseUndoProp_;
    CameraProperty camera_;
    BoolProperty renderSpheres_;

    tgt::ivec2 mouseCurPos2D_;
    tgt::vec4 mouseCurPos3D_;
    tgt::ivec2 mouseStartPos2D_;
    tgt::vec4 mouseStartPos3D_;
    bool mouseDown_;

    float distance_;

    tgt::Font font_;
    GlMeshGeometryUInt16Normal mesh_;
    tgt::ImmediateMode::LightSource lightSource_;
    tgt::ImmediateMode::Material material_;

    float surfaceDistance();
    float measureX();
    float measureY();
};

} // namespace

#endif // VRN_POITOOLS_SURFACEMEASURE_H
