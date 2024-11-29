/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

// This code is based on:
//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
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
// Source altered and distributed from https://github.com/AdrienHerubel/imgui

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <CtrLog.h>
#include <CtrIDevice.h>
#include <CtrITexture.h>
#include <CtrIShader.h>
#include <CtrGpuVariable.h>
#include <CtrIVertexDeclaration.h>
#include <CtrIGpuBuffer.h>
#include <CtrGpuVariable.h>
#include <CtrShaderMgr.h>
#include <CtrTextureMgr.h>
#include <CtrVertexDeclarationMgr.h>
#include <CtrIVertexBuffer.h>
#include <CtrIIndexBuffer.h>
#include <CtrUIRenderer.h>
#include <CtrInputState.h>
#include <CtrTextureImage.h>
#include "Ctrimgui.h"
#include "ocornut_imgui.h"
#include "../nanovg/nanovg.h"


// embedded font
#include "droidsans.ttf.h"

#define USE_NANOVG_FONT 0
#define IMGUI_CONFIG_MAX_FONTS 20
#define MAX_TEMP_COORDS 100
#define NUM_CIRCLE_VERTS (8 * 4)

static const int32_t BUTTON_HEIGHT = 20;
static const int32_t SLIDER_HEIGHT = 20;
static const int32_t SLIDER_MARKER_WIDTH = 10;
static const int32_t CHECK_SIZE = 8;
static const int32_t DEFAULT_SPACING = 4;
static const int32_t TEXT_HEIGHT = 8;
static const int32_t SCROLL_AREA_PADDING = 6;
static const int32_t AREA_HEADER = 20;
static const float s_tabStops[4] = {150, 210, 270, 330};

static void* imguiMalloc(size_t size, void* /*_userptr*/)
{
    return malloc(size);
}

static void imguiFree(void* _ptr, void* /*_userptr*/)
{
    free(_ptr);
}

#define IMGUI_MIN(_a, _b) (_a)<(_b)?(_a):(_b)
#define IMGUI_MAX(_a, _b) (_a)>(_b)?(_a):(_b)
#define IMGUI_CLAMP(_a, _min, _max) IMGUI_MIN(IMGUI_MAX(_a, _min), _max)

#define STBTT_malloc(_x, _y) imguiMalloc(_x, _y)
#define STBTT_free(_x, _y) imguiFree(_x, _y)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace
{
    static uint32_t addQuad(uint32_t* _indices, uint16_t _idx0, uint16_t _idx1, uint16_t _idx2, uint16_t _idx3)
    {
        _indices[0] = _idx0;
        _indices[1] = _idx3;
        _indices[2] = _idx1;

        _indices[3] = _idx1;
        _indices[4] = _idx3;
        _indices[5] = _idx2;

        return 6;
    }

    float sign(float px, float py, float ax, float ay, float bx, float by)
    {
        return (px - bx) * (ay - by) - (ax - bx) * (py - by);
    }

    bool pointInTriangle(float px, float py, float ax, float ay, float bx, float by, float cx, float cy)
    {
        const bool b1 = sign(px, py, ax, ay, bx, by) < 0.0f;
        const bool b2 = sign(px, py, bx, by, cx, cy) < 0.0f;
        const bool b3 = sign(px, py, cx, cy, ax, ay) < 0.0f;

        return ( (b1 == b2) && (b2 == b3) );
    }

    void closestPointOnLine(float& ox, float &oy, float px, float py, float ax, float ay, float bx, float by)
    {
        float dx = px - ax;
        float dy = py - ay;

        float lx = bx - ax;
        float ly = by - ay;

        float len = sqrtf(lx*lx+ly*ly);

        // Normalize.
        float invLen = 1.0f/len;
        lx*=invLen;
        ly*=invLen;

        float dot = (dx*lx + dy*ly);

        if (dot < 0.0f)
        {
            ox = ax;
            oy = ay;
        }
        else if (dot > len)
        {
            ox = bx;
            oy = by;
        }
        else
        {
            ox = ax + lx*dot;
            oy = ay + ly*dot;
        }
    }

    void closestPointOnTriangle(float& ox, float &oy, float px, float py, float ax, float ay, float bx, float by, float cx, float cy)
    {
        float abx, aby;
        float bcx, bcy;
        float cax, cay;
        closestPointOnLine(abx, aby, px, py, ax, ay, bx, by);
        closestPointOnLine(bcx, bcy, px, py, bx, by, cx, cy);
        closestPointOnLine(cax, cay, px, py, cx, cy, ax, ay);

        const float pabx = px - abx;
        const float paby = py - aby;
        const float pbcx = px - bcx;
        const float pbcy = py - bcy;
        const float pcax = px - cax;
        const float pcay = py - cay;

        const float lab = sqrtf(pabx*pabx+paby*paby);
        const float lbc = sqrtf(pbcx*pbcx+pbcy*pbcy);
        const float lca = sqrtf(pcax*pcax+pcay*pcay);

        const float m = Ctr::minValue(lab, Ctr::minValue(lbc, lca));
        if (m == lab)
        {
            ox = abx;
            oy = aby;
        }
        else if (m == lbc)
        {
            ox = bcx;
            oy = bcy;
        }
        else// if (m == lca).
        {
            ox = cax;
            oy = cay;
        }
    }

    inline float vec2Dot(const float* __restrict _a, const float* __restrict _b)
    {
        return _a[0]*_b[0] + _a[1]*_b[1];
    }

    void barycentric(float& _u, float& _v, float& _w
        , float _ax, float _ay
        , float _bx, float _by
        , float _cx, float _cy
        , float _px, float _py
        )
    {
        const float v0[2] = { _bx - _ax, _by - _ay };
        const float v1[2] = { _cx - _ax, _cy - _ay };
        const float v2[2] = { _px - _ax, _py - _ay };
        const float d00 = vec2Dot(v0, v0);
        const float d01 = vec2Dot(v0, v1);
        const float d11 = vec2Dot(v1, v1);
        const float d20 = vec2Dot(v2, v0);
        const float d21 = vec2Dot(v2, v1);
        const float denom = d00 * d11 - d01 * d01;
        _v = (d11 * d20 - d01 * d21) / denom;
        _w = (d00 * d21 - d01 * d20) / denom;
        _u = 1.0f - _v - _w;
    }

    struct PosColorVertex
    {
        float m_x;
        float m_y;
        uint32_t m_abgr;


        static void init()
        {

            std::vector<Ctr::VertexElement> vertexElements;
            vertexElements.push_back(Ctr::VertexElement(0, 0, Ctr::FLOAT2, Ctr::METHOD_DEFAULT, Ctr::POSITION, 0));
            vertexElements.push_back(Ctr::VertexElement(0, 8, Ctr::UBYTE4, Ctr::METHOD_DEFAULT, Ctr::COLOR, 0));
            vertexElements.push_back(Ctr::VertexElement(0xFF, 0, Ctr::UNUSED, 0, 0, 0));

            Ctr::VertexDeclarationParameters resource = Ctr::VertexDeclarationParameters(vertexElements);
            if (Ctr::IVertexDeclaration* vertexDeclaration =
                Ctr::VertexDeclarationMgr::vertexDeclarationMgr()->createVertexDeclaration(&resource))
            {
                ms_decl = vertexDeclaration;
            }
            else
            {
                LOG("Failed to init PosColorVertex decl.")
                assert(0);
            }
        }

        static Ctr::IVertexDeclaration* ms_decl;
    };

    Ctr::IVertexDeclaration* PosColorVertex::ms_decl;

    struct PosColorUvVertex
    {
        float m_x;
        float m_y;
        float m_u;
        float m_v;
        uint32_t m_abgr;

        static void init()
        {
            std::vector<Ctr::VertexElement> vertexElements;
            vertexElements.push_back(Ctr::VertexElement(0, 0, Ctr::FLOAT2, Ctr::METHOD_DEFAULT, Ctr::POSITION, 0));
            vertexElements.push_back(Ctr::VertexElement(0, 8, Ctr::FLOAT2, Ctr::METHOD_DEFAULT, Ctr::TEXCOORD, 0));
            vertexElements.push_back(Ctr::VertexElement(0, 16, Ctr::UBYTE4, Ctr::METHOD_DEFAULT, Ctr::COLOR, 0));
            vertexElements.push_back(Ctr::VertexElement(0xFF, 0, Ctr::UNUSED, 0, 0, 0));

            Ctr::VertexDeclarationParameters resource = Ctr::VertexDeclarationParameters(vertexElements);
            if (Ctr::IVertexDeclaration* vertexDeclaration =
                Ctr::VertexDeclarationMgr::vertexDeclarationMgr()->createVertexDeclaration(&resource))
            {
                ms_decl = vertexDeclaration;
            }
            else
            {
                LOG("Failed to init PosCorVertex decl.")
                    assert(0);
            }
        }

        static Ctr::IVertexDeclaration* ms_decl;
    };

    Ctr::IVertexDeclaration* PosColorUvVertex::ms_decl;

    struct PosUvVertex
    {
        float m_x;
        float m_y;
        float m_u;
        float m_v;

        static void init()
        {

            std::vector<Ctr::VertexElement> vertexElements;
            vertexElements.push_back(Ctr::VertexElement(0, 0, Ctr::FLOAT2, Ctr::METHOD_DEFAULT, Ctr::POSITION, 0));
            vertexElements.push_back(Ctr::VertexElement(0, 8, Ctr::FLOAT2, Ctr::METHOD_DEFAULT, Ctr::TEXCOORD, 0));
            vertexElements.push_back(Ctr::VertexElement(0xFF, 0, Ctr::UNUSED, 0, 0, 0));

            Ctr::VertexDeclarationParameters resource = Ctr::VertexDeclarationParameters(vertexElements);
            if (Ctr::IVertexDeclaration* vertexDeclaration =
                Ctr::VertexDeclarationMgr::vertexDeclarationMgr()->createVertexDeclaration(&resource))
            {
                ms_decl = vertexDeclaration;
            }
            else
            {
                LOG("Failed to init PosCorVertex decl.")
                    assert(0);
            }
        }

        static Ctr::IVertexDeclaration* ms_decl;
    };

    Ctr::IVertexDeclaration* PosUvVertex::ms_decl;

    struct PosNormalVertex
    {
        float m_x;
        float m_y;
        float m_z;
        float m_nx;
        float m_ny;
        float m_nz;

        static void init()
        {


            std::vector<Ctr::VertexElement> vertexElements;
            vertexElements.push_back(Ctr::VertexElement(0, 0, Ctr::FLOAT3, Ctr::METHOD_DEFAULT, Ctr::POSITION, 0));
            vertexElements.push_back(Ctr::VertexElement(0, 12, Ctr::FLOAT3, Ctr::METHOD_DEFAULT, Ctr::NORMAL, 0));
            vertexElements.push_back(Ctr::VertexElement(0xFF, 0, Ctr::UNUSED, 0, 0, 0));

            Ctr::VertexDeclarationParameters resource = Ctr::VertexDeclarationParameters(vertexElements);
            if (Ctr::IVertexDeclaration* vertexDeclaration =
                Ctr::VertexDeclarationMgr::vertexDeclarationMgr()->createVertexDeclaration(&resource))
            {
                ms_decl = vertexDeclaration;
            }
            else
            {
                LOG("Failed to init PosCorVertex decl.")
                    assert(0);
            }
        }

        void set(float _x, float _y, float _z, float _nx, float _ny, float _nz)
        {
            m_x = _x;
            m_y = _y;
            m_z = _z;
            m_nx = _nx;
            m_ny = _ny;
            m_nz = _nz;
        }

        static Ctr::IVertexDeclaration* ms_decl;
    };

    Ctr::IVertexDeclaration* PosNormalVertex::ms_decl;

} // namespace

static float getTextLength(stbtt_bakedchar* _chardata, const char* _text, uint32_t& _numVertices)
{
    float xpos = 0;
    float len = 0;
    uint32_t numVertices = 0;

    while (*_text)
    {
        int32_t ch = (uint8_t)*_text;
        if (ch == '\t')
        {
            for (int32_t ii = 0; ii < 4; ++ii)
            {
                if (xpos < s_tabStops[ii])
                {
                    xpos = s_tabStops[ii];
                    break;
                }
            }
        }
        else if (ch >= ' '
             &&  ch < 128)
        {
            stbtt_bakedchar* b = _chardata + ch - ' ';
            int32_t round_x = STBTT_ifloor( (xpos + b->xoff) + 0.5);
            len = round_x + b->x1 - b->x0 + 0.5f;
            xpos += b->xadvance;
            numVertices += 6;
        }

        ++_text;
    }

    _numVertices = numVertices;

    return len;
}

struct Imgui
{
    Imgui()
        : m_mx(-1)
        , m_my(-1)
        , m_scroll(0)
        , m_active(0)
        , m_hot(0)
        , m_hotToBe(0)
        , m_dragX(0)
        , m_dragY(0)
        , m_dragOrig(0)
        , m_left(false)
        , m_leftPressed(false)
        , m_leftReleased(false)
        , m_isHot(false)
        , m_wentActive(false)
        , m_insideArea(false)
        , m_isActivePresent(false)
        , m_checkActivePresence(false)
        , m_widgetId(0)
        , m_enabledAreaIds(0)
        , m_textureWidth(512)
        , m_textureHeight(512)
        , m_halfTexel(0.0f)
        , m_nvg(NULL)
        , m_view(31)
        , m_viewWidth(0)
        , m_viewHeight(0)
        , m_currentFontIdx(0)
        , m_Device(nullptr)
    {

        memset(m_KeyRepeatTimes, 0, sizeof(float)*512);

        m_areaId.reset();

        m_invTextureWidth  = 1.0f/m_textureWidth;
        m_invTextureHeight = 1.0f/m_textureHeight;

        u_imageLodEnabled = nullptr;
        u_imageSwizzle = nullptr;
        s_texColor = nullptr;
        m_missingTexture = nullptr;

        m_colorProgram = nullptr;
        m_textureProgram = nullptr;
        m_cubeMapProgram = nullptr;
        m_imageProgram = nullptr;
        m_imageSwizzProgram = nullptr;
    }

