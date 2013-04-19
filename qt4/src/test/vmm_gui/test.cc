/*
 * \brief  Nitpicker test program
 * \author Norman Feske
 * \date   2006-08-23
 */

/*
 * Copyright (C) 2006-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/env.h>
#include <util/list.h>
#include <base/sleep.h>
#include <base/printf.h>
#include <nitpicker_session/connection.h>
#include <nitpicker_view/client.h>
#include <timer_session/connection.h>
#include <input/event.h>

#include <vmm_gui_session/connection.h>

using namespace Genode;

class Test_view : public List<Test_view>::Element
{
	private:

		Nitpicker::View_capability  _cap;
		int                         _x, _y, _w, _h;
		const char                 *_title;

	public:

		Test_view(Nitpicker::Session *nitpicker,
		          int x, int y, int w, int h,
		          const char *title)
		:
			_x(x), _y(y), _w(w), _h(h), _title(title)
		{
			using namespace Nitpicker;

			_cap = nitpicker->create_view();
			View_client(_cap).viewport(_x, _y, _w, _h, 0, 0, true);
			View_client(_cap).stack(Nitpicker::View_capability(), true, true);
			View_client(_cap).title(_title);
		}

		void top()
		{
			Nitpicker::View_client(_cap).stack(Nitpicker::View_capability(), true, true);
		}

		void move(int x, int y)
		{
			_x = x;
			_y = y;
			Nitpicker::View_client(_cap).viewport(_x, _y, _w, _h, 0, 0, true);
		}

		/**
		 * Accessors
		 */
		const char *title() { return _title; }
		int         x()     { return _x; }
		int         y()     { return _y; }
		int         w()     { return _w; }
		int         h()     { return _h; }
		Nitpicker::View_capability cap() { return _cap; }
};


int main(int argc, char **argv)
{
	/*
	 * Init sessions to the required external services
	 */
	enum { CONFIG_ALPHA = false };
	static Nitpicker::Connection nitpicker(256, 256, CONFIG_ALPHA);
	static Timer::Connection     timer;

	Framebuffer::Mode const mode = nitpicker.framebuffer()->mode();
	int const scr_w = mode.width(), scr_h = mode.height();

	printf("screen is %dx%d\n", scr_w, scr_h);
	if (!scr_w || !scr_h) {
		PERR("Got invalid screen - spinning");
		sleep_forever();
	}

	short *pixels = env()->rm_session()->attach(nitpicker.framebuffer()->dataspace());
	unsigned char *alpha = (unsigned char *)&pixels[scr_w*scr_h];
	unsigned char *input_mask = CONFIG_ALPHA ? alpha + scr_w*scr_h : 0;

	Input::Event *ev_buf = (env()->rm_session()->attach(nitpicker.input()->dataspace()));

	/*
	 * Paint some crap into pixel buffer, fill alpha channel and input-mask buffer
	 *
	 * Input should refer to the view if the alpha value is more than 50%.
	 */
	for (int i = 0; i < scr_h; i++)
		for (int j = 0; j < scr_w; j++) {
			pixels[i*scr_w + j] = (i/8)*32*64 + (j/4)*32 + i*j/256;
			if (CONFIG_ALPHA) {
				alpha[i*scr_w + j] = (i*2) ^ (j*2);
				input_mask[i*scr_w + j] = alpha[i*scr_w + j] > 127;
			}
		}


	Test_view tv(&nitpicker, 0, 0, 256, 188, "Eins");

	Vmm_gui::Connection gui;

	Signal_receiver sig_rcv;
	Signal_context  play_context;
	Signal_context  stop_context;
	Signal_context  bomb_context;
	Signal_context  power_context;
	Signal_context  fs_context;

	Cpu_state_modes state;
	state.ip=0x80808080;
	state.sp=0x40001000;

	gui.play_resume_sigh(sig_rcv.manage(&play_context));
	gui.stop_sigh(sig_rcv.manage(&stop_context));
	gui.bomb_sigh(sig_rcv.manage(&bomb_context));
	gui.power_sigh(sig_rcv.manage(&power_context));
	gui.fullscreen_sigh(sig_rcv.manage(&fs_context));
	gui.show_view(tv.cap(), 256, 188);
	gui.set_state(&state);

	while (true) {
		Signal s = sig_rcv.wait_for_signal();
		if (s.context() == &play_context) {
			PDBG("PLAY");
		} else if (s.context() == &stop_context) {
			PDBG("STOP");
		} else if (s.context() == &bomb_context) {
			PDBG("BOMB");
		} else if (s.context() == &power_context) {
			PDBG("POWER");
		} else if (s.context() == &fs_context) {
			PDBG("FULLSCREEN");
		} else {
			PWRN("Invalid context");
		}
	}

	Genode::sleep_forever();
	return 0;
}
