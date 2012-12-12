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
#include <base/elf.h>
#include <base/env.h>
#include <base/sleep.h>
#include <base/thread.h>
#include <cpu/cpu_state.h>
#include <io_mem_session/connection.h>
#include <irq_session/connection.h>
#include <rom_session/connection.h>
#include <vm_session/connection.h>
#include <dataspace/client.h>

/* local includes */
#include <atag.h>
#include <m4if.h>

namespace Genode {

	class Ram {

		private:

			addr_t _base;
			size_t _size;
			addr_t _local;

		public:

			Ram(addr_t addr, size_t sz)
			: _base(addr), _size(sz), _local(0) { }

			addr_t base()  { return _base;  }
			size_t size()  { return _size;  }
			addr_t local() { return _local; }

			void attach(Dataspace_capability cap) {
				_local = (addr_t) env()->rm_session()->attach(cap); }
	};


	class Vm {

		private:

			enum {
				ATAG_OFFSET   = 0x100,
				INITRD_OFFSET = 0x1000000
			};

			Vm_connection     _vm_con;
			Rom_connection    _elf_rom;
			Rom_connection    _initrd_rom;
			const char*       _cmdline;
			size_t            _initrd_size;
			Cpu_state_modes  *_state;
			Ram               _ram;
			Io_mem_connection _ram_iomem;

			void _load_elf()
			{
				/* attach ELF locally */
				addr_t elf_addr = env()->rm_session()->attach(_elf_rom.dataspace());

				/* setup ELF object and read program entry pointer */
				Elf_binary elf((addr_t)elf_addr);
				_state->ip = elf.entry();
				if (!elf.valid()) {
					PWRN("Invalid elf binary!");
					return;
				}

				Elf_segment seg;
				for (unsigned n = 0; (seg = elf.get_segment(n)).valid(); ++n) {
					if (seg.flags().skip) continue;

					addr_t addr  = (addr_t)seg.start();
					size_t size  = seg.mem_size();

					if (addr == 0)
						continue; // TODO

					if (addr < _ram.base() ||
						(addr + size) > (_ram.base() + _ram.size())) {
						PWRN("Elf binary doesn't fit into RAM");
						return;
					}

					void  *base  = (void*) (_ram.local() + (addr - _ram.base()));
					addr_t laddr = elf_addr + seg.file_offset();

					/* copy contents */
					memcpy(base, (void *)laddr, seg.file_size());

					/* if writeable region potentially fill with zeros */
					if (size > seg.file_size() && seg.flags().w)
						memset((void *)((addr_t)base + seg.file_size()),
						       0, size - seg.file_size());
				}

				/* detach ELF */
				env()->rm_session()->detach((void*)elf_addr);
			}

			void _load_initrd()
			{
				addr_t addr = env()->rm_session()->attach(_initrd_rom.dataspace());
				memcpy((void*)(_ram.local() + INITRD_OFFSET),
				       (void*)addr, _initrd_size);
				env()->rm_session()->detach((void*)addr);
			}

			void _prepare_atag()
			{
				Atag tag((void*)(_ram.local() + ATAG_OFFSET));
				tag.setup_mem_tag(_ram.base(), _ram.size() + 0x52000000);
				tag.setup_cmdline_tag(_cmdline);
				tag.setup_initrd2_tag(_ram.base() + INITRD_OFFSET, _initrd_size);
				tag.setup_end_tag();
			}

		public:

			Vm(const char *kernel, const char *initrd, const char *cmdline,
			   addr_t ram_base, size_t ram_size)
			: _elf_rom(kernel),
			  _initrd_rom(initrd),
			  _cmdline(cmdline),
			  _initrd_size(Dataspace_client(_initrd_rom.dataspace()).size()),
			  _state((Cpu_state_modes*)env()->rm_session()->attach(_vm_con.cpu_state())),
			  _ram(ram_base, ram_size),
			  _ram_iomem(ram_base, ram_size)
			{
				memset((void*)_state, 0, sizeof(Cpu_state_modes));
				_ram.attach(_ram_iomem.dataspace());
			}

