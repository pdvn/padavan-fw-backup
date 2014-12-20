/**************************************************************************
 *
 *  BRIEF MODULE DESCRIPTION
 *     EHCI/OHCI init for Ralink RT3xxx
 *
 *  Copyright 2009 Ralink Inc. (yyhuang@ralinktech.com.tw)
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 **************************************************************************
 * March 2009 YYHuang Initial Release
 **************************************************************************
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#if defined(CONFIG_USB_EHCI_HCD_PLATFORM)
#include <linux/usb/ehci_pdriver.h>
#endif
#if defined(CONFIG_USB_OHCI_HCD_PLATFORM)
#include <linux/usb/ohci_pdriver.h>
#endif

#include <asm/rt2880/surfboardint.h>
#include <asm/rt2880/rt_mmap.h>

#define IRQ_RT3XXX_USB		SURFBOARDINT_UHST

static struct resource rt3xxx_ehci_resources[] = {
	[0] = {
		.start  = 0x101c0000,
		.end    = 0x101c0fff,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_RT3XXX_USB,
		.end    = IRQ_RT3XXX_USB,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct resource rt3xxx_ohci_resources[] = {
	[0] = {
		.start  = 0x101c1000,
		.end    = 0x101c1fff,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_RT3XXX_USB,
		.end    = IRQ_RT3XXX_USB,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 rt3xxx_ehci_dmamask = DMA_BIT_MASK(32);
static u64 rt3xxx_ohci_dmamask = DMA_BIT_MASK(32);

#if defined(CONFIG_USB_EHCI_HCD_PLATFORM) || defined(CONFIG_USB_OHCI_HCD_PLATFORM)
static atomic_t rt3xxx_power_instance = ATOMIC_INIT(0);

#define SYSCFG1_REG		(RALINK_SYSCTL_BASE + 0x14)
#define RALINK_UHST_MODE	(1UL<<10)

#define CLKCFG1_REG		(RALINK_SYSCTL_BASE + 0x30)
#define RSTCTRL_REG		(RALINK_SYSCTL_BASE + 0x34)

static void rt_usb_wake_up(void)
{
	u32 val;

	/* enable PHY0/1 clock */
	val = le32_to_cpu(*(volatile u32 *)(CLKCFG1_REG));
#if defined (CONFIG_RALINK_RT5350)
	val |= (RALINK_UPHY0_CLK_EN);
#else
	val |= (RALINK_UPHY0_CLK_EN | RALINK_UPHY1_CLK_EN);
#endif
	*(volatile u32 *)(CLKCFG1_REG) = cpu_to_le32(val);

	mdelay(10);

	/* set HOST mode */
	val = le32_to_cpu(*(volatile u32 *)(SYSCFG1_REG));
#if defined (CONFIG_USB_GADGET_RT)
	val &= ~(RALINK_UHST_MODE);
#else
	val |= (RALINK_UHST_MODE);
#endif
	*(volatile u32 *)(SYSCFG1_REG) = cpu_to_le32(val);

	mdelay(1);

	/* release reset */
	val = le32_to_cpu(*(volatile u32 *)(RSTCTRL_REG));
	val &= ~(RALINK_UHST_RST | RALINK_UDEV_RST);
	*(volatile u32 *)(RSTCTRL_REG) = cpu_to_le32(val);

	mdelay(100);
}

static void rt_usb_sleep(void)
{
	u32 val;

	/* raise reset */
	val = le32_to_cpu(*(volatile u32 *)(RSTCTRL_REG));
	val |= (RALINK_UHST_RST | RALINK_UDEV_RST);
	*(volatile u32 *)(RSTCTRL_REG) = cpu_to_le32(val);

	mdelay(10);

	/* disable PHY0/1 clock */
	val = le32_to_cpu(*(volatile u32 *)(CLKCFG1_REG));
#if defined (CONFIG_RALINK_RT5350)
	val &= ~(RALINK_UPHY0_CLK_EN);
#else
	val &= ~(RALINK_UPHY0_CLK_EN | RALINK_UPHY1_CLK_EN);
#endif
	*(volatile u32 *)(CLKCFG1_REG) = cpu_to_le32(val);

	udelay(10);
}

