/*
 * \brief  i.MX53 specific framebuffer session extension
 * \author Stefan Kalkowski
 * \date   2013-02-26
 */

/*
 * Copyright (C) 2011-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__IMX_FRAMEBUFFER_SESSION__IMX_FRAMEBUFFER_SESSION_H_
#define _INCLUDE__IMX_FRAMEBUFFER_SESSION__IMX_FRAMEBUFFER_SESSION_H_

#include <base/capability.h>
#include <base/rpc.h>
#include <framebuffer_session/framebuffer_session.h>

namespace Framebuffer {

	struct Imx_session : Session
	{
		virtual ~Imx_session() { }

		virtual void overlay(Genode::addr_t phys_base) = 0;


		/*********************
		 ** RPC declaration **
		 *********************/

		GENODE_RPC(Rpc_overlay, void, overlay, Genode::addr_t);

		GENODE_RPC_INTERFACE_INHERIT(Session, Rpc_overlay);
	};
}

#endif /* _INCLUDE__IMX_FRAMEBUFFER_SESSION__IMX_FRAMEBUFFER_SESSION_H_ */