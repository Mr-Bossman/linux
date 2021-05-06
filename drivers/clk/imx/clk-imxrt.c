// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 */
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sizes.h>
#include <soc/imx/revision.h>
#include <dt-bindings/clock/imxrt1050-clock.h>

#include "clk.h"
#define ANATOP_BASE_ADDR	0x400d8000


//#define IMXRT_CLK_END 0
static const char * const pll_ref_sels[] = {"osc", "dummy", };
static const char * const per_sels[] = {"ipg_pdof", "osc", };

static const char * const pll1_bypass_sels[] = {"pll1_arm", "pll1_arm_ref_sel", };
static const char * const pll2_bypass_sels[] = {"pll2_sys", "pll2_sys_ref_sel", };
static const char * const pll3_bypass_sels[] = {"pll3_usb_otg", "pll3_usb_otg_ref_sel", };
static const char * const pll5_bypass_sels[] = {"pll5_video", "pll5_video_ref_sel", };

static const char *const pre_periph_sels[] = { "pll2_sys", "pll2_pfd2_396m", "pll2_pfd0_352m", "arm_podf", };
static const char *const periph_sels[] = { "pre_periph_sel", "todo", };
static const char *const usdhc_sels[] = { "pll2_pfd2_396m", "pll2_pfd0_352m", };
static const char *const lpuart_sels[] = { "pll3_80m", "osc", };
static const char *const semc_alt_sels[] = { "pll2_pfd2_396m", "pll3_pfd1_664_62m", };
static const char *const semc_sels[] = { "periph_sel", "semc_alt_sel", };
static const char *const lcdif_sels[] = { "pll2_sys", "pll3_pfd3_454_74m", "pll5_video", "pll2_pfd0_352m", "pll2_pfd1_594m", "pll3_pfd1_664_62m"};




static struct clk *clk[IMXRT1050_CLK_END];
static struct clk_onecell_data clk_data;

static struct clk ** const uart_clks_imxrt[] __initconst = {
	&clk[IMXRT1050_CLK_LPUART1],
	NULL
};

