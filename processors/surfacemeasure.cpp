#include "surfacemeasure.h"

#include "voreen/core/voreenapplication.h"

#include "tgt/textureunit.h"

#include <sstream>

using tgt::TextureUnit;

namespace voreen {


SurfaceMeasure::SurfaceMeasure()
    : ImageProcessor("pointfitting")
    , imgInport_(Port::INPORT, "image", "Image Input")
    , fhpInport_(Port::INPORT, "fhp", "First-hit-points Input", false, Processor::INVALID_PROGRAM, RenderPort::RENDERSIZE_DEFAULT, GL_RGBA16F)
    , refInport_(Port::INPORT, "refvol", "Reference Volume", false)
    , outport_(Port::OUTPORT, "image.output", "Image Output")
    , outportDistance_(Port::OUTPORT, "outport.distance", "Points on the surface")
    , outportDistanceText_(Port::OUTPORT, "outport.distancetext", "Calculated distance as Text")
    , mouseEventProp_("mouseEvent.measure", "Surface measure", this, &SurfaceMeasure::measure, tgt::MouseEvent::MOUSE_BUTTON_LEFT, tgt::MouseEvent::MOTION | tgt::MouseEvent::PRESSED | tgt::MouseEvent::RELEASED, tgt::Event::ALT, false)
    , mouseUndoProp_("mouseEvent.undo", "Undo Surface measure", this, &SurfaceMeasure::undo, tgt::MouseEvent::MOUSE_BUTTON_RIGHT, tgt::MouseEvent::PRESSED | tgt::MouseEvent::RELEASED, tgt::Event::ALT, false)
    , camera_("camera", "Camera", tgt::Camera(tgt::vec3(0.f, 0.f, 3.5f), tgt::vec3(0.f, 0.f, 0.f), tgt::vec3(0.f, 1.f, 0.f)))
    , renderSpheres_("renderSpheres", "Render Spheres", true)
    , font_(VoreenApplication::app()->getFontPath("VeraMono.ttf"), 16)
    , mouseCurPos2D_(0.0f)
    , mouseCurPos3D_(0.0f)
    , mouseStartPos2D_(0.0f)
    , mouseStartPos3D_(0.0f)
    , mouseDown_(false)
    , distance_(0)
    , mesh_()           //
    , lightSource_()    // Are initialized below
    , material_()       //
    , pointsListX_()
    , pointsListY_()
{
    addPort(imgInport_);
    addPort(fhpInport_);
    addPort(refInport_);
    addPort(outport_);
    addPort(outportDistance_);
    addPort(outportDistanceText_);

    addProperty(camera_);
    addProperty(renderSpheres_);

    addEventProperty(&mouseEventProp_);
    addEventProperty(&mouseUndoProp_);

    // light parameters
    lightSource_.position = tgt::vec4(0,1,1,0);
    lightSource_.ambientColor = tgt::vec3(1,1,1);
    lightSource_.diffuseColor = tgt::vec3(1,1,1);
    lightSource_.specularColor = tgt::vec3(1,1,1);

    // material parameters
    material_.shininess = 20.0f;

    // initialize sphere geometry
    mesh_.setSphereGeometry(1.0f, tgt::vec3::zero, tgt::vec4::one, 40);
}

SurfaceMeasure::~SurfaceMeasure() {
}

Processor* SurfaceMeasure::create() const {
    return new SurfaceMeasure();
}

bool SurfaceMeasure::isReady() const {
    if (!isInitialized() || !imgInport_.isReady() || !fhpInport_.isReady() || !outport_.isReady() || !outportDistanceText_.isReady())
        return false;

    return true;
}

tgt::ivec2 SurfaceMeasure::clampToViewport(tgt::ivec2 mousePos) {
    tgt::ivec2 result = mousePos;
    tgt::ivec2 size = imgInport_.getSize();
    if (result.x < 0) result.x = 0;
    else if (result.x > size.x-1) result.x = size.x-1;
    if (result.y < 0) result.y = 0;
    else if (result.y > size.y-1) result.y = size.y-1;
    return result;
}

void SurfaceMeasure::undo(tgt::MouseEvent* e) {

    // reset all parameters
    if(e->action() & tgt::MouseEvent::PRESSED) {
        mouseDown_=false;
        mouseStartPos2D_ = tgt::ivec2(0,0);
        mouseCurPos2D_ = tgt::ivec2(0,0);
        mouseCurPos3D_ = tgt::vec4(0.0f);
        mouseStartPos3D_ = tgt::vec4(0.0f);
        distance_ = 0.0f;
        invalidate();
        e->accept();
    }
}

void SurfaceMeasure::measure(tgt::MouseEvent* e) {
    const VolumeBase* refVolume = refInport_.getData();
    if(!refVolume) {
        LERROR("No reference volume");
        return;
    }

    // left mouse button clicked for the first time
    if (e->action() & tgt::MouseEvent::PRESSED) {
        mouseStartPos2D_ = clampToViewport(tgt::ivec2(e->coord().x, e->viewport().y-e->coord().y));
        tgt::vec4 fhp;
        fhpInport_.activateTarget();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(mouseStartPos2D_.x, mouseStartPos2D_.y, 1, 1, GL_RGBA, GL_FLOAT, &fhp);
        if (length(fhp) > 0.0f) {
            mouseDown_ = true;
            distance_ = 0.0f;
            mouseStartPos3D_ = refVolume->getTextureToWorldMatrix() * fhp;
            e->accept();
        }
        fhpInport_.deactivateTarget();
    }

    // mouse movement
    if (e->action() & tgt::MouseEvent::MOTION) {
        if (mouseDown_) {
            // check domain
            tgt::ivec2 oldMouseCurPos2D_ = mouseCurPos2D_;
            mouseCurPos2D_ = clampToViewport(tgt::ivec2(e->coord().x, e->viewport().y-e->coord().y));
            tgt::vec4 fhp;
            fhpInport_.activateTarget();
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glReadPixels(mouseCurPos2D_.x, mouseCurPos2D_.y, 1, 1, GL_RGBA, GL_FLOAT, &fhp);
            if (length(fhp) > 0.0f) {
                mouseCurPos3D_ = refVolume->getTextureToWorldMatrix() * fhp;
                invalidate();
            } else {
                mouseCurPos2D_ = oldMouseCurPos2D_;
            }
            e->accept();
            fhpInport_.deactivateTarget();
        }
    }

    //mouse released
    if (e->action() & tgt::MouseEvent::RELEASED) {
        if (mouseDown_) {
            distance_ = surfaceDistance();
            invalidate();
            e->accept();
        }
    }
}

