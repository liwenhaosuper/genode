/*
 * \brief  Driver for the PL11x framebuffer
 * \author Stefan Kalkowski
 * \date   2012-09-27
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__FRAMEBUFFER__PL11X_H_
#define _INCLUDE__FRAMEBUFFER__PL11X_H_

/* Genode includes */
#include <util/mmio.h>

namespace Framebuffer
{

	struct Pl11x : Genode::Mmio
	{
			struct Lcd_timing_0 : public Register<0x0, 32, true>
			{
					struct Ppl : Bitfield<2,6>  { }; /* Pixels per line */
					struct Hsw : Bitfield<8,8>  { }; /* Horizontal sync pulse width */
					struct Hfp : Bitfield<16,8> { }; /* Horizontal front porch */
					struct Hbp : Bitfield<24,8> { }; /* Horizontal back porch */
			};

			struct Lcd_timing_1 : public Register<0x4, 32, true>
			{
					struct Lpp : Bitfield<0,10> { }; /* Lines per panel */
					struct Vsw : Bitfield<10,6> { }; /* Vertical sync pulse width */
					struct Vfp : Bitfield<16,8> { }; /* Vertical front porch */
					struct Vbp : Bitfield<24,8> { }; /* Vertical back porch */
			};

			struct Lcd_timing_2 : public Register<0x8, 32, true>
			{
					struct Pcd_lo : Bitfield<0,5>   { };
					struct Clksel : Bitfield<5,1>   { };
					struct Acb    : Bitfield<6,5>   { };
					struct Ivs    : Bitfield<11,1>  { };
					struct Ihs    : Bitfield<12,1>  { };
					struct Ipc    : Bitfield<13,1>  { };
					struct Ioe    : Bitfield<14,1>  { };
					struct Cpl    : Bitfield<16,10> { };
					struct Bcd    : Bitfield<26,1>  { };
					struct Pcd_hi : Bitfield<27,5>  { };
			};

			struct Lcd_timing_3 : public Register<0xc, 32, true>
			{
					struct Led : Bitfield<0,7>  { };
					struct Lee : Bitfield<16,1> { };
			};

			struct Lcd_up_base  : public Register<0x10, 32, true> {};
			struct Lcd_lp_base  : public Register<0x10, 32, true> {};

			struct Lcd_control  : public Register<0x10, 32>
			{
					struct Lcd_en    : Bitfield<0,1>  { };
					struct Lcd_bpp   : Bitfield<1,3>
					{
						enum {
							BPP_1,
							BPP_2,
							BPP_4,
							BPP_8,
							BPP_16,
							BPP_24,
							BPP_16_565,
							BPP_12_444,
						};
					};
					struct Lcd_bw    : Bitfield<4,1>  { };
					struct Lcd_tft   : Bitfield<5,1>  { };
					struct Lcd_mono8 : Bitfield<6,1>  { };
					struct Lcd_dual  : Bitfield<7,1>  { };
					struct Bgr       : Bitfield<8,1>  { };
					struct Bebo      : Bitfield<9,1>  { };
					struct Bepo      : Bitfield<10,1> { };
					struct Lcd_pwr   : Bitfield<11,1> { };
					struct Lcd_v_cmp : Bitfield<12,2> { };
					struct Watermark : Bitfield<16,1> { };
			};

			struct Lcd_imsc     : public Register<0x10, 32>  {};

			struct Pheriph_id_0 : public Register<0xfe0, 32> {};
			struct Pheriph_id_1 : public Register<0xfe4, 32> {};
			struct Pheriph_id_2 : public Register<0xfe8, 32> {};
			struct Pheriph_id_3 : public Register<0xfec, 32> {};

			struct Primecell_id_0 : public Register<0xff0, 32> {};
			struct Primecell_id_1 : public Register<0xff4, 32> {};
			struct Primecell_id_2 : public Register<0xff8, 32> {};
			struct Primecell_id_3 : public Register<0xffc, 32> {};

			Pl11x(Genode::addr_t const base) : Genode::Mmio(base) {}
	};
}

#endif /* _INCLUDE__FRAMEBUFFER__PL11X_H_ */
