#include "pointfitting.h"

#include "voreen/core/voreenapplication.h"

#include "tgt/textureunit.h"
#include "tgt/filesystem.h"

#include <sstream>

using tgt::TextureUnit;

namespace voreen {


PointFitting::PointFitting()
    : ImageProcessor("pointfitting")
    , imgInport_(Port::INPORT, "image", "Image Input")
    , fhpInport_(Port::INPORT, "fhp", "First-hit-points Input", false, Processor::INVALID_PROGRAM, RenderPort::RENDERSIZE_DEFAULT, GL_RGBA16F)
    , refInport_(Port::INPORT, "refvol", "Reference Volume", false)
    , outport_(Port::OUTPORT, "image.output", "Image Output")
    , outportPicked_(Port::OUTPORT, "outport.picked", "Picked Points Geometry")
    , pointListFile_("pointsFile", "Mandatory Points File", "Open Mandatory Points File", VoreenApplication::app()->getUserDataPath(), "Mandatory Points File (*.txt)")
    , mouseEventProp_("mouseEvent.measure", "Point Fitting", this, &PointFitting::measure, tgt::MouseEvent::MOUSE_BUTTON_LEFT, tgt::MouseEvent::PRESSED | tgt::MouseEvent::RELEASED, tgt::Event::ALT, false)
    , mouseUndoProp_("mouseEvent.undo", "Undo Point Fitting", this, &PointFitting::undo, tgt::MouseEvent::MOUSE_BUTTON_RIGHT, tgt::MouseEvent::PRESSED | tgt::MouseEvent::RELEASED, tgt::Event::ALT, false)    
    , camera_("camera", "Camera", tgt::Camera(tgt::vec3(0.f, 0.f, 3.5f), tgt::vec3(0.f, 0.f, 0.f), tgt::vec3(0.f, 1.f, 0.f)))
    , renderSpheres_("renderSpheres", "Render Spheres", true)
    , font_(VoreenApplication::app()->getFontPath("VeraMono.ttf"), 16)
    , mouseCurPos2D_(0.0f)
    , mouseCurPos3D_(0.0f)
    , pointsList_()
    , numSelectedPoints_(0)
    , mouseDown_(false)
    , mesh_()             //
    , lightSource_()      // Are initialized below
    , material_()         //
    , mandatoryPoints_()  //
    , forceReload_(false)
{
    addPort(imgInport_);
    addPort(fhpInport_);
    addPort(refInport_);
    addPort(outport_);
    addPort(outportPicked_);

    pointListFile_.onChange(MemberFunctionCallback<PointFitting>(this, &PointFitting::forceReload));

    addProperty(camera_);
    addProperty(renderSpheres_);
    addProperty(pointListFile_);

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

PointFitting::~PointFitting() {
}

Processor* PointFitting::create() const {
    return new PointFitting();
}

void PointFitting::initialize() {
    Processor::initialize();
    forceReload_ = true;
}

bool PointFitting::isReady() const {
    if (!isInitialized() || !imgInport_.isReady() || !fhpInport_.isReady() || !outport_.isReady())
        return false;

    return true;
}

tgt::ivec2 PointFitting::clampToViewport(tgt::ivec2 mousePos) {
    tgt::ivec2 result = mousePos;
    tgt::ivec2 size = imgInport_.getSize();
    if (result.x < 0) result.x = 0;
    else if (result.x > size.x-1) result.x = size.x-1;
    if (result.y < 0) result.y = 0;
    else if (result.y > size.y-1) result.y = size.y-1;
    return result;
}

void PointFitting::undo(tgt::MouseEvent* e) {
    if(e->action() & tgt::MouseEvent::PRESSED){
        if(!pointsList_.empty()) {
            LINFO("Removed last element");
            pointsList_.pop_back();
            e->accept();
            invalidate();
            numSelectedPoints_--;
        } else {
            LERROR("List of Elements is empty");
        }
    }
}

void PointFitting::measure(tgt::MouseEvent* e) {
    const VolumeBase* refVolume = refInport_.getData();
    if(!refVolume) {
        LERROR("No reference volume");
        return;
    }
    if (e->action() & tgt::MouseEvent::PRESSED) {
        mouseCurPos2D_ = clampToViewport(tgt::ivec2(e->coord().x, e->viewport().y-e->coord().y));
        tgt::vec3 pickedPos = fhpInport_.getRenderTarget()->getColorAtPos(mouseCurPos2D_).xyz();
        if(pickedPos.x != 0 && pickedPos.y != 0 && pickedPos.z !=0){
            mouseCurPos3D_ = refVolume->getTextureToWorldMatrix() * pickedPos;
            mouseDown_ = true;
            std::stringstream out;
            out << mouseCurPos3D_.x << " " << mouseCurPos3D_.y << " " << mouseCurPos3D_.z;            
            LINFO(out.str());
            pointsList_.push_back(tgt::vec3(mouseCurPos3D_));
            e->accept();
            invalidate();
            numSelectedPoints_++;
        }
        fhpInport_.deactivateTarget();
    }
}

void PointFitting::process() {
    if (pointListFile_.get() != "" && forceReload_) {
        try {
            readMandatoryPoints();
        }
        catch (tgt::FileNotFoundException& f) {
            LERROR(f.what());
        }
        forceReload_=false;
    }

    if (getInvalidationLevel() >= Processor::INVALID_PROGRAM)
        compile();

    const VolumeBase* refVolume = refInport_.getData();
    if(!refVolume) {
        LERROR("No reference volume");
        return;
    }


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

    // render text
    glDisable(GL_DEPTH_TEST);

    if(numSelectedPoints_ < mandatoryPoints_.size()){
        std::stringstream out;
        out << mandatoryPoints_.at(numSelectedPoints_);
        std::string label = out.str();

        MatStack.matrixMode(tgt::MatrixStack::MODELVIEW);
        MatStack.pushMatrix();
        MatStack.loadIdentity();
        MatStack.translate(-1.0f, -1.0f, 0.0f);
        float scaleFactorX = 2.0f / static_cast<float>(imgInport_.getSize().x);
        float scaleFactorY = 2.0f / static_cast<float>(imgInport_.getSize().y);
        MatStack.scale(scaleFactorX, scaleFactorY, 1);
        tgt::ivec2 screensize = imgInport_.getSize();

        // Plot the label
        font_.setFontColor(tgt::vec4(255.0f, 0.0f, 0.0f, 1.f));
        font_.render(tgt::vec3(static_cast<float>(11), static_cast<float>(11), 0.0f), label, screensize);
        font_.setFontColor(tgt::vec4(0.7f, 0.7f, 0.7f, 1.f));
        font_.render(tgt::vec3(static_cast<float>(10), static_cast<float>(10), 0.0f), label, screensize);

        MatStack.popMatrix();
        glEnable(GL_DEPTH_TEST);
    }

    // render points in points list
    if(!pointsList_.empty()) {
        if(renderSpheres_.get()) {
            IMode.setLightSource(lightSource_);
            IMode.color(255.0f, 0.5f, 0.5f, 0.5f);
            IMode.setMaterial(material_);
            float sphereRadius = tgt::length(refVolume->getCubeSize())*0.005;

            // set modelview and projection matrices
            MatStack.matrixMode(tgt::MatrixStack::PROJECTION);
            MatStack.pushMatrix();
            MatStack.loadMatrix(camera_.get().getProjectionMatrix(outport_.getSize()));

            MatStack.matrixMode(tgt::MatrixStack::MODELVIEW);
            MatStack.pushMatrix();
            MatStack.loadIdentity();
            MatStack.loadMatrix(camera_.get().getViewMatrix());

            for (auto point = pointsList_.begin(); point != pointsList_.end(); ++point) {
                MatStack.pushMatrix();
                MatStack.translate(point->x, point->y, point->z);
                MatStack.scale(tgt::vec3(sphereRadius));
                mesh_.render();
                MatStack.popMatrix();
            }

            MatStack.popMatrix();

            MatStack.matrixMode(tgt::MatrixStack::PROJECTION);
            MatStack.popMatrix();
        }

            MatStack.matrixMode(tgt::MatrixStack::MODELVIEW);
            IMode.color(1.0f, 1.0f, 1.0f, 1.0f);
    }


    outport_.deactivateTarget();
    LGL_ERROR;

    PointListGeometryVec3* positions = new PointListGeometryVec3();
    positions->setData(pointsList_);
    outportPicked_.setData(positions);

}

void PointFitting::readMandatoryPoints() {
    std::string filename = pointListFile_.get();

    if (pointListFile_.get() == "")
        return;

    if (!tgt::FileSystem::fileExists(pointListFile_.get()))
        throw tgt::FileNotFoundException("File does not exist", pointListFile_.get());

    LINFO("Reading Point List File " << pointListFile_.get());

    std::string str;
    std::ifstream pointListFile(pointListFile_.get());
    while (std::getline(pointListFile, str))
    {
	// Line contains string of length > 0 then save it in vector
	if(str.size() > 0)
		mandatoryPoints_.push_back(str);
    }
}

void PointFitting::forceReload() {
    forceReload_ = true;
    pointsList_.clear();
    invalidate();
}

} // namespace voreen
