#pragma once

extern "C"
{
#include "H5Ipublic.h"
}

#pragma region Defines
// These macros check for overflow of various quantities.  These macros
// assume that file_offset_t is signed and haddr_t and size_t are unsigned.
//
// XHDF5_ADDR_OVERFLOW:  Checks whether a file address of type `haddr_t'
//      is too large to be represented by the second argument
//      of the file seek function.
//
// XHDF5_SIZE_OVERFLOW:  Checks whether a buffer size of type `hsize_t' is too
//      large to be represented by the `size_t' type.
//
// XHDF5_REGION_OVERFLOW:  Checks whether an address and size pair describe data
//      which can be addressed entirely by the second
//      argument of the file seek function.
#define XHDF5_MAXADDR (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define XHDF5_ADDR_OVERFLOW(A)  (HADDR_UNDEF==(A) || ((A) & ~(haddr_t)XHDF5_MAXADDR))
#define XHDF5_SIZE_OVERFLOW(Z)  ((Z) & ~(hsize_t)XHDF5_MAXADDR)
#define XHDF5_REGION_OVERFLOW(A,Z)  (XHDF5_ADDR_OVERFLOW(A) || XHDF5_SIZE_OVERFLOW(Z) || HADDR_UNDEF==(A)+(Z) || (file_offset_t)((A)+(Z))<(file_offset_t)(A))

// This driver supports systems that have the lseek64() function by defining
// some macros here so we don't have to have conditional compilations later
// throughout the code.
//
// file_offset_t:  The datatype for file offsets, the second argument of
//      the lseek() or lseek64() call.
//
// file_seek:    The function which adjusts the current file position,
//      either lseek() or lseek64().
//
#ifdef H5_HAVE_LSEEK64
#   define file_offset_t  off64_t
#   define file_seek    lseek64
#   define file_tell    ltell64
#   define file_truncate  ftruncate64
#elif defined (H5_HAVE_WIN32_API)
# /*MSVC*/
#   define file_offset_t __int64
#   define file_seek _lseeki64
#   define file_tell _telli64
#   define file_truncate  _chsize
#else
#   define file_offset_t  off_t
#   define file_seek    lseek
#   define file_truncate  HDftruncate
#endif

#pragma endregion

namespace XHdf5
{
	// Default values for memory boundary, file block size, and maximal copy buffer size.
	// Application can override these values in the constructor
	enum enDefLimits
	{
		MBOUNDARY_DEF	= 4096,
		FBSIZE_DEF		= 4096,
		CBSIZE_DEF		= 16*1024*1024
	};
	// File operations
	enum enFileOperations
	{		
		OP_UNKNOWN  = 0,
		OP_READ     = 1,
		OP_WRITE    = 2,
	};
	class BlockDriver;
	// Driver-specific file access properties
	typedef struct 
	{
		BlockDriver* drv; 
		size_t       ubsize;
		size_t       fbsize;    // File system block size
		size_t       cbsize;    // Maximal buffer size for copying user data
	} FAPL_t;
	
