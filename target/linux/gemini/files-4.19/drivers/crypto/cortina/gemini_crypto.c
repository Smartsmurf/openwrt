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
#define RX_POLL_NUM 10
#define RX_BUF_SIZE 8192
#define TX_BUF_SIZE 2048

// static unsigned int polling_flag = 0;
// static int polling_process_id = -1;
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

static void crypto_put_queue(struct gemini_crypto_info *secdev, struct CRYPTO_PACKET_S *i)
{
	qhead *q = secdev->queue;
	unsigned long flags;

	spin_lock_irqsave(&secdev->queue_lock, flags);

	i->next = q->next;
	i->prev = q;
	q->next->prev = i;
	q->next = i;

	spin_unlock_irqrestore(&secdev->queue_lock, flags);
	return;
}

 

static struct CRYPTO_PACKET_S * ipsec_get_queue(struct gemini_crypto_info *secdev)
{
	qhead *q = secdev->queue;
	struct CRYPTO_PACKET_S *i;
	unsigned long flags;

	if(q->prev == q)
	{
		return NULL;
	}

	spin_lock_irqsave(&secdev->queue_lock, flags);
	i = q->prev;
	q->prev = i->prev;
	i->prev->next = i->next;

	spin_unlock_irqrestore(&secdev->queue_lock, flags);

	i->next = i->prev = NULL;
	return i;
}

