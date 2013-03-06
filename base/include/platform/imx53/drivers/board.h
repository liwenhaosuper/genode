/*
 * \brief  Board definitions for the i.MX53
 * \author Stefan Kalkowski
 * \date   2012-10-24
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__PLATFORM__BOARD_H_
#define _INCLUDE__PLATFORM__BOARD_H_

namespace Genode
{
	/**
	 * i.MX53 motherboard
	 */
	struct Board
	{
		enum {
			MMIO_BASE          = 0x0,
			MMIO_SIZE          = 0x70000000,

			CSD0_DDR_RAM_BASE  = 0x70000000,
			CSD0_DDR_RAM_SIZE  = 0x80000000,

			UART_1_IRQ         = 31,
			UART_1_MMIO_BASE   = 0x53fbc000,
			UART_1_MMIO_SIZE   = 0x00004000,

			EPIT_1_IRQ         = 40,
			EPIT_1_MMIO_BASE   = 0x53fac000,
			EPIT_1_MMIO_SIZE   = 0x00004000,

			EPIT_2_IRQ         = 41,
			EPIT_2_MMIO_BASE   = 0x53fb0000,
			EPIT_2_MMIO_SIZE   = 0x00004000,

			TZIC_MMIO_BASE     = 0x0fffc000,
			TZIC_MMIO_SIZE     = 0x00004000,

			AIPS_1_MMIO_BASE   = 0x53f00000,
			AIPS_2_MMIO_BASE   = 0x63f00000,

			IOMUXC_BASE        = 0x53fa8000,
			IOMUXC_SIZE        = 0x00004000,

			PWM2_BASE          = 0x53fb8000,
			PWM2_SIZE          = 0x00004000,

			IPU_ERR_IRQ        = 10,
			IPU_SYNC_IRQ       = 11,
			IPU_BASE           = 0x18000000,
			IPU_SIZE           = 0x08000000,

			SRC_BASE           = 0x53fd0000,
			SRC_SIZE           = 0x00004000,

			CCM_BASE           = 0x53fd4000,
			CCM_SIZE           = 0x00004000,

			I2C_1_IRQ          = 62,
			I2C_1_BASE         = 0x63fc8000,
			I2C_1_SIZE         = 0x00004000,

			I2C_2_IRQ          = 63,
			I2C_2_BASE         = 0x63fc4000,
			I2C_2_SIZE         = 0x00004000,

			I2C_3_IRQ          = 64,
			I2C_3_BASE         = 0x53fec000,
			I2C_3_SIZE         = 0x00004000,

			M4IF_BASE          = 0x63fd8000,
			M4IF_SIZE          = 0x00001000,

			SECURITY_EXTENSION = 1,
		};
	};
}

#endif /* _INCLUDE__PLATFORM__BOARD_H_ */