static void __init imxrt_clocks_common_init(void __iomem *base)
{
	clk[IMXRT1050_CLK_DUMMY] = imx_clk_fixed("dummy", 32000000UL);
	clk[IMXRT1050_CLK_OSC] = imx_clk_fixed("osc", 32000000UL); //32MHz

	clk[IMXRT1050_CLK_PLL1_REF_SEL] = imx_clk_mux("pll1_arm_ref_sel", base + 0x0, 14, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clk[IMXRT1050_CLK_PLL2_REF_SEL] = imx_clk_mux("pll2_sys_ref_sel", base + 0x30, 14, 2,pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clk[IMXRT1050_CLK_PLL3_REF_SEL] = imx_clk_mux("pll3_usb_otg_ref_sel", base + 0x10, 14, 2,pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clk[IMXRT1050_CLK_PLL5_REF_SEL] = imx_clk_mux("pll5_video_ref_sel", base + 0xa0, 14, 2,pll_ref_sels, ARRAY_SIZE(pll_ref_sels));

	clk[IMXRT1050_CLK_PLL1_ARM] = imx_clk_pllv3(IMX_PLLV3_SYS, "pll1_arm", "pll1_arm_ref_sel",  base + 0x0, 0x7f);
	clk[IMXRT1050_CLK_PLL2_SYS] = imx_clk_pllv3(IMX_PLLV3_GENERIC, "pll2_sys", "pll2_sys_ref_sel",  base + 0x30, 0x1);
	clk[IMXRT1050_CLK_PLL3_USB_OTG] = imx_clk_pllv3(IMX_PLLV3_USB, "pll3_usb_otg",  "pll3_usb_otg_ref_sel",  base + 0x10, 0x1);
	clk[IMXRT1050_CLK_PLL5_VIDEO] = imx_clk_pllv3(IMX_PLLV3_AV, "pll5_video", "pll5_video_ref_sel",  base + 0xa0, 0x7f);

	/* PLL bypass out */
	clk[IMXRT1050_CLK_PLL1_BYPASS] = imx_clk_mux_flags("pll1_bypass", base + 0x0, 16, 1, pll1_bypass_sels, ARRAY_SIZE(pll1_bypass_sels), CLK_SET_RATE_PARENT);
	clk[IMXRT1050_CLK_PLL2_BYPASS] = imx_clk_mux_flags("pll2_bypass", base + 0x30, 16, 1, pll2_bypass_sels, ARRAY_SIZE(pll2_bypass_sels), CLK_SET_RATE_PARENT);
	clk[IMXRT1050_CLK_PLL3_BYPASS] = imx_clk_mux_flags("pll3_bypass", base + 0x10, 16, 1, pll3_bypass_sels, ARRAY_SIZE(pll3_bypass_sels), CLK_SET_RATE_PARENT);
	clk[IMXRT1050_CLK_PLL5_BYPASS] = imx_clk_mux_flags("pll5_bypass", base + 0xa0, 16, 1, pll5_bypass_sels, ARRAY_SIZE(pll5_bypass_sels), CLK_SET_RATE_PARENT);

	clk[IMXRT1050_CLK_VIDEO_POST_DIV_SEL] = imx_clk_divider("video_post_div_sel", "pll5_video",    base + 0xa0, 19, 2);
	clk[IMXRT1050_CLK_VIDEO_DIV] = imx_clk_divider("video_div", "video_post_div_sel",    base + 0x170, 30, 2);

	clk[IMXRT1050_CLK_PLL3_80M] = imx_clk_fixed_factor("pll3_80m",  "pll3_usb_otg",   1, 6);

	clk[IMXRT1050_CLK_PLL2_PFD0_352M] = imx_clk_pfd("pll2_pfd0_352m", "pll2_sys", base + 0x100, 0);
	clk[IMXRT1050_CLK_PLL2_PFD1_594M] = imx_clk_pfd("pll2_pfd1_594m", "pll2_sys", base + 0x100, 1);
	clk[IMXRT1050_CLK_PLL2_PFD2_396M] = imx_clk_pfd("pll2_pfd2_396m", "pll2_sys", base + 0x100, 2);
	clk[IMXRT1050_CLK_PLL3_PFD1_664_62M] = imx_clk_pfd("pll3_pfd1_664_62m", "pll3_usb_otg", base + 0xf0,1);
	clk[IMXRT1050_CLK_PLL3_PFD3_454_74M] = imx_clk_pfd("pll3_pfd3_454_74m", "pll3_usb_otg", base + 0xf0,3);
}
static void __init imxrt_clocks_init(struct device_node *np)
{

 	void __iomem *ccm_base;
 	void __iomem *pll_base;

	pll_base = ioremap(ANATOP_BASE_ADDR, SZ_16K);
	imxrt_clocks_common_init(pll_base);	/* CCM clocks */ // wriong
 	ccm_base = of_iomap(np, 0);
 	WARN_ON(!ccm_base);

	clk[IMXRT1050_CLK_ARM_PODF] = imx_clk_divider("arm_podf", "pll1_arm",    ccm_base + 0x10, 0, 3);

	clk[IMXRT1050_CLK_PRE_PERIPH_SEL] = imx_clk_mux("pre_periph_sel", ccm_base + 0x18, 18, 2,pre_periph_sels, ARRAY_SIZE(pre_periph_sels));
	clk[IMXRT1050_CLK_PERIPH_SEL] = imx_clk_mux("periph_sel", ccm_base + 0x14, 25, 1,periph_sels, ARRAY_SIZE(periph_sels));
	clk[IMXRT1050_CLK_USDHC1_SEL] = imx_clk_mux("usdhc1_sel", ccm_base + 0x1c, 16, 1,usdhc_sels, ARRAY_SIZE(usdhc_sels));
	clk[IMXRT1050_CLK_USDHC2_SEL] = imx_clk_mux("usdhc2_sel", ccm_base + 0x1c, 17, 1,usdhc_sels, ARRAY_SIZE(usdhc_sels));
	clk[IMXRT1050_CLK_LPUART_SEL] = imx_clk_mux("lpuart_sel", ccm_base + 0x24, 6, 1,lpuart_sels, ARRAY_SIZE(lpuart_sels));
	clk[IMXRT1050_CLK_SEMC_ALT_SEL] = imx_clk_mux("semc_alt_sel", ccm_base + 0x14, 7, 1,semc_alt_sels, ARRAY_SIZE(semc_alt_sels));
	clk[IMXRT1050_CLK_SEMC_SEL] = imx_clk_mux("semc_sel", ccm_base + 0x14, 6, 1,semc_sels, ARRAY_SIZE(semc_sels));
	clk[IMXRT1050_CLK_LCDIF_SEL] = imx_clk_mux("lcdif_sel", ccm_base + 0x38, 15, 3,lcdif_sels, ARRAY_SIZE(lcdif_sels));
	clk[IMXRT1050_CLK_PER_CLK_SEL] = imx_clk_mux("per_sel", ccm_base + 0x1C, 6, 1,per_sels, ARRAY_SIZE(per_sels)); //

	clk[IMXRT1050_CLK_AHB_PODF] = imx_clk_divider("ahb", "periph_sel",    ccm_base + 0x14, 10, 3);
	clk[IMXRT1050_CLK_IPG_PDOF] = imx_clk_divider("ipg", "ahb",    ccm_base + 0x14, 8, 2); // 
	clk[IMXRT1050_CLK_PER_PDOF] = imx_clk_divider("per", "per_sel",    ccm_base + 0x1C, 0, 5); //

	clk[IMXRT1050_CLK_USDHC1_PODF] = imx_clk_divider("usdhc1_podf", "usdhc1_sel",    ccm_base + 0x24, 11, 3);
	clk[IMXRT1050_CLK_USDHC2_PODF] = imx_clk_divider("usdhc2_podf", "usdhc2_sel",    ccm_base + 0x24, 16, 3);
	clk[IMXRT1050_CLK_LPUART_PODF] = imx_clk_divider("lpuart_podf", "lpuart_sel",    ccm_base + 0x24, 0, 6);
	clk[IMXRT1050_CLK_SEMC_PODF] = imx_clk_divider("semc_podf", "semc_sel",    ccm_base + 0x14, 16, 3);
	clk[IMXRT1050_CLK_LCDIF_PRED] = imx_clk_divider("lcdif_pred", "lcdif_sel",    ccm_base + 0x38, 12, 3);
	clk[IMXRT1050_CLK_LCDIF_PODF] = imx_clk_divider("lcdif_podf", "lcdif_pred",    ccm_base + 0x18, 23, 3);

	clk[IMXRT1050_CLK_USDHC1] = imx_clk_gate2("usdhc1", "usdhc1_podf", ccm_base + 0x80, 2);
	clk[IMXRT1050_CLK_USDHC2] = imx_clk_gate2("usdhc2", "usdhc2_podf", ccm_base + 0x80, 4);
	clk[IMXRT1050_CLK_LPUART1] = imx_clk_gate2("lpuart1", "lpuart_podf", ccm_base + 0x7c, 24);
	clk[IMXRT1050_CLK_SEMC] = imx_clk_gate2("semc", "semc_podf", ccm_base + 0x74, 4);
	clk[IMXRT1050_CLK_LCDIF] = imx_clk_gate2("lcdif", "lcdif_podf", ccm_base + 0x74, 10);
	clk[IMXRT1050_CLK_DMA] = imx_clk_gate("dma", "ipg", ccm_base + 0x7C, 6);
	clk[IMXRT1050_CLK_DMA_MUX] = imx_clk_gate("dmamux0", "ipg", ccm_base + 0x7C, 7);



	imx_check_clocks(clk, ARRAY_SIZE(clk));

	clk_data.clks = clk;
	clk_data.clk_num = ARRAY_SIZE(clk);
	of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);

	clk_set_parent(clk[IMXRT1050_CLK_PLL1_BYPASS], clk[IMXRT1050_CLK_PLL1_REF_SEL]);

	clk_prepare_enable(clk[IMXRT1050_CLK_PLL1_ARM]);
	//clk_set_rate(clk[IMXRT1050_CLK_PLL1_ARM], 1056000000UL);

	clk_set_parent(clk[IMXRT1050_CLK_PLL1_BYPASS], clk[IMXRT1050_CLK_PLL1_ARM]);

	clk_set_parent(clk[IMXRT1050_CLK_SEMC_SEL], clk[IMXRT1050_CLK_SEMC_ALT_SEL]);

	//clk_set_rate(clk[IMXRT1050_CLK_PLL2_SYS], 528000000UL);
	clk_prepare_enable(clk[IMXRT1050_CLK_PLL2_SYS]);

	clk_set_parent(clk[IMXRT1050_CLK_PLL2_BYPASS], clk[IMXRT1050_CLK_PLL2_SYS]);

	//clk_set_rate(clk[IMXRT1050_CLK_PLL3_USB_OTG], 480000000UL);
	clk_prepare_enable(clk[IMXRT1050_CLK_PLL3_USB_OTG]);

	clk_set_parent(clk[IMXRT1050_CLK_PLL3_BYPASS], clk[IMXRT1050_CLK_PLL3_USB_OTG]);


	//imx_register_uart_clocks(uart_clks_mx50_mx53);


/*	struct clk *clk, *clk1;

#ifdef CONFIG_SPL_BUILD
	clk_get_by_id(IMXRT1050_CLK_PLL1_REF_SEL, &clk);
	clk_get_by_id(IMXRT1050_CLK_PLL1_BYPASS, &clk1);
	clk_set_parent(clk1, clk);

	clk_get_by_id(IMXRT1050_CLK_PLL1_ARM, &clk);
	clk_prepare_enable(clk);
	clk_set_rate(clk, 1056000000UL);

	clk_get_by_id(IMXRT1050_CLK_PLL1_BYPASS, &clk1);
	clk_set_parent(clk1, clk);

	clk_get_by_id(IMXRT1050_CLK_SEMC_SEL, &clk1);
	clk_get_by_id(IMXRT1050_CLK_SEMC_ALT_SEL, &clk);
	clk_set_parent(clk1, clk);

	clk_get_by_id(IMXRT1050_CLK_PLL2_SYS, &clk);
	clk_prepare_enable(clk);
	clk_set_rate(clk, 528000000UL);

	clk_get_by_id(IMXRT1050_CLK_PLL2_BYPASS, &clk1);
	clk_set_parent(clk1, clk);

	clk_get_by_id(IMXRT1050_CLK_PLL3_USB_OTG, &clk);
	clk_prepare_enable(clk);
	clk_set_rate(clk, 480000000UL);

	clk_get_by_id(IMXRT1050_CLK_PLL3_BYPASS, &clk1);
	clk_set_parent(clk1, clk);
	clk_get_by_id(IMXRT1050_CLK_PLL5_VIDEO, &clk);
	clk_prepare_enable(clk);
	clk_set_rate(clk, 650000000UL);

	clk_get_by_id(IMXRT1050_CLK_PLL5_BYPASS, &clk1);
	clk_set_parent(clk1, clk);
*/
}
CLK_OF_DECLARE(imxrt_ccm, "fsl,imxrt-ccm", imxrt_clocks_init);

