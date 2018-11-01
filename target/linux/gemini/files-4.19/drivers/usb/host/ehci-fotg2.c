/*
 *  EHCI Host Controller driver
 *
 *  Copyright (C) 2012 Tobias Waldvogel
 *  based on GPLd code from Sony Computer Entertainment Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/ehci_pdriver.h>
#include <linux/dma-mapping.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/bitops.h>
/* For Cortina Gemini */
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <asm/byteorder.h>
#include <asm/irq.h>
#include <asm/unaligned.h>

//#include <mach/hardware.h>
//#include <mach/global_reg.h>

#include "ehci.h"
/* includes were taken from u-boot project (include/usb) */
#include "ehci-fotg2.h"
#include "ehci-fusbh2.h"


struct fotg2_ehci {
	struct clk *clk;
};

union ehci_fotg2_regs {
	struct fusbh200_regs usb;
	struct fotg210_regs  otg;
};

#define to_fotg2_ehci(hcd)	(struct fotg2_ehci *)(hcd_to_ehci(hcd)->priv)

#define DRV_NAME			"ehci-fotg2"

#define HCD_MISC			0x40

#define OTGC_SCR			0x80
#define OTGC_INT_STS			0x84
#define OTGC_INT_EN			0x88

#define GLOBAL_ISR			0xC0	/* Global Interrupt Status Register (W1C) */
#define ISR_HOST            (1 << 2)  /* USB Host interrupt */
#define ISR_OTG             (1 << 1)  /* USB OTG interrupt */
#define ISR_DEV             (1 << 0)  /* USB Device interrupt */
#define ISR_MASK            0x07

#define GLOBAL_ICR			0xC4

#define GLOBAL_INT_POLARITY		(1 << 3)
#define GLOBAL_INT_MASK_HC		(1 << 2)
#define GLOBAL_INT_MASK_OTG		(1 << 1)
#define GLOBAL_INT_MASK_DEV		(1 << 0)

MODULE_ALIAS("platform:" DRV_NAME);

/*
 * Gemini-specific initialization function, only executed on the
 * Gemini SoC using the global misc control register.
 */
#define GEMINI_GLOBAL_SYSCON_MISC_CTRL	0x30
#define GEMINI_MISC_USB0_WAKEUP		BIT(14)
#define GEMINI_MISC_USB1_WAKEUP		BIT(15)
#define GEMINI_MISC_USB0_VBUS_ON	BIT(22)
#define GEMINI_MISC_USB1_VBUS_ON	BIT(23)
#define GEMINI_MISC_USB0_MINI_B		BIT(29)
#define GEMINI_MISC_USB1_MINI_B		BIT(30)

static int fotg210_gemini_init(struct device *dev, struct usb_hcd *hcd)
{
	struct device_node *np = dev->of_node;
	struct regmap *map;
	bool mini_b;
	bool wakeup;
	u32 mask, val;
	int ret;
	map = syscon_regmap_lookup_by_phandle(np, "syscon");
	if (IS_ERR(map)) {
		dev_err(dev, "no syscon\n");
		return PTR_ERR(map);
	}
	mini_b = of_property_read_bool(np, "cortina,gemini-mini-b");
	wakeup = of_property_read_bool(np, "wakeup-source");

	/*
	 * Figure out if this is USB0 or USB1 by simply checking the
	 * physical base address.
	 */
	mask = 0;
	if (hcd->rsrc_start == 0x69000000) {
		val = GEMINI_MISC_USB1_VBUS_ON;
		if (mini_b)
			val |= GEMINI_MISC_USB1_MINI_B;
		else
			mask |= GEMINI_MISC_USB1_MINI_B;
		if (wakeup)
			val |= GEMINI_MISC_USB1_WAKEUP;
		else
			mask |= GEMINI_MISC_USB1_WAKEUP;
	} else {
		val = GEMINI_MISC_USB0_VBUS_ON;
		if (mini_b)
			val |= GEMINI_MISC_USB0_MINI_B;
		else
			mask |= GEMINI_MISC_USB0_MINI_B;
		if (wakeup)
			val |= GEMINI_MISC_USB0_WAKEUP;
		else
			mask |= GEMINI_MISC_USB0_WAKEUP;
	}

	dev_info(dev, "initializing Gemini PHY @ 0x%x (val=0x%x, mask=0x%x)\n", hcd->rsrc_start, val, mask);

	ret = regmap_update_bits(map, GEMINI_GLOBAL_SYSCON_MISC_CTRL, mask, val);
	if (ret) {
		dev_err(dev, "failed to initialize Gemini PHY\n");
		return ret;
	}

	dev_info(dev, "initialized Gemini PHY\n");
	return 0;
}

