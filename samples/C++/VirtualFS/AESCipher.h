#pragma once
#include "../Encryption/AesCrypto.h"

namespace XDX
{
class FSCryptoAES: public XDX::Objects::ICrypto
{
public: // Own methods
	FSCryptoAES();
	virtual ~FSCryptoAES();
public: // ICrypto	
	TIC_REF_LOCK_MEMBERS
	virtual VOID WINAPI SetParams(DWORD Mode, DWORD AParam, DWORD BParam) override;
	virtual VOID WINAPI GetParams(DWORD* Mode, DWORD* AParam, DWORD* BParam) override;
	virtual VOID WINAPI SetKeyWithIV(PBYTE KeyBuffer, DWORD KeySize, PBYTE IVBuffer, DWORD IVSize) override;
	virtual DWORD WINAPI EncryptAlloc(PBYTE Buffer, DWORD Length, BOOL ResetIV, PBYTE* OutBuffer, DWORD* OutLength) override;
	virtual DWORD WINAPI DecryptAlloc(PBYTE Buffer, DWORD Length, BOOL ResetIV, PBYTE* OutBuffer, DWORD* OutLength) override;
	virtual DWORD WINAPI Encrypt(PBYTE Buffer, DWORD Length, BOOL ResetIV) override;
	virtual DWORD WINAPI Decrypt(PBYTE Buffer, DWORD Length, BOOL ResetIV) override;
private: // Members
	CAESCrypto* pAES;
    PBYTE       m_IVector;
};
}