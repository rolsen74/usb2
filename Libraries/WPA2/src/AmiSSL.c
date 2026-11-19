
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

// Main interface vector numbers from the AmiSSL 5.16 and 6.1 SDKs.
// Only opaque OpenSSL handles cross these interfaces.
#define AMISSL_V331 35
#define AMISSL_V41x 47
#define AMISSL_GET_INTERFACE ( TAG_USER + 0x0f )
#define AMISSL_USES_OPENSSL_STRUCTS ( TAG_USER + 0x0c )
#define EVP_WRAP_ALLOW 1

typedef struct evp_md_st EVP_MD;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_cipher_st EVP_CIPHER;
typedef struct engine_st ENGINE;

struct AmiSSLMasterIFace
{
	struct InterfaceData Data;
	APTR BeforeClose[6];
	void APICALL (*CloseAmiSSL)( struct AmiSSLMasterIFace *self );
	APTR BeforeOpen[2];
	LONG APICALL (*OpenAmiSSLTagList)( struct AmiSSLMasterIFace *self, LONG version, struct TagItem *tags );
};

struct AmiSSL5IFace
{
	struct InterfaceData Data;
	APTR BeforeDecrypt[717];
	int APICALL (*EVP_DecryptInit_ex)( struct AmiSSL5IFace *, EVP_CIPHER_CTX *, const EVP_CIPHER *,
		ENGINE *, const U8 *, const U8 * );
	int APICALL (*EVP_DecryptUpdate)( struct AmiSSL5IFace *, EVP_CIPHER_CTX *, U8 *, int *, const U8 *, int );
	APTR BeforeDecryptFinal[1];
	int APICALL (*EVP_DecryptFinal_ex)( struct AmiSSL5IFace *, EVP_CIPHER_CTX *, U8 *, int * );
	APTR BeforeSHA1[755 - 720 - 1];
	const EVP_MD * APICALL (*EVP_sha1)( struct AmiSSL5IFace * );
	APTR BeforePBKDF2[860 - 755 - 1];
	int APICALL (*PKCS5_PBKDF2_HMAC_SHA1)( struct AmiSSL5IFace *, const char *, int,
		const U8 *, int, int, int, U8 * );
	APTR BeforeHMAC[873 - 860 - 1];
	U8 * APICALL (*HMAC)( struct AmiSSL5IFace *, const EVP_MD *, const void *, int,
		const U8 *, size_t, U8 *, unsigned int * );
	APTR BeforeRand[1347 - 873 - 1];
	int APICALL (*RAND_bytes)( struct AmiSSL5IFace *, U8 *, int );
	APTR BeforeFlags[2611 - 1347 - 1];
	void APICALL (*EVP_CIPHER_CTX_set_flags)( struct AmiSSL5IFace *, EVP_CIPHER_CTX *, int );
	APTR BeforeNew[3405 - 2611 - 1];
	EVP_CIPHER_CTX * APICALL (*EVP_CIPHER_CTX_new)( struct AmiSSL5IFace * );
	void APICALL (*EVP_CIPHER_CTX_free)( struct AmiSSL5IFace *, EVP_CIPHER_CTX * );
	APTR BeforeWrap[3968 - 3406 - 1];
	const EVP_CIPHER * APICALL (*EVP_aes_128_wrap)( struct AmiSSL5IFace * );
};

struct AmiSSL6IFace
{
	struct InterfaceData Data;
	APTR BeforeDecrypt[490];
	int APICALL (*EVP_DecryptInit_ex)( struct AmiSSL6IFace *, EVP_CIPHER_CTX *, const EVP_CIPHER *,
		ENGINE *, const U8 *, const U8 * );
	int APICALL (*EVP_DecryptUpdate)( struct AmiSSL6IFace *, EVP_CIPHER_CTX *, U8 *, int *, const U8 *, int );
	APTR BeforeDecryptFinal[1];
	int APICALL (*EVP_DecryptFinal_ex)( struct AmiSSL6IFace *, EVP_CIPHER_CTX *, U8 *, int * );
	APTR BeforeSHA1[523 - 493 - 1];
	const EVP_MD * APICALL (*EVP_sha1)( struct AmiSSL6IFace * );
	APTR BeforePBKDF2[609 - 523 - 1];
	int APICALL (*PKCS5_PBKDF2_HMAC_SHA1)( struct AmiSSL6IFace *, const char *, int,
		const U8 *, int, int, int, U8 * );
	APTR BeforeHMAC[615 - 609 - 1];
	U8 * APICALL (*HMAC)( struct AmiSSL6IFace *, const EVP_MD *, const void *, int,
		const U8 *, size_t, U8 *, unsigned int * );
	APTR BeforeRand[995 - 615 - 1];
	int APICALL (*RAND_bytes)( struct AmiSSL6IFace *, U8 *, int );
	APTR BeforeFlags[1857 - 995 - 1];
	void APICALL (*EVP_CIPHER_CTX_set_flags)( struct AmiSSL6IFace *, EVP_CIPHER_CTX *, int );
	APTR BeforeNew[2203 - 1857 - 1];
	EVP_CIPHER_CTX * APICALL (*EVP_CIPHER_CTX_new)( struct AmiSSL6IFace * );
	void APICALL (*EVP_CIPHER_CTX_free)( struct AmiSSL6IFace *, EVP_CIPHER_CTX * );
	APTR BeforeWrap[2653 - 2204 - 1];
	const EVP_CIPHER * APICALL (*EVP_aes_128_wrap)( struct AmiSSL6IFace * );
};

