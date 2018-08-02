#include "stdafx.h"
extern "C"
{
	#include "H5private.h"     // Generic Functions     
	#include "H5Edefin.h"      // Errors
	#include "H5Eprivate.h"    // Error handling   
	#include "H5Epublic.h"    // Error handling   
	#include "H5Fprivate.h"    // File access     
	#include "H5FDprivate.h"   // File drivers      
	#include "H5FDblock.h"     // Block file driver    
	#include "H5FLprivate.h"   // Free Lists  
	#include "H5Iprivate.h"    // IDs         
	#include "H5MMprivate.h"   // Memory management     
	#include "H5Pprivate.h"    // Property lists  
}

hid_t XHdf5::BlockDriver::m_DriverID = 0;
hid_t H5E_ERR_CLS_g;
namespace XHdf5
{
BlockDriver::BlockDriver(/*size_t userblock_size, */size_t block_size, size_t cbuf_size, DriverCallback* Callback)
{
	m_Callback   = Callback;
	m_UBlockSize = block_size;
	m_BlockSize  = block_size;
	m_MemBufSize = cbuf_size;
	m_File       = nullptr;
}
BlockDriver::~BlockDriver()
{
}
////////////////////////////////////////////////////////////////
// Description:  
//      Initialize this driver by registering the driver with the library.
// Return: 
//      Success:  The driver ID for the crypto driver.
//      Failure:  Negative.
////////////////////////////////////////////////////////////////
hid_t BlockDriver::InitDriver(void)
{
	hid_t ret_value;        // Return value
//	FUNC_ENTER_NOAPI(FAIL)
	H5FD_class_t H5FD_crypto_g = {
									"xdx_h5_block_driver",       // name      
									XHDF5_MAXADDR,               // maxaddr    
									H5F_CLOSE_WEAK,              // fc_degree    
									NULL,                        // sb_size    
									NULL,                        // sb_encode    
									NULL,                        // sb_decode    
									sizeof(FAPL_t),              // fapl_size    
									BlockDriver::fapl_get,       // fapl_get    
									BlockDriver::fapl_copy,      // fapl_copy    
									NULL,                        // fapl_free    
									0,                           // dxpl_size    
									NULL,                        // dxpl_copy    
									NULL,                        // dxpl_free    
									BlockDriver::open,           // open      
									BlockDriver::close,          // close      
									NULL,/*BlockDriver::cmp, */  // cmp      
									BlockDriver::query,          // query      
									NULL,                        // get_type_map    
									NULL,/*BlockDriver::alloc*/  // alloc      
									NULL,                        // free      
									BlockDriver::get_eoa,        // get_eoa    
									BlockDriver::set_eoa,        // set_eoa    
									BlockDriver::get_eof,        // get_eof    
									BlockDriver::get_handle,     // get_handle            
									BlockDriver::read,           // read      
									BlockDriver::write,          // write      
									BlockDriver::flush,          // flush      
									BlockDriver::truncate,       // truncate    
									NULL,                        // lock                  
									NULL,                        // unlock                
									H5FD_FLMAP_SINGLE            // fl_map    
								};

	if (H5I_VFL != H5I_get_type(m_DriverID))
		m_DriverID = H5FD_register(&H5FD_crypto_g, sizeof(H5FD_class_t), FALSE);

	// Set return value
	ret_value = m_DriverID;

	//return (ret_value)
	return ret_value;
}
////////////////////////////////////////////////////////////////
// Description:  
//      Shut down the VFD
////////////////////////////////////////////////////////////////
void BlockDriver::TerminateDriver(void)
{
	// FUNC_ENTER_NOAPI_NOINIT_NOERR
	// Reset VFL ID
	m_DriverID = 0;

	
}
////////////////////////////////////////////////////////////////
// Description:  
//      Modify the file access property list to use the H5FD_CRYPTO
//      driver defined in this source file.  There are no driver
//      specific properties.
// Return: 
//      Non-negative on success/Negative on failure
////////////////////////////////////////////////////////////////
herr_t BlockDriver::Attach(hid_t fapl_id)
{
	H5P_genplist_t* plist;      // Property list pointer
	FAPL_t          fa;
	herr_t          ret_value;

	//// FUNC_ENTER_API(FAIL)
	//	//H5TRACE4("e", "izzz", fapl_id, boundary, m_BlockSize, m_MemBufSize);

	if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
	{
		m_Callback->OnH5ToLog(H5E_BADTYPE, L"not a file access property list"); 
		return FAIL;
	}

	if(m_BlockSize != 0)
		fa.fbsize = m_BlockSize;
	else
		fa.fbsize = FBSIZE_DEF;

	if(m_UBlockSize != 0)
		fa.ubsize = m_UBlockSize;
	else
		fa.ubsize = FBSIZE_DEF;

	if(m_MemBufSize != 0)
		fa.cbsize = m_MemBufSize;
	else
		fa.cbsize = CBSIZE_DEF;


	// Copy buffer size must be a multiple of file block size
	if(fa.cbsize % fa.fbsize != 0)
	{
		m_Callback->OnH5ToLog(H5E_BADTYPE, L"copy buffer size must be a multiple of block size"); 
		return FAIL;
	}
	// User block size must be musltiple of 512 and be >=512
	if(fa.ubsize % 512 != 0 || fa.ubsize<512)
	{
		m_Callback->OnH5ToLog(H5E_BADTYPE, L"wrong user block size"); 
		return FAIL;
	}

	fa.drv = this;
	ret_value= H5P_set_driver(plist, InitDriver(), &fa);

//done:
	return(ret_value);
}
herr_t BlockDriver::UpdateUserBlock()
{
	if(m_File==nullptr)
		return -1;
	// Allocate a buffer for use block
	uint32_t UserBlockSize = m_File->fa.ubsize;
	void* UserBlockBuffer = HDmalloc(UserBlockSize);
	if(UserBlockBuffer == nullptr)
	{
		m_Callback->OnH5ToLog(H5E_NOSPACE, L"unable to allocate user block"); 
		return -1;
	}

	// Let the callback fill it
	if(DoWriteUserBlock(m_File->fd, UserBlockBuffer, UserBlockSize)!=0)
	{
		HDfree(UserBlockBuffer);
		m_Callback->OnH5ToLog(H5E_NOSPACE, L"callback was unable to make user block"); 
		return -1;
	}

	// Free user block
	HDfree(UserBlockBuffer);
	return 0;
}
////////////////////////////////////////////////////////////////
// Description:  
//      Returns information about the crypto file access property
//      list though the function arguments.
// Return: 
//      Success:  Non-negative
//      Failure:  Negative
////////////////////////////////////////////////////////////////
herr_t BlockDriver::ReadFAPL(hid_t fapl_id, size_t* userblock_size, size_t *BlockSize/*out*/, size_t *MemBufSize/*out*/)
{
	FAPL_t  *fa;
	H5P_genplist_t *plist;          // Property list pointer
	herr_t      ret_value=SUCCEED;  // Return value

	// FUNC_ENTER_API(FAIL)
		//H5TRACE4("e", "ixxx", fapl_id, boundary, BlockSize, MemBufSize);

	if(NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
	{
		m_Callback->OnH5ToLog(H5E_BADTYPE, L"not a file access list"); 
		return FAIL;
	}
	if (InitDriver()!=H5P_get_driver(plist))
	{
		m_Callback->OnH5ToLog(H5E_BADTYPE, L"incorrect VFL driver"); 
		return FAIL;
	}
	if (NULL==(fa=(FAPL_t*)H5P_get_driver_info(plist)))
	{
		m_Callback->OnH5ToLog(H5E_BADVALUE, L"bad VFL driver info"); 
		return FAIL;
	}


	if (BlockSize)
		*BlockSize = fa->fbsize;
	if (MemBufSize)
		*MemBufSize = fa->cbsize;
	if (userblock_size)
		*userblock_size = fa->ubsize;
//done:
	return (ret_value);
}
int BlockDriver::DoFillEmptyBlock(void * Buffer, unsigned int Size)
{
	if(m_Callback == nullptr)
		return 0;
	int remaining_bytes = Size;
	int pos = 0;
	int ChunkSize = (m_BlockSize>0)?m_BlockSize:Size;
	do
	{
		pos = (Size - remaining_bytes);
		if(m_Callback->OnH5FillEmptyBlock((char*)Buffer + pos, ChunkSize)<0)
			return -1;
		remaining_bytes-=ChunkSize;
	}
	while(remaining_bytes>0);

	return 0;
}
int BlockDriver::DoWriteUserBlock(int FileHandle, void * Buffer, unsigned int Size)
{
	if(m_Callback == nullptr)
		return 0;
	if(m_Callback->OnH5WriteUserBlock(Buffer, Size)!=0)
		return -1;

	// Get the current fileposition
	file_offset_t old_pos = file_tell(FileHandle);

	// Jump to the user-block place in the file	
	if(file_seek(FileHandle, (file_offset_t)(0), SEEK_SET) < 0)
	{
		m_Callback->OnH5ToLog(H5E_IO, L"unable to seek to proper position"); 
		return FAIL;
	}
	
	// Write the userblock
	int res = HDwrite(FileHandle, Buffer, Size);
	if(res<=0)
		return -1;

	// Jump back to the old place in the file	
	if(file_seek(FileHandle, (file_offset_t)(old_pos), SEEK_SET) < 0)
	{
		m_Callback->OnH5ToLog(H5E_IO, L"unable to seek to proper position"); 
		return FAIL;
	}
	return 0;
}
int BlockDriver::DoReadUserBlock(void * Buffer, unsigned int Size)
{
	if(m_Callback == nullptr)
		return 0;
	return m_Callback->OnH5ReadUserBlock(Buffer, Size);
}
int BlockDriver::DoBlockRead(int FileHandle, void * Buffer, unsigned int Size)
{	
	int res = HDread(FileHandle, Buffer, Size);
	if(res<0)
		return res;
	if(m_Callback == nullptr)
		return res;
	
	int remaining_bytes = Size;
	int pos = 0;
	int ChunkSize = (m_BlockSize>0)?m_BlockSize:Size;
	do
	{
		pos = (Size - remaining_bytes);
		if(m_Callback->OnH5AfterBlockRead((char*)Buffer + pos, ChunkSize)<0)
			return -1;
		remaining_bytes-=ChunkSize;
	}
	while(remaining_bytes>0);
	return 0;
}
int BlockDriver::DoBlockWrite(int FileHandle, void * Buffer, unsigned int Size)
{
	int res = 0;
	if(m_Callback != nullptr)
	{			
		int remaining_bytes = Size;
		int pos = 0;
		int ChunkSize = (m_BlockSize>0)?m_BlockSize:Size;
		do
		{
			pos = (Size - remaining_bytes);
			if(m_Callback->OnH5BeforeBlockWrite((char*)Buffer + pos, ChunkSize)<0)
				return -1;
			remaining_bytes-=ChunkSize;
		}
		while(remaining_bytes>0);
	}
	res = HDwrite(FileHandle, Buffer, Size);
	if(res<=0)
		return res; 
	return res;
}

////////////////////////////////////////////////////////////////
// Description:  
//      Returns a file access property list which indicates how the
//      specified file is being accessed. The return list could be
//      used to access another file the same way.
// Return: 
//      Success:  Ptr to new file access property list with all
//      members copied from the file struct.
//      Failure:  NULL
////////////////////////////////////////////////////////////////
void* BlockDriver::fapl_get(H5FD_t *_file)
{
	FileHandle_t  *file = (FileHandle_t*)_file;
	void *ret_value;    // Return value

	// FUNC_ENTER_NOAPI_NOINIT

	// Set return value
	ret_value= fapl_copy(&(file->fa));


	return (ret_value);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Copies the crypto-specific file access properties.
// Return: 
//      Success:  Ptr to a new property list
//      Failure:  NULL
////////////////////////////////////////////////////////////////
void* BlockDriver::fapl_copy(const void *_old_fa)
{
	FAPL_t *old_fa = (FAPL_t*)_old_fa;
	FAPL_t *new_fa = (FAPL_t*)HDmalloc(sizeof(FAPL_t));

	//// FUNC_ENTER_NOAPI_NOINIT_NOERR

	assert(new_fa);

	/* Copy the general information */
	HDmemcpy(new_fa, old_fa, sizeof(FAPL_t));

	//return (new_fa)
	return new_fa;
}
////////////////////////////////////////////////////////////////
// Description:  
//       Create and/or opens a Unix file for crypto I/O as an HDF5 file.
// Return: 
//      Success:  A pointer to a new file data structure. The
//      public fields will be initialized by the
//      caller, which is always H5FD_open().
//      Failure:  NULL
////////////////////////////////////////////////////////////////
H5FD_t* BlockDriver::open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
	int      o_flags;
	int      fd=(-1);
	FileHandle_t  *file=NULL;
	FAPL_t  *fa;
#ifdef H5_HAVE_WIN32_API
	intptr_t     filehandle;
	struct _BY_HANDLE_FILE_INFORMATION fileinfo;
#endif
	h5_stat_t    sb;
	H5P_genplist_t   *plist;      /* Property list */

	H5FD_t    *ret_value;


	// Sanity check on file offsets
	assert(sizeof(file_offset_t)>=sizeof(size_t));

	// Get the driver specific information
	if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
	{
		wprintf(L"not a file access property list"); 
		return NULL;
	}
	fa = (FAPL_t*)H5P_get_driver_info(plist);

	/* Check arguments */
	if (!name || !*name)
	{
		fa->drv->m_Callback->OnH5ToLog(H5E_BADVALUE, L"invalid file name"); 
		return NULL;
	}
	if (0==maxaddr || HADDR_UNDEF==maxaddr)
	{
		fa->drv->m_Callback->OnH5ToLog(H5E_BADRANGE, L"bogus maxaddr"); 
		return NULL;
	}
	if (XHDF5_ADDR_OVERFLOW(maxaddr))
	{
		fa->drv->m_Callback->OnH5ToLog(H5E_OVERFLOW, L"bogus maxaddr"); 
		return NULL;
	}

	// Build the open flags
	o_flags = (H5F_ACC_RDWR & flags) ? O_RDWR : O_RDONLY;
	if (H5F_ACC_TRUNC & flags) o_flags |= O_TRUNC;
	if (H5F_ACC_CREAT & flags) o_flags |= O_CREAT;
	if (H5F_ACC_EXCL & flags) o_flags |= O_EXCL;

	// Define if the file is being created
	bool IsCreate = (H5F_ACC_EXCL & flags || H5F_ACC_TRUNC & flags || H5F_ACC_CREAT & flags) ;

	// Open the file
	if ((fd=HDopen(name, o_flags, 0666))<0)
	{
		fa->drv->m_Callback->OnH5ToLog(H5E_CANTOPENFILE, L"unable to open file"); 
		return NULL;
	}

	if (HDfstat(fd, &sb)<0)
	{
		fa->drv->m_Callback->OnH5ToLog(H5E_BADFILE, L"unable to fstat file"); 
		return NULL;
	}

	// Check if the file is just being created
	if(IsCreate)
	{
		// Allocate a buffer for use block
		uint32_t UserBlockSize = fa->ubsize;
		void* UserBlockBuffer = HDmalloc(UserBlockSize);
		if(UserBlockBuffer == nullptr)
		{
			fa->drv->m_Callback->OnH5ToLog(H5E_NOSPACE, L"unable to allocate user block"); 
			return NULL;
		}

		// Let the callback fill it
		if(fa->drv->DoWriteUserBlock(fd, UserBlockBuffer, UserBlockSize)!=0)
		{
			HDfree(UserBlockBuffer);
			fa->drv->m_Callback->OnH5ToLog(H5E_NOSPACE, L"callback was unable to make user block"); 
			return NULL;
		}

		// Free user block
		HDfree(UserBlockBuffer);
		
	}
	// Now let's pretend that we are opening the old file, even if just created
	// we need to reinitialize encryption
	
	// First thing is to read the userblock
	// Allocate a buffer for use block
	uint32_t UserBlockSize = fa->ubsize;
	void* UserBlockBuffer = HDmalloc(UserBlockSize);
	if(UserBlockBuffer == nullptr)
	{
		fa->drv->m_Callback->OnH5ToLog(H5E_NOSPACE, L"unable to allocate user block"); 
		return NULL;
	}
	// Read it from the file
	int res = HDread(fd, UserBlockBuffer, UserBlockSize);
	if(res<=0)
	{
		HDfree(UserBlockBuffer);
		fa->drv->m_Callback->OnH5ToLog(H5E_NOSPACE, L"failed to read user block"); 
		return NULL; 
	}

	// Ask the callback to check it
	if(fa->drv->DoReadUserBlock(UserBlockBuffer, UserBlockSize)!=0)
	{
		HDfree(UserBlockBuffer);
		return NULL;
	}
	// Free user block
	HDfree(UserBlockBuffer);
	

	// Create the new file struct
	file = (FileHandle_t*)HDmalloc(sizeof(FileHandle_t));
	if (file == NULL)
	{
		fa->drv->m_Callback->OnH5ToLog(H5E_NOSPACE, L"unable to allocate file struct"); 
		return NULL;
	}
	ZeroMemory(file, sizeof(FileHandle_t));


	file->fd = fd;
	H5_ASSIGN_OVERFLOW(file->eof,sb.st_size,h5_stat_size_t,haddr_t);
	file->pos = HADDR_UNDEF;
	file->op = OP_UNKNOWN;
#ifdef H5_HAVE_WIN32_API
	filehandle = _get_osfhandle(fd);
	(void)GetFileInformationByHandle((HANDLE)filehandle, &fileinfo);
	file->fileindexhi = fileinfo.nFileIndexHigh;
	file->fileindexlo = fileinfo.nFileIndexLow;
#else
	file->device = sb.st_dev;
#ifdef H5_VMS
	file->inode[0] = sb.st_ino[0];
	file->inode[1] = sb.st_ino[1];
	file->inode[2] = sb.st_ino[2];
#else
	file->inode = sb.st_ino;
#endif /*H5_VMS*/
#endif /*H5_HAVE_WIN32_API*/
	file->fa.ubsize = fa->ubsize;
	file->fa.fbsize = fa->fbsize;
	file->fa.cbsize = fa->cbsize;
	file->fa.drv    = fa->drv;

	/* Try to decide if the fiel can be opened.
	*/
	// buf1 = (int*)HDmalloc(sizeof(int));
	// HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, NULL, "HDposix_memalign failed")
	//if(buf1) HDfree(buf1);

	// Set return value
	ret_value=(H5FD_t*)file;
	fa->drv->m_File = file;

//done:
	if(ret_value==NULL) 
	{
		if(fd>=0)
			HDclose(fd);
	}

	return (ret_value);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Closes the file.
// Return: 
//      Success:  0
//      Failure:  -1, file not closed.
////////////////////////////////////////////////////////////////
herr_t BlockDriver::close(H5FD_t *_file)
{
	FileHandle_t  *file     = (FileHandle_t*)_file;
	herr_t        ret_value = SUCCEED;       /* Return value */

	// FUNC_ENTER_NOAPI_NOINIT

	if (HDclose(file->fd)<0)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_CANTCLOSEFILE, L"unable to close file"); 
		return FAIL;
	}
	
	HDfree(file);

//done:
	return (ret_value);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Compares two files belonging to this driver using an
//       arbitrary (but consistent) ordering.
// Return: 
//      Success:  A value like strcmp()
//      Failure:  never fails (arguments were checked by the caller).
////////////////////////////////////////////////////////////////
int BlockDriver::cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
	const FileHandle_t  *f1 = (const FileHandle_t*)_f1;
	const FileHandle_t  *f2 = (const FileHandle_t*)_f2;
	int ret_value=0;

	// FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef H5_HAVE_WIN32_API
	if (f1->fileindexhi < f2->fileindexhi) HGOTO_DONE(-1)
	if (f1->fileindexhi > f2->fileindexhi) HGOTO_DONE(1)
	if (f1->fileindexlo < f2->fileindexlo) HGOTO_DONE(-1)
	if (f1->fileindexlo > f2->fileindexlo) HGOTO_DONE(1)

#else
#ifdef H5_DEV_T_IS_SCALAR
		if (f1->device < f2->device) HGOTO_DONE(-1)
			if (f1->device > f2->device) HGOTO_DONE(1)
#else /* H5_DEV_T_IS_SCALAR */
		/* If dev_t isn't a scalar value on this system, just use memcmp to
		* determine if the values are the same or not.  The actual return value
		* shouldn't really matter...
		*/
		if(HDmemcmp(&(f1->device),&(f2->device),sizeof(dev_t))<0) HGOTO_DONE(-1)
			if(HDmemcmp(&(f1->device),&(f2->device),sizeof(dev_t))>0) HGOTO_DONE(1)
#endif /* H5_DEV_T_IS_SCALAR */

#ifndef H5_VMS
				if (f1->inode < f2->inode) HGOTO_DONE(-1)
					if (f1->inode > f2->inode) HGOTO_DONE(1)
#else
				if(HDmemcmp(&(f1->inode),&(f2->inode),3*sizeof(ino_t))<0) HGOTO_DONE(-1)
					if(HDmemcmp(&(f1->inode),&(f2->inode),3*sizeof(ino_t))>0) HGOTO_DONE(1)
#endif /*H5_VMS*/

#endif

done:
	return (ret_value);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Set the flags that this VFL driver is capable of supporting.
//       (listed in H5FDpublic.h)
// Return: 
//      Success:  non-negative
//      Failure:  negative
////////////////////////////////////////////////////////////////
herr_t BlockDriver::query(const H5FD_t UNUSED * _f, unsigned long *flags /* out */)
{
	// Set the VFL feature flags that this driver supports
	if(flags) 
	{
		*flags = 0;
		*flags|=H5FD_FEAT_AGGREGATE_METADATA;  // OK to aggregate metadata allocations */
		*flags|=H5FD_FEAT_ACCUMULATE_METADATA; // OK to accumulate metadata for faster writes */
		*flags|=H5FD_FEAT_DATA_SIEVE;          // OK to perform data sieving for faster raw data reads & writes */
		*flags|=H5FD_FEAT_AGGREGATE_SMALLDATA; // OK to aggregate "small" raw data allocations */
	}

	return SUCCEED;
}
////////////////////////////////////////////////////////////////
// Description:  
//       Gets the end-of-address marker for the file. The EOA marker
//       is the first address past the last byte allocated in the
//       format address space.
// Return: 
//      Success:  The end-of-address marker.
//      Failure:  HADDR_UNDEF
////////////////////////////////////////////////////////////////
haddr_t BlockDriver::get_eoa(const H5FD_t *_file, H5FD_mem_t UNUSED type)
{
	const FileHandle_t  *file = (const FileHandle_t*)_file;

	// FUNC_ENTER_NOAPI_NOINIT_NOERR

	return (file->eoa);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Set the end-of-address marker for the file. This function is
//       called shortly after an existing HDF5 file is opened in order
//       to tell the driver where the end of the HDF5 data is located.
// Return: 
//      Success:  0
//      Failure:  -1
////////////////////////////////////////////////////////////////
herr_t BlockDriver::set_eoa(H5FD_t *_file, H5FD_mem_t UNUSED type, haddr_t addr)
{
	FileHandle_t  *file = (FileHandle_t*)_file;

	// FUNC_ENTER_NOAPI_NOINIT_NOERR

	file->eoa = addr;

	return (SUCCEED);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Returns the end-of-file marker, which is the greater of
//       either the Unix end-of-file or the HDF5 end-of-address
//       markers.
// Return: 
//       Success:  End of file address, the first address past
//       the end of the "file", either the Unix file
//       or the HDF5 file.
//      Failure:  HADDR_UNDEF
////////////////////////////////////////////////////////////////
haddr_t BlockDriver::get_eof(const H5FD_t *_file)
{
	const FileHandle_t  *file = (const FileHandle_t*)_file;

	// FUNC_ENTER_NOAPI_NOINIT

	return (MAX(file->eof, file->eoa));
}
////////////////////////////////////////////////////////////////
// Description:  
//       Returns the file handle of crypto file driver.
// Return: 
//       Non-negative if succeed or negative if fails.
////////////////////////////////////////////////////////////////
herr_t BlockDriver::get_handle(H5FD_t *_file, hid_t UNUSED fapl, void** file_handle)
{
	FileHandle_t       *file = (FileHandle_t *)_file;
	herr_t              ret_value = SUCCEED;

	// FUNC_ENTER_NOAPI_NOINIT

	if(!file_handle)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_BADVALUE, L"file handle not valid"); 
		return FAIL;
	}
	*file_handle = &(file->fd);

//done:
	return (ret_value);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Reads SIZE bytes of data from FILE beginning at address ADDR
//       into buffer BUF according to data transfer properties in
//       DXPL_ID.
// Return: 
//       Success:  Zero. Result is stored in caller-supplied buffer BUF.
//       Failure:  -1, Contents of buffer BUF are undefined.
////////////////////////////////////////////////////////////////
herr_t BlockDriver::read(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr, size_t size, void *buf/*out*/)
{
	FileHandle_t* file = (FileHandle_t*)_file;
	ssize_t        nbytes;
	herr_t         ret_value=SUCCEED;       // Return value 
	size_t         alloc_size;
	void*          copy_buf = NULL, *p2;
	size_t         _fbsize;
	size_t         _cbsize;
	haddr_t        read_size;              // Size to read into copy buffer 
	size_t         copy_size = size;       // Size remaining to read when using copy buffer 
	size_t         copy_offset;            // Offset into copy buffer of the requested data 

	// FUNC_ENTER_NOAPI_NOINIT

	assert(file && file->pub.cls);
	assert(buf);

	// Check for overflow conditions
	if (HADDR_UNDEF==addr)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_BADVALUE, L"addr undefined"); 
		return FAIL;
	}
	if (XHDF5_REGION_OVERFLOW(addr, size))
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_OVERFLOW, L"addr overflow"); 
		return FAIL;
	}
	if((addr + size) > file->eoa)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_OVERFLOW, L"addr overflow"); 
		return FAIL;
	}


	// Get the memory boundary for alignment, file system block size, and maximalcopy buffer size.
	_fbsize = file->fa.fbsize;
	_cbsize = file->fa.cbsize;
	
	// Calculate where we will begin copying from the copy buffer
	copy_offset = (size_t)(addr % _fbsize);

	// Allocate memory needed for the Direct IO option up to the maximal
	// copy buffer size. Make a bigger buffer for aligned I/O if size is
	// smaller than maximal copy buffer. 
	alloc_size = ((copy_offset + size - 1) / _fbsize + 1) * _fbsize;
	if(alloc_size > _cbsize)
		alloc_size = _cbsize;
	HDassert(!(alloc_size % _fbsize));

	copy_buf = HDmalloc(alloc_size);
	//if (HDposix_memalign(&copy_buf, _boundary, alloc_size) != 0)
	if (copy_buf == NULL)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_RESOURCE, L"posix_memalign no memory"); 
		return FAIL;
	}

	// look for the aligned position for reading the data 
	HDassert(!(((addr / _fbsize) * _fbsize) % _fbsize));

	if(file_seek(file->fd, (file_offset_t)((addr / _fbsize) * _fbsize), SEEK_SET) < 0)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_IO, L"unable to seek to proper position"); 
		return FAIL;
	}


	// Read the aligned data in file into aligned buffer first, then copy the data
	// into the final buffer.  If the data size is bigger than maximal copy buffer
	// size, do the reading by segment (the outer while loop).  If not, do one step
	// reading.
	do 
	{
		// Read the aligned data in file first.  Not able to handle interrupted
		// system calls and partial results like sec2 driver does because the
		// data may no longer be aligned. It's expecially true when the data in
		// file is smaller than ALLOC_SIZE.
		HDmemset(copy_buf, 0, alloc_size);

		// Calculate how much data we have to read in this iteration (including unused parts of blocks)
		if((copy_size + copy_offset) < alloc_size)
			read_size = ((copy_size + copy_offset - 1) / _fbsize + 1) * _fbsize;
		else
			read_size = alloc_size;

		HDassert(!(read_size % _fbsize));
		do 
		{
			nbytes = file->fa.drv->DoBlockRead(file->fd, copy_buf, (unsigned int)read_size);
		}
		while(-1==nbytes && EINTR==errno);

		if (-1==nbytes) /* error */
		{
			file->fa.drv->m_Callback->OnH5ToLog(H5E_IO, L"file read failed"); 
			return FAIL;
		}

		// Copy the needed data from the copy buffer to the output
		// buffer, and update copy_size.  If the copy buffer does not
		// contain the rest of the data, just copy what's in the copy
		// buffer and also update read_addr and copy_offset to read the
		// next section of data.
		p2 = (unsigned char*)copy_buf + copy_offset;
		if((copy_size + copy_offset) <= alloc_size) 
		{
			HDmemcpy(buf, p2, copy_size);
			buf = (unsigned char *)buf + copy_size;
			copy_size = 0;
		}
		else 
		{
			HDmemcpy(buf, p2, alloc_size - copy_offset);
			buf = (unsigned char*)buf + alloc_size - copy_offset;
			copy_size -= alloc_size - copy_offset;
			copy_offset = 0;
		}
	} 
	while (copy_size > 0);

	// Final step: update address
	addr = (haddr_t)(((addr + size - 1) / _fbsize + 1) * _fbsize);

	if(copy_buf) 
	{
		HDfree(copy_buf);
		copy_buf = NULL;
	} 
	

	// Update current position 
	file->pos = addr;
	file->op = OP_READ;

