/*
 * \brief  Platform support specific to x86
 * \author Christian Helmuth
 * \author Stefan Kalkowski
 * \date   2006-04-11
 */

/*
 * Copyright (C) 2006-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/printf.h>

#include "platform.h"
#include "util.h"

/* Fiasco.OC includes */
namespace Fiasco {
#include <l4/sys/ipc.h>
}

void Genode::Platform::_setup_io_port_alloc()
{
	using namespace Fiasco;

	l4_fpage_t fpage = l4_iofpage(0, L4_WHOLE_IOADDRESS_SPACE);
	l4_utcb_mr()->mr[0] = fpage.raw;
	l4_utcb_br()->bdr  &= ~L4_BDR_OFFSET_MASK;
	l4_utcb_br()->br[0] = L4_ITEM_MAP;
	l4_utcb_br()->br[1] = fpage.raw;
	l4_msgtag_t tag = l4_ipc_call(L4_BASE_PAGER_CAP, l4_utcb(),
	                              l4_msgtag(L4_PROTO_IO_PAGE_FAULT, 1, 0, 0),
	                              L4_IPC_NEVER);

	if (l4_ipc_error(tag, l4_utcb()))
		panic("Received no I/O ports from sigma0");

	/* setup allocator */
	_io_port_alloc.add_range(0, 0x10000);
}