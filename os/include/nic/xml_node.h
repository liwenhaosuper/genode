/*
 * \brief  Xml-node routines used internally in NIC drivers
 * \author Sebastian Sumpf
 * \author Stefan Kalkowski
 * \date   2012-10-22
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__NIC__XML_NODE_H_
#define _INCLUDE__NIC__XML_NODE_H_

#include <nic_session/nic_session.h>
#include <util/string.h>

namespace Genode {

	/**
	 * Convert ASCII string to mac address
	 */
	template <>
	inline size_t ascii_to<Nic::Mac_address>(char const *s,
	                                         Nic::Mac_address* mac, unsigned)
	{
		enum {
			HEX          = true,
			MAC_CHAR_LEN = 17,   /* 12 number and 6 colons */
			MAC_SIZE     = 6,
		};

		if(strlen(s) < MAC_CHAR_LEN)
			throw -1;

		char mac_str[6];
		for (int i = 0; i < MAC_SIZE; i++) {
			int hi = i * 3;
			int lo = hi + 1;

			if (!is_digit(s[hi], HEX) || !is_digit(s[lo], HEX))
				throw -1;

			mac_str[i] = (digit(s[hi], HEX) << 4) | digit(s[lo], HEX);
		}

		Genode::memcpy(mac->addr, mac_str, MAC_SIZE);

		return MAC_CHAR_LEN;
	}
}

#endif /* _INCLUDE__NIC__XML_NODE_H_ */
