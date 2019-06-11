/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __GEMINI_CRYPTO_H__
#define __GEMINI_CRYPTO_H__

#include <crypto/aes.h>
#include <crypto/des.h>
#include <crypto/algapi.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <crypto/internal/hash.h>

#include <crypto/md5.h>
#include <crypto/sha.h>

/* Cortina crypto registers */
#define CRYPTO_ID            0x0000
#define CRYPTO_CTRL          0x0004
#define CRYPTO_PACKET_PARAM  0x0008
#define CRYPTO_AUTH_PARAM    0x000C
#define CRYPTO_CIPHER_PARAM  0x0010
#define CRYPTO_AUTH_KEY_CHK  0x00A7
#define CRYPTO_STATUS_REG    0x00A8
#define CRYPTO_RAND_NUM_REG  0x00AC
#define CRYPTO_FRAME_CHKSUM  0x00B0

/************************************************/
/*          the offset of DMA register          */
/************************************************/
enum CRYPTO_DMA_REGISTER {
	CRYPTO_DMA_DEVICE_ID	= 0xff00,
	CRYPTO_DMA_STATUS	= 0xff04,
	CRYPTO_TXDMA_CTRL 	= 0xff08,
	CRYPTO_TXDMA_FIRST_DESC 	= 0xff0c,
	CRYPTO_TXDMA_CURR_DESC	= 0xff10,
	CRYPTO_RXDMA_CTRL	= 0xff14,
	CRYPTO_RXDMA_FIRST_DESC	= 0xff18,
	CRYPTO_RXDMA_CURR_DESC	= 0xff1c,
	CRYPTO_TXDMA_BUF_ADDR    = 0xff28,
	CRYPTO_RXDMA_BUF_ADDR    = 0xff38,
	CRYPTO_RXDMA_BUF_SIZE	= 0xff30,
};


/******************************************************/
/* the field definition of CRYPTO DMA Module Register  */
/******************************************************/
typedef union
{
	unsigned int bits32;
	struct bit2_ff00
	{
		unsigned int revision_id	:  4;
		unsigned int device_id		: 12;
		unsigned int 				:  8;
		unsigned int p_rclk 		:  4;	/* DMA_APB read clock period */
		unsigned int p_wclk	    	:  4;	/* DMA_APB write clock period */
	} bits;
} CRYPTO_DMA_DEVICE_ID_T;

typedef union
{
	unsigned int bits32;
	struct bit2_ff04
	{
		unsigned int intr_enable    :  8;   /* Peripheral Interrupt Enable */
		unsigned int loop_back		:  1;	/* loopback TxDMA to RxDMA */
		unsigned int 				:  3;
		unsigned int peri_reset    	:  1;   /* write 1 to this bit will cause DMA PClk domain soft reset */
		unsigned int dma_reset 		:  1;	/* write 1 to this bit will cause DMA HClk domain soft reset */
		unsigned int intr           :  8;   /* Peripheral interrupt */  
		unsigned int rs_eofi		:  1;	/* RxDMA end of frame interrupt */
		unsigned int rs_eodi		:  1;	/* RxDMA end of descriptor interrupt */
		unsigned int rs_perr		:  1;   /* Rx Descriptor protocol error */
		unsigned int rs_derr		:  1;   /* AHB Bus Error while rx */ 
		unsigned int rs_finish		:  1;   /* finished rx interrupt */
		unsigned int ts_eofi		:  1;   /* TxDMA end of frame interrupt */
		unsigned int ts_eodi		:  1;	/* TxDMA end of descriptor interrupt */
		unsigned int ts_perr		:  1;   /* Tx Descriptor protocol error */
		unsigned int ts_derr		:  1;   /* AHB Bus Error while tx */ 
		unsigned int ts_finish		:  1;	/* finished tx interrupt */
	} bits;
} CRYPTO_DMA_STATUS_T;

