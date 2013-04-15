/*
 * \brief  Virtual Machine Monitor i.mx53 specific input virtual device
 * \author Stefan Kalkowski
 * \date   2012-06-25
 */

/*
 * Copyright (C) 2008-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _SRC__SERVER__VMM__IMX53__INPUT_H_
#define _SRC__SERVER__VMM__IMX53__INPUT_H_

/* Genode includes */
#include <nitpicker_session/connection.h>
#include <nitpicker_view/client.h>
#include <input/event_queue.h>
#include <input/event.h>

/* local includes */
#include <vm.h>

namespace Vmm {
	class Input;
}


class Vmm::Input
{
	private:

		Vm                        *_vm;
		Nitpicker::Connection      _nitpicker;
		Nitpicker::View_capability _view_cap;
		::Input::Event            *_ev_buf;
		unsigned                   _num_events;
		unsigned                   _event;

		enum Resolution {
			WIDTH  = 1024,
			HEIGHT = 768
		};

		enum Opcodes {
			GET_EVENT,
		};

		enum Type {
			INVALID,
			PRESS,
			RELEASE,
			MOTION
		};

	public:

		Input(Vm *vm)
		: _vm(vm),
		  _nitpicker(WIDTH, HEIGHT, false),
		  _view_cap(_nitpicker.create_view()),
		  _ev_buf(Genode::env()->rm_session()->attach(_nitpicker.input()->dataspace())),
		  _num_events(0),
		  _event(0)
		{
			using namespace Nitpicker;

			View_client(_view_cap).viewport(0, 0, WIDTH, HEIGHT, 0, 0, true);
			View_client(_view_cap).stack(Nitpicker::View_capability(), true, true);
			View_client(_view_cap).title("Android");
		}

		void handle(Vm_state * volatile state)
		{
			switch (state->r1) {
			case GET_EVENT:
				{
					state->r0 = INVALID;

					if (!_num_events && _nitpicker.input()->is_pending())
						_num_events = _nitpicker.input()->flush();

					if (_event < _num_events) {
						::Input::Event *ev = &_ev_buf[_event];
						switch (ev->type()) {
						case ::Input::Event::PRESS:
							state->r0 = PRESS;
							state->r3 = ev->keycode();
							break;
						case ::Input::Event::RELEASE:
							state->r0 = RELEASE;
							state->r3 = ev->keycode();
							break;
						case ::Input::Event::MOTION:
							state->r0 = MOTION;
							break;
						default:
							return;
						}
						state->r1 = ev->ax();
						state->r2 = ev->ay();
						_event++;
					}

					if (_event == _num_events) {
						_num_events = 0;
						_event      = 0;
					}

					break;
				}
			default:
				PWRN("Unknown opcode!");
				_vm->dump();
			};
		}
};

#endif /* _SRC__SERVER__VMM__IMX53__INPUT_H_ */
