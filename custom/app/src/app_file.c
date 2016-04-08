/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_file.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
#ifdef __CUSTOMER_CODE__
#include "string.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_fs.h"
#include "ql_error.h"
#include "ql_system.h"
#include "ql_stdlib.h"
#include "app_common.h"
#include "app_file.h"
#if 0
s32 open_file(char *fileName)
{   
    s32 ret = -1;
    s32 handle = -1;
    u32 writenLen = 0;
    u32 readenLen = 0;
    s32 filesize = 0;
    s32 position = 0;    
    bool isdir = FALSE;
    
    u8 strBuf[LENGTH] = {0};
    u8 filePath[LENGTH] = {0};
    u8 fileNewPath[LENGTH] = {0};
    u8 writeBuffer[LENGTH] = {"This is test data!"};

	Enum_FSStorage storage = Ql_FS_UFS;

    //open file if file does not exist ,creat it
    handle = Ql_FS_Open(fileName,QL_FS_READ_WRITE | QL_FS_CREATE_ALWAYS );

    return handle;
    
}

bool have_data_in_file(char *fileName)
{
	s32 ret,filesize;
	ret = Ql_FS_Seek(handle,0,QL_FS_FILE_BEGIN);
	if(ret != QL_RET_OK)
	{
		APP_ERROR("\r\n<--!! Ql_FS_Seek error ret =%d-->\r\n",ret); 
	}

	filesize = Ql_FS_GetSize(fileName);

	if(filesize > 10)
		return TRUE;
	else
		return FALSE;
}

s32 get_an_item(s32 handle, u8 *buffer)
{
	s32 ret;
	
	u32 readenLen = 0;
	u32 read_p,write_p;

	u8 total_item;
	u8 strBuf[150];
	s32 Seek_offset = 0;
	s32 readLen = 0;
	
	ret = Ql_FS_Seek(handle,Seek_offset,QL_FS_FILE_BEGIN);
	if(ret != QL_RET_OK)
	{
		APP_ERROR("\r\n<--!! Ql_FS_Seek(%d) error ret =%d-->\r\n",ret,Seek_offset);
	}

    readLen = 12;
	ret = Ql_FS_Read(handle, strBuf, readLen, &readenLen);
	if(ret != QL_RET_OK || readenLen != readLen)
	{
		APP_ERROR("\r\n<--!! Ql_FS_Read(%d) error ret =%d-->\r\n",ret,readLen);
	}
	
	total_item = strBuf[0];
	Ql_memcpy(read_p, strBuf+1, 4);
	//Ql_memcpy(write_p, strBuf+5, 4);

	if(total_item == 0)
	{
		return 0;
	}

	Seek_offset = read_p;
	ret = Ql_FS_Seek(handle,0,QL_FS_FILE_BEGIN);
	if(ret != QL_RET_OK)
	{
		APP_ERROR("\r\n<--!! Ql_FS_Seek error ret =%d-->\r\n",ret);
	}
	ret = Ql_FS_Read(handle, strBuf, 150, &readenLen);
	if(ret != QL_RET_OK)
	{
		APP_ERROR("\r\n<--!! Ql_FS_Read error ret =%d-->\r\n",ret);
	}

	
	
}

{
	//write file
    ret = Ql_FS_Write(handle, writeBuffer, Ql_strlen(writeBuffer), &writenLen);
    APP_DEBUG("\r\n<--!! Ql_FS_Write  ret =%d  writenLen =%d-->\r\n",ret,writenLen);
    //fflush
     Ql_FS_Flush(handle);      
    //seek 
    ret = Ql_FS_Seek(handle,5,QL_FS_FILE_BEGIN);
    APP_DEBUG("\r\n<--!! Ql_FS_Seek   ret =%d-->\r\n",ret);    
    
    //APP_DEBUG("\r\n<--!! Ql_FS_GetFilePosition   position =%d-->\r\n",position);
    //truncate 
    //ret = Ql_FS_Truncate(handle);
    //seek begin 
    //ret = Ql_FS_Seek(handle,0,QL_FS_FILE_BEGIN);    
    //read file
    ret = Ql_FS_Read(handle, strBuf, LENGTH, &readenLen);
    APP_DEBUG("\r\n<-- Ql_FS_Read() ret=%d: readedlen=%d, readBuffer=%s-->\r\n", ret, readenLen, strBuf);
    //close file
    Ql_FS_Close(handle);    
    
    //get file size
    filesize = Ql_FS_GetSize(filePath);
    APP_DEBUG("\r\n<-- Ql_FS_GetSize() filesize=%d: -->\r\n",filesize);
    //rename file
    //Ql_sprintf(fileNewPath,"%s\\%s\0",PATH_ROOT,fileNewName);    
    //ret = Ql_FS_Rename(filePath, fileNewPath);
    //APP_DEBUG("\r\n<-- Ql_FS_Rename() ret=%d: -->\r\n",ret);

    //find dir files
    Ql_memset(filePath, 0, LENGTH);        
    Ql_memset(strBuf, 0, LENGTH);    
    Ql_sprintf(filePath,"%s\\%s\0",PATH_ROOT,"*");        
    handle  = Ql_FS_FindFirst(filePath,strBuf,LENGTH,&filesize,&isdir);
    if(handle > 0)
    {
        do
        {
            //list find files
            APP_DEBUG("\r\n<-- ql_fs_find filename= %s filesize=%d: -->\r\n",strBuf,filesize);
            Ql_memset(strBuf, 0, LENGTH);
        }while((Ql_FS_FindNext(handle, strBuf,LENGTH,&filesize,&isdir)) == QL_RET_OK);
        Ql_FS_FindClose(handle);
    }
    
    //delete file
    //ret = Ql_FS_Delete(fileNewPath);
    //APP_DEBUG("\r\n<-- Ql_FS_Delete() ret=%d: -->\r\n",ret);   

#ifndef __TEST_FOR_RAM_FILE__	
    //delet dir
    //ret = Ql_FS_DeleteDir(PATH_ROOT);
    //APP_DEBUG("\r\n<-- Ql_FS_DeleteDir() ret=%d: -->\r\n",ret);   
#endif
    
    while (TRUE)
    {
         Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
        case 0:
            break;
        default:
            break;
        }
        
    }

}
#endif
#endif // __EXAMPLE_FILESYSTEM__