static int gemini_crypto_rx_packet(struct gemini_crypto_info *secdev, unsigned int mode)
{
	CRYPTO_DESCRIPTOR_T	*rx_desc = secdev->tp->rx_cur_desc ;
	struct CRYPTO_PACKET_S	*op_info ;
	unsigned int		pkt_len;
	unsigned int            desc_count;
	unsigned int            process_id;
	unsigned int            auth_cmp_result;
	unsigned int            checksum = 0;
	unsigned int            i;
	unsigned long		flags;
	unsigned int		count = 0;

	while ((count < RX_POLL_NUM) || (secdev->polling_flag == 1)){
		if (((unsigned long)rx_desc < 0xf0000000) || ((unsigned long)rx_desc > 0xffffffff))
			dev_warn(secdev->dev,"%s: descriptor address is out of range - 0x%p\n",__func__,rx_desc);
		// consistent_sync((void *)rx_desc,sizeof(IPSEC_DESCRIPTOR_T),PCI_DMA_FROMDEVICE);
		// debug message
		if (rx_desc == NULL){
			dev_err(secdev->dev,"%s: fatal error. rx_desc == NULL\n",__func__);
			return -1;
		}
		if (rx_desc->frame_ctrl.bits.own == CPU){
	    	    if ( (rx_desc->frame_ctrl.bits.derr==1)||(rx_desc->frame_ctrl.bits.perr==1) )
	    	    {
	    	        dev_err(secdev->dev,"%s : descriptor processing error\n",__func__);
	    	    }
	    	    pkt_len = rx_desc->flag_status.bits_rx_status.frame_count;  /* total byte count in a frame*/
		    process_id = rx_desc->flag_status.bits_rx_status.process_id; /* get process ID from descriptor */
		    auth_cmp_result = rx_desc->flag_status.bits_rx_status.auth_result;
		    // wep_crc_ok = rx_desc->flag_status.bits_rx_status.wep_crc_ok;
		    // tkip_mic_ok = rx_desc->flag_status.bits_rx_status.tkip_mic_ok;
		    // ccmp_mic_ok = rx_desc->flag_status.bits_rx_status.ccmp_mic_ok;
	    	    desc_count = rx_desc->frame_ctrl.bits.desc_count; /* get descriptor count per frame */ 
	    	} else {
	    	    return count;
	    	}    


		if (secdev->last_rx_pid == process_id){
			dev_err(secdev->dev, "last_rx_pid = process_id = %d\n",process_id);
//			spin_unlock_irqrestore(&ipsec_rx_lock,flags_a);
			rx_desc->frame_ctrl.bits.own = DMA;
			secdev->tp->rx_finished_desc = rx_desc;
			/* get next RX descriptor pointer */
			rx_desc = (CRYPTO_DESCRIPTOR_T *)(rx_desc->next_desc.next_descriptor & 0xfffffff0);
			rx_desc += secdev->rx_desc_virtual_base;
			secdev->tp->rx_cur_desc = rx_desc;
			return -1;
		}

		if (process_id != 256)
			secdev->last_rx_pid = process_id;


		/* get request information from queue */
		if ((op_info = ipsec_get_queue(secdev)) != NULL){
			//    printk("%s : ipsec_get_queue op_info->process_id=%d pkt_len=%d\n",__func__,op_info->process_id,op_info->pkt_len);
		    	/* fill request result */
			// consistent_sync(op_info->out_packet,pkt_len,DMA_BIDIRECTIONAL);
			op_info->out_pkt_len = pkt_len;
			op_info->auth_cmp_result = auth_cmp_result;
			op_info->checksum = checksum;
			op_info->status = 0;
	    		
			// problme might be caused by prefetch and cache.
			mb();
			if (((unsigned long)(op_info->out_packet) <  0xc0000000) 
			||  ((unsigned long)(op_info->out_packet) >= 0xd0000000))
				dev_warn(secdev->dev, "%s::op_info->out_packet address is out of range? 0x%p\n",__func__,op_info->out_packet);
			// consistent_sync((void *)op_info->out_packet,pkt_len,PCI_DMA_FROMDEVICE);
			mb();

			//if(op_info->auth_result_mode)
			//	op_info->out_pkt_len-=0x10;
			//if ((op_info->out_pkt_len != op_info->pkt_len) || (op_info->process_id != process_id))
			if ((op_info->process_id != process_id)){
				op_info->status = 2;
				dev_info(secdev->dev,"op_info->out_pkt_len =%x , op_info->pkt_len= %x\n",
					op_info->out_pkt_len,op_info->pkt_len);
				dev_err(secdev->dev, "%s: Process ID or Packet Length Error %d %d !\n",__func__,
					op_info->process_id,process_id);
			}
			if ((secdev->polling_flag == 1 ) && ((int)process_id == secdev->polling_process_id)) {
				spin_lock_irqsave(&secdev->polling_lock,flags);
				secdev->polling_flag = 0;
				secdev->polling_process_id = -1;
				spin_unlock_irqrestore(&secdev->polling_lock,flags);
			}
		} else {
		    // op_info->status = 1;
		    dev_warn(secdev->dev,"%s:IPSec Queue Empty!\n", __func__);
		}    
    		count++;
		for (i=0; i<desc_count; i++)
		{
			/* return RX descriptor to DMA */
			rx_desc->frame_ctrl.bits.own = DMA;
			/* get next RX descriptor pointer */
			rx_desc  = (CRYPTO_DESCRIPTOR_T *)(rx_desc->next_desc.next_descriptor & 0xfffffff0);
			rx_desc += secdev->rx_desc_virtual_base;
		}
		secdev->tp->rx_cur_desc = rx_desc;
	//        wake_up_interruptible(&ipsec_wait_q);
	    
	        /* to call callback function */
		if (op_info > 0){
			if (op_info->out_packet == NULL){
				printk("%s::shouldn't happen!!!\n",__func__);
				return -1;
			}
			// if callback exists, use callback function, if not. just skip it.
			if (op_info->callback != NULL) {
				// op_info->flag_polling = secdev->polling_flag;
				op_info->callback(op_info);
			}
		}
	}           

	return count;
}

