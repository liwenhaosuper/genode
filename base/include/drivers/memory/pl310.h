/*
 * \brief  Simple Driver for the PL310 L2 cache controller
 * \author Stefan Kalkowski
 * \date   2011-06-20
 */

/*
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__DRIVERS__MEMORY__PL310_H_
#define _INCLUDE__DRIVERS__MEMORY__PL310_H_

/* Genode includes */
#include <util/register.h>
#include <util/mmio.h>

namespace Genode {

	class Pl310 : Mmio
	{
		protected:

			struct Cache_sync              : Register<0x730, 32> { };
			struct Invalidate_by_pa        : Register<0x770, 32> { };
			struct Invalidate_by_way       : Register<0x77c, 32> { };
			struct Clean_by_pa             : Register<0x7b0, 32> { };
			struct Clean_by_way            : Register<0x7bc, 32> { };
			struct Clean_invalidate_by_pa  : Register<0x7f0, 32> { };
			struct Clean_invalidate_by_way : Register<0x7fc, 32> { };

			struct Auxiliary_control : Register<0x104, 32>
			{
				struct Associativity : Bitfield<16,1> { };
				struct Way_size      : Bitfield<17,3> { };
			};

			size_t _associativity;
			size_t _way_size;
			size_t _memory_size;

			size_t _tag_size() {
				return log2(_memory_size / cache_size() * _associativity); }

			size_t _index_size() { return 27 - _tag_size(); }

		public:

			Pl310(addr_t const base, size_t memory_size)
			: Mmio(base),
			  _associativity(0),
			  _way_size(16*1024),
			  _memory_size(memory_size)
			{
				unsigned short cnt = read<Auxiliary_control::Way_size>();
				_way_size         *= 1 << cnt;
				_associativity     = read<Auxiliary_control::Associativity>()
				                     ? 16 : 8;
			}

			size_t cache_size() { return _way_size * _associativity; }

			void flush()
			{
				addr_t way_mask = (1 << _associativity) - 1;

				// TODO: potentially write to debug registers,
				//       have a look at ERRATA_588369

				write<Clean_invalidate_by_way>(way_mask);
				while (read<Clean_invalidate_by_way>() & way_mask) ;
				write<Cache_sync>(0);

				// TODO: potentially write to debug registers,
				//       have a look at ERRATA_588369
			}
	};
}

#endif /* _INCLUDE__DRIVERS__MEMORY__PL310_H_ */
