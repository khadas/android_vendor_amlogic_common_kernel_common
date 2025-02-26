/*
 * include/linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h
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

#ifndef _HDMI_TX_MODULE_H
#define _HDMI_TX_MODULE_H
#include "hdmi_info_global.h"
#include "hdmi_config.h"
#include "hdmi_hdcp.h"
#include "hdmi_tx_notify.h"
#include <linux/wait.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/clk-provider.h>
#include <linux/device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/spinlock.h>

#define DEVICE_NAME "amhdmitx"

/* HDMITX driver version */
#define HDMITX_VER "20210902"

/* chip type */
#define MESON_CPU_ID_M8B		0
#define MESON_CPU_ID_GXBB		1
#define MESON_CPU_ID_GXTVBB	        2
#define MESON_CPU_ID_GXL		3
#define MESON_CPU_ID_GXM		4
#define MESON_CPU_ID_TXL		5
#define MESON_CPU_ID_TXLX		6
#define MESON_CPU_ID_AXG		7
#define MESON_CPU_ID_GXLX		8
#define MESON_CPU_ID_TXHD		9
#define MESON_CPU_ID_G12A		10
#define MESON_CPU_ID_G12B		11
#define MESON_CPU_ID_SM1		12
#define MESON_CPU_ID_TM2		13
#define MESON_CPU_ID_TM2B		14
#define MESON_CPU_ID_SC2		15

/*****************************
 *    hdmitx attr management
 ******************************/

/************************************
 *    hdmitx device structure
 *************************************/
/*  VIC_MAX_VALID_MODE and VIC_MAX_NUM are associated with
 *	HDMITX_VIC420_OFFSET and HDMITX_VIC_MASK in hdmi_common.h
 */
#define VIC_MAX_VALID_MODE	256 /* consider 4k2k */
/* half for valid vic, half for vic with y420*/
#define VIC_MAX_NUM 512
#define AUD_MAX_NUM 60
struct rx_audiocap {
	unsigned char audio_format_code;
	unsigned char channel_num_max;
	unsigned char freq_cc;
	unsigned char cc3;
};

struct dolby_vsadb_cap {
	unsigned char rawdata[7 + 1]; // padding extra 1 byte
	unsigned int ieeeoui;
	unsigned char length;
	unsigned char dolby_vsadb_ver;
	unsigned char spk_center:1;
	unsigned char spk_surround:1;
	unsigned char spk_height:1;
	unsigned char headphone_only:1;
	unsigned char mat_48k_pcm_only:1;
};

#define MAX_RAW_LEN 64
struct raw_block {
	int len;
	char raw[MAX_RAW_LEN];
};

enum hd_ctrl {
	VID_EN, VID_DIS, AUD_EN, AUD_DIS, EDID_EN, EDID_DIS, HDCP_EN, HDCP_DIS,
};

struct hdr_dynamic_struct {
	unsigned int type;
	unsigned int hd_len;/*hdr_dynamic_length*/
	unsigned char support_flags;
	unsigned char optional_fields[20];
};
#define VESA_MAX_TIMING 64

struct rx_cap {
	unsigned int native_Mode;
	/*video*/
	unsigned int VIC[VIC_MAX_NUM];
	unsigned int VIC_count;
	unsigned int native_VIC;
	enum hdmi_vic vesa_timing[VESA_MAX_TIMING]; /* Max 64 */
	/*audio*/
	struct rx_audiocap RxAudioCap[AUD_MAX_NUM];
	unsigned char AUD_count;
	unsigned char RxSpeakerAllocation;
	struct dolby_vsadb_cap dolby_vsadb_cap;
	/*vendor*/
	unsigned int ieeeoui;
	unsigned char Max_TMDS_Clock1; /* HDMI1.4b TMDS_CLK */
	unsigned int hf_ieeeoui;	/* For HDMI Forum */
	unsigned int Max_TMDS_Clock2; /* HDMI2.0 TMDS_CLK */
	/* CEA861-F, Table 56, Colorimetry Data Block */
	unsigned int colorimetry_data;
	unsigned int scdc_present:1;
	unsigned int scdc_rr_capable:1; /* SCDC read request */
	unsigned int lte_340mcsc_scramble:1;
	unsigned int dc_y444:1;
	unsigned int dc_30bit:1;
	unsigned int dc_36bit:1;
	unsigned int dc_48bit:1;
	unsigned int dc_30bit_420:1;
	unsigned int dc_36bit_420:1;
	unsigned int dc_48bit_420:1;
	unsigned int max_frl_rate:4;
	unsigned int fpap_start_loc:1;
	unsigned int allm:1;
	unsigned int cnc0:1; /* Graphics */
	unsigned int cnc1:1; /* Photo */
	unsigned int cnc2:1; /* Cinema */
	unsigned int cnc3:1; /* Game */
	unsigned int mdelta:1;
	unsigned int fva:1;
	unsigned int hdmi2ver;
	struct hdr_info hdr_info;
	struct dv_info dv_info;
	/* When hdr_priority is 1, then dv_info will be all 0;
	 * when hdr_priority is 2, then dv_info/hdr_info will be all 0
	 * App won't get real dv_cap/hdr_cap, but can get real dv_cap2/hdr_cap2
	 */
	struct hdr_info hdr_info2;
	struct dv_info dv_info2;
	unsigned char IDManufacturerName[4];
	unsigned char IDProductCode[2];
	unsigned char IDSerialNumber[4];
	unsigned char ReceiverProductName[16];
	unsigned char manufacture_week;
	unsigned char manufacture_year;
	unsigned short physical_width;
	unsigned short physical_height;
	unsigned char edid_version;
	unsigned char edid_revision;
	unsigned char ColorDeepSupport;
	unsigned int vLatency;
	unsigned int aLatency;
	unsigned int i_vLatency;
	unsigned int i_aLatency;
	unsigned int threeD_present;
	unsigned int threeD_Multi_present;
	unsigned int hdmi_vic_LEN;
	unsigned int HDMI_3D_LEN;
	unsigned int threeD_Structure_ALL_15_0;
	unsigned int threeD_MASK_15_0;
	struct {
		unsigned char frame_packing;
		unsigned char top_and_bottom;
		unsigned char side_by_side;
	} support_3d_format[VIC_MAX_NUM];
	enum hdmi_vic preferred_mode;
	struct dtd dtd[16];
	unsigned char dtd_idx;
	unsigned char flag_vfpdb;
	unsigned char number_of_dtd;
	struct raw_block asd;
	struct raw_block vsd;
	/*blk0 check sum*/
	unsigned char blk0_chksum;
	unsigned char chksum[10];
};

