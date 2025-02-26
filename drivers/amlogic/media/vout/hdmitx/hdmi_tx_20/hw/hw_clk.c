/*
 * drivers/amlogic/media/vout/hdmitx/hdmi_tx_20/hw/hw_clk.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#include "common.h"
#include "mach_reg.h"
#include "reg_sc2.h"
#include "hw_clk.h"
#ifdef CONFIG_AMLOGIC_VPU
#include <linux/amlogic/media/vpu/vpu.h>
#endif

/* local frac_rate flag */
static uint32_t frac_rate;

/*
 * HDMITX Clock configuration
 */

static inline int check_div(unsigned int div)
{
	if (div == -1)
		return -1;
	switch (div) {
	case 1:
		div = 0;
		break;
	case 2:
		div = 1;
		break;
	case 4:
		div = 2;
		break;
	case 6:
		div = 3;
		break;
	case 12:
		div = 4;
		break;
	default:
		break;
	}
	return div;
}

void hdmitx_set_sys_clk(struct hdmitx_dev *hdev, unsigned char flag)
{
	if (flag&4)
		hdmitx_set_cts_sys_clk(hdev);

	if (flag&2) {
		hdmitx_set_top_pclk(hdev);

		if (hdev->chip_type < MESON_CPU_ID_SC2)
			hd_set_reg_bits(P_HHI_GCLK_OTHER, 1, 17, 1);
	}
}

static void hdmitx_disable_encp_clk(struct hdmitx_dev *hdev)
{
	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 0, 2, 1);
	else
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL2, 0, 2, 1);

#ifdef CONFIG_AMLOGIC_VPU
	switch_vpu_clk_gate_vmod(VPU_VENCP, VPU_CLK_GATE_OFF);
	switch_vpu_mem_pd_vmod(VPU_VENCP, VPU_MEM_POWER_DOWN);
#endif
}

static void hdmitx_enable_encp_clk(struct hdmitx_dev *hdev)
{
#ifdef CONFIG_AMLOGIC_VPU
	switch_vpu_clk_gate_vmod(VPU_VENCP, VPU_CLK_GATE_ON);
	switch_vpu_mem_pd_vmod(VPU_VENCP, VPU_MEM_POWER_ON);
#endif

	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 1, 2, 1);
	else
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL2, 1, 2, 1);
}

static void hdmitx_disable_enci_clk(struct hdmitx_dev *hdev)
{
	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 0, 0, 1);
	else
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL2, 0, 0, 1);

#ifdef CONFIG_AMLOGIC_VPU
	switch_vpu_clk_gate_vmod(VPU_VENCI, VPU_CLK_GATE_OFF);
	switch_vpu_mem_pd_vmod(VPU_VENCI, VPU_MEM_POWER_DOWN);
#endif

	if (hdev->hdmitx_clk_tree.venci_top_gate)
		clk_disable_unprepare(hdev->hdmitx_clk_tree.venci_top_gate);

	if (hdev->hdmitx_clk_tree.venci_0_gate)
		clk_disable_unprepare(hdev->hdmitx_clk_tree.venci_0_gate);

	if (hdev->hdmitx_clk_tree.venci_1_gate)
		clk_disable_unprepare(hdev->hdmitx_clk_tree.venci_1_gate);
}

static void hdmitx_enable_enci_clk(struct hdmitx_dev *hdev)
{
	if (hdev->hdmitx_clk_tree.venci_top_gate)
		clk_prepare_enable(hdev->hdmitx_clk_tree.venci_top_gate);

	if (hdev->hdmitx_clk_tree.venci_0_gate)
		clk_prepare_enable(hdev->hdmitx_clk_tree.venci_0_gate);

	if (hdev->hdmitx_clk_tree.venci_1_gate)
		clk_prepare_enable(hdev->hdmitx_clk_tree.venci_1_gate);

#ifdef CONFIG_AMLOGIC_VPU
	switch_vpu_clk_gate_vmod(VPU_VENCI, VPU_CLK_GATE_ON);
	switch_vpu_mem_pd_vmod(VPU_VENCI, VPU_MEM_POWER_ON);
#endif
	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 1, 0, 1);
	else
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL2, 1, 0, 1);
}

static void hdmitx_disable_tx_pixel_clk(struct hdmitx_dev *hdev)
{
	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 0, 5, 1);
	else
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL2, 0, 5, 1);
}

