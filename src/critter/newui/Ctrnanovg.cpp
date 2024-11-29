/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#define NVG_ANTIALIAS 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nanovg.h"
#include <CtrIVertexDeclaration.h>
#include <CtrIShader.h>
#include <CtrGpuVariable.h>
#include <CtrITexture.h>
#include <CtrVertexDeclarationMgr.h>
#include <CtrIIndexBuffer.h>
#include <CtrIVertexBuffer.h>
#include <CtrShaderMgr.h>
#include <CtrUIRenderer.h>

namespace
{
    static Ctr::IDevice* s_Device;
    static Ctr::IVertexDeclaration* s_nvgDecl;

    enum GLNVGshaderType
    {
        NSVG_SHADER_FILLGRAD,
        NSVG_SHADER_FILLIMG,
        NSVG_SHADER_SIMPLE,
        NSVG_SHADER_IMG
    };

    // These are additional flags on top of NVGimageFlags.
    enum NVGimageFlagsGL 
    {
        NVG_IMAGE_NODELETE = 1<<16, // Do not delete GL texture handle.
    };

    struct GLNVGtexture
    {
        Ctr::ITexture* texture;
        int id;
        int width, height;
        int type;
        int flags;
    };

    enum GLNVGcallType
    {
        GLNVG_FILL,
        GLNVG_CONVEXFILL,
        GLNVG_STROKE,
        GLNVG_TRIANGLES,
    };

    struct GLNVGcall
    {
        int type;
        int image;
        int pathOffset;
        int pathCount;
        int vertexOffset;
        int vertexCount;
        int uniformOffset;
    };

    struct GLNVGpath
    {
        int fillOffset;
        int fillCount;
        int strokeOffset;
        int strokeCount;
    };

    struct GLNVGfragUniforms
    {
        float scissorMat[16]; // matrices are actually 3 vec4s
        float paintMat[16];
        NVGcolor innerCol;
        NVGcolor outerCol;

        // u_scissorExtScale
        float scissorExt[2];
        float scissorScale[2];

        // u_extentRadius
        float extent[2];
        float radius;

        // u_params
        float feather;
        float strokeMult;
        float texType;
        float type;
    };

    struct GLNVGcontext
    {
        const Ctr::IShader* prog;
        Ctr::ITexture* s_tex;

        uint64_t state;
        int      th;

        Ctr::IVertexBuffer* tvb;
        uint8_t viewid;

        struct GLNVGtexture* textures;
        float view[2];
        int ntextures;
        int ctextures;
        int textureId;
        int vertBuf;
        int fragSize;
        int edgeAntiAlias;

        // Per frame buffers
        struct GLNVGcall* calls;
        int ccalls;
        int ncalls;
        struct GLNVGpath* paths;
        int cpaths;
        int npaths;
        struct NVGvertex* verts;
        int cverts;
        int nverts;
        unsigned char* uniforms;
        int cuniforms;
        int nuniforms;
    };

    static struct GLNVGtexture* glnvg__allocTexture(struct GLNVGcontext* gl)
    {
        struct GLNVGtexture* tex = NULL;
        int i;

        for (i = 0; i < gl->ntextures; i++)
        {
            if (gl->textures[i].texture == nullptr)
            {
                tex = &gl->textures[i];
                break;
            }
        }

        if (tex == NULL)
        {
            if (gl->ntextures+1 > gl->ctextures)
            {
                int old = gl->ctextures;
                gl->ctextures = (gl->ctextures == 0) ? 2 : gl->ctextures*2;
                gl->textures = (struct GLNVGtexture*)realloc(gl->textures, sizeof(struct GLNVGtexture)*gl->ctextures);
                memset(&gl->textures[old], 0xff, (gl->ctextures-old)*sizeof(struct GLNVGtexture) );

                if (gl->textures == NULL)
                {
                    return NULL;
                }
            }
            tex = &gl->textures[gl->ntextures++];
        }

        memset(tex, 0, sizeof(*tex));

        return tex;
    }

    static struct GLNVGtexture* glnvg__findTexture(struct GLNVGcontext* gl, int id)
    {
        int i;
        for (i = 0; i < gl->ntextures; i++)
        {
            if (gl->textures[i].id == id)
                return &gl->textures[id];
        }

        return nullptr;
    }

