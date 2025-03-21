/******************************************************************************
 *
 * Copyright(c) 2017 - 2019 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/

#include "halmac_8822c_cfg.h"
#include "halmac_common_8822c.h"
#include "../halmac_common_88xx.h"
#if HALMAC_SDIO_SUPPORT
#include "halmac_sdio_8822c.h"
#endif
#include "halmac_cfg_wmac_8822c.h"
#if HALMAC_PCIE_SUPPORT
#include "halmac_pcie_8822c.h"
#endif

#if HALMAC_8822C_SUPPORT

static void
cfg_ldo25_8822c(struct halmac_adapter *adapter, u8 enable);

/**
 * get_hw_value_8822c() -get hw config value
 * @adapter : the adapter of halmac
 * @hw_id : hw id for driver to query
 * @value : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
get_hw_value_8822c(struct halmac_adapter *adapter, enum halmac_hw_id hw_id,
		   void *value)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (!value) {
		PLTFM_MSG_ERR("[ERR]%s (NULL ==value)\n", __func__);
		return HALMAC_RET_NULL_POINTER;
	}

	if (get_hw_value_88xx(adapter, hw_id, value) == HALMAC_RET_SUCCESS)
		return HALMAC_RET_SUCCESS;

	switch (hw_id) {
	case HALMAC_HW_FW_MAX_SIZE:
		*(u32 *)value = WLAN_FW_MAX_SIZE_8822C;
		break;
#if HALMAC_SDIO_SUPPORT
	case HALMAC_HW_SDIO_INT_LAT:
		if (adapter->intf != HALMAC_INTERFACE_SDIO)
			return HALMAC_RET_WRONG_INTF;
		*(u32 *)value = get_sdio_int_lat_8822c(adapter);
		break;
	case HALMAC_HW_SDIO_CLK_CNT:
		if (adapter->intf != HALMAC_INTERFACE_SDIO)
			return HALMAC_RET_WRONG_INTF;
		status = get_sdio_clk_cnt_8822c(adapter, (u32 *)value);
		break;
#endif
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return status;
}

/**
 * set_hw_value_8822c() -set hw config value
 * @adapter : the adapter of halmac
 * @hw_id : hw id for driver to config
 * @value : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
set_hw_value_8822c(struct halmac_adapter *adapter, enum halmac_hw_id hw_id,
		   void *value)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (!value) {
		PLTFM_MSG_ERR("[ERR]null pointer\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (set_hw_value_88xx(adapter, hw_id, value) == HALMAC_RET_SUCCESS)
		return HALMAC_RET_SUCCESS;

	switch (hw_id) {
	case HALMAC_HW_AMPDU_CONFIG:
		cfg_ampdu_8822c(adapter, (struct halmac_ampdu_config *)value);
		break;
	case HALMAC_HW_RXGCK_FIFO:
		cfg_rxgck_fifo_8822c(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_RX_IGNORE:
		cfg_rx_ignore_8822c(adapter,
				    (struct halmac_mac_rx_ignore_cfg *)value);
		break;
	case HALMAC_HW_LDO25_EN:
		cfg_ldo25_8822c(adapter, *(u8 *)value);
		break;
#if HALMAC_PCIE_SUPPORT
	case HALMAC_HW_PCIE_REF_AUTOK:
		if (adapter->intf != HALMAC_INTERFACE_PCIE)
			return HALMAC_RET_WRONG_INTF;
		status = auto_refclk_cal_8822c_pcie(adapter);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		break;
#endif
#if HALMAC_SDIO_SUPPORT
	case HALMAC_HW_SDIO_TX_FORMAT:
		if (adapter->intf != HALMAC_INTERFACE_SDIO)
			return HALMAC_RET_WRONG_INTF;
		status =
		cfg_tx_fmt_sdio_8822c(adapter,
				      *(enum halmac_sdio_tx_format *)value);
		break;
	case HALMAC_HW_SDIO_WT_EN:
		if (adapter->intf != HALMAC_INTERFACE_SDIO)
			return HALMAC_RET_WRONG_INTF;
		status = set_sdio_wt_en_8822c(adapter, 1);
		break;
	case HALMAC_HW_SDIO_CLK_MONITOR:
		if (adapter->intf != HALMAC_INTERFACE_SDIO)
			return HALMAC_RET_WRONG_INTF;
		status =
		set_sdio_clk_mon_8822c(adapter,
				       *(enum halmac_sdio_clk_monitor *)value);
		break;
#endif
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return status;
}

/**
 * halmac_fill_txdesc_check_sum_88xx() -  fill in tx desc check sum
 * @adapter : the adapter of halmac
 * @txdesc : tx desc packet
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
fill_txdesc_check_sum_8822c(struct halmac_adapter *adapter, u8 *txdesc)
{
	__le16 chksum = 0;
	u16 txdesc_size;
	__le16 *data;
	u32 i;

	if (!txdesc) {
		PLTFM_MSG_ERR("[ERR]null pointer\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (adapter->tx_desc_checksum != 1)
		PLTFM_MSG_TRACE("[TRACE]chksum disable\n");

	SET_TX_DESC_TXDESC_CHECKSUM(txdesc, 0x0000);

	data = (u16 *)(txdesc);

	/*unit: 4 Bytes*/
	txdesc_size = (u16)((GET_TX_DESC_PKT_OFFSET(txdesc) +
					(TX_DESC_SIZE_88XX >> 3)) << 1);
	for (i = 0; i < txdesc_size; i++)
		chksum ^= (*(data + 2 * i) ^ *(data + (2 * i + 1)));

	/* *(data + 2 * i) & *(data + (2 * i + 1) have endain issue*/
	/* Process eniadn issue after checksum calculation */
	SET_TX_DESC_TXDESC_CHECKSUM(txdesc, rtk_le16_to_cpu(chksum));

	return HALMAC_RET_SUCCESS;
}

static void
cfg_ldo25_8822c(struct halmac_adapter *adapter, u8 enable)
{
	u8 value8;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	value8 = HALMAC_REG_R8(REG_ANAPARLDO_POW_MAC);

	if (enable == 1)
		HALMAC_REG_W8(REG_ANAPARLDO_POW_MAC, (u8)(value8 | BIT(0)));
	else
		HALMAC_REG_W8(REG_ANAPARLDO_POW_MAC, (u8)(value8 & ~BIT(0)));
}

#endif /* HALMAC_8822C_SUPPORT */