			void start(Signal_context_capability sig_cap)
			{
				_load_elf();
				_load_initrd();
				_prepare_atag();
				_state->cpsr = 0x93; /* SVC mode and IRQs disabled */
				_state->r1 = 3273;   /* MACH_TYPE mx53 loco board  */
				_state->r2 = _ram.base() + ATAG_OFFSET; /* ATAG addr */
				_vm_con.exception_handler(sig_cap);
			}

			void run() { _vm_con.run(); }

			void dump()
			{
				const char * const modes[] =
					{ "und", "svc", "abt", "irq", "fiq" };
				const char * const exc[] =
					{ "invalid", "reset", "undefined", "smc", "pf_abort",
				      "data_abort", "irq", "fiq" };

				printf("Cpu state:\n");
				printf("  r0        = %08lx\n", _state->r0);
				printf("  r1        = %08lx\n", _state->r1);
				printf("  r2        = %08lx\n", _state->r2);
				printf("  r3        = %08lx\n", _state->r3);
				printf("  r4        = %08lx\n", _state->r4);
				printf("  r5        = %08lx\n", _state->r5);
				printf("  r6        = %08lx\n", _state->r6);
				printf("  r7        = %08lx\n", _state->r7);
				printf("  r8        = %08lx\n", _state->r8);
				printf("  r9        = %08lx\n", _state->r9);
				printf("  r10       = %08lx\n", _state->r10);
				printf("  r11       = %08lx\n", _state->r11);
				printf("  r12       = %08lx\n", _state->r12);
				printf("  sp        = %08lx\n", _state->sp);
				printf("  lr        = %08lx\n", _state->lr);
				printf("  ip        = %08lx\n", _state->ip);
				printf("  cpsr      = %08lx\n", _state->cpsr);
				for (unsigned i = 0;
				     i < Cpu_state_modes::Mode_state::MAX; i++) {
					printf("  sp_%s    = %08lx\n", modes[i], _state->mode[i].sp);
					printf("  lr_%s    = %08lx\n", modes[i], _state->mode[i].lr);
					printf("  spsr_%s  = %08lx\n", modes[i], _state->mode[i].spsr);
				}
				printf("  exception = %s\n", exc[_state->cpu_exception]);
			}

			Cpu_state_modes *state() const { return _state; }
	};


	class Iim : Mmio
	{
		private:

			struct Srev : public Register<0x24, 32> { };

		public:

			Iim(addr_t base) : Mmio(base) {}

			uint32_t srev() { return read<Srev>(); }
	};


	class Ccm : Mmio
	{
		private:

			struct Cbcdr : public Register<0x14, 32> { };

			struct Cbcmr : public Register<0x18, 32>
			{
				 struct Apm_sel     : Bitfield<1,1> { };
				 struct Ddr_clk_sel : Bitfield<10,2> { };
			};

			struct Cbcgr0 : public Register<0x68, 32> {};
			struct Cbcgr1 : public Register<0x6c, 32> {};
			struct Cbcgr2 : public Register<0x70, 32> {};
			struct Cbcgr3 : public Register<0x74, 32> {};
			struct Cbcgr4 : public Register<0x78, 32> {};
			struct Cbcgr5 : public Register<0x7c, 32> {};
			struct Cbcgr6 : public Register<0x80, 32> {};
			struct Cbcgr7 : public Register<0x84, 32> {};

		public:

			Ccm(addr_t base) : Mmio(base) {}

			void perclk_lp_apm_sel() { write<Cbcmr::Apm_sel>(1); }

			void prediv1_set() { write<Cbcdr>(0x80);    }

			uint32_t prediv1() { return read<Cbcdr>(); }

			void disable()
			{
				write<Cbcgr0>(0x350f00c5);
				write<Cbcgr1>(0);
				write<Cbcgr2>(0);
				write<Cbcgr3>(0);
				write<Cbcgr4>(0x10000);
				write<Cbcgr5>(0xc5d010);
				write<Cbcgr6>(0xf00010d);
				write<Cbcgr7>(0);
			}

			uint32_t mr_read() { return read<Cbcmr>(); }

			uint32_t mr_ddr_clk() { return read<Cbcmr::Ddr_clk_sel>(); }
	};


	class Vmm : public Genode::Thread<8192>
	{
		private:

