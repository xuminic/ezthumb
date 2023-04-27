/***************************************************************************
 * wnd.h is part of Math Graphic Library
 * Copyright (C) 2007-2016 Alexey Balakin <mathgl.abalakin@gmail.ru>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef _MGL_WND_H_
#define _MGL_WND_H_

#include "mgl2/mgl.h"
//-----------------------------------------------------------------------------
MGL_EXPORT void *mgl_draw_calc(void *p);
//-----------------------------------------------------------------------------
/// Class for drawing in windows (like, mglCanvasFL, mglCanvasQT and so on)
/// Make inherited class and redefine Draw() function if you don't want to use function pointers.
class MGL_EXPORT mglDraw
{
public:
	virtual int Draw(mglGraph *)=0;	///< Function for drawing
	virtual void Reload(){}		///< Function for reloading data
	virtual void Click() {}		///< Callback function on mouse click
#if MGL_HAVE_PTHR_WIDGET
	mglDraw()	{	running=false;	pthread_mutex_init(&mutex,NULL);	}
	virtual ~mglDraw()	{	pthread_mutex_destroy(&mutex);	}

	virtual void Calc()	{}		///< Function for calculations
	inline void Run()			///< Run/resume calculation in other thread
	{
		if(!running)
		{	pthread_mutex_trylock(&mutex);	pthread_mutex_unlock(&mutex);
			pthread_create(&thr,0,mgl_draw_calc,this);
			pthread_detach(thr);	running = true;	}
	}
	inline void Cancel()		///< Cancel thread with calculations
	{	pthread_cancel(thr);	running = false;	}
	inline void Pause()			///< Pause calculation
	{	pthread_mutex_lock(&mutex);	}
	inline void Continue()		///< Continue calculation
	{	pthread_mutex_trylock(&mutex);	pthread_mutex_unlock(&mutex);	}
	inline void Check()			///< Check if calculation can be continued (should be called inside Calc() )
	{	pthread_mutex_lock(&mutex);	pthread_mutex_unlock(&mutex);	}
//protected:
	pthread_t thr;
	bool running;
	pthread_mutex_t mutex;

#else
	mglDraw() {}
	virtual ~mglDraw() {}
#endif
};
//-----------------------------------------------------------------------------
extern "C" {
int MGL_EXPORT mgl_draw_graph(HMGL gr, void *p);
// NOTE: MGL_EXPORT mgl_draw_class() and MGL_EXPORT mgl_draw_load() use mglWindow* only. Don't use it with inherited classes
int MGL_EXPORT mgl_draw_class(HMGL gr, void *p);
void MGL_EXPORT mgl_click_class(void *p);
void MGL_EXPORT mgl_reload_class(void *p);
}
//-----------------------------------------------------------------------------
/// Abstract class for windows displaying graphics
class MGL_EXPORT mglWnd : public mglGraph
{
	mglWnd(const mglWnd &) {}	// copying is not allowed
	const mglWnd &operator=(const mglWnd &t)	{	return t;	}
public:
	mglWnd() : mglGraph(-1)	{}
	virtual ~mglWnd() {	mgl_use_graph(gr,-255);	}
	virtual int Run()=0;		///< Run main loop for event handling
	/// Return pointer to widget used for plotting
	virtual void *Widget()	{	return NULL;	}

	inline void ToggleAlpha()	///< Switch on/off transparency (do not overwrite user settings)
	{	mgl_wnd_toggle_alpha(gr);	}
	inline void ToggleLight()	///< Switch on/off lighting (do not overwrite user settings)
	{	mgl_wnd_toggle_light(gr);	}
	inline void ToggleZoom()	///< Switch on/off zooming by mouse
	{	mgl_wnd_toggle_zoom(gr);	}
	inline void ToggleRotate()	///< Switch on/off rotation by mouse
	{	mgl_wnd_toggle_rotate(gr);	}
	inline void ToggleNo()		///< Switch off all zooming and rotation
	{	mgl_wnd_toggle_no(gr);	}
	inline void Update()		///< Update picture by calling user drawing function
	{	mgl_wnd_update(gr);	}
	inline void ReLoad()		///< Reload user data and update picture
	{	mgl_wnd_reload(gr);	}
	inline void Adjust()		///< Adjust size of bitmap to window size
	{	mgl_wnd_adjust(gr);	}
	inline void NextFrame()		///< Show next frame (if one)
	{	mgl_wnd_next_frame(gr);	}
	inline void PrevFrame()		///< Show previous frame (if one)
	{	mgl_wnd_prev_frame(gr);	}
	inline void Animation()		///< Run slideshow (animation) of frames
	{	mgl_wnd_animation(gr);	}
	inline void SetClickFunc(void (*func)(void *p))	///< Callback function for mouse click
	{	mgl_set_click_func(gr,func);	}
	/// Set callback functions for drawing and data reloading
	inline void SetDrawFunc(int (*draw)(mglBase *gr, void *p), void *par=NULL, void (*reload)(void *p)=NULL)
	{	mgl_wnd_set_func(gr,draw,par,reload);	}
	inline void SetDrawFunc(int (*draw)(mglGraph *gr))
	{	mgl_wnd_set_func(gr,draw?mgl_draw_graph:0,(void*)draw,0);	}
	inline void SetDrawFunc(mglDraw *draw)
	{	mgl_wnd_set_func(gr,draw?mgl_draw_class:0,draw,mgl_reload_class);
#if MGL_HAVE_PTHR_WIDGET
		mgl_wnd_set_mutex(gr, &(draw->mutex));
#endif
		mgl_set_click_func(gr, mgl_click_class);	}
#if MGL_HAVE_PTHR_WIDGET
	/// Mutex for lock/unlock by widget
	inline void SetMutex(pthread_mutex_t *mutex)
	{	mgl_wnd_set_mutex(gr, mutex);	}
#endif

	inline void SetDelay(double dt)	///< Set delay for animation in seconds
	{	mgl_wnd_set_delay(gr, dt);	}
	inline double GetDelay()		///< Get delay for animation in seconds
	{	return mgl_wnd_get_delay(gr);	}
	inline void Setup(bool clf_upd=true, bool showpos=false)
	{	mgl_setup_window(gr, clf_upd, showpos);	}
	inline mglPoint LastMousePos()		///< Last mouse position
	{	mreal x,y,z;	mgl_get_last_mouse_pos(gr,&x,&y,&z);	return mglPoint(x,y,z);	}
};
//-----------------------------------------------------------------------------
#endif
