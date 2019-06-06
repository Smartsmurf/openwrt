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

#if 0
static int gemini_crypto_enable_clk(struct gemini_crypto_info *dev)
{
	int err;

	err = clk_prepare_enable(dev->sclk);
	if (err) {
		dev_err(dev->dev, "[%s:%d], Couldn't enable clock sclk\n",
			__func__, __LINE__);
		goto err_return;
	}
	err = clk_prepare_enable(dev->aclk);
	if (err) {
		dev_err(dev->dev, "[%s:%d], Couldn't enable clock aclk\n",
			__func__, __LINE__);
		goto err_aclk;
	}
	err = clk_prepare_enable(dev->hclk);
	if (err) {
		dev_err(dev->dev, "[%s:%d], Couldn't enable clock hclk\n",
			__func__, __LINE__);
		goto err_hclk;
	}
	err = clk_prepare_enable(dev->dmaclk);
	if (err) {
		dev_err(dev->dev, "[%s:%d], Couldn't enable clock dmaclk\n",
			__func__, __LINE__);
		goto err_dmaclk;
	}
	return err;
err_dmaclk:
	clk_disable_unprepare(dev->hclk);
err_hclk:
	clk_disable_unprepare(dev->aclk);
err_aclk:
	clk_disable_unprepare(dev->sclk);
err_return:
	return err;
}

static void gemini_crypto_disable_clk(struct gemini_crypto_info *dev)
{
	clk_disable_unprepare(dev->dmaclk);
	clk_disable_unprepare(dev->hclk);
	clk_disable_unprepare(dev->aclk);
	clk_disable_unprepare(dev->sclk);
}

static int check_alignment(struct scatterlist *sg_src,
			   struct scatterlist *sg_dst,
			   int align_mask)
{
	int in, out, align;

	in = IS_ALIGNED((uint32_t)sg_src->offset, 4) &&
	     IS_ALIGNED((uint32_t)sg_src->length, align_mask);
	if (!sg_dst)
		return in;
	out = IS_ALIGNED((uint32_t)sg_dst->offset, 4) &&
	      IS_ALIGNED((uint32_t)sg_dst->length, align_mask);
	align = in && out;

	return (align && (sg_src->length == sg_dst->length));
}

static int gemini_load_data(struct gemini_crypto_info *dev,
			struct scatterlist *sg_src,
			struct scatterlist *sg_dst)
{
	unsigned int count;

	dev->aligned = dev->aligned ?
		check_alignment(sg_src, sg_dst, dev->align_size) :
		dev->aligned;
	if (dev->aligned) {
		count = min(dev->left_bytes, sg_src->length);
		dev->left_bytes -= count;

		if (!dma_map_sg(dev->dev, sg_src, 1, DMA_TO_DEVICE)) {
			dev_err(dev->dev, "[%s:%d] dma_map_sg(src)  error\n",
				__func__, __LINE__);
			return -EINVAL;
		}
		dev->addr_in = sg_dma_address(sg_src);

		if (sg_dst) {
			if (!dma_map_sg(dev->dev, sg_dst, 1, DMA_FROM_DEVICE)) {
				dev_err(dev->dev,
					"[%s:%d] dma_map_sg(dst)  error\n",
					__func__, __LINE__);
				dma_unmap_sg(dev->dev, sg_src, 1,
					     DMA_TO_DEVICE);
				return -EINVAL;
			}
			dev->addr_out = sg_dma_address(sg_dst);
		}
	} else {
		count = (dev->left_bytes > PAGE_SIZE) ?
			PAGE_SIZE : dev->left_bytes;

		if (!sg_pcopy_to_buffer(dev->first, dev->nents,
					dev->addr_vir, count,
					dev->total - dev->left_bytes)) {
			dev_err(dev->dev, "[%s:%d] pcopy err\n",
				__func__, __LINE__);
			return -EINVAL;
		}
		dev->left_bytes -= count;
		sg_init_one(&dev->sg_tmp, dev->addr_vir, count);
		if (!dma_map_sg(dev->dev, &dev->sg_tmp, 1, DMA_TO_DEVICE)) {
			dev_err(dev->dev, "[%s:%d] dma_map_sg(sg_tmp)  error\n",
				__func__, __LINE__);
			return -ENOMEM;
		}
		dev->addr_in = sg_dma_address(&dev->sg_tmp);

		if (sg_dst) {
			if (!dma_map_sg(dev->dev, &dev->sg_tmp, 1,
					DMA_FROM_DEVICE)) {
				dev_err(dev->dev,
					"[%s:%d] dma_map_sg(sg_tmp)  error\n",
					__func__, __LINE__);
				dma_unmap_sg(dev->dev, &dev->sg_tmp, 1,
					     DMA_TO_DEVICE);
				return -ENOMEM;
			}
			dev->addr_out = sg_dma_address(&dev->sg_tmp);
		}
	}
	dev->count = count;
	return 0;
}

