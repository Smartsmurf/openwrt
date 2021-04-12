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
#include <crypto/internal/skcipher.h>
#include "gemini_crypto.h"

// #define RK_CRYPTO_DEC			BIT(0)

//static void gemini_crypto_complete(struct crypto_async_request *base, int err)
//{
//	if (base->complete)
//		base->complete(base, err);
//}

//static int gemini_handle_req(struct gemini_crypto_info *dev,
//			 struct ablkcipher_request *req)
//{
//	if (!IS_ALIGNED(req->nbytes, dev->align_size))
//		return -EINVAL;
//	else
//		return dev->enqueue(dev, &req->base);
//}

static int gemini_aes_setkey(struct crypto_ablkcipher *cipher,
			 const u8 *key, unsigned int keylen)
{
	struct gemini_cipher_ctx *ctx = crypto_ablkcipher_ctx(cipher);
	struct CRYPTO_CIPHER_ECB_S *ecb = &ctx->ecb;
	struct CRYPTO_CIPHER_CBC_S *cbc = &ctx->cbc;

	switch( keylen ){
	case AES_KEYSIZE_128:
	case AES_KEYSIZE_192:
	case AES_KEYSIZE_256:
		break;
	default:
		crypto_ablkcipher_set_flags(cipher, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}
	/* crypto is MSB */
	gemini_key_swap( ecb->cipher_key, key, keylen );
	gemini_key_swap( cbc->cipher_key, key, keylen );
 
	/* keysize is in bytes -> crypto word size is 32 bits */
	keylen = keylen >> 2;

        ecb->control.bits.aesnk = keylen; /* AES key size */ 
        cbc->control.bits.aesnk = keylen; /* AES key size */ 
	return (0);
}

static int gemini_aes_ecb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct gemini_cipher_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	struct CRYPTO_CIPHER_ECB_S *ecb = &ctx->ecb;
	int	ret = 0;

	ecb->control.bits.op_mode = CIPHER_ENC;		/* cipher encryption */
	ecb->control.bits.cipher_algorithm = ECB_AES;	/* AES ECB mode */ 
	ecb->control.bits.process_id = 0;		/* set frame process id */
	ecb->cipher.bits.cipher_header_len = 0;		/* the header length to be skipped by the cipher */
	ecb->cipher.bits.cipher_algorithm_len = req->nbytes;

	crypto_hw_cipher(ctx->secdev, (unsigned char *)&ecb, sizeof(CRYPTO_CIPHER_ECB_T), req->src, req->nbytes, 0x7B,
		req->dst, req->nbytes);

	return ret;
}

static int gemini_aes_ecb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct gemini_cipher_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	struct CRYPTO_CIPHER_ECB_S *ecb = &ctx->ecb;
	int ret = 0;

	ecb->control.bits.op_mode = CIPHER_DEC;
	ecb->control.bits.cipher_algorithm = ECB_AES;
	ecb->control.bits.process_id = 0;		/* set frame process id */
	ecb->cipher.bits.cipher_header_len = 0;		/* the header length to be skipped by the cipher */
	ecb->cipher.bits.cipher_algorithm_len = req->nbytes;

	crypto_hw_cipher(ctx->secdev, (unsigned char *)&ecb, sizeof(CRYPTO_CIPHER_ECB_T), req->src, req->nbytes, 0x7B,
		req->dst, req->nbytes);

	return ret;
}

static int gemini_aes_cbc_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct gemini_cipher_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	struct CRYPTO_CIPHER_CBC_S *cbc = &ctx->cbc;
	int	ret = 0;

	cbc->control.bits.op_mode = CIPHER_ENC;		/* cipher encryption */
	cbc->control.bits.cipher_algorithm = CBC_AES;	/* AES ECB mode */ 
	cbc->control.bits.process_id = 0;		/* set frame process id */
	cbc->cipher.bits.cipher_header_len = 0;		/* the header length to be skipped by the cipher */
	cbc->cipher.bits.cipher_algorithm_len = req->nbytes;
	gemini_key_swap(cbc->cipher_iv, (void *)req->info, 16);

	crypto_hw_cipher(ctx->secdev, (unsigned char *)&cbc, sizeof(CRYPTO_CIPHER_CBC_T), req->src, req->nbytes, 0x7B,
		req->dst, req->nbytes);

	return ret;
}

static int gemini_aes_cbc_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct gemini_cipher_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	struct CRYPTO_CIPHER_CBC_S *cbc = &ctx->cbc;
	int	ret = 0;

	cbc->control.bits.op_mode = CIPHER_DEC;		/* cipher encryption */
	cbc->control.bits.cipher_algorithm = CBC_AES;	/* AES ECB mode */ 
	cbc->control.bits.process_id = 0;		/* set frame process id */
	cbc->cipher.bits.cipher_header_len = 0;		/* the header length to be skipped by the cipher */
	cbc->cipher.bits.cipher_algorithm_len = req->nbytes;
	gemini_key_swap(cbc->cipher_iv, (void *)req->info, 16);

	crypto_hw_cipher(ctx->secdev, (unsigned char *)&cbc, sizeof(CRYPTO_CIPHER_CBC_T), req->src, req->nbytes, 0x7B,
		req->dst, req->nbytes);

	return ret;
}

