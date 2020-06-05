#ifndef VRN_POITOOLS_POINTFITTING_H
#define VRN_POITOOLS_POINTFITTING_H

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
#include "voreen/core/properties/filedialogproperty.h"

#include "voreen/core/ports/volumeport.h"

#include "tgt/font.h"
#include "tgt/glmath.h"
#include "tgt/immediatemode/immediatemode.h"

namespace voreen {

class VRN_CORE_API PointFitting : public ImageProcessor {
public:
    PointFitting();
    ~PointFitting();
    virtual Processor* create() const;

    virtual std::string getCategory() const  { return "Utility";         }
    virtual std::string getClassName() const { return "PointFitting"; }
    virtual CodeState getCodeState() const   { return CODE_STATE_STABLE; }
    virtual bool isUtility() const           { return true; }

    virtual bool isReady() const;

    void measure(tgt::MouseEvent* e);
    void undo(tgt::MouseEvent* e);

protected:
    virtual void setDescriptions() {
        setDescription(
                "Allows to interactively fit points of interest on the surface rendered volumes. "
                "This processor expects the rendered volume, a rendering of the first hitpoints "
                "(use FHP compositing in a SingleVolumeRaycaster), and the volume that is currently "
                "being rendered (for scaling information) as input."
                );
    }

    void process();
    virtual void initialize();

private:
    tgt::ivec2 clampToViewport(tgt::ivec2 mousePos);

    void readMandatoryPoints(); // read mandatory points from file
    void forceReload(); // reload the mandatory points

    RenderPort imgInport_;
    RenderPort fhpInport_;
    VolumePort refInport_;
    RenderPort outport_;
    GeometryPort outportPicked_;
    bool forceReload_;

    long unsigned int numSelectedPoints_;
    std::vector<std::string> mandatoryPoints_;

    EventProperty<PointFitting> mouseEventProp_;
    EventProperty<PointFitting> mouseUndoProp_;
    CameraProperty camera_;
    BoolProperty renderSpheres_;

    tgt::ivec2 mouseCurPos2D_;
    tgt::vec3 mouseCurPos3D_;
    bool mouseDown_;

    std::vector<tgt::vec3> pointsList_;

    tgt::Font font_;
    GlMeshGeometryUInt16Normal mesh_;
    tgt::ImmediateMode::LightSource lightSource_;
    tgt::ImmediateMode::Material material_;

    FileDialogProperty pointListFile_;   ///< filename of the file containing the mandatory points information
};

} // namespace

#endif // VRN_POITOOLS_POINTFITTING_H