typedef union
{
	unsigned int bits32;
	struct bit2_ff08
	{
		unsigned int 				: 14;
		unsigned int td_eof_en      :  1;   /* End of frame interrupt Enable;1-enable;0-mask */
		unsigned int td_eod_en  	:  1;	/* End of Descriptor interrupt Enable;1-enable;0-mask */
		unsigned int td_perr_en 	:  1;	/* Protocol Failure Interrupt Enable;1-enable;0-mask */
		unsigned int td_fail_en 	:  1;	/* DMA Fail Interrupt Enable;1-enable;0-mask */
		unsigned int td_finish_en   :  1;	/* DMA Finish Event Interrupt Enable;1-enable;0-mask */
		unsigned int td_endian		:  1;	/* AHB Endian. 0-little endian; 1-big endian */
		unsigned int td_bus		    :  2;	/* peripheral bus width;0 - 8 bits;1 - 16 bits */
		unsigned int td_burst_size  :  2;	/* TxDMA max burst size for every AHB request */
		unsigned int td_prot		:  4;	/* TxDMA protection control */
		unsigned int 				:  1;
		unsigned int td_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int td_continue	:  1;   /* Continue DMA operation */
		unsigned int td_start		:  1;	/* Start DMA transfer */
	} bits;
} CRYPTO_TXDMA_CTRL_T;
				
typedef union 
{
	unsigned int bits32;
	struct bit2_ff0c
	{
		unsigned int 					:  3;
		unsigned int td_busy			:  1;/* 1-TxDMA busy; 0-TxDMA idle */
		unsigned int td_first_des_ptr	: 28;/* first descriptor address */
	} bits;
} CRYPTO_TXDMA_FIRST_DESC_T;					

typedef union
{
	unsigned int bits32;
	struct bit2_ff10
	{
		unsigned int sof_eof		:  2;
		unsigned int dec			:  1;	/* AHB bus address increment(0)/decrement(1) */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int ndar			: 28;	/* next descriptor address */
	} bits;
} CRYPTO_TXDMA_CURR_DESC_T;
			

typedef union
{
	unsigned int bits32;
	struct bit2_ff14
	{
		unsigned int 				: 14;
		unsigned int rd_eof_en      :  1;   /* End of frame interrupt Enable;1-enable;0-mask */
		unsigned int rd_eod_en  	:  1;	/* End of Descriptor interrupt Enable;1-enable;0-mask */
		unsigned int rd_perr_en 	:  1;	/* Protocol Failure Interrupt Enable;1-enable;0-mask */
		unsigned int rd_fail_en  	:  1;	/* DMA Fail Interrupt Enable;1-enable;0-mask */
		unsigned int rd_finish_en   :  1;	/* DMA Finish Event Interrupt Enable;1-enable;0-mask */
		unsigned int rd_endian		:  1;	/* AHB Endian. 0-little endian; 1-big endian */
		unsigned int rd_bus		    :  2;	/* peripheral bus width;0 - 8 bits;1 - 16 bits */
		unsigned int rd_burst_size  :  2;	/* DMA max burst size for every AHB request */
		unsigned int rd_prot		:  4;	/* DMA protection control */
		unsigned int 				:  1;
		unsigned int rd_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int rd_continue	:  1;   /* Continue DMA operation */
		unsigned int rd_start		:  1;	/* Start DMA transfer */
	} bits;
} CRYPTO_RXDMA_CTRL_T;
				
typedef union 
{
	unsigned int bits32;
	struct bit2_ff18
	{
		unsigned int 					:  3;
		unsigned int rd_busy			:  1;/* 1-RxDMA busy; 0-RxDMA idle */
		unsigned int rd_first_des_ptr	: 28;/* first descriptor address */
	} bits;
} CRYPTO_RXDMA_FIRST_DESC_T;					

typedef union
{
	unsigned int bits32;
	struct bit2_ff1c
	{
		unsigned int sof_eof		:  2;
		unsigned int dec			:  1;	/* AHB bus address increment(0)/decrement(1) */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int ndar			: 28;	/* next descriptor address */
	} bits;
} CRYPTO_RXDMA_CURR_DESC_T;



