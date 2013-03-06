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
#include <drivers/timer/epit.h>

/* core includes */
#include <arm/v7/cpu.h>
#include <drivers/pic/tzic.h>

struct Cpu : Arm_v7::Cpu { };

namespace Kernel
{
	/**
	 * Programmable interrupt controller
	 */
	class Pic : public Genode::Tzic {

		public:

			enum { MAX_INTERRUPT_ID = 108 };

			Pic() : Tzic(Genode::Board::TZIC_MMIO_BASE)
			{
				for (unsigned i = 0; i <= MAX_INTERRUPT_ID; i++) {

					/* configure interrupt's security level */
					if ((i == ::Board::EPIT_1_IRQ) | (i == ::Board::EPIT_2_IRQ) |
						(i == ::Board::I2C_2_IRQ)  | (i == ::Board::I2C_3_IRQ) |
						(i >= 50 && i < 58) | (i >= 103 && i < 109)) {
						write<Intsec::Nonsecure>(0, i);

						/* set highest priority */
						write<Priority>(0, i);
					} else {
						write<Intsec::Nonsecure>(1, i);
						write<Priority>(0x80, i);
					}

					/* disable interrupt */
					write<Enclear::Clear_enable>(1, i);

				}

				write<Priomask::Mask>(0xff);
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

			enum { IRQ = Genode::Board::EPIT_1_IRQ };

			/**
			 * Constructor
			 */
			Timer() : Genode::Epit_base(Genode::Board::EPIT_1_MMIO_BASE) { }
	};
}

#endif /* _SRC__CORE__KERNEL_SUPPORT_H_ */

