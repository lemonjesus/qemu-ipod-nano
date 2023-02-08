#ifndef HW_ARM_IPOD_NANO3G_H
#define HW_ARM_IPOD_NANO3G_H

#include "qemu-common.h"
#include "exec/hwaddr.h"
#include "ui/console.h"
#include "hw/boards.h"
#include "hw/arm/boot.h"
#include "hw/intc/pl192.h"
#include "hw/arm/ipod_nano3g_timer.h"
#include "hw/arm/ipod_nano3g_clock.h"
#include "hw/arm/ipod_nano3g_spi.h"
#include "hw/arm/ipod_nano3g_aes.h"
#include "hw/arm/ipod_nano3g_sha1.h"
#include "hw/arm/ipod_nano3g_8702_engine.h"
#include "hw/arm/ipod_nano3g_usb_otg.h"
#include "hw/arm/ipod_nano3g_nand.h"
#include "hw/arm/ipod_nano3g_nand_ecc.h"
#include "hw/arm/ipod_nano3g_pcf50633_pmu.h"
#include "hw/arm/ipod_nano3g_adm.h"
#include "hw/arm/ipod_nano3g_sysic.h"
#include "hw/arm/ipod_nano3g_chipid.h"
#include "hw/arm/ipod_nano3g_tvout.h"
#include "hw/arm/ipod_nano3g_lcd.h"
#include "hw/arm/ipod_nano3g_gpio.h"
#include "hw/arm/ipod_nano3g_sdio.h"
#include "hw/i2c/ipod_nano3g_i2c.h"
#include "hw/arm/ipod_nano3g_jpeg.h"
#include "cpu.h"

#define TYPE_IPOD_NANO3G "iPod-Nano3G"

#define TYPE_IPOD_NANO3G_MACHINE   MACHINE_TYPE_NAME(TYPE_IPOD_NANO3G)
#define IPOD_NANO3G_MACHINE(obj) \
    OBJECT_CHECK(IPodNano3GMachineState, (obj), TYPE_IPOD_NANO3G_MACHINE)

// VIC
#define S5L8702_VIC_N	  2
#define S5L8702_VIC_SIZE  32

#define S5L8702_TIMER1_IRQ 0x7

#define S5L8702_SPI0_IRQ 0x9
#define S5L8702_SPI1_IRQ 0xA
#define S5L8702_SPI2_IRQ 0xB

#define S5L8702_LCD_IRQ 0xD

#define S5L8702_DMAC0_IRQ 0x10
#define S5L8702_DMAC1_IRQ 0x11

#define S5L8702_I2C0_IRQ 0x15
#define S5L8702_I2C1_IRQ 0x16

#define S5L8702_ADM_IRQ 0x25

#define S5L8702_NAND_ECC_IRQ 0x2B

#define S5L8702_TVOUT_SDO_IRQ 0x1E
#define S5L8702_TVOUT_MIXER_IRQ 0x26

// GPIO interrupts
#define S5L8702_GPIO_G0_IRQ 0x21
#define S5L8702_GPIO_G1_IRQ 0x20
#define S5L8702_GPIO_G2_IRQ 0x1F
#define S5L8702_GPIO_G3_IRQ 0x03
#define S5L8702_GPIO_G4_IRQ 0x02
#define S5L8702_GPIO_G5_IRQ 0x01
#define S5L8702_GPIO_G6_IRQ 0x00

const int S5L8702_GPIO_IRQS[7] = { S5L8702_GPIO_G0_IRQ, S5L8702_GPIO_G1_IRQ, S5L8702_GPIO_G2_IRQ, S5L8702_GPIO_G3_IRQ, S5L8702_GPIO_G4_IRQ, S5L8702_GPIO_G5_IRQ, S5L8702_GPIO_G6_IRQ };

