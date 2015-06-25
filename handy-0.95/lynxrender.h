//
// Copyright (c) 2004 K. Wilkins
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//

//
// Base class for rendering subsystem
//

#ifndef _LYNXRENDER_H
#define _LYNXRENDER_H

enum {	RENDER_ERROR,RENDER_8BPP,RENDER_16BPP_555,RENDER_16BPP_565,RENDER_24BPP,RENDER_32BPP };

class CLynxRender 
{
    public:
		CLynxRender() {};
		virtual ~CLynxRender() {};

        virtual bool Create (CWnd *pcwnd,int src_width, int src_height, int scrn_width, int scrn_height,int zoom)
		{
			mWidth=scrn_width;
			mHeight=scrn_height;
			mSrcWidth=src_width;
			mSrcHeight=src_height;
			mZoom=zoom;
			return 1;
		};
        virtual bool   Destroy (CWnd *pcwnd) { return 1; }; 
		virtual bool   Render (int dest_x,int dest_y) { return 1; };
		virtual UBYTE* BackBuffer(void) { return NULL; };
		virtual int    BufferPitch(void) { return 0; };
		virtual int    PixelFormat(void) { return RENDER_ERROR; };
		virtual int    Windowed(void) { return TRUE; }

		int mWidth;
		int mHeight;
		int	mSrcWidth;
		int	mSrcHeight;
		int mZoom;
};

#endif