#include "modules/mod_sampler2d.frag"
#include "modules/mod_normdepth.frag"

uniform sampler2D colorTex_;
uniform sampler2D depthTex_;
uniform TextureParameters textureParameters_;

void main() {
    FragData0 = textureLookup2Dscreen(colorTex_, textureParameters_, gl_FragCoord.xy);
    gl_FragDepth = textureLookup2Dscreen(depthTex_, textureParameters_, gl_FragCoord.xy).x;
}
