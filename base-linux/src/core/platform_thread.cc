/*
 * \brief  Linux-specific platform thread implementation
 * \author Norman Feske
 * \date   2007-10-15
 */

/*
 * Copyright (C) 2007-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <util/token.h>
#include <util/misc_math.h>
#include <base/printf.h>

/* local includes */
#include "platform_thread.h"
#include "server_socket_pair.h"

using namespace Genode;


typedef Token<Scanner_policy_identifier_with_underline> Tid_token;


Platform_thread::Platform_thread(const char *name, unsigned, addr_t)
: _tid(-1), _pid(-1)
{
	strncpy(_name, name, min(sizeof(_name), strlen(name)));
}


Platform_thread::~Platform_thread()
{
	ep_sd_registry()->disassociate(_ncs.client_sd);

	if (_ncs.client_sd)
		lx_close(_ncs.client_sd);

	if (_ncs.server_sd)
		lx_close(_ncs.server_sd);
}


void Platform_thread::cancel_blocking()
{
	PDBG("send cancel-blocking signal to %ld\n", _tid);
	lx_tgkill(_pid, _tid, LX_SIGUSR1);
}


void Platform_thread::pause()
{
	PDBG("not implemented");
}


void Platform_thread::resume()
{
	PDBG("not implemented");
}


int Platform_thread::client_sd()
{
	/* construct socket pair on first call */
	if (_ncs.client_sd == -1)
		_ncs = create_server_socket_pair(_tid);

	return _ncs.client_sd;
}


int Platform_thread::server_sd()
{
	client_sd();
	return _ncs.server_sd;
}
