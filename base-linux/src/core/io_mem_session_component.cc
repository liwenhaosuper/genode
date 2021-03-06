/*
 * \brief  Linux-specific IO_MEM service
 * \author Christian Helmuth
 * \date   2006-09-01
 */

/*
 * Copyright (C) 2006-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/printf.h>

#include <io_mem_session_component.h>

using namespace Genode;


Io_mem_session_component::Io_mem_session_component(Range_allocator *io_mem_alloc,
                                                   Range_allocator *ram_alloc,
                                                   Rpc_entrypoint  *ds_ep,
                                                   const char      *args)
{
	PWRN("no io_mem support on Linux (args=\"%s\")", args);
}