struct cts_conftab {
	unsigned int fixed_n;
	unsigned int tmds_clk;
	unsigned int fixed_cts;
};

struct vic_attrmap {
	enum hdmi_vic VIC;
	unsigned int tmds_clk;
};

enum hdmi_event_t {
	HDMI_TX_NONE = 0,
	HDMI_TX_HPD_PLUGIN = 1,
	HDMI_TX_HPD_PLUGOUT = 2,
	HDMI_TX_INTERNAL_INTR = 4,
};

struct hdmi_phy_t {
	unsigned long reg;
	unsigned long val_sleep;
	unsigned long val_save;
};

struct audcts_log {
	unsigned int val:20;
	unsigned int stable:1;
};

struct frac_rate_table {
	char *hz;
	u32 sync_num_int;
	u32 sync_den_int;
	u32 sync_num_dec;
	u32 sync_den_dec;
};

struct ced_cnt {
	bool ch0_valid;
	u16 ch0_cnt:15;
	bool ch1_valid;
	u16 ch1_cnt:15;
	bool ch2_valid;
	u16 ch2_cnt:15;
	u8 chksum;
};

struct scdc_locked_st {
	u8 clock_detected:1;
	u8 ch0_locked:1;
	u8 ch1_locked:1;
	u8 ch2_locked:1;
};

#define CEDST_BUF_NO 60
#define ERR_MAX_CNT (0xffff - 1)
struct cedst_buf {
	int buf_idx;
	unsigned int unlock1_no;
	unsigned int maxval_no;
	unsigned int cnt_avg;
	struct {
		struct ced_cnt cnt;
		struct scdc_locked_st st;
	} buf[CEDST_BUF_NO];
};

enum hdmi_hdr_transfer {
	T_UNKNOWN = 0,
	T_BT709,
	T_UNDEF,
	T_BT601,
	T_BT470M,
	T_BT470BG,
	T_SMPTE170M,
	T_SMPTE240M,
	T_LINEAR,
	T_LOG100,
	T_LOG316,
	T_IEC61966_2_4,
	T_BT1361E,
	T_IEC61966_2_1,
	T_BT2020_10,
	T_BT2020_12,
	T_SMPTE_ST_2084,
	T_SMPTE_ST_28,
	T_HLG,
};

enum hdmi_hdr_color {
	C_UNKNOWN = 0,
	C_BT709,
	C_UNDEF,
	C_BT601,
	C_BT470M,
	C_BT470BG,
	C_SMPTE170M,
	C_SMPTE240M,
	C_FILM,
	C_BT2020,
};

struct hdmitx_clk_tree_s {
	/* hdmitx clk tree */
	struct clk *hdmi_clk_vapb;
	struct clk *hdmi_clk_vpu;
	struct clk *hdcp22_tx_skp;
	struct clk *hdcp22_tx_esm;
	struct clk *venci_top_gate;
	struct clk *venci_0_gate;
	struct clk *venci_1_gate;
};

