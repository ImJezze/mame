// license:GPL-2.0+
// copyright-holders:Dirk Best, R. Belmont
/***************************************************************************

    AT&T Unix PC series

    Skeleton driver by Dirk Best and R. Belmont

    DIVS instruction at 0x801112 (the second time) causes a divide-by-zero
    exception the system isn't ready for due to word at 0x5EA6 being zero.

    Code might not get there if the attempted FDC boot succeeds; FDC hookup
    probably needs help.  2797 isn't asserting DRQ?

***************************************************************************/


#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "bus/centronics/ctronics.h"
#include "bus/rs232/rs232.h"
#include "machine/6850acia.h"
#include "machine/74259.h"
#include "machine/bankdev.h"
#include "machine/output_latch.h"
#include "machine/ram.h"
//#include "machine/tc8250.h"
//#include "machine/wd1010.h"
#include "machine/wd_fdc.h"
#include "machine/z80sio.h"
#include "emupal.h"
#include "screen.h"

#include "unixpc.lh"


/***************************************************************************
    DRIVER STATE
***************************************************************************/

class unixpc_state : public driver_device
{
public:
	unixpc_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_gcr(*this, "gcr"),
		m_tcr(*this, "tcr"),
		m_ram(*this, RAM_TAG),
		m_wd2797(*this, "wd2797"),
		m_floppy(*this, "wd2797:0:525dd"),
		m_ramrombank(*this, "ramrombank"),
		m_mapram(*this, "mapram"),
		m_videoram(*this, "videoram")
	{ }

	void unixpc(machine_config &config);

private:
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	virtual void machine_start() override;
	virtual void machine_reset() override;

	DECLARE_READ16_MEMBER(line_printer_r);
	DECLARE_WRITE16_MEMBER(disk_control_w);
	DECLARE_WRITE16_MEMBER(gcr_w);
	DECLARE_WRITE_LINE_MEMBER(romlmap_w);
	DECLARE_WRITE_LINE_MEMBER(error_enable_w);
	DECLARE_WRITE_LINE_MEMBER(parity_enable_w);
	DECLARE_WRITE_LINE_MEMBER(bpplus_w);
	DECLARE_READ16_MEMBER(ram_mmu_r);
	DECLARE_WRITE16_MEMBER(ram_mmu_w);
	DECLARE_READ16_MEMBER(gsr_r);
	DECLARE_WRITE16_MEMBER(tcr_w);
	DECLARE_READ16_MEMBER(tsr_r);
	DECLARE_READ16_MEMBER(rtc_r);
	DECLARE_WRITE16_MEMBER(rtc_w);
	DECLARE_READ16_MEMBER(diskdma_size_r);
	DECLARE_WRITE16_MEMBER(diskdma_size_w);
	DECLARE_WRITE16_MEMBER(diskdma_ptr_w);

	DECLARE_WRITE_LINE_MEMBER(wd2797_intrq_w);
	DECLARE_WRITE_LINE_MEMBER(wd2797_drq_w);

	void ramrombank_map(address_map &map);
	void unixpc_mem(address_map &map);

private:
	required_device<cpu_device> m_maincpu;
	required_device<ls259_device> m_gcr;
	required_device<ls259_device> m_tcr;
	required_device<ram_device> m_ram;
	required_device<wd2797_device> m_wd2797;
	required_device<floppy_image_device> m_floppy;
	required_device<address_map_bank_device> m_ramrombank;

	required_shared_ptr<uint16_t> m_mapram;
	required_shared_ptr<uint16_t> m_videoram;

	uint16_t *m_ramptr;
	uint32_t m_ramsize;
	uint16_t m_diskdmasize;
	uint32_t m_diskdmaptr;
	bool m_fdc_intrq;
};


/***************************************************************************
    MEMORY
***************************************************************************/

WRITE16_MEMBER(unixpc_state::gcr_w)
{
	m_gcr->write_bit(offset >> 11, BIT(data, 15));
}

WRITE_LINE_MEMBER(unixpc_state::romlmap_w)
{
	m_ramrombank->set_bank(state ? 1 : 0);
}