    static int glnvg__deleteTexture(struct GLNVGcontext* gl, int id)
    {
        for (int ii = 0; ii < gl->ntextures; ii++)
        {
            if (gl->textures[id].id == id)
            {
                safedelete(gl->textures[id].texture);
                memset(&gl->textures[ii], 0, sizeof(gl->textures[ii]));
                return 1;
            }
        }

        return 0;
    }

    static int nvgRenderCreate(void* _userPtr)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;

        if (Ctr::ShaderMgr* shaderMgr = s_Device->shaderMgr())
        {
            if (!shaderMgr->addShader("nanovg_fill.fx", gl->prog, true, false))
            {
                return 0;
            }
        }

        std::vector<Ctr::VertexElement> vertexElements;
        vertexElements.push_back(Ctr::VertexElement(0, 0, Ctr::FLOAT2, Ctr::METHOD_DEFAULT, Ctr::POSITION, 0));
        vertexElements.push_back(Ctr::VertexElement(0, 8, Ctr::FLOAT2, Ctr::METHOD_DEFAULT, Ctr::TEXCOORD, 0));
        vertexElements.push_back(Ctr::VertexElement(0xFF, 0, Ctr::UNUSED, 0, 0, 0));

        Ctr::VertexDeclarationParameters resource = Ctr::VertexDeclarationParameters(vertexElements);
        if (Ctr::IVertexDeclaration* vertexDeclaration =
            Ctr::VertexDeclarationMgr::vertexDeclarationMgr()->createVertexDeclaration(&resource))
        {
            s_nvgDecl = vertexDeclaration;
        }

        int align = 16;
        gl->fragSize = sizeof(struct GLNVGfragUniforms) + align - sizeof(struct GLNVGfragUniforms) % align;

        // TODO, build UIRenderer.

