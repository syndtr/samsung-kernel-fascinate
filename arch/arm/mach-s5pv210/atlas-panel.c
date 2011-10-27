/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/tl2796.h>

#include <mach/regs-clock.h>
#include <mach/gpio.h>
#include <mach/gpio-atlas.h>

#include <plat/devs.h>
#include <plat/fb.h>

#include <../../../drivers/video/samsung/s3cfb.h>

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DEFMASK			0xFF00

#define LCD_BUS_NUM     3
#define DISPLAY_CS      S5PV210_MP01(1)
#define SUB_DISPLAY_CS  S5PV210_MP01(2)
#define DISPLAY_CLK     S5PV210_MP04(1)
#define DISPLAY_SI      S5PV210_MP04(3)

#define S5PV210_LCD_WIDTH 480
#define S5PV210_LCD_HEIGHT 800

static const u16 s6e63m0_SEQ_STANDBY_ON[] = {
	0x010,	/* Stand-by On Command */
	SLEEPMSEC, 160,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_STANDBY_OFF[] = {
	0x011,	/* Stand-by Off Command */
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_DISPLAY_SETTING[] = {
	SLEEPMSEC, 10,
	0x0F8,	/* Panel Condition Set Command*/
	0x101,	/* DOCT */
	0x127,	/* CLWEA */
	0x127,	/* CLWEB*/
	0x107,	/* CLTE */
	0x107,	/* SHE */
	0x154,	/* FLTE */
	0x19F,	/* FLWE */
	0x163,	/* SCTE */
	0x186,	/* SCWE */
	0x11A,	/* INTE */
	0x133,	/* INWE */
	0x10D,	/* EMPS */
	0x100,	/* E_INTE */
	0x100,	/* E_INWE */
	0x0F2,	/* Display Condition Set Command*/
	0x102,	/* Number of Line */
	0x103,	/* VBP */
	0x11C,	/* VFP */
	0x110,	/* HBP */
	0x110,	/* HFP */
	0x0F7,	/* Command */
	0x103,	/* GTCON */
	0x100,	/* Display Mode */
	0x100,	/* Vsync/Hsync, DOCCLK, RGB mode */
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_ETC_SETTING[] = {
	/* ETC Condition Set Command  */
	0x0F6,
	0x100,	0x18E,
	0x107,
	0x0B3,
	0x16C,
	0x0B5,
	0x127,	0x10A,
	0x109,	0x107,
	0x130,	0x11C,
	0x113,	0x109,
	0x110,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0B6,
	0x100,	0x100,
	0x123,	0x111,
	0x132,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x0B7,
	0x127,	0x10A,
	0x109,	0x107,
	0x130,	0x11C,
	0x113,	0x109,
	0x110,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0B8,
	0x100,	0x100,
	0x123,	0x111,
	0x132,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x0B9,
	0x127,	0x10A,
	0x109,	0x107,
	0x130,	0x11C,
	0x113,	0x109,
	0x110,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0BA,
	0x100,	0x100,
	0x123,	0x111,
	0x132,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x011,
	SLEEPMSEC, 120,
	0x029,
	ENDDEF, 0x0000
};

static struct s3cfb_lcd s6e63m0 = {
	.width = S5PV210_LCD_WIDTH,
	.height = S5PV210_LCD_HEIGHT,
	.p_width = 52,
	.p_height = 86,
	.bpp = 24,
	.freq = 60,

	.timing = {
		.h_fp = 16,
		.h_bp = 16,
		.h_sw = 2,
		.v_fp = 28,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};

static const struct tl2796_gamma_adj_points gamma_adj_points = {
	.v0 = 0,
	.v1 = BV_1,
	.v19 = BV_19,
	.v43 = BV_43,
	.v87 = BV_87,
	.v171 = BV_171,
	.v255 = BV_255,
};

static const struct gamma_entry gamma_table[] = {
	{       BV_0, { 4200000, 4200000, 4200000, }, },
	{          1, { 3994200, 4107600, 3910200, }, },
	{ 0x00000400, { 3669486, 3738030, 3655093, }, },
	{ 0x000004C2, { 3664456, 3732059, 3649872, }, },
	{ 0x000005A8, { 3659356, 3726019, 3644574, }, },
	{ 0x000006BA, { 3654160, 3719879, 3639171, }, },
	{ 0x00000800, { 3648872, 3713646, 3633668, }, },
	{ 0x00000983, { 3643502, 3707331, 3628075, }, },
	{ 0x00000B50, { 3638029, 3700909, 3622368, }, },
	{ 0x00000D74, { 3632461, 3694392, 3616558, }, },
	{ 0x00001000, { 3626792, 3687772, 3610636, }, },
	{ 0x00001307, { 3621022, 3681052, 3604605, }, },
	{ 0x000016A1, { 3615146, 3674224, 3598455, }, },
	{ 0x00001AE9, { 3609163, 3667289, 3592189, }, },
	{ 0x00002000, { 3603070, 3660245, 3585801, }, },
	{ 0x0000260E, { 3596860, 3653083, 3579284, }, },
	{ 0x00002D41, { 3590532, 3645805, 3572637, }, },
	{ 0x000035D1, { 3584081, 3638403, 3565854, }, },
	{ 0x00004000, { 3577504, 3630876, 3558930, }, },
	{ 0x00004C1C, { 3570797, 3623221, 3551863, }, },
	{ 0x00005A82, { 3563956, 3615434, 3544649, }, },
	{ 0x00006BA2, { 3556976, 3607510, 3537279, }, },
	{ 0x00008000, { 3549853, 3599444, 3529750, }, },
	{ 0x00009838, { 3542582, 3591234, 3522056, }, },
	{ 0x0000B505, { 3535159, 3582874, 3514193, }, },
	{ 0x0000D745, { 3527577, 3574360, 3506153, }, },
	{ 0x00010000, { 3519832, 3565687, 3497931, }, },
	{ 0x00013070, { 3511918, 3556849, 3489519, }, },
	{ 0x00016A0A, { 3503829, 3547842, 3480912, }, },
	{ 0x0001AE8A, { 3495559, 3538659, 3472102, }, },
	{ 0x00020000, { 3487101, 3529295, 3463080, }, },
	{ 0x000260E0, { 3478447, 3519742, 3453839, }, },
	{ 0x0002D414, { 3469592, 3509996, 3444372, }, },
	{ 0x00035D14, { 3460527, 3500049, 3434667, }, },
	{ 0x00040000, { 3451244, 3489893, 3424717, }, },
	{ 0x0004C1C0, { 3441734, 3479522, 3414512, }, },
	{ 0x0005A828, { 3431990, 3468927, 3404040, }, },
	{ 0x0006BA28, { 3422000, 3458099, 3393292, }, },
	{ 0x00080000, { 3411756, 3447030, 3382254, }, },
	{ 0x0009837F, { 3401247, 3435711, 3370915, }, },
	{ 0x000B504F, { 3390462, 3424131, 3359262, }, },
	{ 0x000D7450, { 3379388, 3412280, 3347281, }, },
	{ 0x00100000, { 3368014, 3400147, 3334957, }, },
	{ 0x001306FE, { 3356325, 3387721, 3322274, }, },
	{ 0x0016A09E, { 3344309, 3374988, 3309216, }, },
	{ 0x001AE8A0, { 3331950, 3361936, 3295765, }, },
	{ 0x00200000, { 3319231, 3348550, 3281902, }, },
	{ 0x00260DFC, { 3306137, 3334817, 3267607, }, },
	{ 0x002D413D, { 3292649, 3320719, 3252859, }, },
	{ 0x0035D13F, { 3278748, 3306240, 3237634, }, },
	{ 0x00400000, { 3264413, 3291361, 3221908, }, },
	{ 0x004C1BF8, { 3249622, 3276065, 3205654, }, },
	{ 0x005A827A, { 3234351, 3260329, 3188845, }, },
	{ 0x006BA27E, { 3218576, 3244131, 3171449, }, },
	{ 0x00800000, { 3202268, 3227448, 3153434, }, },
	{ 0x009837F0, { 3185399, 3210255, 3134765, }, },
	{ 0x00B504F3, { 3167936, 3192523, 3115404, }, },
	{ 0x00D744FD, { 3149847, 3174223, 3095308, }, },
	{ 0x01000000, { 3131093, 3155322, 3074435, }, },
	{ 0x01306FE1, { 3111635, 3135786, 3052735, }, },
	{ 0x016A09E6, { 3091431, 3115578, 3030156, }, },
	{ 0x01AE89FA, { 3070432, 3094655, 3006641, }, },
	{ 0x02000000, { 3048587, 3072974, 2982127, }, },
	{ 0x0260DFC1, { 3025842, 3050485, 2956547, }, },
	{ 0x02D413CD, { 3002134, 3027135, 2929824, }, },
	{ 0x035D13F3, { 2977397, 3002865, 2901879, }, },
	{ 0x04000000, { 2951558, 2977611, 2872620, }, },
	{ 0x04C1BF83, { 2924535, 2951302, 2841948, }, },
	{ 0x05A8279A, { 2896240, 2923858, 2809753, }, },
	{ 0x06BA27E6, { 2866574, 2895192, 2775914, }, },
	{ 0x08000000, { 2835426, 2865207, 2740295, }, },
	{ 0x09837F05, { 2802676, 2833793, 2702744, }, },
	{ 0x0B504F33, { 2768187, 2800829, 2663094, }, },
	{ 0x0D744FCD, { 2731806, 2766175, 2621155, }, },
	{ 0x10000000, { 2693361, 2729675, 2576712, }, },
	{ 0x1306FE0A, { 2652659, 2691153, 2529527, }, },
	{ 0x16A09E66, { 2609480, 2650402, 2479324, }, },
	{ 0x1AE89F99, { 2563575, 2607191, 2425793, }, },
	{ 0x20000000, { 2514655, 2561246, 2368579, }, },
	{ 0x260DFC14, { 2462394, 2512251, 2307272, }, },
	{ 0x2D413CCD, { 2406412, 2459834, 2241403, }, },
	{ 0x35D13F32, { 2346266, 2403554, 2170425, }, },
	{ 0x40000000, { 2281441, 2342883, 2093706, }, },
	{ 0x4C1BF828, { 2211332, 2277183, 2010504, }, },
	{ 0x5A82799A, { 2135220, 2205675, 1919951, }, },
	{ 0x6BA27E65, { 2052250, 2127391, 1821028, }, },
	{ 0x80000000, { 1961395, 2041114, 1712536, }, },
	{ 0x9837F051, { 1861415, 1945288, 1593066, }, },
	{ 0xB504F333, { 1750800, 1837874, 1460986, }, },
	{ 0xD744FCCA, { 1627706, 1716150, 1314437, }, },
	{ 0xFFFFFFFF, { 1489879, 1576363, 1151415, }, },
};

static void reset_lcd(struct s5p_panel_data *pdata)
{
	gpio_direction_output(pdata->gpio_rst, 1);
	msleep(10);

	gpio_set_value(pdata->gpio_rst, 0);
	msleep(10);

	gpio_set_value(pdata->gpio_rst, 1);
	msleep(10);
}

static int configure_mtp_gpios(struct s5p_panel_data *pdata, bool enable)
{
	int i;
	int ret = 0;
	if (enable) {
		/* wrx and csx are already requested by the spi driver */
		ret = gpio_request(pdata->gpio_rdx, "tl2796_rdx");
		if (ret)
			goto err_rdx;
		ret = gpio_request(pdata->gpio_dcx, "tl2796_dcx");
		if (ret)
			goto err_dcx;
		ret = gpio_request(pdata->gpio_rst, "tl2796_rst");
		if (ret)
			goto err_rst;
		for (i = 0; i < 8; i++) {
			ret = gpio_request(pdata->gpio_db[i], "tl2796_dbx");
			if (ret)
				goto err_dbx;
		}
		for (i = 0; i < 8; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_OUTPUT);
			s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
			gpio_set_value(S5PV210_GPF0(i), 0);
		}
		for (i = 0; i < 8; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_OUTPUT);
			s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_UP);
			gpio_set_value(S5PV210_GPF1(i), 0);
		}
		return 0;
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
	}
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	reset_lcd(pdata);

	for (i = 7; i >= 0; i--) {
		gpio_free(pdata->gpio_db[i]);
err_dbx:
		;
	}
	gpio_free(pdata->gpio_rst);
err_rst:
	gpio_free(pdata->gpio_dcx);
err_dcx:
	gpio_free(pdata->gpio_rdx);
err_rdx:
	return ret;
}

struct s5p_panel_data atlas_panel_data = {
	.seq_display_set = s6e63m0_SEQ_DISPLAY_SETTING,
	.seq_etc_set = s6e63m0_SEQ_ETC_SETTING,
	.standby_on = s6e63m0_SEQ_STANDBY_ON,
	.standby_off = s6e63m0_SEQ_STANDBY_OFF,
	.gpio_dcx = S5PV210_GPF0(0), /* H_SYNC pad */
	.gpio_rdx = S5PV210_GPF0(2), /* Enable */
	.gpio_csx = S5PV210_MP01(1),
	.gpio_wrx = S5PV210_MP04(1), /* SCL pad */
	.gpio_rst = S5PV210_MP05(5),
	.gpio_db = {
		S5PV210_GPF0(4),
		S5PV210_GPF0(5),
		S5PV210_GPF0(6),
		S5PV210_GPF0(7),
		S5PV210_GPF1(0),
		S5PV210_GPF1(1),
		S5PV210_GPF1(2),
		S5PV210_GPF1(3),
	},
	.configure_mtp_gpios = configure_mtp_gpios,
	.factory_v255_regs = {
		0x0b9,
		0x0b8,
		0x0fc,
	},
	.color_adj = {
		/* Convert from 8500K to D65, assuming:
		 * Rx 0.66950, Ry 0.33100
		 * Gx 0.18800, Gy 0.74350
		 * Bx 0.14142, By 0.04258
		 */
		.mult = {
			2318372099U,
			2117262806U,
			1729744557U,
		},
		.rshift = 31,
	},

	.gamma_adj_points = &gamma_adj_points,
	.gamma_table = gamma_table,
	.gamma_table_size = ARRAY_SIZE(gamma_table),
};

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "tl2796",
		.platform_data	= &atlas_panel_data,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

static void panel_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
#ifdef CONFIG_FB_S3C_MDNIE
	writel(0x1, S5P_MDNIE_SEL);
#else
	writel(0x2, S5P_MDNIE_SEL);
#endif

	/* DISPLAY_CS */
	s3c_gpio_cfgpin(S5PV210_MP01(1), S3C_GPIO_SFN(1));
	/* DISPLAY_CLK */
	s3c_gpio_cfgpin(S5PV210_MP04(1), S3C_GPIO_SFN(1));
	/* DISPLAY_SO */
	s3c_gpio_cfgpin(S5PV210_MP04(2), S3C_GPIO_SFN(1));
	/* DISPLAY_SI */
	s3c_gpio_cfgpin(S5PV210_MP04(3), S3C_GPIO_SFN(1));

	/* DISPLAY_CS */
	s3c_gpio_setpull(S5PV210_MP01(1), S3C_GPIO_PULL_NONE);
	/* DISPLAY_CLK */
	s3c_gpio_setpull(S5PV210_MP04(1), S3C_GPIO_PULL_NONE);
	/* DISPLAY_SO */
	s3c_gpio_setpull(S5PV210_MP04(2), S3C_GPIO_PULL_NONE);
	/* DISPLAY_SI */
	s3c_gpio_setpull(S5PV210_MP04(3), S3C_GPIO_PULL_NONE);
}

