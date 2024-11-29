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
#ifndef INCLUDED_CRT_VARIABLE_INTERFACE
#define INCLUDED_CRT_VARIABLE_INTERFACE

#include <CtrIRenderResource.h>
#include <CtrShaderParameterValue.h>
#include <CtrLog.h>

namespace Ctr
{
class ITexture;
class GpuTechnique;
class IGpuBuffer;
class IVertexBuffer;
class IIndexBuffer;
class IDepthSurface;

bool isIndexedSemantic (const std::string& targetSemantic);
int semanticIndex (const std::string& targetSemantic);

class GpuVariable : public IRenderResource
{
  public:
    GpuVariable (Ctr::IDevice* device);

    virtual ~GpuVariable();

    virtual void                setParameterType (Ctr::ShaderParameter type) = 0;
    //-----------------------
    // Gets the semantic name
    //-----------------------            
    virtual const std::string&    semantic() const = 0;

    //-----------------------
    // Gets the variable name
    //-----------------------
    virtual const std::string&    name() const = 0;

    //---------------------------
    // Gets the annotation stream
    //---------------------------
    virtual const std::string&    annotation (const std::string&) = 0;

    virtual const ITexture*        texture() const = 0;

    uint32_t                      valueIndex() const { return _valueIndex;};

    virtual void                  unbind() const = 0;

    virtual void                  set (const void*, uint32_t size) const = 0;
    virtual void                  setMatrix(const float*) const = 0;
    virtual void                  setMatrixArray (const float*, uint32_t size) const = 0;
    virtual void                  setVectorArray (const float*, uint32_t size) const = 0;    
    virtual void                  setVector (const float*) const = 0;
    virtual void                  setFloatArray(const float*, uint32_t count) const = 0;
    virtual void                  setTexture (const ITexture*) const = 0;
    virtual void                  setResource (const Ctr::IGpuBuffer*) const = 0;

    virtual void                  setUnorderedResource(const Ctr::IGpuBuffer*) const = 0;
    virtual void                  setStream (const Ctr::IVertexBuffer* vertexBuffer) const = 0;

    virtual void                  setStream (const Ctr::IIndexBuffer* indexBuffer) const = 0;

    virtual void                  setDepthTexture (const Ctr::IDepthSurface*) const = 0;
    const Ctr::IDevice*     device() const;
    Ctr::IDevice*           device();

  protected:
    uint32_t                      _valueIndex;
};

}

#endif