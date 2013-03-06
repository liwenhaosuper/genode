/*
 * \brief  Input-driver
 * \author Stefan Kalkowski
 * \date   2013-03-15
 */

/*
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <base/env.h>
#include <base/thread.h>
#include <base/signal.h>
#include <drivers/board.h>
#include <io_mem_session/connection.h>
#include <gpio_session/connection.h>
#include <input/event_queue.h>
#include <input/event.h>
#include <input/keycodes.h>
#include <i2c.h>

namespace Input {

	class Driver : Genode::Thread<8192>
	{
		private:

			enum Gpio_irqs {
				GPIO_TOUCH  = 84,
				GPIO_BUTTON = 132,
			};

			enum I2c_addresses {
				I2C_ADDR_TS = 0x4,
				I2C_ADDR_TB = 0x5a,
			};

			enum Touch_button_thresholds {
				TOUCH_THRES   = 0x8,
				RELEASE_THRES = 0x5,
			};

			enum Button_state { PRESSED, RELEASED };

			Event_queue                      &_ev_queue;
			Genode::Io_mem_connection         _i2c_2_io_mem;
			Genode::Io_mem_connection         _i2c_3_io_mem;
			Gpio::Connection                  _gpio;
			I2c::I2c                          _i2c_2;
			I2c::I2c                          _i2c_3;
			Genode::Signal_receiver           _receiver;
			Genode::Signal_context            _ts_rx;
			Genode::Signal_context_capability _ts_sig_cap;
			Genode::Signal_context            _bt_rx;
			Genode::Signal_context_capability _bt_sig_cap;
			Button_state                      _finger;
			int                               _button;

		public:

			Driver(Event_queue &ev_queue)
			: _ev_queue(ev_queue),
			  _i2c_2_io_mem(Genode::Board::I2C_2_BASE, Genode::Board::I2C_2_SIZE),
			  _i2c_3_io_mem(Genode::Board::I2C_3_BASE, Genode::Board::I2C_3_SIZE),
			  _i2c_2((Genode::addr_t)Genode::env()->rm_session()->attach(_i2c_2_io_mem.dataspace()), Genode::Board::I2C_2_IRQ),
			  _i2c_3((Genode::addr_t)Genode::env()->rm_session()->attach(_i2c_3_io_mem.dataspace()), Genode::Board::I2C_3_IRQ),
			  _ts_sig_cap(_receiver.manage(&_ts_rx)),
			  _bt_sig_cap(_receiver.manage(&_bt_rx)),
			  _finger(RELEASED),
			  _button(0)
			{
				using namespace Genode;

				_gpio.direction_output(GPIO_TOUCH, false);
				_gpio.dataout(GPIO_TOUCH, true);
				_gpio.direction_input(GPIO_TOUCH);

				_gpio.direction_output(GPIO_BUTTON, false);
				_gpio.dataout(GPIO_BUTTON, true);
				_gpio.direction_input(GPIO_BUTTON);

				_gpio.irq_sigh(_ts_sig_cap, GPIO_TOUCH);
				_gpio.irq_sigh(_bt_sig_cap, GPIO_BUTTON);

				_gpio.irq_enable(GPIO_TOUCH, true);

				_gpio.falling_detect(GPIO_BUTTON, true);
				_gpio.irq_enable(GPIO_BUTTON, true);

				/* ask for touchscreen firmware version */
				static const Genode::uint8_t cmd[10] = { 0x03, 0x03, 0xa, 0x01, 0x41 };
				_i2c_3.send(I2C_ADDR_TS, cmd, sizeof(cmd));


				/* initialize mpr121 touch button device */
				static Genode::uint8_t commands[][2] = {
					{0x41, 0x8 }, {0x42, 0x5 }, {0x43, 0x8 },
					{0x44, 0x5 }, {0x45, 0x8 }, {0x46, 0x5 },
					{0x47, 0x8 }, {0x48, 0x5 }, {0x49, 0x8 },
					{0x4a, 0x5 }, {0x4b, 0x8 }, {0x4c, 0x5 },
					{0x4d, 0x8 }, {0x4e, 0x5 }, {0x4f, 0x8 },
					{0x50, 0x5 }, {0x51, 0x8 }, {0x52, 0x5 },
					{0x53, 0x8 }, {0x54, 0x5 }, {0x55, 0x8 },
					{0x56, 0x5 }, {0x57, 0x8 }, {0x58, 0x5 },
					{0x59, 0x8 }, {0x5a, 0x5 }, {0x2b, 0x1 },
					{0x2c, 0x1 }, {0x2d, 0x0 }, {0x2e, 0x0 },
					{0x2f, 0x1 }, {0x30, 0x1 }, {0x31, 0xff},
					{0x32, 0x2 }, {0x5d, 0x4 }, {0x5c, 0xb },
					{0x7b, 0xb }, {0x7d, 0xc9}, {0x7e, 0x82},
					{0x7f, 0xb4}, {0x5e, 0x84}};
				for (unsigned i = 0; i < sizeof(commands)/2; i++)
					_i2c_2.send(I2C_ADDR_TB, commands[i], 2);

				PDBG("start thread");
				start();
			}


			void entry()
			{
				Genode::uint8_t buf[10];
				while (true) {
					Genode::Signal sig = _receiver.wait_for_signal();
					if (sig.context() == &_ts_rx) {
						_i2c_3.recv(I2C_ADDR_TS, buf, sizeof(buf));

						/* ignore all events except of multitouch*/
						if (buf[0] != 4)
							continue;

						int x = (buf[3] << 8) | buf[2];
						int y = (buf[5] << 8) | buf[4];

						Genode::uint8_t state = buf[1];
						bool valid = state & (1 << 7);
						int  id    = (state >> 2) & 0xf;
						int  down  = state & 1;

						if (!valid || id > 5)
							continue; /* invalid point */

						x = 102400 / (3276700 / x);
						y = 76800 / (3276700 / y);

						/* motion event */
						_ev_queue.add(Input::Event(Input::Event::MOTION,
												   0, x, y, 0, 0));

						/* button event */
						if ((down  && (_finger == RELEASED)) ||
							(!down && (_finger == PRESSED))) {
							_ev_queue.add(Input::Event(down ? Input::Event::PRESS
													   : Input::Event::RELEASE,
													   Input::BTN_LEFT, 0, 0, 0, 0));
							_finger = down ? PRESSED : RELEASED;
						}
					} else if (sig.context() == &_bt_rx) {
						buf[0] = 0;
						_i2c_2.send(I2C_ADDR_TB, buf, 1);
						_i2c_2.recv(I2C_ADDR_TB, buf, 1);
						switch (buf[0]) {
						case 0: /* release   */
							_ev_queue.add(Input::Event(Input::Event::RELEASE,
							                           _button, 0, 0, 0, 0));
							break;
						case 1: /* back      */
							_ev_queue.add(Input::Event(Input::Event::PRESS,
							                           Input::KEY_BACK, 0, 0, 0, 0));
							_button = Input::KEY_BACK;
							break;
						case 2: /* home      */
							_ev_queue.add(Input::Event(Input::Event::PRESS,
							                           Input::KEY_HOME, 0, 0, 0, 0));
							_button = Input::KEY_HOME;
							break;
						case 4: /* menu      */
							_ev_queue.add(Input::Event(Input::Event::PRESS,
							                           Input::KEY_MENU, 0, 0, 0, 0));
							_button = Input::KEY_MENU;
							break;
						case 8: /* power     */
							_ev_queue.add(Input::Event(Input::Event::PRESS,
							                           Input::KEY_POWER, 0, 0, 0, 0));
							_button = Input::KEY_POWER;
							break;
						}
					}
				}
			}
	};
}

#endif /* _DRIVER_H_ */
