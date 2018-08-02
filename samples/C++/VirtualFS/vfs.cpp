#include "stdafx.h"
#include "vfs.h"
#include "md5.h"


namespace XDX
{
VirtualFS::VirtualFS(LPCWSTR Alias, LPCWSTR DataFolder)
{
	_ClearMem();
	ZeroMemory(m_DataFolder, sizeof(m_DataFolder));
	wcscpy_s(m_DataFolder, sizeof(m_DataFolder)/2, DataFolder);
	m_Alias     = Alias;
	m_Logger    = nullptr;
	m_RefCount  = 1;
	m_HandlesCounter = 0;

	// Disable printing errors
	H5Eset_auto (H5E_DEFAULT, nullptr, nullptr);

	
	//Acquire a lock object
	m_Lock = new CReaderWriterLock(); //(IRWLock*)m_srv->ObjectCreate("IRWLock", LOG_NAME);

}
VirtualFS::~VirtualFS()
{
	Close();
	m_Lock->Release();
}
void VirtualFS::ToLog(DWORD Event, LPCWSTR Message)
{
	if(!m_Logger)
		return;
	m_Logger->Log(Event, m_Alias.c_str(), Message);
}
//***********************************************************************************

VOID  WINAPI VirtualFS::AddRef()
{
	InterlockedIncrement(&m_RefCount);
}
VOID  WINAPI VirtualFS::Release()
{
	if(InterlockedDecrement(&m_RefCount) == 0) 
	{ 
		delete this;
	} 
}
VOID  WINAPI VirtualFS::GetLastErrorDesc(LPWSTR Buf, DWORD Size)
{
	std::string sResult;
	std::wstring wsResult;
	//int _err_num = 0;
	char _msg[512];
	_msg[0] = L'\0';
	H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, VirtualFS::_WalkErrorCallback, _msg);
	sResult = _msg;
	//convert codepages
	wsResult.assign(sResult.begin(), sResult.end()); 
	wcscpy_s(Buf, Size, wsResult.c_str());
}
VOID  WINAPI VirtualFS::SetLogger(pILog Logger)
{
	m_Logger = Logger;
}
BOOL  WINAPI VirtualFS::IsOpen()
{
	if(m_hFile<0)
		return FALSE;
	return TRUE;
}
DWORD WINAPI VirtualFS::Create(LPCWSTR FileName, LPCWSTR Name, ICrypto* PwdCrypt, ICrypto* DataCrypt,DWORD BlockSize, DWORD InitialBlocks, DWORD Version)
{
	DWORD hRes = ERR_SUCCESS;
	if(FileName==nullptr || Name==nullptr || BlockSize<4000)
		return ERR_ERROR_PARAM;
	if(wcslen(Name)<3)
	{
		ToLog(Logs::EV_ERROR, L"File system name is too short");
		return ERR_ERROR_PARAM;
	}
	if(wcslen(FileName)<3)
	{
		ToLog(Logs::EV_ERROR, L"File system image filename is too short");
		return ERR_ERROR_PARAM;
	}
		
	CAutoWriteLock l(m_Lock);

	// Close the FS first, if it was open
	CheckXErr(Close());

	// Make sure that the file doesn't exist
	wchar_t DataFilePath[MAX_PATH];
	swprintf_s(DataFilePath, sizeof(DataFilePath)/2, L"%s\\%s.dat", m_DataFolder, FileName);

	DWORD attributes = GetFileAttributes(DataFilePath);
	bool FileExists = !(attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY));
	if(FileExists)
	{
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to create the file system \"%s\": index file already exists", FileName);
		ToLog(Logs::EV_ERROR, Msg);
		//Close();
		return ERR_DUPLICATE;
	}

	// Now call the initialization
	if((hRes = _Init(FileName, Name, PwdCrypt, DataCrypt, BlockSize, Version, TRUE))!=ERR_SUCCESS)
	{
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to create the file system \"%s\": %s", FileName, ErrorTexts::GetErrorDesc(hRes));
		ToLog(Logs::EV_ERROR, Msg);
		Close();
		return hRes;
	}
	return hRes;
}
DWORD WINAPI VirtualFS::Open(LPCWSTR FileName, ICrypto* PwdCrypt, ICrypto* DataCrypt, UINT64 MemMapSize)
{
	DWORD hRes = ERR_SUCCESS;
	if(FileName==nullptr)
		return ERR_ERROR_PARAM;

	CAutoWriteLock l(m_Lock);

	// Close the FS first, if it was open
	CheckXErr(Close());

	if((hRes = _Init(FileName, nullptr, PwdCrypt, DataCrypt, 0, FS_VERSION_ID, FALSE))!=ERR_SUCCESS)
	{
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to open the file system \"%s\": %s", FileName, ErrorTexts::GetErrorDesc(hRes));
		ToLog(Logs::EV_ERROR, Msg);
		Close();
		return hRes;
	}
	return hRes;
}
DWORD WINAPI VirtualFS::Close()
{
	DWORD hError = ERROR_SUCCESS;
	CAutoWriteLock l(m_Lock);


	if( m_hFile>0 && H5Fflush(m_hFile, H5F_SCOPE_GLOBAL)<0 || 
		m_hFile>0 && H5Fclose(m_hFile)<0 || 
		m_hFapl!=H5P_DEFAULT && H5Pclose(m_hFapl)<0|| 
		m_hFcpl!=H5P_DEFAULT && H5Pclose(m_hFcpl)<0)		
	{
		wchar_t H5Msg[512] = {0};
		GetLastErrorDesc(H5Msg, sizeof(H5Msg)/2-1);
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to close the file system: %ls", H5Msg);
		ToLog(Logs::EV_ERROR, Msg);
		hError = ERR_DISK_WRITE;
	}
	if(m_Driver)
		delete m_Driver;

	// Release crypto providers
	if(m_PwdCrypt!=nullptr && m_DataCrypt!=nullptr)
	{
		m_PwdCrypt->Release();
		m_DataCrypt->Release();
	}

	// Clear the fields
	_ClearMem();
	return hError;
}
DWORD WINAPI VirtualFS::GetMeta(DWORD FieldId, PBYTE ByteVal, DWORD *MaxSize)
{
	if(ByteVal==nullptr || MaxSize==nullptr)
		return ERR_ERROR_PARAM;
	if(*MaxSize==0)
		return ERR_ERROR_PARAM;

	CAutoReadLock l(m_Lock);
	if(!IsOpen())
		return ERR_NOT_READY; // the fs is not open
	switch(FieldId)
	{
		case FSMF_DW_VERSION: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.FS_VERSION;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_SZ_NAME: 
			if(*MaxSize<3)
				return ERR_LIMITS;
			wcscpy_s((wchar_t*)ByteVal, *MaxSize/2, INFO.NAME);
			*MaxSize=wcslen(INFO.NAME)*2;
			break;            
		case FSMF_DW_BLOCK_SIZE: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.BLOCK_SIZE;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_FILES_COUNT: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.FILES_COUNT;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_DIR_COUNT : 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.DIR_COUNT;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_USED_BLOCKS: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.USED_BLOCKS;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_FREE_BLOCKS: 
			{
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			hsize_t FreeSpace = H5Fget_freespace(m_hFile);

			*(DWORD*)ByteVal = FreeSpace;
			*MaxSize=sizeof(DWORD);
			}
			break;
		case FSMF_QW_USED_BYTES: 
			{
			if(*MaxSize<sizeof(UINT64))
				return ERR_LIMITS;
			hsize_t UsedBytes = 0;
			if(H5Fget_filesize(m_hFile, &UsedBytes)>=0)
			{
				*(UINT64*)ByteVal = UsedBytes;
				*MaxSize=sizeof(UINT64);
			}
			}
			break;
		case FSMF_DW_IS_ENCRYPTED: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.IS_ENCRYPTED;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_XX_MKEY: 
		case FSMF_XX_MKEY_HASH: 
		case FSMF_XX_IV: 
			return ERR_SECURITY;
		case FSMF_DW_KEY_ENC_MODE: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.KEY_ENC_MODE;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_KEY_ENC_APARAM: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.KEY_ENC_APARAM;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_KEY_ENC_BPARAM: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.KEY_ENC_BPARAM;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_DAT_ENC_MODE: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.DAT_ENC_MODE;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_DAT_ENC_APARAM: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.DAT_ENC_APARAM;
			*MaxSize=sizeof(DWORD);
			break;
		case FSMF_DW_DAT_ENC_BPARAM: 
			if(*MaxSize<sizeof(DWORD))
				return ERR_LIMITS;
			*(DWORD*)ByteVal = INFO.DAT_ENC_BPARAM;
			*MaxSize=sizeof(DWORD);
			break;
		default:   
			return ERR_ERROR_PARAM;
	};
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::SetMeta(DWORD FieldId, PBYTE ByteVal, DWORD Size)
{
	DWORD hRes = 0;
	if(FieldId!=FSMF_SZ_NAME)
		return ERR_ACCESS_DENIED;
	if(ByteVal==nullptr || Size<3)
		return ERR_ERROR_PARAM;

	CAutoWriteLock l(m_Lock);
	if(!IsOpen())
		return ERR_NOT_READY; // the fs is not open
	DWORD NameSize = max(Size, FS_MAX_ALIAS);
	CheckXErr(_WriteMetaBytes(FSMF_SZ_NAME, ByteVal, NameSize, FS_MAX_ALIAS));    // The alias of the file system
	CheckXErr(_ReadMeta(FSMF_SZ_NAME, (PBYTE)INFO.NAME, &NameSize));      
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::ChangeAccessPassword(PBYTE Password, DWORD Length)
{	
	DWORD hRes = ERR_SUCCESS;
	if(!IsOpen())
		return ERR_NOT_READY; // the fs is not open
	if(!_IsCrypto())
		return ERR_EMPTY;

	// 1. As password here is passed as plain text, we must hash it first	
	MD5 hasher;
	hasher.update(Password, Length);
	hasher.finalize();	

	// 2. Assign the given new access password to access crypto
	BYTE iv[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // dummy iv for the master key encryptor
	m_PwdCrypt->SetKeyWithIV(hasher.digest, sizeof(hasher.digest), iv, sizeof(iv)); 

	// Update the MasterKey and iv encrypted with a new password
	if((hRes = _AssignAccessPassword())!=ERR_SUCCESS)
		return hRes;
	
	// Trigger the driver to write the user block
	if(m_Driver->UpdateUserBlock()<0)
		return ERR_EXTERNAL;
	return hRes;
}
DWORD WINAPI VirtualFS::FileCreate(LPCWSTR FileName, UINT64 CreatedBy, DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, HANDLE* File)
{
	DWORD hRes = ERR_SUCCESS;
	if(FileName == nullptr || File == nullptr)
		return ERR_ERROR_PARAM;
	if(!_IsPathValid(FileName))
		return ERR_ERROR_PARAM;
	*File = INVALID_HANDLE_VALUE;

	CAutoWriteLock l(m_Lock);
	if(!IsOpen())
		return ERR_NOT_READY; // the fs is not open
	
	// 1. Acquire the real file handle
	RealHandle hFile;
	if((hRes=_AcquireRealHandle(FileName, CreatedBy, DesiredAccess, ShareMode, CreationDisposition, hFile))!=ERR_SUCCESS)
		return hRes;
	
	// 2. Allocate user handle
	*File = (void*)InterlockedIncrement(&m_HandlesCounter); // For 32 bits we will lose the precision
	UserHandle hUser;
	hUser.hRealHandle = hFile.rawHandle;
	hUser.oCursor     = (DesiredAccess & FILE_APPEND_DATA)?0:0;//TODO: place cursor at the end of file
	hUser.uCreatedBy  = CreatedBy;
	hUser.fAccessMode = DesiredAccess;

	// 3. Insert it to the user handles store
	m_UserHandles[*File] = hUser;

	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::FileLock(HANDLE File, BOOL Exclusive, UINT64 Offset, UINT64 Length)
{
	CAutoWriteLock l(m_Lock);
	if(!IsOpen())
		return ERR_NOT_READY; // the fs is not open
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::FileUnlock(HANDLE File, UINT64 Offset, UINT64 Length)
{
	CAutoWriteLock l(m_Lock);
	if(!IsOpen()) return ERR_NOT_READY; // the fs is not open
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::FileRead(HANDLE File, LPVOID Buffer, UINT64 Offset, DWORD LengthToRead, LPDWORD LengthRead)
{
	CAutoReadLock l(m_Lock);
	if(!IsOpen()) return ERR_NOT_READY; // the fs is not open
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::FileWrite(HANDLE File, LPVOID Buffer, UINT64 Offset, DWORD LengthToWrite, LPDWORD LengthWritten)
{
	if(!IsOpen()) return ERR_NOT_READY; // the fs is not open
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::FileFlush(HANDLE File)
{
	if(!IsOpen()) return ERR_NOT_READY; // the fs is not open
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::FileClose(HANDLE File)
{
	DWORD hRes = ERR_SUCCESS;
	CAutoWriteLock l(m_Lock);
	if(!IsOpen()) 
		return ERR_NOT_READY; // the fs is not open

	// 1. Find the handle among the users handles
	auto iiUserHandle = m_UserHandles.find(File);
	if(iiUserHandle == m_UserHandles.end())
		return ERR_ERROR_PARAM;

	return _FileCloseInternal(iiUserHandle);
}
DWORD WINAPI VirtualFS::FilesCloseAll(UINT64 CreatedBy)
{
	DWORD hRes = ERR_SUCCESS;
	CAutoWriteLock l(m_Lock);
	if(!IsOpen()) 
		return ERR_NOT_READY; // the fs is not open

	// 1. Iterate along the users handles
	auto iiUserHandle = m_UserHandles.begin();
	auto ieUserHandle = m_UserHandles.end();
	while(iiUserHandle != ieUserHandle)
	{
		bool mustDelete = (iiUserHandle->second.uCreatedBy==CreatedBy || CreatedBy==0);
		if(mustDelete)
		{
			if((hRes=_FileCloseInternal(iiUserHandle++))!=ERR_SUCCESS)
				return hRes;
		}
		else
			iiUserHandle++;
	}
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::Move(LPCWSTR ExistingName, LPCWSTR NewName)
{
	DWORD hRes = ERR_SUCCESS;
	
	if(!IsOpen()) 
		return ERR_NOT_READY; // the fs is not open	

	// Check if the given paths are valid
	if(ExistingName == nullptr || NewName == nullptr)
		return ERR_ERROR_PARAM;		
	// Check the names
	if(!_IsPathValid(ExistingName) || !_IsPathValid(NewName))
		return ERR_ERROR_PARAM;

	CAutoWriteLock l(m_Lock);


	// 1. Make sure that the old name exists
	hid_t old_item_id = -1;
	H5I_type_t old_item_type = H5I_UNINIT;
	hRes=_FollowPath(ExistingName, old_item_type, old_item_id);
	CloseH5handle(old_item_id, old_item_type);
	if(hRes!=ERR_SUCCESS)
		return ERR_NOT_FOUND;

	// 2. Make sure that the new name is not already taken	
	hid_t new_item_id = -1;
	H5I_type_t new_item_type = H5I_UNINIT;
	hRes=_FollowPath(NewName, new_item_type, new_item_id);
	CloseH5handle(new_item_id, new_item_type);
	if(hRes==ERR_SUCCESS)
		return ERR_DUPLICATE;

	// 3. Make sure that new parent exists and get its id
	DWORD dwNewParentId = 0;	
	wchar_t* wsNewItemName = (wchar_t*)wcsrchr(NewName, L'/');
	if(wsNewItemName==nullptr)
		return ERR_ERROR_PARAM;
	wchar_t PathCopy[MAX_PATH];
	int CopyLength = wcslen(NewName);
	if(wsNewItemName != NewName)//not the first level
		CopyLength = wsNewItemName - NewName;
	else
		CopyLength = 1;
	wcsncpy_s(PathCopy, sizeof(PathCopy)/2, NewName, CopyLength);	

	hid_t parent_item_id = -1;
	H5I_type_t parent_item_type = H5I_UNINIT;
	hRes=_FollowPath(PathCopy, parent_item_type, parent_item_id);
	CloseH5handle(parent_item_id, parent_item_type);
	if(hRes!=ERR_SUCCESS)
		return ERR_NOT_FOUND;

	// 3. Convert the names to UTF8
	std::string oldNameUtf8 = TICUtils::WStringToUtf8(ExistingName);
	std::string newNameUtf8 = TICUtils::WStringToUtf8(NewName);

	// Check that if it's a group
	if(old_item_type == H5I_GROUP) // It's a group
	{
		if(H5Gmove(m_hFile, oldNameUtf8.c_str(), newNameUtf8.c_str())<0)
			return ERR_DISK_WRITE;
	}
	else
	if(old_item_type == H5I_DATASET) // It's a file
	{
	}
	else
	{
		return ERR_ERROR_PARAM;
	}
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::Delete(LPCWSTR Name)
{
	DWORD hRes = ERR_SUCCESS;
	if(!IsOpen()) return ERR_NOT_READY; // the fs is not open
	
	// Check if the given paths are valid
	if(Name == nullptr)
		return ERR_ERROR_PARAM;		
	// Check the names
	if(!_IsPathValid(Name))
		return ERR_ERROR_PARAM;

	CAutoWriteLock l(m_Lock);


	// 1. Make sure that the name exists and identify if it's a file
	hid_t old_item_id = -1;
	H5I_type_t old_item_type = H5I_UNINIT;
	hRes=_FollowPath(Name, old_item_type, old_item_id);
	CloseH5handle(old_item_id, old_item_type);
	if(hRes!=ERR_SUCCESS)
		return ERR_NOT_FOUND;

	// 2. Convert the name to UTF8
	std::string NameUtf8 = TICUtils::WStringToUtf8(Name);
	
	// 3. Actually delete
	if(H5Ldelete(m_hFile, NameUtf8.c_str(), H5P_DEFAULT)<0)
		return ERR_IN_USE;

	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::GetAttributes(LPCWSTR Name, pAttrInfo Attr)
{
	DWORD hRes = ERR_SUCCESS;
	std::string Utf8Name;
	if(Attr == nullptr || Name == nullptr)
		return ERR_ERROR_PARAM;

	wchar_t* wsItemName = (wchar_t*)wcsrchr(Name, L'/');
	if(wsItemName==nullptr)
		return ERR_ERROR_PARAM;
	wsItemName++;

	ZeroMemory(Attr, sizeof(AttrInfo));
	Attr->FileSize = 0;

	CAutoReadLock l(m_Lock);
	if(!IsOpen()) 
		return ERR_NOT_READY; // the fs is not open

	// Check the given path
	if(!_IsPathValid(Name))
		return ERR_ERROR_PARAM;
	
	// Check that the given name exists
	hid_t item_id = -1;
	H5I_type_t item_type = H5I_UNINIT;
	if((hRes=_FollowPath(Name, item_type, item_id))!=ERR_SUCCESS)
		goto L_DONE;

	if(item_type != H5I_GROUP && item_type != H5I_DATASET)
	{
		hRes = ERR_NOT_FOUND;
		goto L_DONE;
	}
	// If we are here then either a file or folder with such a name exists

	// Prepare the result
	if( (hRes=_GetAttributesById(item_id, Attr, (item_type == H5I_GROUP)))!=ERR_SUCCESS)
	{
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to read the iNODE attributes: %ls", ErrorTexts::GetErrorDesc(hRes));
		ToLog(EV_ERROR, Msg);
		goto L_DONE;
	}
	wcscpy_s(Attr->FileName, sizeof(Attr->FileName)/2, wsItemName);


L_DONE:
	CloseH5handle(item_id, item_type);

	return hRes;
}
DWORD VirtualFS::_GetAttributesById(hid_t ObjId, pAttrInfo Attr, bool IsGroup)
{
	DWORD hRes = ERR_SUCCESS;
	std::string Utf8Name;
	if(Attr == nullptr)
		return ERR_ERROR_PARAM;

	ZeroMemory(Attr, sizeof(AttrInfo));
	Attr->FileSize = 0;

	if(!IsGroup)
	{
		//TODO: Get the size
		Attr->FileSize = H5Dget_storage_size(ObjId); /// !!! Size rounded to chunks
	}
	// If we are here then either a file or folder with such a name exists

	// Prepare the result
	FileAttributesDos attr;
	
	if( (hRes=_ReadAttributeBytes(ObjId, FILE_ATTRIBUTES_DOS, (PBYTE)&attr, sizeof(attr)))!=ERR_SUCCESS)
		return hRes;
	Attr->FileAttributes = attr.DW_FILE_ATTRIBUTES;
	Attr->CreatedBy = attr.QW_CREATED_BY;
	Attr->CreationTime = attr.QW_CREATION_TIME;
	Attr->LastAccessTime = attr.QW_ACCESS_TIME;
	Attr->LastWriteTime = attr.QW_WRITE_TIME;
	/*
	if( (hRes=_ReadAttributeBytes(ObjId, DW_FILE_ATTRIBUTES, (PBYTE)&Attr->FileAttributes, sizeof(Attr->FileAttributes)))!=ERR_SUCCESS ||
		(hRes=_ReadAttributeBytes(ObjId, QW_CREATED_BY, (PBYTE)&Attr->CreatedBy, sizeof(Attr->CreatedBy)))!=ERR_SUCCESS ||
		(hRes=_ReadAttributeBytes(ObjId, QW_CREATION_TIME, (PBYTE)&Attr->CreationTime, sizeof(Attr->CreationTime)))!=ERR_SUCCESS ||
		(hRes=_ReadAttributeBytes(ObjId, QW_ACCESS_TIME, (PBYTE)&Attr->LastAccessTime, sizeof(Attr->LastAccessTime)))!=ERR_SUCCESS ||
		(hRes=_ReadAttributeBytes(ObjId, QW_WRITE_TIME, (PBYTE)&Attr->LastWriteTime, sizeof(Attr->LastWriteTime)))!=ERR_SUCCESS)
	{
		return hRes;
	}*/


	return hRes;
}
DWORD WINAPI VirtualFS::SetAttributes(LPCWSTR Name, pAttrInfo Attr)
{
	if(!IsOpen()) return ERR_NOT_READY; // the fs is not open
	return ERR_SUCCESS;
}
DWORD WINAPI VirtualFS::FolderCreate(LPCWSTR DirName, UINT64 CreatedBy)
{
	DWORD hRes = ERR_SUCCESS;
	std::string Utf8Name;
	if(DirName == nullptr)
		return ERR_ERROR_PARAM;
	CAutoWriteLock l(m_Lock);
	if(!IsOpen()) 
		return ERR_NOT_READY; // the fs is not open

	hRes = _PathCreate(DirName, FILE_ATTRIBUTE_DIRECTORY, CreatedBy);
	
	// Check the given path
	if(hRes != ERR_SUCCESS)
	{
		wchar_t H5Msg[512] = {0};
		GetLastErrorDesc(H5Msg, sizeof(H5Msg)/2-1);
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to create a folder: %ls", H5Msg);
		ToLog(EV_ERROR, Msg);
		return hRes;
	}

	return hRes;
}
DWORD WINAPI VirtualFS::FolderList(LPCWSTR DirName, pAttrInfo *Items, DWORD* ItemCount)
{
	class GroupIterator
	{
	private:
		VirtualFS* parent;
		std::vector<AttrInfo> *results;
		std::wstring group_name;
	public:
		GroupIterator(VirtualFS* _parent, std::vector<AttrInfo> *_results, std::wstring _group_name):parent(_parent),results(_results),group_name(_group_name)
		{
			if(group_name.length()>0 && group_name[group_name.length()-1]!=L'/')
				group_name += L"/";
		}	
		static herr_t file_info(hid_t loc_id, const char *name, void *opdata)
		{
			if(opdata == nullptr)
				return -1;
			GroupIterator *me = (GroupIterator *)opdata;
			
			// Convert key and value to an AttrInfo record
			AttrInfo info;

			// Convert the name to Unicode
			std::wstring Utf16Name = me->group_name + TICUtils::Utf8ToWString(name);

			if(me->parent->GetAttributes(Utf16Name.c_str(), &info)!=ERR_SUCCESS)
				return -1;
			me->results->push_back(info);
			return 0;
		};
	};
	DWORD hRes = ERR_SUCCESS;
	if(DirName==nullptr || Items==nullptr || ItemCount==nullptr)
		return ERR_ERROR_PARAM;

	// Just in case prepare for the empty results
	*ItemCount = 0;
	*Items     = nullptr;

	CAutoReadLock l(m_Lock);
	
	if(!IsOpen()) 
		return ERR_NOT_READY; // the fs is not open

	// Check that the given name exists
	hid_t item_id = -1;
	H5I_type_t item_type = H5I_UNINIT;
	hRes=_FollowPath(DirName, item_type, item_id);
	CloseH5handle(item_id, item_type);
	if(hRes!=ERR_SUCCESS)
		return hRes;

	// Check that it's a group
	if(item_type != H5I_GROUP)
		return ERR_ERROR_PARAM;

	// Convert the name to utf8
	std::string Utf8Name = TICUtils::WStringToUtf8(DirName);
	
	// Temporary vector for storing the results
	std::vector<AttrInfo> Results;

	// Iterate 
	GroupIterator iterator(this, &Results, DirName);
	if(H5Giterate(m_hFile, Utf8Name.c_str(), NULL, GroupIterator::file_info, &iterator)<0)
	{
		hRes = ERR_EMPTY;
		return hRes;
	}

	// Check that there are any items
	if(Results.size()>0)
	{
		// Copy the temp collection to the result buffer
		// 1. How much memory is needed?
		DWORD BufferSize = Results.size() * sizeof(AttrInfo);
		// 2. Allocate that amount
		*Items = (pAttrInfo)MemAlloc(BufferSize);
		if(*Items==nullptr)
			return ERR_MEMORY;
		// 3. How many items is there?
		*ItemCount = Results.size();
		// 4. Copy the values
		memcpy(*Items, Results.data(), BufferSize);
	}
	else
		hRes = ERR_EMPTY;
	return hRes;
}

//***********************************************************************************
int VirtualFS::OnH5WriteUserBlock(void * Buffer, unsigned int Size)
{
	// This method is called when the userblock is to be written to the H5 file
	// Here we assume that the INFO struct has the initialized fields, so we read
	// them and copy AS IS to the H5 driver buffer

	// Make sure that we have enough space
	if(Size<sizeof(RawFileHeader))
		return -1;

	// Fill the block with random data
	std::srand(static_cast<int>(time(NULL)*(DWORD)Buffer));
	std::generate((char*)Buffer, (char*)Buffer + Size, std::rand);

	// Prepare the data
	RawFileHeader fh;
	// Fill the struct with dummydata
	ZeroMemory(&fh, sizeof(RawFileHeader));
	//std::generate((char*)&fh, (char*)&fh + sizeof(RawFileHeader), std::rand);

	// Copy the the INFO struct fields to raw header
	fh.Version          = FS_VERSION_ID;
	fh.Version          = INFO.FS_VERSION; 
	fh.BlockSize        = INFO.BLOCK_SIZE; 
	fh.IsEncrypted      = INFO.IS_ENCRYPTED; 
	fh.KeyEncodeMode    = INFO.KEY_ENC_MODE; 
	fh.KeyEncodeAParam  = INFO.KEY_ENC_APARAM; 
	fh.KeyEncodeBParam  = INFO.KEY_ENC_BPARAM; 
	fh.DataEncodeMode   = INFO.DAT_ENC_MODE; 
	fh.DataEncodeAParam = INFO.DAT_ENC_APARAM; 
	fh.DataEncodeBParam = INFO.DAT_ENC_BPARAM; 
	strcpy(fh.Signature,  XDX_SIGNATURE);
	memcpy(fh.MKey,       INFO.MKEY,      MASTER_KEY_LEN);
	memcpy(fh.MKeyHash,   INFO.MKEY_HASH, 16);       
	memcpy(fh.IV,         INFO.IV,        MASTER_KEY_LEN); 

	// Copy it to the driver buffer
	memcpy(Buffer, &fh, sizeof(RawFileHeader));

	return 0;
}
int VirtualFS::OnH5ReadUserBlock(void * Buffer, unsigned int Size)
{	
	// Make sure that we have enough data to parse
	if(Size<sizeof(RawFileHeader))
		return -1;	
	// Read the data
	RawFileHeader fh;
	// Copy it from the buffer
	memcpy(&fh, Buffer, sizeof(RawFileHeader));
	// Check the version
	if(fh.Version > FS_VERSION_ID)
		return -1;
	// Check the signature
	if(strcmp(fh.Signature, XDX_SIGNATURE)!=0)
		return -1;
	// Check the rest of the data

	// Copy the raw header fields to the INFO struct
	INFO.FS_VERSION      = fh.Version;          
	INFO.BLOCK_SIZE      = fh.BlockSize; 
	INFO.IS_ENCRYPTED    = fh.IsEncrypted;      
	INFO.KEY_ENC_MODE    = fh.KeyEncodeMode;    
	INFO.KEY_ENC_APARAM  = fh.KeyEncodeAParam;  
	INFO.KEY_ENC_BPARAM  = fh.KeyEncodeBParam;  
	INFO.DAT_ENC_MODE    = fh.DataEncodeMode;    
	INFO.DAT_ENC_APARAM  = fh.DataEncodeAParam;  
	INFO.DAT_ENC_BPARAM  = fh.DataEncodeBParam; 
	memcpy(INFO.MKEY,      fh.MKey,     MASTER_KEY_LEN);
	memcpy(INFO.MKEY_HASH, fh.MKeyHash, 16);       
	memcpy(INFO.IV,        fh.IV,       MASTER_KEY_LEN); 
	m_Driver->SetBlockSize(INFO.BLOCK_SIZE);

	// Check the encryptor is set up if it's required
	if(!_IsCrypto() && INFO.IS_ENCRYPTED)
	{
		ToLog(EV_ERROR, L"No crypto factory provided");
		if((m_LastErr=Close())!=ERR_SUCCESS) 
			return -1;
		m_LastErr = ERR_ACCESS_DENIED;
		return -1;
	}
	if(_IsCrypto())
	{
		// Set the parameters of the encryptors
		m_PwdCrypt->SetParams(INFO.KEY_ENC_MODE, INFO.KEY_ENC_APARAM, INFO.KEY_ENC_BPARAM);
		m_DataCrypt->SetParams(INFO.DAT_ENC_MODE, INFO.DAT_ENC_APARAM, INFO.DAT_ENC_BPARAM);

		// Check that the key encryptor has a correct password
	
		// 1. Use the access cryptor to decrypt the master key
		PBYTE MKeyDec       = nullptr;
		DWORD MKeyDecLength = 0;
		if(m_PwdCrypt->DecryptAlloc(INFO.MKEY, sizeof(INFO.MKEY), TRUE, &MKeyDec, &MKeyDecLength)!=ERR_SUCCESS)
		{
			ToLog(EV_ERROR, L"Failed to decrypt the meta key");
			if((m_LastErr=Close())!=ERR_SUCCESS) 
				return -1;
			m_LastErr = ERR_EXTERNAL;
			return -1;
		}
		XDXAutoMem AutoMKey(MKeyDec); // Auto delete using heapfree
		// 2. Calculate hash of the master key just decrypted
		MD5 hasher;
		hasher.update((PBYTE)MKeyDec, MKeyDecLength);
		hasher.finalize();

		// 3. The resulting hash must match the master key's stored hash
		if(memcmp(hasher.digest, INFO.MKEY_HASH, 16)!=0)
		{
			ToLog(EV_ERROR, L"File System: Wrong password. Access denied");
			if((m_LastErr=Close())!=ERR_SUCCESS) 
				return -1;
			m_LastErr = ERR_ACCESS_DENIED;
			return -1;
		}
	
		// 4. Use the access cryptor to decrypt the IV
		PBYTE IVDec = nullptr;
		DWORD IVDecLength = 0;
		if(m_PwdCrypt->DecryptAlloc(INFO.IV, sizeof(INFO.IV), TRUE, &IVDec, &IVDecLength)!=ERR_SUCCESS)
		{
			ToLog(EV_ERROR, L"Failed to encrypt the iv");	
			if((m_LastErr=Close())!=ERR_SUCCESS) 
				return -1;
			m_LastErr = ERR_EXTERNAL;
			return -1;
		}
		XDXAutoMem AutoIV(IVDec); // Auto delete using heapfree
	
		// 5. Pass the decrypted master password and the decrypted IV to the data crypto object
		m_DataCrypt->SetKeyWithIV((PBYTE)MKeyDec, MKeyDecLength, IVDec, DATA_IV_LEN);

		// 6. Save the decrypted data key and IV just for the case when we want to change the access password
		memcpy(MasterKey, MKeyDec, MKeyDecLength);
		memcpy(IV, IVDec, IVDecLength);
	}
	return 0;
}
int VirtualFS::OnH5FillEmptyBlock(void * Buffer, unsigned int Size)
{
	std::srand(static_cast<int>(time(NULL)*(DWORD)Buffer));
	std::generate((char*)Buffer, (char*)Buffer + Size, std::rand);
	return 0;
}
int VirtualFS::OnH5AfterBlockRead(void * Buffer, unsigned int Size)
{
	_MayBeDecrypt(Buffer, Size);
	return 0;
}
int VirtualFS::OnH5BeforeBlockWrite(void * Buffer, unsigned int Size)
{
	_MayBeEncrypt(Buffer, Size);
	return 0;
}
void VirtualFS::OnH5ToLog(DWORD Event, LPCWSTR Message)
{
	ToLog(Event, Message);
}

//***********************************************************************************
herr_t VirtualFS::_WalkErrorCallback(unsigned n, const H5E_error2_t *err_desc, void *udata)
{
	char* MsgBuffer = (char*)udata;
	size_t BufferSize = 512;	
    // Get descriptions for the major and minor error numbers */
    char *maj_str = H5Eget_major (err_desc->maj_num);
    char *min_str = H5Eget_minor (err_desc->min_num);

	char ShortMsg[128];
	_snprintf_s(ShortMsg, sizeof(ShortMsg), sizeof(ShortMsg)-1, "%s (%s, %s); ", err_desc->desc, maj_str, min_str);

	strcat_s(MsgBuffer, BufferSize, ShortMsg);
    free(maj_str);
    free(min_str);
    return 0;
}
VOID  VirtualFS::_ClearMem()
{
	m_LastErr   = 0;
	m_hFile     = -1;
	m_hFapl     = H5P_DEFAULT;
	m_hFcpl     = H5P_DEFAULT;
	m_Driver    = nullptr;
	m_PwdCrypt  = nullptr;
	m_DataCrypt = nullptr;
	ZeroMemory(MasterKey, sizeof(MasterKey));
	ZeroMemory(IV, sizeof(IV));
	//ZeroMemory(m_DataFolder, sizeof(m_DataFolder));
	ZeroMemory(&INFO, sizeof(INFO));

	m_HandlesCounter = 0;
}
time_t VirtualFS::GetTime()
{
	return time(NULL);
}
LPVOID VirtualFS::MemAlloc(SIZE_T Bytes)
{
	return HeapAlloc(GetProcessHeap(), 0, Bytes);
} 
DWORD VirtualFS::_Init(LPCWSTR FileName, LPCWSTR Name, ICrypto* PwdCrypt, ICrypto* DataCrypt, DWORD BlockSize, DWORD Version, BOOL Create)
{
	DWORD hRes = ERR_SUCCESS;
	if(IsOpen())
		return ERR_ACCESS_DENIED;

	// Init the H5 block driver
	m_Driver = new XHdf5::BlockDriver(BlockSize, 0, this);

	// Either both encryptors must be provided or neither
	if((PwdCrypt==nullptr)!=(DataCrypt==nullptr))
		return ERR_ERROR_PARAM;	
	m_PwdCrypt  = PwdCrypt;
	m_DataCrypt = DataCrypt;
	
	// Retain crypto providers
	if(m_PwdCrypt!=nullptr && m_DataCrypt!=nullptr)
	{
		m_PwdCrypt->AddRef();
		m_DataCrypt->AddRef();
	}

	
	// Prepare to open the data file
	m_LastErr = ERROR_SUCCESS;
	
	// Optionally define the File Create Property List
	if(Create==TRUE)
	{
		// Define some fields for the user block as the HDF5 block
		// driver will expect us to provide it
		if((hRes = _DefineRawHeader(BlockSize, Version))!=ERR_SUCCESS)
			return hRes;
		// Initialize the create property list
		m_hFcpl = H5Pcreate(H5P_FILE_CREATE);
		// Set the user block size
		if( m_hFcpl<0 || 
			H5Pset_userblock(m_hFcpl, BlockSize)<0
			// || H5Pset_alignment(fapl, 0, BlockSize)<0 // Set the alignment
			)
		{
			CheckXErr(Close());
			return ERR_EXTERNAL;
		}
	}
	// Define the Access property list	
	m_hFapl = H5Pcreate(H5P_FILE_ACCESS);
	if(m_hFapl<0)
	{
		wchar_t H5Msg[512] = {0};
		GetLastErrorDesc(H5Msg, sizeof(H5Msg)/2-1);
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to open the file system: %ls", H5Msg);
		ToLog(EV_ERROR, Msg);
		CheckXErr(Close());
		return ERR_EXTERNAL;
	}
	// Init the driver with this fapl
	m_Driver->Attach(m_hFapl);	

	// Convert the filename to ansi
	wchar_t UnicodePath[MAX_PATH]={0};
	char AnsiPath[MAX_PATH]={0};
	swprintf_s(UnicodePath, sizeof(UnicodePath)/2, L"%s\\%s.dat", m_DataFolder, FileName);	 
	
	WideCharToMultiByte( CP_ACP,                // ANSI Code Page
						 0,                      // No special handling of unmapped chars
						 UnicodePath,            // wide-character string to be converted
						 lstrlenW( UnicodePath ),
						 AnsiPath, 
						 sizeof(AnsiPath),
						 NULL, NULL );

	// Open or create the data file
	if(Create==TRUE)
		m_hFile = H5Fcreate(AnsiPath, 
							H5F_ACC_EXCL,
							m_hFcpl, 
							m_hFapl);
	else // Open
		m_hFile = H5Fopen  (AnsiPath, 
							H5F_ACC_RDWR,
							m_hFapl);

	if(m_hFile < 0)
	{
		wchar_t Msg[512] = {0};
		if(m_LastErr != ERR_ACCESS_DENIED)
		{
			wchar_t H5Msg[512] = {0};
			GetLastErrorDesc(H5Msg, sizeof(H5Msg)/2-1);
			swprintf_s(Msg, sizeof(Msg)/2, L"Failed to open the file system: %ls", H5Msg);
		}
		else
			swprintf_s(Msg, sizeof(Msg)/2, L"Failed to open the file system: Access denied");

		ToLog(EV_ERROR, Msg);
		DWORD hRet = m_LastErr?m_LastErr:ERR_EXTERNAL;
		CheckXErr(Close());
		return hRet;
	}
	
	// If it's create then initialize the meta records and check the results
	if(Create && (hRes=_CreateMetaRecords(Name, BlockSize, Version))!=ERR_SUCCESS)
	{
		CheckXErr(Close());
		return hRes;
	}

	// By now it must be created, so start the normal open routine
	
	// Read the file system header
	CheckXErr(_ReadMetaRecords());	

	return ERR_SUCCESS;
}
DWORD VirtualFS::_DefineRawHeader(DWORD BlockSize, DWORD Version)
{
	DWORD hRes = ERR_SUCCESS;
	// Generate the master key even if the FS is not 
	std::srand(static_cast<int>(time(NULL)));
	std::generate(MasterKey, MasterKey + sizeof(MasterKey), std::rand);

	// Calculate the hash of the unencrypted master key
	MD5 hasher;
	hasher.update(MasterKey, sizeof(MasterKey));
	hasher.finalize();

	//Generate random IV even if the FS is not encrypted
	std::generate(IV, IV + sizeof(IV), std::rand);

	INFO.FS_VERSION   = Version;
	INFO.BLOCK_SIZE   = BlockSize;
	INFO.IS_ENCRYPTED = _IsCrypto();   
	if(_IsCrypto())
	{		
		DWORD PwMode=0, PwAParam=0, PwBParam=0;
		m_PwdCrypt->GetParams(&PwMode, &PwAParam, &PwBParam);
		DWORD DataMode=0, DataAParam=0, DataBParam=0;
		m_DataCrypt->GetParams(&DataMode, &DataAParam, &DataBParam);
		

		INFO.KEY_ENC_MODE   = PwMode;                   //     Mode for the master key encryptor
		INFO.KEY_ENC_APARAM = PwAParam;                 // (A) Param  for the master key encryptor
		INFO.KEY_ENC_BPARAM = PwBParam;                 // (B) Param  for the master key encryptor
		INFO.DAT_ENC_MODE   = DataMode;                 //     Mode for the data blocks encryptor
		INFO.DAT_ENC_APARAM = DataAParam;               // (A) Param  for the data blocks encryptor
		INFO.DAT_ENC_BPARAM = DataBParam;               // (B) Param  for the data blocks encryptor
		memcpy(INFO.MKEY_HASH, hasher.digest, 16);      // 16 byte hash of unencrypted master key used for its verification when access password is given.

		if((hRes = _AssignAccessPassword())!=ERR_SUCCESS)
		{
			ToLog(EV_ERROR, L"Failed to write the master key");
			return ERR_EXTERNAL;
		}
	}
	else
	{
		// When the FS in not encrpyted we write all the same meta fields, but we set them zeros
		// and leave the keys unencrypted
		INFO.KEY_ENC_MODE   = 0;                         //     Mode for the master key encryptor
		INFO.KEY_ENC_APARAM = 0;                         // (A) Param  for the master key encryptor
		INFO.KEY_ENC_BPARAM = 0;                         // (B) Param  for the master key encryptor
		INFO.DAT_ENC_MODE   = 0;                         //     Mode for the data blocks encryptor
		INFO.DAT_ENC_APARAM = 0;                         // (A) Param  for the data blocks encryptor
		INFO.DAT_ENC_BPARAM = 0;                         // (B) Param  for the data blocks encryptor


		memcpy(INFO.MKEY, MasterKey, sizeof(MasterKey)); // 128 byte master key encrypted with access password. Its decrypted form is used for data encyption.
		memcpy(INFO.MKEY_HASH, hasher.digest, 16);       // 16 byte hash of unencrypted master key used for its verification when access password is given.
		memcpy(INFO.IV, IV, sizeof(IV));                 // 16 byte IV vector encrypted with access password. Its decrypted form is used for data encyption.
	}
	return hRes;
}
DWORD VirtualFS::_AssignAccessPassword()
{	
	// The correct password be already assigned to PwdCrypt
	DWORD hRes = ERR_SUCCESS;

	// 1. Use the access encryptor to encrypt the master key
	PBYTE MKeyEnc = nullptr;
	DWORD MKeyEncLength = 0;
	if((hRes = m_PwdCrypt->EncryptAlloc(MasterKey, sizeof(MasterKey), TRUE, &MKeyEnc, &MKeyEncLength))!=ERR_SUCCESS)
	{
		ToLog(EV_ERROR, L"Failed to encrypt the meta key");
		return ERR_EXTERNAL;
	}
	XDXAutoMem AutoMKey(MKeyEnc); // Auto delete using heapfree

	// 2. Use the access encryptor to encrypt the IV
	PBYTE IVEnc = nullptr;
	DWORD IVEncLength = 0;
	if((hRes = m_PwdCrypt->EncryptAlloc(IV, sizeof(IV), TRUE, &IVEnc, &IVEncLength))!=ERR_SUCCESS)
	{
		ToLog(EV_ERROR, L"Failed to encrypt the iv");
		return ERR_EXTERNAL;
	}
	XDXAutoMem AutoIV(IVEnc); // Auto delete using heapfree

	memcpy(INFO.MKEY, (PBYTE)MKeyEnc, MKeyEncLength); // 128 byte master key encrypted with access password. Its decrypted form is used for data encyption.
	memcpy(INFO.IV, (PBYTE)IVEnc, IVEncLength);       // 16 byte IV vector encrypted with access password. Its decrypted form is used for data encyption.
	return ERR_SUCCESS;
}
DWORD VirtualFS::_CreateMetaRecords(LPCWSTR Name, DWORD BlockSize, DWORD Version)
{
	DWORD hRes = ERR_SUCCESS;
	
	//Fill the file system meta fields
	CheckXErr(_WriteMetaBytes(FSMF_SZ_NAME, (PBYTE)Name, wcslen(Name)*2+2, FS_MAX_ALIAS));    // The alias of the file system
	CheckXErr(_WriteMetaDW(FSMF_DW_FILES_COUNT, 0));                                          // The overall number of files
	CheckXErr(_WriteMetaDW(FSMF_DW_DIR_COUNT, 0));                                            // The overall number of folders

	return ERR_SUCCESS;
}
DWORD VirtualFS::_ReadMetaRecords()
{
	DWORD hRes = ERR_SUCCESS;
	DWORD MaxSize = 0;
	//ZeroMemory(&INFO, sizeof(INFO));
 
	MaxSize = sizeof(INFO.NAME);           CheckXErr(_ReadMeta(FSMF_SZ_NAME,           (PBYTE)INFO.NAME,            &MaxSize));     
	MaxSize = sizeof(INFO.FILES_COUNT);    CheckXErr(_ReadMeta(FSMF_DW_FILES_COUNT,    (PBYTE)&INFO.FILES_COUNT,    &MaxSize));      
	MaxSize = sizeof(INFO.DIR_COUNT);      CheckXErr(_ReadMeta(FSMF_DW_DIR_COUNT,      (PBYTE)&INFO.DIR_COUNT,      &MaxSize)); 

	return ERR_SUCCESS;
}
DWORD VirtualFS::_WriteMetaDW(DWORD MetaId, DWORD Value)
{
	return _WriteMetaBytes(MetaId, (PBYTE)&Value, sizeof(Value), 0);
}
DWORD VirtualFS::_WriteMetaQW(DWORD MetaId, UINT64 Value)
{
	return _WriteMetaBytes(MetaId, (PBYTE)&Value, sizeof(Value), 0);
}
DWORD VirtualFS::_WriteMetaBytes(DWORD MetaId, PBYTE ByteVal, DWORD Size, size_t MaxLength)
{
    hid_t   root;
    DWORD   hRes = ERR_SUCCESS;

	if((root = H5Gopen(m_hFile, "/", H5P_DEFAULT))<0)
		{hRes = ERR_DISK_WRITE; goto L_DONE;}
	if((hRes==_WriteAttributeBytes(root, MetaId, ByteVal, Size, MaxLength))!=ERR_SUCCESS)
		{goto L_DONE;}
L_DONE:
    H5Gclose(root);

    return hRes;
}
DWORD VirtualFS::_WriteAttributeBytes(hid_t ObjId, DWORD FieldId, PBYTE ByteVal, DWORD Size, size_t MaxLength)
{
	if(ByteVal==nullptr)
		return ERR_ERROR_PARAM;
	CAutoWriteLock l(m_Lock);
    hid_t   dataspace, att;
    hid_t   type;
    DWORD   hRes = ERR_SUCCESS;
    hsize_t dims[1] = {1};

	if(MaxLength==0)
		MaxLength = Size; // for integral types

	char* AttrValue = new char[MaxLength];
	
    // Create a datatype to refer to
    if((type = H5Tcopy (H5T_C_S1))<0)
		{hRes = ERR_DISK_WRITE; goto L_DONE;}
    if(H5Tset_size (type, MaxLength)<0)
		{hRes = ERR_DISK_WRITE; goto L_DONE;}
    if((dataspace = H5Screate_simple(1, dims, NULL))<0)
		{hRes = ERR_DISK_WRITE; goto L_DONE;}
	// Convert the Attribute ID to a string
	char AttrName[64];
	sprintf_s(AttrName, sizeof(AttrName), "%d", FieldId);
	// Create or open the attribute
	if(!H5Aexists(ObjId, AttrName))
	{
		if((att = H5Acreate2(ObjId, AttrName, type, dataspace, H5P_DEFAULT, H5P_DEFAULT))<0)
			{hRes = ERR_DISK_WRITE; goto L_DONE;}
	}
	else
	{
		if((att = H5Aopen(ObjId, AttrName, H5P_DEFAULT))<0)
			{hRes = ERR_DISK_WRITE; goto L_DONE;}
	}
	// Copy to the buffer
	memcpy_s(AttrValue, MaxLength, ByteVal, min(MaxLength, Size));

	// Write attribute
    if(H5Awrite(att, type, AttrValue)<0)
		{hRes = ERR_DISK_WRITE; goto L_DONE;}
L_DONE:
	CloseH5handle(att, H5I_ATTR);
    H5Tclose(type);
	CloseH5handle(dataspace, H5I_DATASPACE);
	if(AttrValue!=nullptr)
		delete [] AttrValue;

    return hRes;
}
DWORD VirtualFS::_ReadMeta(DWORD MetaId, PBYTE ByteVal, DWORD *MaxSize)
{
    hid_t root;
    DWORD hRes = ERR_SUCCESS;
	
	// Open the root group
	if((root = H5Gopen(m_hFile, "/", H5P_DEFAULT))<0)
		{hRes = ERR_DISK_READ; goto L_DONE;}
	if((hRes==_ReadAttributeBytes(root, MetaId, ByteVal, *MaxSize))!=ERR_SUCCESS)
		{goto L_DONE;}
L_DONE:
    H5Gclose(root);

    return hRes;
}
DWORD VirtualFS::_ReadAttributeBytes(hid_t ObjId, DWORD FieldId, PBYTE ByteVal, DWORD MaxSize)
{
	if(ByteVal==nullptr)
		return ERR_ERROR_PARAM;
	if(MaxSize==0)
		return ERR_ERROR_PARAM;
	CAutoReadLock l(m_Lock);
    hid_t att;
    hid_t type, ftype;
    H5T_class_t type_class;
	char* AttrValue = nullptr;
	size_t ValLength = 0;
    DWORD hRes = ERR_SUCCESS;
    hsize_t   dims[1] = {1};

	// Convert the Attribute ID to a string
	char AttrName[64];
	sprintf_s(AttrName, sizeof(AttrName), "%d", FieldId);
		
	// Open the dataset's attribute
	if((att = H5Aopen(ObjId, AttrName, H5P_DEFAULT))<0)
		{hRes = ERR_DISK_READ; goto L_DONE;}
	
	// Get the attribute type
    if((ftype = H5Aget_type(att))<0)
		{hRes = ERR_DISK_READ; goto L_DONE;}
	
    if((type_class = H5Tget_class(ftype))!=H5T_STRING) 
		{hRes = ERR_DISK_READ; goto L_DONE;}
	
    if((type = H5Tget_native_type(ftype, H5T_DIR_ASCEND))<0)
		{hRes = ERR_DISK_READ; goto L_DONE;}

	if((ValLength = H5Tget_size(ftype))<0)
		{hRes = ERR_DISK_READ; goto L_DONE;}
	
	// Allocate buffer to read the whole value
	AttrValue = new char[ValLength];

	// Read attribute
    if(H5Aread(att, type, AttrValue)<0)
		{hRes = ERR_DISK_READ; goto L_DONE;}

	// Copy to the buffer
	//ZeroMemory(Buffer, BufLength);
	memcpy_s(ByteVal, MaxSize, AttrValue, min(ValLength, MaxSize));
L_DONE:
    H5Aclose(att);
    H5Tclose(type);
	if(AttrValue!=nullptr)
		delete [] AttrValue;

    return hRes;
}
BOOL  VirtualFS::_IsPathValid(LPCWSTR Path)
{
	// Pointer is valid
	if(Path==nullptr)
		return FALSE; 
	size_t PathLen = wcslen(Path);
	// Path isn't too short
	if(PathLen<2 || PathLen >= MAX_PATH)
		return FALSE;
	// Path starts with a slash
	if(Path[0]!=L'/')
		return FALSE;
	// No empty path components
	if(wcsstr(Path, L"//")!=nullptr)
		return FALSE;
	// Path doesn't end with a slash
	if(Path[PathLen-1]==L'/')
		return FALSE;
	return TRUE;
}
DWORD VirtualFS::_FollowPath(LPCWSTR Path, H5I_type_t& ObjectType, hid_t& ObjectId)
{
	DWORD hRes = ERR_SUCCESS;
	ObjectType = H5I_UNINIT;
	ObjectId = -1;

	// Check the given path
	if(Path == nullptr)
		return ERR_NOT_FOUND;
	size_t PathLength = wcslen(Path);
	if(PathLength==0)
		return ERR_NOT_FOUND;
	if(PathLength<1 || PathLength>=MAX_PATH)
		return ERR_ERROR_PARAM;
	if(Path[0]!=L'/')
		return ERR_ERROR_PARAM;
	
	// Convert the name to utf8
	std::string Utf8Name = TICUtils::WStringToUtf8(Path);

	// Check that the given name exists
	// first try to check it as a dataset
	ObjectId = H5Dopen2(m_hFile, Utf8Name.c_str(), H5P_DEFAULT);
	if(ObjectId<0)
	{		
		// now as a group
		ObjectId = H5Gopen2(m_hFile, Utf8Name.c_str(), H5P_DEFAULT);
		if(ObjectId<0)
		{
			hRes = ERR_NOT_FOUND;
			Rollback();
			return hRes;
		}
		// It's a group
		ObjectType = H5I_GROUP;
	}
	else // it's a file
	{
		ObjectType = H5I_DATASET;
	}

	return hRes;
}
DWORD VirtualFS::_PathCreate(LPCWSTR Name, DWORD Attributes, UINT64 CreatedBy)
{
	DWORD hRes = ERR_SUCCESS;
	std::string Utf8Name;
	if(Name == nullptr)
		return ERR_ERROR_PARAM;

	bool isFolder = (Attributes & FILE_ATTRIBUTE_DIRECTORY)>0;
	
	// Check the given path
	if(!_IsPathValid(Name))
		return ERR_ERROR_PARAM;
	
	hid_t lcpl_id = H5P_DEFAULT;
	hid_t path_id = -1;

	// Setup the group to support unicode (Utf8)
	if((lcpl_id = H5Pcreate(H5P_LINK_CREATE)) < 0 ||
		H5Pset_char_encoding(lcpl_id, H5T_CSET_UTF8) < 0)
	{
		wchar_t H5Msg[512] = {0};
		GetLastErrorDesc(H5Msg, sizeof(H5Msg)/2-1);
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to create an FS element: %ls", H5Msg);
		ToLog(EV_ERROR, Msg);
		Rollback();
		goto L_DONE;
	}
	// Convert the name to utf8
	Utf8Name = TICUtils::WStringToUtf8(Name);
	
	// Check that the given name is not used already
	path_id = isFolder?H5Gopen2(m_hFile, Utf8Name.c_str(), H5P_DEFAULT):H5Dopen2(m_hFile, Utf8Name.c_str(), H5P_DEFAULT);
	if(path_id>0)
	{
		hRes = ERR_DUPLICATE;
		Rollback();
		goto L_DONE;
	}

	// Create a group or a dataset with the given name in the file.
	if(isFolder)
	{
		path_id = H5Gcreate2(m_hFile, Utf8Name.c_str(), lcpl_id, H5P_DEFAULT, H5P_DEFAULT); // Create a group
	}
	else
	{
		// Define a dataspace 
		hsize_t dims[1]  = {0};           // dataset dimensions at creation time
		hsize_t maxdims[1] = {H5S_UNLIMITED};
		hid_t dataspace = H5Screate_simple (1, dims, maxdims); 

		// Modify dataset creation properties, enable chunking 
		hsize_t chunk_dims = m_Driver->GetBlockSize()*FILE_CHUNK_BLOCKS;
		hid_t prop = H5Pcreate (H5P_DATASET_CREATE);
		if(H5Pset_chunk (prop, 1, &chunk_dims)>=0)
		{
			path_id = H5Dcreate2 (m_hFile, Utf8Name.c_str(), H5T_NATIVE_SCHAR, dataspace, H5P_DEFAULT, prop, H5P_DEFAULT); // Create a data file
		}
		
		CloseH5handle(dataspace, H5I_DATASPACE);  // close the space
		CloseH5handle(prop, H5I_GENPROP_LST);     // close create property list
	}
	if(path_id<0)
	{
		hRes = ERR_NOT_FOUND;
		Rollback();
		goto L_DONE;
	}

	// Write its attributes
	uint64_t Time = GetTime();
	
	FileAttributesDos attr;
	attr.DW_FILE_ATTRIBUTES = Attributes ; 
	attr.QW_CREATED_BY      = CreatedBy      ; 
	attr.QW_CREATION_TIME   = Time   ; 
	attr.QW_ACCESS_TIME     = Time ; 
	attr.QW_WRITE_TIME      = Time  ; 
	
	if( (hRes=_WriteAttributeBytes(path_id, FILE_ATTRIBUTES_DOS, (PBYTE)&attr, sizeof(attr), 0))!=ERR_SUCCESS)
	{
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to create an FS element (iNode attributes): %ls", ErrorTexts::GetErrorDesc(hRes));
		ToLog(EV_ERROR, Msg);
		Rollback();
		goto L_DONE;
	}

	// Increment the number of directories
	if(isFolder)
	{
		INFO.DIR_COUNT++;
		hRes=_WriteMetaDW(FSMF_DW_DIR_COUNT, INFO.DIR_COUNT);
	}
	else
	{
		INFO.FILES_COUNT++;
		hRes=_WriteMetaDW(FSMF_DW_FILES_COUNT, INFO.FILES_COUNT);
	}

	if(hRes!=ERR_SUCCESS)
	{
		wchar_t Msg[512] = {0};
		swprintf_s(Msg, sizeof(Msg)/2, L"Failed to an FS element counter (meta counter I/O): %ls", ErrorTexts::GetErrorDesc(hRes));
		ToLog(EV_ERROR, Msg);
		Rollback();
		goto L_DONE;
	}	
	Commit();
L_DONE:
	CloseH5handle(lcpl_id, H5I_GENPROP_LST);
	CloseH5handle(path_id, isFolder?H5I_GROUP:H5I_DATASET);

	return hRes;
}
DWORD VirtualFS::_AcquireRealHandle(LPCWSTR FileName, UINT64 CreatedBy, DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, RealHandle& hResFile)
{
	DWORD hRes = ERR_SUCCESS;
	RealHandle hHandleRes;
	
	// 1. Check the file handle exists already
	bool HandleExists = false;
	RealHandle* hFile = nullptr;
	NamedHandlesT::iterator hFindName = m_NamedHandles.find(FileName);
	if(hFindName!=m_NamedHandles.end())
	{
		RawHandle realKey = hFindName->second;
		RealHandlesT::iterator hFindHandle = m_RealHandles.find(realKey);
		if(hFindHandle == m_RealHandles.end())
			return ERR_EXTERNAL;
		hFile = &hFindHandle->second;
	}
	HandleExists = (hFile!=nullptr);

	// 2. If there's no handle then check if the file exists already
	bool FileExists = false;
	if(!HandleExists)
	{
		H5I_type_t ObjectType = H5I_UNINIT;
		if(_FollowPath(FileName, ObjectType, hHandleRes.rawHandle)==ERR_SUCCESS)
		{
			if(ObjectType != H5I_DATASET)
				return ERR_NOT_FOUND;
			FileExists = true;
		}
	}
	else
		FileExists = true;

	if(!FileExists && HandleExists) // Shall never happen
		return ERR_EXTERNAL;

	// 2. Creation mode basic checks
	//    CREATE_NEW        - Creates, fails to overwrite
	//    CREATE_ALWAYS     - Creates, overwrites
	//    OPEN_EXISTING     - Opens, doesn't create
	//    OPEN_ALWAYS       - Opens, creates
	//    TRUNCATE_EXISTING - Opens, doesn't create, truncates to 0
	if(!FileExists && (CreationDisposition==OPEN_EXISTING || CreationDisposition==TRUNCATE_EXISTING))
		return ERR_NOT_FOUND; // Those must fail if there's no file
	if(FileExists && CreationDisposition==CREATE_NEW)
		return ERR_IN_USE; // That one must fail if file already exists
	if(FileExists && (CreationDisposition==CREATE_ALWAYS || CreationDisposition==TRUNCATE_EXISTING) && (DesiredAccess&GENERIC_WRITE)==0)
		return ERR_ACCESS_DENIED; // That one must fail if file must be rewritten, but the write access not requested
	if(!FileExists && (CreationDisposition==OPEN_ALWAYS || CreationDisposition==CREATE_ALWAYS || CreationDisposition==CREATE_NEW) && (DesiredAccess&GENERIC_WRITE)==0)
		return ERR_ACCESS_DENIED; // That one must fail if file could be rewritten, but the write access not requested

	// 3. If there's no handle, allocate it
	if(!HandleExists)
	{
		// 3.1. If there's no file create it first
		if(!FileExists)
		{
			if((hRes=_PathCreate(FileName, FILE_ATTRIBUTE_NORMAL, CreatedBy))!=ERR_SUCCESS)
			{
				return hRes;
			}
		}
		// 3.2. Either file exists or created by now - Open it normally anyway
		hid_t item_id = -1;
		H5I_type_t item_type = H5I_UNINIT;
		if((hRes=_FollowPath(FileName, item_type, item_id))!=ERR_SUCCESS)
			return hRes;

		// Initilialize the new real handle
		RealHandle tmpFile;
		tmpFile.wsPath     = FileName;
		tmpFile.rawHandle  = item_id;
		tmpFile.uReaders   = 0;
		tmpFile.uWriters   = 0;
		tmpFile.isFile     = true;
		tmpFile.fShareMode = ShareMode;

		// Place the new real handle into the collections
		m_RealHandles[item_id]   = tmpFile;
		m_NamedHandles[FileName] = item_id;

		// Get a reference from the collection
		hFile = &m_RealHandles[item_id];
	}

	// 4. Increment RW lock counters
	if(DesiredAccess & GENERIC_READ)
		hFile->uReaders++;
	if(DesiredAccess & GENERIC_WRITE)
		hFile->uWriters++;

	// 5. Check the share mode compatibility
	//    Can fail only for the already opened handles
	if((hFile->fShareMode & ShareMode) != ShareMode ||
		HandleExists && ShareMode==0 ||
		HandleExists && (hFile->fShareMode & FILE_SHARE_WRITE)==0 && (DesiredAccess&GENERIC_WRITE)>0 || // request write when no write share was previously set
		HandleExists && (hFile->fShareMode & FILE_SHARE_READ)==0 && (DesiredAccess&GENERIC_READ)>0      // request read when no read share was previously set
		)
	{
		_ReleaseRealHandle(DesiredAccess, *hFile);
		return ERR_ACCESS_DENIED;
	}


	hResFile = *hFile;
	return ERR_SUCCESS;
}
DWORD VirtualFS::_ReleaseRealHandle(DWORD AccessMode, RealHandle hFile)
{
	// 1. Find the real handle
	RawHandle realKey = hFile.rawHandle;
	RealHandlesT::iterator hFindHandle = m_RealHandles.find(realKey);
	if(hFindHandle == m_RealHandles.end())
		return ERR_NOT_FOUND;

	// 2. Decrement RW lock counters	
	if((AccessMode & GENERIC_READ) && hFile.uReaders>0)
		hFindHandle->second.uReaders--;
	if((AccessMode & GENERIC_WRITE) && hFile.uWriters>0)
		hFindHandle->second.uWriters--;

	// 3. Check the remaining readers/writers
	bool SomeoneLeft = (hFindHandle->second.uReaders>0 || hFindHandle->second.uWriters>0);
	if(SomeoneLeft)
		return ERR_SUCCESS;

	// 4. If no one uses this handle anymore then actually close it
	CloseH5handle(hFile.rawHandle, hFile.isFile?H5I_DATASET:H5I_GROUP);

	// 5. Now remove it from the collections
	m_NamedHandles.erase(hFindHandle->second.wsPath);
	m_RealHandles.erase(realKey);

	return ERR_SUCCESS;
}
DWORD VirtualFS::_FileCloseInternal(UserHandlesT::iterator iiUserHandle)
{
	DWORD hRes = ERR_SUCCESS;
	// Find the real handle where it points
	UserHandle uhFile = iiUserHandle->second;
	auto iiRealHandle = m_RealHandles.find(uhFile.hRealHandle);
	if(iiRealHandle != m_RealHandles.end())
	{
		RealHandle rhFile = iiRealHandle->second;

		// 3. Close real handle
		if((hRes=_ReleaseRealHandle(uhFile.fAccessMode, rhFile))!=ERR_SUCCESS)
			return hRes;
	}
	// Remove user handle
	m_UserHandles.erase(iiUserHandle);

	return ERR_SUCCESS;
}
}