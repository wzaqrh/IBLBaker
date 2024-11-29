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
#ifndef INCLUDED_CRT_BASE_MESH
#define INCLUDED_CRT_BASE_MESH

#include <CtrPlatform.h>
#include <CtrTransformNode.h>
#include <CtrIDevice.h>
#include <CtrTextureImage.h>
#include <CtrRegion.h>

namespace Ctr
{
class Entity;
class IDevice;
class IVertexDeclaration;
class IVertexBuffer;
class GpuTechnique;
class VertexStream;
class Mesh;
class Camera;
class IGpuBuffer;
class Scene;
class RenderRequest;
class Material;

enum TopologySubtype
{
    Tri
};

//--------------------------------------------------------------------
//
// Mesh
//
// Create a mesh by first instantiating a VertexDelcaration.
// Attach each index and size that you want to assign elements for.
// Set streams for each stream that you want to be set on the mesh.
// Call createVertexBuffer.
// The size and stride is determined from the VertexDeclaration.
// Each buffer is filled using the definition of the VertexDeclaration
//
//--------------------------------------------------------------------

class Mesh : public Ctr::TransformNode
{
  public:
    Mesh (Ctr::IDevice*);
    virtual ~Mesh();

    virtual bool                    create();
    virtual bool                    free();

    virtual bool                    render(const Ctr::RenderRequest* request,
                                           const Ctr::GpuTechnique* technique) const;


    void                            setStreamOutCapable (bool streamOutCapable);
    bool                            streamOutCapable () const;

    void                            setVertexDeclaration (const IVertexDeclaration*);
    virtual const IVertexDeclaration* vertexDeclaration() const;

    void                            setVertexCount (uint32_t vertexCount);
    virtual uint32_t                vertexCount() const;

    void                            setPrimitiveCount (uint32_t primitiveCount);
    virtual uint32_t                primitiveCount() const;

    void                            setPrimitiveDrawCount (uint32_t primitiveDrawCount);
    uint32_t                        primitiveDrawCount() const;

    void*                           lock();
    bool                            unlock();

    void                            setPrimitiveType (Ctr::PrimitiveType);
    virtual Ctr::PrimitiveType      primitiveType () const;

    virtual const Ctr::Material*    material () const;
    virtual Ctr::Material*          material ();
    void                            setMaterial (Ctr::Material* material);

    const Ctr::Entity*               entity() const;
    Ctr::Entity*                     entity();

    void                            setEntity (Ctr::Entity* entity);

    uint32_t                        groupId() const;
    void                            setGroupId(uint32_t groupIdIn);

    bool                            locked() const;


    bool                            visible() const;
    void                            setVisible (bool visible);

    bool                            bindVBToStreamOut() const;

    virtual void*                   internalStreamPtr();
    long                            vertexBufferSize();

    bool                            dynamic() const;
    void                            setDynamic (bool value);


    virtual float                   patchCount () const { return 0;}

    TopologySubtype                 topologySubType() const { return _topologySubtype; }
    void                            setTopologySubType(TopologySubtype value)  { _topologySubtype = value; }

    virtual float                   patchPrimitiveSize() const {return 0;}; 
        
    void                            setShadowMask(uint32_t);
    uint32_t                        shadowMask() const;

  protected:
    const IVertexBuffer*            vertexBuffer() const;

    TopologySubtype                 _topologySubtype;
    const IVertexDeclaration*       _vertexDeclaration;

    bool                            _useResource;
    Ctr::Material*                   _material;

  private:
    Ctr::PrimitiveType               _primitiveType;
    IVertexBuffer*                  _vertexBuffer;
    uint32_t                        _vertexCount;
    uint32_t                        _primitiveCount;
    uint32_t                        _primitiveDrawCount;
    bool                            _locked;
    Ctr::Entity*                     _entity;
    Ctr::BoolProperty*               _visible;
    bool                            _dynamic;
    uint32_t                        _groupId;
};
}
#endif