READ16_MEMBER(unixpc_state::ram_mmu_r)
{
	// TODO: MMU translation
	if (offset > m_ramsize)
	{
		return 0xffff;
	}
	return m_ramptr[offset];
}

WRITE16_MEMBER(unixpc_state::ram_mmu_w)
{
	// TODO: MMU translation
	if (offset < m_ramsize)
	{
		COMBINE_DATA(&m_ramptr[offset]);
	}
}

void unixpc_state::machine_start()
{
	m_ramptr = (uint16_t *)m_ram->pointer();
	m_ramsize = m_ram->size();
}

void unixpc_state::machine_reset()
{
	// force ROM into lower mem on reset
	m_ramrombank->set_bank(0);

	// reset cpu so that it can pickup the new values
	m_maincpu->reset();
}

WRITE_LINE_MEMBER(unixpc_state::error_enable_w)
{
	logerror("error_enable_w: %d\n", state);
}

WRITE_LINE_MEMBER(unixpc_state::parity_enable_w)
{
	logerror("parity_enable_w: %d\n", state);
}

WRITE_LINE_MEMBER(unixpc_state::bpplus_w)
{
	logerror("bpplus_w: %d\n", state);
}

/***************************************************************************
    MISC
***************************************************************************/

READ16_MEMBER(unixpc_state::gsr_r)
{
	return 0;
}

WRITE16_MEMBER(unixpc_state::tcr_w)
{
	m_tcr->write_bit(offset >> 11, BIT(data, 14));
}

READ16_MEMBER(unixpc_state::tsr_r)
{
	return 0;
}

READ16_MEMBER(unixpc_state::rtc_r)
{
	return 0;
}

WRITE16_MEMBER(unixpc_state::rtc_w)
{
	logerror("rtc_w: %04x\n", data);
}

READ16_MEMBER(unixpc_state::line_printer_r)
{
	uint16_t data = 0;

	data |= 1; // no dial tone detected
	data |= 1 << 1; // no parity error
	data |= 0 << 2; // hdc intrq
	data |= m_fdc_intrq ? 1<<3 : 0<<3;

	//logerror("line_printer_r: %04x\n", data);

	return data;
}

/***************************************************************************
    DMA
***************************************************************************/

READ16_MEMBER(unixpc_state::diskdma_size_r)
{
	return m_diskdmasize;
}

WRITE16_MEMBER(unixpc_state::diskdma_size_w)
{
	COMBINE_DATA( &m_diskdmasize );
	logerror("%x to disk DMA size\n", data);
}

WRITE16_MEMBER(unixpc_state::diskdma_ptr_w)
{
	if (offset >= 0x2000)
	{
		// set top 4 bytes
		m_diskdmaptr &= 0xff;
		m_diskdmaptr |= (offset << 8);
	}
	else
	{
		m_diskdmaptr &= 0xffff00;
		m_diskdmaptr |= (offset & 0xff);
	}

	logerror("diskdma_ptr_w: wrote at %x, ptr now %x\n", offset<<1, m_diskdmaptr);
}

/***************************************************************************
    FLOPPY
***************************************************************************/

WRITE16_MEMBER(unixpc_state::disk_control_w)
{
	logerror("disk_control_w: %04x\n", data);

	m_floppy->mon_w(!BIT(data, 5));

	// bit 6 = floppy selected / not selected
	if (BIT(data, 6))
		m_wd2797->set_floppy(m_floppy);
	else
		m_wd2797->set_floppy(nullptr);
}

WRITE_LINE_MEMBER(unixpc_state::wd2797_intrq_w)
{
	logerror("wd2797_intrq_w: %d\n", state);
	m_fdc_intrq = state;
}

WRITE_LINE_MEMBER(unixpc_state::wd2797_drq_w)
{
	logerror("wd2797_drq_w: %d\n", state);
}


/***************************************************************************
    VIDEO
***************************************************************************/

