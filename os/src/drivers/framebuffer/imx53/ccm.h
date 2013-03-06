/*
 * \brief  Clock control module
 * \author Nikolay Golikov <nik@ksyslabs.org>
 * \date   2012-10-09
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CCM_H_
#define _CCM_H_

/* Genode includes */
#include <util/mmio.h>

struct Ccm : Genode::Mmio
{
	enum { IPU_CLK = 133000000 };

	/**
	 * Control divider register
	 */
	struct Ccdr : Register<0x4, 32>
	{
		struct Ipu_hs_mask : Bitfield <21, 1> { };
	};

	struct Cscmr2 : Register<0x20, 32> {};

	struct Cdcdr : Register<0x30, 32> {};

	/**
	 * Low power control register
	 */
	struct Clpcr : Register<0x54, 32>
	{
		struct Bypass_ipu_hs : Bitfield<18, 1> { };
	};

	template <unsigned OFF>
	struct Ccgr : Register<0x68 + OFF*4, 32> {};

	Ccm(Genode::addr_t const mmio_base) : Genode::Mmio(mmio_base)
	{

		write<Ccgr<0> >(~0UL);
		write<Ccgr<1> >(~0UL);
		write<Ccgr<2> >(0xffff7fff);
		write<Ccgr<5> >(~0UL);
		write<Ccgr<6> >(~0UL);
		write<Cdcdr>(0x14370092);
		write<Clpcr::Bypass_ipu_hs>(0);
		write<Ccdr::Ipu_hs_mask>(0);
		write<Cscmr2>(0xa2b32f0b);
	}

};

#endif /* _CCM_H_ */