void hdmitx_set_cts_sys_clk(struct hdmitx_dev *hdev)
{
	/* Enable cts_hdmitx_sys_clk */
	/* .clk0 ( cts_oscin_clk ), */
	/* .clk1 ( fclk_div4 ), */
	/* .clk2 ( fclk_div3 ), */
	/* .clk3 ( fclk_div5 ), */
	/* [10: 9] clk_sel. select cts_oscin_clk=24MHz */
	/* [	8] clk_en. Enable gated clock */
	/* [ 6: 0] clk_div. Divide by 1. = 24/1 = 24 MHz */
	if (hdev->chip_type >= MESON_CPU_ID_SC2) {
		hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, 0, 9, 3);
		hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, 0, 0, 7);
		hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, 1, 8, 1);
		return;
	}
	hd_set_reg_bits(P_HHI_HDMI_CLK_CNTL, 0, 9, 3);
	hd_set_reg_bits(P_HHI_HDMI_CLK_CNTL, 0, 0, 7);
	hd_set_reg_bits(P_HHI_HDMI_CLK_CNTL, 1, 8, 1);
}

void hdmitx_set_top_pclk(struct hdmitx_dev *hdev)
{
	/* top hdmitx pixel clock */
	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_CLKCTRL_SYS_CLK_EN0_REG2, 1, 4, 1);
	else
		hd_set_reg_bits(P_HHI_GCLK_MPEG2, 1, 4, 1);
}

void hdmitx_set_cts_hdcp22_clk(struct hdmitx_dev *hdev)
{
	switch (hdev->chip_type) {
	case MESON_CPU_ID_SC2:
		hd_write_reg(P_CLKCTRL_HDCP22_CLK_CTRL, 0x01000100);
		break;
	case MESON_CPU_ID_TXLX:
		/* Enable cts_hdcp22_skpclk */
		/* .clk0 ( cts_oscin_clk ), */
		/* .clk1 ( fclk_div4 ), */
		/* .clk2 ( fclk_div3 ), */
		/* .clk3 ( fclk_div5 ), */
		/* [26: 25] clk_sel. select cts_oscin_clk=24MHz */
		/* [	24] clk_en. Enable gated clock */
		/* [22: 16] clk_div. Divide by 1. = 24/1 = 24 MHz */
		clk_set_rate(hdev->hdmitx_clk_tree.hdcp22_tx_skp, 24000000);
		clk_prepare_enable(hdev->hdmitx_clk_tree.hdcp22_tx_skp);

		/* Enable cts_hdcp22_esmclk */
		/* .clk0 ( fclk_div7 ), */
		/* .clk1 ( fclk_div4 ), */
		/* .clk2 ( fclk_div3 ), */
		/* .clk3 ( fclk_div5 ), */
		/* [10: 9] clk_sel. select fclk_div7*/
		/* [	8] clk_en. Enable gated clock */
		/* [ 6: 0] clk_div. Divide by 1.*/
		clk_set_rate(hdev->hdmitx_clk_tree.hdcp22_tx_esm, 285714285);
		clk_prepare_enable(hdev->hdmitx_clk_tree.hdcp22_tx_esm);
	break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
	default:
		hd_write_reg(P_HHI_HDCP22_CLK_CNTL, 0x01000100);
	break;
	}
}

void hdmitx_set_hdcp_pclk(struct hdmitx_dev *hdev)
{
	/* top hdcp pixel clock */
	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_CLKCTRL_SYS_CLK_EN0_REG2, 1, 3, 1);
	else
		hd_set_reg_bits(P_HHI_GCLK_MPEG2, 1, 3, 1);
}

static void set_gxb_hpll_clk_out(unsigned int frac_rate, unsigned int clk)
{
	switch (clk) {
	case 5940000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800027b);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x135c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4a05, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4c00, 0, 16);
		break;
	case 5405400:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x58000270);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x135c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4800, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x49cd, 0, 16);
		break;
	case 4455000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800025c);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x135c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4b84, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4d00, 0, 16);
		break;
	case 3712500:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800024d);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4443, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4580, 0, 16);
		break;
	case 3450000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x58000247);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4300, 0, 16);
		break;
	case 3243240:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x58000243);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4300, 0, 16);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4800, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4914, 0, 16);
		break;
	case 2970000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800023d);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4d03, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4e00, 0, 16);
		break;
	case 4324320:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800025a);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4171, 0, 16);
		break;
	default:
		pr_info("error hpll clk: %d\n", clk);
		break;
	}
}

