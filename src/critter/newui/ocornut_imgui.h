/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef OCORNUT_IMGUI_H_HEADER_GUARD
#define OCORNUT_IMGUI_H_HEADER_GUARD

#include <stdint.h>
#include <imgui.h>

namespace Ctr
{
class InputState;
}

void IMGUI_create(const void* _data, uint32_t _size, float _fontSize);
void IMGUI_destroy();
void IMGUI_beginFrame(Ctr::InputState* inputState, int32_t _mx, int32_t _my, uint8_t _button, int _width, int _height, char _inputChar, uint8_t _viewId);
void IMGUI_endFrame();

#endif // OCORNUT_IMGUI_H_HEADER_GUARD
