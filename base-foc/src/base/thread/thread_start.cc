/*
 * \brief  Fiasco-specific implementation of the non-core startup Thread API
 * \author Norman Feske
 * \author Stefan Kalkowski
 * \date   2010-01-19
 */

/*
 * Copyright (C) 2010-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/thread.h>
#include <base/printf.h>
#include <base/sleep.h>
#include <base/env.h>

namespace Fiasco {
#include <l4/sys/utcb.h>
}

using namespace Genode;


void Thread_base::_deinit_platform_thread()
{
	using namespace Fiasco;

	if (_context->utcb && _thread_cap.valid()) {
		Cap_index *i = (Cap_index*)l4_utcb_tcr_u(_context->utcb)->user[UTCB_TCR_BADGE];
		cap_map()->remove(i);
		env()->cpu_session()->kill_thread(_thread_cap);
	}
}


void Thread_base::start()
{
	using namespace Fiasco;

	/* create thread at core */
	char buf[48];
	name(buf, sizeof(buf));
	_thread_cap = env()->cpu_session()->create_thread(buf);

	/* assign thread to protection domain */
	env()->pd_session()->bind_thread(_thread_cap);

	/* create new pager object and assign it to the new thread */
	Pager_capability pager_cap = env()->rm_session()->add_client(_thread_cap);
	env()->cpu_session()->set_pager(_thread_cap, pager_cap);

	/* get gate-capability and badge of new thread */
	Thread_state state;
	try { state = env()->cpu_session()->state(_thread_cap); }
	catch (...) { throw Cpu_session::Thread_creation_failed(); }
	_tid = state.kcap;
	_context->utcb = state.utcb;

	Cap_index *i = cap_map()->insert(state.id, state.kcap);
	l4_utcb_tcr_u(state.utcb)->user[UTCB_TCR_BADGE]      = (unsigned long) i;
	l4_utcb_tcr_u(state.utcb)->user[UTCB_TCR_THREAD_OBJ] = (addr_t)this;

	/* register initial IP and SP at core */
	addr_t thread_sp = (addr_t)&_context->stack[-4];
	thread_sp &= ~0xf;  /* align initial stack to 16 byte boundary */
	env()->cpu_session()->start(_thread_cap, (addr_t)_thread_start, thread_sp);
}


void Thread_base::cancel_blocking()
{
	env()->cpu_session()->cancel_blocking(_thread_cap);
}