    ImguiFontReference createFont(const void* _data, float _fontSize)
    {
        static uint32_t id = 0;
        uint8_t* mem = (uint8_t*)malloc(m_textureWidth * m_textureHeight);
        stbtt_BakeFontBitmap( (uint8_t*)_data, 0, _fontSize, mem, m_textureWidth, m_textureHeight, 32, 96, m_fonts[id].m_cdata);
                
        Ctr::TextureParameters textureData =
            Ctr::TextureParameters("Normal Offsets Texture",
            Ctr::TextureImagePtr(),
            Ctr::TwoD,
            Ctr::Procedural,
            Ctr::PF_L8,
            Ctr::Vector3i(m_textureWidth, m_textureHeight, 1));
        m_fonts[id].m_texture = m_Device->createTexture(&textureData);

        m_fonts[id].m_texture->write(mem);

        m_fonts[id].m_size = _fontSize;
        m_fonts[id].m_id = ++id;
        return m_fonts[id].m_id;
    }

    void setFont(ImguiFontReference _handle)
    {
        {
            m_currentFontIdx = _handle;
        }
    }

    Ctr::ITexture* genMissingTexture(uint32_t _width, uint32_t _height, float _lineWidth = 0.02f)
    {
        uint8_t* bgra8Ptr = (uint8_t*)malloc(_width*_height * 4);
        uint32_t* bgra8 = (uint32_t*)bgra8Ptr;
        const float sx = 0.70710677f;
        const float cx = 0.70710677f;

        for (uint32_t yy = 0; yy < _height; ++yy)
        {
            for (uint32_t xx = 0; xx < _width; ++xx)
            {
                float px = xx / float(_width)  * 2.0f - 1.0f;
                float py = yy / float(_height) * 2.0f - 1.0f;

                float sum = Ctr::pulse(px * cx - py * sx, _lineWidth, -_lineWidth)
                          + Ctr::pulse(px * sx + py * cx, _lineWidth, -_lineWidth)
                          ;
                *bgra8++ = sum >= 1.0f ? 0xffff0000 : 0xffffffff;
            }
        }

        Ctr::TextureParameters textureData =
            Ctr::TextureParameters("Normal Offsets Texture",
            Ctr::TextureImagePtr(),
            Ctr::TwoD,
            Ctr::Procedural,
            Ctr::PF_BYTE_RGBA,
            Ctr::Vector3i(_width, _height, 1));
        Ctr::ITexture* texture = m_Device->createTexture(&textureData);
        texture->write(bgra8Ptr);

        return texture;
    }

    ImguiFontReference create(Ctr::IDevice* _device, const void* _data, uint32_t _size, float _fontSize)
    {
        m_Device = _device;

        if (NULL == _data)
        {
            _data = s_droidSansTtf;
            _size = sizeof(s_droidSansTtf);
        }

        Ctr::UIRenderer::create(m_Device);
        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
        IMGUI_create(_data, _size, _fontSize);

        m_nvg = nvgCreate(1, m_view, m_Device);
        nvgCreateFontMem(m_nvg, "default", (unsigned char*)_data, INT32_MAX, 0);
        nvgFontSize(m_nvg, _fontSize);
        nvgFontFace(m_nvg, "default");


        for (int32_t ii = 0; ii < NUM_CIRCLE_VERTS; ++ii)
        {
            float a = (float)ii / (float)NUM_CIRCLE_VERTS * (float)(3.141591 * 2.0);
            m_circleVerts[ii * 2 + 0] = cosf(a);
            m_circleVerts[ii * 2 + 1] = sinf(a);
        }

        PosColorVertex::init();
        PosColorUvVertex::init();
        PosUvVertex::init();
        PosNormalVertex::init();

        if (Ctr::ShaderMgr* shaderMgr = m_Device->shaderMgr())
        { 
            if (!shaderMgr->addShader("imgui_color.fx", m_colorProgram, true, false))
            {
                LOG("Failed to load imgui color shader");
                assert(0);
            }
            if (!shaderMgr->addShader("imgui_texture.fx", m_textureProgram, true, false))
            {
                LOG("Failed to load texture shader");
                assert(0);
            }
            if (!shaderMgr->addShader("imgui_cubemap.fx", m_cubeMapProgram, true, false))
            {
                LOG("Failed to load imgui cubemap shader");
                assert(0);
            }
            if (!shaderMgr->addShader("imgui_image.fx", m_imageProgram, true, false))
            {
                LOG("Failed to load imgui image shader");
                assert(0);
            }
            if (!shaderMgr->addShader("imgui_image_swizz.fx", m_imageSwizzProgram, true, false))
            {
                LOG("Failed to load imgui image shader");
                assert(0);
            }
        }
        else
        {
            LOG("Could not find the shader manager");
            assert(0);
        }
        
        m_missingTexture = genMissingTexture(256, 256, 0.04f);

        const ImguiFontReference handle = createFont(_data, _fontSize);
        m_currentFontIdx = handle;

        return handle;
    }

    void destroy()
    {

        IMGUI_destroy();
    }

    bool anyActive() const
    {
        return m_active != 0;
    }

    inline void updatePresence(uint32_t _id)
    {
        if (m_checkActivePresence && m_active == _id)
        {
            m_isActivePresent = true;
        }
    }

    uint32_t getId()
    {
        const uint32_t id = (m_areaId << 16) | m_widgetId++;
        updatePresence(id);
        return id;
    }

    bool isActive(uint32_t _id) const
    {
        return m_active == _id;
    }

    bool isActiveInputField(uint32_t _id) const
    {
        return m_inputField == _id;
    }

    bool isHot(uint32_t _id) const
    {
        return m_hot == _id;
    }

    bool inRect(int32_t _x, int32_t _y, int32_t _width, int32_t _height, bool _checkScroll = true) const
    {
        return (!_checkScroll || m_areas[m_areaId].m_inside)
            && m_mx >= _x
            && m_mx <= _x + _width
            && m_my >= _y
            && m_my <= _y + _height;
    }

    bool isEnabled(uint16_t _areaId)
    {
        return (m_enabledAreaIds>>_areaId)&0x1;
    }

    void setEnabled(uint16_t _areaId)
    {
        m_enabledAreaIds |= (UINT64_C(1)<<_areaId);
    }

    void clearInput()
    {
        m_leftPressed = false;
        m_leftReleased = false;
        m_scroll = 0;
    }

    void clearActive()
    {
        m_active = 0;
        // mark all UI for this frame as processed
        clearInput();
    }

    void clearActiveInputField()
    {
        m_inputField = 0;
    }

    void setActive(uint32_t _id)
    {
        m_active = _id;
        m_wentActive = true;
        m_inputField = 0;
    }

    void setActiveInputField(uint32_t _id)
    {
        m_inputField = _id;
    }

    void setHot(uint32_t _id)
    {
        m_hotToBe = _id;
    }

    bool buttonLogic(uint32_t _id, bool _over)
    {
        bool res = false;
        // process down
        if (!anyActive() )
        {
            if (_over)
            {
                setHot(_id);
            }

            if (isHot(_id)
            && m_leftPressed)
            {
                setActive(_id);
            }
        }

        // if button is active, then react on left up
        if (isActive(_id) )
        {
            if (_over)
            {
                setHot(_id);
            }

            if (m_leftReleased)
            {
                if (isHot(_id) )
                {
                    res = true;
                }

                clearActive();
            }
        }

        if (isHot(_id) )
        {
            m_isHot = true;
        }

        return res;
    }

    void inputLogic(uint32_t _id, bool _over)
    {
        if (!anyActive() )
        {
            if (_over)
            {
                setHot(_id);
            }

            if (isHot(_id)
            && m_leftPressed)
            {
                // Toggle active input.
                if (isActiveInputField(_id))
                {
                    clearActiveInputField();
                }
                else
                {
                    setActiveInputField(_id);
                }
            }
        }

        if (isHot(_id) )
        {
            m_isHot = true;
        }

        if (m_leftPressed
        &&  !m_isHot
        &&  m_inputField != 0)
        {
            clearActiveInputField();
        }
    }

    void updateInput(int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, char _inputChar)
    {
        bool left = (_button & IMGUI_MBUT_LEFT) != 0;

        m_mx = _mx;
        m_my = _my;
        m_leftPressed = !m_left && left;
        m_leftReleased = m_left && !left;
        m_left = left;
        m_scroll = _scroll;

        _inputChar = _inputChar & 0x7f; // ASCII or GTFO! :)
        m_lastChar = m_char;
        m_char = _inputChar;
    }

    inline bool scanCodeToAscii(HKL keyboardLayout, uint8_t* keyboardState, uint32_t scancode, uint8_t& result)
    {
        UINT vk = MapVirtualKeyEx(scancode, 1, keyboardLayout);
        uint16_t converted[4];
        if (ToAsciiEx(vk, scancode, keyboardState, converted, 0, keyboardLayout) == 1)
        {
            result = converted[0] & 0x7f;
            return true;
        }
        return false;
    }