			enum Hypervisor_calls {
				CCM_APM_SEL,
				CCM_PRED1_WRITE,
				CCM_PRED1_READ,
				CCM_DISABLE,
				CCM_MR_READ,
				CCM_CSCMR2,
				CCM_CS1CDR,
				CCM_CBCMR_DDR_CLK,
				IIM_SREV,
			};

			Vm                   *_vm;
			Io_mem_connection     _m4if_io_mem;
			Io_mem_connection     _ccm_io_mem;
			M4if                  _m4if;
			Ccm                   _ccm;

			void _handle_hypervisor_call()
			{
				switch (_vm->state()->r1) {
				case 100:
					Genode::printf("write: %lx (ip=%lx)\n",
								   _vm->state()->r0,
								   _vm->state()->ip);
					break;
				case CCM_APM_SEL:
					{
						PDBG("CCM_APM_SEL");
						_ccm.perclk_lp_apm_sel();
						break;
					}
				case CCM_PRED1_WRITE:
					{
						PDBG("CCM_PRED1_WRITE");
						PDBG("%lx", _vm->state()->r0);
						//_ccm.prediv1_set();
						PDBG("return");
						break;
					}
				case CCM_PRED1_READ:
					{
						PDBG("CCM_PRED1_READ");
						_vm->state()->r0 = _ccm.prediv1() | 0x80;
						PDBG("%lx", _vm->state()->r0);
						break;
					}
				case CCM_DISABLE:
					{
						PDBG("CCM_DISABLE");
						_ccm.disable();
						break;
					}
				case CCM_MR_READ:
					{
						PDBG("CCM_MR_READ");
						_vm->state()->r0 = _ccm.mr_read();
						PDBG("%lx", _vm->state()->r0);
						break;
					}
				case CCM_CSCMR2:
					{
						PDBG("CCM_CSCMR2");
						PDBG("%lx", _vm->state()->r0);
						break;
					}
				case CCM_CS1CDR:
					{
						PDBG("CCM_CS1CDR");
						PDBG("%lx", _vm->state()->r0);
						break;
					}
				case CCM_CBCMR_DDR_CLK:
					{
						PDBG("CCM_CBCMR_DDR_CLK");
						_vm->state()->r0 = _ccm.mr_ddr_clk();
						PDBG("%lx", _vm->state()->r0);
						break;
					}
				case IIM_SREV:
					{
						PDBG("IIM_SREV");
						Io_mem_connection iomem(0x63f98000, 0x1000);
						Iim iim((addr_t)env()->rm_session()->attach(iomem.dataspace()));
						_vm->state()->r0 = iim.srev();
						PDBG("%lx", _vm->state()->r0);
						break;
					}
				default:
					PERR("Unknown hypervisor call!");
					_vm->dump();
				}
			}

			bool _handle_data_abort()
			{
				_vm->dump();
				return false;
			}

			bool _handle_vm()
			{
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
					if (!_handle_vm())
						return;
				}
			};

		public:

			Vmm(Vm    *vm,
			    addr_t m4if_base)
			: _vm(vm),
			  _m4if_io_mem(m4if_base, 0x1000),
			  _ccm_io_mem(0x53fd4000, 0x1000),
			  _m4if((addr_t)env()->rm_session()->attach(_m4if_io_mem.dataspace())),
			  _ccm((addr_t)env()->rm_session()->attach(_ccm_io_mem.dataspace()))
			{ _m4if.set_region(0x70000000, 0xfffffff); }
	};
}


int main()
{
	enum {
		MAIN_MEM_START    = 0x80000000,
		MAIN_MEM_SIZE     = 0x10000000,
		M4IF_PHYS_BASE    = 0x63fd8000,
	};

	static const char* cmdline = "console=ttymxc0,115200 root=/dev/ram video=mxcdi1fb:GBR24,VGA-XGA di1_primary vga";
	static Genode::Vm  vm("linux", "initrd.gz", cmdline,
	                      MAIN_MEM_START, MAIN_MEM_SIZE);
	static Genode::Vmm vmm(&vm, M4IF_PHYS_BASE);

	PINF("Start virtual machine");
	vmm.start();

	Genode::sleep_forever();
	return 0;
}
