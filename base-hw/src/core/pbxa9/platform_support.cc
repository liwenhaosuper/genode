/*
 * \brief   Parts of platform that are specific to PBXA9
 * \author  Martin Stein
 * \date    2012-04-27
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <drivers/board.h>

/* core includes */
#include <platform.h>
#include <cortex_a9/cpu.h>
#include <cortex_a9/no_trustzone/pic.h>


using namespace Genode;


Native_region * Platform::_ram_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		{ Board::RAM_0_BASE, Board::RAM_0_SIZE },
		{ Board::RAM_1_BASE, Board::RAM_1_SIZE }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_irq_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		{ 0, Cortex_a9_no_trustzone::Pic::MAX_INTERRUPT_ID + 1 }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_core_only_irq_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		/* core timer */
		{ Cortex_a9::Cpu::PRIVATE_TIMER_IRQ, 1 },

		/* core UART */
		{ Board::PL011_0_IRQ, 1 }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_mmio_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		{ Board::MMIO_0_BASE, Board::MMIO_0_SIZE },
		{ Board::MMIO_1_BASE, Board::MMIO_1_SIZE }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_core_only_mmio_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		/* core timer and PIC */
		{ Board::CORTEX_A9_PRIVATE_MEM_BASE, Board::CORTEX_A9_PRIVATE_MEM_SIZE },

		/* core UART */
		{ Board::PL011_0_MMIO_BASE, Board::PL011_0_MMIO_SIZE }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}