#define EDID_MAX_BLOCK              4
struct hdmitx_dev {
	struct cdev cdev; /* The cdev structure */
	dev_t hdmitx_id;
	struct proc_dir_entry *proc_file;
	struct task_struct *task;
	struct task_struct *task_monitor;
	struct task_struct *task_hdcp;
	struct notifier_block nb;
	struct workqueue_struct *hdmi_wq;
	struct workqueue_struct *rxsense_wq;
	struct workqueue_struct *cedst_wq;
	struct device *hdtx_dev;
	struct device *pdev; /* for pinctrl*/
	struct pinctrl_state *pinctrl_i2c;
	struct pinctrl_state *pinctrl_default;
	struct vinfo_s *vinfo;
	struct delayed_work work_hpd_plugin;
	struct delayed_work work_hpd_plugout;
	struct delayed_work work_rxsense;
	struct delayed_work work_internal_intr;
	struct delayed_work work_cedst;
	struct work_struct work_hdr;
	struct delayed_work work_do_hdcp;
#ifdef CONFIG_AML_HDMI_TX_14
	struct delayed_work cec_work;
#endif
	struct timer_list hdcp_timer;
	int chip_type;
	int hdmi_init;
	int hpdmode;
	/* -1, no hdcp; 0, NULL; 1, 1.4; 2, 2.2 */
	int hdcp_mode;
	/* in board dts file, here can add
	 * &amhdmitx {
	 *     hdcp_type_policy = <1>;
	 * };
	 * 0 is default for NTS 0->1, 1 is fixed as 1, and 2 is fixed as 0
	 */
	/* -1, fixed 0; 0, NTS 0->1; 1, fixed 1 */
	int hdcp_type_policy;
	int hdcp_bcaps_repeater;
	int ready;	/* 1, hdmi stable output, others are 0 */
	int hdcp_hpd_stick;	/* 1 not init & reset at plugout */
	int hdcp_tst_sig;
	unsigned int div40;
	unsigned int lstore;
	struct {
		void (*setpacket)(int type, unsigned char *DB,
				  unsigned char *HB);
		void (*disablepacket)(int type);
		/* In original setpacket, there are many policys, like
		 *  if ((DB[4] >> 4) == T3D_FRAME_PACKING)
		 * Need a only pure data packet to call
		 */
		void (*setdatapacket)(int type, unsigned char *DB,
				      unsigned char *HB);
		void (*setaudioinfoframe)(unsigned char *AUD_DB,
					  unsigned char *CHAN_STAT_BUF);
		int (*setdispmode)(struct hdmitx_dev *hdmitx_device);
		int (*setaudmode)(struct hdmitx_dev *hdmitx_device,
				  struct hdmitx_audpara *audio_param);
		void (*setupirq)(struct hdmitx_dev *hdmitx_device);
		void (*debugfun)(struct hdmitx_dev *hdmitx_device,
				 const char *buf);
		void (*debug_bist)(struct hdmitx_dev *hdmitx_device,
				   unsigned int num);
		void (*uninit)(struct hdmitx_dev *hdmitx_device);
		int (*cntlpower)(struct hdmitx_dev *hdmitx_device,
				 unsigned int cmd, unsigned int arg);
		/* edid/hdcp control */
		int (*cntlddc)(struct hdmitx_dev *hdmitx_device,
			       unsigned int cmd, unsigned long arg);
		/* Audio/Video/System Status */
		int (*getstate)(struct hdmitx_dev *hdmitx_device,
				unsigned int cmd, unsigned int arg);
		int (*cntlpacket)(struct hdmitx_dev *hdmitx_device,
				  unsigned int cmd,
				  unsigned int arg); /* Packet control */
		int (*cntlconfig)(struct hdmitx_dev *hdmitx_device,
				  unsigned int cmd,
				  unsigned int arg); /* Configure control */
		int (*cntlmisc)(struct hdmitx_dev *hdmitx_device,
				unsigned int cmd, unsigned int arg);
		int (*cntl)(struct hdmitx_dev *hdmitx_device, unsigned int cmd,
			    unsigned int arg); /* Other control */
		void (*am_hdmitx_hdcp_disable)(void);
		void (*am_hdmitx_hdcp_enable)(void);
		void (*am_hdmitx_hdcp_result)(unsigned int *exe_type,
				unsigned int *result_type);
		void (*am_hdmitx_set_hdcp_mode)(unsigned int user_type);
	} hwop;
	struct {
		unsigned int hdcp14_en;
		unsigned int hdcp14_rslt;
	} hdcpop;
	struct hdmi_config_platform_data config_data;
	enum hdmi_event_t hdmitx_event;
	unsigned int irq_hpd;
	unsigned int irq_viu1_vsync;
	/*EDID*/
	unsigned int cur_edid_block;
	unsigned int cur_phy_block_ptr;
	unsigned char EDID_buf[EDID_MAX_BLOCK * 128];
	unsigned char EDID_buf1[EDID_MAX_BLOCK*128]; /* for second read */
	unsigned char tmp_edid_buf[128*EDID_MAX_BLOCK];
	unsigned char *edid_ptr;
	/* indicate RX edid data integrated, HEAD valid and checksum pass */
	unsigned int edid_parsing;
	unsigned char EDID_hash[20];
	struct rx_cap rxcap;
	struct hdmitx_vidpara *cur_video_param;
	int vic_count;
	struct hdmitx_clk_tree_s hdmitx_clk_tree;
	/*audio*/
	struct hdmitx_audpara cur_audio_param;
	int audio_param_update_flag;
	unsigned char unplug_powerdown;
	unsigned short physical_addr;
	unsigned int cur_VIC;
	char fmt_attr[16];
	char backup_fmt_attr[16];
	atomic_t kref_video_mute;
	atomic_t kref_audio_mute;
	/**/
	unsigned char hpd_event; /* 1, plugin; 2, plugout */
	unsigned char hpd_state; /* 1, connect; 0, disconnect */
	unsigned char drm_mode_setting; /* 1, setting; 0, keeping */
	unsigned char rhpd_state; /* For repeater use only, no delay */
	unsigned char hdcp_max_exceed_state;
	unsigned int hdcp_max_exceed_cnt;
	unsigned char force_audio_flag;
	unsigned char mux_hpd_if_pin_high_flag;
	int auth_process_timer;
	struct hdmitx_info hdmi_info;
	unsigned int log;
	unsigned int tx_aud_cfg; /* 0, off; 1, on */
	/* For some un-well-known TVs, no edid at all */
	unsigned int tv_no_edid;
	unsigned int hpd_lock;
	struct hdmi_format_para *para;
	/* 0: RGB444  1: Y444  2: Y422  3: Y420 */
	/* 4: 24bit  5: 30bit  6: 36bit  7: 48bit */
	/* if equals to 1, means current video & audio output are blank */
	unsigned int output_blank_flag;
	unsigned int audio_notify_flag;
	unsigned int audio_step;
	bool hdcp22_type;
	unsigned int repeater_tx;
	struct hdcprp_topo *topo_info;
	/* 0.1% clock shift, 1080p60hz->59.94hz */
	unsigned int frac_rate_policy;
	unsigned int backup_frac_rate_policy;
	unsigned int rxsense_policy;
	unsigned int cedst_policy;
	struct ced_cnt ced_cnt;
	struct scdc_locked_st chlocked_st;
	unsigned int phy_idx;
	unsigned int backup_phy_idx;
	struct cedst_buf *cedst_buf; /* current only test 3 phy para */
	unsigned int allm_mode; /* allm_mode: 1/on 0/off */
	unsigned int ct_mode; /* 0/off 1/game, 2/graphcis, 3/photo, 4/cinema */
	unsigned int sspll;
	/* if HDMI plugin even once time, then set 1 */
	/* if never hdmi plugin, then keep as 0 */
	unsigned int already_used;
	/* configure for I2S: 8ch in, 2ch out */
	/* 0: default setting  1:ch0/1  2:ch2/3  3:ch4/5  4:ch6/7 */
	unsigned int aud_output_ch;
	unsigned int hdmi_ch;
	unsigned int tx_aud_src; /* 0: SPDIF  1: I2S */
/* if set to 1, then HDMI will output no audio */
/* In KTV case, HDMI output Picture only, and Audio is driven by other
 * sources.
 */
	unsigned char hdmi_audio_off_flag;
	enum hdmi_hdr_transfer hdr_transfer_feature;
	enum hdmi_hdr_color hdr_color_feature;
	/* 0: sdr 1:standard HDR 2:non standard 3:HLG*/
	unsigned int colormetry;
	unsigned int hdmi_last_hdr_mode;
	unsigned int hdmi_current_hdr_mode;
	unsigned int dv_src_feature;
	unsigned int sdr_hdr_feature;
	unsigned int hdr10plus_feature;
	enum eotf_type hdmi_current_eotf_type;
	enum mode_type hdmi_current_tunnel_mode;
	bool hdmi_current_signal_sdr;
	unsigned int hdr_priority;
	unsigned int flag_3dfp:1;
	unsigned int flag_3dtb:1;
	unsigned int flag_3dss:1;
	unsigned int dongle_mode:1;
	unsigned int cedst_en:1; /* configure in DTS */
	unsigned int bist_lock:1;
	unsigned int drm_feature;/*Direct Rander Management*/
	unsigned int vend_id_hit:1;
	bool systemcontrol_on;
	unsigned char vid_mute_op;
	spinlock_t edid_spinlock; /* edid hdr/dv cap lock */
};

