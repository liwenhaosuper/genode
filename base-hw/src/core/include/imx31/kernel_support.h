/*
 * \brief  Kernel support for i.MX31
 * \author Norman Feske
 * \author Martin Stein
 * \date   2012-08-30
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CORE__INCLUDE__IMX31__KERNEL_SUPPORT_H_
#define _CORE__INCLUDE__IMX31__KERNEL_SUPPORT_H_

/* Genode includes */
#include <drivers/board.h>
#include <drivers/timer/epit.h>
#include <arm/v6/cpu.h>
#include <imx31/pic.h>

struct Cpu : Arm_v6::Cpu { };

namespace Kernel
{
	/**
	 * Programmable interrupt controller
	 */
	class Pic : public Imx31::Pic { };


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

#endif /* _CORE__INCLUDE__IMX31__KERNEL_SUPPORT_H_ */
