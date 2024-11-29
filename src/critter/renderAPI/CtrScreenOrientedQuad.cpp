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
#include <CtrScreenOrientedQuad.h>
#include <CtrIVertexDeclaration.h>
#include <CtrVertexElement.h>
#include <CtrVertexStream.h>
#include <CtrVector4.h>
#include <CtrVector2.h>
#include <CtrLog.h>
#include <CtrVertexDeclarationMgr.h>
#include <CtrISurface.h>

namespace Ctr
{
ScreenOrientedQuad::ScreenOrientedQuad(Ctr::IDevice* device) : StreamedMesh (device)
{
}

bool
ScreenOrientedQuad::initialize (const Ctr::Region2f& screenLocation, bool background)
{    
    setPrimitiveType (Ctr::TriangleStrip);
    setPrimitiveCount (2);
    setVertexCount (4);

    _screenLocation = screenLocation;
    uint32_t width = 1;
    uint32_t height = 1;

    //create vertex streams and vertex declaration information
    std::vector<VertexElement>       vertexElements;
    vertexElements.push_back (VertexElement( 0, 0,  FLOAT4, METHOD_DEFAULT, POSITION, 0));
    vertexElements.push_back (VertexElement( 0, 16, FLOAT2, METHOD_DEFAULT, TEXCOORD, 0));
    vertexElements.push_back (VertexElement(0xFF,0, UNUSED,0,0,0));

    float lhx = _screenLocation.minExtent.x * width;
    float lhy = _screenLocation.minExtent.y * height;
    float screenWidth = _screenLocation.size().x * width;
    float screenHeight = _screenLocation.size().y * height;

    float z = 0;
    float w = 1;

    if (background)
    {
        z = 1;
        w = 1;
    }

    Vector4f vpos[] = {Vector4f(lhx, lhy, z, w), 
                       Vector4f(lhx, lhy + screenHeight, z, w), 
                       Vector4f(lhx+screenWidth, lhy, z, w), 
                       Vector4f(lhx+screenWidth, lhy + screenHeight, z, w) };    

    Vector2f tpos[] = {Vector2f(0, 1), Vector2f(0, 0), 
                           Vector2f(1, 1), Vector2f(1, 0) };    

    if (IVertexDeclaration* vertexDeclaration =
        Ctr::VertexDeclarationMgr::vertexDeclarationMgr()->createVertexDeclaration
        (&VertexDeclarationParameters(vertexElements)))
    {
        setVertexDeclaration (vertexDeclaration);
        _positionStream = new Ctr::VertexStream
            (Ctr::POSITION, 0, 4, 4, (float*)vpos);
        _texCoordStream = new Ctr::VertexStream
            (Ctr::TEXCOORD, 0, 2, 4, (float*)tpos);

        addStream (_positionStream);
        addStream (_texCoordStream);

        if (create())
        {
            return cache();
        }
    }
    else
    {
        safedelete (vertexDeclaration);
        return false;
    }

    return false;
}

ScreenOrientedQuad::~ScreenOrientedQuad()
{
}

bool
ScreenOrientedQuad::create()
{
    return StreamedMesh::create();
}

}