/******************************************************/
/*    the field definition of CRYPTO module Register   */ 
/******************************************************/
typedef union
{
	unsigned int id;
	struct bit_0000
	{
		unsigned int revision_id	:  4;
		unsigned int device_id		: 28;	
	} bits;
} CRYPTO_ID_T;

typedef union
{
    unsigned int control;
    struct bit_0004
    {
        unsigned int process_id         :  8; /* Used to identify process.This number will be */
                                              /* copied to the descriptor status of received packet*/ 
        unsigned int auth_check_len     :  3; /* Number of 32-bit words to be check or appended */
                                              /* by the authentication module */   
        unsigned int                    :  1;
        unsigned int auth_algorithm     :  3; 
        unsigned int auth_mode          :  1; /* 0-Append or 1-Check Authentication Result */
        unsigned int fcs_stream_copy    :  1; /* enable authentication stream copy */
        unsigned int                    :  2;
        unsigned int mix_key_sel        :  1; /* 0:use rCipherKey0-3  1:use Key Mixer */
        unsigned int aesnk              :  4; /* AES Key Size */
        unsigned int cipher_algorithm   :  3; 
        unsigned int                    :  1;
        unsigned int op_mode            :  4; /* Operation Mode for the CRYPTO Module */
    } bits;
} CRYPTO_CONTROL_T;
                                             

typedef union
{
    unsigned int cipher_packet;
    struct bit_00080
    {
        unsigned int cipher_algorithm_len : 16; /* The length of message body to be encrypted/decrypted */  
        unsigned int cipher_header_len    : 16; /* The header length to be skipped by the cipher */
    } bits;
} CRYPTO_CIPHER_PACKET_T;

typedef union
{
    unsigned int auth_packet;
    struct bit_000c0
    {
        unsigned int auth_algorithm_len : 16; /* The length of message body that is to be authenticated */
        unsigned int auth_header_len    : 16; /* The header length that is to be skipped by the authenticator */
    } bits;
} CRYPTO_AUTH_PACKET_T;        

typedef union
{
    unsigned int status;
    struct bit_00a8
    {
        unsigned int cipher_err_code:  4; /* Cipher module erroe code */
        unsigned int auth_err_code  :  4; /* Authentication module error code */
		/* for auth_err_code and cipher_err_code ******
		 * 0: Normal
		 * 1: Input Frame is shorter than what it expects
		 * 2: Input Frame is longer than what it expects
		 **********************************************/
        unsigned int parser_err_code:  4; /* Authentication Compare result */
        unsigned int                : 16;
        unsigned int ccm_mic_ok     :  1; /* CCM Mic compare result */
        unsigned int tkip_mic_ok    :  1; /* TKIP Mic compare result */
        unsigned int wep_crc_ok     :  1; /* WEP ICV compare result */
        unsigned int auth_cmp_rslt  :  1; /* Authentication Compare result */
    } bits;
} CRYPTO_STATUS_T;

        
 
/************************************************************************/
/*              CRYPTO Descriptor Format                                 */
/************************************************************************/		
typedef struct descriptor_t
{
	union frame_control_t
	{
		unsigned int bits32;
		struct bits_0000
		{
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 6;	/* number of descriptors used for the current frame */
			unsigned int            : 6;    /* checksum[15:8] */
			unsigned int            : 1;    /* authentication compare result */
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
		} bits;		
	} frame_ctrl;
	
	union flag_status_t
	{
		unsigned int bits32;
		struct bits_0004
		{
			unsigned int frame_count:16;	
			unsigned int process_id : 8;
			unsigned int ccmp_mic_ok: 1;
			unsigned int tkip_mic_ok: 1;
			unsigned int wep_crc_ok : 1;
			unsigned int auth_result: 1;
			unsigned int            : 4;
//            unsigned int checksum   : 8; /* checksum[7:0] */ 
		} bits_rx_status;
		
		struct bits_0005
		{
			unsigned int frame_count:16;	
			unsigned int process_id : 8;	
            unsigned int            : 8; 
		} bits_tx_status;

		struct bits_0006
		{
			unsigned int tqflag     :10;    
			unsigned int            :22;
		} bits_tx_flag;
	} flag_status;

	unsigned int buf_adr;	/* data buffer address */	
	
	union next_desc_t
	{
		unsigned int next_descriptor;
		struct bits_000c
		{
			unsigned int sof_eof	: 2;	/* 00-the linking descriptor   01-the last descriptor of a frame*/
			                                /* 10-the first descriptor of a frame    11-only one descriptor for a frame*/
			unsigned int dec   		: 1;	/* AHB bus address. 0-increment; 1-decrement */   
			unsigned int eofie		: 1;	/* end of frame interrupt enable */
			unsigned int ndar		:28;	/* next descriptor address */
		} bits;                    			
	} next_desc;					        
} CRYPTO_DESCRIPTOR_T;		            	


