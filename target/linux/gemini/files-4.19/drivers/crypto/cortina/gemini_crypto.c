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
			rx_desc += secdev->tp->rx_desc_virtual_base;
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
			rx_desc += secdev->tp->rx_desc_virtual_base;
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
//	unsigned char               *pkt_ptr;
//	unsigned int                reg_val;


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
		tx_desc += secdev->tp->tx_desc_virtual_base;
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

void gemini_key_swap(unsigned char *out_key, const unsigned char *in_key, unsigned int in_len)
{
	int i;
	for( i = 0; i < in_len; i += 4 ){
//		*out_key++ = cpu_to_be32(*in_key++);	/* exception on misaligment of in_key/out_key
		out_key[3] = *in_key++;
		out_key[2] = *in_key++;
		out_key[1] = *in_key++;
		out_key[0] = *in_key++;
		out_key += 4;
	}
}

static int crypto_buf_init(struct gemini_crypto_info *secdev)
{
	dma_addr_t   tx_first_desc_dma=0;
	dma_addr_t   rx_first_desc_dma=0;
	CRYPTO_T *tp; 
	int i;

	secdev->tp = devm_kmalloc(secdev->dev, sizeof(CRYPTO_T), GFP_ATOMIC);
	if (secdev->tp == NULL)
	{
		dev_err(secdev->dev, "%s: memory allocation failed.\n", __func__);
		return -ENOMEM;
	}

	tp = secdev->tp;

	/* allocates TX/RX descriptors */
	tp->tx_desc = dma_zalloc_coherent(secdev->dev,CRYPTO_TX_DESC_NUM*sizeof(CRYPTO_DESCRIPTOR_T),(dma_addr_t *)&tp->tx_desc_dma, GFP_KERNEL);
	tp->tx_desc_virtual_base = (unsigned int)tp->tx_desc - (unsigned int)tp->tx_desc_dma;
	tp->rx_desc = dma_zalloc_coherent(secdev->dev,CRYPTO_RX_DESC_NUM*sizeof(CRYPTO_DESCRIPTOR_T),(dma_addr_t *)&tp->rx_desc_dma, GFP_KERNEL);
	tp->rx_desc_virtual_base = (unsigned int)tp->rx_desc - (unsigned int)tp->rx_desc_dma;

	if (tp->tx_desc==0x00 || tp->rx_desc==0x00) 
	{
		if (tp->tx_desc)
			dma_free_coherent(secdev->dev, CRYPTO_TX_DESC_NUM*sizeof(CRYPTO_DESCRIPTOR_T), tp->tx_desc, (unsigned int)tp->tx_desc_dma);
		if (tp->rx_desc)
			dma_free_coherent(secdev->dev, CRYPTO_RX_DESC_NUM*sizeof(CRYPTO_DESCRIPTOR_T), tp->rx_desc, (unsigned int)tp->rx_desc_dma);
		dev_err(secdev->dev, "%s: no coherent DMA memory available.\n", __func__);
		return -ENOMEM;
	}
	
	/* TX descriptors initial */
	tp->tx_cur_desc = tp->tx_desc;          /* virtual address */
	tp->tx_finished_desc = tp->tx_desc;     /* virtual address */
	tx_first_desc_dma = tp->tx_desc_dma;    /* physical address */
	for (i = 1; i < CRYPTO_TX_DESC_NUM; i++)
	{
		tp->tx_desc->frame_ctrl.bits.own = CPU; /* set owner to CPU */
		tp->tx_desc->frame_ctrl.bits.buffer_size = TX_BUF_SIZE;  /* set tx buffer size for descriptor */
		tp->tx_desc_dma = tp->tx_desc_dma + sizeof(CRYPTO_DESCRIPTOR_T); /* next tx descriptor DMA address */
		tp->tx_desc->next_desc.next_descriptor = tp->tx_desc_dma | 0x0000000b;
		tp->tx_desc = &tp->tx_desc[1] ; /* next tx descriptor virtual address */
	}
	/* the last descriptor will point back to first descriptor */
	tp->tx_desc->frame_ctrl.bits.own = CPU;
	tp->tx_desc->frame_ctrl.bits.buffer_size = TX_BUF_SIZE;
	tp->tx_desc->next_desc.next_descriptor = tx_first_desc_dma | 0x0000000b;
	tp->tx_desc = tp->tx_cur_desc;
	tp->tx_desc_dma = tx_first_desc_dma;
	
	/* RX descriptors initial */
	tp->rx_cur_desc = tp->rx_desc;
	rx_first_desc_dma = tp->rx_desc_dma;
	tp->rx_desc_index[0] = tp->rx_desc;
	for (i = 1; i < CRYPTO_RX_DESC_NUM; i++)
	{
		tp->rx_desc->frame_ctrl.bits.own = DMA;  /* set owner bit to DMA */
		tp->rx_desc->frame_ctrl.bits.buffer_size = RX_BUF_SIZE; /* set rx buffer size for descriptor */
		tp->rx_desc_dma = tp->rx_desc_dma + sizeof(CRYPTO_DESCRIPTOR_T); /* next rx descriptor DMA address */
		tp->rx_desc->next_desc.next_descriptor = tp->rx_desc_dma | 0x0000000b;
		tp->rx_desc = &tp->rx_desc[1]; /* next rx descriptor virtual address */
		tp->rx_desc_index[i] = tp->rx_desc;
	}
	/* the last descriptor will point back to first descriptor */
	tp->rx_desc->frame_ctrl.bits.own = DMA;
	tp->rx_desc->frame_ctrl.bits.buffer_size = RX_BUF_SIZE;
	tp->rx_desc->next_desc.next_descriptor = rx_first_desc_dma | 0x0000000b;
	tp->rx_desc = tp->rx_cur_desc;
	tp->rx_desc_dma = rx_first_desc_dma;

	dev_info(secdev->dev, "tx_desc = %08x\n",(unsigned int)tp->tx_desc);
	dev_info(secdev->dev, "rx_desc = %08x\n",(unsigned int)tp->rx_desc);
	dev_info(secdev->dev, "tx_desc_dma = %08x\n",tp->tx_desc_dma);
	dev_info(secdev->dev, "rx_desc_dma = %08x\n",tp->rx_desc_dma);
	return 0;	
}