static void set_gxtvbb_hpll_clk_out(unsigned int frac_rate, unsigned int clk)
{
	switch (clk) {
	case 5940000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800027b);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4281, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4300, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x12dc5081);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		break;
	case 5405400:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x58000270);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4200, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4273, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x12dc5081);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		break;
	case 4455000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800025c);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x42e1, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4340, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x12dc5081);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		break;
	case 3712500:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800024d);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4111, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4160, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		break;
	case 3450000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x58000247);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4300, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		break;
	case 3243240:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x58000243);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4200, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4245, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		break;
	case 2970000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800023d);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4341, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4380, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x4, 28, 3);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		/* hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x4e00, 0, 16); */
		break;
	case 4324320:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL, 0x5800025a);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		if (frac_rate)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x0, 0, 16);
		else
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0x405c, 0, 16);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x71486980);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x00002e55);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0x0, 28, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		break;
	default:
		pr_info("error hpll clk: %d\n", clk);
		break;
	}
}

static void set_hpll_clk_out(unsigned int clk)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	pr_info("config HPLL = %d frac_rate = %d\n", clk, frac_rate);

	switch (hdev->chip_type) {
	case MESON_CPU_ID_GXBB:
		set_gxb_hpll_clk_out(frac_rate, clk);
		break;
	case MESON_CPU_ID_GXTVBB:
		set_gxtvbb_hpll_clk_out(frac_rate, clk);
		break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
	case MESON_CPU_ID_TXLX:
		set_gxl_hpll_clk_out(frac_rate, clk);
		break;
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
		set_g12a_hpll_clk_out(frac_rate, clk);
		break;
	case MESON_CPU_ID_SC2:
		set_sc2_hpll_clk_out(frac_rate, clk);
	default:
		break;
	}

	pr_info("config HPLL done\n");
}

/* HERE MUST BE BIT OPERATION!!! */
static void set_hpll_sspll(enum hdmi_vic vic)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (hdev->chip_type) {
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
		set_hpll_sspll_g12a(vic);
		break;
	case MESON_CPU_ID_GXBB:
		break;
	case MESON_CPU_ID_GXTVBB:
		break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
		set_hpll_sspll_gxl(vic);
		break;
	case MESON_CPU_ID_SC2:
		set_hpll_sspll_sc2(vic);
		break;
	default:
		break;
	}
}

static void set_hpll_od1(unsigned int div)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (hdev->chip_type) {
	case MESON_CPU_ID_GXBB:
	case MESON_CPU_ID_GXTVBB:
		switch (div) {
		case 1:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0, 16, 2);
			break;
		case 2:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 1, 16, 2);
			break;
		case 4:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 2, 16, 2);
			break;
		case 8:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 3, 16, 2);
			break;
		default:
			break;
		}
		break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
		set_hpll_od1_gxl(div);
		break;
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
		set_hpll_od1_g12a(div);
		break;
	case MESON_CPU_ID_SC2:
		set_hpll_od1_sc2(div);
		break;
	default:
		set_hpll_od1_gxl(div);
		break;
	}
}

static void set_hpll_od2(unsigned int div)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (hdev->chip_type) {
	case MESON_CPU_ID_GXBB:
	case MESON_CPU_ID_GXTVBB:
		switch (div) {
		case 1:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0, 22, 2);
			break;
		case 2:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 1, 22, 2);
			break;
		case 4:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 2, 22, 2);
			break;
		case 8:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 3, 22, 2);
			break;
		default:
			break;
		}
		break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
		set_hpll_od2_gxl(div);
		break;
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
		set_hpll_od2_g12a(div);
		break;
	case MESON_CPU_ID_SC2:
		set_hpll_od2_sc2(div);
		break;
	default:
		set_hpll_od2_gxl(div);
		break;
	}
}

static void set_hpll_od3(unsigned int div)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (hdev->chip_type) {
	case MESON_CPU_ID_GXBB:
	case MESON_CPU_ID_GXTVBB:
		switch (div) {
		case 1:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 0, 18, 2);
			break;
		case 2:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 1, 18, 2);
			break;
		case 4:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 2, 18, 2);
			break;
		case 8:
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 3, 18, 2);
			break;
		default:
			break;
		}
		break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
		set_hpll_od3_gxl(div);
		break;
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
		set_hpll_od3_g12a(div);
		break;
	case MESON_CPU_ID_SC2:
		set_hpll_od3_sc2(div);
		break;
	case MESON_CPU_ID_TM2:
		set_hpll_od3_g12a(div);
		/* new added in TM2 */
		hd_set_reg_bits(P_HHI_LVDS_TX_PHY_CNTL1, 1, 29, 1);
		break;
	default:
		set_hpll_od3_gxl(div);
		break;
	}
}