uint32_t unixpc_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	for (int y = 0; y < 348; y++)
		for (int x = 0; x < 720/16; x++)
			for (int b = 0; b < 16; b++)
				bitmap.pix16(y, x * 16 + b) = BIT(m_videoram[y * (720/16) + x], b);

	return 0;
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

void unixpc_state::unixpc_mem(address_map &map)
{
	map(0x000000, 0x3fffff).m(m_ramrombank, FUNC(address_map_bank_device::amap16));
	map(0x400000, 0x4007ff).ram().share("mapram");
	map(0x410000, 0x410001).r(FUNC(unixpc_state::gsr_r));
	map(0x420000, 0x427fff).ram().share("videoram");
	map(0x450000, 0x450001).r(FUNC(unixpc_state::tsr_r));
	map(0x460000, 0x460001).rw(FUNC(unixpc_state::diskdma_size_r), FUNC(unixpc_state::diskdma_size_w));
	map(0x470000, 0x470001).r(FUNC(unixpc_state::line_printer_r));
	map(0x480000, 0x480001).w(FUNC(unixpc_state::rtc_w));
	map(0x490000, 0x490001).select(0x7000).w(FUNC(unixpc_state::tcr_w));
	map(0x4a0000, 0x4a0000).w("mreg", FUNC(output_latch_device::bus_w));
	map(0x4d0000, 0x4d7fff).w(FUNC(unixpc_state::diskdma_ptr_w));
	map(0x4e0000, 0x4e0001).w(FUNC(unixpc_state::disk_control_w));
	map(0x4f0001, 0x4f0001).w("printlatch", FUNC(output_latch_device::bus_w));
	//map(0xe00000, 0xe0000f).rw("hdc", FUNC(wd1010_device::read), FUNC(wd1010_device::write)).umask16(0x00ff);
	map(0xe10000, 0xe10007).rw(m_wd2797, FUNC(wd_fdc_device_base::read), FUNC(wd_fdc_device_base::write)).umask16(0x00ff);
	map(0xe30000, 0xe30001).r(FUNC(unixpc_state::rtc_r));
	map(0xe40000, 0xe40001).select(0x7000).w(FUNC(unixpc_state::gcr_w));
	map(0xe50000, 0xe50007).rw("mpsc", FUNC(upd7201_new_device::cd_ba_r), FUNC(upd7201_new_device::cd_ba_w)).umask16(0x00ff);
	map(0xe70000, 0xe70003).rw("kbc", FUNC(acia6850_device::read), FUNC(acia6850_device::write)).umask16(0xff00);
	map(0x800000, 0x803fff).mirror(0x7fc000).rom().region("bootrom", 0);
}

void unixpc_state::ramrombank_map(address_map &map)
{
	map(0x000000, 0x3fffff).rom().region("bootrom", 0);
	map(0x400000, 0x7fffff).rw(FUNC(unixpc_state::ram_mmu_r), FUNC(unixpc_state::ram_mmu_w));
}

/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( unixpc )
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static void unixpc_floppies(device_slot_interface &device)
{
	device.option_add("525dd", FLOPPY_525_DD);
}

