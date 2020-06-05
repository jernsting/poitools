#ifndef VRN_POITOOLS_H
#define VRN_POITOOLS_H
 
//include module base class
#include "voreen/core/voreenmodule.h"
 
//use namespace voreen
namespace voreen {
 
/**
 * Each Module has to inherit from VoreenModule.
 */
class PoiTools : public VoreenModule {
 
public:
 
    /**
     * Constructor of the module.
     * @param modulePath the path to the module
     */
    PoiTools(const std::string& modulePath);
 
    /**
     * Sets the description to be shown in the VoreenVE GUI.
     */
    virtual std::string getDescription() const;
};
 
} // namespace
 
#endif //VRN_POITOOLS_H