  float SurfaceMeasure::measureX() {
    const VolumeBase* refVolume = refInport_.getData();
    if(!refVolume) {
      // this could never happen due to the test in the previous function,
      // but we never know...
      LERROR("No reference volume");
      return 0;
    }
    float dist=0;
    tgt::vec3 tmp, tmp1, derivate;
    tgt::vec3 one = tgt::vec3(1.0f);
    tgt::ivec2 start, end;

    if (mouseStartPos2D_.x > mouseCurPos2D_.x){
      start = mouseCurPos2D_;
      end = mouseStartPos2D_;
    } else {
      start = mouseStartPos2D_;
      end = mouseCurPos2D_;
    }

    float m = ((float)end.y - (float)start.y) / ((float)end.x - (float)start.x);
    float b = start.y-m*start.x;




    for(float i=start.x+1;i!=end.x;++i){
      // calculate difference quotient
      tmp  = fhpInport_.getRenderTarget()->getColorAtPos(tgt::ivec2(i,   m*i+b)).xyz();
      tmp1 = fhpInport_.getRenderTarget()->getColorAtPos(tgt::ivec2(i+1, m*(i+1)+b)).xyz();
      tmp  = refVolume->getTextureToWorldMatrix() * tmp;
      tmp1 = refVolume->getTextureToWorldMatrix() * tmp1;
      derivate = (tmp1-tmp)/one;

      // add the current point to the points list
      pointsListX_.push_back(tmp);

      // integrate over the square root of the difference quotient of the elements
      dist+=sqrt(derivate.x*derivate.x+derivate.y*derivate.y+derivate.z*derivate.z);
    }
    return dist;
}

  float SurfaceMeasure::measureY() {
    const VolumeBase* refVolume = refInport_.getData();
    if(!refVolume) {
      // this could never happen due to the test in the previous function,
      // but we never know...
      LERROR("No reference volume");
      return 0;
    }
    tgt::ivec2 start, end;
    float dist=0;
    tgt::vec3 tmp, tmp1, derivate;
    tgt::vec3 one = tgt::vec3(1.0f);

    if(mouseStartPos2D_.y > mouseCurPos2D_.y) {
      start = mouseCurPos2D_;
      end = mouseStartPos2D_;
    } else {
      start = mouseStartPos2D_;
      end = mouseCurPos2D_;
    }
    float m = ((float)end.x - (float)start.x) / ((float)end.y - (float)start.y);
    float b = start.x-m*start.y;


    for(float i=start.y+1;i!=end.y;++i){
        // calculate difference quotient
        tmp  = fhpInport_.getRenderTarget()->getColorAtPos(tgt::ivec2(m*i+b,   i)).xyz();
        tmp1 = fhpInport_.getRenderTarget()->getColorAtPos(tgt::ivec2(m*(i+1)+b, i+1)).xyz();
        tmp  = refVolume->getTextureToWorldMatrix() * tmp;
        tmp1 = refVolume->getTextureToWorldMatrix() * tmp1;
        derivate = (tmp1-tmp)/one;

        // add the current point to the points list
        pointsListY_.push_back(tmp);

        // integrate over the square root of the difference quotient of the elements
        dist+=sqrt(derivate.x*derivate.x+derivate.y*derivate.y+derivate.z*derivate.z);
    }
    return dist;
}

float SurfaceMeasure::surfaceDistance(){
    LDEBUG("Started New Points");
    pointsListX_.clear();
    pointsListY_.clear();
    float xval = measureX();
    float max = std::max(xval, measureY());

    PointListGeometryVec3* positions = new PointListGeometryVec3();
    if (max == xval)
        positions->setData(pointsListX_);
    else
        positions->setData(pointsListY_);
    
    outportDistance_.setData(positions);
    return max;
}

void SurfaceMeasure::process() {
    if (getInvalidationLevel() >= Processor::INVALID_PROGRAM)
        compile();

    const VolumeBase* refVolume = refInport_.getData();
    if(!refVolume) {
        LERROR("No reference volume");
        return;
    }

    std::ostringstream ss;
    ss << distance_;
    outportDistanceText_.setData(ss.str());

    outport_.activateTarget();
    outport_.clearTarget();

    TextureUnit colorUnit, depthUnit;
    imgInport_.bindTextures(colorUnit.getEnum(), depthUnit.getEnum());

    // initialize shader
    program_->activate();
    setGlobalShaderParameters(program_);
    program_->setUniform("colorTex_", colorUnit.getUnitNumber());
    program_->setUniform("depthTex_", depthUnit.getUnitNumber());
    imgInport_.setTextureParameters(program_, "textureParameters_");

    renderQuad();

    program_->deactivate();
    TextureUnit::setZeroUnit();

    outport_.deactivateTarget();
    LGL_ERROR;

}

} // namespace voreen