static void crypto_hw_start(struct gemini_crypto_info *secdev)
{
	volatile CRYPTO_TXDMA_CURR_DESC_T	tx_desc;
	volatile CRYPTO_RXDMA_CURR_DESC_T	rx_desc;
	volatile CRYPTO_TXDMA_CTRL_T		txdma_ctrl,txdma_ctrl_mask;
	volatile CRYPTO_RXDMA_CTRL_T		rxdma_ctrl,rxdma_ctrl_mask;
	volatile CRYPTO_DMA_STATUS_T		dma_status,dma_status_mask;

    	// crypto_sw_reset();

	gemini_crypto_write_mask(secdev->base + CRYPTO_RXDMA_MAGIC, 0x00000044, 0xffffffff);

	/* program TxDMA Current Descriptor Address register for first descriptor */
	tx_desc.bits32 = (unsigned int)(secdev->tp->tx_desc_dma);
	tx_desc.bits.eofie = 0;			// turn off by wen
	tx_desc.bits.dec = 0;
	tx_desc.bits.sof_eof = 0x03;
	gemini_crypto_write_mask(secdev->base + CRYPTO_TXDMA_CURR_DESC,tx_desc.bits32,0xffffffff);
	
	/* program RxDMA Current Descriptor Address register for first descriptor */
	rx_desc.bits32 = (unsigned int)(secdev->tp->rx_desc_dma);
	rx_desc.bits.eofie = 1;			// turn off by Wen
	rx_desc.bits.dec = 0;
	rx_desc.bits.sof_eof = 0x03;
	gemini_crypto_write_mask(secdev->base + CRYPTO_RXDMA_CURR_DESC,rx_desc.bits32,0xffffffff);
		
	/* enable IPSEC interrupt & disable loopback */
//	dma_status.bits32 = (unsigned int)(tp->tx_desc_dma) - 6;
	dma_status.bits32 = 0;
	dma_status.bits.loop_back = 0;
	dma_status_mask.bits32 = 0xffffffff;
	dma_status_mask.bits.loop_back = 1;
	gemini_crypto_write_mask(secdev->base + CRYPTO_DMA_STATUS,dma_status.bits32,dma_status_mask.bits32);
	
	txdma_ctrl.bits32 = 0;
	txdma_ctrl.bits.td_start = 0;    /* start DMA transfer */
	txdma_ctrl.bits.td_continue = 0; /* continue DMA operation */
	txdma_ctrl.bits.td_chain_mode = 1; /* chain mode */
	txdma_ctrl.bits.td_prot = 0;
	txdma_ctrl.bits.td_burst_size = 1;
	txdma_ctrl.bits.td_bus = 0;
	txdma_ctrl.bits.td_endian = 0;				// turn off by wen
	txdma_ctrl.bits.td_finish_en = 0;			// turn off by wen
	txdma_ctrl.bits.td_fail_en = 0;				// turn off by wen
	txdma_ctrl.bits.td_perr_en = 0;				// turn off by wen
	txdma_ctrl.bits.td_eod_en = 0;				// turn off by wen
	txdma_ctrl.bits.td_eof_en = 0;				// turn off by wen
	txdma_ctrl_mask.bits32 = 0;
	txdma_ctrl_mask.bits.td_start = 1;    
	txdma_ctrl_mask.bits.td_continue = 1; 
	txdma_ctrl_mask.bits.td_chain_mode = 1;
	txdma_ctrl_mask.bits.td_prot = 0xf;
	txdma_ctrl_mask.bits.td_burst_size = 3;
	txdma_ctrl_mask.bits.td_bus = 3;
	txdma_ctrl_mask.bits.td_endian = 1;
	txdma_ctrl_mask.bits.td_finish_en = 0;		// turn off by wen
	txdma_ctrl_mask.bits.td_fail_en = 0;		// turn off by wen
	txdma_ctrl_mask.bits.td_perr_en = 0;		// turn off by wen
	txdma_ctrl_mask.bits.td_eod_en = 0;			// turn off by wen
	txdma_ctrl_mask.bits.td_eof_en = 0;			// turn off by wen
	gemini_crypto_write_mask(secdev->base + CRYPTO_TXDMA_CTRL,txdma_ctrl.bits32,txdma_ctrl_mask.bits32);

	rxdma_ctrl.bits32 = 0;
	rxdma_ctrl.bits.rd_start = 0;    /* start DMA transfer */
	rxdma_ctrl.bits.rd_continue = 0; /* continue DMA operation */
	rxdma_ctrl.bits.rd_chain_mode = 1;   /* chain mode */
	rxdma_ctrl.bits.rd_prot = 0;
	rxdma_ctrl.bits.rd_burst_size = 1;
	rxdma_ctrl.bits.rd_bus = 0;
	rxdma_ctrl.bits.rd_endian = 0;
	rxdma_ctrl.bits.rd_finish_en = 0;			// turn off by wen
	rxdma_ctrl.bits.rd_fail_en = 0;				// turn off by wen
	rxdma_ctrl.bits.rd_perr_en = 0;				// turn off by wen
	rxdma_ctrl.bits.rd_eod_en = 0;				// turn off by wen
	rxdma_ctrl.bits.rd_eof_en = 1;				// turn off by wen
	rxdma_ctrl_mask.bits32 = 0;
	rxdma_ctrl_mask.bits.rd_start = 1;    
	rxdma_ctrl_mask.bits.rd_continue = 1; 
	rxdma_ctrl_mask.bits.rd_chain_mode = 1;
	rxdma_ctrl_mask.bits.rd_prot = 15;
	rxdma_ctrl_mask.bits.rd_burst_size = 3;
	rxdma_ctrl_mask.bits.rd_bus = 3;
	rxdma_ctrl_mask.bits.rd_endian = 1;
	rxdma_ctrl_mask.bits.rd_finish_en = 0;		// turn off by wen
	rxdma_ctrl_mask.bits.rd_fail_en = 0;		// turn off by wen
	rxdma_ctrl_mask.bits.rd_perr_en = 0;		// turn off by wen
	rxdma_ctrl_mask.bits.rd_eod_en = 0;			// turn off by wen
	rxdma_ctrl_mask.bits.rd_eof_en = 1;			// turn off by wen
	gemini_crypto_write_mask(secdev->base + CRYPTO_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
	
    return;	
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
	struct scatterlist *out_pkt, int out_len )
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
	unsigned int rxdma_desc;
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
	rxdma_ctrl.bits32 = gemini_crypto_read_reg(secdev->base, CRYPTO_RXDMA_CTRL);
	rxdma_ctrl.bits.rd_eof_en = 0;
	gemini_crypto_write_reg(secdev->base, CRYPTO_RXDMA_CTRL, rxdma_ctrl.bits32);
	spin_unlock_irqrestore(&secdev->irq_lock,flags2);

	// 2nd, turn on the polling flag.
	spin_lock_irqsave(&secdev->polling_lock,flags);
	secdev->polling_flag = 1;
//	if (polling_process_id != -1)
//		printk("current polling_process_id %d will be updated to %d, last_rx_pid = %d\n",
//			polling_process_id, op_info->process_id, last_rx_pid);
	secdev->polling_process_id = (int)(op_info->process_id);
	spin_unlock_irqrestore(&secdev->polling_lock,flags);

	/* get rx descriptor for this operation */
	rx_desc = secdev->tp->rx_desc_index[secdev->tp->rx_index % CRYPTO_RX_DESC_NUM];
	/* set receive buffer address for this operation */
//	consistent_sync(op_info->out_packet,op_info->pkt_len,PCI_DMA_TODEVICE);
	rx_desc->buf_adr = __pa(op_info->out_packet); //virt_to_phys(op_info->out_packet);
//	ipsec_write_reg(IPSEC_RXDMA_BUF_ADDR,rx_desc->buf_adr,0xffffffff);
	rxdma_desc = (gemini_crypto_read_reg(secdev->base, CRYPTO_RXDMA_CURR_DESC) & 0xfffffff0)+secdev->tp->rx_desc_virtual_base;
	if ((unsigned int)rx_desc == rxdma_desc) {
		gemini_crypto_write_reg(secdev->base, CRYPTO_RXDMA_BUF_ADDR, rx_desc->buf_adr);
//		consistent_sync(rx_desc,sizeof(IPSEC_DESCRIPTOR_T),PCI_DMA_TODEVICE);
	}

	if (op_info->out_pkt_len)
		rx_desc->frame_ctrl.bits.buffer_size = op_info->out_pkt_len;
	else
		rx_desc->frame_ctrl.bits.buffer_size = RX_BUF_SIZE;

	secdev->tp->rx_index++;
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
		gemini_cipher_algs[i]->secdev = crypto_info;
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

/*
static void gemini_crypto_action(void *data)
{
//	struct gemini_crypto_info *crypto_info = data;

//	reset_control_assert(crypto_info->rst);
}
*/

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
//	crypto_info->polling_flag = 0;
//	crypto_info->pid = 0;
	crypto_info->polling_process_id = -1;
	crypto_info->last_rx_pid = 255;

	crypto_buf_init(crypto_info);
	crypto_hw_start(crypto_info);
	/* Reset the crypto */
	crypto_info->reset = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(crypto_info->reset)) {
		dev_err(dev, "crypto_info->reset\n");
		return PTR_ERR(crypto_info->reset);
	}
	reset_control_reset(crypto_info->reset);

	spin_lock_init(&crypto_info->irq_lock);
	spin_lock_init(&crypto_info->queue_lock);
	spin_lock_init(&crypto_info->polling_lock);
	spin_lock_init(&crypto_info->tx_lock);
	spin_lock_init(&crypto_info->pid_lock);
//	spin_lock_init(&crypto_info->lock);

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

	crypto_info->dev = dev;
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