#define CMD_DDC_OFFSET          (0x10 << 24)
#define CMD_STATUS_OFFSET       (0x11 << 24)
#define CMD_PACKET_OFFSET       (0x12 << 24)
#define CMD_MISC_OFFSET         (0x13 << 24)
#define CMD_CONF_OFFSET         (0x14 << 24)
#define CMD_STAT_OFFSET         (0x15 << 24)

/***********************************************************************
 *             DDC CONTROL //cntlddc
 **********************************************************************/
#define DDC_RESET_EDID          (CMD_DDC_OFFSET + 0x00)
#define DDC_RESET_HDCP          (CMD_DDC_OFFSET + 0x01)
#define DDC_HDCP_OP             (CMD_DDC_OFFSET + 0x02)
	#define HDCP14_ON	0x1
	#define HDCP14_OFF	0x2
	#define HDCP22_ON	0x3
	#define HDCP22_OFF	0x4
#define DDC_IS_HDCP_ON          (CMD_DDC_OFFSET + 0x04)
#define DDC_HDCP_GET_AKSV       (CMD_DDC_OFFSET + 0x05)
#define DDC_HDCP_GET_BKSV       (CMD_DDC_OFFSET + 0x06)
#define DDC_HDCP_GET_AUTH       (CMD_DDC_OFFSET + 0x07)
#define DDC_PIN_MUX_OP          (CMD_DDC_OFFSET + 0x08)
#define PIN_MUX             0x1
#define PIN_UNMUX           0x2
#define DDC_EDID_READ_DATA      (CMD_DDC_OFFSET + 0x0a)
#define DDC_IS_EDID_DATA_READY  (CMD_DDC_OFFSET + 0x0b)
#define DDC_EDID_GET_DATA       (CMD_DDC_OFFSET + 0x0c)
#define DDC_EDID_CLEAR_RAM      (CMD_DDC_OFFSET + 0x0d)
#define DDC_HDCP_MUX_INIT	(CMD_DDC_OFFSET + 0x0e)
#define DDC_HDCP_14_LSTORE	(CMD_DDC_OFFSET + 0x0f)
#define DDC_HDCP_22_LSTORE	(CMD_DDC_OFFSET + 0x10)
#define DDC_GLITCH_FILTER_RESET	(CMD_DDC_OFFSET + 0x11)
#define DDC_SCDC_DIV40_SCRAMB	(CMD_DDC_OFFSET + 0x20)
#define DDC_HDCP14_GET_BCAPS_RP	(CMD_DDC_OFFSET + 0x30)
#define DDC_HDCP14_GET_TOPO_INFO (CMD_DDC_OFFSET + 0x31)
#define DDC_HDCP_SET_TOPO_INFO (CMD_DDC_OFFSET + 0x32)
#define DDC_HDCP14_SAVE_OBS	(CMD_DDC_OFFSET + 0x40)

