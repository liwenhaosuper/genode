/*
 * \brief  Virtual Machine Monitor i.mx53 specific framebuffer virtual device
 * \author Stefan Kalkowski
 * \date   2012-06-25
 */

/*
 * Copyright (C) 2008-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _SRC__SERVER__VMM__IMX53__FRAMEBUFFER_H_
#define _SRC__SERVER__VMM__IMX53__FRAMEBUFFER_H_

/* Genode includes */
#include <imx_framebuffer_session/connection.h>

/* local includes */
#include <vm.h>

namespace Vmm {
	class Framebuffer;
};

class Vmm::Framebuffer
{
	private:

		Vm                            *_vm;
		::Framebuffer::Imx_connection  _overlay;
		Genode::addr_t                 _base;

		enum Opcodes {
			BASE,
		};

	public:

		Framebuffer(Vm *vm)
		: _vm(vm), _base(0) { }

		void handle(Vm_state *state)
		{
			switch (state->r1) {
			case BASE:
				{
					_base = state->r2;
					_overlay.overlay(_base);
					break;
				}
			default:
				PWRN("Unknown opcode!");
				_vm->dump();
			};
		}
};

#endif /* _SRC__SERVER__VMM__IMX53__FRAMEBUFFER_H_ */