typedef struct CRYPTO_S
{
    unsigned char       *tx_bufs;
    unsigned char       *rx_bufs;
	CRYPTO_DESCRIPTOR_T	*tx_desc;	    /* point to virtual TX descriptor address*/
	CRYPTO_DESCRIPTOR_T	*rx_desc;	    /* point to virtual RX descriptor address*/
	CRYPTO_DESCRIPTOR_T	*tx_cur_desc;	/* point to current TX descriptor */
	CRYPTO_DESCRIPTOR_T	*rx_cur_desc;	/* point to current RX descriptor */
	CRYPTO_DESCRIPTOR_T  *tx_finished_desc;
	CRYPTO_DESCRIPTOR_T  *rx_finished_desc;
	dma_addr_t          rx_desc_dma;	/* physical RX descriptor address */
	dma_addr_t          tx_desc_dma;    /* physical TX descriptor address */
	dma_addr_t          rx_bufs_dma;    /* physical RX descriptor address */
	dma_addr_t          tx_bufs_dma;    /* physical TX descriptor address */
} CRYPTO_T;


#if 0
#define _SBF(v, f)			((v) << (f))

/* Crypto control registers*/
#define RK_CRYPTO_INTSTS		0x0000
#define RK_CRYPTO_PKA_DONE_INT		BIT(5)
#define RK_CRYPTO_HASH_DONE_INT		BIT(4)
#define RK_CRYPTO_HRDMA_ERR_INT		BIT(3)
#define RK_CRYPTO_HRDMA_DONE_INT	BIT(2)
#define RK_CRYPTO_BCDMA_ERR_INT		BIT(1)
#define RK_CRYPTO_BCDMA_DONE_INT	BIT(0)

#define RK_CRYPTO_INTENA		0x0004
#define RK_CRYPTO_PKA_DONE_ENA		BIT(5)
#define RK_CRYPTO_HASH_DONE_ENA		BIT(4)
#define RK_CRYPTO_HRDMA_ERR_ENA		BIT(3)
#define RK_CRYPTO_HRDMA_DONE_ENA	BIT(2)
#define RK_CRYPTO_BCDMA_ERR_ENA		BIT(1)
#define RK_CRYPTO_BCDMA_DONE_ENA	BIT(0)

#define RK_CRYPTO_CTRL			0x0008
#define RK_CRYPTO_WRITE_MASK		_SBF(0xFFFF, 16)
#define RK_CRYPTO_TRNG_FLUSH		BIT(9)
#define RK_CRYPTO_TRNG_START		BIT(8)
#define RK_CRYPTO_PKA_FLUSH		BIT(7)
#define RK_CRYPTO_HASH_FLUSH		BIT(6)
#define RK_CRYPTO_BLOCK_FLUSH		BIT(5)
#define RK_CRYPTO_PKA_START		BIT(4)
#define RK_CRYPTO_HASH_START		BIT(3)
#define RK_CRYPTO_BLOCK_START		BIT(2)
#define RK_CRYPTO_TDES_START		BIT(1)
#define RK_CRYPTO_AES_START		BIT(0)