/* --------------------------------------------------
 *              clocks_set_vid_clk_div
 * --------------------------------------------------
 * wire            clk_final_en    = control[19];
 * wire            clk_div1        = control[18];
 * wire    [1:0]   clk_sel         = control[17:16];
 * wire            set_preset      = control[15];
 * wire    [14:0]  shift_preset    = control[14:0];
 */
static void set_hpll_od3_clk_div(int div_sel)
{
	int shift_val = 0;
	int shift_sel = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	unsigned int reg_vid_pll = P_HHI_VID_PLL_CLK_DIV;

	pr_info("%s[%d] div = %d\n", __func__, __LINE__, div_sel);

	if (hdev->chip_type >= MESON_CPU_ID_SC2)
		reg_vid_pll = P_CLKCTRL_VID_PLL_CLK_DIV;

	/* Disable the output clock */
	hd_set_reg_bits(reg_vid_pll, 0, 18, 2);
	hd_set_reg_bits(reg_vid_pll, 0, 15, 1);

	switch (div_sel) {
	case VID_PLL_DIV_1:
		shift_val = 0xFFFF;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_2:
		shift_val = 0x0aaa;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_3:
		shift_val = 0x0db6;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_3p5:
		shift_val = 0x36cc;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_3p75:
		shift_val = 0x6666;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_4:
		shift_val = 0x0ccc;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_5:
		shift_val = 0x739c;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_6:
		shift_val = 0x0e38;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_6p25:
		shift_val = 0x0000;
		shift_sel = 3;
		break;
	case VID_PLL_DIV_7:
		shift_val = 0x3c78;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_7p5:
		shift_val = 0x78f0;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_12:
		shift_val = 0x0fc0;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_14:
		shift_val = 0x3f80;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_15:
		shift_val = 0x7f80;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_2p5:
		shift_val = 0x5294;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_3p25:
		shift_val = 0x66cc;
		shift_sel = 2;
		break;
	default:
		pr_info("Error: clocks_set_vid_clk_div:  Invalid parameter\n");
		break;
	}

	if (shift_val == 0xffff)      /* if divide by 1 */
		hd_set_reg_bits(reg_vid_pll, 1, 18, 1);
	else {
		hd_set_reg_bits(reg_vid_pll, 0, 18, 1);
		hd_set_reg_bits(reg_vid_pll, 0, 16, 2);
		hd_set_reg_bits(reg_vid_pll, 0, 15, 1);
		hd_set_reg_bits(reg_vid_pll, 0, 0, 15);

		hd_set_reg_bits(reg_vid_pll, shift_sel, 16, 2);
		hd_set_reg_bits(reg_vid_pll, 1, 15, 1);
		hd_set_reg_bits(reg_vid_pll, shift_val, 0, 15);
		hd_set_reg_bits(reg_vid_pll, 0, 15, 1);
	}
	/* Enable the final output clock */
	hd_set_reg_bits(reg_vid_pll, 1, 19, 1);
}

static void set_vid_clk_div(struct hdmitx_dev *hdev, unsigned int div)
{
	if (hdev->chip_type >= MESON_CPU_ID_SC2) {
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 0, 16, 3);
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_DIV, div - 1, 0, 8);
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 7, 0, 3);
	} else {
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL, 0, 16, 3);
		hd_set_reg_bits(P_HHI_VID_CLK_DIV, div - 1, 0, 8);
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL, 7, 0, 3);
	}
}

static void set_hdmi_tx_pixel_div(struct hdmitx_dev *hdev, unsigned int div)
{
	div = check_div(div);
	if (div == -1)
		return;
	if (hdev->chip_type >= MESON_CPU_ID_SC2) {
		hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, div, 16, 4);
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 1, 5, 1);
	} else {
		hd_set_reg_bits(P_HHI_HDMI_CLK_CNTL, div, 16, 4);
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL2, 1, 5, 1);
	}
}

static void set_encp_div(struct hdmitx_dev *hdev, unsigned int div)
{
	div = check_div(div);
	if (div == -1)
		return;
	if (hdev->chip_type >= MESON_CPU_ID_SC2) {
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_DIV, div, 24, 4);
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 1, 19, 1);
	} else {
		hd_set_reg_bits(P_HHI_VID_CLK_DIV, div, 24, 4);
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL, 1, 19, 1);
	}
}