	// The description of a file belonging to this driver. 
	// The `eoa' and `eof'
	// determine the amount of hdf5 address space in use and the high-water mark
	// of the file (the current size of the underlying Unix file). 
	// The `pos'
	// value is used to eliminate file position updates when they would be a
	// no-op. Unfortunately we've found systems that use separate file position
	// indicators for reading and writing so the lseek can only be eliminated if
	// the current operation is the same as the previous operation.  When opening
	// a file the `eof' will be set to the current file size, `eoa' will be set
	// to zero, `pos' will be set to H5F_ADDR_UNDEF (as it is when an error
	// occurs), and `op' will be set to H5F_OP_UNKNOWN.
	typedef struct 
	{
		H5FD_t       pub;      // public stuff, must be first
		int          fd;       // the unix file   
		haddr_t      eoa;      // end of allocated region 
		haddr_t      eof;      // end of file; current file size
		haddr_t      pos;      // current file I/O position 
		int          op;       // last operation  
		FAPL_t       fa;       // file access properties
	#ifndef H5_HAVE_WIN32_API
		// On most systems the combination of device and i-node number uniquely identify a file.
		dev_t  device;     // file device number
	#ifdef H5_VMS
		ino_t  inode[3];   // file i-node number
	#else
		ino_t  inode;      // file i-node number
	#endif /*H5_VMS*/
	#else
		// On H5_HAVE_WIN32_API the low-order word of a unique identifier associated with the
		// file and the volume serial number uniquely identify a file. This number
		// (which, both? -rpm) may change when the system is restarted or when the
		// file is opened. After a process opens a file, the identifier is
		// constant until the file is closed. An application can use this
		// identifier and the volume serial number to determine whether two
		// handles refer to the same file.
		DWORD    fileindexlo;
		DWORD    fileindexhi;
	#endif
	} FileHandle_t;

	class DriverCallback
	{
	public:
		virtual int OnH5WriteUserBlock(void * Buffer, unsigned int Size)=0;
		virtual int OnH5ReadUserBlock(void * Buffer, unsigned int Size)=0;
		virtual int OnH5FillEmptyBlock(void * Buffer, unsigned int Size)=0;
		virtual int OnH5AfterBlockRead(void * Buffer, unsigned int Size)=0;
		virtual int OnH5BeforeBlockWrite(void * Buffer, unsigned int Size)=0;
		virtual void OnH5ToLog(DWORD Event, LPCWSTR Message)=0;
	};

	class BlockDriver
	{
	public:
		BlockDriver(/*size_t userblock_size, */size_t block_size, size_t cbuf_size, DriverCallback* Callback);
		virtual ~BlockDriver();
		herr_t Attach(hid_t fapl_id);
		herr_t UpdateUserBlock();
		void SetBlockSize(/*size_t userblock_size, */size_t BlockSize)
		{
			m_UBlockSize = BlockSize;
			m_BlockSize = BlockSize;
		}
		size_t GetBlockSize()
		{
			return m_BlockSize;
		}
	protected: // Low-level routines
		hid_t  InitDriver(void);
		static void TerminateDriver(void);
		herr_t ReadFAPL(hid_t fapl_id, size_t* userblock_size, size_t *block_size/*out*/, size_t *cbuf_size/*out*/);
		int DoFillEmptyBlock(void * Buffer, unsigned int Size);
		int DoWriteUserBlock(int FileHandle, void * Buffer, unsigned int Size);
		int DoReadUserBlock(void * Buffer, unsigned int Size);
		int DoBlockRead(int FileHandle, void * Buffer, unsigned int Size);
		int DoBlockWrite(int FileHandle, void * Buffer, unsigned int Size);
	protected: // Callbacks
		static void*   fapl_get(H5FD_t *_file);
		static void*   fapl_copy(const void *_old_fa);
		static H5FD_t* open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr);
		static herr_t  close(H5FD_t *_file);
		static int     cmp(const H5FD_t *_f1, const H5FD_t *_f2);
		static herr_t  query(const H5FD_t * _f, unsigned long *flags /* out */);		
		static haddr_t get_eoa(const H5FD_t *_file, H5FD_mem_t type);
		static herr_t  set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
		static haddr_t get_eof(const H5FD_t *_file);
		static herr_t  get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
		static herr_t  read(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size, void *buf/*out*/);
		static herr_t  write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size, const void *buf);
		static herr_t  truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);
		static herr_t  flush(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);
		static haddr_t alloc(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
	private:
		static hid_t    m_DriverID;    // The driver identification number, initialized at runtime
		DriverCallback* m_Callback;    // User defined callbacks optionally used by the driver
		FileHandle_t*   m_File;
		size_t          m_UBlockSize;  // User block size
		size_t          m_BlockSize;   // File block size
		size_t          m_MemBufSize;  // Memory to be allocated for copying data
	};
}