#define RK_CRYPTO_CONF			0x000c
/* HASH Receive DMA Address Mode:   fix | increment */
#define RK_CRYPTO_HR_ADDR_MODE		BIT(8)
/* Block Transmit DMA Address Mode: fix | increment */
#define RK_CRYPTO_BT_ADDR_MODE		BIT(7)
/* Block Receive DMA Address Mode:  fix | increment */
#define RK_CRYPTO_BR_ADDR_MODE		BIT(6)
#define RK_CRYPTO_BYTESWAP_HRFIFO	BIT(5)
#define RK_CRYPTO_BYTESWAP_BTFIFO	BIT(4)
#define RK_CRYPTO_BYTESWAP_BRFIFO	BIT(3)
/* AES = 0 OR DES = 1 */
#define RK_CRYPTO_DESSEL				BIT(2)
#define RK_CYYPTO_HASHINSEL_INDEPENDENT_SOURCE		_SBF(0x00, 0)
#define RK_CYYPTO_HASHINSEL_BLOCK_CIPHER_INPUT		_SBF(0x01, 0)
#define RK_CYYPTO_HASHINSEL_BLOCK_CIPHER_OUTPUT		_SBF(0x02, 0)

/* Block Receiving DMA Start Address Register */
#define RK_CRYPTO_BRDMAS		0x0010
/* Block Transmitting DMA Start Address Register */
#define RK_CRYPTO_BTDMAS		0x0014
/* Block Receiving DMA Length Register */
#define RK_CRYPTO_BRDMAL		0x0018
/* Hash Receiving DMA Start Address Register */
#define RK_CRYPTO_HRDMAS		0x001c
/* Hash Receiving DMA Length Register */
#define RK_CRYPTO_HRDMAL		0x0020

/* AES registers */
#define RK_CRYPTO_AES_CTRL			  0x0080
#define RK_CRYPTO_AES_BYTESWAP_CNT	BIT(11)
#define RK_CRYPTO_AES_BYTESWAP_KEY	BIT(10)
#define RK_CRYPTO_AES_BYTESWAP_IV	BIT(9)
#define RK_CRYPTO_AES_BYTESWAP_DO	BIT(8)
#define RK_CRYPTO_AES_BYTESWAP_DI	BIT(7)
#define RK_CRYPTO_AES_KEY_CHANGE	BIT(6)
#define RK_CRYPTO_AES_ECB_MODE		_SBF(0x00, 4)
#define RK_CRYPTO_AES_CBC_MODE		_SBF(0x01, 4)
#define RK_CRYPTO_AES_CTR_MODE		_SBF(0x02, 4)
#define RK_CRYPTO_AES_128BIT_key	_SBF(0x00, 2)
#define RK_CRYPTO_AES_192BIT_key	_SBF(0x01, 2)
#define RK_CRYPTO_AES_256BIT_key	_SBF(0x02, 2)
/* Slave = 0 / fifo = 1 */
#define RK_CRYPTO_AES_FIFO_MODE		BIT(1)
/* Encryption = 0 , Decryption = 1 */
#define RK_CRYPTO_AES_DEC		BIT(0)

#define RK_CRYPTO_AES_STS		0x0084
#define RK_CRYPTO_AES_DONE		BIT(0)

/* AES Input Data 0-3 Register */
#define RK_CRYPTO_AES_DIN_0		0x0088
#define RK_CRYPTO_AES_DIN_1		0x008c
#define RK_CRYPTO_AES_DIN_2		0x0090
#define RK_CRYPTO_AES_DIN_3		0x0094

/* AES output Data 0-3 Register */
#define RK_CRYPTO_AES_DOUT_0		0x0098
#define RK_CRYPTO_AES_DOUT_1		0x009c
#define RK_CRYPTO_AES_DOUT_2		0x00a0
#define RK_CRYPTO_AES_DOUT_3		0x00a4

/* AES IV Data 0-3 Register */
#define RK_CRYPTO_AES_IV_0		0x00a8
#define RK_CRYPTO_AES_IV_1		0x00ac
#define RK_CRYPTO_AES_IV_2		0x00b0
#define RK_CRYPTO_AES_IV_3		0x00b4