static void set_enci_div(struct hdmitx_dev *hdev, unsigned int div)
{
	div = check_div(div);
	if (div == -1)
		return;
	if (hdev->chip_type >= MESON_CPU_ID_SC2) {
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_DIV, div, 28, 4);
		hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 1, 19, 1);
	} else {
		hd_set_reg_bits(P_HHI_VID_CLK_DIV, div, 28, 4);
		hd_set_reg_bits(P_HHI_VID_CLK_CNTL, 1, 19, 1);
	}
}

/* mode hpll_clk_out od1 od2(PHY) od3
 * vid_pll_div vid_clk_div hdmi_tx_pixel_div encp_div enci_div
 */
static struct hw_enc_clk_val_group setting_enc_clk_val_24[] = {
	{{HDMI_720x480i60_16x9, HDMI_720x480i60_4x3,
	  HDMI_720x576i50_16x9, HDMI_720x576i50_4x3,
	  HDMI_VIC_END},
		4324320, 4, 4, 1, VID_PLL_DIV_5, 1, 2, -1, 2},
	{{HDMI_720x576p50_16x9, HDMI_720x576p50_4x3,
	  HDMI_720x480p60_16x9, HDMI_720x480p60_4x3,
	  HDMI_VIC_END},
		4324320, 4, 4, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_1280x720p50_16x9,
	  HDMI_1280x720p60_16x9,
	  HDMI_VIC_END},
		5940000, 4, 2, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_1920x1080i60_16x9,
	  HDMI_1920x1080i50_16x9,
	  HDMI_VIC_END},
		5940000, 4, 2, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_1920x1080p60_16x9,
	  HDMI_1920x1080p50_16x9,
	  HDMI_VIC_END},
		5940000, 4, 1, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_1920x1080p30_16x9,
	  HDMI_1920x1080p24_16x9,
	  HDMI_1920x1080p25_16x9,
	  HDMI_VIC_END},
		5940000, 4, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_2560x1080p50_64x27,
	  HDMI_VIC_END},
		3712500, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_2560x1080p60_64x27,
	  HDMI_VIC_END},
		3960000, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_3840x2160p30_16x9,
	  HDMI_3840x2160p25_16x9,
	  HDMI_3840x2160p24_16x9,
	  HDMI_1920x1080p120_16x9,
	  HDMI_4096x2160p24_256x135,
	  HDMI_4096x2160p25_256x135,
	  HDMI_4096x2160p30_256x135,
	  HDMI_VIC_END},
		5940000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMI_3840x2160p60_16x9,
	  HDMI_3840x2160p50_16x9,
	  HDMI_4096x2160p60_256x135,
	  HDMI_4096x2160p50_256x135,
	  HDMI_VIC_END},
		5940000, 1, 1, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_4096x2160p60_256x135_Y420,
	  HDMI_4096x2160p50_256x135_Y420,
	  HDMI_3840x2160p60_16x9_Y420,
	  HDMI_3840x2160p50_16x9_Y420,
	  HDMI_VIC_END},
		5940000, 2, 1, 1, VID_PLL_DIV_5, 1, 2, 1, -1},
	{{HDMI_VIC_FAKE,
	  HDMI_VIC_END},
		3450000, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	/* pll setting for VESA modes */
	{{HDMIV_480x320p60hz,
	  HDMI_VIC_END},
		252000, 4, 4, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_640x480p60hz, /* 4.028G / 16 = 251.75M */
	  HDMI_VIC_END},
		251750, 4, 4, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_800x480p60hz,
	  HDMI_VIC_END},
		292300, 4, 4, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_800x600p60hz,
	  HDMI_VIC_END},
		400000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_852x480p60hz,
	   HDMIV_854x480p60hz,
	  HDMI_VIC_END},
		4838400, 4, 4, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1024x600p60hz,
	  HDMI_VIC_END},
		445800, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1024x768p60hz,
	  HDMI_VIC_END},
		650000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1280x480p60hz,
	  HDMI_VIC_END},
		432000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1280x768p60hz,
	  HDMI_VIC_END},
		3180000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1280x800p60hz,
	  HDMI_VIC_END},
		835000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1152x864p75hz,
	  HDMIV_1280x960p60hz,
	  HDMIV_1280x1024p60hz,
	  HDMIV_1600x900p60hz,
	  HDMI_VIC_END},
		1080000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1600x1200p60hz,
	  HDMI_VIC_END},
		1620000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1360x768p60hz,
	  HDMIV_1366x768p60hz,
	  HDMI_VIC_END},
		855000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1400x1050p60hz,
	  HDMI_VIC_END},
		4870000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1440x900p60hz,
	  HDMI_VIC_END},
		1065000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1440x2560p60hz,
	  HDMI_VIC_END},
		4897000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1680x1050p60hz,
	  HDMI_VIC_END},
		1190000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1920x1200p60hz,
	  HDMI_VIC_END},
		1932500, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_2048x1080p24hz,
	  HDMI_VIC_END},
		5940000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_2160x1200p90hz,
	  HDMI_VIC_END},
		5371100, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMIV_2560x1080p60hz,
	  HDMI_VIC_END},
		1980000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_2560x1440p60hz,
	  HDMI_VIC_END},
		2415000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_2560x1600p60hz,
	  HDMI_VIC_END},
		3485000, 1, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_3440x1440p60hz,
	  HDMI_VIC_END},
		3197500, 1, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_2400x1200p90hz,
	  HDMI_VIC_END},
		5600000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
};

