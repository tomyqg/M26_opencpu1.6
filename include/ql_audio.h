/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2014
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ril_aud.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The file is for OpenCPU RIL sytem definitions and APIs.
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
#ifndef __QL_AUDIO_H__
#define __QL_AUDIO_H__

/* audio format */
typedef enum QlAudioStreamFormattag
{
    QL_AUDIO_STREAMFORMAT_MP3 = 1,
    QL_AUDIO_STREAMFORMAT_AMR = 2,
    QL_AUDIO_STREAMFORMAT_WAV = 3,
    QL_AUDIO_STREAMFORMAT_END
}QlAudioStreamFormat;

/*****************************************************************
* Function:     Ql_StartPlayAudioStream 
* 
* Description:
*               Plays an audio stream with stream size and stream format,
*               and sets the items as below:
*                   - Repeatedly or just Once
*                   - Volume level
*                   - Audio output source.
*               When audio plays ended,
*               a EVENT_MEDIA_FINISH event will be received by embedded application.
* Parameters:
*               stream:
*                       Audio stream
*
*               streamsize:
*                       Audio stream size
*
*               streamformat:
*                       Audio stream format
*
*               repeat:
*                       TRUE or FALSE, indicates that Core System
*                       play the audio repeatedly or just once.
*
*               volumelevel:
*                       Volume level reference "QlAudioVolumeLevel"
*
*               audiopath:
*                       Audio output source
* Return:        
*               QL_RET_OK, suceess
*               or other error codes, please see 'Error Code Definition'
*****************************************************************/
s32 Ql_StartPlayAudioStream(u8 * stream, u32 streamsize, s32 streamformat, bool repeat, u8 volume, u8 audiopath);

#endif
