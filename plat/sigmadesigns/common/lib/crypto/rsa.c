/*
 *  The RSA public-key cryptosystem
 *
 *  Copyright (C) 2006-2007  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  RSA was designed by Ron Rivest, Adi Shamir and Len Adleman.
 *
 *  http://theory.lcs.mit.edu/~rivest/rsapaper.pdf
 *  http://www.cacr.math.uwaterloo.ca/hac/about/chap8.pdf
 */

#include <bl_common.h>
#include <string.h>

#include <crypto/config.h>

#if defined(XYSSL_RSA_C)

#include <crypto/rsa.h>
/*
 * Initialize an RSA context
 */
void rsa_init( rsa_context *ctx,
               int padding,
               int hash_id,
               int (*f_rng)(void *),
               void *p_rng,
               t_int* pbuf,
               unsigned int size)
{
    memset( ctx, 0, sizeof( rsa_context ) );

    ctx->padding = padding;
    ctx->hash_id = hash_id;

    ctx->f_rng = f_rng;
    ctx->p_rng = p_rng;

    mpi_globleinit(pbuf, size);
}

/*
 * Do an RSA public key operation
 */
int rsa_public( rsa_context *ctx,
                unsigned char *input,
                unsigned char *output )
{
    int ret, olen;
    mpi T;

    mpi_init(&T);

    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

    if( mpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        mpi_free(&T);
        return( XYSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    olen = ctx->len;
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx->E, &ctx->N, &ctx->RN ) );
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

    mpi_free(&T);

    if( ret != 0 )
        return( XYSSL_ERR_RSA_PUBLIC_FAILED | ret );

    return( 0 );
}

/*
 * Do an RSA operation and check the message digest
 */
int rsa_pkcs1_verify( rsa_context *ctx,
                      int mode,
                      int hash_id,
                      int hashlen,
                      unsigned char *hash,
                      unsigned char *sig )
{
    int ret, len, siglen;
    unsigned char *p;
    unsigned char buf[512];
    unsigned char tempbuf[20];

    siglen = ctx->len;

    if( siglen < 16 || siglen > (int) sizeof( buf ) )
        return( XYSSL_ERR_RSA_BAD_INPUT_DATA );

    //ret = ( mode == RSA_PUBLIC )
    //      ? rsa_public(  ctx, sig, buf )
    //      : rsa_private( ctx, sig, buf );

    ret = rsa_public(  ctx, sig, buf );

	if( ret != 0 )
        return( ret );

    p = buf;

    switch( ctx->padding )
    {
        case RSA_PKCS_V15:

            if( *p++ != 0 || *p++ != RSA_SIGN )
                return( XYSSL_ERR_RSA_INVALID_PADDING );

            while( *p != 0 )
            {
                if( p >= buf + siglen - 1 || *p != 0xFF )
                    return( XYSSL_ERR_RSA_INVALID_PADDING );
                p++;
            }
            p++;
            break;

        default:

            return( XYSSL_ERR_RSA_INVALID_PADDING );
    }

    len = siglen - (int)( p - buf );

    if( len == 35 && hash_id == RSA_SHA1 )
    {
        memcpy(tempbuf, ASN1_HASH_SHA1, sizeof(ASN1_HASH_SHA1));
        if( memcmp( p, tempbuf, 15 ) == 0 &&
            memcmp( p + 15, hash, 20 ) == 0 )
            return( 0 );
        else
            return( XYSSL_ERR_RSA_VERIFY_FAILED );
    }

    if( len == 51 && hash_id == RSA_SHA256 )
    {
        memcpy(tempbuf, ASN1_HASH_SHA256, sizeof(ASN1_HASH_SHA256));
        if( memcmp( p, tempbuf, 19 ) == 0 &&
            memcmp( p + 19, hash, 32 ) == 0 )
            return( 0 );
        else
            return( XYSSL_ERR_RSA_VERIFY_FAILED );
    }

    return( XYSSL_ERR_RSA_INVALID_PADDING );
}

/*
 * Free the components of an RSA key
 */
void rsa_free( rsa_context *ctx )
{
    mpi_free(&ctx->RQ);
    mpi_free(&ctx->RP);
    mpi_free(&ctx->RN);
    mpi_free(&ctx->QP);
    mpi_free(&ctx->DQ);
    mpi_free(&ctx->DP);
    mpi_free(&ctx->Q);
    mpi_free(&ctx->P);
    mpi_free(&ctx->D);
    mpi_free(&ctx->E);
    mpi_free(&ctx->N);
}

#endif