static inline int ehci_is_fotg2xx(struct usb_hcd *hcd)
{
	union ehci_fotg2_regs *regs = hcd->regs;
	return !ioread32(&regs->usb.easstr);
}

static void fotg2_otg_init(struct usb_hcd *hcd)
{
	u32 val;
	union ehci_fotg2_regs *regs = hcd->regs;

	if (ehci_is_fotg2xx(hcd)) {
		ehci_info(hcd_to_ehci(hcd),
			"resetting fotg2xx\n");

		/* ... Power off A-device */
		val = ioread32(&regs->otg.otgcsr);
		val |= OTGCSR_A_BUSDROP;
		iowrite32(val, &regs->otg.otgcsr);
		/* ... Drop vbus and bus traffic */
		val = ioread32(&regs->otg.otgcsr);
		val &= ~OTGCSR_A_BUSREQ;
		iowrite32(val, &regs->otg.otgcsr);
		msleep(10);
		/* ... Power on A-device */
		val = ioread32(&regs->otg.otgcsr);
		val &= ~OTGCSR_A_BUSDROP;
		iowrite32(val, &regs->otg.otgcsr);
		/* ... Drive vbus and bus traffic */
		val = ioread32(&regs->otg.otgcsr);
		val |= OTGCSR_A_BUSREQ;
		iowrite32(val, &regs->otg.otgcsr);
		msleep(10);
	} else {
		ehci_warn(hcd_to_ehci(hcd),
			"device not in host mode\n");

		/* chip enable */
		//iowrite32(DEVCTRL_EN, hcd->regs + DEV_CTRL_REG);

		/* device address reset */
		//iowrite32(0, hcd->regs + DEV_ADDR_REG);

		/* set idle counter to 7ms */
		//iowrite32(7, hcd->regs + DEV_IDLE_CTR_REG);

		/* chip reset */
		//val = ioread32(hcd->regs + DEV_CTRL_REG);
		//val |= DEVCTRL_RESET;
		//iowrite32(val, hcd->regs + DEV_CTRL_REG);
		//msleep(10); 
		//if (ioread32(hcd->regs + DEV_CTRL_REG) & DEVCTRL_RESET) {
		//	ehci_warn(hcd_to_ehci(hcd),
		//		"fotg210: chip reset failed\n");
		//}	
		/* suspend delay = 3 ms */
		//iowrite32(3, hcd->regs + DEV_IDLE_CTR_REG);
	}

//#if 0

//	iowrite32(GLOBAL_INT_POLARITY | GLOBAL_INT_MASK_HC |
//	       GLOBAL_INT_MASK_OTG | GLOBAL_INT_MASK_DEV,
//		hcd->regs + GLOBAL_ICR);

//	val = ioread32(hcd->regs + OTGC_SCR);
//	val &= ~(OTGC_SCR_A_SRP_RESP_TYPE | OTGC_SCR_A_SRP_DET_EN |
//		 OTGC_SCR_A_BUS_DROP      | OTGC_SCR_A_SET_B_HNP_EN);
//	val |= OTGC_SCR_A_BUS_REQ;
//	iowrite32(val, hcd->regs + OTGC_SCR);

//	iowrite32(OTGC_INT_A_TYPE, hcd->regs + OTGC_INT_EN);
//#else
	
//	iowrite32(GLOBAL_INT_POLARITY | GLOBAL_INT_MASK_HC |
//	       GLOBAL_INT_MASK_OTG | GLOBAL_INT_MASK_DEV,
//		hcd->regs + GLOBAL_ICR);

//	val = ioread32(hcd->regs + OTGC_SCR);
//	val &= ~(OTGC_SCR_A_SRP_RESP_TYPE | OTGC_SCR_A_SRP_DET_EN |
//		 OTGC_SCR_A_BUS_DROP      | OTGC_SCR_A_SET_B_HNP_EN);
//	val |= OTGC_SCR_A_BUS_REQ;
//	iowrite32(val, hcd->regs + OTGC_SCR);

//	iowrite32(OTGC_INT_A_TYPE, hcd->regs + OTGC_INT_EN);
	

//#endif
	/* Disable HOST, OTG & DEV interrupts, triggered at level-high */
	iowrite32(IMR_IRQLH | ISR_HOST | IMR_OTG | IMR_DEV, &regs->otg.imr);
	/* Clear all interrupt status */
	iowrite32(ISR_HOST | ISR_OTG | ISR_DEV, &regs->otg.isr);
	/* enable Mini-A and A-device IRQ */
	iowrite32(OTGIER_ASRP | OTGIER_AVBUSERR | OTGIER_OVD |
		  OTGIER_RLCHG | OTGIER_IDCHG | OTGIER_APRM, &regs->otg.otgier);

	/* setup MISC register, fixes timing problems */
	val = ioread32(&regs->otg.miscr);
	val |= 0xD;
	iowrite32(val, &regs->otg.miscr);

//	iowrite32(~0, hcd->regs + GLOBAL_ISR);
//	iowrite32(~0, hcd->regs + OTGC_INT_STS);

}

