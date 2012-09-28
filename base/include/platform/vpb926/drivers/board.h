/*
 * \brief  Driver for the Versatile Platform Baseboard for ARM926
 * \author Stefan Kalkowski
 * \date   2012-09-28
 */

/*
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__DRIVERS__BOARD_H_
#define _INCLUDE__DRIVERS__BOARD_H_

namespace Genode
{
	/**
	 * Driver for the Versatile Platform Baseboard for ARM926
	 */
	struct Board
	{
		enum
		{
			PL11X_MMIO_BASE = 0x10120000,
			PL11X_MMIO_SIZE = 0x1000,
		};
	};
}

#endif /* _INCLUDE__DRIVERS__BOARD_H_ */
