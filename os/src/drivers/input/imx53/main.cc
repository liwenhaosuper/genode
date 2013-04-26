/**
 * \brief  Input driver front-end
 * \author Norman Feske
 * \author Christian Helmuth
 * \date   2006-08-30
 */

/*
 * Copyright (C) 2006-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode */
#include <base/env.h>
#include <base/sleep.h>
#include <base/rpc_server.h>
#include <root/component.h>
#include <cap_session/connection.h>
#include <input/component.h>
#include <base/printf.h>

#include <driver.h>

using namespace Genode;

static Event_queue ev_queue;

namespace Input {

	/*
	 * Event handling is disabled on queue creation and will be enabled later if a
	 * session is created.
	 */
	void event_handling(bool enable)
	{
		if (enable)
			ev_queue.enable();
		else
			ev_queue.disable();
	}

	bool event_pending() { return !ev_queue.empty(); }
	Event get_event() { return ev_queue.get(); }
}


int main(int argc, char **argv)
{
	/* initialize server entry point */
	enum { STACK_SIZE = 4096 };
	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "input_ep");

	static Input::Driver driver(ev_queue);

	/* entry point serving input root interface */
	static Input::Root input_root(&ep, env()->heap());

	/* tell parent about the service */
	env()->parent()->announce(ep.manage(&input_root));

	/* main's done - go to sleep */

	sleep_forever();
	return 0;
}