/* For colordepth 10bits */
static struct hw_enc_clk_val_group setting_enc_clk_val_30[] = {
	{{HDMI_720x480i60_16x9, HDMI_720x480i60_4x3,
	  HDMI_720x576i50_16x9, HDMI_720x576i50_4x3,
	  HDMI_VIC_END},
		5405400, 4, 4, 1, VID_PLL_DIV_6p25, 1, 2, -1, 2},
	{{HDMI_720x576p50_16x9, HDMI_720x576p50_4x3,
	  HDMI_720x480p60_16x9, HDMI_720x480p60_4x3,
	  HDMI_VIC_END},
		5405400, 4, 4, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1280x720p50_16x9,
	  HDMI_1280x720p60_16x9,
	  HDMI_VIC_END},
		3712500, 4, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1920x1080i60_16x9,
	  HDMI_1920x1080i50_16x9,
	  HDMI_VIC_END},
		3712500, 4, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1920x1080p60_16x9,
	  HDMI_1920x1080p50_16x9,
	  HDMI_VIC_END},
		3712500, 1, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, -1},
	{{HDMI_1920x1080p30_16x9,
	  HDMI_1920x1080p24_16x9,
	  HDMI_1920x1080p25_16x9,
	  HDMI_VIC_END},
		3712500, 2, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, -1},
	{{HDMI_2560x1080p50_64x27,
	  HDMI_VIC_END},
		4640625, 1, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, -1},
	{{HDMI_2560x1080p60_64x27,
	  HDMI_VIC_END},
		4950000, 1, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, -1},
	{{HDMI_4096x2160p60_256x135_Y420,
	  HDMI_4096x2160p50_256x135_Y420,
	  HDMI_3840x2160p60_16x9_Y420,
	  HDMI_3840x2160p50_16x9_Y420,
	  HDMI_VIC_END},
		3712500, 1, 1, 1, VID_PLL_DIV_6p25, 1, 2, 1, -1},
	{{HDMI_3840x2160p24_16x9,
	  HDMI_3840x2160p25_16x9,
	  HDMI_3840x2160p30_16x9,
	  HDMI_1920x1080p120_16x9,
	  HDMI_4096x2160p24_256x135,
	  HDMI_4096x2160p25_256x135,
	  HDMI_4096x2160p30_256x135,
	  HDMI_VIC_END},
		3712500, 1, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_VIC_FAKE,
	  HDMI_VIC_END},
		3450000, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
};

