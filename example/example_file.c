/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   example_file.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   This example demonstrates how to use file function with APIs in OpenCPU.
 *   All debug information will be output  through DEBUG port.
 * Usage:
 * ------
 *   Compile & Run:
 *
 *     Set "C_PREDEF=-D __EXAMPLE_FILESYSTEM__" in gcc_makefile file. And compile the 
 *     app using "make clean/new".
 *     Download image bin to module to run.
 *
 *   Operation:
 *   Insert a SD card ,you can see the debug information from the com tools.
 *   If you want test in UFS,you can modify the storage and undefine __TEST_FOR_MEMORY_CARD__
 * 
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
#ifdef __EXAMPLE_FILESYSTEM__
#include "string.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_fs.h"
#include "ql_error.h"
#include "ql_system.h"
#include "ql_stdlib.h"


#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT1
#define DBG_BUF_LEN   512
static char DBG_BUFFER[DBG_BUF_LEN];
#define APP_DEBUG(FORMAT,...) {\
    Ql_memset(DBG_BUFFER, 0, DBG_BUF_LEN);\
    Ql_sprintf(DBG_BUFFER,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)) \
    {\
        Ql_Debug_Trace(DBG_BUFFER);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(DBG_BUFFER), Ql_strlen((const char *)(DBG_BUFFER)));\
    }\
}
#else
#define APP_DEBUG(FORMAT,...) 
#endif


#define __TEST_FOR_RAM_FILE__
//#define __TEST_FOR_UFS__
//#define __TEST_FOR_MEMORY_CARD__

#if defined (__TEST_FOR_RAM_FILE__)
#define  PATH_ROOT    		((u8 *)"RAM:")
#elif defined (__TEST_FOR_UFS__)
#define  PATH_ROOT    		((u8 *)"myroot")
#else   //!__TEST_FOR_MEMORY_CARD__
#define  PATH_ROOT    		((u8 *)"SD:myroot")
#endif


#define LENGTH 100


static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
     
}


/******************************************************************************/
/*                              Main Code                                     */
/******************************************************************************/
void proc_main_task(void)
{   
    ST_MSG msg;
    s32 ret = -1;
    s32 handle = -1;
    s64  space = 0;
    u32 writenLen = 0;
    u32 readenLen = 0;
    s32 filesize = 0;
    s32 position = 0;    
    bool isdir = FALSE;
    
    u8 *filename = "test.txt";
    u8 *fileNewName = "testNew.txt";

    
    u8 strBuf[LENGTH] = {0};
    u8 filePath[LENGTH] = {0};
    u8 fileNewPath[LENGTH] = {0};
    u8  writeBuffer[LENGTH] = {"This is test data!"};

    // Register & open UART port
    ret = Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to register serial port[%d], ret=%d\r\n", UART_PORT1, ret);
    }
    ret = Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to open serial port[%d], ret=%d\r\n", UART_PORT1, ret);
    }

#if defined (__TEST_FOR_RAM_FILE__)
	Enum_FSStorage storage = Ql_FS_RAM;
    APP_DEBUG("\r\n<--OpenCPU: FILE(RAM) TEST!-->\r\n");
#elif defined (__TEST_FOR_UFS__)
	Enum_FSStorage storage = Ql_FS_UFS;
    APP_DEBUG("\r\n<--OpenCPU: FILE(UFS) TEST!-->\r\n");
#else   //!__TEST_FOR_MEMORY_CARD__
	Enum_FSStorage storage = Ql_FS_SD;
    APP_DEBUG("\r\n<--OpenCPU: FILE(SD) TEST!-->\r\n");
#endif

    // format 
    ret = Ql_FS_Format(storage);
    APP_DEBUG("\r\n<-- Ql_FS_Format(storage=%d)ret =%d -->\r\n",storage,ret);        
    
    //check freespace 
    space  = Ql_FS_GetFreeSpace(storage);
    APP_DEBUG("\r\n<--Ql_FS_GetFreeSpace(storage=%d) =%lld-->\r\n",storage,space);
    
    //check total space
    space = Ql_FS_GetTotalSpace(storage);                
    APP_DEBUG("\r\n<--Ql_FS_GetTotalSpace(storage=%d)=%lld-->\r\n",storage,space);

