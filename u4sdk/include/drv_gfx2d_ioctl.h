/******************************************************************************
*              Copyright 2004 - 2014, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_gfx2d_type.h
* Description: Describe graphic 2D engine data type.
*
* History:
* Version   Date         Author     DefectNum    Description
* main\1    2013-08-02              NULL         Create this file.
*******************************************************************************/

#ifndef _HI_DRV_GFX2D_H_
#define _HI_DRV_GFX2D_H_

#include <linux/ioctl.h>

#define GFX2D_IOC_MAGIC 'G'

typedef struct
{
    HI_GFX2D_DEV_ID_E enDevId;
    HI_GFX2D_COMPOSE_LIST_S *pstComposeList;
    HI_GFX2D_SURFACE_S *pstDst;
    HI_BOOL bSync;
    HI_U32 u32Timeout;
}GFX2D_COMPOSE_CMD_S;

typedef struct
{
    HI_GFX2D_DEV_ID_E enDevId;
    HI_U32 u32Timeout;
}GFX2D_WAITALLDONE_CMD_S;

#define GFX2D_COMPOSE _IOW(GFX2D_IOC_MAGIC, 1, GFX2D_COMPOSE_CMD_S)
#define GFX2D_WATIALLDONE _IOW(GFX2D_IOC_MAGIC, 2, GFX2D_WAITALLDONE_CMD_S)

#endif
