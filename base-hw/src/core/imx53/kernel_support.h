/*
 * \brief  Platform specific parts of kernel
 * \author Stefan Kalkowski
 * \date   2012-10-24
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _SRC__CORE__KERNEL_SUPPORT_H_
#define _SRC__CORE__KERNEL_SUPPORT_H_

/* Genode includes */
#include <imx53/drivers/board.h>
#include <arm/v7/cpu.h>
#include <drivers/timer/epit.h>
#include <drivers/pic/tzic.h>

struct Cpu : Arm_v7::Cpu { };

namespace Kernel
{
	/**
	 * Programmable interrupt controller
	 */
	class Pic : public Genode::Tzic
	{
		public:

			Pic() : Tzic(Board::TZIC_MMIO_BASE)
			{
				/* configure interrupts as nonsecure, and disable them */
				for (unsigned i = 0; i <= MAX_INTERRUPT; i++) {
					write<Enclear::Clear_enable>(1, i);
					write<Intsec::Nonsecure>(1, i);
				}

				write<Priomask::Mask>(0x1f);
				write<Intctrl>(Intctrl::Enable::bits(1) |
				               Intctrl::Nsen::bits(1)   |
				               Intctrl::Nsen_mask::bits(1));
			}
	};


	/**
	 * Timer
	 */
	class Timer : public Genode::Epit_base
	{
		public:

			enum { IRQ = Board::EPIT_1_IRQ };

			/**
			 * Constructor
			 */
			Timer() : Genode::Epit_base(Board::EPIT_1_MMIO_BASE) { }
	};
}

#endif /* _SRC__CORE__KERNEL_SUPPORT_H_ */