/* For colordepth 12bits */
static struct hw_enc_clk_val_group setting_enc_clk_val_36[] = {
	{{HDMI_720x480i60_16x9, HDMI_720x480i60_4x3,
	  HDMI_720x576i50_16x9, HDMI_720x576i50_4x3,
	  HDMI_VIC_END},
		3243240, 2, 4, 1, VID_PLL_DIV_7p5, 1, 2, -1, 2},
	{{HDMI_720x576p50_16x9, HDMI_720x576p50_4x3,
	  HDMI_720x480p60_16x9, HDMI_720x480p60_4x3,
	  HDMI_VIC_END},
		3243240, 2, 4, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1280x720p50_16x9,
	  HDMI_1280x720p60_16x9,
	  HDMI_VIC_END},
		4455000, 4, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1920x1080i60_16x9,
	  HDMI_1920x1080i50_16x9,
	  HDMI_VIC_END},
		4455000, 4, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1920x1080p60_16x9,
	  HDMI_1920x1080p50_16x9,
	  HDMI_VIC_END},
		4455000, 1, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, -1},
	{{HDMI_2560x1080p50_64x27,
	  HDMI_VIC_END},
		5568750, 1, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, -1},
	{{HDMI_2560x1080p60_64x27,
	  HDMI_VIC_END},
		5940000, 1, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, -1},
	{{HDMI_1920x1080p30_16x9,
	  HDMI_1920x1080p24_16x9,
	  HDMI_1920x1080p25_16x9,
	  HDMI_VIC_END},
		4455000, 2, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, -1},
	{{HDMI_4096x2160p60_256x135_Y420,
	  HDMI_4096x2160p50_256x135_Y420,
	  HDMI_3840x2160p60_16x9_Y420,
	  HDMI_3840x2160p50_16x9_Y420,
	  HDMI_VIC_END},
		4455000, 1, 1, 2, VID_PLL_DIV_3p25, 1, 2, 1, -1},
	{{HDMI_3840x2160p24_16x9,
	  HDMI_3840x2160p25_16x9,
	  HDMI_3840x2160p30_16x9,
	  HDMI_1920x1080p120_16x9,
	  HDMI_4096x2160p24_256x135,
	  HDMI_4096x2160p25_256x135,
	  HDMI_4096x2160p30_256x135,
	  HDMI_VIC_END},
		4455000, 1, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_VIC_FAKE,
	  HDMI_VIC_END},
		3450000, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
};

/* For 3D Frame Packing Clock Setting
 * mode hpll_clk_out od1 od2(PHY) od3
 * vid_pll_div vid_clk_div hdmi_tx_pixel_div encp_div enci_div
 */