    void beginFrame(Ctr::InputState* inputState, int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, uint16_t _width, uint16_t _height, char _inputChar, uint8_t _view)
    {
        IMGUI_beginFrame(inputState, _mx, _my, _button, _width, _height, _inputChar, _view);

        m_view = _view;
        m_viewWidth = _width;
        m_viewHeight = _height;
        {
            Ctr::Matrix44f ortho;            
            float L = 0;
            float R = float(m_Device->backbuffer()->width());
            float T = 0;
            float B = float(m_Device->backbuffer()->height());

            const float mvp[4][4] =
            {
                { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
                { 0.0f, 2.0f / (T - B), 0.0f, 0.0f, },
                { 0.0f, 0.0f, 0.5f, 0.0f },
                { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
            };
            memcpy(&ortho._m[0][0], &mvp[0][0], sizeof(ortho._m));

            Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
            uiRenderer->setViewProj(ortho);
            Ctr::Viewport viewport(0.0f, 0.0f, float(_width), float(_height), 0.0f, 1.0f);
            m_Device->setViewport(&viewport);
        }

        bool recordedKeyDown = false;
        m_char = 0;
        // Update Inputs
        {
            updateInput(_mx, _my, _button, _scroll, _inputChar);
            static HKL keyboardLayout = GetKeyboardLayout(0);
            static uint8_t keyboardState[256];
            GetKeyboardState(keyboardState);

            for (uint32_t scanCode = 0; scanCode < 512; scanCode++)
            {
                bool keydown = inputState->getKeyState(scanCode);

                if (!keydown)
                {
                    m_KeyRepeatTimes[scanCode] = 0.0f;
                }

                // If keycode and key state converts to ascii, add it as an input character.
                if (keydown)
                {
                    if (m_KeyRepeatTimes[scanCode] > 0.2)
                    {
                        m_KeyRepeatTimes[scanCode] = 0.0f;
                    }

                    if (m_KeyRepeatTimes[scanCode] == 0.0f)
                    {
                        uint8_t asciiInput = (uint8_t)(0);
                        if (scanCodeToAscii(keyboardLayout, keyboardState, scanCode, asciiInput))
                        {
                            if (!recordedKeyDown)
                            {
                                m_char = asciiInput;
                                _inputChar = asciiInput;
                                recordedKeyDown = true;
                            }
                        }
                    }

                    m_KeyRepeatTimes[scanCode] += 1.0f / 60.0f;
                }
            }
        }
        m_hot = m_hotToBe;
        m_hotToBe = 0;

        m_wentActive = false;
        m_isHot = false;

        Area& area = getCurrentArea();
        area.m_widgetX = 0;
        area.m_widgetY = 0;
        area.m_widgetW = 0;

        m_areaId.reset();
        m_widgetId = 0;
        m_enabledAreaIds = 0;
        m_insideArea = false;

        m_isActivePresent = false;
    }

    void endFrame()
    {
        if (m_checkActivePresence && !m_isActivePresent)
        {
            // The ui element is not present any more, reset active field.
            m_active = 0;
        }
        m_checkActivePresence = (0 != m_active);

        clearInput();

        IMGUI_endFrame();
        // Reset Scissor Enabled.
        m_Device->setScissorEnabled(false);
    }

    bool beginScroll(int32_t _height, int32_t* _scroll, bool _enabled)
    {
        Area& parentArea = getCurrentArea();

        m_areaId.next();
        const uint32_t scrollId = getId();
        Area& area = getCurrentArea();

        const uint16_t parentBottom = parentArea.m_scissorY + parentArea.m_scissorHeight;
        const uint16_t childBottom = parentArea.m_widgetY + _height;
        const uint16_t bottom = IMGUI_MIN(childBottom, parentBottom);

        const uint16_t top = IMGUI_MAX(parentArea.m_widgetY, parentArea.m_scissorY);

        area.m_contentX      = parentArea.m_contentX;
        area.m_contentY      = parentArea.m_widgetY;
        area.m_contentWidth  = parentArea.m_contentWidth - (SCROLL_AREA_PADDING*3);
        area.m_contentHeight = _height;
        area.m_widgetX       = parentArea.m_widgetX;
        area.m_widgetY       = parentArea.m_widgetY + (*_scroll);
        area.m_widgetW       = parentArea.m_widgetW - (SCROLL_AREA_PADDING*3);

        area.m_scissorX     = area.m_contentX;
        area.m_scissorWidth = area.m_contentWidth;

        area.m_scissorY       = top - 1;
        area.m_scissorHeight  = bottom - top;
        area.m_scissorEnabled = true;

        area.m_height = _height;

        area.m_scrollVal = _scroll;
        area.m_scrollId = scrollId;

        area.m_inside = inRect(parentArea.m_scissorX
                             , area.m_scissorY
                             , parentArea.m_scissorWidth
                             , area.m_scissorHeight
                             , false
                             );
        area.m_didScroll = false;

        parentArea.m_widgetY += (_height + DEFAULT_SPACING);

        if (_enabled)
        {
            setEnabled(m_areaId);
        }

        nvgScissor(m_nvg
                 , float(area.m_scissorX)
                 , float(area.m_scissorY-1)
                 , float(area.m_scissorWidth)
                 , float(area.m_scissorHeight+1)
                 );

        m_insideArea |= area.m_inside;

        return area.m_inside;
    }

    void endScroll(int32_t _r)
    {
        Area& area = getCurrentArea();
        area.m_scissorEnabled = false;

        const int32_t xx     = area.m_contentX + area.m_contentWidth - 1;
        const int32_t yy     = area.m_contentY;
        const int32_t width  = SCROLL_AREA_PADDING * 2;
        const int32_t height = area.m_height;

        const int32_t aa = area.m_contentY+area.m_height;
        const int32_t bb = area.m_widgetY-DEFAULT_SPACING;
        const int32_t sbot = IMGUI_MAX(aa, bb);
        const int32_t stop = area.m_contentY + (*area.m_scrollVal);
        const int32_t sh   = IMGUI_MAX(1, sbot - stop); // The scrollable area height.

        const uint32_t hid = area.m_scrollId;
        const float barHeight = (float)height / (float)sh;
        const bool hasScrollBar = (barHeight < 1.0f);

        // Handle mouse scrolling.
        if (area.m_inside && !area.m_didScroll && !anyActive() )
        {
            if (m_scroll)
            {
                const int32_t diff = height - sh;

                const int32_t val = *area.m_scrollVal + 20*m_scroll;
                const int32_t min = (diff < 0) ? diff : *area.m_scrollVal;
                const int32_t max = 0;
                const int32_t newVal = IMGUI_CLAMP(val, min, max);
                *area.m_scrollVal = newVal;

                if (hasScrollBar)
                {
                    area.m_didScroll = true;
                }
            }
        }

        area.m_inside = false;

        int32_t* scroll = area.m_scrollVal;

        // This must be called here before drawing scroll bars
        // so that scissor of parrent area applies.
        m_areaId.pop();

        // Propagate 'didScroll' to parrent area to avoid scrolling multiple areas at once.
        Area& parentArea = getCurrentArea();
        parentArea.m_didScroll = (parentArea.m_didScroll || area.m_didScroll);

        // Draw and handle scroll click.
        if (hasScrollBar)
        {
            const float barY = Ctr::saturate( (float)(-(*scroll) ) / (float)sh);

            // Handle scroll bar logic.
            const int32_t hx = xx;
            const int32_t hy = yy + (int)(barY * height);
            const int32_t hw = width;
            const int32_t hh = (int)(barHeight * height);

            const int32_t range = height - (hh - 1);
            const bool over = inRect(hx, hy, hw, hh);
            buttonLogic(hid, over);
            if (isActive(hid) )
            {
                float uu = (float)(hy - yy) / (float)range;
                if (m_wentActive)
                {
                    m_dragY = m_my;
                    m_dragOrig = uu;
                }

                if (m_dragY != m_my)
                {
                    const int32_t diff = height - sh;

                    const int32_t drag = m_my - m_dragY;
                    const float dragFactor = float(sh)/float(height);

                    const int32_t val = *scroll - int32_t(drag*dragFactor);
                    const int32_t min = (diff < 0) ? diff : *scroll;
                    const int32_t max = 0;
                    *scroll = IMGUI_CLAMP(val, min, max);

                    m_dragY = m_my;
                }
            }

            // BG
            drawRoundedRect( (float)xx
                           , (float)yy
                           , (float)width
                           , (float)height
                           , (float)_r
                           , imguiRGBA(0, 0, 0, 196)
                           );

            // Bar
            if (isActive(hid) )
            {
                drawRoundedRect( (float)hx
                               , (float)hy
                               , (float)hw
                               , (float)hh
                               , (float)_r
                               , imguiRGBA(255, 196, 0, 196)
                               );
            }
            else
            {
                drawRoundedRect( (float)hx
                               , (float)hy
                               , (float)hw
                               , (float)hh
                               , (float)_r
                               , isHot(hid) ? imguiRGBA(255, 196, 0, 96) : imguiRGBA(255, 255, 255, 64)
                               );
            }
        }
        else
        {
            // Clear active if scroll is selected but not visible any more.
            if (isActive(hid))
            {
                clearActive();
            }
        }

        nvgScissor(m_nvg
                 , float(parentArea.m_scissorX)
                 , float(parentArea.m_scissorY-1)
                 , float(parentArea.m_scissorWidth)
                 , float(parentArea.m_scissorHeight+1)
                 );
    }

    bool beginArea(const char* _name, int32_t _x, int32_t _y, int32_t _width, int32_t _height, bool _enabled, int32_t _r)
    {
        m_areaId.next();
        const uint32_t scrollId = getId();

        const bool hasTitle = (NULL != _name && '\0' != _name[0]);
        const int32_t header = hasTitle ? AREA_HEADER : 0;

        Area& area = getCurrentArea();
        area.m_x = _x;
        area.m_y = _y;
        area.m_width = _width;
        area.m_height = _height;

        area.m_contentX      = area.m_x      + SCROLL_AREA_PADDING;
        area.m_contentY      = area.m_y      + SCROLL_AREA_PADDING + header;
        area.m_contentWidth  = area.m_width  - SCROLL_AREA_PADDING;
        area.m_contentHeight = area.m_height - SCROLL_AREA_PADDING*2 - header;

        area.m_scissorX      = area.m_contentX;
        area.m_scissorY      = area.m_y      + SCROLL_AREA_PADDING + header;
        area.m_scissorHeight = area.m_height - SCROLL_AREA_PADDING*2 - header;
        area.m_scissorWidth  = area.m_contentWidth;
        area.m_scissorEnabled = false;

        area.m_widgetX = area.m_contentX;
        area.m_widgetY = area.m_contentY;
        area.m_widgetW = area.m_width - SCROLL_AREA_PADDING*2;

        static int32_t s_zeroScroll = 0;
        area.m_scrollVal = &s_zeroScroll;
        area.m_scrollId = scrollId;
        area.m_inside = inRect(area.m_scissorX, area.m_scissorY, area.m_scissorWidth, area.m_scissorHeight, false);
        area.m_didScroll = false;

        if (_enabled)
        {
            setEnabled(m_areaId);
        }

        if (0 == _r)
        {
            drawRect( (float)_x
                    , (float)_y
                    , (float)_width  + 0.3f /*border fix for seamlessly joining two scroll areas*/
                    , (float)_height + 0.3f /*border fix for seamlessly joining two scroll areas*/
                    , imguiRGBA(0, 0, 0, 192)
                    );
        }
        else
        {
            drawRoundedRect( (float)_x
                           , (float)_y
                           , (float)_width
                           , (float)_height
                           , (float)_r
                           , imguiRGBA(0, 0, 0, 192)
                           );
        }

        if (hasTitle)
        {
            drawText(_x + 10
                   , _y + 18
                   , ImguiTextAlign::Left
                   , _name
                   , imguiRGBA(255, 255, 255, 128)
                   );
        }
        area.m_scissorEnabled = true;

        nvgBeginFrame(m_nvg, m_viewWidth, m_viewHeight, 1.0f);
        nvgScissor(m_nvg
                 , float(area.m_scissorX)
                 , float(area.m_scissorY-1)
                 , float(area.m_scissorWidth)
                 , float(area.m_scissorHeight+1)
                 );

        m_insideArea |= area.m_inside;
        return area.m_inside;
    }

    void endArea()
    {
        nvgResetScissor(m_nvg);
        nvgEndFrame(m_nvg);
    }

    bool button(const char* _text, bool _enabled, ImguiAlign::Enum _align, uint32_t _rgb0, int32_t _r)
    {
        const uint32_t id = getId();

        Area& area = getCurrentArea();
        const int32_t yy = area.m_widgetY;
        const int32_t height = BUTTON_HEIGHT;
        area.m_widgetY += BUTTON_HEIGHT + DEFAULT_SPACING;

        int32_t xx;
        int32_t width;
        if (ImguiAlign::Left == _align)
        {
            xx = area.m_contentX + SCROLL_AREA_PADDING;
            width = area.m_widgetW;
        }
        else if (ImguiAlign::LeftIndented == _align
             ||  ImguiAlign::Right        == _align)
        {
            xx = area.m_widgetX;
            width = area.m_widgetW-1; //TODO: -1 !
        }
        else //if (ImguiAlign::Center         == _align
             //||  ImguiAlign::CenterIndented == _align).
        {
            xx = area.m_widgetX;
            width = area.m_widgetW - (area.m_widgetX-area.m_scissorX);
        }

        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx, yy, width, height);
        const bool res = buttonLogic(id, over);

        const uint32_t rgb0 = _rgb0&0x00ffffff;

        drawRoundedRect( (float)xx
                       , (float)yy
                       , (float)width
                       , (float)height
                       , (float)_r
                       , rgb0 | imguiRGBA(0, 0, 0, isActive(id) ? 196 : 96)
                       );

        if (enabled)
        {
            drawText(xx + BUTTON_HEIGHT / 2
                , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , isHot(id) ? imguiRGBA(255, 196, 0, 255) : imguiRGBA(255, 255, 255, 200)
                );
        }
        else
        {
            drawText(xx + BUTTON_HEIGHT / 2
                , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , imguiRGBA(128, 128, 128, 200)
                );
        }

        return res;
    }

    bool item(const char* _text, bool _enabled)
    {
        const uint32_t id = getId();

        Area& area = getCurrentArea();
        const int32_t xx = area.m_widgetX;
        const int32_t yy = area.m_widgetY;
        const int32_t width = area.m_widgetW;
        const int32_t height = BUTTON_HEIGHT;
        area.m_widgetY += BUTTON_HEIGHT + DEFAULT_SPACING;

        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx, yy, width, height);
        const bool res = buttonLogic(id, over);

        if (isHot(id) )
        {
            drawRoundedRect( (float)xx
                , (float)yy
                , (float)width
                , (float)height
                , 2.0f
                , imguiRGBA(255, 196, 0, isActive(id) ? 196 : 96)
                );
        }

        if (enabled)
        {
            drawText(xx + BUTTON_HEIGHT / 2
                , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , imguiRGBA(255, 255, 255, 200)
                );
        }
        else
        {
            drawText(xx + BUTTON_HEIGHT / 2
                , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , imguiRGBA(128, 128, 128, 200)
                );
        }

        return res;
    }

    bool check(const char* _text, bool _checked, bool _enabled)
    {
        const uint32_t id = getId();

        Area& area = getCurrentArea();
        const int32_t xx = area.m_widgetX;
        const int32_t yy = area.m_widgetY;
        const int32_t width = area.m_widgetW;
        const int32_t height = BUTTON_HEIGHT;
        area.m_widgetY += BUTTON_HEIGHT + DEFAULT_SPACING;

        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx, yy, width, height);
        const bool res = buttonLogic(id, over);

        const int32_t cx = xx + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;
        const int32_t cy = yy + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;
        drawRoundedRect( (float)cx - 3
            , (float)cy - 3
            , (float)CHECK_SIZE + 6
            , (float)CHECK_SIZE + 6
            , 4
            , imguiRGBA(128, 128, 128, isActive(id) ? 196 : 96)
            );

        if (_checked)
        {
            if (enabled)
            {
                drawRoundedRect( (float)cx
                    , (float)cy
                    , (float)CHECK_SIZE
                    , (float)CHECK_SIZE
                    , (float)CHECK_SIZE / 2 - 1
                    , imguiRGBA(255, 255, 255, isActive(id) ? 255 : 200)
                    );
            }
            else
            {
                drawRoundedRect( (float)cx
                    , (float)cy
                    , (float)CHECK_SIZE
                    , (float)CHECK_SIZE
                    , (float)CHECK_SIZE / 2 - 1
                    , imguiRGBA(128, 128, 128, 200)
                    );
            }
        }

        if (enabled)
        {
            drawText(xx + BUTTON_HEIGHT
                , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , isHot(id) ? imguiRGBA(255, 196, 0, 255) : imguiRGBA(255, 255, 255, 200)
                );
        }
        else
        {
            drawText(xx + BUTTON_HEIGHT
                , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , imguiRGBA(128, 128, 128, 200)
                );
        }

