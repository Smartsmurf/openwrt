/*
 * Crypto acceleration support for Cortina Gemini SoC
 *
 * Copyright (c) 2019, Andreas Fiedler
 *
 * Author: Andreas Fiedler <andreas.fiedler@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * Some ideas are from Rockwell driver.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/crypto.h>
#include <linux/reset.h>
#include "gemini_crypto.h"

#define gemini_crypto_read_reg(base, offset)		(readl(base + offset))
#define gemini_crypto_write_reg(base, offset, data)     (writel(data, base + offset))

#define MAX_POLLING_LOOPS 40000

// static unsigned int polling_flag = 0;
static int polling_process_id = -1;
static unsigned int flag_tasklet_scheduled = 0;

static void gemini_crypto_write_mask(void __iomem *addr,unsigned int data,unsigned int bit_mask)
{
	volatile unsigned int reg_val;
	reg_val = ( readl(addr) & (~bit_mask) ) | (data & bit_mask);
	writel(reg_val,addr);
}	

static irqreturn_t gemini_crypto_irq_handle(int irq, void *dev_id)
{
	struct gemini_crypto_info *dev  = platform_get_drvdata(dev_id);
	CRYPTO_DMA_STATUS_T	status;

	/* read DMA status */
	status.bits32 = readl(dev->base + CRYPTO_DMA_STATUS);

	/* clear DMA status */
	gemini_crypto_write_mask(dev->base + CRYPTO_DMA_STATUS,status.bits32,status.bits32);

	if ((status.bits32 & 0x63000000) > 0)
	{
		if (status.bits.ts_derr==1)
		{
			dev_err(dev->dev, "AHB bus error while Tx.\n");
		}
		if (status.bits.ts_perr==1)
		{
			dev_err(dev->dev, "Tx descriptor protocol error.\n");
		}    
		if (status.bits.rs_derr==1)
		{
			dev_err(dev->dev, "AHB bus error while Rx.\n");
		}
		if (status.bits.rs_perr==1)
		{
			dev_err(dev->dev, "Rx descriptor protocol error.\n");         
		} 
	}

	tasklet_schedule(&dev->done_tasklet);

	return IRQ_HANDLED;
/*
	struct gemini_crypto_info *dev  = platform_get_drvdata(dev_id);
	u32 interrupt_status;

	spin_lock(&dev->lock);
	interrupt_status = CRYPTO_READ(dev, gemini_crypto_INTSTS);
	CRYPTO_WRITE(dev, gemini_crypto_INTSTS, interrupt_status);

	if (interrupt_status & 0x0a) {
		dev_warn(dev->dev, "DMA Error\n");
		dev->err = -EFAULT;
	}
	tasklet_schedule(&dev->done_task);

	spin_unlock(&dev->lock);
	return IRQ_HANDLED;
*/
}

static int gemini_crypto_rx_packet(struct gemini_crypto_info *secdev, unsigned int mode)
{
	return 1;
}

static int gemini_crypto_tx_packet(struct gemini_crypto_info *secdev, struct scatterlist *packet, 
	int len, unsigned int tqflag)
{
	return 1;
}

static int gemini_interrupt_polling(struct gemini_crypto_info *secdev)
{
	CRYPTO_DMA_STATUS_T	status;
	unsigned int        i;
	unsigned long		flags;
	volatile CRYPTO_RXDMA_CTRL_T	rxdma_ctrl;

	if (secdev->polling_flag == 0)
	{
		dev_warn(secdev->dev, "polling flag has been turned off (1)\n");
		return 0;
	}

//	disable_irq(IRQ_IPSEC);
	for (i=0; i < MAX_POLLING_LOOPS; i++)
	{
		/* read DMA status */
		status.bits32 = gemini_crypto_read_reg(secdev->base, CRYPTO_DMA_STATUS);

		if (status.bits.rs_eofi==1){
			/* clear DMA status */
			gemini_crypto_write_mask(secdev->base + CRYPTO_DMA_STATUS,status.bits32,status.bits32);
			break;
		}
	        if( secdev->polling_flag == 0){
			dev_warn(secdev->dev, "polling flag has been turned off (2)\n");
			return 0;
		}
	}
	if( i >= MAX_POLLING_LOOPS ){
		dev_err(secdev->dev, "FCS timeout while polling.\n");
		return 0;
	}

	if (status.bits.rs_eofi==1) {
		gemini_crypto_rx_packet(secdev, 1);
	}
//	enable_irq(IRQ_IPSEC);

	if (secdev->polling_flag == 1){
//		printk("%s::gotta run polling more to get to the number we want !!\n",__func__);
//		printk("polling_process_id = %d, last_rx_pid = %d\n",polling_process_id,last_rx_pid);
//		start_dma();
		if( !gemini_interrupt_polling(secdev) ){
			dev_err(secdev->dev, "polling failed.\n");
			return 0;
		}
	} else {
		if (flag_tasklet_scheduled != 1) {
			spin_lock_irqsave(&secdev->irq_lock,flags);
			rxdma_ctrl.bits32 = gemini_crypto_read_reg(secdev->base, CRYPTO_RXDMA_CTRL);
			rxdma_ctrl.bits.rd_eof_en = 1;
			gemini_crypto_write_reg(secdev->base, CRYPTO_RXDMA_CTRL,rxdma_ctrl.bits32);
			spin_unlock_irqrestore(&secdev->irq_lock,flags);
		}
	}
	return 1;    
}