static int fotg2_ehci_reset(struct usb_hcd *hcd)
{
	int retval;

	ehci_info(hcd_to_ehci(hcd),
			"fotg2_ehci_reset called\n");

	retval = ehci_setup(hcd);
	if (retval)
		return retval;

	iowrite32(GLOBAL_INT_POLARITY, hcd->regs + GLOBAL_ICR);
	return 0;
}

static const struct ehci_driver_overrides fotg2_overrides __initdata = {
	.reset =	fotg2_ehci_reset,
};

static struct hc_driver __read_mostly fotg2_ehci_hc_driver;

static irqreturn_t fotg2_ehci_irq(int irq, void *data)
{
	struct usb_hcd *hcd = data;
	u32 icr, sts;
	irqreturn_t retval;
	
	icr = ioread32(hcd->regs + GLOBAL_ICR);
	iowrite32(GLOBAL_INT_POLARITY | GLOBAL_INT_MASK_HC |
	       GLOBAL_INT_MASK_OTG | GLOBAL_INT_MASK_DEV,
		hcd->regs + GLOBAL_ICR);
	
	retval = IRQ_NONE;

	sts = ~icr;
	sts &= GLOBAL_INT_MASK_HC | GLOBAL_INT_MASK_OTG | GLOBAL_INT_MASK_DEV;
	sts &= ioread32(hcd->regs + GLOBAL_ISR);
	iowrite32(sts, hcd->regs + GLOBAL_ISR);
	
	if (unlikely(sts & GLOBAL_INT_MASK_DEV)) {
		ehci_warn(hcd_to_ehci(hcd),
			"Received unexpected irq for device role\n");
		retval = IRQ_HANDLED;
	}
	
	if (unlikely(sts & GLOBAL_INT_MASK_OTG)) {
		u32	otg_sts;

		otg_sts = ioread32(hcd->regs + OTGC_INT_STS);
		iowrite32(otg_sts, hcd->regs + OTGC_INT_STS);

		ehci_warn(hcd_to_ehci(hcd),
			"Received unexpected irq for OTG management\n");
		retval = IRQ_HANDLED;
	}
	
	if (sts & GLOBAL_INT_MASK_HC) {
		retval = IRQ_NONE;
	}

	iowrite32(icr, hcd->regs + GLOBAL_ICR);
	return retval;
}