static void gemini_unload_data(struct gemini_crypto_info *dev)
{
	struct scatterlist *sg_in, *sg_out;

	sg_in = dev->aligned ? dev->sg_src : &dev->sg_tmp;
	dma_unmap_sg(dev->dev, sg_in, 1, DMA_TO_DEVICE);

	if (dev->sg_dst) {
		sg_out = dev->aligned ? dev->sg_dst : &dev->sg_tmp;
		dma_unmap_sg(dev->dev, sg_out, 1, DMA_FROM_DEVICE);
	}
}

static irqreturn_t gemini_crypto_irq_handle(int irq, void *dev_id)
{
	struct gemini_crypto_info *dev  = platform_get_drvdata(dev_id);
	u32 interrupt_status;

	spin_lock(&dev->lock);
/*	interrupt_status = CRYPTO_READ(dev, gemini_crypto_INTSTS);
	CRYPTO_WRITE(dev, gemini_crypto_INTSTS, interrupt_status);

	if (interrupt_status & 0x0a) {
		dev_warn(dev->dev, "DMA Error\n");
		dev->err = -EFAULT;
	}
	tasklet_schedule(&dev->done_task);
*/
	spin_unlock(&dev->lock);
	return IRQ_HANDLED;
}

static int gemini_crypto_enqueue(struct gemini_crypto_info *dev,
			      struct crypto_async_request *async_req)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&dev->lock, flags);
	ret = crypto_enqueue_request(&dev->queue, async_req);
	if (dev->busy) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return ret;
	}
	dev->busy = true;
	spin_unlock_irqrestore(&dev->lock, flags);
	tasklet_schedule(&dev->queue_task);

	return ret;
}

static void gemini_crypto_queue_task_cb(unsigned long data)
{
	struct gemini_crypto_info *dev = (struct gemini_crypto_info *)data;
	struct crypto_async_request *async_req, *backlog;
	unsigned long flags;
	int err = 0;

	dev->err = 0;
	spin_lock_irqsave(&dev->lock, flags);
	backlog   = crypto_get_backlog(&dev->queue);
	async_req = crypto_dequeue_request(&dev->queue);

	if (!async_req) {
		dev->busy = false;
		spin_unlock_irqrestore(&dev->lock, flags);
		return;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	if (backlog) {
		backlog->complete(backlog, -EINPROGRESS);
		backlog = NULL;
	}

	dev->async_req = async_req;
	err = dev->start(dev);
	if (err)
		dev->complete(dev->async_req, err);
}

#endif

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

//	for (i = 0; i < ARRAY_SIZE(gemini_cipher_algs); i++) {
//		if (gemini_cipher_algs[i]->type == ALG_TYPE_CIPHER)
//			crypto_unregister_alg(&gemini_cipher_algs[i]->alg.crypto);
//		else
//			crypto_unregister_ahash(&gemini_cipher_algs[i]->alg.hash);
//	}
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

	spin_lock_init(&crypto_info->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	crypto_info->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(crypto_info->base)) {
		err = PTR_ERR(crypto_info->base);
		goto err_crypto;
	}
/*
	crypto_info->irq = platform_get_irq(pdev, 0);
	if (crypto_info->irq < 0) {
		dev_warn(crypto_info->dev,
			 "control Interrupt is not available.\n");
		err = crypto_info->irq;
		goto err_crypto;
	}

	err = devm_request_irq(&pdev->dev, crypto_info->irq,
			       gemini_crypto_irq_handle, IRQF_SHARED,
			       "rk-crypto", pdev);

	if (err) {
		dev_err(crypto_info->dev, "irq request failed.\n");
		goto err_crypto;
	}
*/

	/* Hardcoded Clk at the moment
	crypto_info->clk = devm_clk_get(dev, "crypto");
			if (IS_ERR(mtk->clk)) {
				mtk->clk = NULL;
				dev_err(dev, "Could not find clock\n");
			}
	*/
	crypto_info->clk = NULL;

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
	{ .compatible = "cortina-crypto" },
	{}
};
MODULE_DEVICE_TABLE(of, crypto_of_id_table);

static struct platform_driver crypto_driver = {
	.probe		= gemini_crypto_probe,
	.remove		= gemini_crypto_remove,
	.driver		= {
		.name	= "gemini-crypto",
		.of_match_table	= crypto_of_id_table,
	},
};

module_platform_driver(crypto_driver);

MODULE_AUTHOR("Andreas Fiedler <andreas.fiedler@gmx.net>");
MODULE_DESCRIPTION("Support for Cortina Gemini SoC cryptographic engine");
MODULE_LICENSE("GPL"); 