MACHINE_CONFIG_START(unixpc_state::unixpc)
	// basic machine hardware
	MCFG_DEVICE_ADD("maincpu", M68010, 40_MHz_XTAL / 4)
	MCFG_DEVICE_PROGRAM_MAP(unixpc_mem)

	LS259(config, m_gcr); // 7K
	m_gcr->q_out_cb<0>().set(FUNC(unixpc_state::error_enable_w));
	m_gcr->q_out_cb<1>().set(FUNC(unixpc_state::parity_enable_w));
	m_gcr->q_out_cb<2>().set(FUNC(unixpc_state::bpplus_w));
	m_gcr->q_out_cb<3>().set(FUNC(unixpc_state::romlmap_w));

	LS259(config, m_tcr); // 10K

	output_latch_device &mreg(OUTPUT_LATCH(config, "mreg"));
	mreg.bit_handler<0>().set_output("led_0").invert();
	mreg.bit_handler<1>().set_output("led_1").invert();
	mreg.bit_handler<2>().set_output("led_2").invert();
	mreg.bit_handler<3>().set_output("led_3").invert();
	// bit 4 (D12) = 0 = modem baud rate from UART clock inputs, 1 = baud from programmable timer
	mreg.bit_handler<5>().set("printer", FUNC(centronics_device::write_strobe)).invert();
	// bit 6 (D14) = 0 for disk DMA write, 1 for disk DMA read
	// bit 7 (D15) = VBL ack (must go high-low-high to ack)

	// video hardware
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_UPDATE_DRIVER(unixpc_state, screen_update)
	MCFG_SCREEN_RAW_PARAMS(40_MHz_XTAL / 2, 896, 0, 720, 367, 0, 348)
	MCFG_SCREEN_PALETTE("palette")
	// vsync should actually last 17264 pixels

	config.set_default_layout(layout_unixpc);

	MCFG_PALETTE_ADD_MONOCHROME("palette")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1M")
	MCFG_RAM_EXTRA_OPTIONS("2M")

	// RAM/ROM bank
	MCFG_DEVICE_ADD("ramrombank", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(ramrombank_map)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_BIG)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(16)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x400000)

	// floppy
	MCFG_DEVICE_ADD("wd2797", WD2797, 40_MHz_XTAL / 40) // 1PCK (CPU clock) divided by custom DMA chip
	MCFG_WD_FDC_INTRQ_CALLBACK(WRITELINE(*this, unixpc_state, wd2797_intrq_w))
	MCFG_WD_FDC_DRQ_CALLBACK(WRITELINE(*this, unixpc_state, wd2797_drq_w))
	MCFG_FLOPPY_DRIVE_ADD("wd2797:0", unixpc_floppies, "525dd", floppy_image_device::default_floppy_formats)

	MCFG_DEVICE_ADD("mpsc", UPD7201_NEW, 19.6608_MHz_XTAL / 8)
	MCFG_Z80SIO_OUT_TXDA_CB(WRITELINE("rs232", rs232_port_device, write_txd))
	MCFG_Z80SIO_OUT_DTRA_CB(WRITELINE("rs232", rs232_port_device, write_dtr))
	MCFG_Z80SIO_OUT_RTSA_CB(WRITELINE("rs232", rs232_port_device, write_rts))

	MCFG_DEVICE_ADD("kbc", ACIA6850, 0)

	// TODO: HDC
	//MCFG_DEVICE_ADD("hdc", WD1010, 40_MHz_XTAL / 8)

	// TODO: RTC
	//MCFG_DEVICE_ADD("rtc", TC8250, 32.768_kHz_XTAL)

	MCFG_DEVICE_ADD("rs232", RS232_PORT, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(WRITELINE("mpsc", upd7201_new_device, rxa_w))
	MCFG_RS232_DSR_HANDLER(WRITELINE("mpsc", upd7201_new_device, dcda_w))
	MCFG_RS232_CTS_HANDLER(WRITELINE("mpsc", upd7201_new_device, ctsa_w))

	MCFG_DEVICE_ADD("printer", CENTRONICS, centronics_devices, nullptr)
	MCFG_CENTRONICS_OUTPUT_LATCH_ADD("printlatch", "printer")
MACHINE_CONFIG_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

// ROMs were provided by Michael Lee und imaged by Philip Pemberton
ROM_START( 3b1 )
	ROM_REGION16_BE(0x400000, "bootrom", 0)
	ROM_LOAD16_BYTE("72-00617.15c", 0x000000, 0x002000, CRC(4e93ff40) SHA1(1a97c8d32ec862f7f5fa1032f1688b76ea0672cc))
	ROM_LOAD16_BYTE("72-00616.14c", 0x000001, 0x002000, CRC(c61f7ae0) SHA1(ab3ac29935a2a587a083c4d175a5376badd39058))
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

//    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT   CLASS         INIT        COMPANY  FULLNAME  FLAGS
COMP( 1985, 3b1,  0,      0,      unixpc,  unixpc, unixpc_state, empty_init, "AT&T",  "3B1",    MACHINE_NOT_WORKING | MACHINE_NO_SOUND )