static int fotg2_ehci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct usb_hcd *hcd;
	struct resource *res;
	struct fotg2_ehci *fehci;
	struct clk *usbh_clk;

	int irq , err;

	if (usb_disabled())
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "Found HC with no IRQ. Check %s setup!\n",
				dev_name(dev));
		return -ENODEV;
	}
	irq = res->start;

	/* Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we have dma capability bindings this can go away.
	 */
	err = dma_coerce_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (err)
		return -ENXIO;

	pdev->dev.power.power_state = PMSG_ON;

	usbh_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(usbh_clk)) {
		dev_err(&pdev->dev, "Error getting interface clock\n");
		err = PTR_ERR(usbh_clk);
		goto fail;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource provided");
		return -ENXIO;
	}

	hcd = usb_create_hcd(&fotg2_ehci_hc_driver, &pdev->dev,
			     dev_name(&pdev->dev));
	if (!hcd)
		return -ENOMEM;

	hcd->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hcd->regs)) {
		err = PTR_ERR(hcd->regs);
		goto err_put_hcd;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	fehci = to_fotg2_ehci(hcd);
	fehci->clk = usbh_clk;

	hcd->has_tt = 1;
	/* registers start at offset 0x0 */
	hcd_to_ehci(hcd)->caps = hcd->regs;

	fotg2_otg_init(hcd);

	if (of_device_is_compatible(dev->of_node, "cortina,gemini-usb")) {
		err = fotg210_gemini_init(dev, hcd);
		if (err)
			goto err_put_hcd;
	}

	err = request_irq(irq, &fotg2_ehci_irq, IRQF_SHARED, "fotg2", hcd);
	if (err)
		goto err_put_hcd;

	clk_prepare_enable(fehci->clk);
	err = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (err)
		goto err_put_hcd;

	device_wakeup_enable(hcd->self.controller);
	
	platform_set_drvdata(pdev, hcd);
	return 0;

err_put_hcd:
	usb_put_hcd(hcd);
fail:
	dev_err(&pdev->dev, "init fail, %d\n", err);
	return err;
}

static int fotg2_ehci_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct fotg2_ehci *fehci = to_fotg2_ehci(hcd);

	iowrite32(GLOBAL_INT_POLARITY | GLOBAL_INT_MASK_HC |
	       GLOBAL_INT_MASK_OTG | GLOBAL_INT_MASK_DEV,
		hcd->regs + GLOBAL_ICR);

	free_irq(hcd->irq, hcd);
	usb_remove_hcd(hcd);
	if (fehci->clk)
		clk_disable_unprepare(fehci->clk);
	usb_put_hcd(hcd);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id fotg210_hcd_of_match[] = {
	{
		.compatible = "faraday,fotg210",
	},
	{},
};

static struct platform_driver ehci_fotg2_driver = {
	.driver = {
		.name   = "fotg210-ehci",
		.of_match_table = of_match_ptr(fotg210_hcd_of_match),
		.owner	= THIS_MODULE,
	},
	.probe		= fotg2_ehci_probe,
	.remove		= fotg2_ehci_remove,
};

static int __init ehci_platform_init(void)
{
	ehci_init_driver(&fotg2_ehci_hc_driver, &fotg2_overrides);
	return platform_driver_register(&ehci_fotg2_driver);
}
module_init(ehci_platform_init);

static void __exit ehci_platform_cleanup(void)
{
	platform_driver_unregister(&ehci_fotg2_driver);
}
module_exit(ehci_platform_cleanup);