        return 1;
    }

    static int nvgRenderCreateTexture(void* _userPtr, int _type, int _width, int _height, int _flags, const unsigned char* _rgba)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;
        struct GLNVGtexture* tex = glnvg__allocTexture(gl);

        if (tex == nullptr)
        {
            return 0;
        }

        tex->width  = _width;
        tex->height = _height;
        tex->type   = _type;
        tex->flags  = _flags;

        uint32_t bytesPerPixel = NVG_TEXTURE_RGBA == tex->type ? 4 : 1;
        uint32_t pitch = tex->width * bytesPerPixel;


        static int textureId = 1;

        Ctr::TextureParameters textureData =
            Ctr::TextureParameters("nanovg texture",
            Ctr::TextureImagePtr(),
            Ctr::TwoD,
            Ctr::Procedural,
            NVG_TEXTURE_RGBA == _type ? Ctr::PF_BYTE_RGBA : Ctr::PF_L8,
            Ctr::Vector3i(tex->width, tex->height, 1));
        tex->texture = s_Device->createTexture(&textureData);
        if (_rgba != nullptr)
            tex->texture->write((uint8_t*)_rgba);
        tex->id = textureId;
        textureId++;

        return tex->id;
    }

    static int nvgRenderDeleteTexture(void* _userPtr, int image)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;
        return glnvg__deleteTexture(gl, image);
    }

    static int nvgRenderUpdateTexture(void* _userPtr, int image, int x, int y, int w, int h, const unsigned char* data)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;
        struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
        if (tex == NULL)
        {
            return 0;
        }

        uint32_t bytesPerPixel = NVG_TEXTURE_RGBA == tex->type ? 4 : 1;
        uint32_t pitch = tex->width * bytesPerPixel;

        tex->texture->writeSubRegion(data, x, y, w, h, bytesPerPixel);

        return 1;
    }

    static int nvgRenderGetTextureSize(void* _userPtr, int image, int* w, int* h)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;
        struct GLNVGtexture* tex = glnvg__findTexture(gl, image);

        if (tex->id == 0)
        {
            return 0;
        }

        *w = tex->width;
        *h = tex->height;

        return 1;
    }

    static void glnvg__xformIdentity(float* t)
    {
        t[0] = 1.0f; t[1] = 0.0f;
        t[2] = 0.0f; t[3] = 1.0f;
        t[4] = 0.0f; t[5] = 0.0f;
    }

    static void glnvg__xformInverse(float* inv, float* t)
    {
        double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
        if (det > -1e-6 && det < 1e-6) {
            glnvg__xformIdentity(t);
            return;
        }
        invdet = 1.0 / det;
        inv[0] = (float)(t[3] * invdet);
        inv[2] = (float)(-t[2] * invdet);
        inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
        inv[1] = (float)(-t[1] * invdet);
        inv[3] = (float)(t[0] * invdet);
        inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
    }

    static void glnvg__xformToMat3x4(float* m3, float* t)
    {
        m3[0] = t[0];
        m3[1] = t[1];
        m3[2] = 0.0f;
        m3[3] = 0.0f;
        m3[4] = t[2];
        m3[5] = t[3];
        m3[6] = 0.0f;
        m3[7] = 0.0f;
        m3[8] = t[4];
        m3[9] = t[5];
        m3[10] = 1.0f;
        m3[11] = 0.0f;
    }

    static void nvg_copyMatrix3to4(float* pDest, const float* pSource)
    {
        unsigned int i;
        for (i = 0; i < 3; i++)
        {
            memcpy(&pDest[i * 4], &pSource[i * 3], sizeof(float) * 3);
        }
    }

    static struct NVGcolor glnvg__premulColor(struct NVGcolor c)
    {
        c.r *= c.a;
        c.g *= c.a;
        c.b *= c.a;
        return c;
    }

    static void glnvg__xformToMat3x3(float* m3, float* t)
    {
        m3[0] = t[0];
        m3[1] = t[1];
        m3[2] = 0.0f;
        m3[3] = t[2];
        m3[4] = t[3];
        m3[5] = 0.0f;
        m3[6] = t[4];
        m3[7] = t[5];
        m3[8] = 1.0f;
    }



    static int glnvg__convertPaint(struct GLNVGcontext* gl, struct GLNVGfragUniforms* frag, struct NVGpaint* paint,
                                   struct NVGscissor* scissor, float width, float fringe)
    {
        struct GLNVGtexture* tex = NULL;
        float invxform[6], paintMat[9], scissorMat[9];

        memset(frag, 0, sizeof(*frag));

        //frag->innerCol = glnvg__premulColor(paint->innerColor);
        //frag->outerCol = glnvg__premulColor(paint->outerColor);
        frag->innerCol = (paint->innerColor);
        frag->outerCol = (paint->outerColor);


        nvgTransformInverse(invxform, paint->xform);
        glnvg__xformToMat3x3(paintMat, invxform);
        nvg_copyMatrix3to4(frag->paintMat, paintMat);

        if (scissor->extent[0] < 0.5f || scissor->extent[1] < 0.5f)
        {
            memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
            frag->scissorExt[0] = 1.0f;
            frag->scissorExt[1] = 1.0f;
            frag->scissorScale[0] = 1.0f;
            frag->scissorScale[1] = 1.0f;
        }
        else
        {
            nvgTransformInverse(invxform, scissor->xform);
            glnvg__xformToMat3x3(scissorMat, invxform);
            frag->scissorExt[0] = scissor->extent[0];
            frag->scissorExt[1] = scissor->extent[1];
            frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
            frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
        }
        nvg_copyMatrix3to4(frag->scissorMat, scissorMat);

        memcpy(frag->extent, paint->extent, sizeof(frag->extent));
        frag->strokeMult = (width*0.5f + fringe*0.5f) / fringe;

        gl->th = 0;

        if (paint->image != 0)
        {
            tex = glnvg__findTexture(gl, paint->image);
            if (tex == NULL)
            {
                return 0;
            }
            frag->type = NSVG_SHADER_FILLIMG;
            frag->texType = tex->type == NVG_TEXTURE_RGBA ? 0.0f : 1.0f;
            gl->th = tex->id;
        }
        else
        {
            frag->type = NSVG_SHADER_FILLGRAD;
            frag->radius = paint->radius;
            frag->feather = paint->feather;
        }

        return 1;
    }

    static void glnvg__mat3(float* dst, float* src)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        dst[3] = src[4];
        dst[4] = src[5];
        dst[5] = src[6];

        dst[6] = src[8];
        dst[7] = src[9];
        dst[8] = src[10];
    }

    static struct GLNVGfragUniforms* nvg__fragUniformPtr(struct GLNVGcontext* gl, int i)
    {
        return (struct GLNVGfragUniforms*)&gl->uniforms[i];
    }

    static void nvgRenderSetUniforms(struct GLNVGcontext* gl, int uniformOffset, int image)
    {
        const Ctr::GpuVariable*      scissorMatVariable = nullptr;
        const Ctr::GpuVariable*      paintMatVariable = nullptr;
        const Ctr::GpuVariable*      innerColVariable= nullptr;
        const Ctr::GpuVariable*      outerColVariable = nullptr;
        const Ctr::GpuVariable*      scissorExtScaleVariable = nullptr;
        const Ctr::GpuVariable*      extentRadiusVariable = nullptr;
        const Ctr::GpuVariable*      paramsVariable = nullptr;
        const Ctr::GpuVariable*      texVariable = nullptr;

        //gl->prog->getTechniqueByName(std::string("basic"), importanceSamplingSpecularTechnique);
        gl->prog->getParameterByName("u_scissorMat", scissorMatVariable);
        gl->prog->getParameterByName("u_paintMat", paintMatVariable);
        gl->prog->getParameterByName("u_innerCol", innerColVariable);
        gl->prog->getParameterByName("u_outerCol", outerColVariable);
        gl->prog->getParameterByName("u_scissorExtScale", scissorExtScaleVariable);
        gl->prog->getParameterByName("u_extentRadius", extentRadiusVariable);
        gl->prog->getParameterByName("u_params", paramsVariable);
        gl->prog->getParameterByName("s_tex", texVariable);

        struct GLNVGfragUniforms* frag = nvg__fragUniformPtr(gl, uniformOffset);

        if (scissorMatVariable)
            scissorMatVariable->setMatrix(&frag->scissorMat[0]);
        if (paintMatVariable)
            paintMatVariable->setMatrix(&frag->paintMat[0]);

        if (innerColVariable)
            innerColVariable->setVector(&frag->innerCol.rgba[0]);
        if (outerColVariable)
            outerColVariable->setVector(&frag->outerCol.rgba[0]);

        Ctr::Vector4f scissorExt(frag->scissorExt[0], frag->scissorExt[1], frag->scissorScale[0], frag->scissorScale[1]);
        if (scissorExtScaleVariable)
            scissorExtScaleVariable->setVector(&scissorExt.x);


        Ctr::Vector4f extentRadius = Ctr::Vector4f(frag->extent[0], frag->extent[1], frag->radius, 0);
        if (extentRadiusVariable)
            extentRadiusVariable->setVector(&extentRadius.x);

        // u_param

        Ctr::Vector4f params = Ctr::Vector4f(frag->feather, frag->strokeMult, frag->texType, frag->type);
        if (paramsVariable)
            paramsVariable->setVector(&params.x);

        Ctr::ITexture* handle = nullptr;
        gl->th = 0;
        if (image != 0)
        {
            struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
            if (tex != NULL)
            {
                handle = tex->texture;
                gl->th = tex->id;
                if (texVariable)
                    texVariable->setTexture(handle);
            }
        }
    }

    static void nvgRenderViewport(void* _userPtr, int width, int height)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;
        gl->view[0] = (float)width;
        gl->view[1] = (float)height;
        Ctr::Viewport viewport = Ctr::Viewport(0, 0, width, height, 0, 1);
        s_Device->setViewport(&viewport);
    }


    static void fan(uint32_t _start, uint32_t _count)
    {
        uint32_t numTris = _count-2;
        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
        uint32_t* data = (uint32_t*)uiRenderer->indexBuffer()->lock(numTris * 3 * sizeof(uint32_t));
        for (uint32_t ii = 0; ii < numTris; ++ii)
        {
            data[ii*3+0] = _start;
            data[ii*3+1] = _start + ii + 1;
            data[ii*3+2] = _start + ii + 2;
        }
        uiRenderer->indexBuffer()->unlock();
    }

    void setupNanoVgBlending()
    {
        s_Device->setBlendProperty(Ctr::OpAdd);
        s_Device->setSrcFunction(Ctr::SourceAlpha);
        s_Device->setAlphaSrcFunction(Ctr::SourceAlpha);
        s_Device->setDestFunction(Ctr::InverseSourceAlpha);
        s_Device->setAlphaDestFunction(Ctr::InverseSourceAlpha);
        s_Device->setAlphaBlendProperty(Ctr::OpAdd);

    }

    static void glnvg__fill(struct GLNVGcontext* gl, struct GLNVGcall* call)
    {
        struct GLNVGpath* paths = &gl->paths[call->pathOffset];
        int i, npaths = call->pathCount;

        nvgRenderSetUniforms(gl, call->uniformOffset, 0);
        s_Device->setCullMode(Ctr::CullNone);
        s_Device->enableStencilTest();
        s_Device->disableAlphaBlending();
        s_Device->setColorWriteState(false, false, false, false);
        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();

        for (i = 0; i < npaths; i++)
        {
            if (2 < paths[i].fillCount)
            {
                uiRenderer->setDrawIndexed(true);
                uiRenderer->setPrimitiveType(Ctr::TriangleList);
                uiRenderer->setShader(gl->prog);
                s_Device->setupStencil(0xff, 0xff, 
                                       Ctr::Always, Ctr::StencilKeep, Ctr::StencilIncrement, Ctr::StencilKeep, 
                                       Ctr::Always, Ctr::StencilKeep, Ctr::StencilDecrement, Ctr::StencilKeep);

                uiRenderer->setVertexBuffer(gl->tvb);
                //bgfx::setTexture(0, gl->s_tex, gl->th);
                fan(paths[i].fillOffset, paths[i].fillCount);

                uiRenderer->render((paths[i].fillCount-2)*3, 0);
                uiRenderer->setDrawIndexed(false);
            }
        }

        // Draw aliased off-pixels
        nvgRenderSetUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
        s_Device->enableAlphaBlending();
        s_Device->setColorWriteState(true, true, true, true);

        s_Device->setCullMode(Ctr::CW);
        s_Device->setupStencil(0xff, 0xff, Ctr::Equal, Ctr::StencilKeep, Ctr::StencilKeep, Ctr::StencilKeep);
        uiRenderer->setPrimitiveType(Ctr::TriangleStrip);
        if (gl->edgeAntiAlias)
        {
            for (i = 0; i < npaths; i++)
            {
                uiRenderer->setShader(gl->prog);

                uiRenderer->setVertexBuffer(gl->tvb);
                uiRenderer->render(paths[i].strokeCount, paths[i].strokeOffset);
            }
        }

        s_Device->setCullMode(Ctr::CullNone);
        uiRenderer->setPrimitiveType(Ctr::TriangleList);
        // Draw fill

        s_Device->setupStencil(0xff, 0xff, Ctr::NotEqual, Ctr::StencilZero, Ctr::StencilZero, Ctr::StencilZero);

        uiRenderer->setShader(gl->prog);        
        uiRenderer->setVertexBuffer(gl->tvb);
        

        uiRenderer->render(call->vertexCount, call->vertexOffset);

        s_Device->disableStencilTest();
        // New
    }

    static void glnvg__convexFill(struct GLNVGcontext* gl, struct GLNVGcall* call)
    {
        
        struct GLNVGpath* paths = &gl->paths[call->pathOffset];
        int i, npaths = call->pathCount;
        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();

        nvgRenderSetUniforms(gl, call->uniformOffset, call->image);
        uiRenderer->setPrimitiveType(Ctr::TriangleList);

        for (i = 0; i < npaths; i++)
        {
            if (paths[i].fillCount == 0) continue;
            uiRenderer->setShader(gl->prog);
            //bgfx::setState(gl->state);
            uiRenderer->setVertexBuffer(gl->tvb);
            //bgfx::setTexture(0, gl->s_tex, gl->th);
            uiRenderer->setDrawIndexed(true);

            fan(paths[i].fillOffset, paths[i].fillCount);

            // Index count. Offset is baked into the indices.
            uiRenderer->render((paths[i].fillCount - 2) * 3, 0);
            uiRenderer->setDrawIndexed(false);

        }

        uiRenderer->setPrimitiveType(Ctr::TriangleStrip);
        if (gl->edgeAntiAlias)
        {
            // Draw fringes
            for (i = 0; i < npaths; i++)
            {
                uiRenderer->setShader(gl->prog);
                //bgfx::setTexture(0, gl->s_tex, gl->th);
                // Vertex count and vertex offset.
                uiRenderer->render(paths[i].strokeCount, paths[i].strokeOffset);
            }
        }
        // New primitive
        uiRenderer->setPrimitiveType(Ctr::TriangleList);
    }

    static void glnvg__stroke(struct GLNVGcontext* gl, struct GLNVGcall* call)
    {
        // TODO: State setup.
        struct GLNVGpath* paths = &gl->paths[call->pathOffset];
        int npaths = call->pathCount, i;
        nvgRenderSetUniforms(gl, call->uniformOffset, call->image);

        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
        // Draw Strokes
        uiRenderer->setPrimitiveType(Ctr::TriangleStrip);
        for (i = 0; i < npaths; i++)
        {
            uiRenderer->setShader(gl->prog);
            uiRenderer->setVertexBuffer(gl->tvb);
            uiRenderer->render(paths[i].strokeCount, paths[i].strokeOffset);
        }
        // New primitive
        uiRenderer->setPrimitiveType(Ctr::TriangleList);
        
    }

    static void glnvg__triangles(struct GLNVGcontext* gl, struct GLNVGcall* call)
    {

        // TODO: State setup.
        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
        uiRenderer->setPrimitiveType(Ctr::TriangleList);
        if (3 <= call->vertexCount)
        {
            nvgRenderSetUniforms(gl, call->uniformOffset, call->image);

            uiRenderer->setShader(gl->prog);
            // And other mixed state. Uggh.
            //uiRenderer->setState(gl->state);
            uiRenderer->render(call->vertexCount, call->vertexOffset);
        }
    }

    static void nvgRenderFlush(void* _userPtr)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;
        if (gl->ncalls > 0)
        {
            Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
            gl->tvb = uiRenderer->vertexBuffer(s_nvgDecl);
            size_t vertexBufferSize = gl->nverts * sizeof(struct NVGvertex);
            void* vertexBufferData = gl->tvb->lock(vertexBufferSize);
            memcpy(vertexBufferData, gl->verts, gl->nverts * sizeof(struct NVGvertex) );
            gl->tvb->unlock();

            Ctr::IDevice* device = uiRenderer->device();
            device->enableAlphaBlending();
            device->setZFunction(Ctr::LessEqual);
            device->disableZTest();
            device->disableDepthWrite();
            setupNanoVgBlending();

            const Ctr::GpuVariable*      viewSizeVariable = nullptr;
            gl->prog->getParameterByName("u_viewSize", viewSizeVariable);
            if (viewSizeVariable)
                viewSizeVariable->set(gl->view, sizeof(float)*2);

            for (uint32_t ii = 0, num = gl->ncalls; ii < num; ++ii)
            {
                struct GLNVGcall* call = &gl->calls[ii];
                switch (call->type)
                {
                case GLNVG_FILL:
                    glnvg__fill(gl, call);
                    break;

                case GLNVG_CONVEXFILL:
                    glnvg__convexFill(gl, call);
                    break;

                case GLNVG_STROKE:
                    glnvg__stroke(gl, call);
                    break;

                case GLNVG_TRIANGLES:
                    glnvg__triangles(gl, call);
                    break;
                }
            }
            device->disableAlphaBlending();
            device->setupBlendPipeline(Ctr::BlendAlpha);
            device->enableDepthWrite();
            device->enableZTest();
        }



        // Reset calls
        gl->nverts    = 0;
        gl->npaths    = 0;
        gl->ncalls    = 0;
        gl->nuniforms = 0;
    }

    static int glnvg__maxVertCount(const struct NVGpath* paths, int npaths)
    {
        int i, count = 0;
        for (i = 0; i < npaths; i++)
        {
            count += paths[i].nfill;
            count += paths[i].nstroke;
        }
        return count;
    }

    static int glnvg__maxi(int a, int b) { return a > b ? a : b; }

    static struct GLNVGcall* glnvg__allocCall(struct GLNVGcontext* gl)
    {
        struct GLNVGcall* ret = NULL;
        if (gl->ncalls+1 > gl->ccalls)
        {
            gl->ccalls = gl->ccalls == 0 ? 32 : gl->ccalls * 2;
            gl->calls = (struct GLNVGcall*)realloc(gl->calls, sizeof(struct GLNVGcall) * gl->ccalls);
        }
        ret = &gl->calls[gl->ncalls++];
        memset(ret, 0, sizeof(struct GLNVGcall));
        return ret;
    }

    static int glnvg__allocPaths(struct GLNVGcontext* gl, int n)
    {
        int ret = 0;
        if (gl->npaths + n > gl->cpaths) {
            GLNVGpath* paths;
            int cpaths = glnvg__maxi(gl->npaths + n, 128) + gl->cpaths / 2; // 1.5x Overallocate
            paths = (GLNVGpath*)realloc(gl->paths, sizeof(GLNVGpath) * cpaths);
            if (paths == NULL) return -1;
            gl->paths = paths;
            gl->cpaths = cpaths;
        }
        ret = gl->npaths;
        gl->npaths += n;
        return ret;
    }

    static int glnvg__allocVerts(GLNVGcontext* gl, int n)
    {
        int ret = 0;
        if (gl->nverts+n > gl->cverts)
        {
            NVGvertex* verts;
            int cverts = glnvg__maxi(gl->nverts + n, 4096) + gl->cverts/2; // 1.5x Overallocate
            verts = (NVGvertex*)realloc(gl->verts, sizeof(NVGvertex) * cverts);
            if (verts == NULL) return -1;
            gl->verts = verts;
            gl->cverts = cverts;
        }
        ret = gl->nverts;
        gl->nverts += n;
        return ret;
    }

    static int glnvg__allocFragUniforms(struct GLNVGcontext* gl, int n)
    {
        int ret = 0, structSize = gl->fragSize;
        if (gl->nuniforms+n > gl->cuniforms)
        {
            gl->cuniforms = gl->cuniforms == 0 ? glnvg__maxi(n, 32) : gl->cuniforms * 2;
            gl->uniforms = (unsigned char*)realloc(gl->uniforms, gl->cuniforms * structSize);
        }
        ret = gl->nuniforms * structSize;
        gl->nuniforms += n;
        return ret;
    }

    static void glnvg__vset(struct NVGvertex* vtx, float x, float y, float u, float v)
    {
        vtx->x = x;
        vtx->y = y;
        vtx->u = u;
        vtx->v = v;
    }

    static void nvgRenderFill(void* _userPtr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
                                  const float* bounds, const struct NVGpath* paths, int npaths)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;

        struct GLNVGcall* call = glnvg__allocCall(gl);
        struct NVGvertex* quad;
        struct GLNVGfragUniforms* frag;
        int i, maxverts, offset;

        call->type = GLNVG_FILL;
        call->pathOffset = glnvg__allocPaths(gl, npaths);
        call->pathCount = npaths;
        call->image = paint->image;

        if (npaths == 1 && paths[0].convex)
        {
            call->type = GLNVG_CONVEXFILL;
        }

        // Allocate vertices for all the paths.
        maxverts = glnvg__maxVertCount(paths, npaths) + 6;
        offset = glnvg__allocVerts(gl, maxverts);

        for (i = 0; i < npaths; i++)
        {
            struct GLNVGpath* copy = &gl->paths[call->pathOffset + i];
            const struct NVGpath* path = &paths[i];
            memset(copy, 0, sizeof(struct GLNVGpath));
            if (path->nfill > 0)
            {
                copy->fillOffset = offset;
                copy->fillCount = path->nfill;
                memcpy(&gl->verts[offset], path->fill, sizeof(struct NVGvertex) * path->nfill);
                offset += path->nfill;
            }

            if (path->nstroke > 0)
            {
                copy->strokeOffset = offset;
                copy->strokeCount = path->nstroke;
                memcpy(&gl->verts[offset], path->stroke, sizeof(struct NVGvertex) * path->nstroke);
                offset += path->nstroke;
            }
        }

        // Quad
        call->vertexOffset = offset;
        call->vertexCount = 6;
        quad = &gl->verts[call->vertexOffset];
        glnvg__vset(&quad[0], bounds[0], bounds[3], 0.5f, 1.0f);
        glnvg__vset(&quad[1], bounds[2], bounds[3], 0.5f, 1.0f);
        glnvg__vset(&quad[2], bounds[2], bounds[1], 0.5f, 1.0f);

        glnvg__vset(&quad[3], bounds[0], bounds[3], 0.5f, 1.0f);
        glnvg__vset(&quad[4], bounds[2], bounds[1], 0.5f, 1.0f);
        glnvg__vset(&quad[5], bounds[0], bounds[1], 0.5f, 1.0f);

        // Setup uniforms for draw calls
        if (call->type == GLNVG_FILL)
        {
            call->uniformOffset = glnvg__allocFragUniforms(gl, 2);
            // Simple shader for stencil
            frag = nvg__fragUniformPtr(gl, call->uniformOffset);
            memset(frag, 0, sizeof(*frag));
            frag->type = NSVG_SHADER_SIMPLE;
            // Fill shader
            glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, fringe, fringe);
        }
        else
        {
            call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
            // Fill shader
            glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint, scissor, fringe, fringe);
        }
    }

    static void nvgRenderStroke(void* _userPtr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
                                    float strokeWidth, const struct NVGpath* paths, int npaths)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;

        struct GLNVGcall* call = glnvg__allocCall(gl);
        int i, maxverts, offset;

        call->type = GLNVG_STROKE;
        call->pathOffset = glnvg__allocPaths(gl, npaths);
        call->pathCount = npaths;
        call->image = paint->image;

        // Allocate vertices for all the paths.
        maxverts = glnvg__maxVertCount(paths, npaths);
        offset = glnvg__allocVerts(gl, maxverts);

        for (i = 0; i < npaths; i++)
        {
            struct GLNVGpath* copy = &gl->paths[call->pathOffset + i];
            const struct NVGpath* path = &paths[i];
            memset(copy, 0, sizeof(struct GLNVGpath));
            if (path->nstroke)
            {
                copy->strokeOffset = offset;
                copy->strokeCount = path->nstroke;
                memcpy(&gl->verts[offset], path->stroke, sizeof(struct NVGvertex) * path->nstroke);
                offset += path->nstroke;
            }
        }

        // Fill shader
        call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
        glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint, scissor, strokeWidth, fringe);
    }

    static void nvgRenderTriangles(void* _userPtr, struct NVGpaint* paint, struct NVGscissor* scissor,
                                       const struct NVGvertex* verts, int nverts)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;
        struct GLNVGcall* call = glnvg__allocCall(gl);
        struct GLNVGfragUniforms* frag;

        call->type = GLNVG_TRIANGLES;
        call->image = paint->image;

        // Allocate vertices for all the paths.
        call->vertexOffset = glnvg__allocVerts(gl, nverts);
        call->vertexCount = nverts;
        memcpy(&gl->verts[call->vertexOffset], verts, sizeof(struct NVGvertex) * nverts);

        // Fill shader
        call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
        frag = nvg__fragUniformPtr(gl, call->uniformOffset);
        glnvg__convertPaint(gl, frag, paint, scissor, 1.0f, 1.0f);
        frag->type = NSVG_SHADER_IMG;
    }

    static void nvgRenderDelete(void* _userPtr)
    {
        struct GLNVGcontext* gl = (struct GLNVGcontext*)_userPtr;

        if (gl == NULL)
        {
            return;
        }

        for (uint32_t ii = 0, num = gl->ntextures; ii < num; ++ii)
        {
            if (gl->textures[ii].texture != nullptr)
            {
                safedelete(gl->textures[ii].texture);
                gl->textures[ii].id = 0;
            }
        }

        free(gl->textures);

        free(gl);
    }

} // namespace