/* AES Key Data 0-3 Register */
#define RK_CRYPTO_AES_KEY_0		0x00b8
#define RK_CRYPTO_AES_KEY_1		0x00bc
#define RK_CRYPTO_AES_KEY_2		0x00c0
#define RK_CRYPTO_AES_KEY_3		0x00c4
#define RK_CRYPTO_AES_KEY_4		0x00c8
#define RK_CRYPTO_AES_KEY_5		0x00cc
#define RK_CRYPTO_AES_KEY_6		0x00d0
#define RK_CRYPTO_AES_KEY_7		0x00d4

/* des/tdes */
#define RK_CRYPTO_TDES_CTRL		0x0100
#define RK_CRYPTO_TDES_BYTESWAP_KEY	BIT(8)
#define RK_CRYPTO_TDES_BYTESWAP_IV	BIT(7)
#define RK_CRYPTO_TDES_BYTESWAP_DO	BIT(6)
#define RK_CRYPTO_TDES_BYTESWAP_DI	BIT(5)
/* 0: ECB, 1: CBC */
#define RK_CRYPTO_TDES_CHAINMODE_CBC	BIT(4)
/* TDES Key Mode, 0 : EDE, 1 : EEE */
#define RK_CRYPTO_TDES_EEE		BIT(3)
/* 0: DES, 1:TDES */
#define RK_CRYPTO_TDES_SELECT		BIT(2)
/* 0: Slave, 1:Fifo */
#define RK_CRYPTO_TDES_FIFO_MODE	BIT(1)
/* Encryption = 0 , Decryption = 1 */
#define RK_CRYPTO_TDES_DEC		BIT(0)

#define RK_CRYPTO_TDES_STS		0x0104
#define RK_CRYPTO_TDES_DONE		BIT(0)

#define RK_CRYPTO_TDES_DIN_0		0x0108
#define RK_CRYPTO_TDES_DIN_1		0x010c
#define RK_CRYPTO_TDES_DOUT_0		0x0110
#define RK_CRYPTO_TDES_DOUT_1		0x0114
#define RK_CRYPTO_TDES_IV_0		0x0118
#define RK_CRYPTO_TDES_IV_1		0x011c
#define RK_CRYPTO_TDES_KEY1_0		0x0120
#define RK_CRYPTO_TDES_KEY1_1		0x0124
#define RK_CRYPTO_TDES_KEY2_0		0x0128
#define RK_CRYPTO_TDES_KEY2_1		0x012c
#define RK_CRYPTO_TDES_KEY3_0		0x0130
#define RK_CRYPTO_TDES_KEY3_1		0x0134

/* HASH */
#define RK_CRYPTO_HASH_CTRL		0x0180
#define RK_CRYPTO_HASH_SWAP_DO		BIT(3)
#define RK_CRYPTO_HASH_SWAP_DI		BIT(2)
#define RK_CRYPTO_HASH_SHA1		_SBF(0x00, 0)
#define RK_CRYPTO_HASH_MD5		_SBF(0x01, 0)
#define RK_CRYPTO_HASH_SHA256		_SBF(0x02, 0)
#define RK_CRYPTO_HASH_PRNG		_SBF(0x03, 0)

#define RK_CRYPTO_HASH_STS		0x0184
#define RK_CRYPTO_HASH_DONE		BIT(0)

#define RK_CRYPTO_HASH_MSG_LEN		0x0188
#define RK_CRYPTO_HASH_DOUT_0		0x018c
#define RK_CRYPTO_HASH_DOUT_1		0x0190
#define RK_CRYPTO_HASH_DOUT_2		0x0194
#define RK_CRYPTO_HASH_DOUT_3		0x0198
#define RK_CRYPTO_HASH_DOUT_4		0x019c
#define RK_CRYPTO_HASH_DOUT_5		0x01a0
#define RK_CRYPTO_HASH_DOUT_6		0x01a4
#define RK_CRYPTO_HASH_DOUT_7		0x01a8

