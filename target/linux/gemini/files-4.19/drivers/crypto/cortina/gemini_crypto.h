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

#define MAX_POLLING_LOOPS 	40000
#define RX_POLL_NUM 		10
#define CRYPTO_RX_DESC_NUM 	64
#define CRYPTO_TX_DESC_NUM 	128
#define RX_BUF_SIZE 		8192
#define TX_BUF_SIZE 		2048

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
	CRYPTO_DMA_DEVICE_ID     = 0xff00,
	CRYPTO_DMA_STATUS        = 0xff04,
	CRYPTO_TXDMA_CTRL        = 0xff08,
	CRYPTO_TXDMA_FIRST_DESC  = 0xff0c,
	CRYPTO_TXDMA_CURR_DESC   = 0xff10,
	CRYPTO_RXDMA_CTRL        = 0xff14,
	CRYPTO_RXDMA_FIRST_DESC  = 0xff18,
	CRYPTO_RXDMA_CURR_DESC   = 0xff1c,
	CRYPTO_TXDMA_BUF_ADDR    = 0xff28,
	CRYPTO_RXDMA_BUF_SIZE    = 0xff30,
	CRYPTO_RXDMA_BUF_ADDR    = 0xff38,
	CRYPTO_RXDMA_MAGIC       = 0xff40,
};

/* define owner bit */
enum CRYPTO_OWN_BIT {
    CPU = 0,
    DMA	= 1
};   

/* define cipher algorithm */
enum CRYPTO_CIPHER {
	DES_ECB_E	=20,
	TDES_ECB_E	=21,
	AES_ECB_E	=22,
	DES_CBC_E	=24,
	TDES_CBC_E	=25,
	AES_CBC_E	=26,
	
	DES_ECB_D	=27,
	TDES_ECB_D	=28,
	AES_ECB_D	=29,
	DES_CBC_D	=31,
	TDES_CBC_D	=32,
	AES_CBC_D	=33,
	A_SHA1      =12,
	A_HMAC_SHA1 =13,
	A_MD5       =14,
	A_HMAC_MD5  =15,
};

// opMode
enum CRYPTO_OPMODE {
	CIPHER_ENC = 0x1,
	CIPHER_DEC = 0x3,
	AUTH       = 0x4,
	ENC_AUTH   = 0x5,
	AUTH_DEC   = 0x7,
};

// cipherAlgorithm
enum CRYPTO_CIPHER_ALGORITHM {
	CBC_DES    =  0x4,
	CBC_3DES   =  0x5,
	CBC_AES    =  0x6,
	ECB_DES    =  0x0,
	ECB_3DES   =  0x1,
	ECB_AES    =  0x2,
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
	CRYPTO_DESCRIPTOR_T	*tx_desc;	/* pointer to virtual TX descriptor address*/
	CRYPTO_DESCRIPTOR_T	*rx_desc;	/* pointer to virtual RX descriptor address*/
	CRYPTO_DESCRIPTOR_T	*tx_cur_desc;	/* pointer to current TX descriptor */
	CRYPTO_DESCRIPTOR_T	*rx_cur_desc;	/* pointer to current RX descriptor */
	CRYPTO_DESCRIPTOR_T	*tx_finished_desc;
	CRYPTO_DESCRIPTOR_T	*rx_finished_desc;
	dma_addr_t		rx_desc_dma;	/* physical RX descriptor address */
	dma_addr_t		tx_desc_dma;    /* physical TX descriptor address */
//    unsigned char       *tx_bufs;
//    unsigned char       *rx_bufs;
//	dma_addr_t          rx_bufs_dma;    /* physical RX descriptor address */
//	dma_addr_t          tx_bufs_dma;    /* physical TX descriptor address */
	unsigned int		tx_desc_virtual_base;
	unsigned int		rx_desc_virtual_base;
	unsigned int		rx_index;
	CRYPTO_DESCRIPTOR_T     *rx_desc_index[CRYPTO_RX_DESC_NUM];
} CRYPTO_T;