NVGcontext* nvgCreate(int edgeaa, unsigned char viewid, void* device)
{
    s_Device = reinterpret_cast<Ctr::IDevice*>(device);

    struct NVGparams params;
    struct NVGcontext* ctx = NULL;
    struct GLNVGcontext* gl = (struct GLNVGcontext*)malloc(sizeof(struct GLNVGcontext));
    if (gl == NULL) goto error;
    
    
    memset(gl, 0, sizeof(struct GLNVGcontext));

    memset(&params, 0, sizeof(params));
    params.renderCreate         = nvgRenderCreate;
    params.renderCreateTexture  = nvgRenderCreateTexture;
    params.renderDeleteTexture  = nvgRenderDeleteTexture;
    params.renderUpdateTexture  = nvgRenderUpdateTexture;
    params.renderGetTextureSize = nvgRenderGetTextureSize;
    params.renderViewport       = nvgRenderViewport;
    params.renderFlush          = nvgRenderFlush;
    params.renderFill           = nvgRenderFill;
    params.renderStroke         = nvgRenderStroke;
    params.renderTriangles      = nvgRenderTriangles;
    params.renderDelete         = nvgRenderDelete;
    params.userPtr = gl;
    params.edgeAntiAlias = edgeaa;

    gl->edgeAntiAlias = edgeaa;
    gl->viewid = uint8_t(viewid);

    ctx = nvgCreateInternal(&params);
    if (ctx == NULL) goto error;

    return ctx;

error:
    // 'gl' is freed by nvgDeleteInternal.
    if (ctx != NULL)
    {
        nvgDeleteInternal(ctx);
    }

    return NULL;
}

void nvgDelete(struct NVGcontext* ctx)
{
    nvgDeleteInternal(ctx);
}