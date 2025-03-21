/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#ifndef __HALRF_TSSI_8822C_H__
#define __HALRF_TSSI_8822C_H__

#if (RTL8822C_SUPPORT == 1)
/*--------------------------Define Parameters-------------------------------*/
#if 0
/*efuse defind*/
#define CCK_PATHA_G1 0x10
#define CCK_PATHA_G2 0x11
#define CCK_PATHA_G3 0x12
#define CCK_PATHA_G4 0x13
#define CCK_PATHA_G5 0x14
#define CCK_PATHA_G6 0x15
#define OFDM_2G_OFDM_PATHA_G7 0x16
#define OFDM_2G_OFDM_PATHA_G8 0x17
#define OFDM_2G_OFDM_PATHA_G9 0x18
#define OFDM_2G_OFDM_PATHA_G10 0x19
#define OFDM_2G_OFDM_PATHA_G11 0x1a
#define OFDM_5G_OFDM_PATHA_G12 0x22
#define OFDM_5G_OFDM_PATHA_G13 0x23
#define OFDM_5G_OFDM_PATHA_G14 0x24
#define OFDM_5G_OFDM_PATHA_G15 0x25
#define OFDM_5G_OFDM_PATHA_G16 0x26
#define OFDM_5G_OFDM_PATHA_G17 0x27
#define OFDM_5G_OFDM_PATHA_G18 0x28
#define OFDM_5G_OFDM_PATHA_G19 0x29
#define OFDM_5G_OFDM_PATHA_G20 0x2a
#define OFDM_5G_OFDM_PATHA_G21 0x2b
#define OFDM_5G_OFDM_PATHA_G22 0x2c
#define OFDM_5G_OFDM_PATHA_G23 0x2d
#define OFDM_5G_OFDM_PATHA_G24 0x2e
#define OFDM_5G_OFDM_PATHA_G25 0x2f


#define CCK_PATHB_G1 0x3a
#define CCK_PATHB_G2 0x3b
#define CCK_PATHB_G3 0x3c
#define CCK_PATHB_G4 0x3d
#define CCK_PATHB_G5 0x3e
#define CCK_PATHB_G6 0x3f
#define OFDM_2G_OFDM_PATHB_G7 0x40
#define OFDM_2G_OFDM_PATHB_G8 0x41
#define OFDM_2G_OFDM_PATHB_G9 0x42
#define OFDM_2G_OFDM_PATHB_G10 0x43
#define OFDM_2G_OFDM_PATHB_G11 0x44
#define OFDM_5G_OFDM_PATHB_G12 0x4c
#define OFDM_5G_OFDM_PATHB_G13 0x4d
#define OFDM_5G_OFDM_PATHB_G14 0x4e
#define OFDM_5G_OFDM_PATHB_G15 0x4f
#define OFDM_5G_OFDM_PATHB_G16 0x50
#define OFDM_5G_OFDM_PATHB_G17 0x51
#define OFDM_5G_OFDM_PATHB_G18 0x52
#define OFDM_5G_OFDM_PATHB_G19 0x53
#define OFDM_5G_OFDM_PATHB_G20 0x54
#define OFDM_5G_OFDM_PATHB_G21 0x55
#define OFDM_5G_OFDM_PATHB_G22 0x56
#define OFDM_5G_OFDM_PATHB_G23 0x57
#define OFDM_5G_OFDM_PATHB_G24 0x58
#define OFDM_5G_OFDM_PATHB_G25 0x59
#endif

/*---------------------------End Define Parameters----------------------------*/

void halrf_calculate_tssi_codeword_8822c(
	void *dm_void);

void halrf_set_tssi_codeword_8822c(
	void *dm_void, u16 *tssi_value);

void halrf_tssi_dck_8822c(
	void *dm_void);

void halrf_tssi_get_efuse_8822c(
	void *dm_void);

u32 halrf_tssi_get_de_8822c(
	void *dm_void, u8 path);

void halrf_tssi_get_kfree_efuse_8822c(
	void *dm_void);

void halrf_tssi_set_de_for_tx_verify_8822c(
	void *dm_void, u32 tssi_de, u8 path);

void halrf_enable_tssi_8822c(
	void *dm_void);

void halrf_disable_tssi_8822c(
	void *dm_void);

void halrf_do_tssi_8822c(
	void *dm_void);

void halrf_do_thermal_8822c(
	void *dm_void);

u32 halrf_set_tssi_value_8822c(
	void *dm_void,
	u32 tssi_value);

void halrf_set_tssi_poewr_8822c(
	void *dm_void,
	s8 power);

void halrf_get_efuse_thermal_pwrtype_8822c(
	void *dm_void);

u32 halrf_query_tssi_value_8822c(
	void *dm_void);

void halrf_tssi_cck_8822c(
	void *dm_void);

void halrf_thermal_cck_8822c(
	void *dm_void);

#endif /* RTL8822C_SUPPORT */
#endif /*#ifndef __HALRF_TSSI_8822C_H__*/