/***********************************************************************
 *             CONFIG CONTROL //cntlconfig
 **********************************************************************/
/* Video part */
#define CONF_HDMI_DVI_MODE      (CMD_CONF_OFFSET + 0x02)
#define HDMI_MODE           0x1
#define DVI_MODE            0x2
#define CONF_AVI_BT2020		(CMD_CONF_OFFSET + 0X2000 + 0x00)
	#define CLR_AVI_BT2020	0x0
	#define SET_AVI_BT2020	0x1
/* set value as COLORSPACE_RGB444, YUV422, YUV444, YUV420 */
#define CONF_AVI_RGBYCC_INDIC	(CMD_CONF_OFFSET + 0X2000 + 0x01)
#define CONF_AVI_Q01		(CMD_CONF_OFFSET + 0X2000 + 0x02)
	#define RGB_RANGE_DEFAULT	0
	#define RGB_RANGE_LIM		1
	#define RGB_RANGE_FUL		2
	#define RGB_RANGE_RSVD		3
#define CONF_AVI_YQ01		(CMD_CONF_OFFSET + 0X2000 + 0x03)
	#define YCC_RANGE_LIM		0
	#define YCC_RANGE_FUL		1
	#define YCC_RANGE_RSVD		2
#define CONF_CT_MODE		(CMD_CONF_OFFSET + 0X2000 + 0x04)
	#define SET_CT_OFF		0
	#define SET_CT_GAME		1
	#define SET_CT_GRAPHICS	2
	#define SET_CT_PHOTO	3
	#define SET_CT_CINEMA	4
#define CONF_GET_AVI_BT2020 (CMD_CONF_OFFSET + 0X2000 + 0x05)
#define CONF_VIDEO_MUTE_OP      (CMD_CONF_OFFSET + 0x1000 + 0x04)
#define VIDEO_NONE_OP		0x0
#define VIDEO_MUTE          0x1
#define VIDEO_UNMUTE        0x2
#define CONF_EMP_NUMBER         (CMD_CONF_OFFSET + 0x3000 + 0x00)
#define CONF_EMP_PHY_ADDR       (CMD_CONF_OFFSET + 0x3000 + 0x01)

/* Audio part */
#define CONF_CLR_AVI_PACKET     (CMD_CONF_OFFSET + 0x04)
#define CONF_CLR_VSDB_PACKET    (CMD_CONF_OFFSET + 0x05)
#define CONF_VIDEO_MAPPING	(CMD_CONF_OFFSET + 0x06)
#define CONF_GET_HDMI_DVI_MODE	(CMD_CONF_OFFSET + 0x07)
#define CONF_CLR_DV_VS10_SIG	(CMD_CONF_OFFSET + 0x10)

#define CONF_AUDIO_MUTE_OP      (CMD_CONF_OFFSET + 0x1000 + 0x00)
#define AUDIO_MUTE          0x1
#define AUDIO_UNMUTE        0x2
#define CONF_CLR_AUDINFO_PACKET (CMD_CONF_OFFSET + 0x1000 + 0x01)

/***********************************************************************
 *             MISC control, hpd, hpll //cntlmisc
 **********************************************************************/
#define MISC_HPD_MUX_OP         (CMD_MISC_OFFSET + 0x00)
#define MISC_HPD_GPI_ST         (CMD_MISC_OFFSET + 0x02)
#define MISC_HPLL_OP            (CMD_MISC_OFFSET + 0x03)
#define		HPLL_ENABLE         0x1
#define		HPLL_DISABLE        0x2
#define		HPLL_SET	    0x3
#define MISC_TMDS_PHY_OP        (CMD_MISC_OFFSET + 0x04)
#define TMDS_PHY_ENABLE     0x1
#define TMDS_PHY_DISABLE    0x2
#define MISC_VIID_IS_USING      (CMD_MISC_OFFSET + 0x05)
#define MISC_CONF_MODE420       (CMD_MISC_OFFSET + 0x06)
#define MISC_TMDS_CLK_DIV40     (CMD_MISC_OFFSET + 0x07)
#define MISC_COMP_HPLL         (CMD_MISC_OFFSET + 0x08)
#define COMP_HPLL_SET_OPTIMISE_HPLL1    0x1
#define COMP_HPLL_SET_OPTIMISE_HPLL2    0x2
#define MISC_COMP_AUDIO         (CMD_MISC_OFFSET + 0x09)
#define COMP_AUDIO_SET_N_6144x2          0x1
#define COMP_AUDIO_SET_N_6144x3          0x2
#define MISC_AVMUTE_OP          (CMD_MISC_OFFSET + 0x0a)
#define MISC_FINE_TUNE_HPLL     (CMD_MISC_OFFSET + 0x0b)
	#define OFF_AVMUTE	0x0
	#define CLR_AVMUTE	0x1
	#define SET_AVMUTE	0x2