void lcd_cfg_gpio_early_suspend(void)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF0(i), 0);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF1(i), 0);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV210_GPF2(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF2(i), 0);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV210_GPF3(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF3(i), 0);
	}
	/* drive strength to min */
	writel(0x00000000, S5P_VA_GPIO + 0x12c); /* GPF0DRV */
	writel(0x00000000, S5P_VA_GPIO + 0x14c); /* GPF1DRV */
	writel(0x00000000, S5P_VA_GPIO + 0x16c); /* GPF2DRV */
	writel(0x00000000, S5P_VA_GPIO + 0x18c); /* GPF3DRV */

	/* OLED_DET */
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_OLED_DET, 0);

	/* LCD_RST */
	s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MLCD_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_MLCD_RST, 0);

	/* DISPLAY_CS */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CS, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_CS, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_DISPLAY_CS, 0);

	/* DISPLAY_CLK */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CLK, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_CLK, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_DISPLAY_CLK, 0);

	/* DISPLAY_SO */
	/*
	s3c_gpio_cfgpin(S5PV210_MP04(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_MP04(2), S3C_GPIO_PULL_DOWN);
	*/

	/* DISPLAY_SI */
	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_DISPLAY_SI, 0);

	/* OLED_ID */
	s3c_gpio_cfgpin(GPIO_OLED_ID, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OLED_ID, S3C_GPIO_PULL_DOWN);
	/* gpio_set_value(GPIO_OLED_ID, 0); */

	/* DIC_ID */
	s3c_gpio_cfgpin(GPIO_DIC_ID, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_DIC_ID, S3C_GPIO_PULL_DOWN);
	/* gpio_set_value(GPIO_DIC_ID, 0); */
}
EXPORT_SYMBOL(lcd_cfg_gpio_early_suspend);