static void gemini_crypto_start_dma(struct gemini_crypto_info *secdev)
{
	CRYPTO_TXDMA_FIRST_DESC_T	txdma_busy;
	unsigned int			reg_val;

	/* if TX DMA process is stopped , restart it */
	txdma_busy.bits32 = gemini_crypto_read_reg(secdev->base, CRYPTO_TXDMA_FIRST_DESC);
	if (txdma_busy.bits.td_busy == 0)
	{
		/* restart Rx DMA process */
		reg_val = gemini_crypto_read_reg(secdev->base, CRYPTO_RXDMA_CTRL);
		reg_val |= (0x03<<30);
		gemini_crypto_write_reg(secdev->base, CRYPTO_RXDMA_CTRL, reg_val);

		/* restart Tx DMA process */
		reg_val = gemini_crypto_read_reg(secdev->base, CRYPTO_TXDMA_CTRL);
		reg_val |= (0x03<<30);
		gemini_crypto_write_reg(secdev->base, CRYPTO_TXDMA_CTRL, reg_val);
	}
}


void ipsec_hw_cipher(struct gemini_crypto_info *secdev, volatile unsigned char *ctrl_pkt,int ctrl_len,
	volatile struct scatterlist *data_pkt, int data_len, unsigned int tqflag,
	unsigned char *out_pkt,int *out_len )
{
	volatile struct	scatterlist sg[1];
	unsigned long		flags;

//	disable_irq(IRQ_IPSEC);
	// https://stackoverflow.com/questions/41802779/scatterlist-in-linux-crypto-api

	sg[0].page_link = (long)virt_to_page(ctrl_pkt);
	sg[0].offset = offset_in_page(ctrl_pkt);
	sg[0].length = ctrl_len;
//	ipsec_tx_packet(ctrl_pkt,ctrl_len,tqflag);
	spin_lock_irqsave(&secdev->tx_lock,flags);
	gemini_crypto_tx_packet(secdev, sg, ctrl_len,tqflag);
	gemini_crypto_tx_packet(secdev, data_pkt,data_len,0);
	gemini_crypto_start_dma( secdev );
	spin_unlock_irqrestore(&secdev->tx_lock,flags);

	if (secdev->polling_flag) {
		if( !gemini_interrupt_polling(secdev) )
			dev_err(secdev->dev, "%s: interrupt polling failed.\n",__func__);
	}
}


static struct gemini_crypto_tmp *gemini_cipher_algs[] = {
	&gemini_ecb_aes_alg,
/*	&gemini_cbc_aes_alg,
	&gemini_ecb_des_alg,
	&gemini_cbc_des_alg,
	&gemini_ecb_des3_ede_alg,
	&gemini_cbc_des3_ede_alg,
	&gemini_ahash_sha1,
	&gemini_ahash_sha256,
	&gemini_ahash_md5, */
};

static int gemini_crypto_register(struct gemini_crypto_info *crypto_info)
{
	unsigned int i, k;
	int err = 0;

	for (i = 0; i < ARRAY_SIZE(gemini_cipher_algs); i++) {
		gemini_cipher_algs[i]->dev = crypto_info;
		if (gemini_cipher_algs[i]->type == ALG_TYPE_CIPHER)
			err = crypto_register_alg(
					&gemini_cipher_algs[i]->alg.crypto);
		else
			err = crypto_register_ahash(
					&gemini_cipher_algs[i]->alg.hash);
		if (err)
			goto err_cipher_algs;
	}
	return 0;

err_cipher_algs:
	for (k = 0; k < i; k++) {
		if (gemini_cipher_algs[i]->type == ALG_TYPE_CIPHER)
			crypto_unregister_alg(&gemini_cipher_algs[k]->alg.crypto);
		else
			crypto_unregister_ahash(&gemini_cipher_algs[i]->alg.hash);
	}
	return err;
}