//done:
	if(ret_value<0) 
	{
		if(copy_buf)
			HDfree(copy_buf);

		// Reset last file I/O information
		file->pos = HADDR_UNDEF;
		file->op = OP_UNKNOWN;
	} 

	return (ret_value);
}
////////////////////////////////////////////////////////////////
// Description:  
//       Writes SIZE bytes of data to FILE beginning at address ADDR
//       from buffer BUF according to data transfer properties in
//       DXPL_ID.
// Return: 
//       Success:  Zero
//       Failure:  -1
////////////////////////////////////////////////////////////////
herr_t BlockDriver::write(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr, size_t size, const void *buf)
{
	FileHandle_t* file = (FileHandle_t*)_file;
	ssize_t       nbytes;
	herr_t        ret_value=SUCCEED;      // Return value */
	size_t        alloc_size;
	void*         copy_buf = NULL, *p1;
	const void*   p3;
	size_t        _fbsize;
	size_t        _cbsize;
	haddr_t       write_addr;             // Address to write copy buffer
	haddr_t       write_size;             // Size to write from copy buffer
	haddr_t       read_size;              // Size to read into copy buffer
	size_t        copy_size = size;       // Size remaining to write when using copy buffer
	size_t        copy_offset;            // Offset into copy buffer of the data to write

	// FUNC_ENTER_NOAPI_NOINIT

	assert(file && file->pub.cls);
	assert(buf);

	// Check for overflow conditions
	if (HADDR_UNDEF==addr)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_BADVALUE, L"addr undefined"); 
		return FAIL;
	}
	if (XHDF5_REGION_OVERFLOW(addr, size))
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_OVERFLOW, L"addr overflow"); 
		return FAIL;
	}
	if (addr+size>file->eoa)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_OVERFLOW, L"addr overflow");  
		return FAIL;
	}

	// Get the memory boundary for alignment, file system block size, and maximal copy buffer size.
	_fbsize = file->fa.fbsize;
	_cbsize = file->fa.cbsize;
 
	
	// Calculate where we will begin reading from (on disk) and where we
	// will begin copying from the copy buffer
	write_addr = (addr / _fbsize) * _fbsize;
	copy_offset = (size_t)(addr % _fbsize);

	// allocate memory needed for the Direct IO option up to the maximal
	// copy buffer size. Make a bigger buffer for aligned I/O if size is
	// smaller than maximal copy buffer.
	alloc_size = ((copy_offset + size - 1) / _fbsize + 1) * _fbsize;
	if(alloc_size > _cbsize)
		alloc_size = _cbsize;
	HDassert(!(alloc_size % _fbsize));

	copy_buf = HDmalloc(alloc_size);
	//if (HDposix_memalign(&copy_buf, _boundary, alloc_size) != 0)
	if (copy_buf == NULL)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_CANTALLOC, L"malloc failed");  
		return FAIL;
	}
	// Ask the callback to fill it 
	file->fa.drv->DoFillEmptyBlock(copy_buf, alloc_size);


	// if we have moved beyond the file's eof fill the gap with garbage
	/*if(write_addr>file->eof)
	{
		// jump to eof
		if(file_seek(file->fd, 0, SEEK_END) < 0)
		{
			file->fa.drv->m_Callback->OnH5ToLog(H5E_SEEKERROR, L"unable to seek to eof position");  
			return FAIL;
		}
		// prepare junk blob
		size_t JunkSize = write_addr - file->eof;
		char * JunkMem = (char *)HDmalloc(JunkSize);
		if (JunkMem == NULL)
		{
			file->fa.drv->m_Callback->OnH5ToLog(H5E_CANTALLOC, L"malloc failed");  
			return FAIL;
		}
		// Ask the callback to fill it 
		file->fa.drv->DoFillEmptyBlock(JunkMem, JunkSize);

		// write junk
		int res = file->fa.drv->DoBlockWrite(file->fd, JunkMem, JunkSize);
			//HDwrite(file->fd, JunkMem, JunkSize);
		if(res<=0)
			return res; 

		// free mem
		HDfree(JunkMem);
	}*/

	// look for the right position for reading or writing the data 
	if(file_seek(file->fd, (file_offset_t)write_addr, SEEK_SET) < 0)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_SEEKERROR, L"unable to seek to proper position");  
		return FAIL;
	}

	p3 = buf;

	do 
	{
		// Calculate how much data we have to write in this iteration
		// (including unused parts of blocks) 
		if((copy_size + copy_offset) < alloc_size)
			write_size = ((copy_size + copy_offset - 1) / _fbsize + 1) * _fbsize;
		else
			write_size = alloc_size;

		// Read the aligned data first if the aligned region doesn't fall
		// entirely in the range to be writen.  Not able to handle interrupted
		// system calls and partial results like sec2 driver does because the
		// data may no longer be aligned. It's expecially true when the data in
		// file is smaller than ALLOC_SIZE.  Only read the entire section if
		// both ends are misaligned, otherwise only read the block on the
		// misaligned end.
		HDmemset(copy_buf, 0, _fbsize);

		if(copy_offset > 0) 
		{
			if((write_addr + write_size) > (addr + size)) 
			{
				HDassert((write_addr + write_size) - (addr + size) < _fbsize);
				read_size = write_size;
				p1 = copy_buf;
			} 
			else 
			{
				read_size = _fbsize;
				p1 = copy_buf;
			} 
		}
		else if((write_addr + write_size) > (addr + size)) 
		{
			HDassert((write_addr + write_size) - (addr + size) < _fbsize);
			read_size = _fbsize;
			p1 = (unsigned char *)copy_buf + write_size - _fbsize;

			// Seek to the last block, for reading
			HDassert(!((write_addr + write_size - _fbsize) % _fbsize));
			if(file_seek(file->fd, (file_offset_t)(write_addr + write_size - _fbsize), SEEK_SET) < 0)
			{
				file->fa.drv->m_Callback->OnH5ToLog(H5E_SEEKERROR, L"unable to seek to proper position");  
				return FAIL;
			}
		}
		else
			p1 = NULL;

		if(p1) 
		{
			HDassert(!(read_size % _fbsize));
			do 
			{
				//nbytes = read(file->fd, p1, read_size);
				nbytes = file->fa.drv->DoBlockRead(file->fd, p1, (unsigned int)read_size);
			} 
			while (-1==nbytes && EINTR==errno);

			if (-1==nbytes)
			{
				file->fa.drv->m_Callback->OnH5ToLog(H5E_READERROR, L"file read failed");  
				return FAIL;
			}
		}

		// look for the right position and append or copy the data to be written to
		// the aligned buffer.
		// Consider all possible situations here: file address is not aligned on
		// file block size; the end of data address is not aligned; the end of data
		// address is aligned; data size is smaller or bigger than maximal copy size.
		p1 = (unsigned char *)copy_buf + copy_offset;
		if((copy_size + copy_offset) <= alloc_size) 
		{
			HDmemcpy(p1, p3, copy_size);
			copy_size = 0;
		}
		else 
		{
			HDmemcpy(p1, p3, alloc_size - copy_offset);
			p3 = (const unsigned char *)p3 + (alloc_size - copy_offset);
			copy_size -= alloc_size - copy_offset;
			copy_offset = 0;
		}

		// look for the aligned position for writing the data
		HDassert(!(write_addr % _fbsize));
		if(file_seek(file->fd, (file_offset_t)write_addr, SEEK_SET) < 0)
		{
			file->fa.drv->m_Callback->OnH5ToLog(H5E_SEEKERROR, L"unable to seek to proper position");  
			return FAIL;
		}

		// Write the data. It doesn't truncate the extra data introduced by
		// alignment because that step is done in H5FD_crypto_flush.
		HDassert(!(write_size % _fbsize));
		do 
		{
			nbytes = file->fa.drv->DoBlockWrite(file->fd, copy_buf, (unsigned int)write_size);
		} 
		while (-1==nbytes && EINTR==errno);

		if (-1==nbytes)
		{
			file->fa.drv->m_Callback->OnH5ToLog(H5E_WRITEERROR, L"file write failed");  
			return FAIL;
		}

		// update the write address
		write_addr += write_size;
	} 
	while (copy_size > 0);

	// Update the address and size
	addr = write_addr;
	buf = (const char*)buf + size;

	if(copy_buf) 
	{
		HDfree(copy_buf);
		copy_buf = NULL;
	}
	

	// Update current position and eof
	file->pos = addr;
	file->op = OP_WRITE;
	if (file->pos>file->eof)
		file->eof = file->pos;


	if(ret_value<0) 
	{
		if(copy_buf)
			HDfree(copy_buf);

		// Reset last file I/O information
		file->pos = HADDR_UNDEF;
		file->op = OP_UNKNOWN;
	}

	return (ret_value);
}
////////////////////////////////////////////////////////////////
// Description:  
//      Makes sure that the true file size is the same (or larger)
//      than the end-of-address.
// Return: 
//       Success:  Non-negative
//       Failure:  Negative
////////////////////////////////////////////////////////////////
herr_t BlockDriver::truncate(H5FD_t *_file, hid_t UNUSED dxpl_id, hbool_t UNUSED closing)
{
	FileHandle_t  *file = (FileHandle_t*)_file;
	herr_t        ret_value = SUCCEED;       /* Return value */

	// FUNC_ENTER_NOAPI_NOINIT

	assert(file);

	// Extend the file to make sure it's large enough

	//if (file->eoa!=file->eof) 
	{
#ifdef H5_HAVE_WIN32_API
		intptr_t filehandle;   /* Windows file handle */
		LARGE_INTEGER li;   /* 64-bit integer for SetFilePointer() call */

		/* Map the posix file handle to a Windows file handle */
		filehandle = _get_osfhandle(file->fd);

		/* Translate 64-bit integers into form Windows wants */
		/* [This algorithm is from the Windows documentation for SetFilePointer()] */
		size_t RoundedSize = ceil((double)file->eoa / file->fa.fbsize) * file->fa.fbsize;
		li.QuadPart = RoundedSize;//(LONGLONG)file->eoa;
		(void)SetFilePointer((HANDLE)filehandle,li.LowPart,&li.HighPart,FILE_BEGIN);
		if(SetEndOfFile((HANDLE)filehandle)==0)
		{
			file->fa.drv->m_Callback->OnH5ToLog(H5E_SEEKERROR, L"unable to extend file properly");  
			return FAIL;
		}
#else
		if (-1==file_truncate(file->fd, (file_offset_t)file->eoa))
			HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to extend file properly")
#endif

		// Update the eof value
		file->eof = file->eoa;

		// Reset last file I/O information
		file->pos = HADDR_UNDEF;
		file->op = OP_UNKNOWN;
	}
	//else if (file->fa.must_align)
	//{
	//	/*Even though eof is equal to eoa, file is still truncated because Direct I/O
	//	*write introduces some extra data for alignment.
	//	*/
	//	if (-1==file_truncate(file->fd, (file_offset_t)file->eof))
	//		HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to extend file properly")
	//}

//done:
	return (ret_value);
} 
herr_t BlockDriver::flush(H5FD_t *_file, hid_t dxpl_id, hbool_t closing)
{
	FileHandle_t  *file = (FileHandle_t*)_file;
	intptr_t filehandle = _get_osfhandle(file->fd);
	BOOL bRes = FlushFileBuffers((HANDLE)filehandle);
	return (!bRes)?(FAIL):(SUCCEED);
}
haddr_t BlockDriver::alloc(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, hsize_t size)
{
	FileHandle_t  *file = (FileHandle_t*)_file;    
	haddr_t addr;
    haddr_t ret_value;          // Return value

	// Compute the address for the block to allocate
    addr = file->eoa;

	// Set return value
    ret_value = addr;

	// Check if we need to align this block
    if(size >= file->pub.threshold) 
	{
        // Check for an already aligned block
        if(addr % file->pub.alignment != 0)
            addr = ((addr / file->pub.alignment) + 1) * file->pub.alignment;
    }
	set_eoa(_file, type, addr + size);
	// Get the current fileposition
	file_offset_t old_pos = file_tell(file->fd);

	// Allocate mem first
	// prepare junk blob
	size_t JunkSize = size;
	char * JunkMem = (char *)HDmalloc(JunkSize);
	if (JunkMem == NULL)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_CANTALLOC, L"malloc failed");  
		return FAIL;
	}
	// Ask the callback to fill it 
	file->fa.drv->DoFillEmptyBlock(JunkMem, JunkSize);

	// Jump to the right place in the file	
	if(file_seek(file->fd, (file_offset_t)(addr), SEEK_SET) < 0)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_IO, L"unable to seek to proper position"); 
		return FAIL;
	}

	// write junk
	int res = HDwrite(file->fd, JunkMem, JunkSize);
	if(res<=0)
		return HADDR_UNDEF; 

	// free mem
	HDfree(JunkMem);

	// Jump back to the old place in the file	
	if(file_seek(file->fd, (file_offset_t)(old_pos), SEEK_SET) < 0)
	{
		file->fa.drv->m_Callback->OnH5ToLog(H5E_IO, L"unable to seek to proper position"); 
		return FAIL;
	}


	return ret_value;
}
}
