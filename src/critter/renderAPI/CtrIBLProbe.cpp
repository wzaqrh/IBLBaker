//------------------------------------------------------------------------------------//
//                                                                                    //
//               _________        .__  __    __                                       //
//               \_   ___ \_______|__|/  |__/  |_  ___________                        //
//               /    \  \/\_  __ \  \   __\   __\/ __ \_  __ \                       //
//               \     \____|  | \/  ||  |  |  | \  ___/|  | \/                       //
//                \______  /|__|  |__||__|  |__|  \___  >__|                          //
//                       \/                           \/                              //
//                                                                                    //
//    Critter is provided under the MIT License(MIT)                                  //
//    Critter uses portions of other open source software.                            //
//    Please review the LICENSE file for further details.                             //
//                                                                                    //
//    Copyright(c) 2015 Matt Davidson                                                 //
//                                                                                    //
//    Permission is hereby granted, free of charge, to any person obtaining a copy    //
//    of this software and associated documentation files(the "Software"), to deal    //
//    in the Software without restriction, including without limitation the rights    //
//    to use, copy, modify, merge, publish, distribute, sublicense, and / or sell     //
//    copies of the Software, and to permit persons to whom the Software is           //
//    furnished to do so, subject to the following conditions :                       //
//                                                                                    //
//    1. Redistributions of source code must retain the above copyright notice,       //
//    this list of conditions and the following disclaimer.                           //
//    2. Redistributions in binary form must reproduce the above copyright notice,    //
//    this list of conditions and the following disclaimer in the                     //
//    documentation and / or other materials provided with the distribution.          //
//    3. Neither the name of the copyright holder nor the names of its                //
//    contributors may be used to endorse or promote products derived                 //
//    from this software without specific prior written permission.                   //
//                                                                                    //
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      //
//    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        //
//    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE      //
//    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          //
//    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   //
//    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN       //
//    THE SOFTWARE.                                                                   //
//                                                                                    //
//------------------------------------------------------------------------------------//

#include <CtrIBLProbe.h>
#include <CtrIDevice.h>
#include <CtrITexture.h>
#include <CtrISurface.h>
#include <CtrPostEffect.h>
#include <CtrPostEffectsMgr.h>
#include <CtrScene.h>
#include <CtrMatrixAlgo.h>
#include <MurmurHash.h>
#include <Ctrimgui.h>