static int rt3xxx_power_on(struct platform_device *pdev)
{
	if (atomic_inc_return(&rt3xxx_power_instance) == 1)
		rt_usb_wake_up();
	return 0;
}

static void rt3xxx_power_off(struct platform_device *pdev)
{
	if (atomic_dec_return(&rt3xxx_power_instance) == 0)
		rt_usb_sleep();
}

static struct usb_ehci_pdata rt3xxx_ehci_pdata = {
	.caps_offset		= 0,
	.has_synopsys_hc_bug	= 1,
	.port_power_off		= 1,
	.power_on		= rt3xxx_power_on,
	.power_off		= rt3xxx_power_off,
};

static struct usb_ohci_pdata rt3xxx_ohci_pdata = {
	.power_on		= rt3xxx_power_on,
	.power_off		= rt3xxx_power_off,
};
#else
static struct platform_device rt3xxx_ehci_device = {
	.name		= "rt3xxx-ehci",
	.id		= -1,
	.dev		= {
		.dma_mask = &rt3xxx_ehci_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= 2,
	.resource	= rt3xxx_ehci_resources,
};

static struct platform_device rt3xxx_ohci_device = {
	.name		= "rt3xxx-ohci",
	.id		= -1,
	.dev		= {
		.dma_mask = &rt3xxx_ohci_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= 2,
	.resource	= rt3xxx_ohci_resources,
};

static struct platform_device *rt3xxx_devices[] __initdata = {
	&rt3xxx_ehci_device,
	&rt3xxx_ohci_device,
};
#endif

int __init init_rt3xxx_ehci_ohci(void)
{
#if defined(CONFIG_USB_EHCI_HCD_PLATFORM)
	struct platform_device *ehci_pdev;
#endif
#if defined(CONFIG_USB_OHCI_HCD_PLATFORM)
	struct platform_device *ohci_pdev;
#endif

	printk("MTK/Ralink EHCI/OHCI init.\n");

#if defined(CONFIG_USB_EHCI_HCD_PLATFORM)
	ehci_pdev = platform_device_register_resndata(NULL, "rt3xxx-ehci", -1,
			rt3xxx_ehci_resources, ARRAY_SIZE(rt3xxx_ehci_resources),
			&rt3xxx_ehci_pdata, sizeof(rt3xxx_ehci_pdata));
	if (IS_ERR(ehci_pdev)) {
		pr_err("MTK/Ralink %s: unable to register USB, err=%d\n", "EHCI", (int)PTR_ERR(ehci_pdev));
		return -1;
	}
	ehci_pdev->dev.dma_mask = &rt3xxx_ehci_dmamask;
	ehci_pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
#endif
#if defined(CONFIG_USB_OHCI_HCD_PLATFORM)
	ohci_pdev = platform_device_register_resndata(NULL, "rt3xxx-ohci", -1,
			rt3xxx_ohci_resources, ARRAY_SIZE(rt3xxx_ohci_resources),
			&rt3xxx_ohci_pdata, sizeof(rt3xxx_ohci_pdata));
	if (IS_ERR(ohci_pdev)) {
		pr_err("MTK/Ralink %s: unable to register USB, err=%d\n", "OHCI", (int)PTR_ERR(ohci_pdev));
#if defined(CONFIG_USB_EHCI_HCD_PLATFORM)
		platform_device_unregister(ehci_pdev);
#endif
		return -1;
	}
	ohci_pdev->dev.dma_mask = &rt3xxx_ohci_dmamask;
	ohci_pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
#endif

#if !defined(CONFIG_USB_EHCI_HCD_PLATFORM) && !defined(CONFIG_USB_OHCI_HCD_PLATFORM)
	platform_add_devices(rt3xxx_devices, ARRAY_SIZE(rt3xxx_devices));
#endif

	return 0;
}

device_initcall(init_rt3xxx_ehci_ohci);