#define CRYPTO_READ(dev, offset)		  \
		readl_relaxed(((dev)->reg + (offset)))
#define CRYPTO_WRITE(dev, offset, val)	  \
		writel_relaxed((val), ((dev)->reg + (offset)))

struct gemini_crypto_info {
	struct device			*dev;
	struct clk			*aclk;
	struct clk			*hclk;
	struct clk			*sclk;
	struct clk			*dmaclk;
	struct reset_control		*rst;
	void __iomem			*reg;
	int				irq;
	struct crypto_queue		queue;
	struct tasklet_struct		queue_task;
	struct tasklet_struct		done_task;
	struct crypto_async_request	*async_req;
	int 				err;
	/* device lock */
	spinlock_t			lock;

	/* the public variable */
	struct scatterlist		*sg_src;
	struct scatterlist		*sg_dst;
	struct scatterlist		sg_tmp;
	struct scatterlist		*first;
	unsigned int			left_bytes;
	void				*addr_vir;
	int				aligned;
	int				align_size;
	size_t				nents;
	unsigned int			total;
	unsigned int			count;
	dma_addr_t			addr_in;
	dma_addr_t			addr_out;
	bool				busy;
	int (*start)(struct gemini_crypto_info *dev);
	int (*update)(struct gemini_crypto_info *dev);
	void (*complete)(struct crypto_async_request *base, int err);
	int (*enable_clk)(struct gemini_crypto_info *dev);
	void (*disable_clk)(struct gemini_crypto_info *dev);
	int (*load_data)(struct gemini_crypto_info *dev,
			 struct scatterlist *sg_src,
			 struct scatterlist *sg_dst);
	void (*unload_data)(struct gemini_crypto_info *dev);
	int (*enqueue)(struct gemini_crypto_info *dev,
		       struct crypto_async_request *async_req);
};



#endif // 0

struct gemini_crypto_info {
	void __iomem			*base;
	struct device			*dev;
	struct clk			*pclk;
	int				irq;

	struct aes_txdesc		*tx;
	struct aes_rxdesc		*rx;
	dma_addr_t			phy_tx;
	dma_addr_t			phy_rx;
	dma_addr_t			phy_rec;

	struct list_head		aes_list;

	struct tasklet_struct		done_tasklet;
	unsigned int			rec_front_idx;
	unsigned int			rec_rear_idx;
	struct mtk_dma_rec		*rec;
	unsigned int			count;
	spinlock_t			lock;
	spinlock_t			irq_lock;
	spinlock_t			tx_lock;
	unsigned int			polling_flag;

};

/* the private variable of hash */
struct gemini_ahash_ctx {
	struct gemini_crypto_info		*dev;
	/* for fallback */
	struct crypto_ahash		*fallback_tfm;
};

/* the private variable of hash for fallback */
struct gemini_ahash_rctx {
	struct ahash_request		fallback_req;
	u32				mode;
};

/* the private variable of cipher */
struct gemini_cipher_ctx {
	struct gemini_crypto_info		*dev;
	unsigned int			keylen;
	u32				mode;
};

enum alg_type {
	ALG_TYPE_HASH,
	ALG_TYPE_CIPHER,
};

struct gemini_crypto_tmp {
	struct gemini_crypto_info		*dev;
	union {
		struct crypto_alg	crypto;
		struct ahash_alg	hash;
	} alg;
	enum alg_type			type;
};

extern struct gemini_crypto_tmp gemini_ecb_aes_alg;
extern struct gemini_crypto_tmp gemini_cbc_aes_alg;
extern struct gemini_crypto_tmp gemini_ecb_des_alg;
extern struct gemini_crypto_tmp gemini_cbc_des_alg;
extern struct gemini_crypto_tmp gemini_ecb_des3_ede_alg;
extern struct gemini_crypto_tmp gemini_cbc_des3_ede_alg;

extern struct gemini_crypto_tmp gemini_ahash_sha1;
extern struct gemini_crypto_tmp gemini_ahash_sha256;
extern struct gemini_crypto_tmp gemini_ahash_md5;

#endif
