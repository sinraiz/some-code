#pragma once
#include "defines.h"
#include "AESCipher.h"
#include <Hdf5.h>
#include "h5fdblock.h"
#include "vfile.h"

using namespace XDX::Objects;
namespace XDX
{

class VirtualFS: public IFileSystem, public XHdf5::DriverCallback
{
public:
	VirtualFS(LPCWSTR Alias, LPCWSTR DataFolder);
	virtual ~VirtualFS();
	void ToLog(DWORD Event, LPCWSTR Message);
	void Commit(){}
	void Rollback(){}
public: // interface methods
	virtual VOID     WINAPI AddRef() override;
	virtual VOID     WINAPI Release() override;
	virtual VOID     WINAPI GetLastErrorDesc(LPWSTR Buf, DWORD Size) override;
	virtual VOID     WINAPI SetLogger(pILog Logger) override;
	virtual BOOL     WINAPI IsOpen() override;
	virtual DWORD    WINAPI Create(LPCWSTR FileName, LPCWSTR Name, ICrypto* PwdCrypt, ICrypto* DataCrypt, DWORD BlockSize, DWORD InitialBlocks, DWORD Version) override;
	virtual DWORD    WINAPI Open(LPCWSTR FileName, ICrypto* PwdCrypt, ICrypto* DataCrypt, UINT64 MemMapSize) override;
	virtual DWORD    WINAPI Close() override;
	virtual DWORD    WINAPI GetMeta(DWORD FieldId, PBYTE ByteVal, DWORD *MaxSize) override;
	virtual DWORD    WINAPI SetMeta(DWORD FieldId, PBYTE ByteVal, DWORD Size) override;
	virtual DWORD    WINAPI ChangeAccessPassword(PBYTE Password, DWORD Length) override;
	virtual DWORD    WINAPI FileCreate(LPCWSTR FileName, UINT64 CreatedBy, DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, HANDLE* File) override;
	virtual DWORD    WINAPI FileLock(HANDLE File, BOOL Exclusive, UINT64 Offset, UINT64 Length) override;
	virtual DWORD    WINAPI FileUnlock(HANDLE File, UINT64 Offset, UINT64 Length) override;
	virtual DWORD    WINAPI FileRead(HANDLE File, LPVOID Buffer, UINT64 Offset, DWORD LengthToRead, LPDWORD LengthRead) override;
	virtual DWORD    WINAPI FileWrite(HANDLE File, LPVOID Buffer, UINT64 Offset, DWORD LengthToWrite, LPDWORD LengthWritten) override;
	virtual DWORD    WINAPI FileFlush(HANDLE File) override;
	virtual DWORD    WINAPI FileClose(HANDLE File) override;
	virtual DWORD    WINAPI FilesCloseAll(UINT64 CreatedBy) override; 
	virtual DWORD    WINAPI Move(LPCWSTR ExistingName, LPCWSTR NewName) override;
	virtual DWORD    WINAPI Delete(LPCWSTR Name) override;
	virtual DWORD    WINAPI GetAttributes(LPCWSTR Name, pAttrInfo Attr) override;
	virtual DWORD    WINAPI SetAttributes(LPCWSTR Name, pAttrInfo Attr) override;
	virtual DWORD    WINAPI FolderCreate(LPCWSTR DirName, UINT64 CreatedBy) override;
	virtual DWORD    WINAPI FolderList(LPCWSTR DirName, pAttrInfo *Items, DWORD* ItemCount) override;
protected: // HDF5 block driver callback	
	virtual int OnH5WriteUserBlock(void * Buffer, unsigned int Size);
	virtual int OnH5ReadUserBlock(void * Buffer, unsigned int Size);
	virtual int OnH5FillEmptyBlock(void * Buffer, unsigned int Size);
	virtual int OnH5AfterBlockRead(void * Buffer, unsigned int Size);
	virtual int OnH5BeforeBlockWrite(void * Buffer, unsigned int Size);
	virtual void OnH5ToLog(DWORD Event, LPCWSTR Message);
private: //methods
	inline BOOL _IsCrypto(){return (m_PwdCrypt!=nullptr && m_DataCrypt!=nullptr)?TRUE:FALSE;}
	inline void _MayBeEncrypt(void *_Data, DWORD _Size){if(_IsCrypto()) m_DataCrypt->Encrypt((PBYTE)_Data, _Size, TRUE);}
	inline void _MayBeDecrypt(void *_Data, DWORD _Size){if(_IsCrypto()) m_DataCrypt->Decrypt((PBYTE)_Data, _Size, TRUE);}
	static herr_t _WalkErrorCallback(unsigned n, const H5E_error2_t *err_desc, void *udata);
	VOID  _ClearMem();
	inline time_t GetTime();
	inline void* MemAlloc(SIZE_T Bytes);
	inline herr_t CloseH5handle(hid_t id, H5I_type_t type)
	{
		switch(type)
		{
			case H5I_GROUP:
				return H5Gclose(id);	
			case H5I_DATASET:
				return H5Dclose(id);
			case H5I_GENPROP_LST:
				return H5Pclose(id);
			case H5I_DATASPACE:
				return H5Sclose(id);
			case H5I_ATTR:
				return H5Aclose(id);
		}
		return -1;
	}
	DWORD _Init(LPCWSTR FileName, LPCWSTR Name, ICrypto* PwdCrypt, ICrypto* DataCrypt, DWORD BlockSize, DWORD Version, BOOL Create);
	DWORD _DefineRawHeader(DWORD BlockSize, DWORD Version);
	DWORD _AssignAccessPassword();
	DWORD _CreateMetaRecords(LPCWSTR Name, DWORD BlockSize, DWORD Version);
	DWORD _ReadMetaRecords();
	DWORD _WriteMetaDW(DWORD MetaId, DWORD Value);
	DWORD _WriteMetaQW(DWORD MetaId, UINT64 Value);
	DWORD _WriteMetaBytes(DWORD MetaId, PBYTE ByteVal, DWORD Size, size_t MaxLength);
	DWORD _WriteAttributeBytes(hid_t ObjId, DWORD FieldId, PBYTE ByteVal, DWORD Size, size_t MaxLength);
	DWORD _ReadMeta(DWORD MetaId, PBYTE ByteVal, DWORD *MaxSize);
	DWORD _ReadAttributeBytes(hid_t ObjId, DWORD FieldId, PBYTE ByteVal, DWORD MaxSize);
	DWORD _GetAttributesById(hid_t ObjId, pAttrInfo Attr, bool IsGroup);
	BOOL  _IsPathValid(LPCWSTR Path);
	DWORD _FollowPath(LPCWSTR Path, H5I_type_t& ObjectType, hid_t& ObjectId);
	DWORD _PathCreate(LPCWSTR Name, DWORD Attributes, UINT64 CreatedBy);
	DWORD _AcquireRealHandle(LPCWSTR FileName, UINT64 CreatedBy, DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, RealHandle& hFile);
	DWORD _ReleaseRealHandle(DWORD AccessMode, RealHandle hFile);
	DWORD _FileCloseInternal(UserHandlesT::iterator File);
private://members
	// Own stuff
	pILog               m_Logger;
	std::wstring        m_Alias;
	wchar_t             m_DataFolder[MAX_PATH];
	volatile long       m_RefCount;
	DWORD               m_LastErr;
	IRWLock*            m_Lock;
	
	// Specific data for the raw storage
	hid_t               m_hFile;
	hid_t               m_hFapl;
	hid_t               m_hFcpl;
	XHdf5::BlockDriver* m_Driver;

	// Encryption related stuff
	ICrypto*            m_PwdCrypt;
	ICrypto*            m_DataCrypt;
	FSInfo              INFO;
	BYTE                MasterKey[MASTER_KEY_LEN]; 
	BYTE                IV[MASTER_KEY_LEN]; // we don't need so much, so the encryptor will take only the required part of it

	// Stuff related to the virtual files
	uint64_t            m_HandlesCounter;
	NamedHandlesT       m_NamedHandles;
	RealHandlesT        m_RealHandles;
	UserHandlesT        m_UserHandles;
};
}