#define MISC_HPLL_FAKE			(CMD_MISC_OFFSET + 0x0c)
#define MISC_ESM_RESET		(CMD_MISC_OFFSET + 0x0d)
#define MISC_HDCP_CLKDIS	(CMD_MISC_OFFSET + 0x0e)
#define MISC_TMDS_RXSENSE	(CMD_MISC_OFFSET + 0x0f)
#define MISC_I2C_REACTIVE       (CMD_MISC_OFFSET + 0x10) /* For gxl */
#define MISC_I2C_RESET		(CMD_MISC_OFFSET + 0x11) /* For g12 */
#define MISC_READ_AVMUTE_OP     (CMD_MISC_OFFSET + 0x12)
#define MISC_TMDS_CEDST		(CMD_MISC_OFFSET + 0x13)
#define MISC_TRIGGER_HPD        (CMD_MISC_OFFSET + 0X14)
#define MISC_SUSFLAG		(CMD_MISC_OFFSET + 0X15)
#define MISC_AUDIO_RESET	(CMD_MISC_OFFSET + 0x16)

/***********************************************************************
 *                          Get State //getstate
 **********************************************************************/
#define STAT_VIDEO_VIC          (CMD_STAT_OFFSET + 0x00)
#define STAT_VIDEO_CLK          (CMD_STAT_OFFSET + 0x01)
#define STAT_AUDIO_FORMAT       (CMD_STAT_OFFSET + 0x10)
#define STAT_AUDIO_CHANNEL      (CMD_STAT_OFFSET + 0x11)
#define STAT_AUDIO_CLK_STABLE   (CMD_STAT_OFFSET + 0x12)
#define STAT_AUDIO_PACK         (CMD_STAT_OFFSET + 0x13)
#define STAT_HDR_TYPE		(CMD_STAT_OFFSET + 0x20)


/* HDMI LOG */
#define HDMI_LOG_HDCP           (1 << 0)

#define HDMI_SOURCE_DESCRIPTION 0
#define HDMI_PACKET_VEND        1
#define HDMI_MPEG_SOURCE_INFO   2
#define HDMI_PACKET_AVI         3
#define HDMI_AUDIO_INFO         4
#define HDMI_AUDIO_CONTENT_PROTECTION   5
#define HDMI_PACKET_HBR         6
#define HDMI_PACKET_DRM		0x86

#define HDMI_PROCESS_DELAY  msleep(10)
/* reduce a little time, previous setting is 4000/10 */
#define AUTH_PROCESS_TIME   (1000/100)

/***********************************************************************
 *    hdmitx protocol level interface
 **********************************************************************/
extern enum hdmi_vic hdmitx_edid_vic_tab_map_vic(const char *disp_mode);

extern int hdmitx_edid_parse(struct hdmitx_dev *hdmitx_device);
extern int check_dvi_hdmi_edid_valid(unsigned char *buf);

enum hdmi_vic hdmitx_edid_get_VIC(struct hdmitx_dev *hdmitx_device,
	const char *disp_mode, char force_flag);

extern int hdmitx_edid_VIC_support(enum hdmi_vic vic);

extern int hdmitx_edid_dump(struct hdmitx_dev *hdmitx_device, char *buffer,
	int buffer_len);
bool hdmitx_edid_check_valid_mode(struct hdmitx_dev *hdev,
	struct hdmi_format_para *para);
const char *hdmitx_edid_vic_tab_map_string(enum hdmi_vic vic);
extern const char *hdmitx_edid_vic_to_string(enum hdmi_vic vic);
extern void hdmitx_edid_clear(struct hdmitx_dev *hdmitx_device);

extern void hdmitx_edid_ram_buffer_clear(struct hdmitx_dev *hdmitx_device);

extern void hdmitx_edid_buf_compare_print(struct hdmitx_dev *hdmitx_device);

extern const char *hdmitx_edid_get_native_VIC(struct hdmitx_dev *hdmitx_device);
bool hdmitx_check_edid_all_zeros(unsigned char *buf);
bool hdmitx_edid_notify_ng(unsigned char *buf);

extern struct hdmitx_audpara hdmiaud_config_data;
extern struct hdmitx_audpara hsty_hdmiaud_config_data[8];
extern unsigned int hsty_hdmiaud_config_loc, hsty_hdmiaud_config_num;

