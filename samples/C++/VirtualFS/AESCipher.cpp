#include "stdafx.h"
#include "AESCipher.h" 

namespace XDX
{
FSCryptoAES::FSCryptoAES()
{
	refCount  = 1;
	pAES      = new CAESCrypto();
	m_IVector = new BYTE[XDX::Objects::enAesParameters::AES_BLOCKSIZE];
}
FSCryptoAES::~FSCryptoAES()
{
	delete pAES;
	delete m_IVector;
}
VOID WINAPI FSCryptoAES::SetParams(DWORD Mode, DWORD AParam, DWORD BParam) 
{
	// not implemented for this AES 
}
VOID WINAPI FSCryptoAES::GetParams(DWORD* Mode, DWORD* AParam, DWORD* BParam) 
{
	// not implemented for this AES 
	*Mode   = 0;
	*AParam = 0;
	*BParam = 0;
}
VOID WINAPI FSCryptoAES::SetKeyWithIV(PBYTE KeyBuffer, DWORD KeySize, PBYTE IVBuffer, DWORD IVSize) 
{
	memcpy_s(m_IVector, XDX::Objects::enAesParameters::AES_BLOCKSIZE, IVBuffer, IVSize);
	pAES->SetKeyWithIV(KeyBuffer, min(KeySize, 16), IVBuffer, IVSize);
}
DWORD WINAPI FSCryptoAES::EncryptAlloc(PBYTE Buffer, DWORD Length, BOOL ResetIV, PBYTE* OutBuffer, DWORD* OutLength) 
{
	if(OutBuffer==nullptr || OutLength==nullptr || Buffer==nullptr)
		return ERR_ERROR_PARAM;
	if((Length%XDX::Objects::enAesParameters::AES_BLOCKSIZE)!=0 || Length==0)
		return ERR_ERROR_PARAM;

	*OutLength = Length;
	*OutBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, Length);
	memcpy(*OutBuffer, Buffer, Length);

	// Encrypt inplace
	return this->Encrypt(*OutBuffer, Length, ResetIV);
}
DWORD WINAPI FSCryptoAES::DecryptAlloc(PBYTE Buffer, DWORD Length, BOOL ResetIV, PBYTE* OutBuffer, DWORD* OutLength) 
{
	if(OutBuffer==nullptr || OutLength==nullptr || Buffer==nullptr)
		return ERR_ERROR_PARAM;
	if((Length%XDX::Objects::enAesParameters::AES_BLOCKSIZE)!=0 || Length==0)
		return ERR_ERROR_PARAM;

	*OutLength = Length;
	*OutBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, Length);
	memcpy(*OutBuffer, Buffer, Length);

	// Decrypt inplace
	return this->Decrypt(*OutBuffer, Length, ResetIV);
}
DWORD WINAPI FSCryptoAES::Encrypt(PBYTE Buffer, DWORD Length, BOOL ResetIV) 
{
	// Must we reset the IV ?
	if(ResetIV)
		pAES->SetIV(m_IVector, XDX::Objects::enAesParameters::AES_BLOCKSIZE);

	// Actually encrypt
	if(!pAES->Encrypt(Buffer, Length))
		return ERR_EXTERNAL;
	else
		return ERR_SUCCESS;
}
DWORD WINAPI FSCryptoAES::Decrypt(PBYTE Buffer, DWORD Length, BOOL ResetIV) 
{
	// Must we reset the IV ?
	if(ResetIV)
		pAES->SetIV(m_IVector, XDX::Objects::enAesParameters::AES_BLOCKSIZE);

	// Actually decrypt
	if(!pAES->Decrypt(Buffer, Length))
		return ERR_EXTERNAL;
	else
		return ERR_SUCCESS;
}

}