static struct hw_enc_clk_val_group setting_3dfp_enc_clk_val[] = {
	{{HDMI_1920x1080p60_16x9,
	  HDMI_1920x1080p50_16x9,
	  HDMI_VIC_END},
		5940000, 2, 1, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_1280x720p50_16x9,
	  HDMI_1280x720p60_16x9,
	  HDMI_1920x1080p30_16x9,
	  HDMI_1920x1080p24_16x9,
	  HDMI_1920x1080p25_16x9,
	  HDMI_VIC_END},
		5940000, 2, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	/* NO 2160p mode*/
	{{HDMI_VIC_FAKE,
	  HDMI_VIC_END},
		3450000, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
};

static void set_hdmitx_fe_clk(struct hdmitx_dev *hdev)
{
	unsigned int tmp = 0;
	enum hdmi_vic vic = hdev->cur_VIC;
	unsigned int vid_clk_cntl2 = P_HHI_VID_CLK_CNTL2;
	unsigned int vid_clk_div = P_HHI_VID_CLK_DIV;
	unsigned int hdmi_clk_cntl = P_HHI_HDMI_CLK_CNTL;

	if (hdev->chip_type < MESON_CPU_ID_TM2B)
		return;
	if (hdev->chip_type >= MESON_CPU_ID_SC2) {
		vid_clk_cntl2 = P_CLKCTRL_VID_CLK_CTRL2;
		vid_clk_div = P_CLKCTRL_VID_CLK_DIV;
		hdmi_clk_cntl = P_CLKCTRL_HDMI_CLK_CTRL;
	}

	hd_set_reg_bits(vid_clk_cntl2, 1, 9, 1);

	switch (vic) {
	case HDMI_720x480i60_16x9:
	case HDMI_720x576i50_16x9:
		tmp = (hd_read_reg(vid_clk_div) >> 28) & 0xf;
		break;
	default:
		tmp = (hd_read_reg(vid_clk_div) >> 24) & 0xf;
		break;
	}

	hd_set_reg_bits(hdmi_clk_cntl, tmp, 20, 4);
}

static void hdmitx_set_clk_(struct hdmitx_dev *hdev)
{
	int i = 0;
	int j = 0;
	struct hw_enc_clk_val_group *p_enc = NULL;
	enum hdmi_vic vic = hdev->cur_VIC;
	enum hdmi_color_space cs = hdev->para->cs;
	enum hdmi_color_depth cd = hdev->para->cd;

	/* YUV 422 always use 24B mode */
	if (cs == COLORSPACE_YUV422)
		cd = COLORDEPTH_24B;

	if (hdev->flag_3dfp) {
		p_enc = &setting_3dfp_enc_clk_val[0];
		for (j = 0; j < sizeof(setting_3dfp_enc_clk_val)
			/ sizeof(struct hw_enc_clk_val_group); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= HDMI_VIC_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == sizeof(setting_3dfp_enc_clk_val)
			/ sizeof(struct hw_enc_clk_val_group)) {
			pr_info("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else if (cd == COLORDEPTH_24B) {
		p_enc = &setting_enc_clk_val_24[0];
		for (j = 0; j < sizeof(setting_enc_clk_val_24)
			/ sizeof(struct hw_enc_clk_val_group); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= HDMI_VIC_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == sizeof(setting_enc_clk_val_24)
			/ sizeof(struct hw_enc_clk_val_group)) {
			pr_info("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else if (cd == COLORDEPTH_30B) {
		p_enc = &setting_enc_clk_val_30[0];
		for (j = 0; j < sizeof(setting_enc_clk_val_30)
			/ sizeof(struct hw_enc_clk_val_group); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= HDMI_VIC_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == sizeof(setting_enc_clk_val_30) /
			sizeof(struct hw_enc_clk_val_group)) {
			pr_info("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else if (cd == COLORDEPTH_36B) {
		p_enc = &setting_enc_clk_val_36[0];
		for (j = 0; j < sizeof(setting_enc_clk_val_36)
			/ sizeof(struct hw_enc_clk_val_group); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= HDMI_VIC_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == sizeof(setting_enc_clk_val_36) /
			sizeof(struct hw_enc_clk_val_group)) {
			pr_info("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else {
		pr_info("not support colordepth 48bits\n");
		return;
	}
next:
	hdmitx_set_cts_sys_clk(hdev);
	set_hpll_clk_out(p_enc[j].hpll_clk_out);
	if ((cd == COLORDEPTH_24B) && (hdev->sspll))
		set_hpll_sspll(vic);
	set_hpll_od1(p_enc[j].od1);
	set_hpll_od2(p_enc[j].od2);
	set_hpll_od3(p_enc[j].od3);
	set_hpll_od3_clk_div(p_enc[j].vid_pll_div);
	pr_info("j = %d  vid_clk_div = %d\n", j, p_enc[j].vid_clk_div);
	set_vid_clk_div(hdev, p_enc[j].vid_clk_div);
	set_hdmi_tx_pixel_div(hdev, p_enc[j].hdmi_tx_pixel_div);

	if (hdev->para->hdmitx_vinfo.viu_mux == VIU_MUX_ENCI) {
		set_enci_div(hdev, p_enc[j].enci_div);
		hdmitx_enable_enci_clk(hdev);
	} else {
		set_encp_div(hdev, p_enc[j].encp_div);
		hdmitx_enable_encp_clk(hdev);
	}
	set_hdmitx_fe_clk(hdev);
}

static int likely_frac_rate_mode(char *m)
{
	if (strstr(m, "24hz") || strstr(m, "30hz") || strstr(m, "60hz")
		|| strstr(m, "120hz") || strstr(m, "240hz"))
		return 1;
	else
		return 0;
}

static void hdmitx_check_frac_rate(struct hdmitx_dev *hdev)
{
	enum hdmi_vic vic = hdev->cur_VIC;
	struct hdmi_format_para *para = NULL;

	frac_rate = hdev->frac_rate_policy;
	para = hdmi_get_fmt_paras(vic);
	if (para && (para->name) && likely_frac_rate_mode(para->name))
		;
	else {
		pr_info("this mode doesn't have frac_rate\n");
		frac_rate = 0;
	}

	pr_info("frac_rate = %d\n", hdev->frac_rate_policy);
}

void hdmitx_set_clk(struct hdmitx_dev *hdev)
{
	hdmitx_check_frac_rate(hdev);

	hdmitx_set_clk_(hdev);
}

void hdmitx_disable_clk(struct hdmitx_dev *hdev)
{
	/* cts_encp/enci_clk */
	if (hdev->para->hdmitx_vinfo.viu_mux == VIU_MUX_ENCI)
		hdmitx_disable_enci_clk(hdev);
	else
		hdmitx_disable_encp_clk(hdev);

	/* hdmi_tx_pixel_clk */
	hdmitx_disable_tx_pixel_clk(hdev);
}

