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
#include <CtrMesh.h>
#include <CtrIVertexDeclaration.h>
#include <CtrVertexStream.h>
#include <CtrLog.h>
#include <CtrIVertexBuffer.h>
#include <CtrIGpuBuffer.h>
#include <CtrTransformProperty.h>
#include <CtrRenderRequest.h>
#include <CtrIShader.h>
#include <CtrTextureMgr.h>
#include <CtrITexture.h>

namespace Ctr
{
//-------------------------------
// Mesh
Mesh::Mesh(Ctr::IDevice* device) :
Ctr::TransformNode(device),
_vertexDeclaration (0),
_vertexBuffer (0),
_primitiveCount (0),
_primitiveDrawCount (0),
_vertexCount (0),
_locked (false),
_entity (0),
_material (0),
_topologySubtype(Tri),
_groupId(0)
{
    _visible = new BoolProperty (this, std::string("visible"));
    setVisible (true);
    _dynamic = false;
    _useResource = false;
}

Mesh::~Mesh()
{
    _device->destroyResource(_vertexBuffer);
}

uint32_t
Mesh::groupId() const
{
    return _groupId;
}

void
Mesh::setGroupId(uint32_t group)
{
    _groupId = group;
}

bool
Mesh::dynamic() const 
{ 
    return _dynamic; 
}

void
Mesh::setDynamic (bool value) 
{ 
    _dynamic = value;
}

bool
Mesh::visible() const
{
    return _visible->get();
}

void
Mesh::setVisible (bool visible)
{
    _visible->set (visible);
}

bool
Mesh::locked() const
{
    return _locked;
}

const Ctr::Material*    
Mesh::material () const
{
    return _material;
}

Ctr::Material*    
Mesh::material ()
{
    return _material;
}

void
Mesh::setMaterial (Ctr::Material* material)
{
    _material = material;
}

const Ctr::Entity*
Mesh::entity() const
{
    return _entity;
}

Ctr::Entity*
Mesh::entity()
{
    return _entity;
}

void
Mesh::setEntity (Ctr::Entity* entity)
{
    _entity = entity;
}

void
Mesh::setVertexDeclaration (const IVertexDeclaration* declaration)
{
    _vertexDeclaration = declaration;
}

const IVertexDeclaration*
Mesh::vertexDeclaration()  const
{ 
    return _vertexDeclaration; 
}

void*                   
Mesh::internalStreamPtr() 
{ 
    return 0;
}

bool
Mesh::create()
{
    if (!_vertexBuffer)
    {
        void* vbData = internalStreamPtr();
        _useResource = vbData != nullptr;

        VertexBufferParameters vertexBufferParameters;
        vertexBufferParameters = 
                VertexBufferParameters (vertexBufferSize(), false, false, 
                                        _vertexDeclaration->vertexStride(), 
                                        vbData, _useResource, 
                                        _dynamic);

        if (_vertexBuffer = _device->createVertexBuffer(&vertexBufferParameters))
        {
            _vertexBuffer->setVertexDeclaration (_vertexDeclaration);
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool
Mesh::free()
{
    return true;
}

void
Mesh::setVertexCount (uint32_t vertexCount)
{
    _vertexCount = vertexCount;
}

uint32_t
Mesh::vertexCount() const
{
    return _vertexCount;
}

void
Mesh::setPrimitiveCount (uint32_t primitiveCount)
{
    _primitiveCount = primitiveCount;
    _primitiveDrawCount = primitiveCount;
}

uint32_t
Mesh::primitiveCount() const
{
    return _primitiveCount;
}

void
Mesh::setPrimitiveDrawCount (uint32_t primitiveDrawCount)
{
    if (primitiveDrawCount <= _primitiveCount)
    {
        _primitiveDrawCount = primitiveDrawCount;
    }
    else
    {
        LOG ("Error, attempting to set primitive draw count to greater than draw count");
        _primitiveDrawCount = _primitiveCount;
    }
}

uint32_t
Mesh::primitiveDrawCount() const
{
    return _primitiveDrawCount;
}

void
Mesh::setPrimitiveType (Ctr::PrimitiveType primitiveType)
{
    _primitiveType = primitiveType;
}

Ctr::PrimitiveType                
Mesh::primitiveType () const
{
    return _primitiveType;
}

void*
Mesh::lock() 
{
    _locked = true; 
    return _vertexBuffer->lock(0);
}

bool
Mesh::unlock()
{
    _locked = false;
    return _vertexBuffer->unlock();
}

long
Mesh::vertexBufferSize()
{
    if (_vertexDeclaration)
    {
        return (uint32_t)(_vertexCount * (_vertexDeclaration->vertexStride()));
    }
    LOG ("WARNING, null vertex declaration, return 0 vertex buffer size");
    return 0;
}

bool
Mesh::render(const Ctr::RenderRequest* request,   
             const Ctr::GpuTechnique* technique) const
{

    {
        return _device->drawPrimitive (_vertexDeclaration, _vertexBuffer, technique, 
                                      (PrimitiveType)primitiveType(), _primitiveCount, 0);
    }
    return true;
}

const IVertexBuffer*
Mesh::vertexBuffer() const
{
    return _vertexBuffer;
}

}