#define SSL_CALL( crypto, function, ... ) \
	( ( crypto )->Version6 ? \
		( (struct AmiSSL6IFace *) ( crypto )->SSL )->function( __VA_ARGS__ ) : \
		( (struct AmiSSL5IFace *) ( crypto )->SSL )->function( __VA_ARGS__ ) )

// --

S32
Crypto_Open( struct Crypto *crypto )
{
	struct TagItem tags[3];

	// Both supported interfaces use opaque OpenSSL handles.
	tags[0].ti_Tag = AMISSL_USES_OPENSSL_STRUCTS;
	tags[0].ti_Data = TRUE;
	tags[1].ti_Tag = AMISSL_GET_INTERFACE;
	tags[1].ti_Data = (Tag) & crypto->SSL;
	tags[2].ti_Tag = TAG_END;
	tags[2].ti_Data = 0;

	crypto->MasterBase = OpenLibrary( "amisslmaster.library", 5 );

	if ( ! crypto->MasterBase )
	{
		return ( FALSE );
	}

	crypto->Master = (struct AmiSSLMasterIFace *) GetInterface( crypto->MasterBase, "main", 1, NULL );

	if ( ! crypto->Master )
	{
		return ( FALSE );
	}

	crypto->Opened = crypto->Master->OpenAmiSSLTagList( AMISSL_V41x, tags ) == 0;

	if ( ( crypto->Opened ) && ( ! crypto->SSL ) )
	{
		crypto->Master->CloseAmiSSL();

		crypto->Opened = FALSE;
	}

	if ( crypto->Opened )
	{
		crypto->Version6 = TRUE;
	}
	else
	{
		crypto->SSL = NULL;

		crypto->Opened = crypto->Master->OpenAmiSSLTagList( AMISSL_V331, tags ) == 0;
	}

	return ( ( crypto->Opened ) && ( crypto->SSL ) );
}

// --

void
Crypto_Close( struct Crypto *crypto )
{
	if ( crypto->Opened )
	{
		crypto->Master->CloseAmiSSL();
	}

	if ( crypto->Master )
	{
		DropInterface( (struct Interface *) crypto->Master );
	}

	if ( crypto->MasterBase )
	{
		CloseLibrary( crypto->MasterBase );
	}
}

// --

S32
Crypto_Random( struct Crypto *crypto, U8 *buffer, U32 size )
{
	return ( SSL_CALL( crypto, RAND_bytes, buffer, size ) == 1 );
}

// --

S32
Crypto_PMK( struct Crypto *crypto, const char *password, U32 length,
	const U8 *ssid, U32 ssid_length, U8 *pmk )
{
	return ( SSL_CALL( crypto, PKCS5_PBKDF2_HMAC_SHA1, password, length,
		ssid, ssid_length, 4096, 32, pmk ) == 1 );
}

// --

S32
Crypto_HMAC( struct Crypto *crypto, const U8 *key, U32 key_length,
	const U8 *data, U32 data_length, U8 *hash )
{
	const EVP_MD *sha1;
	unsigned int length;

	sha1 = SSL_CALL( crypto, EVP_sha1 );

	return ( ( sha1 ) && ( SSL_CALL( crypto, HMAC, sha1, key, key_length,
		data, data_length, hash, & length ) ) && ( length == 20 ) );
}

// --

S32
Crypto_Unwrap( struct Crypto *crypto, const U8 *key, const U8 *input, U32 length, U8 *output )
{
	EVP_CIPHER_CTX *context;
	const EVP_CIPHER *wrap;
	int actual;
	int tail;
	S32 success;

	context = SSL_CALL( crypto, EVP_CIPHER_CTX_new );

	if ( ! context )
	{
		return ( FALSE );
	}

	SSL_CALL( crypto, EVP_CIPHER_CTX_set_flags, context, EVP_WRAP_ALLOW );

	wrap = SSL_CALL( crypto, EVP_aes_128_wrap );

	success = wrap && SSL_CALL( crypto, EVP_DecryptInit_ex, context, wrap, NULL, key, NULL ) &&
		SSL_CALL( crypto, EVP_DecryptUpdate, context, output, & actual, input, length ) &&
		( actual == (int) length - 8 ) &&
		SSL_CALL( crypto, EVP_DecryptFinal_ex, context, output + actual, & tail ) && ( ! tail );

	SSL_CALL( crypto, EVP_CIPHER_CTX_free, context );

	return ( success );
}
