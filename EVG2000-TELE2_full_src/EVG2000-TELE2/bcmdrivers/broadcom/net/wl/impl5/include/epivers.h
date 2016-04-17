/*
 * Copyright (C) 2008, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: epivers.h.in,v 13.27.2.1 2009/04/16 17:04:53 Exp $
 *
*/

#ifndef _epivers_h_
#define _epivers_h_

#define	EPI_MAJOR_VERSION	5

#define	EPI_MINOR_VERSION	10

#define	EPI_RC_NUMBER		120

#define	EPI_INCREMENTAL_NUMBER	0

#define EPI_BUILD_NUMBER	1

#define	EPI_VERSION		5, 10, 120, 0

#define	EPI_VERSION_NUM		0x050a7800

#define EPI_VERSION_DEV		5.10.0

/* Driver Version String, ASCII, 32 chars max */
#ifndef DSLCPE
#define	EPI_VERSION_STR		"5.10.120.0"
#else
#define EPI_VERSION_STR     	DSLCPE_WLAN_VERSION
#endif

#endif /* _epivers_h_ */
