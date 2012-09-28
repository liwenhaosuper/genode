/*
 * \brief  Video modes
 * \author Stefan Kalkowski
 * \date   2012-09-28
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__FRAMEBUFFER__MODES_H_
#define _INCLUDE__FRAMEBUFFER__MODES_H_

namespace Framebuffer {

	namespace Vga_60hz {
		enum {
			SCR_WIDTH        =  640,
			SCR_HEIGHT       =  480,
			LEFT_MARGIN      =   64, //48,
			RIGHT_MARGIN     =   32, //16,
			UPPER_MARGIN     =    9, //33,
			LOWER_MARGIN     =   11, //10,
			HSYNC_LEN        =   64, //96,
			VSYNC_LEN        =   25, // 2,
		};
	}

	namespace Svga_60hz {
		enum {
			SCR_WIDTH        =  800,
			SCR_HEIGHT       =  600,
			LEFT_MARGIN      =   88,
			RIGHT_MARGIN     =   40,
			UPPER_MARGIN     =   23,
			LOWER_MARGIN     =    1,
			HSYNC_LEN        =  128,
			VSYNC_LEN        =    4,
		};
	}

	namespace Xga_60hz {
		enum {
			SCR_WIDTH        = 1024,
			SCR_HEIGHT       =  768,
			LEFT_MARGIN      =  160,
			RIGHT_MARGIN     =   24,
			UPPER_MARGIN     =   29,
			LOWER_MARGIN     =    3,
			HSYNC_LEN        =  136,
			VSYNC_LEN        =    6,
		};
	}
}

#endif /* _INCLUDE__FRAMEBUFFER__MODES_H_ */