/* VSIF: Vendor Specific InfoFrame
 * It has multiple purposes:
 * 1. HDMI1.4 4K, HDMI_VIC=1/2/3/4, 2160p30/25/24hz, smpte24hz, AVI.VIC=0
 *    In CTA-861-G, matched with AVI.VIC=95/94/93/98
 * 2. 3D application, TB/SS/FP
 * 3. DolbyVision, with Len=0x18
 * 4. HDR10plus
 * 5. HDMI20 3D OSD disparity / 3D dual-view / 3D independent view / ALLM
 * Some functions are exclusive, but some may compound.
 * Consider various state transitions carefully, such as play 3D under HDMI14
 * 4K, exit 3D under 4K, play DV under 4K, enable ALLM under 3D dual-view
 */
enum vsif_type {
	/* Below 4 functions are exclusive */
	VT_HDMI14_4K = 1,
	VT_T3D_VIDEO,
	VT_DOLBYVISION,
	VT_HDR10PLUS,
	/* Maybe compound 3D dualview + ALLM */
	VT_T3D_OSD_DISPARITY = 0x10,
	VT_T3D_DUALVIEW,
	VT_T3D_INDEPENDVEW,
	VT_ALLM,
	/* default: if non-HDMI4K, no any vsif; if HDMI4k, = VT_HDMI14_4K */
	VT_DEFAULT,
	VT_MAX,
};
int hdmitx_construct_vsif(struct hdmitx_dev *hdev, enum vsif_type type, int on,
	void *param);

/* if vic is 93 ~ 95, or 98 (HDMI14 4K), return 1 */
bool is_hdmi14_4k(enum hdmi_vic vic);

/* if 4k is Y420, return 1 */
bool is_hdmi4k_420(enum hdmi_vic vic);

/* set vic to AVI.VIC */
void hdmitx_set_avi_vic(enum hdmi_vic vic);

/*
 * HDMI Repeater TX I/F
 * RX downstream Information from rptx to rprx
 */
/* send part raw edid from TX to RX */
extern void rx_repeat_hpd_state(unsigned int st);
/* prevent compile error in no HDMIRX case */
void __attribute__((weak))rx_repeat_hpd_state(unsigned int st)
{
}

extern void rx_edid_physical_addr(unsigned char a, unsigned char b,
	unsigned char c, unsigned char d);
void __attribute__((weak))rx_edid_physical_addr(unsigned char a,
	unsigned char b, unsigned char c, unsigned char d)
{
}

extern int rx_set_hdr_lumi(unsigned char *data, int len);
int __attribute__((weak))rx_set_hdr_lumi(unsigned char *data, int len)
{
	return 0;
}

extern void rx_set_repeater_support(bool enable);
void __attribute__((weak))rx_set_repeater_support(bool enable)
{
}

extern void rx_set_receiver_edid(unsigned char *data, int len);
void __attribute__((weak))rx_set_receiver_edid(unsigned char *data, int len)
{
}

extern void rx_set_receive_hdcp(unsigned char *data, int len, int depth,
	bool max_cascade, bool max_devs);
void __attribute__((weak))rx_set_receive_hdcp(unsigned char *data, int len,
	int depth, bool max_cascade, bool max_devs)
{
}

extern int hdmitx_set_display(struct hdmitx_dev *hdmitx_device,
	enum hdmi_vic VideoCode);

extern int hdmi_set_3d(struct hdmitx_dev *hdmitx_device, int type,
	unsigned int param);

extern int hdmitx_set_audio(struct hdmitx_dev *hdmitx_device,
	struct hdmitx_audpara *audio_param);

#define HDMI_SUSPEND	0
#define HDMI_WAKEUP	1

enum hdmitx_event {
	HDMITX_NONE_EVENT = 0,
	HDMITX_HPD_EVENT,
	HDMITX_HDCP_EVENT,
	HDMITX_AUDIO_EVENT,
	HDMITX_HDCPPWR_EVENT,
	HDMITX_HDR_EVENT,
	HDMITX_RXSENSE_EVENT,
	HDMITX_CEDST_EVENT,
};

#define MAX_UEVENT_LEN 64
struct hdmitx_uevent {
	const enum hdmitx_event type;
	int state;
	const char *env;
};

int hdmitx_set_uevent(enum hdmitx_event type, int val);

#ifdef CONFIG_AMLOGIC_HDMITX
extern struct hdmitx_dev *get_hdmitx_device(void);
extern int get_hpd_state(void);
#ifdef CONFIG_DRM_MESON_HDMI
void hdmitx_notify_hpd(int hpd, void *p);
#endif
bool is_tv_changed(void);
extern void hdmitx_hdcp_status(int hdmi_authenticated);
#else
static inline struct hdmitx_dev *get_hdmitx_device(void)
{
	return NULL;
}
static inline int get_hpd_state(void)
{
	return 0;
}
static inline int hdmitx_event_notifier_regist(struct notifier_block *nb)
{
	return -EINVAL;
}

static inline int hdmitx_event_notifier_unregist(struct notifier_block *nb)
{
	return -EINVAL;
}
#endif

extern void hdmi_set_audio_para(int para);
extern int get_cur_vout_index(void);
extern void phy_pll_off(void);
extern int get_hpd_state(void);
extern void hdmitx_hdcp_do_work(struct hdmitx_dev *hdev);

/***********************************************************************
 *    hdmitx hardware level interface
 ***********************************************************************/
