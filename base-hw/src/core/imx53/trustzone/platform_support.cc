/*
 * \brief   Platform implementations specific for base-hw and i.MX53
 * \author  Stefan Kalkowski
 * \date    2012-10-24
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/service.h>
#include <drivers/board.h>

/* core includes */
#include <platform.h>
#include <platform_services.h>
#include <vm_root.h>
#include <kernel_support.h>

using namespace Genode;


Native_region * Platform::_ram_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		{ Board::CSD0_DDR_RAM_BASE, 0x10000000 }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_irq_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		{ 0, Kernel::Pic::MAX_INTERRUPT_ID + 1 }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_core_only_irq_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		/* core timer */
		{ Board::EPIT_1_IRQ, 1 },

		/* core UART */
		{ Board::UART_1_IRQ, 1 }
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_mmio_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		{ 0x07000000, 0x1000000  }, /* security controller */
		{ 0x10000000, 0x30000000 }, /* SATA, IPU, GPU      */
		{ 0x50000000, 0x20000000 }, /* Misc.               */
		{ 0x80000000, 0x30000000 }, /* Unsecure RAM        */
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


Native_region * Platform::_core_only_mmio_regions(unsigned const i)
{
	static Native_region _regions[] =
	{
		/* core UART */
		{ Board::UART_1_MMIO_BASE, Board::UART_1_MMIO_SIZE },

		/* core timer */
		{ Board::EPIT_1_MMIO_BASE, Board::EPIT_1_MMIO_SIZE },

		/* interrupt controller */
		{ Board::TZIC_MMIO_BASE, Board::TZIC_MMIO_SIZE },
	};
	return i < sizeof(_regions)/sizeof(_regions[0]) ? &_regions[i] : 0;
}


void Genode::platform_add_local_services(Rpc_entrypoint *ep, Sliced_heap *sh,
                                         Service_registry *ls)
{
	/* add TrustZone specific vm service */
	static Vm_root vm_root(ep, sh, platform()->ram_alloc());
	static Local_service vm_ls(Vm_session::service_name(), &vm_root);
	ls->insert(&vm_ls);
}