        return res;
    }

    void input(const char* _label, char* _str, uint32_t _len, bool _enabled, ImguiAlign::Enum _align, int32_t _r)
    {
        const uint32_t id = getId();

        Area& area = getCurrentArea();
        const int32_t yy = area.m_widgetY;
        area.m_widgetY += BUTTON_HEIGHT + DEFAULT_SPACING;

        int32_t xx;
        int32_t width;
        if (ImguiAlign::Left == _align)
        {
            xx = area.m_contentX + SCROLL_AREA_PADDING;
            width = area.m_widgetW;
        }
        else if (ImguiAlign::LeftIndented == _align
             ||  ImguiAlign::Right        == _align)
        {
            xx = area.m_widgetX;
            width = area.m_widgetW-1; //TODO: -1 !
        }
        else //if (ImguiAlign::Center         == _align
             //||  ImguiAlign::CenterIndented == _align).
        {
            xx = area.m_widgetX;
            width = area.m_widgetW - (area.m_widgetX-area.m_scissorX);
        }

        const bool drawLabel = (NULL != _label && _label[0] != '\0');

        if (drawLabel)
        {
            drawText(xx
                   , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                   , ImguiTextAlign::Left
                   , _label
                   , imguiRGBA(255, 255, 255, 200)
                   );
        }

        // Handle input.
        if (isActiveInputField(id) )
        {
            const size_t cursor = size_t(strlen(_str));

            if (m_char == 0x08) //backspace
            {
                _str[cursor-1] = '\0';
            }
            else if (m_char == 0x0d || m_char == 0x1b) //enter or escape
            {
                clearActiveInputField();
            }
            else if (cursor < _len-1
                 &&  0 != m_char)
            {
                _str[cursor] = m_char;
                _str[cursor+1] = '\0';
            }
        }

        // Draw input area.
        const int32_t height = BUTTON_HEIGHT;
        if (drawLabel)
        {
            uint32_t numVertices = 0; //unused
            const int32_t labelWidth = int32_t(getTextLength(m_fonts[m_currentFontIdx-1].m_cdata, _label, numVertices));
            xx    += (labelWidth + 6);
            width -= (labelWidth + 6);
        }
        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx, yy, width, height);
        inputLogic(id, over);

        drawRoundedRect( (float)xx
                       , (float)yy
                       , (float)width
                       , (float)height
                       , (float)_r
                       , isActiveInputField(id)?imguiRGBA(255,196,0,255):imguiRGBA(128,128,128,96)
                       );

        if (isActiveInputField(id) )
        {
            drawText(xx + 6
                    , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                    , ImguiTextAlign::Left
                    , _str
                    , imguiRGBA(0, 0, 0, 255)
                    );
        }
        else
        {
            drawText(xx + 6
                    , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
                    , ImguiTextAlign::Left
                    , _str
                    , isHot(id) ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,255)
                    );
        }
    }

    uint8_t tabs(uint8_t _selected, bool _enabled, ImguiAlign::Enum _align, int32_t _height, int32_t _r, uint8_t _nTabs, const ImguiEnumVal* values)
    {
        const char* titles[16];
        bool tabEnabled[16];
        const uint8_t tabCount = IMGUI_MIN(_nTabs, 16);
        const uint8_t enabledCount = 0;

        // Read titles.
        {
            uint8_t ii = 0;
            for (; ii < tabCount; ++ii)
            {
                const char* str = values[ii].label;
                titles[ii] = str;
            }
        }

        // *Set* enabled tabs o_O.
        {
            uint8_t ii = 0;          
            for (ii = 0; ii < _nTabs; ++ii)
            {
                tabEnabled[ii] = true;
            }
        }

        Area& area = getCurrentArea();
        const int32_t yy = area.m_widgetY;
        area.m_widgetY += _height + DEFAULT_SPACING;

        int32_t xx;
        int32_t width;
        if (ImguiAlign::Left == _align)
        {
            xx = area.m_contentX + SCROLL_AREA_PADDING;
            width = area.m_widgetW;
        }
        else if (ImguiAlign::LeftIndented == _align
            || ImguiAlign::Right == _align)
        {
            xx = area.m_widgetX;
            width = area.m_widgetW - 1; //TODO: -1 !
        }
        else //if (ImguiAlign::Center         == _align
            //||  ImguiAlign::CenterIndented == _align).
        {
            xx = area.m_widgetX;
            width = area.m_widgetW - (area.m_widgetX - area.m_scissorX);
        }

        uint8_t selected = _selected;
        const int32_t tabWidth = width / tabCount;
        const int32_t tabWidthHalf = width / (tabCount * 2);
        const int32_t textY = yy + _height / 2 + int32_t(m_fonts[m_currentFontIdx - 1].m_size) / 2 - 2;

        drawRoundedRect((float)xx
            , (float)yy
            , (float)width
            , (float)_height
            , (float)_r
            , _enabled ? imguiRGBA(128, 128, 128, 96) : imguiRGBA(128, 128, 128, 64)
            );

        for (uint8_t ii = 0; ii < tabCount; ++ii)
        {
            const uint32_t id = getId();

            int32_t buttonX = xx + ii*width / tabCount;
            int32_t textX = buttonX + tabWidthHalf;

            const bool enabled = _enabled && tabEnabled[ii] && isEnabled(m_areaId);
            const bool over = enabled && inRect(buttonX, yy, tabWidth, _height);
            const bool res = buttonLogic(id, over);

            const uint32_t textColor = (ii == selected)
                ? (enabled ? imguiRGBA(0, 0, 0, 255) : imguiRGBA(255, 255, 255, 100))
                : (isHot(id) ? imguiRGBA(255, 196, 0, enabled ? 255 : 100) : imguiRGBA(255, 255, 255, enabled ? 200 : 100))
                ;

            if (ii == selected)
            {
                drawRoundedRect((float)buttonX
                    , (float)yy
                    , (float)tabWidth
                    , (float)_height
                    , (float)_r
                    , enabled ? imguiRGBA(255, 196, 0, 200) : imguiRGBA(128, 128, 128, 32)
                    );
            }
            else if (isActive(id))
            {
                drawRoundedRect((float)buttonX
                    , (float)yy
                    , (float)tabWidth
                    , (float)_height
                    , (float)_r
                    , imguiRGBA(128, 128, 128, 196)
                    );
            }

            drawText(textX
                , textY
                , ImguiTextAlign::Center
                , titles[ii]
                , textColor
                );

            if (res)
            {
                selected = ii;
            }
        }

        return selected;
    }



    bool image(const Ctr::ITexture* _image, float _lod, int32_t _width, int32_t _height, ImguiAlign::Enum _align, bool _enabled, bool _originBottomLeft)
    {
        const uint32_t id = getId();
        Area& area = getCurrentArea();
        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();

        int32_t xx;
        if (ImguiAlign::Left == _align)
        {
            xx = area.m_contentX + SCROLL_AREA_PADDING;
        }
        else if (ImguiAlign::LeftIndented == _align)
        {
            xx = area.m_widgetX;
        }
        else if (ImguiAlign::Center == _align)
        {
            xx = area.m_contentX + (area.m_widgetW-_width)/2;
        }
        else if (ImguiAlign::CenterIndented == _align)
        {
            xx = (area.m_widgetX + area.m_widgetW + area.m_contentX - _width)/2;
        }
        else //if (ImguiAlign::Right == _align).
        {
            xx = area.m_contentX + area.m_widgetW - _width;
        }

        const int32_t yy = area.m_widgetY;
        area.m_widgetY += _height + DEFAULT_SPACING;

        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx, yy, _width, _height);
        const bool res = buttonLogic(id, over);

        const float lodEnabled[4] = { _lod, float(enabled), 0.0f, 0.0f };

        screenQuad(xx, yy, _width, _height, _originBottomLeft);

        // TODO. Primitive type!
        const Ctr::GpuVariable* imageLodEnabledVariable = nullptr;
        if (m_imageProgram->getParameterByName("u_imageLodEnabled", imageLodEnabledVariable))
        {
            imageLodEnabledVariable->setVector(lodEnabled);
        }
        const Ctr::GpuVariable* textureVariable = nullptr;
        if (m_imageProgram->getParameterByName("s_tex", textureVariable))
        {
            textureVariable->setTexture(_image ? _image : m_missingTexture);
        }

        uiRenderer->device()->enableAlphaBlending();
        uiRenderer->device()->setupBlendPipeline(Ctr::BlendAlpha);
        uiRenderer->setShader(m_imageProgram);
        setCurrentScissor();
        uiRenderer->render(6, 0);
        uiRenderer->device()->disableAlphaBlending();



        return res;
    }

    bool image(const Ctr::ITexture* _image, float _lod, float _width, float _aspect, ImguiAlign::Enum _align, bool _enabled, bool _originBottomLeft)
    {
        const float width = _width*float(getCurrentArea().m_widgetW);
        const float height = width/_aspect;

        return image(_image, _lod, int32_t(width), int32_t(height), _align, _enabled, _originBottomLeft);
    }

    bool imageChannel(const Ctr::ITexture* _image, uint8_t _channel, float _lod, int32_t _width, int32_t _height, ImguiAlign::Enum _align, bool _enabled)
    {
//        BX_CHECK(_channel < 4, "Channel param must be from 0 to 3!");
        const uint32_t id = getId();
        Area& area = getCurrentArea();
        Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();

        int32_t xx;
        if (ImguiAlign::Left == _align)
        {
            xx = area.m_contentX + SCROLL_AREA_PADDING;
        }
        else if (ImguiAlign::LeftIndented == _align)
        {
            xx = area.m_widgetX;
        }
        else if (ImguiAlign::Center == _align)
        {
            xx = area.m_contentX + (area.m_widgetW-_width)/2;
        }
        else if (ImguiAlign::CenterIndented == _align)
        {
            xx = (area.m_widgetX + area.m_widgetW + area.m_contentX - _width)/2;
        }
        else //if (ImguiAlign::Right == _align).
        {
            xx = area.m_contentX + area.m_widgetW - _width;
        }

        const int32_t yy = area.m_widgetY;
        area.m_widgetY += _height + DEFAULT_SPACING;

        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx, yy, _width, _height);
        const bool res = buttonLogic(id, over);

        screenQuad(xx, yy, _width, _height);

        const float lodEnabled[4] = { _lod, float(enabled), 0.0f, 0.0f };
        const Ctr::GpuVariable* imageLodEnabledVariable = nullptr;
        if (m_imageSwizzProgram->getParameterByName("u_imageLodEnabled", imageLodEnabledVariable))
        {
            imageLodEnabledVariable->setVector(lodEnabled);
        }


        const Ctr::GpuVariable* imageSwizzleVariable = nullptr;
        float swizz[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        swizz[_channel] = 1.0f;

        if (m_imageSwizzProgram->getParameterByName("u_imageSwizzle", imageSwizzleVariable))
        {
            imageSwizzleVariable->setVector(swizz);
        }

        const Ctr::GpuVariable* textureVariable = nullptr;
        if (m_imageSwizzProgram->getParameterByName("s_tex", textureVariable))
        {
            textureVariable->setTexture(_image ? _image : m_missingTexture);
        }

        uiRenderer->device()->enableAlphaBlending();
        uiRenderer->device()->setupBlendPipeline(Ctr::BlendAlpha);

        uiRenderer->setShader(m_imageSwizzProgram);
        setCurrentScissor();
        // Draw screen quad.
        uiRenderer->render(6, 0);
        uiRenderer->device()->disableAlphaBlending();

        return res;
    }

    bool imageChannel(const Ctr::ITexture*_image, uint8_t _channel, float _lod, float _width, float _aspect, ImguiAlign::Enum _align, bool _enabled)
    {
        const float width = _width*float(getCurrentArea().m_widgetW);
        const float height = width/_aspect;

        return imageChannel(_image, _channel, _lod, int32_t(width), int32_t(height), _align, _enabled);
    }

    bool cubeMap(const Ctr::ITexture* _cubemap, float _lod, bool _cross, ImguiAlign::Enum _align, bool _enabled)
    {
        const uint32_t numVertices = 14;
        const uint32_t numIndices  = 36;

        {
            Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
            Ctr::IVertexBuffer* vb = uiRenderer->vertexBuffer(PosNormalVertex::ms_decl);
            PosNormalVertex* vertex = (PosNormalVertex*)vb->lock(numVertices * sizeof(PosNormalVertex));

            Ctr::IIndexBuffer * ib = uiRenderer->indexBuffer();
            uint32_t* indices = (uint32_t*)ib->lock(numIndices * sizeof(uint32_t));

            if (_cross)
            {
                vertex->set(0.0f, 0.5f, 0.0f, -1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(0.0f, 1.0f, 0.0f, -1.0f, -1.0f, -1.0f); ++vertex;

                vertex->set(0.5f, 0.0f, 0.0f, -1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(0.5f, 0.5f, 0.0f, -1.0f,  1.0f,  1.0f); ++vertex;
                vertex->set(0.5f, 1.0f, 0.0f, -1.0f, -1.0f,  1.0f); ++vertex;
                vertex->set(0.5f, 1.5f, 0.0f, -1.0f, -1.0f, -1.0f); ++vertex;

                vertex->set(1.0f, 0.0f, 0.0f,  1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(1.0f, 0.5f, 0.0f,  1.0f,  1.0f,  1.0f); ++vertex;
                vertex->set(1.0f, 1.0f, 0.0f,  1.0f, -1.0f,  1.0f); ++vertex;
                vertex->set(1.0f, 1.5f, 0.0f,  1.0f, -1.0f, -1.0f); ++vertex;

                vertex->set(1.5f, 0.5f, 0.0f,  1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(1.5f, 1.0f, 0.0f,  1.0f, -1.0f, -1.0f); ++vertex;

                vertex->set(2.0f, 0.5f, 0.0f, -1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(2.0f, 1.0f, 0.0f, -1.0f, -1.0f, -1.0f); ++vertex;

                indices += addQuad(indices,  0,  3,  4,  1);
                indices += addQuad(indices,  2,  6,  7,  3);
                indices += addQuad(indices,  3,  7,  8,  4);
                indices += addQuad(indices,  4,  8,  9,  5);
                indices += addQuad(indices,  7, 10, 11,  8);
                indices += addQuad(indices, 10, 12, 13, 11);
            }
            else
            {
                vertex->set(0.0f, 0.25f, 0.0f, -1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(0.0f, 0.75f, 0.0f, -1.0f, -1.0f, -1.0f); ++vertex;

                vertex->set(0.5f, 0.00f, 0.0f, -1.0f,  1.0f,  1.0f); ++vertex;
                vertex->set(0.5f, 0.50f, 0.0f, -1.0f, -1.0f,  1.0f); ++vertex;
                vertex->set(0.5f, 1.00f, 0.0f,  1.0f, -1.0f, -1.0f); ++vertex;

                vertex->set(1.0f, 0.25f, 0.0f,  1.0f,  1.0f,  1.0f); ++vertex;
                vertex->set(1.0f, 0.75f, 0.0f,  1.0f, -1.0f,  1.0f); ++vertex;

                vertex->set(1.0f, 0.25f, 0.0f,  1.0f,  1.0f,  1.0f); ++vertex;
                vertex->set(1.0f, 0.75f, 0.0f,  1.0f, -1.0f,  1.0f); ++vertex;

                vertex->set(1.5f, 0.00f, 0.0f, -1.0f,  1.0f,  1.0f); ++vertex;
                vertex->set(1.5f, 0.50f, 0.0f,  1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(1.5f, 1.00f, 0.0f,  1.0f, -1.0f, -1.0f); ++vertex;

                vertex->set(2.0f, 0.25f, 0.0f, -1.0f,  1.0f, -1.0f); ++vertex;
                vertex->set(2.0f, 0.75f, 0.0f, -1.0f, -1.0f, -1.0f); ++vertex;

                indices += addQuad(indices,  0,  2,  3,  1);
                indices += addQuad(indices,  1,  3,  6,  4);
                indices += addQuad(indices,  2,  5,  6,  3);
                indices += addQuad(indices,  7,  9, 12, 10);
                indices += addQuad(indices,  7, 10, 11,  8);
                indices += addQuad(indices, 10, 12, 13, 11);
            }
            vb->unlock();
            uiRenderer->indexBuffer()->unlock();

            const uint32_t id = getId();

            Area& area = getCurrentArea();
            int32_t xx;
            int32_t width;
            if (ImguiAlign::Left == _align)
            {
                xx = area.m_contentX + SCROLL_AREA_PADDING;
                width = area.m_widgetW;
            }
            else if (ImguiAlign::LeftIndented == _align
                 ||  ImguiAlign::Right        == _align)
            {
                xx = area.m_widgetX;
                width = area.m_widgetW;
            }
            else //if (ImguiAlign::Center         == _align
                 //||  ImguiAlign::CenterIndented == _align).
            {
                xx = area.m_widgetX;
                width = area.m_widgetW - (area.m_widgetX-area.m_scissorX);
            }

            const uint32_t height = _cross ? (width*3)/4 : (width/2);
            const int32_t yy = area.m_widgetY;
            area.m_widgetY += height + DEFAULT_SPACING;

            const bool enabled = _enabled && isEnabled(m_areaId);
            const bool over = enabled && inRect(xx, yy, width, height);
            const bool res = buttonLogic(id, over);

            const float scale = float(width/2)+0.25f;

            Ctr::Matrix44f srt;
            Ctr::Matrix44f projection;
            Ctr::Matrix44f transform;

            const float L = 0.0f;
            const float R = float(m_Device->backbuffer()->width());
            const float B = float(m_Device->backbuffer()->height());
            const float T = 0.0f;
            const float ortho[4][4] =
            {
                { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
                { 0.0f, 2.0f / (T - B), 0.0f, 0.0f, },
                { 0.0f, 0.0f, 0.5f, 0.0f },
                { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
            };
            memcpy(&projection._m[0][0], &ortho[0][0], sizeof(float) * 16); 

            Ctr::Matrix44f t;
            Ctr::Matrix44f s;

            s.scaling(Ctr::Vector3f(scale, scale, 1.0f));
            t.setTranslation(Ctr::Vector3f(float(xx),float(yy),0));
            transform = s * t * projection;

            uiRenderer->setViewProj(transform);


            const float lodEnabled[4] = { _lod, float(enabled), 0.0f, 0.0f };
            const Ctr::GpuVariable* imageLodEnabledVariable = nullptr;
            if (m_cubeMapProgram->getParameterByName("u_imageLodEnabled", imageLodEnabledVariable))
            {
                imageLodEnabledVariable->setVector(lodEnabled);
            }


            const Ctr::GpuVariable* textureVariable = nullptr;
            if (m_cubeMapProgram->getParameterByName("s_tex", textureVariable))
            {
                textureVariable->setTexture(_cubemap);
            }



            uiRenderer->setShader(m_cubeMapProgram);
            uiRenderer->setVertexBuffer(vb);
            uiRenderer->setDrawIndexed(true);
            

            uiRenderer->device()->enableAlphaBlending();
            uiRenderer->device()->setupBlendPipeline(Ctr::BlendAlpha);

            setCurrentScissor();
            uiRenderer->render(numIndices, 0);
            uiRenderer->setDrawIndexed(false);

            uiRenderer->device()->disableAlphaBlending();
            uiRenderer->setViewProj(projection);

            return res;
        }
        return false;
    }

    bool collapse(const char* _text, const char* _subtext, bool _checked, bool _enabled)
    {
        const uint32_t id = getId();

        Area& area = getCurrentArea();
        const int32_t xx = area.m_widgetX;
        const int32_t yy = area.m_widgetY;
        const int32_t width = area.m_widgetW;
        const int32_t height = BUTTON_HEIGHT;
        area.m_widgetY += BUTTON_HEIGHT + DEFAULT_SPACING;

        const int32_t cx = xx + BUTTON_HEIGHT/2 - CHECK_SIZE/2;
        const int32_t cy = yy + BUTTON_HEIGHT/2 - CHECK_SIZE/2 + DEFAULT_SPACING/2;

        const int32_t textY = yy + BUTTON_HEIGHT/2 + TEXT_HEIGHT/2 + DEFAULT_SPACING/2;

        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx, yy, width, height);
        const bool res = buttonLogic(id, over);

        if (_checked)
        {
            drawTriangle(cx
                , cy
                , CHECK_SIZE
                , CHECK_SIZE
                , TriangleOrientation::Up
                , imguiRGBA(255, 255, 255, isActive(id) ? 255 : 200)
                );
        }
        else
        {
            drawTriangle(cx-1 // With -1 is more aesthetically pleasing.
                , cy
                , CHECK_SIZE
                , CHECK_SIZE
                , TriangleOrientation::Right
                , imguiRGBA(255, 255, 255, isActive(id) ? 255 : 200)
                );
        }

        if (enabled)
        {
            drawText(xx + BUTTON_HEIGHT
                , textY
                , ImguiTextAlign::Left
                , _text
                , isHot(id) ? imguiRGBA(255, 196, 0, 255) : imguiRGBA(255, 255, 255, 200)
                );
        }
        else
        {
            drawText(xx + BUTTON_HEIGHT
                , textY
                , ImguiTextAlign::Left
                , _text
                , imguiRGBA(128, 128, 128, 200)
                );
        }

        if (_subtext)
        {
            drawText(xx + width - BUTTON_HEIGHT / 2
                , textY
                , ImguiTextAlign::Right
                , _subtext
                , imguiRGBA(255, 255, 255, 128)
                );
        }

        return res;
    }

    bool borderButton(ImguiBorder::Enum _border, bool _checked, bool _enabled)
    {
        // Since border button isn't part of any area, just use this custom/unique areaId.
        const uint16_t areaId = UINT16_MAX-1;
        const uint32_t id = (areaId << 16) | m_widgetId++;
        updatePresence(id);

        const int32_t triSize = 12;
        const int32_t borderSize = 15;

        int32_t xx;
        int32_t yy;
        int32_t width;
        int32_t height;
        int32_t triX;
        int32_t triY;
        TriangleOrientation::Enum orientation;

        if (ImguiBorder::Left == _border)
        {
            xx = -borderSize;
            yy = -1;
            width = 2*borderSize+1;
            height = m_viewHeight;
            triX = 0;
            triY = (m_viewHeight-triSize)/2;
            orientation = _checked ? TriangleOrientation::Left : TriangleOrientation::Right;
        }
        else if (ImguiBorder::Right == _border)
        {
            xx = m_viewWidth - borderSize;
            yy = -1;
            width = 2*borderSize+1;
            height = m_viewHeight;
            triX = m_viewWidth - triSize - 2;
            triY = (m_viewHeight-width)/2;
            orientation = _checked ? TriangleOrientation::Right : TriangleOrientation::Left;
        }
        else if (ImguiBorder::Top == _border)
        {
            xx = 0;
            yy = -borderSize;
            width = m_viewWidth;
            height = 2*borderSize;
            triX = (m_viewWidth-triSize)/2;
            triY = 0;
            orientation = _checked ? TriangleOrientation::Up : TriangleOrientation::Down;
        }
        else //if (ImguiBorder::Bottom == _border).
        {
            xx = 0;
            yy = m_viewHeight - borderSize;
            width = m_viewWidth;
            height = 2*borderSize;
            triX = (m_viewWidth-triSize)/2;
            triY = m_viewHeight-triSize;
            orientation = _checked ? TriangleOrientation::Down : TriangleOrientation::Up;
        }

        const bool over = _enabled && inRect(xx, yy, width, height, false);
        const bool res = buttonLogic(id, over);

        drawRect( (float)xx
                , (float)yy
                , (float)width
                , (float)height
                , isActive(id) ? imguiRGBA(23, 23, 23, 192) : imguiRGBA(0, 0, 0, 222)
                );

        drawTriangle( triX
                    , triY
                    , triSize
                    , triSize
                    , orientation
                    , isHot(id) ? imguiRGBA(255, 196, 0, 222) : imguiRGBA(255, 255, 255, 192)
                    );

        return res;
    }

    void labelVargs(const char* _format, va_list _argList, uint32_t _rgba)
    {
        char temp[8192];
        char* out = temp;
        int32_t len = vsnprintf(out, sizeof(temp), _format, _argList);
        if ( (int32_t)sizeof(temp) < len)
        {
            out = (char*)alloca(len+1);
            len = vsnprintf(out, len, _format, _argList);
        }
        out[len] = '\0';

        Area& area = getCurrentArea();
        const int32_t xx = area.m_widgetX;
        const int32_t yy = area.m_widgetY;
        area.m_widgetY += BUTTON_HEIGHT;
        drawText(xx
            , yy + BUTTON_HEIGHT/2 + TEXT_HEIGHT/2
            , ImguiTextAlign::Left
            , out
            , _rgba
            );
    }

    void value(const char* _text)
    {
        Area& area = getCurrentArea();
        const int32_t xx = area.m_widgetX;
        const int32_t yy = area.m_widgetY;
        const int32_t ww = area.m_widgetW;
        area.m_widgetY += BUTTON_HEIGHT;

        drawText(xx + ww - BUTTON_HEIGHT / 2
            , yy + BUTTON_HEIGHT / 2 + TEXT_HEIGHT / 2
            , ImguiTextAlign::Right
            , _text
            , imguiRGBA(255, 255, 255, 200)
            );
    }

    bool slider(const char* _text, float& _val, float _vmin, float _vmax, float _vinc, bool _enabled, ImguiAlign::Enum _align)
    {
        const uint32_t id = getId();

        Area& area = getCurrentArea();
        const int32_t yy = area.m_widgetY;
        const int32_t height = SLIDER_HEIGHT;
        area.m_widgetY += SLIDER_HEIGHT + DEFAULT_SPACING;

        int32_t xx;
        int32_t width;
        if (ImguiAlign::Left == _align)
        {
            xx = area.m_contentX + SCROLL_AREA_PADDING;
            width = area.m_widgetW;
        }
        else if (ImguiAlign::LeftIndented == _align
             ||  ImguiAlign::Right        == _align)
        {
            xx = area.m_widgetX;
            width = area.m_widgetW;
        }
        else //if (ImguiAlign::Center         == _align
             //||  ImguiAlign::CenterIndented == _align).
        {
            xx = area.m_widgetX;
            width = area.m_widgetW - (area.m_widgetX-area.m_scissorX);
        }

        drawRoundedRect( (float)xx, (float)yy, (float)width, (float)height, 4.0f, imguiRGBA(0, 0, 0, 128) );

        const int32_t range = width - SLIDER_MARKER_WIDTH;

        float uu = Ctr::saturate( (_val - _vmin) / (_vmax - _vmin) );
        int32_t m = (int)(uu * range);
        bool valChanged = false;

        const bool enabled = _enabled && isEnabled(m_areaId);
        const bool over = enabled && inRect(xx + m, yy, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT);
        const bool res = buttonLogic(id, over);

        if (isActive(id) )
        {
            if (m_wentActive)
            {
                m_dragX = m_mx;
                m_dragOrig = uu;
            }

            if (m_dragX != m_mx)
            {
                uu = Ctr::saturate(m_dragOrig + (float)(m_mx - m_dragX) / (float)range);

                _val = _vmin + uu * (_vmax - _vmin);
                _val = floorf(_val / _vinc + 0.5f) * _vinc; // Snap to vinc
                m = (int)(uu * range);
                valChanged = true;
            }
        }

        if (isActive(id) )
        {
            drawRoundedRect( (float)(xx + m)
                , (float)yy
                , (float)SLIDER_MARKER_WIDTH
                , (float)SLIDER_HEIGHT
                , 4.0f
                , imguiRGBA(255, 255, 255, 255)
                );
        }
        else
        {
            drawRoundedRect( (float)(xx + m)
                , (float)yy
                , (float)SLIDER_MARKER_WIDTH
                , (float)SLIDER_HEIGHT
                , 4.0f
                , isHot(id) ? imguiRGBA(255, 196, 0, 128) : imguiRGBA(255, 255, 255, 64)
                );
        }

        // TODO: fix this, take a look at 'nicenum'.
        int32_t digits = (int)(ceilf(log10f(_vinc) ) );
        char fmt[16];
        sprintf(fmt, "%%.%df", digits >= 0 ? 0 : -digits);
        char msg[128];
        sprintf(msg, fmt, _val);

        if (enabled)
        {
            drawText(xx + SLIDER_HEIGHT / 2
                , yy + SLIDER_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , isHot(id) ? imguiRGBA(255, 196, 0, 255) : imguiRGBA(255, 255, 255, 200)
                );

            drawText(xx + width - SLIDER_HEIGHT / 2
                , yy + SLIDER_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Right
                , msg
                , isHot(id) ? imguiRGBA(255, 196, 0, 255) : imguiRGBA(255, 255, 255, 200)
                );
        }
        else
        {
            drawText(xx + SLIDER_HEIGHT / 2
                , yy + SLIDER_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Left
                , _text
                , imguiRGBA(128, 128, 128, 200)
                );

            drawText(xx + width - SLIDER_HEIGHT / 2
                , yy + SLIDER_HEIGHT / 2 + TEXT_HEIGHT / 2
                , ImguiTextAlign::Right
                , msg
                , imguiRGBA(128, 128, 128, 200)
                );
        }

        return res || valChanged;
    }

    void indent(uint16_t _width)
    {
        Area& area = getCurrentArea();
        area.m_widgetX += _width;
        area.m_widgetW -= _width;
    }

    void unindent(uint16_t _width)
    {
        Area& area = getCurrentArea();
        area.m_widgetX -= _width;
        area.m_widgetW += _width;
    }

    void separator(uint16_t _height)
    {
        Area& area = getCurrentArea();
        area.m_widgetY += _height;
    }

    void separatorLine(uint16_t _height)
    {
        Area& area = getCurrentArea();
        const int32_t rectWidth = area.m_widgetW;
        const int32_t rectHeight = 1;
        const int32_t xx = area.m_widgetX;
        const int32_t yy = area.m_widgetY + _height/2 - rectHeight;
        area.m_widgetY += _height;

        drawRect( (float)xx
                , (float)yy
                , (float)rectWidth
                , (float)rectHeight
                , imguiRGBA(255, 255, 255, 32)
                );
    }

    void drawPolygon(const float* _coords, uint32_t _numCoords, float _r, uint32_t _abgr)
    {
        _numCoords = Ctr::minValue((uint32_t)_numCoords, (uint32_t)(MAX_TEMP_COORDS));

        for (uint32_t ii = 0, jj = _numCoords - 1; ii < _numCoords; jj = ii++)
        {
            const float* v0 = &_coords[jj * 2];
            const float* v1 = &_coords[ii * 2];
            float dx = v1[0] - v0[0];
            float dy = v1[1] - v0[1];
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 0)
            {
                d = 1.0f / d;
                dx *= d;
                dy *= d;
            }

            m_tempNormals[jj * 2 + 0] = dy;
            m_tempNormals[jj * 2 + 1] = -dx;
        }

        for (uint32_t ii = 0, jj = _numCoords - 1; ii < _numCoords; jj = ii++)
        {
            float dlx0 = m_tempNormals[jj * 2 + 0];
            float dly0 = m_tempNormals[jj * 2 + 1];
            float dlx1 = m_tempNormals[ii * 2 + 0];
            float dly1 = m_tempNormals[ii * 2 + 1];
            float dmx = (dlx0 + dlx1) * 0.5f;
            float dmy = (dly0 + dly1) * 0.5f;
            float dmr2 = dmx * dmx + dmy * dmy;
            if (dmr2 > 0.000001f)
            {
                float scale = 1.0f / dmr2;
                if (scale > 10.0f)
                {
                    scale = 10.0f;
                }

                dmx *= scale;
                dmy *= scale;
            }

            m_tempCoords[ii * 2 + 0] = _coords[ii * 2 + 0] + dmx * _r;
            m_tempCoords[ii * 2 + 1] = _coords[ii * 2 + 1] + dmy * _r;
        }

        uint32_t numVertices = _numCoords*6 + (_numCoords-2)*3;
        {
            Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
            Ctr::IVertexBuffer* vb = uiRenderer->vertexBuffer(PosColorVertex::ms_decl);
            PosColorVertex* vertex = (PosColorVertex*)vb->lock(numVertices * sizeof(PosColorVertex));
            uint32_t trans = _abgr&0xffffff;

            for (uint32_t ii = 0, jj = _numCoords-1; ii < _numCoords; jj = ii++)
            {
                vertex->m_x = _coords[ii*2+0];
                vertex->m_y = _coords[ii*2+1];
                vertex->m_abgr = _abgr;
                ++vertex;

                vertex->m_x = _coords[jj*2+0];
                vertex->m_y = _coords[jj*2+1];
                vertex->m_abgr = _abgr;
                ++vertex;

                vertex->m_x = m_tempCoords[jj*2+0];
                vertex->m_y = m_tempCoords[jj*2+1];
                vertex->m_abgr = trans;
                ++vertex;

                vertex->m_x = m_tempCoords[jj*2+0];
                vertex->m_y = m_tempCoords[jj*2+1];
                vertex->m_abgr = trans;
                ++vertex;

                vertex->m_x = m_tempCoords[ii*2+0];
                vertex->m_y = m_tempCoords[ii*2+1];
                vertex->m_abgr = trans;
                ++vertex;

                vertex->m_x = _coords[ii*2+0];
                vertex->m_y = _coords[ii*2+1];
                vertex->m_abgr = _abgr;
                ++vertex;
            }

            for (uint32_t ii = 2; ii < _numCoords; ++ii)
            {
                vertex->m_x = _coords[0];
                vertex->m_y = _coords[1];
                vertex->m_abgr = _abgr;
                ++vertex;

                vertex->m_x = _coords[(ii-1)*2+0];
                vertex->m_y = _coords[(ii-1)*2+1];
                vertex->m_abgr = _abgr;
                ++vertex;

                vertex->m_x = _coords[ii*2+0];
                vertex->m_y = _coords[ii*2+1];
                vertex->m_abgr = _abgr;
                ++vertex;
            }
            vb->unlock();

            uiRenderer->setVertexBuffer(vb);
            uiRenderer->device()->enableAlphaBlending();
            uiRenderer->device()->setupBlendPipeline(Ctr::BlendAlpha);

            uiRenderer->setShader(m_colorProgram);
            setCurrentScissor();
            uiRenderer->render(numVertices, 0);
            uiRenderer->device()->disableAlphaBlending();
        }
    }

    void drawRect(float _x, float _y, float _w, float _h, uint32_t _argb, float _fth = 1.0f)
    {
        float verts[4 * 2] =
        {
            _x + 0.5f,      _y + 0.5f,
            _x + _w - 0.5f, _y + 0.5f,
            _x + _w - 0.5f, _y + _h - 0.5f,
            _x + 0.5f,      _y + _h - 0.5f,
        };

        drawPolygon(verts, 4, _fth, _argb);
    }

    void drawRoundedRect(float _x, float _y, float _w, float _h, float _r, uint32_t _argb, float _fth = 1.0f)
    {
        if (0.0f == _r)
        {
            return drawRect(_x, _y, _w, _h, _argb, _fth);
        }

        const uint32_t num = NUM_CIRCLE_VERTS / 4;
        const float* cverts = m_circleVerts;
        float verts[(num + 1) * 4 * 2];
        float* vv = verts;

        for (uint32_t ii = 0; ii <= num; ++ii)
        {
            *vv++ = _x + _w - _r + cverts[ii * 2] * _r;
            *vv++ = _y + _h - _r + cverts[ii * 2 + 1] * _r;
        }

        for (uint32_t ii = num; ii <= num * 2; ++ii)
        {
            *vv++ = _x + _r + cverts[ii * 2] * _r;
            *vv++ = _y + _h - _r + cverts[ii * 2 + 1] * _r;
        }

        for (uint32_t ii = num * 2; ii <= num * 3; ++ii)
        {
            *vv++ = _x + _r + cverts[ii * 2] * _r;
            *vv++ = _y + _r + cverts[ii * 2 + 1] * _r;
        }

        for (uint32_t ii = num * 3; ii < num * 4; ++ii)
        {
            *vv++ = _x + _w - _r + cverts[ii * 2] * _r;
            *vv++ = _y + _r + cverts[ii * 2 + 1] * _r;
        }

        *vv++ = _x + _w - _r + cverts[0] * _r;
        *vv++ = _y + _r + cverts[1] * _r;

        drawPolygon(verts, (num + 1) * 4, _fth, _argb);
    }

    void drawLine(float _x0, float _y0, float _x1, float _y1, float _r, uint32_t _abgr, float _fth = 1.0f)
    {
        float dx = _x1 - _x0;
        float dy = _y1 - _y0;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 0.0001f)
        {
            d = 1.0f / d;
            dx *= d;
            dy *= d;
        }

        float nx = dy;
        float ny = -dx;
        float verts[4 * 2];
        _r -= _fth;
        _r *= 0.5f;
        if (_r < 0.01f)
        {
            _r = 0.01f;
        }

        dx *= _r;
        dy *= _r;
        nx *= _r;
        ny *= _r;

        verts[0] = _x0 - dx - nx;
        verts[1] = _y0 - dy - ny;

        verts[2] = _x0 - dx + nx;
        verts[3] = _y0 - dy + ny;

        verts[4] = _x1 + dx + nx;
        verts[5] = _y1 + dy + ny;

        verts[6] = _x1 + dx - nx;
        verts[7] = _y1 + dy - ny;

        drawPolygon(verts, 4, _fth, _abgr);
    }

    struct TriangleOrientation
    {
        enum Enum
        {
            Left,
            Right,
            Up,
            Down,
        };
    };

    void drawTriangle(int32_t _x, int32_t _y, int32_t _width, int32_t _height, TriangleOrientation::Enum _orientation, uint32_t _abgr)
    {
        if (TriangleOrientation::Left == _orientation)
        {
            const float verts[3 * 2] =
            {
                (float)_x + 0.5f + (float)_width * 1.0f, (float)_y + 0.5f,
                (float)_x + 0.5f,                        (float)_y + 0.5f + (float)_height / 2.0f - 0.5f,
                (float)_x + 0.5f + (float)_width * 1.0f, (float)_y + 0.5f + (float)_height - 1.0f,
            };

            drawPolygon(verts, 3, 1.0f, _abgr);
        }
        else if (TriangleOrientation::Right == _orientation)
        {
            const float verts[3 * 2] =
            {
                (float)_x + 0.5f,                        (float)_y + 0.5f,
                (float)_x + 0.5f + (float)_width * 1.0f, (float)_y + 0.5f + (float)_height / 2.0f - 0.5f,
                (float)_x + 0.5f,                        (float)_y + 0.5f + (float)_height - 1.0f,
            };

            drawPolygon(verts, 3, 1.0f, _abgr);
        }
        else if (TriangleOrientation::Up == _orientation)
        {
            const float verts[3 * 2] =
            {
                (float)_x + 0.5f,                               (float)_y + 0.5f + (float)_height - 1.0f,
                (float)_x + 0.5f + (float)_width / 2.0f - 0.5f, (float)_y + 0.5f,
                (float)_x + 0.5f + (float)_width - 1.0f,        (float)_y + 0.5f + (float)_height - 1.0f,
            };

            drawPolygon(verts, 3, 1.0f, _abgr);
        }
        else //if (TriangleOrientation::Down == _orientation).
        {
            const float verts[3 * 2] =
            {
                (float)_x + 0.5f,                               (float)_y + 0.5f,
                (float)_x + 0.5f + (float)_width / 2.0f - 0.5f, (float)_y + 0.5f + (float)_height - 1.0f,
                (float)_x + 0.5f + (float)_width - 1.0f,        (float)_y + 0.5f,
            };

            drawPolygon(verts, 3, 1.0f, _abgr);
        }
    }


    void getBakedQuad(stbtt_bakedchar* _chardata, int32_t char_index, float* _xpos, float* _ypos, stbtt_aligned_quad* _quad)
    {
        stbtt_bakedchar* b = _chardata + char_index;
        int32_t round_x = STBTT_ifloor(*_xpos + b->xoff);
        int32_t round_y = STBTT_ifloor(*_ypos + b->yoff);

        _quad->x0 = (float)round_x;
        _quad->y0 = (float)round_y;
        _quad->x1 = (float)round_x + b->x1 - b->x0;
        _quad->y1 = (float)round_y + b->y1 - b->y0;

        _quad->s0 = (b->x0 + m_halfTexel) * m_invTextureWidth;
        _quad->t0 = (b->y0 + m_halfTexel) * m_invTextureWidth;
        _quad->s1 = (b->x1 + m_halfTexel) * m_invTextureHeight;
        _quad->t1 = (b->y1 + m_halfTexel) * m_invTextureHeight;

        *_xpos += b->xadvance;
    }

    void drawText(int32_t _x, int32_t _y, ImguiTextAlign::Enum _align, const char* _text, uint32_t _abgr)
    {
        drawText( (float)_x, (float)_y, _text, _align, _abgr);
    }

    void drawText(float _x, float _y, const char* _text, ImguiTextAlign::Enum _align, uint32_t _abgr)
    {
        if (NULL == _text
        ||  '\0' == _text[0])
        {
            return;
        }


        uint32_t numVertices = 0;
        if (_align == ImguiTextAlign::Center)
        {
            _x -= getTextLength(m_fonts[m_currentFontIdx-1].m_cdata, _text, numVertices) / 2;
        }
        else if (_align == ImguiTextAlign::Right)
        {
            _x -= getTextLength(m_fonts[m_currentFontIdx - 1].m_cdata, _text, numVertices);
        }
        else // just count vertices
        {
            getTextLength(m_fonts[m_currentFontIdx - 1].m_cdata, _text, numVertices);
        }

        {
            Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
            Ctr::IVertexBuffer* vb = uiRenderer->vertexBuffer(PosColorUvVertex::ms_decl);
            PosColorUvVertex* vertex = (PosColorUvVertex*)vb->lock(numVertices * sizeof(PosColorUvVertex));

            const float ox = _x;

            while (*_text)
            {
                int32_t ch = (uint8_t)*_text;
                if (ch == '\t')
                {
                    for (int32_t i = 0; i < 4; ++i)
                    {
                        if (_x < s_tabStops[i] + ox)
                        {
                            _x = s_tabStops[i] + ox;
                            break;
                        }
                    }
                }
                else if (ch >= ' '
                     &&  ch < 128)
                {
                    stbtt_aligned_quad quad;
                    getBakedQuad(m_fonts[m_currentFontIdx-1].m_cdata, ch - 32, &_x, &_y, &quad);

                    vertex->m_x = quad.x0;
                    vertex->m_y = quad.y0;
                    vertex->m_u = quad.s0;
                    vertex->m_v = quad.t0;
                    vertex->m_abgr = _abgr;
                    ++vertex;

                    vertex->m_x = quad.x1;
                    vertex->m_y = quad.y1;
                    vertex->m_u = quad.s1;
                    vertex->m_v = quad.t1;
                    vertex->m_abgr = _abgr;
                    ++vertex;

                    vertex->m_x = quad.x1;
                    vertex->m_y = quad.y0;
                    vertex->m_u = quad.s1;
                    vertex->m_v = quad.t0;
                    vertex->m_abgr = _abgr;
                    ++vertex;

                    vertex->m_x = quad.x0;
                    vertex->m_y = quad.y0;
                    vertex->m_u = quad.s0;
                    vertex->m_v = quad.t0;
                    vertex->m_abgr = _abgr;
                    ++vertex;

                    vertex->m_x = quad.x0;
                    vertex->m_y = quad.y1;
                    vertex->m_u = quad.s0;
                    vertex->m_v = quad.t1;
                    vertex->m_abgr = _abgr;
                    ++vertex;

                    vertex->m_x = quad.x1;
                    vertex->m_y = quad.y1;
                    vertex->m_u = quad.s1;
                    vertex->m_v = quad.t1;
                    vertex->m_abgr = _abgr;
                    ++vertex;
                }

                ++_text;
            }
            vb->unlock();

            const Ctr::GpuVariable* textureVariable = nullptr;
            if (m_textureProgram->getParameterByName("s_tex", textureVariable))
            {
                textureVariable->setTexture(m_fonts[m_currentFontIdx-1].m_texture);
            }

            uiRenderer->setVertexBuffer(vb);
            uiRenderer->device()->enableAlphaBlending();
            uiRenderer->device()->setupBlendPipeline(Ctr::BlendAlpha);
            uiRenderer->setShader(m_textureProgram);
            setCurrentScissor();
            uiRenderer->render(numVertices, 0);
            uiRenderer->device()->disableAlphaBlending();
        }
    }

    void screenQuad(int32_t _x, int32_t _y, int32_t _width, uint32_t _height, bool _originBottomLeft = false)
    {
        {
            Ctr::UIRenderer* uiRenderer = Ctr::UIRenderer::renderer();
            Ctr::IVertexBuffer* vb = uiRenderer->vertexBuffer(PosUvVertex::ms_decl);            
            PosUvVertex* vertex = (PosUvVertex*)vb->lock(6 * sizeof(PosUvVertex));

            const float widthf  = float(_width);
            const float heightf = float(_height);

            const float minx = float(_x);
            const float miny = float(_y);
            const float maxx = minx+widthf;
            const float maxy = miny+heightf;

            const float texelHalfW = m_halfTexel/widthf;
            const float texelHalfH = m_halfTexel/heightf;
            const float minu = texelHalfW;
            const float maxu = 1.0f - texelHalfW;
            const float minv = _originBottomLeft ? texelHalfH+1.0f : texelHalfH     ;
            const float maxv = _originBottomLeft ? texelHalfH      : texelHalfH+1.0f;

            vertex[0].m_x = minx;
            vertex[0].m_y = miny;
            vertex[0].m_u = minu;
            vertex[0].m_v = minv;

            vertex[1].m_x = maxx;
            vertex[1].m_y = miny;
            vertex[1].m_u = maxu;
            vertex[1].m_v = minv;

            vertex[2].m_x = maxx;
            vertex[2].m_y = maxy;
            vertex[2].m_u = maxu;
            vertex[2].m_v = maxv;

            vertex[3].m_x = maxx;
            vertex[3].m_y = maxy;
            vertex[3].m_u = maxu;
            vertex[3].m_v = maxv;

            vertex[4].m_x = minx;
            vertex[4].m_y = maxy;
            vertex[4].m_u = minu;
            vertex[4].m_v = maxv;

            vertex[5].m_x = minx;
            vertex[5].m_y = miny;
            vertex[5].m_u = minu;
            vertex[5].m_v = minv;

            vb->unlock();
            uiRenderer->setVertexBuffer(vb);
        }
    }

    void colorWheelWidget(float _rgb[3], bool _respectIndentation, float _size, bool _enabled)
    {
        const uint32_t wheelId = getId();
        const uint32_t triangleId = getId();

        Area& area = getCurrentArea();

        const float areaX = float(_respectIndentation ? area.m_widgetX : area.m_contentX);
        const float areaW = float(_respectIndentation ? area.m_widgetW : area.m_contentWidth);

        const float width = areaW*_size;
        const float xx = areaX + areaW*0.5f;
        const float yy = float(area.m_widgetY) + width*0.5f;
        const float center[2] = { xx, yy };

        area.m_widgetY += int32_t(width) + DEFAULT_SPACING;

        const float ro = width*0.5f - 5.0f; // radiusOuter.
        const float rd = _size*25.0f; // radiusDelta.
        const float ri = ro - rd; // radiusInner.
        const float aeps = 0.5f / ro; // Half a pixel arc length in radians (2pi cancels out).
        const float cmx = float(m_mx) - center[0];
        const float cmy = float(m_my) - center[1];

        const float aa[2] = { ri - 6.0f, 0.0f }; // Hue point.
        const float bb[2] = { cosf(-120.0f/180.0f*NVG_PI) * aa[0], sinf(-120.0f/180.0f*NVG_PI) * aa[0] }; // Black point.
        const float cc[2] = { cosf( 120.0f/180.0f*NVG_PI) * aa[0], sinf( 120.0f/180.0f*NVG_PI) * aa[0] }; // White point.

        const float ca[2] = { aa[0] - cc[0], aa[1] - cc[1] };
        const float lenCa = sqrtf(ca[0]*ca[0]+ca[1]*ca[1]);
        const float invLenCa = 1.0f/lenCa;
        const float dirCa[2] = { ca[0]*invLenCa, ca[1]*invLenCa };

        float sel[2];

        float hsv[3];
        Ctr::rgbToHsv(hsv, _rgb);

        const bool enabled = _enabled && isEnabled(m_areaId);
        if (enabled)
        {
            if (m_leftPressed)
            {
                const float len = sqrtf(cmx*cmx + cmy*cmy);
                if (len > ri)
                {
                    if (len < ro)
                    {
                        setActive(wheelId);
                    }
                }
                else
                {
                    setActive(triangleId);
                }
            }

            if (m_leftReleased
            && (isActive(wheelId) || isActive(triangleId) ) )
            {
                clearActive();
            }

            // Set hue.
            if (m_left
            &&  isActive(wheelId) )
            {
                hsv[0] = atan2f(cmy, cmx)/NVG_PI*0.5f;
                if (hsv[0] < 0.0f)
                {
                    hsv[0]+=1.0f;
                }
            }

        }

        if (enabled
        &&  m_left
        &&  isActive(triangleId) )
        {
            float an = -hsv[0]*NVG_PI*2.0f;
            float tmx = (cmx*cosf(an)-cmy*sinf(an) );
            float tmy = (cmx*sinf(an)+cmy*cosf(an) );

            if (pointInTriangle(tmx, tmy, aa[0], aa[1], bb[0], bb[1], cc[0], cc[1]) )
            {
                sel[0] = tmx;
                sel[1] = tmy;
            }
            else
            {
                closestPointOnTriangle(sel[0], sel[1], tmx, tmy, aa[0], aa[1], bb[0], bb[1], cc[0], cc[1]);
            }
        }
        else
        {
            /*
             *                  bb (black)
             *                  /\
             *                 /  \
             *                /    \
             *               /      \
             *              /        \
             *             /    .sel  \
             *            /            \
             *  cc(white)/____.ss_______\aa (hue)
             */
            const float ss[2] =
            {
                cc[0] + dirCa[0]*lenCa*hsv[1],
                cc[1] + dirCa[1]*lenCa*hsv[1],
            };

            const float sb[2] = { bb[0]-ss[0], bb[1]-ss[1] };
            const float lenSb = sqrtf(sb[0]*sb[0]+sb[1]*sb[1]);
            const float invLenSb = 1.0f/lenSb;
            const float dirSb[2] = { sb[0]*invLenSb, sb[1]*invLenSb };

            sel[0] = cc[0] + dirCa[0]*lenCa*hsv[1] + dirSb[0]*lenSb*(1.0f - hsv[2]);
            sel[1] = cc[1] + dirCa[1]*lenCa*hsv[1] + dirSb[1]*lenSb*(1.0f - hsv[2]);
        }

        float uu, vv, ww;
        barycentric(uu, vv, ww
                  , aa[0],  aa[1]
                  , bb[0],  bb[1]
                  , cc[0],  cc[1]
                  , sel[0], sel[1]
                  );

        const float val = Ctr::clamped(1.0f-vv, 0.0001f, 1.0f);
        const float sat = Ctr::clamped(uu/val,  0.0001f, 1.0f);

        const float out[3] = { hsv[0], sat, val };
        Ctr::hsvToRgb(_rgb, out);

        // Draw widget.
        nvgSave(m_nvg);
        {
            float saturation;
            uint8_t alpha0;
            uint8_t alpha1;
            if (enabled)
            {
                saturation = 1.0f;
                alpha0 = 255;
                alpha1 = 192;
            }
            else
            {
                saturation = 0.0f;
                alpha0 = 10;
                alpha1 = 10;
            }

            // Circle.
            for (uint8_t ii = 0; ii < 6; ii++)
            {
                const float a0 = float(ii)/6.0f      * 2.0f*NVG_PI - aeps;
                const float a1 = float(ii+1.0f)/6.0f * 2.0f*NVG_PI + aeps;
                nvgBeginPath(m_nvg);
                nvgArc(m_nvg, center[0], center[1], ri, a0, a1, NVG_CW);
                nvgArc(m_nvg, center[0], center[1], ro, a1, a0, NVG_CCW);
                nvgClosePath(m_nvg);

                const float ax = center[0] + cosf(a0) * (ri+ro)*0.5f;
                const float ay = center[1] + sinf(a0) * (ri+ro)*0.5f;
                const float bx = center[0] + cosf(a1) * (ri+ro)*0.5f;
                const float by = center[1] + sinf(a1) * (ri+ro)*0.5f;
                NVGpaint paint = nvgLinearGradient(m_nvg
                                                 , ax, ay
                                                 , bx, by
                                                 , nvgHSLA(a0/NVG_PI*0.5f,saturation,0.55f,alpha0)
                                                 , nvgHSLA(a1/NVG_PI*0.5f,saturation,0.55f,alpha0)
                                                 );

                nvgFillPaint(m_nvg, paint);
                nvgFill(m_nvg);
            }

            // Circle stroke.
            nvgBeginPath(m_nvg);
            nvgCircle(m_nvg, center[0], center[1], ri-0.5f);
            nvgCircle(m_nvg, center[0], center[1], ro+0.5f);
            nvgStrokeColor(m_nvg, nvgRGBA(0,0,0,64) );
            nvgStrokeWidth(m_nvg, 1.0f);
            nvgStroke(m_nvg);

            nvgSave(m_nvg);
            {
                // Hue selector.
                nvgTranslate(m_nvg, center[0], center[1]);
                nvgRotate(m_nvg, hsv[0]*NVG_PI*2.0f);
                nvgStrokeWidth(m_nvg, 2.0f);
                nvgBeginPath(m_nvg);
                nvgRect(m_nvg, ri-1.0f,-3.0f,rd+2.0f,6.0f);
                nvgStrokeColor(m_nvg, nvgRGBA(255,255,255,alpha1) );
                nvgStroke(m_nvg);

                // Hue selector drop shadow.
                NVGpaint paint = nvgBoxGradient(m_nvg, ri-3.0f,-5.0f,ro-ri+6.0f,10.0f, 2.0f,4.0f, nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0) );
                nvgBeginPath(m_nvg);
                nvgRect(m_nvg, ri-2.0f-10.0f,-4.0f-10.0f,ro-ri+4.0f+20.0f,8.0f+20.0f);
                nvgRect(m_nvg, ri-2.0f,-4.0f,ro-ri+4.0f,8.0f);
                nvgPathWinding(m_nvg, NVG_HOLE);
                nvgFillPaint(m_nvg, paint);
                nvgFill(m_nvg);

                // Center triangle stroke.
                nvgBeginPath(m_nvg);
                nvgMoveTo(m_nvg, aa[0], aa[1]);
                nvgLineTo(m_nvg, bb[0], bb[1]);
                nvgLineTo(m_nvg, cc[0], cc[1]);
                nvgClosePath(m_nvg);
                nvgStrokeColor(m_nvg, nvgRGBA(0,0,0,64) );
                nvgStroke(m_nvg);

                // Center triangle fill.
                paint = nvgLinearGradient(m_nvg, aa[0], aa[1], bb[0], bb[1], nvgHSL(hsv[0],saturation,0.5f), nvgRGBA(0,0,0,alpha0) );
                nvgFillPaint(m_nvg, paint);
                nvgFill(m_nvg);
                paint = nvgLinearGradient(m_nvg, (aa[0]+bb[0])*0.5f, (aa[1]+bb[1])*0.5f, cc[0], cc[1], nvgRGBA(0,0,0,0), nvgRGBA(255,255,255,alpha0) );
                nvgFillPaint(m_nvg, paint);
                nvgFill(m_nvg);

                // Color selector.
                nvgStrokeWidth(m_nvg, 2.0f);
                nvgBeginPath(m_nvg);
                nvgCircle(m_nvg, sel[0], sel[1], 5);
                nvgStrokeColor(m_nvg, nvgRGBA(255,255,255,alpha1) );
                nvgStroke(m_nvg);

                // Color selector stroke.
                paint = nvgRadialGradient(m_nvg, sel[0], sel[1], 7.0f, 9.0f, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0) );
                nvgBeginPath(m_nvg);
                nvgRect(m_nvg, sel[0]-20.0f, sel[1]-20.0f, 40.0f, 40.0f);
                nvgCircle(m_nvg, sel[0], sel[1], 7.0f);
                nvgPathWinding(m_nvg, NVG_HOLE);
                nvgFillPaint(m_nvg, paint);
                nvgFill(m_nvg);
            }
            nvgRestore(m_nvg);
        }
        nvgRestore(m_nvg);
    }

    struct Area
    {
        int32_t m_x;
        int32_t m_y;
        int32_t m_width;
        int32_t m_height;
        int16_t m_contentX;
        int16_t m_contentY;
        int16_t m_contentWidth;
        int16_t m_contentHeight;
        int16_t m_scissorX;
        int16_t m_scissorY;
        int16_t m_scissorHeight;
        int16_t m_scissorWidth;
        int32_t m_widgetX;
        int32_t m_widgetY;
        int32_t m_widgetW;
        int32_t* m_scrollVal;
        uint32_t m_scrollId;
        bool m_inside;
        bool m_didScroll;
        bool m_scissorEnabled;
    };

    inline Area& getCurrentArea()
    {
        return m_areas[m_areaId];
    }

    inline void setCurrentScissor()
    {
        const Area& area = getCurrentArea();
        if (area.m_scissorEnabled)
        {
            m_Device->setScissorEnabled(true);
            m_Device->setScissorRect(uint16_t(IMGUI_MAX(0, area.m_scissorX))
                           , uint16_t(IMGUI_MAX(0, area.m_scissorY-1) )
                           , area.m_scissorWidth
                           , area.m_scissorHeight+1
                           );
        }
        else
        {
            m_Device->setScissorEnabled(false);
            m_Device->setScissorRect(0, 0, m_Device->backbuffer()->width(), m_Device->backbuffer()->height());
        }
    }

    template <typename Ty, uint16_t Max=64>
    struct IdStack
    {
        IdStack()
        {
            reset();
        }

        void reset()
        {
            m_current = 0;
            m_idGen   = 0;
            m_ids[0]  = 0;
        }

        void next()
        {
            m_ids[++m_current] = ++m_idGen;
        }

        void pop()
        {
            m_current = m_current > 0 ? m_current-1 : 0;
        }

        Ty current() const
        {
            return m_ids[m_current];
        }

        operator Ty() const
        {
            return m_ids[m_current];
        }

    private:
        uint16_t m_current;
        Ty m_idGen;
        Ty m_ids[Max];
    };

    int32_t m_mx;
    int32_t m_my;
    int32_t m_scroll;
    uint32_t m_active;
    uint32_t m_hot;
    uint32_t m_hotToBe;
    float    m_KeyRepeatTimes[512];
    char m_char;
    char m_lastChar;
    uint32_t m_inputField;
    int32_t m_dragX;
    int32_t m_dragY;
    float m_dragOrig;
    bool m_left;
    bool m_leftPressed;
    bool m_leftReleased;
    bool m_isHot;
    bool m_wentActive;
    bool m_insideArea;
    bool m_isActivePresent;
    bool m_checkActivePresence;

    IdStack<uint16_t> m_areaId;
    uint16_t m_widgetId;
    uint64_t m_enabledAreaIds;
    Area m_areas[64];

    float m_tempCoords[MAX_TEMP_COORDS * 2];
    float m_tempNormals[MAX_TEMP_COORDS * 2];

    float m_circleVerts[NUM_CIRCLE_VERTS * 2];

    uint16_t m_textureWidth;
    uint16_t m_textureHeight;
    float m_invTextureWidth;
    float m_invTextureHeight;
    float m_halfTexel;

    NVGcontext* m_nvg;
    uint8_t  m_view;
    uint16_t m_viewWidth;
    uint16_t m_viewHeight;

    struct Font
    {
        stbtt_bakedchar m_cdata[96]; // ASCII 32..126 is 95 glyphs
        Ctr::ITexture*  m_texture;
        float           m_size;
        uint32_t        m_id;
    };

    ImguiFontReference m_currentFontIdx;
    ImguiFontReference m_fontHandle[IMGUI_CONFIG_MAX_FONTS];
    Font m_fonts[IMGUI_CONFIG_MAX_FONTS];

    Ctr::GpuVariable* u_imageLodEnabled;
    Ctr::GpuVariable* u_imageSwizzle;
    Ctr::GpuVariable* s_texColor;
    const Ctr::IShader* m_colorProgram;
    const Ctr::IShader* m_textureProgram;
    const Ctr::IShader* m_cubeMapProgram;
    const Ctr::IShader* m_imageProgram;
    const Ctr::IShader* m_imageSwizzProgram;
    Ctr::ITexture*      m_missingTexture;

    Ctr::IDevice*       m_Device;
};

static Imgui s_imgui;

ImguiFontReference imguiCreate(Ctr::IDevice* _device, const void* _data, uint32_t _size, float _fontSize)
{
    return s_imgui.create(_device, _data, _size, _fontSize);
}

void imguiDestroy()
{
    s_imgui.destroy();
}

ImguiFontReference imguiCreateFont(const void* _data, float _fontSize)
{
    return s_imgui.createFont(_data, _fontSize);
}

void imguiSetFont(ImguiFontReference _handle)
{
    s_imgui.setFont(_handle);
}

ImguiFontReference imguiGetCurrentFont()
{
    const ImguiFontReference handle = { s_imgui.m_currentFontIdx };
    return handle;
}

void imguiBeginFrame(Ctr::InputState* inputState, int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, uint16_t _width, uint16_t _height, char _inputChar, uint8_t _view)
{
    s_imgui.beginFrame(inputState, _mx, _my, _button, _scroll, _width, _height, _inputChar, _view);
}

void imguiEndFrame()
{
    s_imgui.endFrame();

}

void imguiDrawText(int32_t _x, int32_t _y, ImguiTextAlign::Enum _align, const char* _text, uint32_t _argb)
{
    s_imgui.drawText(_x, _y, _align, _text, _argb);
}

void imguiDrawLine(float _x0, float _y0, float _x1, float _y1, float _r, uint32_t _argb)
{
    s_imgui.drawLine(_x0, _y0, _x1, _y1, _r, _argb);
}

void imguiDrawRoundedRect(float _x, float _y, float _width, float _height, float _r, uint32_t _argb)
{
    s_imgui.drawRoundedRect(_x, _y, _width, _height, _r, _argb);
}

void imguiDrawRect(float _x, float _y, float _width, float _height, uint32_t _argb)
{
    s_imgui.drawRect(_x, _y, _width, _height, _argb);
}

bool imguiBorderButton(ImguiBorder::Enum _border, bool _checked, bool _enabled)
{
    return s_imgui.borderButton(_border, _checked, _enabled);
}

bool imguiBeginArea(const char* _name, int _x, int _y, int _width, int _height, bool _enabled, int32_t _r)
{
    return s_imgui.beginArea(_name, _x, _y, _width, _height, _enabled, _r);
}

void imguiEndArea()
{
    return s_imgui.endArea();
}

bool imguiBeginScroll(int32_t _height, int32_t* _scroll, bool _enabled)
{
    return s_imgui.beginScroll(_height, _scroll, _enabled);
}

void imguiEndScroll(int32_t _r)
{
    s_imgui.endScroll(_r);
}

bool imguiBeginScrollArea(const char* _name, int32_t _x, int32_t _y, int32_t _width, int32_t _height, int32_t* _scroll, bool _enabled, int32_t _r)
{
    const bool result = s_imgui.beginArea(_name, _x, _y, _width, _height, _enabled, _r);
    const bool hasTitle = (NULL != _name && '\0' != _name[0]);
    const int32_t margins = int32_t(hasTitle)*(AREA_HEADER+2*SCROLL_AREA_PADDING-1);
    s_imgui.beginScroll(_height - margins, _scroll, _enabled);
    return result;
}

void imguiEndScrollArea(int32_t _r)
{
    s_imgui.endScroll(_r);
    s_imgui.endArea();
}

void imguiIndent(uint16_t _width)
{
    s_imgui.indent(_width);
}

void imguiUnindent(uint16_t _width)
{
    s_imgui.unindent(_width);
}

void imguiSeparator(uint16_t _height)
{
    s_imgui.separator(_height);
}

void imguiSeparatorLine(uint16_t _height)
{
    s_imgui.separatorLine(_height);
}

int32_t imguiGetWidgetX()
{
    return s_imgui.getCurrentArea().m_widgetX;
}

int32_t imguiGetWidgetY()
{
    return s_imgui.getCurrentArea().m_widgetY;
}

int32_t imguiGetWidgetW()
{
    return s_imgui.getCurrentArea().m_widgetW;
}

void imguiSetCurrentScissor()
{
    return s_imgui.setCurrentScissor();
}

bool imguiButton(const char* _text, bool _enabled, ImguiAlign::Enum _align, uint32_t _rgb0, int32_t _r)
{
    return s_imgui.button(_text, _enabled, _align, _rgb0, _r);
}

bool imguiItem(const char* _text, bool _enabled)
{
    return s_imgui.item(_text, _enabled);
}

bool imguiCheck(const char* _text, bool _checked, bool _enabled)
{
    return s_imgui.check(_text, _checked, _enabled);
}

void imguiBool(const char* _text, bool& _flag, bool _enabled)
{
    if (imguiCheck(_text, _flag, _enabled) )
    {
        _flag = !_flag;
    }
}

bool imguiCollapse(const char* _text, const char* _subtext, bool _checked, bool _enabled)
{
    return s_imgui.collapse(_text, _subtext, _checked, _enabled);
}

void imguiLabel(const char* _format, ...)
{
    va_list argList;
    va_start(argList, _format);
    s_imgui.labelVargs(_format, argList, imguiRGBA(255, 255, 255, 255) );
    va_end(argList);
}

void imguiLabel(uint32_t _rgba, const char* _format, ...)
{
    va_list argList;
    va_start(argList, _format);
    s_imgui.labelVargs(_format, argList, _rgba);
    va_end(argList);
}

void imguiValue(const char* _text)
{
    s_imgui.value(_text);
}

bool imguiSlider(const char* _text, float& _val, float _vmin, float _vmax, float _vinc, bool _enabled, ImguiAlign::Enum _align)
{
    return s_imgui.slider(_text, _val, _vmin, _vmax, _vinc, _enabled, _align);
}

bool imguiSlider(const char* _text, int32_t& _val, int32_t _vmin, int32_t _vmax, bool _enabled, ImguiAlign::Enum _align)
{
    float val = (float)_val;
    bool result = s_imgui.slider(_text, val, (float)_vmin, (float)_vmax, 1.0f, _enabled, _align);
    _val = (int32_t)val;
    return result;
}

void imguiInput(const char* _label, char* _str, uint32_t _len, bool _enabled, ImguiAlign::Enum _align, int32_t _r)
{
    s_imgui.input(_label, _str, _len, _enabled, _align, _r);
}

uint8_t imguiTabsForEnum(uint8_t _selected, bool _enabled, ImguiAlign::Enum _align, int32_t _height, int32_t _r, uint8_t _nTabs, const ImguiEnumVal* labels)
{
    const uint8_t result = s_imgui.tabs(_selected, _enabled, _align, _height, _r, _nTabs, labels);
    return result;
}


uint32_t imguiChooseUseMacroInstead(uint32_t _selected, ...)
{
    va_list argList;
    va_start(argList, _selected);

    const char* str = va_arg(argList, const char*);
    for (uint32_t ii = 0; str != NULL; ++ii, str = va_arg(argList, const char*) )
    {
        if (imguiCheck(str, ii == _selected) )
        {
            _selected = ii;
        }
    }

    va_end(argList);

    return _selected;
}

uint32_t imguiChooseFromArrayInstead(uint32_t _selected, const ImguiEnumVal* chooseFrom, uint32_t enumCount)
{
    for (uint32_t ii = 0; ii < enumCount; ii++)
    {
        const char* str = chooseFrom[ii].label;
        if (imguiCheck(str, ii == _selected))
        {
            _selected = ii;
        }
    }

    return _selected;
}

void imguiColorWheel(float* _rgb, bool _respectIndentation, float _size, bool _enabled)
{
    s_imgui.colorWheelWidget(_rgb, _respectIndentation, _size, _enabled);
}

void imguiColorWheel(const char* _text, float* _rgb, bool& _activated, float _size, bool _enabled)
{
    char buf[128];
    sprintf(buf,  "[RGB %-2.2f %-2.2f %-2.2f]"
        , _rgb[0]
        , _rgb[1]
        , _rgb[2]
        );

    if (imguiCollapse(_text, buf, _activated) )
    {
        _activated = !_activated;
    }

    if (_activated)
    {
        imguiColorWheel(_rgb, false, _size, _enabled);
    }
}

bool imguiImage(const Ctr::ITexture* _image, float _lod, int32_t _width, int32_t _height, ImguiAlign::Enum _align, bool _enabled, bool _originBottomLeft)
{
    return s_imgui.image(_image, _lod, _width, _height, _align, _enabled, _originBottomLeft);
}

bool imguiImage(const Ctr::ITexture* _image, float _lod, float _width, float _aspect, ImguiAlign::Enum _align, bool _enabled, bool _originBottomLeft)
{
    return s_imgui.image(_image, _lod, _width, _aspect, _align, _enabled, _originBottomLeft);
}

bool imguiImageChannel(const Ctr::ITexture* _image, uint8_t _channel, float _lod, int32_t _width, int32_t _height, ImguiAlign::Enum _align, bool _enabled)
{
    return s_imgui.imageChannel(_image, _channel, _lod, _width, _height, _align, _enabled);
}

bool imguiImageChannel(const Ctr::ITexture* _image, uint8_t _channel, float _lod, float _width, float _aspect, ImguiAlign::Enum _align, bool _enabled)
{
    return s_imgui.imageChannel(_image, _channel, _lod, _width, _aspect, _align, _enabled);
}

bool imguiCube(const Ctr::ITexture* _cubemap, float _lod, bool _cross, ImguiAlign::Enum _align, bool _enabled)
{
    return s_imgui.cubeMap(_cubemap, _lod, _cross, _align, _enabled);
}

float imguiGetTextLength(const char* _text, ImguiFontReference _handle)
{
    uint32_t numVertices = 0; //unused
    return getTextLength(s_imgui.m_fonts[_handle].m_cdata, _text, numVertices);
}

bool imguiMouseOverArea()
{
    return s_imgui.m_insideArea;
}
