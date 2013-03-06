/*
 * \brief  Frame-buffer driver for Freescale's i.MX53
 * \author Nikolay Golikov
 * \date   2012-06-21
 */

/* Genode includes */
#include <drivers/board.h>
#include <os/attached_io_mem_dataspace.h>
#include <io_mem_session/connection.h>
#include <gpio_session/connection.h>
#include <util/mmio.h>

/* local includes */
#include <ipu.h>
#include <src.h>
#include <ccm.h>
#include <pwm.h>


namespace Framebuffer {
	using namespace Genode;
	class Driver;
};


class Framebuffer::Driver
{
	private:

		size_t               _size;
		Dataspace_capability _ds;
		addr_t               _phys_base;

		/* System reset controller registers */
		Attached_io_mem_dataspace _src_mmio;
		Src                       _src;

		/* Clocks control module */
		Attached_io_mem_dataspace _ccm_mmio;
		Ccm                       _ccm;

		Attached_io_mem_dataspace _iomuxc_mmio;
		Iomuxc                    _iomuxc;

		Attached_io_mem_dataspace _pwm_mmio;
		Pwm                       _pwm;

		/* Image processing unit memory */
		Attached_io_mem_dataspace _ipu_mmio;
		Ipu                       _ipu;

		//Gpio::Connection          _gpio;

	public:

		enum {
			REFRESH          = 60,
			WIDTH            = 1024,
			HEIGHT           = 768,
			PIX_CLK          = 29850,
			ROUND_PIX_CLK    = 38000,
			LEFT_MARGIN      = 89,
			RIGHT_MARGIN     = 104,
			UPPER_MARGIN     = 10,
			LOWER_MARGIN     = 10,
			VSYNC_LEN        = 10,
			HSYNC_LEN        = 10,
			BYTES_PER_PIXEL  = 2,
			FRAMEBUFFER_SIZE = WIDTH * HEIGHT * BYTES_PER_PIXEL,

			LCD_BL_GPIO      = 173,
			LCD_CONT_GPIO    = 1,
		};


		Driver()
		: _ds(env()->ram_session()->alloc(FRAMEBUFFER_SIZE, false)),
		  _phys_base(Dataspace_client(_ds).phys_addr()),
		  _src_mmio(Board::SRC_BASE, Board::SRC_SIZE),
		  _src((addr_t)_src_mmio.local_addr<void>()),
		  _ccm_mmio(Board::CCM_BASE, Board::CCM_SIZE),
		  _ccm((addr_t)_ccm_mmio.local_addr<void>()),
		  _iomuxc_mmio(Board::IOMUXC_BASE, Board::IOMUXC_SIZE),
		  _iomuxc((addr_t)_iomuxc_mmio.local_addr<void>()),
		  _pwm_mmio(Board::PWM2_BASE, Board::PWM2_SIZE),
		  _pwm((addr_t)_pwm_mmio.local_addr<void>()),
		  _ipu_mmio(Board::IPU_BASE, Board::IPU_SIZE),
		  _ipu((addr_t)_ipu_mmio.local_addr<void>())
		{
			_ipu.init(WIDTH, HEIGHT, WIDTH * BYTES_PER_PIXEL, _phys_base);

			/* Turn on lcd power */
			_iomuxc.enable_di1();
			_pwm.enable_display();
			//_gpio.direction_output(LCD_BL_GPIO, true);
			//_gpio.direction_output(LCD_CONT_GPIO, true);
		}

		Dataspace_capability dataspace() { return _ds; }

		void overlay(addr_t phys_base) { _ipu.overlay_base(phys_base); }
};