// memory addresses
#define IPOD_NANO3G_PHYS_BASE (0xc0000000)
#define IBOOT_BASE 0x18000000
#define LLB_BASE 0x22000000
#define SRAM1_MEM_BASE 0x22020000
#define CLOCK0_MEM_BASE 0x38100000
#define CLOCK1_MEM_BASE 0x3C500000
#define SYSIC_MEM_BASE 0x39A00000
#define VIC0_MEM_BASE 0x38E00000
#define VIC1_MEM_BASE 0x38E01000
#define EDGEIC_MEM_BASE 0x38E02000
#define IIS0_MEM_BASE 0x3D400000
#define IIS1_MEM_BASE 0x3CD00000
#define IIS2_MEM_BASE 0x3CA00000
#define NOR_MEM_BASE 0x24000000
#define TIMER1_MEM_BASE 0x3C700000
#define USBOTG_MEM_BASE 0x38400000
#define USBPHYS_MEM_BASE 0x3C400000
#define GPIO_MEM_BASE 0x3CF00000
#define I2C0_MEM_BASE 0x3C600000
#define I2C1_MEM_BASE 0x3C900000
#define SPI0_MEM_BASE 0x3C300000
#define SPI1_MEM_BASE 0x3CE00000
#define SPI2_MEM_BASE 0x3D200000
#define WATCHDOG_MEM_BASE 0x3C800000
#define DISPLAY_MEM_BASE 0x38300000
#define CHIPID_MEM_BASE 0x3D100000
#define RAM_MEM_BASE 0x8000000
#define SHA1_MEM_BASE 0x38000000
#define AES_MEM_BASE 0x38C00000
#define VROM_MEM_BASE 0x20000000
#define NAND_MEM_BASE 0x38A00000
#define NAND_ECC_MEM_BASE 0x38F00000
#define DMAC0_MEM_BASE 0x38200000
#define DMAC1_MEM_BASE 0x39900000
#define UART0_MEM_BASE 0x3CC00000
#define UART1_MEM_BASE 0x3CC04000
#define UART2_MEM_BASE 0x3CC08000
#define UART3_MEM_BASE 0x3CC0C000
#define UART4_MEM_BASE 0x3CC10000
#define ADM_MEM_BASE 0x38800000
#define MBX_MEM_BASE 0x3B000000
#define ENGINE_8702_MEM_BASE 0x3F000000
#define KERNELCACHE_BASE 0x9000000
#define SDIO_MEM_BASE 0x38d00000
#define FRAMEBUFFER_MEM_BASE 0xfe00000
#define TVOUT1_MEM_BASE 0x39100000
#define TVOUT2_MEM_BASE 0x39200000
#define TVOUT3_MEM_BASE 0x39300000
#define MPVD_MEM_BASE 0x39600000
#define H264BPD_MEM_BASE 0x39800000

#define TVOUT_WORKAROUND_MEM_BASE 0x8a25960  // workaround for TV Out, to make sure that it can be correctly deallocated without reverse engineering the entire TVOut protocol

typedef struct {
    MachineClass parent;
} IPodNano3GMachineClass;

typedef struct S5L8702_usb_phys_s
{
	uint32_t usb_ophypwr;
	uint32_t usb_ophyclk;
	uint32_t usb_orstcon;
	uint32_t usb_ophytune;

} S5L8702_usb_phys_s;

typedef struct {
	MachineState parent;
	qemu_irq **irq;
	PL192State *vic0;
	PL192State *vic1;
	synopsys_usb_state *usb_otg;
	IPodNano3GClockState *clock0;
	IPodNano3GClockState *clock1;
	IPodNano3GTimerState *timer1;
	IPodNano3GSYSICState *sysic;
	S5L8702SPIState *spi2_state;
	S5L8702_usb_phys_s *usb_phys;
	S5L8702AESState *aes_state;
	S5L8702SHA1State *sha1_state;
	ITNandState *nand_state;
	ITNandECCState *nand_ecc_state;
	IPodNano3GI2CState *i2c0_state;
	IPodNano3GI2CState *i2c1_state;
	IPodNano3GADMState *adm_state;
	IPodNano3GChipIDState *chipid_state;
	IPodNano3GTVOutState *tvout1_state;
	IPodNano3GTVOutState *tvout2_state;
	IPodNano3GTVOutState *tvout3_state;
	IPodNano3GLCDState *lcd_state;
	IPodNano3GGPIOState *gpio_state;
	IPodNano3GSDIOState *sdio_state;
	S5L8702JPEGState *jpeg_state;
	Clock *sysclk;
	uint32_t kpc_pa;
	uint32_t kbootargs_pa;
	uint32_t uart_mmio_pa;
	ARMCPU *cpu;
	char bootrom_path[1024];
	char iboot_path[1024];
	char nand_path[1024];
	char nor_path[1024];
} IPodNano3GMachineState;

#endif