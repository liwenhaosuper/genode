/*
 * \brief  IOMUX controller
 * \author Stefan Kalkowski
 * \date   2013-03-01
 */

#ifndef _IOMUXC_H_
#define _IOMUXC_H_

/* Genode includes */
#include <util/mmio.h>
#include <drivers/board.h>
#include <os/attached_io_mem_dataspace.h>


using namespace Genode;


struct Iomuxc : Genode::Mmio
{

	struct Gpr2 : Register<0x8,32>
	{
		struct Ch1_mode         : Bitfield<2, 2> { enum { ROUTED_TO_DI1 = 0x3 };    };
		struct Data_width_ch1   : Bitfield<7, 1> { enum { PX_18_BITS, PX_24_BITS }; };
		struct Bit_mapping_ch1  : Bitfield<8, 1> { enum { SPWG, JEIDA };            };
		struct Di1_vs_polarity  : Bitfield<10,1> { };
	};

	struct Key_col3 : Register<0x3c, 32> {};
	struct Key_row3 : Register<0x40, 32> {};

	struct Eim_a24 : Register<0x15c, 32> { };

	template <unsigned OFF>
	struct Sw_mux_ctl_pad_gpio : Register<0x314 + OFF*4, 32> { };

	struct Sw_pad_ctl_pad_key_col3 : Register<0x364, 32> { };
	struct Sw_pad_ctl_pad_key_row3 : Register<0x368, 32> { };

	struct Sw_pad_ctl_pad_eim_a24 : Register<0x4a8, 32> { };

	template <unsigned OFF>
	struct Sw_pad_ctl_pad_gpio : Register<0x6a4 + OFF*4, 32> { };

	struct I2c2_ipp_scl_in_select_input : Register<0x81c, 32> { };
	struct I2c2_ipp_sda_in_select_input : Register<0x820, 32> { };
	struct I2c3_ipp_scl_in_select_input : Register<0x824, 32> { };
	struct I2c3_ipp_sda_in_select_input : Register<0x828, 32> { };

	Iomuxc(Genode::addr_t mmio_base) : Genode::Mmio(mmio_base)
	{
		write<Eim_a24>(1);
		write<Sw_pad_ctl_pad_eim_a24>(0);

		write<Sw_mux_ctl_pad_gpio<1> >(0x4);
		write<Sw_pad_ctl_pad_gpio<1> >(0x0);

		/* I2C-2 stuff */
		write<Key_col3>(0x14);
		write<I2c2_ipp_scl_in_select_input>(0);
		write<Sw_pad_ctl_pad_key_col3>(0x12d);
		write<Key_row3>(0x14);
		write<I2c2_ipp_sda_in_select_input>(0);
		write<Sw_pad_ctl_pad_key_row3>(0x12d);

		/* I2C-3 stuff */
		write<Sw_mux_ctl_pad_gpio<3> >(0x12);
		write<I2c3_ipp_scl_in_select_input>(0x1);
		write<Sw_pad_ctl_pad_gpio<3> >(0x12d);
		write<Sw_mux_ctl_pad_gpio<4> >(0x12);
		write<I2c3_ipp_sda_in_select_input>(0x1);
		write<Sw_pad_ctl_pad_gpio<4> >(0x12d);
	}

	void enable_di1()
	{
		write<Gpr2::Di1_vs_polarity>(1);
		write<Gpr2::Data_width_ch1>(Gpr2::Data_width_ch1::PX_18_BITS);
		write<Gpr2::Bit_mapping_ch1>(Gpr2::Bit_mapping_ch1::SPWG);
		write<Gpr2::Ch1_mode>(Gpr2::Ch1_mode::ROUTED_TO_DI1);
	}
};

#endif /* _IOMUXC_H_ */
