/*
 * \brief  Virtual Machine Monitor
 * \author Stefan Kalkowski
 * \date   2012-06-25
 */

/*
 * Copyright (C) 2008-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/env.h>
#include <base/sleep.h>
#include <base/thread.h>
#include <drivers/board.h>

/* local includes */
#include <vm.h>
#include <atag.h>
#include <m4if.h>
#include <framebuffer.h>
#include <input.h>

using namespace Genode;

enum {
	SECURE_MEM_START = 0x70000000,
	SECURE_MEM_SIZE  = 0xfffffff,
	VM_MEM_START     = 0x80000000,
	VM_MEM_SIZE      = 0x70000000,
	MACH_TYPE_TABLET = 3011,
	MACH_TYPE_QSB    = 3273,
	BOARD_REV_TABLET = 0x53321,
};


static const char* cmdline_tablet =
	"console=ttymxc0,115200 androidboot.console=ttymxc0 lpj=4997120 video=mxcdi1fb:RGB666,XGA gpu_memory=64M";


namespace Vmm {
	class Vmm;
}


class Vmm::Vmm : public Thread<8192>
{
	private:

		enum Devices {
			FRAMEBUFFER,
			INPUT,
		};

		Vm                   *_vm;
		Io_mem_connection     _m4if_io_mem;
		M4if                  _m4if;
		Framebuffer           _fb;
		Input                 _input;

		void _handle_hypervisor_call()
		{
			/* check device number*/
			switch (_vm->state()->r0) {
			case FRAMEBUFFER:
				_fb.handle(_vm->state());
				break;
			case INPUT:
				_input.handle(_vm->state());
				break;
			default:
				PERR("Unknown hypervisor call!");
				_vm->dump();
			};
		}

		bool _handle_data_abort()
		{
			_vm->dump();
			return false;
		}

		bool _handle_vm()
		{
			/* check exception reason */
			switch (_vm->state()->cpu_exception) {
			case Cpu_state::DATA_ABORT:
				if (!_handle_data_abort()) {
					PERR("Could not handle data-abort will exit!");
					return false;
				}
				break;
			case Cpu_state::SUPERVISOR_CALL:
				_handle_hypervisor_call();
				break;
			default:
				PERR("Curious exception occured");
				_vm->dump();
				return false;
			}
			return true;
		}

	protected:

		void entry()
		{
			Signal_receiver sig_rcv;
			Signal_context  sig_cxt;
			Signal_context_capability sig_cap(sig_rcv.manage(&sig_cxt));
			_vm->start(sig_cap);

			while (true) {
				_vm->run();
				Signal s = sig_rcv.wait_for_signal();
				if (s.context() != &sig_cxt) {
					PWRN("Invalid context");
					continue;
				}
				if ((s.context() == &sig_cxt) && !_handle_vm())
					return;
			}
		};

	public:

		Vmm(Vm *vm)
		: _vm(vm),
		  _m4if_io_mem(Board::M4IF_BASE, Board::M4IF_SIZE),
		  _m4if((addr_t)env()->rm_session()->attach(_m4if_io_mem.dataspace())),
		  _fb(vm),
		  _input(vm) {
			_m4if.set_region(SECURE_MEM_START, SECURE_MEM_SIZE); }
};


int main()
{
	static Vm vm("linux", "initrd.gz", cmdline_tablet, VM_MEM_START,
	             VM_MEM_SIZE, MACH_TYPE_TABLET, BOARD_REV_TABLET);
	static Vmm::Vmm vmm(&vm);

	PINF("Start virtual machine ...");
	vmm.start();

	sleep_forever();
	return 0;
}