void lcd_cfg_gpio_late_resume(void)
{
	/* OLED_DET */
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	/* OLED_ID */
	s3c_gpio_cfgpin(GPIO_OLED_ID, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_ID, S3C_GPIO_PULL_NONE);
	/* gpio_set_value(GPIO_OLED_ID, 0); */
	/* DIC_ID */
	s3c_gpio_cfgpin(GPIO_DIC_ID, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DIC_ID, S3C_GPIO_PULL_NONE);
	/* gpio_set_value(GPIO_DIC_ID, 0); */
}
EXPORT_SYMBOL(lcd_cfg_gpio_late_resume);

static int panel_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PV210_MP05(5), "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MP0(5) for "
				"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PV210_MP05(5), 1);
	msleep(10);

	gpio_set_value(S5PV210_MP05(5), 0);
	msleep(10);

	gpio_set_value(S5PV210_MP05(5), 1);
	msleep(10);

	gpio_free(S5PV210_MP05(5));

	return 0;
}

static int panel_backlight_on(struct platform_device *pdev)
{
	return 0;
}

static struct s3c_platform_fb tl2796_data __initdata = {
	.hw_ver		= 0x62,
	.clk_name	= "sclk_fimd",
	.nr_wins	= 5,
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,

	.lcd		= &s6e63m0,
	.cfg_gpio	= panel_cfg_gpio,
	.backlight_on	= panel_backlight_on,
	.reset_lcd	= panel_reset_lcd,
};

void atlas_panel_init(void)
{
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&tl2796_data);
}

static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect = 2,
};

struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};