namespace Ctr
{

ImguiEnumVal IblFormatEnum[] =
{
    { Ctr::PF_FLOAT16_RGBA, "RGBA 16" },
    { Ctr::PF_FLOAT32_RGBA, "RGBA 32" }
};
static const EnumTweakType IblFormatType(&IblFormatEnum[0], 2, "ProbeFormat");

ImguiEnumVal IblSourceResolutionEnum[] =
{
    { 64, "64" },
    { 128, "128" },
	{ 256, "256" },
    { 512, "512" },
    { 1024, "1024" },
    { 2048, "2048" }
};

static const EnumTweakType IblSourceResolutionType(&IblSourceResolutionEnum[0], 6, "SourceResolution");

IBLProbe::IBLProbe(Ctr::IDevice * device) : 
    Ctr::TransformNode(device),
    _environmentCubeMap(nullptr),
    _device (device),
    _center (Vector3f(0,0,0)),
    _sampleOffset(0),
    _cachedRotation (Ctr::Vector3f (Ctr::Limits<float>::minimum(),Ctr::Limits<float>::minimum(),Ctr::Limits<float>::minimum())),
    _cachedTranslation (Ctr::Vector3f (Ctr::Limits<float>::minimum(),Ctr::Limits<float>::minimum(),Ctr::Limits<float>::minimum())),
    _renderId(0),
    _samplesRemaining(1024),
    _sampleCountProperty(new Ctr::IntProperty(this, "Total Samples", new Ctr::TweakFlags(0, 16384, 1, "IBL"))),
    _samplesPerFrameProperty(new Ctr::IntProperty(this, "Samples Per Frame", new Ctr::TweakFlags(0, 16384, 1, "IBL"))),
    _markedComputedProperty(new Ctr::BoolProperty(this, "Computed")),
    _diffuseResolutionProperty(new Ctr::IntProperty(this, "Diffuse Resolution", new TweakFlags(&IblSourceResolutionType, "IBL"))),
    _specularResolutionProperty(new Ctr::IntProperty(this, "Specular Resolution", new TweakFlags(&IblSourceResolutionType, "IBL"))),
    _environmentScaleProperty(new Ctr::FloatProperty(this, "Environment Scale", new Ctr::TweakFlags(0, 12.0f, 1e-3f, "IBL"))),
    _mipDropProperty(new Ctr::IntProperty(this, "Mip Drop", new Ctr::TweakFlags(0, 20, 1, "IBL"))),
    _iblSaturationProperty(new Ctr::FloatProperty(this, "IBL Saturation", new Ctr::TweakFlags(0, 2.0f, 1e-3f, "IBL"))),
    _iblContrastProperty(new Ctr::FloatProperty(this, "IBL Contrast", new Ctr::TweakFlags(0, 2.0f, 1e-3f, "IBL"))),
    _iblHueProperty(new Ctr::FloatProperty(this, "IBL Hue", new Ctr::TweakFlags(0, 360.0f, 0.25f, "IBL"))),
    _maxPixelRProperty(new Ctr::FloatProperty(this, "Max R", new Ctr::TweakFlags(0, 360.0f, 0.25f, "IBL"))),
    _maxPixelGProperty(new Ctr::FloatProperty(this, "Max G", new Ctr::TweakFlags(0, 360.0f, 0.25f, "IBL"))),
    _maxPixelBProperty(new Ctr::FloatProperty(this, "Max B", new Ctr::TweakFlags(0, 360.0f, 0.25f, "IBL"))),
    _deviceProperty(new DeviceProperty(this, "device")),
    _hdrPixelFormatProperty(new PixelFormatProperty(this, "HDRFormat", new TweakFlags(&IblFormatType, "IBL"))),
    _mdrPixelFormatProperty(new PixelFormatProperty(this, "MDRFormat", new TweakFlags(&IblFormatType, "IBL"))),
    _dimensionProperty(new IntProperty(this, "Dimension")),
    _sourceResolutionProperty(new IntProperty(this, "Source Resolution", new TweakFlags(&IblSourceResolutionType, "IBL")))
{
    _samplesPerFrameProperty->set(1024);
    _sampleCountProperty->set(1024);
    _sourceResolutionProperty->set(2048);
    _markedComputedProperty->set(false);
    _diffuseResolutionProperty->set(128);
    _specularResolutionProperty->set(512);
    _environmentScaleProperty->set(2.0);
    _mipDropProperty->set(0);
    _iblHueProperty->set(0.0f);
    _iblContrastProperty->set(0.0f);
    _iblSaturationProperty->set(1.0f);

    _maxPixelRProperty->set(0);
    _maxPixelGProperty->set(0);
    _maxPixelBProperty->set(0);
    
    _hdrPixelFormatProperty->set(Ctr::PF_FLOAT32_RGBA);
    _mdrPixelFormatProperty->set(Ctr::PF_A8R8G8B8);

    _deviceProperty->set(_device);
    _dimensionProperty->set(Ctr::CubeMap);

    // RTT Properties.
    for (uint32_t map = 0; map < 2; map++)
    {
        _specularCubeMap[map] = new RenderTextureProperty(this, "SpecularHDR", true /* use mips */, false /* gen mips*/);
        setupCubeMap (_specularCubeMap[map], _specularResolutionProperty, _hdrPixelFormatProperty);
    }    

    for (uint32_t map = 0; map < 2; map++)
    {
        _diffuseCubeMap[map] = new RenderTextureProperty(this, "DiffuseHDR", false /* use mips */, false /* gen mips*/);
        setupCubeMap(_diffuseCubeMap[map], _diffuseResolutionProperty, _hdrPixelFormatProperty);
    }

    _environmentCubeMap = new RenderTextureProperty(this, "SourceHDR", true /* use mips */, true /* gen mips*/ );
    setupCubeMap(_environmentCubeMap, _sourceResolutionProperty, _hdrPixelFormatProperty);

    _environmentCubeMapMDR = new RenderTextureProperty(this, "SourceMDR", true /* use mips */, true /* gen mips*/);
    setupCubeMap(_environmentCubeMapMDR, _sourceResolutionProperty, _mdrPixelFormatProperty);

    _diffuseCubeMapMDR = new RenderTextureProperty(this, "DiffuseMDR", false /* use mips */, false /* gen mips*/);
    setupCubeMap(_diffuseCubeMapMDR, _diffuseResolutionProperty, _mdrPixelFormatProperty);

    _specularCubeMapMDR = new RenderTextureProperty(this, "SpecularMDR", true /* use mips */, false /* gen mips*/);
    setupCubeMap(_specularCubeMapMDR, _specularResolutionProperty, _mdrPixelFormatProperty);
}

IBLProbe::~IBLProbe()
{
}

void
IBLProbe::setupCubeMap(Ctr::RenderTextureProperty* cubeMapProperty, 
                       IntProperty* size, 
                       PixelFormatProperty* format)
{
    cubeMapProperty->addDependency(_dimensionProperty, RenderTextureProperty::Dimension);
    cubeMapProperty->addDependency(size, RenderTextureProperty::Width);
    cubeMapProperty->addDependency(size, RenderTextureProperty::Height);
    cubeMapProperty->addDependency(_deviceProperty, RenderTextureProperty::Device);
    cubeMapProperty->addDependency(format, RenderTextureProperty::Format);
}

int32_t
IBLProbe::mipDrop() const
{
    return _mipDropProperty->get();
}

IntProperty*
IBLProbe::mipDropProperty()
{
    return _mipDropProperty;
}

float
IBLProbe::environmentScale() const
{
    return _environmentScaleProperty->get();
}


FloatProperty*
IBLProbe::maxPixelRProperty()
{
    return _maxPixelRProperty;
}

float
IBLProbe::maxPixelR() const
{
    return _maxPixelRProperty->get();
}

FloatProperty*
IBLProbe::maxPixelGProperty()
{
    return _maxPixelGProperty;
}

float
IBLProbe::maxPixelG() const
{
    return _maxPixelGProperty->get();
}

FloatProperty*
IBLProbe::maxPixelBProperty()
{
    return _maxPixelBProperty;
}

float
IBLProbe::maxPixelB() const
{
    return _maxPixelBProperty->get();
}

Ctr::FloatProperty*
IBLProbe::environmentScaleProperty()
{
    return _environmentScaleProperty;
}

int32_t
IBLProbe::diffuseResolution() const
{
    return _diffuseResolutionProperty->get();
}

int32_t
IBLProbe::specularResolution() const
{
    return _specularResolutionProperty->get();
}

IntProperty*
IBLProbe::diffuseResolutionProperty()
{
    return _diffuseResolutionProperty;
    /*
    bool recreate = false;
    if (_diffuseResolution != resolution)
        recreate = true;
    _diffuseResolution = resolution;
    if (recreate)
        createDiffuseResources();
    */
}

IntProperty*
IBLProbe::specularResolutionProperty()
{
    /*
    bool recreate = false;
    if (_specularResolution != resolution)
        recreate = true;
    _specularResolution = resolution;
    if (recreate)
        createSpecularResources();
    */
    return _specularResolutionProperty;
}


float
IBLProbe::iblContrast() const
{
    return _iblContrastProperty->get();
}

FloatProperty*
IBLProbe::iblContrastProperty()
{
    return _iblContrastProperty;
}

float
IBLProbe::iblHue() const
{
    return _iblSaturationProperty->get();
}

FloatProperty*
IBLProbe::iblHueProperty()
{
    return _iblHueProperty;
}

float
IBLProbe::iblSaturation() const
{
    return _iblSaturationProperty->get();
}

FloatProperty*
IBLProbe::iblSaturationProperty()
{
    return _iblSaturationProperty;
}

void
IBLProbe::markComputed(bool computed)
{
    _markedComputedProperty->set(computed);
}

ITexture*
IBLProbe::diffuseCubeMap() const
{
    return _diffuseCubeMap[_renderId]->renderTexture();
}

Ctr::ITexture*
IBLProbe::specularCubeMap() const
{
    return _specularCubeMap[_renderId]->renderTexture();
}

Ctr::ITexture*
IBLProbe::lastDiffuseCubeMap() const
{
    return _diffuseCubeMap[_renderId == 1 ? 0 : 1]->renderTexture();
}

Ctr::ITexture*
IBLProbe::environmentCubeMapMDR() const
{
    return _environmentCubeMapMDR->renderTexture();
}

Ctr::ITexture*
IBLProbe::diffuseCubeMapMDR() const
{
    return _diffuseCubeMapMDR->renderTexture();
}

Ctr::ITexture*
IBLProbe::specularCubeMapMDR() const
{
    return _specularCubeMapMDR->renderTexture();
}

Ctr::ITexture*
IBLProbe::lastSpecularCubeMap() const
{
    return _specularCubeMap[_renderId == 1 ? 0 : 1]->renderTexture();
}

int32_t
IBLProbe::samplesRemaining() const
{
    return _samplesRemaining;
}

int32_t
IBLProbe::samplesPerFrame() const
{
    return _samplesPerFrameProperty->get();
}

IntProperty*
IBLProbe::samplesPerFrameProperty()
{
    return _samplesPerFrameProperty;
}

int32_t
IBLProbe::sampleCount() const
{
    return _sampleCountProperty->get();
}

IntProperty*
IBLProbe::sampleCountProperty()
{
    return _sampleCountProperty;
}

int32_t
IBLProbe::sampleOffset() const
{
    return _sampleOffset;
}

void
IBLProbe::updateSamples()
{
    _samplesRemaining -= _samplesPerFrameProperty->get();
    if (_samplesRemaining > 0)
    {
        _renderId = _renderId == 0 ? 1 : 0;
        _sampleOffset += 1;
    }

    LOG ("Samples remaining " << _samplesRemaining << "\n");
    LOG ("Sample offset  " << _sampleOffset << "\n");
    LOG ("Render Id " << _renderId << "\n");
}

bool
IBLProbe::computed() const
{
    if(_samplesRemaining <= 0 || _markedComputedProperty->get() == true)
        return true;
    return false;
}

void
IBLProbe::uncache()
{
    _markedComputedProperty->set(false);
    _samplesRemaining = _sampleCountProperty->get();
    _sampleOffset = 0;
    _renderId = 0;
}

bool
IBLProbe::isCached ()
{
    // For now
    return computed();
}

void
IBLProbe::update()
{
    std::ostringstream stream;
    stream << _specularResolutionProperty->get() << 
              _diffuseResolutionProperty->get() << 
              _mipDropProperty->get() << 
              _sampleCountProperty->get() << 
              _samplesPerFrameProperty->get() <<
              _iblHueProperty->get() <<
              _iblContrastProperty->get() <<
              _iblSaturationProperty->get() <<
              _hdrPixelFormatProperty->get() <<
              _sourceResolutionProperty->get() <<
              _environmentScaleProperty->get();

    // Compute hash using Murmur
    Hash hash;
    hash.build(stream.str());

    // Compare the hash
    if (_probeHash != hash)
    {
        _probeHash = hash;

        // Setup the probe to render again.
        uncache();
    }
}

const Ctr::Matrix44f &
IBLProbe::basis() const
{
    return _basis;
}

const Ctr::Matrix44f &
IBLProbe::inverseBasis() const
{
    return _inverseBasis;
}

ITexture*
IBLProbe::environmentCubeMap() const
{
    return _environmentCubeMap->renderTexture();
}


const Ctr::Vector3f&
IBLProbe::center() const
{
    return _center;
}

void
IBLProbe::setCenter(const Ctr::Vector3f & center)
{
    _center = center;
}

IntProperty*
IBLProbe::sourceResolutionProperty()
{
    return _sourceResolutionProperty;
}

int32_t
IBLProbe::sourceRespolution() const
{
    return _sourceResolutionProperty->get();
}

Ctr::PixelFormatProperty*
IBLProbe::hdrPixelFormatProperty()
{
    return _hdrPixelFormatProperty;
}

Ctr::PixelFormat
IBLProbe::hdrPixelFormat() const
{
    return _hdrPixelFormatProperty->get();
}

}