/*=====================================================================================================*/
/*  Data Structure of Control Packet  */
/*=====================================================================================================*/    
typedef struct CRYPTO_ECB_AUTH_S
{
    CRYPTO_CONTROL_T         control; /* control parameter */
    CRYPTO_CIPHER_PACKET_T   cipher; /* cipher packet parameter */
    CRYPTO_AUTH_PACKET_T     auth;   /* authentication packet parameter */
    unsigned char           cipher_key[8*4];
    unsigned char           auth_check_val[5*4];
} CRYPTO_ECB_AUTH_T;    

typedef struct CRYPTO_CBC_AUTH_S
{
    CRYPTO_CONTROL_T         control; /* control parameter */
    CRYPTO_CIPHER_PACKET_T   cipher; /* cipher packet parameter */
    CRYPTO_AUTH_PACKET_T     auth;   /* authentication packet parameter */
    unsigned char           cipher_iv[4*4]; 
    unsigned char           cipher_key[8*4];
    unsigned char           auth_check_val[5*4];
} CRYPTO_CBC_AUTH_T;    

typedef struct CRYPTO_ECB_HMAC_AUTH_S
{
    CRYPTO_CONTROL_T         control; /* control parameter */
    CRYPTO_CIPHER_PACKET_T   cipher; /* cipher packet parameter */
    CRYPTO_AUTH_PACKET_T     auth;   /* authentication packet parameter */
    unsigned char           cipher_key[8*4];
    unsigned char           auth_key[16*4];
    unsigned char           auth_check_val[5*4];
} CRYPTO_ECB_AUTH_HMAC_T;    

typedef struct CRYPTO_CBC_HMAC_AUTH_S
{
    CRYPTO_CONTROL_T         control; /* control parameter */
    CRYPTO_CIPHER_PACKET_T   cipher; /* cipher packet parameter */
    CRYPTO_AUTH_PACKET_T     auth;   /* authentication packet parameter */
    unsigned char           cipher_iv[4*4]; 
    unsigned char           cipher_key[8*4];
    unsigned char           auth_key[16*4];
    unsigned char           auth_check_val[5*4];
} CRYPTO_CBC_AUTH_HMAC_T;    

typedef struct CRYPTO_HMAC_AUTH_S
{
    CRYPTO_CONTROL_T         control; /* control parameter */
    CRYPTO_AUTH_PACKET_T     auth;   /* authentication packet parameter */
    unsigned char           auth_key[16*4];
    unsigned char           auth_check_val[5*4];
} CRYPTO_HMAC_AUTH_T;    

typedef union 
{
    unsigned char auth_pkt[28];
    
    struct CRYPTO_AUTH_S
    {
        CRYPTO_CONTROL_T         control; /* control parameter(4-byte) */
        CRYPTO_AUTH_PACKET_T     auth;   /* authentication packet parameter(4-byte) */
        unsigned char           auth_check_val[5*4];
    } var;    
} CRYPTO_AUTH_T;    
    
typedef struct CRYPTO_CIPHER_CBC_S
{
    CRYPTO_CONTROL_T         control; /* control parameter */
    CRYPTO_CIPHER_PACKET_T   cipher; /* cipher packet parameter */
    unsigned char           cipher_iv[4*4]; 
    unsigned char           cipher_key[8*4];
} CRYPTO_CIPHER_CBC_T;    

typedef struct CRYPTO_CIPHER_ECB_S
{
    CRYPTO_CONTROL_T         control; /* control parameter */
    CRYPTO_CIPHER_PACKET_T   cipher; /* cipher packet parameter */
    unsigned char           cipher_key[8*4];
} CRYPTO_CIPHER_ECB_T; 


