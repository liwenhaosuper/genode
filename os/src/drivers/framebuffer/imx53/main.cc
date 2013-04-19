/*
 * \brief  Frame-buffer driver for the i.MX53
 * \author Nikolay Golikov
 * \date   2012-06-21
 */

/* Genode includes */
#include <imx_framebuffer_session/imx_framebuffer_session.h>
#include <cap_session/connection.h>
#include <dataspace/client.h>
#include <base/printf.h>
#include <base/sleep.h>
#include <os/static_root.h>

/* local includes */
#include <driver.h>

namespace Framebuffer {
	using namespace Genode;
	class Session_component;
};


class Framebuffer::Session_component :
	public Genode::Rpc_object<Framebuffer::Imx_session>
{
	private:

		Driver              &_driver;

	public:

		Session_component(Driver &driver) : _driver(driver) { }


		/**************************************
		 **  Framebuffer::session interface  **
		 **************************************/

		Dataspace_capability dataspace() { return _driver.dataspace(); }
		void release() { }
		Mode mode() const {
			return Mode(Driver::WIDTH, Driver::HEIGHT, Mode::RGB565); }
		void mode_sigh(Genode::Signal_context_capability) { }
		void refresh(int, int, int, int) { }
		void overlay(addr_t phys_base, int x, int y, int alpha) {
			_driver.overlay(phys_base, x, y, alpha); }
};

int main(int, char **)
{
	Genode::printf("Starting i.MX53 framebuffer driver\n");

	using namespace Framebuffer;

	static Driver driver;

	enum { STACK_SIZE = 4096 };
	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "fb_ep");

	static Session_component fb_session(driver);
	static Static_root<Framebuffer::Imx_session> fb_root(ep.manage(&fb_session));

	env()->parent()->announce(ep.manage(&fb_root));

	sleep_forever();
	return 0;
}
