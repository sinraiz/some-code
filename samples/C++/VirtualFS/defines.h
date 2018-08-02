#pragma once


using namespace XDX::Objects;
namespace XDX
{
	#pragma pack(push, 1)
	enum
	{		
		//USERBLOCK_SIZE = 1024,
		MASTER_KEY_LEN = 128,
		DATA_IV_LEN = 16,
		FILE_CHUNK_BLOCKS = 16  // Number of Data Blocks per allocation chunk
	};
	
	#define XDX_SIGNATURE "XDX FS"
	struct RawFileHeader
	{
		char     Signature[8];              // 'X','D','X',' ','F', 'S' - The XDX block file signature
		uint32_t Version;          
		uint32_t BlockSize; 
		uint32_t IsEncrypted;          
		uint32_t KeyEncodeMode;    
		uint32_t KeyEncodeAParam;  
		uint32_t KeyEncodeBParam;  
		uint32_t DataEncodeMode;    
		uint32_t DataEncodeAParam;  
		uint32_t DataEncodeBParam; 
		byte     MKey[MASTER_KEY_LEN];            
		byte     MKeyHash[16];       
		byte     IV[MASTER_KEY_LEN]; 
	};
	struct FSInfo
	{
		DWORD  FS_VERSION;
		WCHAR  NAME[MAX_PATH];            
		DWORD  BLOCK_SIZE;     
		DWORD  FILES_COUNT;     
		DWORD  DIR_COUNT;       
		DWORD  USED_BLOCKS;     
		DWORD  FREE_BLOCKS;    
		UINT64 USED_BYTES;  
		DWORD  IS_ENCRYPTED;    
		BYTE   MKEY[MASTER_KEY_LEN];            
		BYTE   MKEY_HASH[16];       
		BYTE   IV[128];               
		DWORD  KEY_ENC_MODE;    
		DWORD  KEY_ENC_APARAM;  
		DWORD  KEY_ENC_BPARAM;  
		DWORD  DAT_ENC_MODE;    
		DWORD  DAT_ENC_APARAM;  
		DWORD  DAT_ENC_BPARAM; 
	};
	enum enFileAttributes
	{
		FILE_ATTRIBUTES_DOS = 1,
		/*QW_CREATED_BY      = 2,
		QW_CREATION_TIME   = 3,
		QW_ACCESS_TIME     = 4,
		QW_WRITE_TIME      = 5*/
	};
	struct FileAttributesDos
	{
		uint32_t DW_FILE_ATTRIBUTES;
		uint64_t QW_CREATED_BY;
		uint64_t QW_CREATION_TIME;
		uint64_t QW_ACCESS_TIME;
		uint64_t QW_WRITE_TIME;
	};
	/*
	enum class FSRecords: BYTE
	{
		META    = 0,     // File system meta information records
		FILE    = 1,     // Identifies a file or a folder general info
		FS_VERSION = 2,     // A record corresponding to a specific version of a file
		EXTENT  = 3,     // Identifies an extent record for a file
		HISTORY = 4,     // Identifies a historic record of user activity
		INDEX   = 5,     // For the context search
	};

	struct MetaKey
	{
		FSRecords    type;       // Must be set to FSRecords::META
		DWORD        FieldId;
	};

	struct FileKey
	{
		FSRecords    type;       // Must be set to FSRecords::FILE
		DWORD        id_parent;
		WCHAR        name[FS_MAX_FILENAME];
	};
	struct FileData
	{
		BYTE         dummy[4];   //If the data structure changes to have an option to move to variadic saving

		DWORD        id;
		DWORD        attributes;
		UINT64       created_by;
		UINT64       time_created;
		UINT64       time_accessed;
		UINT64       time_written;
		UINT64       file_size;
		DWORD        id_last_version;
		UINT64       reserved[5];
	};

	struct VersionKey
	{
		FSRecords    type;       // Must be set to FSRecords::VERSION
		DWORD        id_file;
		DWORD        id_version;
	};
	struct VersionData
	{
		BYTE         dummy[4];   //If the data structure changes to have an option to move to variadic saving
		UINT64       created_by;
		UINT64       time_created;
		UINT64       file_size;
	};
	
	struct ExtentKey
	{
		FSRecords    type;       // Must be set to FSRecords::EXTENT
		DWORD        id_file;
		DWORD        id_version;
		DWORD        id_start_block;
	};*/

#pragma pack(pop)
}