struct CRYPTO_PACKET_S
{
    unsigned int    op_mode;            /* CIPHER_ENC(1),CIPHER_DEC(3),AUTH(4),ENC_AUTH(5),AUTH_DEC(7) */
    unsigned int    cipher_algorithm;   /* ECB_DES(0),ECB_3DES(1),ECB_AES(2),CBC_DES(4),CBC_3DES(5),CBC_AES(6) */
    unsigned int    auth_algorithm;     /* SHA1(0),MD5(1),HMAC_SHA1(2),HMAC_MD5(3),FCS(4) */
    unsigned int    auth_result_mode;   /* AUTH_APPEND(0),AUTH_CHKVAL(1) */
    unsigned int    process_id;         /* Used to identify the process */
    unsigned int    auth_header_len;    /* Header length to be skipped by the authenticator */
    unsigned int    auth_algorithm_len; /* Length of message body that is to be authenticated */
    unsigned int    cipher_header_len;  /* Header length to be skipped by the cipher */
    unsigned int    cipher_algorithm_len;   /* Length of message body to be encrypted or decrypted */
    unsigned char   iv[16];             /* Initial vector used for DES,3DES,AES */
    unsigned int    iv_size;            /* Initial vector size */
    unsigned char   auth_key[64];       /* authentication key */
    unsigned int    auth_key_size;      /* authentication key size */
    unsigned char   cipher_key[32];     /* cipher key */
    unsigned int    cipher_key_size;    /* cipher key size */
    struct scatterlist *in_packet;         /* input_packet buffer pointer */
    //unsigned char		*in_packet;         /* input_packet buffer pointer */
    unsigned int    pkt_len;            /* input total packet length */
    unsigned char   auth_checkval[20];  /* Authentication check value/FCS check value */
    struct CRYPTO_PACKET_S *next,*prev;        /* pointer to next/previous operation to perform on buffer */
    void (*callback)(struct CRYPTO_PACKET_S *); /* function to call when done authentication/cipher */ 
    unsigned char   *out_packet;        /* output_packet buffer pointer */
    //struct scatterlist *out_packet;        /* output_packet buffer pointer */
    unsigned int    out_pkt_len;        /* output total packet length */
    unsigned int    auth_cmp_result;    /* authentication compare result */
    unsigned int    checksum;           /* checksum value */
    unsigned int    status;             /* return status. 0:success, others:fail */
//#if (IPSEC_TEST == 1)          
//    unsigned char    *sw_packet;         /* for test only */
//    unsigned int    sw_pkt_len;         /* for test only */
//#endif    
} ;

typedef struct CRYPTO_PACKET_S qhead;



struct gemini_crypto_info {
	void __iomem		*base;
	struct device		*dev;
	struct clk		*pclk;
	int			irq;
	struct reset_control 	*reset;

//	struct aes_txdesc	*tx;
//	struct aes_rxdesc	*rx;
//	dma_addr_t		phy_tx;
//	dma_addr_t		phy_rx;
//	dma_addr_t		phy_rec;
//	struct list_head	aes_list;
//	unsigned int     	tx_desc_virtual_base;
//	unsigned int     	rx_desc_virtual_base;


	struct tasklet_struct	done_tasklet;
//	unsigned int		rec_front_idx;
//	unsigned int		rec_rear_idx;
//	struct mtk_dma_rec	*rec;
//	unsigned int		count;
//	spinlock_t		lock;
	spinlock_t		irq_lock;
	spinlock_t		tx_lock;
	spinlock_t		queue_lock;
	spinlock_t		polling_lock;
	spinlock_t		pid_lock;
	unsigned int		polling_flag;
	int			polling_process_id;
	unsigned int 		pid;
	unsigned int		last_rx_pid;
	CRYPTO_T		*tp;
	qhead			*queue;
};

/* the private variable of hash */
struct gemini_ahash_ctx {
	struct gemini_crypto_info	*dev;
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
	struct gemini_crypto_info	*secdev;
//	unsigned int			keylen;
//	u32				mode;
	CRYPTO_CIPHER_CBC_T       	cbc;
	CRYPTO_CIPHER_ECB_T       	ecb;
//	struct crypto_skcipher		*fallback;
//	struct CRYPTO_PACKET_S		op;
};

enum alg_type {
	ALG_TYPE_HASH,
	ALG_TYPE_CIPHER,
};

struct gemini_crypto_tmp {
	struct gemini_crypto_info	*secdev;
	union {
		struct crypto_alg	crypto;
		struct ahash_alg	hash;
	} alg;
	enum alg_type			type;
};

void crypto_hw_cipher(struct gemini_crypto_info *secdev, unsigned char *ctrl_pkt,int ctrl_len,
	struct scatterlist *data_pkt, int data_len, unsigned int tqflag,
	struct scatterlist *out_pkt, int out_len );
int crypto_hw_process(struct gemini_crypto_info *secdev, struct CRYPTO_PACKET_S  *op_info);
void gemini_key_swap(unsigned char *out_key, const unsigned char *in_key, unsigned int in_len);


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