extern void HDMITX_Meson_Init(struct hdmitx_dev *hdmitx_device);
extern unsigned int get_hdcp22_base(void);
/*
 * hdmitx_audio_mute_op() is used by external driver call
 * flag: 0: audio off   1: audio_on
 *       2: for EDID auto mode
 */
extern void hdmitx_audio_mute_op(unsigned int flag);
extern void hdmitx_video_mute_op(unsigned int flag);

/*
 * HDMITX HPD HW related operations
 */
enum hpd_op {
	HPD_INIT_DISABLE_PULLUP,
	HPD_INIT_SET_FILTER,
	HPD_IS_HPD_MUXED,
	HPD_MUX_HPD,
	HPD_UNMUX_HPD,
	HPD_READ_HPD_GPIO,
};
extern int hdmitx_hpd_hw_op(enum hpd_op cmd);
/*
 * HDMITX DDC HW related operations
 */
enum ddc_op {
	DDC_INIT_DISABLE_PULL_UP_DN,
	DDC_MUX_DDC,
	DDC_UNMUX_DDC,
};
extern int hdmitx_ddc_hw_op(enum ddc_op cmd);

#define HDMITX_HWCMD_MUX_HPD_IF_PIN_HIGH       0x3
#define HDMITX_HWCMD_TURNOFF_HDMIHW           0x4
#define HDMITX_HWCMD_MUX_HPD                0x5
#define HDMITX_HWCMD_PLL_MODE                0x6
#define HDMITX_HWCMD_TURN_ON_PRBS           0x7
#define HDMITX_FORCE_480P_CLK                0x8
#define HDMITX_GET_AUTHENTICATE_STATE        0xa
#define HDMITX_SW_INTERNAL_HPD_TRIG          0xb
#define HDMITX_HWCMD_OSD_ENABLE              0xf

#define HDMITX_HDCP_MONITOR                  0x11
#define HDMITX_IP_INTR_MASN_RST              0x12
#define HDMITX_EARLY_SUSPEND_RESUME_CNTL     0x14
#define HDMITX_EARLY_SUSPEND             0x1
#define HDMITX_LATE_RESUME               0x2
/* Refer to HDMI_OTHER_CTRL0 in hdmi_tx_reg.h */
#define HDMITX_IP_SW_RST                     0x15
#define TX_CREG_SW_RST      (1<<5)
#define TX_SYS_SW_RST       (1<<4)
#define CEC_CREG_SW_RST     (1<<3)
#define CEC_SYS_SW_RST      (1<<2)
#define HDMITX_AVMUTE_CNTL                   0x19
#define AVMUTE_SET          0   /* set AVMUTE to 1 */
#define AVMUTE_CLEAR        1   /* set AVunMUTE to 1 */
#define AVMUTE_OFF          2   /* set both AVMUTE and AVunMUTE to 0 */
#define HDMITX_CBUS_RST                      0x1A
#define HDMITX_INTR_MASKN_CNTL               0x1B
#define INTR_MASKN_ENABLE   0
#define INTR_MASKN_DISABLE  1
#define INTR_CLEAR          2

#define HDMI_HDCP_DELAYTIME_AFTER_DISPLAY    20      /* unit: ms */

#define HDMITX_HDCP_MONITOR_BUF_SIZE         1024
struct Hdcp_Sub {
	char *hdcp_sub_name;
	unsigned int hdcp_sub_addr_start;
	unsigned int hdcp_sub_len;
};

void am_hdmitx_hdcp_disable(void);
void am_hdmitx_hdcp_enable(void);
void hdmi_tx_edid_proc(unsigned char *edid);

extern void setup_attr(const char *buf);
extern void get_attr(char attr[16]);
extern unsigned int hd_read_reg(unsigned int addr);
extern void hd_write_reg(unsigned int addr, unsigned int val);
extern void hd_set_reg_bits(unsigned int addr, unsigned int value,
		unsigned int offset, unsigned int len);
extern void hdmitx_wr_reg(unsigned int addr, unsigned int data);
extern void hdmitx_poll_reg(unsigned int addr, unsigned int val,
	unsigned long timeout);
extern void hdmitx_set_reg_bits(unsigned int addr, unsigned int value,
	unsigned int offset, unsigned int len);
extern unsigned int hdmitx_rd_reg(unsigned int addr);
extern unsigned int hdmitx_rd_check_reg(unsigned int addr,
	unsigned int exp_data,
	unsigned int mask);
extern void vsem_init_cfg(struct hdmitx_dev *hdev);
void update_current_para(struct hdmitx_dev *hdev);

enum hdmi_tf_type hdmitx_get_cur_hdr_st(void);
enum hdmi_tf_type hdmitx_get_cur_dv_st(void);
enum hdmi_tf_type hdmitx_get_cur_hdr10p_st(void);
bool hdmitx_hdr_en(void);
bool hdmitx_dv_en(void);
bool hdmitx_hdr10p_en(void);
bool LGAVIErrorTV(struct rx_cap *prxcap);
bool hdmitx_find_vendor_6g(struct hdmitx_dev *hdev);
bool hdmitx_find_vendor_ratio(struct hdmitx_dev *hdev);
int hdmitx_uboot_already_display(int type);
#endif
