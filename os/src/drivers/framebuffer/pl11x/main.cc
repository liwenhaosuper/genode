/*
 * \brief  PL11x frame-buffer driver
 * \author Norman Feske
 * \author Stefan Kalkowski
 * \date   2010-02-17
 */

/*
 * Copyright (C) 2010-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/printf.h>
#include <base/sleep.h>
#include <base/rpc_server.h>
#include <io_mem_session/connection.h>
#include <cap_session/connection.h>
#include <dataspace/client.h>
#include <framebuffer_session/framebuffer_session.h>
#include <root/component.h>

/* device configuration */
#include <drivers/board.h>
#include <drivers/framebuffer/pl11x.h>
#include <drivers/framebuffer/modes.h>
#include <video_memory.h>


/***********************************************
 ** Implementation of the framebuffer service **
 ***********************************************/

using namespace Framebuffer::Vga_60hz;

namespace Framebuffer
{
	enum {
		BPP     = 2,
		FB_SIZE = SCR_WIDTH * SCR_HEIGHT * BPP,
	};


	class Session_component : public Genode::Rpc_object<Session>,
	                          public Pl11x
	{
		private:

			Genode::Dataspace_capability _fb_ds;

		public:

			/**
			 * Constructor
			 */
			Session_component(void *regs_base,
			                  Genode::Dataspace_capability fb_cap)
			: Pl11x((Genode::addr_t)regs_base), _fb_ds(fb_cap)
			{
				/* reset video if already enabled */
				write<Lcd_control::Lcd_pwr>(0);
				write<Lcd_control::Lcd_en>(0);

				//TODO OSC CONTROL

				/* init video timing */
				write<Lcd_timing_0>(Lcd_timing_0::Ppl::bits(SCR_WIDTH/16 - 1) |
				                    Lcd_timing_0::Hsw::bits(HSYNC_LEN - 1)    |
				                    Lcd_timing_0::Hfp::bits(RIGHT_MARGIN - 1) |
				                    Lcd_timing_0::Hbp::bits(LEFT_MARGIN - 1));
				write<Lcd_timing_1>(Lcd_timing_1::Lpp::bits(SCR_HEIGHT - 1)   |
				                    Lcd_timing_1::Vsw::bits(VSYNC_LEN - 1)    |
				                    Lcd_timing_1::Vfp::bits(LOWER_MARGIN - 1) |
				                    Lcd_timing_1::Vbp::bits(UPPER_MARGIN - 1));
				write<Lcd_timing_2>(Lcd_timing_2::Cpl::bits(SCR_WIDTH - 1) |
				                    Lcd_timing_2::Ivs::bits(1) |
				                    Lcd_timing_2::Ihs::bits(1) |
				                    Lcd_timing_2::Bcd::bits(1));
				write<Lcd_timing_3>(0);

				/* set framebuffer address */
				write<Lcd_up_base>(Genode::Dataspace_client(_fb_ds).phys_addr());
				write<Lcd_lp_base>(0);

				/* no IRQs */
				write<Lcd_imsc>(0);

				/* prepare control register */
				write<Lcd_control>(
					Lcd_control::Bgr::bits(1)       |
					Lcd_control::Lcd_en::bits(1)    |
					Lcd_control::Lcd_tft::bits(1)   |
					Lcd_control::Lcd_v_cmp::bits(1) |
					Lcd_control::Lcd_bpp::bits(Lcd_control::Lcd_bpp::BPP_16_565));

				/* power on */
				write<Lcd_control::Lcd_pwr>(0);
			}

			Genode::Dataspace_capability dataspace() { return _fb_ds; }

			void release() { }

			Mode mode() const {
				return Mode(SCR_WIDTH, SCR_HEIGHT, Mode::RGB565); }

			void mode_sigh(Genode::Signal_context_capability) { }

			void refresh(int x, int y, int w, int h) { }
	};


	class Root : public Genode::Root_component<Session_component>
	{
		private:

			void                        *_regs_base;
			Genode::Dataspace_capability _fb_ds_cap;

		protected:

			Session_component *_create_session(const char *args) {
				return new (md_alloc()) Session_component(_regs_base,
				                                          _fb_ds_cap); }

		public:

			Root(Genode::Rpc_entrypoint *session_ep, Genode::Allocator *md_alloc,
			     void *regs_base, Genode::Dataspace_capability fb_ds_cap)
			: Genode::Root_component<Session_component>(session_ep, md_alloc),
			  _regs_base(regs_base),
			  _fb_ds_cap(fb_ds_cap) { }
	};
}


int main(int, char **)
{
	using namespace Genode;

	printf("--- pl11x framebuffer driver ---\n");

	/* locally map LCD control registers */
	Io_mem_connection io_mem(Board::PL11X_MMIO_BASE, Board::PL11X_MMIO_SIZE);
	void *regs_base = env()->rm_session()->attach(io_mem.dataspace());

	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, 4096, "fb_ep");

	Dataspace_capability fb_ds_cap =
		Framebuffer::alloc_video_memory(Framebuffer::FB_SIZE);

	/*
	 * Let the entry point serve the framebuffer and input root interfaces
	 */
	static Framebuffer::Root fb_root(&ep, env()->heap(), regs_base, fb_ds_cap);
	env()->parent()->announce(ep.manage(&fb_root));

	sleep_forever();

	return 0;
}
