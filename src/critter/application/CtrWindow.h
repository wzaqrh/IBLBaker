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

#ifndef INCLUDED_WINDOW
#define INCLUDED_WINDOW

#include <CtrPlatform.h>
#include <CtrRegion.h>

namespace Ctr
{
class Application;
class Window;

class WindowData
{
  public:
    WindowData (WindowHandle window, UINT msg, WPARAM wparam, LPARAM lparam);
    WindowHandle               _window;
    UINT                       _msg;
    WPARAM                     _wparam; 
    LPARAM                     _lparam;
};

class WindowDependency
{
  public:
    WindowDependency();
    virtual ~WindowDependency();
    virtual bool               handleWindowChanged(Ctr::WindowData* data) = 0;
};


class Window
{
  public:
    Window(const Application*);
    virtual ~Window();

    void                       addWindowDependency(WindowDependency* dependency);
    void                       removeWindowDependency(WindowDependency* dependency);

    virtual bool               show() { return false; }

    virtual bool               create(const std::string& name,
                                      bool windowed = true,
                                      int width = 1024,
                                      int height = 768,
                                      bool headless = false);
    
    bool                       created() const;

    bool                       destroy();
    const WindowHandle         windowHandle() const;

    void                       setShowCursor(bool);
    bool                       showCursor() const;

    bool                       windowed() const;

    int                        width() const;
    void                       setWidth(int);

    int                        height() const;
    void                       setHeight(int height);

    const Application*         application() const;

    const std::string&         windowName() const;

    operator const WindowHandle() const { return windowHandle(); }

    const Ctr::Region2i &         windowBounds() const;
    void                       updateBounds();
    const Ctr::Region2i &         clientRect() const;

    bool                       hasFocus() const;

    Ctr::Vector2i              mousePosition() const;

  protected:
    void                       sendWindowChanged (Ctr::WindowData* windowData);
    virtual void               handleSizeChanged();

    void                       setWindowHandle (WindowHandle);

    static LRESULT CALLBACK     windowProc (HWND window, 
                                           UINT msg, 
                                           WPARAM wParam, 
                                           LPARAM lParam);

    virtual LRESULT               callback (HWND window, 
                                         UINT msg, 
                                         WPARAM wParam, 
                                         LPARAM lParam);
  protected:
    const Application *        _application;
    WindowHandle               _window;
    std::string                _windowName;
    int                        _width;
    int                        _height;
    bool                       _windowed;
    bool                       _showCursor;
    Ctr::Region2i              _windowBounds;
    bool                       _resizing;
    Ctr::Region2i              _clientRect;
    bool                       _headless;

    typedef std::vector<WindowDependency*> DependencyVector;
    DependencyVector           _dependencies;
};
}
#endif
