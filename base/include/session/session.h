/*
 * \brief  Session
 * \author Norman Feske
 * \date   2011-05-15
 */

/*
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__SESSION_H_
#define _INCLUDE__SESSION_H_

/*
 * Each session interface declares an RPC interface and, therefore, relies on
 * the RPC framework. By including 'base/rpc.h' here, we relieve the interfaces
 * from including 'base/rpc.h' in addition to 'session/session.h'.
 */
#include <base/rpc.h>

namespace Genode {

	/**
	 * Base class of session interfaces
	 *
	 * Each session interface must implement the function 'service_name'
	 * ! static const char *service_name();
	 * This function returns the name of the service provided via the session
	 * interface.
	 */
	class Session { };
}


#endif /* _INCLUDE__SESSION_H_ */