static int gemini_crypto_tx_packet(struct gemini_crypto_info *secdev, struct scatterlist *packet, 
	int len, unsigned int tqflag)
{
	CRYPTO_DESCRIPTOR_T	        *tx_desc = secdev->tp->tx_cur_desc;
//	CRYPTO_TXDMA_CTRL_T		    tx_ctrl,tx_ctrl_mask;
//	CRYPTO_RXDMA_CTRL_T		    rx_ctrl,rx_ctrl_mask;
//	CRYPTO_TXDMA_FIRST_DESC_T	txdma_busy;
	unsigned int                desc_cnt;
	unsigned int                i,tmp_len;
	unsigned int                sof;
	unsigned int                last_desc_byte_cnt;
	unsigned char               *pkt_ptr;
	unsigned int                reg_val;


	if (tx_desc->frame_ctrl.bits.own != CPU){
		dev_warn(secdev->dev,"\n%s : current Tx Descriptor is in use.\n",__func__);
        	gemini_crypto_read_reg(secdev->base, CRYPTO_ID);
	}

	sof = 0x02; /* the first descriptor */
	desc_cnt = (len/TX_BUF_SIZE) ;
	last_desc_byte_cnt = len % TX_BUF_SIZE;

	tmp_len=0;i=0;
	while(tmp_len < len){
		tx_desc->frame_ctrl.bits32 = 0;
		tx_desc->flag_status.bits32 = 0;
	   
		tx_desc->frame_ctrl.bits.buffer_size = packet[i].length; /* descriptor byte count */
		tx_desc->flag_status.bits_tx_flag.tqflag = tqflag;    /* set tqflag */

		//pkt_ptr = kmap(packet[i].page) + packet[i].offset;
		//consistent_sync(pkt_ptr,packet[i].length,PCI_DMA_TODEVICE);
		//pkt_ptr = (unsigned char *)virt_to_phys(pkt_ptr);  //__pa(packet);  
		//tx_desc->buf_adr = (unsigned int)pkt_ptr;
        	tx_desc->buf_adr = sg_phys(&packet[i]);

		if ( (packet[i].length == len) && i==0 ){
			sof = 0x03; /*only one descriptor*/
		} else if ( ((packet[i].length + tmp_len)== len) && (i != 0) ){
			sof = 0x01; /*the last descriptor*/
		}
		tx_desc->next_desc.bits.eofie = 1;
		tx_desc->next_desc.bits.dec = 0;
		tx_desc->next_desc.bits.sof_eof = sof;
		if (sof==0x02){
			sof = 0x00; /* the linking descriptor */
		}

		wmb();

		//middle
		tmp_len+=packet[i].length;
		i++;
		/* set owner bit */
		tx_desc->frame_ctrl.bits.own = DMA;
	        tx_desc  = (CRYPTO_DESCRIPTOR_T *)(tx_desc->next_desc.next_descriptor & 0xfffffff0);
		tx_desc += secdev->tx_desc_virtual_base;
		if (tx_desc->frame_ctrl.bits.own != CPU){
			dev_err(secdev->dev,"\n%s : Tx descriptor error (2)\n",__func__);
		}
	}
	secdev->tp->tx_cur_desc = tx_desc;
#if 0
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
#endif
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
	}
	if( i >= MAX_POLLING_LOOPS ){
		dev_err(secdev->dev, "FCS timeout while polling.\n");
		return 0;
	}
        if( secdev->polling_flag == 0){
		dev_warn(secdev->dev, "polling flag has been turned off (2)\n");
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


void crypto_hw_cipher(struct gemini_crypto_info *secdev, unsigned char *ctrl_pkt,int ctrl_len,
	struct scatterlist *data_pkt, int data_len, unsigned int tqflag,
	unsigned char *out_pkt,int *out_len )
{
	struct	scatterlist sg[1];
	unsigned long		flags;

//	disable_irq(IRQ_IPSEC);
	// https://stackoverflow.com/questions/41802779/scatterlist-in-linux-crypto-api
	sg_init_one(&sg[0], ctrl_pkt, ctrl_len);
//	sg[0].page_link = (long)virt_to_page(ctrl_pkt);
//	sg[0].offset = offset_in_page(ctrl_pkt);
//	sg[0].length = ctrl_len;
	spin_lock_irqsave(&secdev->tx_lock,flags);
	gemini_crypto_tx_packet(secdev, sg, ctrl_len,tqflag);
	gemini_crypto_tx_packet(secdev, data_pkt, data_len,0);
	gemini_crypto_start_dma(secdev);
	spin_unlock_irqrestore(&secdev->tx_lock,flags);

	if (secdev->polling_flag) {
		if( !gemini_interrupt_polling(secdev) )
			dev_err(secdev->dev, "%s: interrupt polling failed.\n",__func__);
	}
}

int crypto_hw_process(struct gemini_crypto_info *secdev, struct CRYPTO_PACKET_S  *op_info)
{
	volatile CRYPTO_DESCRIPTOR_T  *rx_desc;
	unsigned long flags, flags2, flags3;
	volatile CRYPTO_RXDMA_CTRL_T	rxdma_ctrl;
//	int available_space = desc_free_space();
    
	if (op_info == NULL) {
		printk("%s::hm. op_info is null o_O?\n",__func__);
		return -1;
	}

	// check if there is an available space for this crypto packet
//	if (available_space < 1) {
//		printk("%s::tx queue is full a\n",__func__);
//		return -1;
//	}

	spin_lock_irqsave(&secdev->pid_lock,flags3);
	op_info->process_id = (secdev->pid++) % 256;
	spin_unlock_irqrestore(&secdev->pid_lock,flags3);

	// first turn off the interrupt, such that there won't be conflict
	spin_lock_irqsave(&secdev->irq_lock,flags2);
	rxdma_ctrl.bits32 = crypto_read_reg(secdev->base, CRYPTO_RXDMA_CTRL);
	rxdma_ctrl.bits.rd_eof_en = 0;
	gemini_crypto_write_reg(secdev->base, CRYPTO_RXDMA_CTRL, rxdma_ctrl.bits32);
	spin_unlock_irqrestore(&secdev->irq_lock,flags2);

	// 2nd, turn on the polling flag.
	spin_lock_irqsave(&secdev->polling_lock,flags);
	polling_flag = 1;
//	if (polling_process_id != -1)
//		printk("current polling_process_id %d will be updated to %d, last_rx_pid = %d\n",
//			polling_process_id, op_info->process_id, last_rx_pid);
	secdev->polling_process_id = (int)(op_info->process_id);
	spin_unlock_irqrestore(&secdev->polling_lock,flags);

	/* get rx descriptor for this operation */
	rx_desc = rx_desc_index[rx_index%IPSEC_RX_DESC_NUM];
	/* set receive buffer address for this operation */
//	consistent_sync(op_info->out_packet,op_info->pkt_len,PCI_DMA_TODEVICE);
	rx_desc->buf_adr = __pa(op_info->out_packet); //virt_to_phys(op_info->out_packet);
//	ipsec_write_reg(IPSEC_RXDMA_BUF_ADDR,rx_desc->buf_adr,0xffffffff);
	unsigned int rxdma_desc = (crypto_read_reg(secdev->base, CRYPTO_RXDMA_CURR_DESC) & 0xfffffff0)+secdev->rx_desc_virtual_base;
	if ((unsigned int)rx_desc == (unsigned int)rxdma_desc) {
		gemini_crypto_write_reg(secdev->base, CRYPTO_RXDMA_BUF_ADDR, rx_desc->buf_adr);
//		consistent_sync(rx_desc,sizeof(IPSEC_DESCRIPTOR_T),PCI_DMA_TODEVICE);
	}

	if (op_info->out_buffer_len)
		rx_desc->frame_ctrl.bits.buffer_size = op_info->out_buffer_len;
	else
		rx_desc->frame_ctrl.bits.buffer_size = RX_BUF_SIZE;

	rx_index++;
	crypto_put_queue(secdev,op_info);

	if ((op_info->op_mode==ENC_AUTH) || (op_info->op_mode==AUTH_DEC))
	{
	//	ipsec_auth_and_cipher(op_info);
	}
	else
	{
	//	ipsec_auth_or_cipher(op_info);
	}
	return 0;
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
	crypto_info->polling_process_id = -1;
	crypto_info->pid = 0;
	crypto_info->last_rx_pid = 255;

	spin_lock_init(&crypto_info->lock);
	spin_lock_init(&crypto_info->tx_lock);
	spin_lock_init(&crypto_info->irq_lock);
	spin_lock_init(&crypto_info->queue_lock);
	spin_lock_init(&crypto_info->polling_lock);
	spin_lock_init(&crypto_info->pid_lock);

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
	// crypto_init_queue(&crypto_info->queue, 50);

	err = gemini_crypto_register(crypto_info);
	if (err) {
		dev_err(dev, "err in register alg");
		goto err_register_alg;
	}

	crypto_info->dev = &pdev->dev;
	platform_set_drvdata(pdev, crypto_info);

	reg = gemini_crypto_read_reg(crypto_info->base, CRYPTO_ID);
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
