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
#include <imx_framebuffer_session/connection.h>
#include <blit/blit.h>

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

			class Invalid_addr : Exception {};

			Ram(addr_t addr, size_t sz, addr_t local)
			: _base(addr), _size(sz), _local(local) { }

			addr_t base()          { return _base;  }
			size_t size()          { return _size;  }
			addr_t local()         { return _local; }

			addr_t va(addr_t phys)
			{
				if ((phys < _base) || (phys > (_base + _size)))
					throw Invalid_addr();
				return _local + (phys - _base);
			}
	};


	struct Vm_state : Cpu_state_modes
	{
		addr_t dfar;    /* data fault address             */
		addr_t ttbr[2]; /* translation table base regs    */
		addr_t ttbrc;   /* translation table base control */
	};


	class Mmu
	{
		private:

			Vm_state *_state;
			Ram      *_ram;

			unsigned _n_bits() { return _state->ttbrc & 0x7; }

			bool _ttbr0(addr_t mva) {
				return (!_n_bits() || !(mva >> (32 - _n_bits()))); }

			addr_t _first_level(addr_t va)
			{
				if (!_ttbr0(va))
					return ((_state->ttbr[1] & 0xffffc000) |
					        ((va >> 18) & 0xffffc));
				unsigned shift = 14 - _n_bits();
				return (((_state->ttbr[0] >> shift) << shift) |
				        (((va << _n_bits()) >> (18 + _n_bits())) & 0x3ffc));
			}

			addr_t _page(addr_t fe, addr_t va)
			{
				enum Type { FAULT, LARGE, SMALL };

				addr_t se = *((addr_t*)_ram->va(((fe & (~0UL << 10)) |
				                                 ((va >> 10) & 0x3fc))));
				switch (se & 0x3) {
				case FAULT:
					return 0;
				case LARGE:
					return ((se & (~0UL << 16)) | (va & (~0UL >> 16)));
				default:
					return ((se & (~0UL << 12)) | (va & (~0UL >> 20)));
				}
				return 0;
			}

			addr_t _section(addr_t fe, addr_t va) {
				return ((fe & 0xfff00000) | (va & 0xfffff)); }

			addr_t _supersection(addr_t fe, addr_t va)
			{
				PWRN("NOT IMPLEMENTED YET!");
				return 0;
			}

		public:

			Mmu(Vm_state *state, Ram *ram)
			: _state(state), _ram(ram) {}


			addr_t phys_addr(addr_t va)
			{
				enum Type { FAULT, PAGETABLE, SECTION };

				addr_t fe = *((addr_t*)_ram->va(_first_level(va)));
				switch (fe & 0x3) {
				case PAGETABLE:
					return _page(fe, va);
				case SECTION:
					return (fe & 0x40000) ? _supersection(fe, va)
					                      : _section(fe, va);
				}
				return 0;
			}
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
			Vm_state         *_state;
			Io_mem_connection _ram_iomem;
			Ram               _ram;

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

					void  *base  = (void*) (_ram.va(addr));
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
				tag.setup_mem_tag(_ram.base(), (_ram.size() - 0x1000000));
				tag.setup_cmdline_tag(_cmdline);
				tag.setup_initrd2_tag(_ram.base() + INITRD_OFFSET, _initrd_size);
				tag.setup_rev_tag(0x53321);
				tag.setup_end_tag();
			}

		public:

			Vm(const char *kernel, const char *initrd, const char *cmdline,
			   addr_t ram_base, size_t ram_size)
			: _elf_rom(kernel),
			  _initrd_rom(initrd),
			  _cmdline(cmdline),
			  _initrd_size(Dataspace_client(_initrd_rom.dataspace()).size()),
			  _state((Vm_state*)env()->rm_session()->attach(_vm_con.cpu_state())),
			  _ram_iomem(ram_base, ram_size),
			  _ram(ram_base, ram_size,
			       (addr_t)env()->rm_session()->attach(_ram_iomem.dataspace()))
			{
				/* copy IIM registers */
				Io_mem_connection iomem(0x63f98000, 0x1000);
				addr_t base = (addr_t)env()->rm_session()->attach(iomem.dataspace());
				memset((void*)_ram.va(0xe0000000), 0, 0x1000);
				memcpy((void*)_ram.va(0xe0000000), (void*)base, 0x10);
				memcpy((void*)_ram.va(0xe0000014), (void*)(base + 0x14), 0x28);
				env()->rm_session()->detach(base);

				/* fill VM state with zeroes */
				memset((void*)_state, 0, sizeof(Vm_state));
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
				printf("  Register     Virt     Phys\n");
				printf("---------------------------------\n");
				printf("  r0         = %08lx [%08lx]\n",
				       _state->r0, va_to_pa(_state->r0));
				printf("  r1         = %08lx [%08lx]\n",
				       _state->r1, va_to_pa(_state->r1));
				printf("  r2         = %08lx [%08lx]\n",
				       _state->r2, va_to_pa(_state->r2));
				printf("  r3         = %08lx [%08lx]\n",
				       _state->r3, va_to_pa(_state->r3));
				printf("  r4         = %08lx [%08lx]\n",
				       _state->r4, va_to_pa(_state->r4));
				printf("  r5         = %08lx [%08lx]\n",
				       _state->r5, va_to_pa(_state->r5));
				printf("  r6         = %08lx [%08lx]\n",
				       _state->r6, va_to_pa(_state->r6));
				printf("  r7         = %08lx [%08lx]\n",
				       _state->r7, va_to_pa(_state->r7));
				printf("  r8         = %08lx [%08lx]\n",
				       _state->r8, va_to_pa(_state->r8));
				printf("  r9         = %08lx [%08lx]\n",
				       _state->r9, va_to_pa(_state->r9));
				printf("  r10        = %08lx [%08lx]\n",
				       _state->r10, va_to_pa(_state->r10));
				printf("  r11        = %08lx [%08lx]\n",
				       _state->r11, va_to_pa(_state->r11));
				printf("  r12        = %08lx [%08lx]\n",
				       _state->r12, va_to_pa(_state->r12));
				printf("  sp         = %08lx [%08lx]\n",
				       _state->sp, va_to_pa(_state->sp));
				printf("  lr         = %08lx [%08lx]\n",
				       _state->lr, va_to_pa(_state->lr));
				printf("  ip         = %08lx [%08lx]\n",
				       _state->ip, va_to_pa(_state->ip));
				printf("  cpsr       = %08lx\n", _state->cpsr);
				for (unsigned i = 0;
				     i < Vm_state::Mode_state::MAX; i++) {
					printf("  sp_%s     = %08lx [%08lx]\n", modes[i],
					       _state->mode[i].sp, va_to_pa(_state->mode[i].sp));
					printf("  lr_%s     = %08lx [%08lx]\n", modes[i],
					       _state->mode[i].lr, va_to_pa(_state->mode[i].lr));
					printf("  spsr_%s   = %08lx [%08lx]\n", modes[i],
					       _state->mode[i].spsr, va_to_pa(_state->mode[i].spsr));
				}
				printf("  ttbr0      = %08lx\n", _state->ttbr[0]);
				printf("  ttbr1      = %08lx\n", _state->ttbr[1]);
				printf("  ttbrc      = %08lx\n", _state->ttbrc);
				printf("  dfar       = %08lx [%08lx]\n",
				       _state->dfar, va_to_pa(_state->dfar));
				printf("  exception  = %s\n", exc[_state->cpu_exception]);
			}

			addr_t va_to_pa(addr_t va)
			{
				try {
					Mmu mmu(_state, &_ram);
					return mmu.phys_addr(va);
				} catch(Ram::Invalid_addr) {}
				return 0;
			}

			Vm_state *state() const { return  _state; }
			Ram      *ram()         { return &_ram;   }
	};


	class Framebuffer
	{
		private:

			Vm                            *_vm;
			::Framebuffer::Imx_connection  _overlay;
			addr_t                         _base;

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


	class Vmm : public Genode::Thread<8192>
	{
		private:

			enum Devices { FRAMEBUFFER };

			Vm                   *_vm;
			Io_mem_connection     _m4if_io_mem;
			M4if                  _m4if;
			Framebuffer           _fb;

			void _handle_hypervisor_call()
			{
				switch (_vm->state()->r0) {
				case FRAMEBUFFER:
					_fb.handle(_vm->state());
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
				case Cpu_state::FAST_INTERRUPT_REQUEST:
					PDBG("FIQ: ip = %lx", _vm->state()->ip);
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
			  _m4if((addr_t)env()->rm_session()->attach(_m4if_io_mem.dataspace())),
			  _fb(vm)
			{
				_m4if.set_region(0x70000000, 0xfffffff);
			}
	};
}


int main()
{
	enum {
		MAIN_MEM_START = 0x80000000,
		MAIN_MEM_SIZE  = 0x70000000,
		M4IF_PHYS_BASE = 0x63fd8000,
	};

	static const char* cmdline = "console=ttymxc0,115200 init=/init androidboot.console=ttymxc0 di1_primary debug video=mxcdi1fb:SEIKO-WVGA";
	static Genode::Vm  vm("linux", "initrd.gz", cmdline,
	                      MAIN_MEM_START, MAIN_MEM_SIZE);
	static Genode::Vmm vmm(&vm, M4IF_PHYS_BASE);

	PINF("Start virtual machine");
	vmm.start();

	Genode::sleep_forever();
	return 0;
}