static void gemini_crypto_done_task_cb(unsigned long data)
{
	struct gemini_crypto_info *dev = (struct gemini_crypto_info *)data;

//	if (dev->err) {
//		dev->complete(dev->async_req, dev->err);
//		return;
//	}

//	dev->err = dev->update(dev);
//	if (dev->err)
//		dev->complete(dev->async_req, dev->err);
}

static void gemini_crypto_unregister(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(gemini_cipher_algs); i++) {
		if (gemini_cipher_algs[i]->type == ALG_TYPE_CIPHER)
			crypto_unregister_alg(&gemini_cipher_algs[i]->alg.crypto);
		else
			crypto_unregister_ahash(&gemini_cipher_algs[i]->alg.hash);
	}
}

static void gemini_crypto_action(void *data)
{
//	struct gemini_crypto_info *crypto_info = data;

//	reset_control_assert(crypto_info->rst);
}

static int gemini_crypto_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct gemini_crypto_info *crypto_info;
	int err = 0;
	u32 reg;


	crypto_info = devm_kzalloc(&pdev->dev,
				   sizeof(*crypto_info), GFP_KERNEL);
	if (!crypto_info) {
		err = -ENOMEM;
		goto err_crypto;
	}

/*
	err = devm_add_action_or_reset(dev, gemini_crypto_action, crypto_info);
	if (err)
		goto err_crypto;
*/

	crypto_info->polling_flag = 0;

	spin_lock_init(&crypto_info->lock);
	spin_lock_init(&crypto_info->tx_lock);
	spin_lock_init(&crypto_info->irq_lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	crypto_info->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(crypto_info->base)) {
		err = PTR_ERR(crypto_info->base);
		goto err_crypto;
	}

	crypto_info->irq = platform_get_irq(pdev, 0);
	if (crypto_info->irq < 0) {
		dev_warn(crypto_info->dev,
			 "crypto interrupt is not available.\n");
		err = crypto_info->irq;
		goto err_crypto;
	}

	err = devm_request_irq(&pdev->dev, crypto_info->irq,
			       gemini_crypto_irq_handle, IRQF_SHARED,
			       "cortina-crypto", pdev);
	if (err) {
		dev_err(crypto_info->dev, "irq request failed.\n");
		goto err_crypto;
	}

	crypto_info->pclk = devm_clk_get(dev, "PCLK");
	if (IS_ERR(crypto_info->pclk)) {
		crypto_info->pclk = NULL;
		dev_err(dev, "no PCLK\n");
	}
	// crypto_info->clk = NULL;

	tasklet_init(&crypto_info->done_tasklet,
		     gemini_crypto_done_task_cb, (unsigned long)crypto_info);
//	crypto_init_queue(&crypto_info->queue, 50);

	err = gemini_crypto_register(crypto_info);
	if (err) {
		dev_err(dev, "err in register alg");
		goto err_register_alg;
	}

	crypto_info->dev = &pdev->dev;
	platform_set_drvdata(pdev, crypto_info);

	reg = readl(crypto_info->base + CRYPTO_ID);
	dev_info(dev, "Crypto Accelerator (ID 0x%08x) successfully registered\n", reg);
	return 0;

err_register_alg:
	tasklet_kill(&crypto_info->done_tasklet);
err_crypto:
	return err;
}

static int __exit gemini_crypto_remove(struct platform_device *pdev)
{
	struct gemini_crypto_info *crypto_tmp = platform_get_drvdata(pdev);

	if (!crypto_tmp)
		return -ENODEV;

	gemini_crypto_unregister();
	tasklet_kill(&crypto_tmp->done_tasklet);
	dev_info(crypto_tmp->dev, "Unloaded.\n");
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id crypto_of_id_table[] = {
	{ .compatible = "cortina,gemini-crypto" },
	{}
};
MODULE_DEVICE_TABLE(of, crypto_of_id_table);

static struct platform_driver crypto_driver = {
	.probe		= gemini_crypto_probe,
	.remove		= gemini_crypto_remove,
	.driver		= {
		.name	= "cortina-crypto",
		.of_match_table	= of_match_ptr(crypto_of_id_table),
	},
};

module_platform_driver(crypto_driver);

/*
static int __init cortina_crypto_driver_init(void)
{
	return platform_driver_register(&crypto_driver);
}
module_init(cortina_crypto_driver_init);

static void __exit cortina_crypto_driver_exit(void)
{
	platform_driver_unregister(&crypto_driver);
}
module_exit(cortina_crypto_driver_exit);
*/

MODULE_AUTHOR("Andreas Fiedler <andreas.fiedler@gmx.net>");
MODULE_DESCRIPTION("Support for Cortina Gemini SoC cryptographic engine");
MODULE_LICENSE("GPL"); 