#ifndef __TEST_FOR_RAM_FILE__
    //check dir ,if dir does not exist ,creat it
    ret = Ql_FS_CheckDir(PATH_ROOT);
    if(ret != QL_RET_OK)
    {
		APP_DEBUG("\r\n<-- Dir(%s) is not exist, creating.... -->", PATH_ROOT);
		ret  = Ql_FS_CreateDir(PATH_ROOT);
		if(ret != QL_RET_OK)
		{
			APP_DEBUG("\r\n<-- failed!! Create Dir(%s) fail-->", PATH_ROOT);
			return -1;
		}
		else
		{
			APP_DEBUG("\r\n<-- CreateDir(%s) OK! -->", PATH_ROOT);
		}        
    }
#endif

    //check file  filepath=dir\\file.txt
    Ql_sprintf(filePath,"%s\\%s\0",PATH_ROOT,filename);
    APP_DEBUG("\r\n<--filepath=%s -->\r\n", filePath);    
    ret = Ql_FS_Check(filePath);
    if(ret != QL_RET_OK)
    {
		APP_DEBUG("\r\n<--filepath=%s  filename=%s does not exist creat file! -->", filePath,filename);             
    }

#ifdef __TEST_FOR_RAM_FILE__
    //open file if file does not exist ,creat it
    handle = Ql_FS_OpenRAMFile(filePath,QL_FS_READ_WRITE |QL_FS_CREATE_ALWAYS, LENGTH);
    APP_DEBUG("\r\n<--!! Ql_FS_OpenRAMFile  handle =%d -->\r\n",handle);

#else
    //open file if file does not exist ,creat it
    handle = Ql_FS_Open(filePath,QL_FS_READ_WRITE |QL_FS_CREATE_ALWAYS );
    APP_DEBUG("\r\n<--!! Ql_FS_Open  handle =%d -->\r\n",handle);
#endif
	//write file
    ret = Ql_FS_Write(handle, writeBuffer, Ql_strlen(writeBuffer), &writenLen);
    APP_DEBUG("\r\n<--!! Ql_FS_Write  ret =%d  writenLen =%d-->\r\n",ret,writenLen);
    //fflush
     Ql_FS_Flush(handle);       
    //seek 
    ret = Ql_FS_Seek(handle,5,QL_FS_FILE_BEGIN);
    APP_DEBUG("\r\n<--!! Ql_FS_Seek   ret =%d-->\r\n",ret);    
    //get postion
    position = Ql_FS_GetFilePosition(handle);
    APP_DEBUG("\r\n<--!! Ql_FS_GetFilePosition   position =%d-->\r\n",position);
    //truncate 
    ret = Ql_FS_Truncate(handle);
    //seek begin 
    ret = Ql_FS_Seek(handle,0,QL_FS_FILE_BEGIN);    
    //read file
    ret = Ql_FS_Read(handle, strBuf, LENGTH, &readenLen);
    APP_DEBUG("\r\n<-- Ql_FS_Read() ret=%d: readedlen=%d, readBuffer=%s-->\r\n", ret, readenLen, strBuf);
    //close file
    Ql_FS_Close(handle);    
    
    //get file size
    filesize = Ql_FS_GetSize(filePath);
    APP_DEBUG("\r\n<-- Ql_FS_GetSize() filesize=%d: -->\r\n",filesize);
    //rename file
    Ql_sprintf(fileNewPath,"%s\\%s\0",PATH_ROOT,fileNewName);    
    ret = Ql_FS_Rename(filePath, fileNewPath);
    APP_DEBUG("\r\n<-- Ql_FS_Rename() ret=%d: -->\r\n",ret);

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
    ret = Ql_FS_Delete(fileNewPath);
    APP_DEBUG("\r\n<-- Ql_FS_Delete() ret=%d: -->\r\n",ret);   

#ifndef __TEST_FOR_RAM_FILE__	
    //delet dir
    ret = Ql_FS_DeleteDir(PATH_ROOT);
    APP_DEBUG("\r\n<-- Ql_FS_DeleteDir() ret=%d: -->\r\n",ret);   
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
#endif // __EXAMPLE_FILESYSTEM__