static int gemini_ablk_cra_init(struct crypto_tfm *tfm)
{
	const char *name = crypto_tfm_alg_name(tfm);
	const u32 flags = 0; // CRYPTO_ALG_ASYNC | CRYPTO_ALG_NEED_FALLBACK;
	struct gemini_cipher_ctx *ctx = crypto_tfm_ctx(tfm);
	struct crypto_alg *alg = tfm->__crt_alg;
//	struct crypto_skcipher *blk;
	struct gemini_crypto_tmp *algt;


//	blk = crypto_alloc_skcipher(name, 0, flags);

//	if (IS_ERR(blk))
//		return PTR_ERR(blk);

	algt = container_of(alg, struct gemini_crypto_tmp, alg.crypto );
	ctx->secdev = algt->secdev;
//	ctx->fallback = blk;
	tfm->crt_ablkcipher.reqsize = sizeof(struct gemini_cipher_ctx);

	return 0;
}

static void gemini_ablk_cra_exit(struct crypto_tfm *tfm)
{
	struct gemini_cipher_ctx *ctx = crypto_tfm_ctx(tfm);

//	if (ctx->fallback)
//		crypto_free_skcipher(ctx->fallback);

//	ctx->fallback = NULL;
}


struct gemini_crypto_tmp gemini_ecb_aes_alg = {
	.type = ALG_TYPE_CIPHER,
	.alg.crypto = {
		.cra_name		= "ecb(aes)-gemini",
		.cra_driver_name	= "ecb-aes-gemini",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct gemini_cipher_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= gemini_ablk_cra_init,
		.cra_exit		= gemini_ablk_cra_exit,
		.cra_u.ablkcipher	= {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= gemini_aes_setkey,
			.encrypt	= gemini_aes_ecb_encrypt,
			.decrypt	= gemini_aes_ecb_decrypt,
		}
	}
};

struct gemini_crypto_tmp gemini_cbc_aes_alg = {
	.type = ALG_TYPE_CIPHER,
	.alg.crypto = {
		.cra_name		= "cbc(aes)-gemini",
		.cra_driver_name	= "cbc-aes-gemini",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct gemini_cipher_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= gemini_ablk_cra_init,
		.cra_exit		= gemini_ablk_cra_exit,
		.cra_u.ablkcipher	= {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= gemini_aes_setkey,
			.encrypt	= gemini_aes_cbc_encrypt,
			.decrypt	= gemini_aes_cbc_decrypt,
		}
	}
};
/*
struct gemini_crypto_tmp gemini_ecb_des_alg = {
	.type = ALG_TYPE_CIPHER,
	.alg.crypto = {
		.cra_name		= "ecb(des)",
		.cra_driver_name	= "ecb-des-rk",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct gemini_cipher_ctx),
		.cra_alignmask		= 0x07,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= gemini_ablk_cra_init,
		.cra_exit		= gemini_ablk_cra_exit,
		.cra_u.ablkcipher	= {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.setkey		= gemini_tdes_setkey,
			.encrypt	= gemini_des_ecb_encrypt,
			.decrypt	= gemini_des_ecb_decrypt,
		}
	}
};

struct gemini_crypto_tmp gemini_cbc_des_alg = {
	.type = ALG_TYPE_CIPHER,
	.alg.crypto = {
		.cra_name		= "cbc(des)",
		.cra_driver_name	= "cbc-des-rk",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct gemini_cipher_ctx),
		.cra_alignmask		= 0x07,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= gemini_ablk_cra_init,
		.cra_exit		= gemini_ablk_cra_exit,
		.cra_u.ablkcipher	= {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= gemini_tdes_setkey,
			.encrypt	= gemini_des_cbc_encrypt,
			.decrypt	= gemini_des_cbc_decrypt,
		}
	}
};

struct gemini_crypto_tmp gemini_ecb_des3_ede_alg = {
	.type = ALG_TYPE_CIPHER,
	.alg.crypto = {
		.cra_name		= "ecb(des3_ede)",
		.cra_driver_name	= "ecb-des3-ede-rk",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct gemini_cipher_ctx),
		.cra_alignmask		= 0x07,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= gemini_ablk_cra_init,
		.cra_exit		= gemini_ablk_cra_exit,
		.cra_u.ablkcipher	= {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= gemini_tdes_setkey,
			.encrypt	= gemini_des3_ede_ecb_encrypt,
			.decrypt	= gemini_des3_ede_ecb_decrypt,
		}
	}
};

struct gemini_crypto_tmp gemini_cbc_des3_ede_alg = {
	.type = ALG_TYPE_CIPHER,
	.alg.crypto = {
		.cra_name		= "cbc(des3_ede)",
		.cra_driver_name	= "cbc-des3-ede-rk",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct gemini_cipher_ctx),
		.cra_alignmask		= 0x07,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= gemini_ablk_cra_init,
		.cra_exit		= gemini_ablk_cra_exit,
		.cra_u.ablkcipher	= {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= gemini_tdes_setkey,
			.encrypt	= gemini_des3_ede_cbc_encrypt,
			.decrypt	= gemini_des3_ede_cbc_decrypt,
		}
	}
};
*/
