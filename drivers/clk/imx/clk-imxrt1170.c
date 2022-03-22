// SPDX-License-Identifier: (GPL-2.0+ OR BSD-3-Clause)
/*
 * Copyright (C) 2022
 * Author(s):
 * Jesse Taube <Mr.Bossman075@gmail.com>
 */
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <dt-bindings/clock/imxrt1170-clock.h>

#include "clk.h"

#define CLOCK_MUX_DEFAULT "rcosc48M_div2", "osc", "rcosc400M", "rcosc16M"

#define LPCG_GATE(gate) (0x6000 + (gate * 0x20))

enum root_clock_names {
	m7,
	m4,
	bus,
	bus_lpsr,
	semc,
	cssys,
	cstrace,
	m4_systick,
	m7_systick,
	adc1,
	adc2,
	acmp,
	flexio1,
	flexio2,
	gpt1,
	gpt2,
	gpt3,
	gpt4,
	gpt5,
	gpt6,
	flexspi1,
	flexspi2,
	can1,
	can2,
	can3,
	lpuart1,
	lpuart2,
	lpuart3,
	lpuart4,
	lpuart5,
	lpuart6,
	lpuart7,
	lpuart8,
	lpuart9,
	lpuart10,
	lpuart11,
	lpuart12,
	lpi2c1,
	lpi2c2,
	lpi2c3,
	lpi2c4,
	lpi2c5,
	lpi2c6,
	lpspi1,
	lpspi2,
	lpspi3,
	lpspi4,
	lpspi5,
	lpspi6,
	emv1,
	emv2,
	enet1,
	enet2,
	enet_qos,
	enet_25m,
	enet_timer1,
	enet_timer2,
	enet_timer3,
	usdhc1,
	usdhc2,
	asrc,
	mqs,
	mic,
	spdif,
	sai1,
	sai2,
	sai3,
	sai4,
	gc355,
	lcdif,
	lcdifv2,
	mipi_ref,
	mipi_esc,
	csi2,
	csi2_esc,
	csi2_ui,
	csi,
	cko1,
	cko2,
	end,
};

static const char * const root_clocks[79][8] = {
	{ CLOCK_MUX_DEFAULT, "pll_arm", "pll1_sys", "pll3_sys", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_sys", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_sys", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_sys", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll1_div5", "pll2_sys", "pll2_pfd1", "pll3_pfd0" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_pfd1", "pll2_sys" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd0", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll2_sys", "pll3_div2", "pll1_div5", "pll2_pfd0" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_sys", "pll1_div5", "pll_audio", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll_audio", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll_audio", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd0", "pll2_sys", "pll2_pfd2", "pll3_sys" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd0", "pll2_sys", "pll2_pfd2", "pll3_sys" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll3_pfd2", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll3_pfd2", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1" },
	{ CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1" },
	{ CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1" },
	{ CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1" },
	{ CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1" },
	{ CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1" },
	{ CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll2_pfd0", "pll1_div5", "pll_arm" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll2_pfd0", "pll1_div5", "pll_arm" },
	{ CLOCK_MUX_DEFAULT, "pll1_div5", "pll3_div2", "pll_audio", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll1_div5", "pll3_div2", "pll_audio", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll_audio", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll_audio", "pll3_sys", "pll3_pfd2", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll_audio", "pll3_pfd2", "pll1_div5", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll_audio", "pll3_pfd2", "pll1_div5", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll_audio", "pll3_pfd2", "pll1_div5", "pll2_pfd3" },
	{ CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll_audio", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd1", "pll3_sys", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd2", "pll3_pfd0", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd2", "pll3_pfd0", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd0", "pll3_pfd0", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd0", "pll3_pfd0", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll2_pfd0", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll2_pfd0", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll2_pfd0", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll3_pfd1", "pll_video" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll2_sys", "pll3_pfd1", "pll1_div5" },
	{ CLOCK_MUX_DEFAULT, "pll2_pfd3", "rcosc48M", "pll3_pfd1", "pll_audio"}
};

static const char * const pll_arm_mux[] = {"pll_arm_pre", "osc"};
static const char * const pll3_mux[] = {"pll3_pre", "osc"};
static const char * const pll2_mux[] = {"pll2_pre", "osc"};

static const struct clk_div_table post_div_table[] = {
	{ .val = 3, .div = 1, },
	{ .val = 2, .div = 8, },
	{ .val = 1, .div = 4, },
	{ .val = 0, .div = 2, },
	{ }
};

static int imxrt1170_clocks_probe(struct platform_device *pdev)
{
	void __iomem *ccm_base;
	void __iomem *pll_base;
	struct clk_hw **hws;
	struct clk_hw_onecell_data *clk_hw_data;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *anp;
	int ret;

	clk_hw_data = kzalloc(struct_size(clk_hw_data, hws,
					  IMXRT1170_CLK_END), GFP_KERNEL);
	if (WARN_ON(!clk_hw_data))
		return -ENOMEM;

	clk_hw_data->num = IMXRT1170_CLK_END;
	hws = clk_hw_data->hws;

	hws[IMXRT1170_CLK_OSC] = imx_obtain_fixed_clk_hw(np, "osc");
	hws[IMXRT1170_CLK_RCOSC_16M] = imx_obtain_fixed_clk_hw(np, "rcosc16M");
	hws[IMXRT1170_CLK_OSC_32K] = imx_obtain_fixed_clk_hw(np, "osc32k");

	hws[IMXRT1170_CLK_RCOSC_48M] = imx_clk_hw_fixed_factor("rcosc48M",  "rcosc16M", 3, 1);
	hws[IMXRT1170_CLK_RCOSC_400M] = imx_clk_hw_fixed_factor("rcosc400M",  "rcosc16M", 25, 1);
	hws[IMXRT1170_CLK_RCOSC_48M_DIV2] = imx_clk_hw_fixed_factor("rcosc48M_div2",  "rcosc48M", 1, 2);

	anp = of_find_compatible_node(NULL, NULL, "fsl,imxrt-anatop");
	pll_base = of_iomap(anp, 0);
	of_node_put(anp);
	if (WARN_ON(!pll_base))
		return -ENOMEM;

	/* Anatop clocks */
	hws[IMXRT1170_CLK_DUMMY] = imx_clk_hw_fixed("dummy", 0UL);

	hws[IMXRT1170_CLK_PLL_ARM_PRE] = imx_clk_hw_pllv3(IMX_PLLV3_SYSV2, "pll_arm_pre", "osc", pll_base + 0x200, 0xff);
	imx_clk_hw_pll3_powerbit(hws[IMXRT1170_CLK_PLL_ARM_PRE], 13);
	hws[IMXRT1170_CLK_PLL_ARM_BYPASS] = imx_clk_hw_mux("pll_arm_bypass",
		pll_base + 0x200, 17, 1, pll_arm_mux, 2);
	hws[IMXRT1170_CLK_PLL_ARM_DIV] = clk_hw_register_divider_table(NULL, "pll_arm_div",
		"pll_arm_bypass", CLK_SET_RATE_PARENT | CLK_SET_RATE_GATE,
		pll_base + 0x200, 15, 2, 0, post_div_table, &imx_ccm_lock);
	hws[IMXRT1170_CLK_PLL_ARM] = imx_clk_hw_gate("pll_arm", "pll_arm_div", pll_base + 0x200, 14);

	hws[IMXRT1170_CLK_PLL3_PRE] = imx_clk_hw_pllv3(IMX_PLLV3_GENERICV2, "pll3_pre", "osc", pll_base + 0x210, 0x1);
	imx_clk_hw_pll3_powerbit(hws[IMXRT1170_CLK_PLL3_PRE], 21);
	hws[IMXRT1170_CLK_PLL3_BYPASS] = imx_clk_hw_mux("pll3_bypass",
		pll_base + 0x210, 16, 1, pll3_mux, 2);
	hws[IMXRT1170_CLK_PLL3] = imx_clk_hw_gate("pll3_sys", "pll3_bypass", pll_base + 0x210, 13);

	hws[IMXRT1170_CLK_PLL2_PRE] = imx_clk_hw_pllv3(IMX_PLLV3_GENERICV2, "pll2_pre", "osc", pll_base + 0x240, 0x1);
	imx_clk_hw_pll3_powerbit(hws[IMXRT1170_CLK_PLL2_PRE], 23);
	hws[IMXRT1170_CLK_PLL2_BYPASS] = imx_clk_hw_mux("pll2_bypass",
		pll_base + 0x240, 16, 1, pll2_mux, 2);
	hws[IMXRT1170_CLK_PLL2] = imx_clk_hw_gate("pll2_sys", "pll2_bypass", pll_base + 0x240, 13);

	hws[IMXRT1170_CLK_PLL3_PFD0] = imx_clk_hw_pfd("pll3_pfd0", "pll3_sys", pll_base + 0x230, 0);
	hws[IMXRT1170_CLK_PLL3_PFD1] = imx_clk_hw_pfd("pll3_pfd1", "pll3_sys", pll_base + 0x230, 1);
	hws[IMXRT1170_CLK_PLL3_PFD2] = imx_clk_hw_pfd("pll3_pfd2", "pll3_sys", pll_base + 0x230, 2);
	hws[IMXRT1170_CLK_PLL3_PFD3] = imx_clk_hw_pfd("pll3_pfd3", "pll3_sys", pll_base + 0x230, 3);
	hws[IMXRT1170_CLK_PLL3_DIV2_GATE] = imx_clk_hw_fixed_factor("pll3_div2_gate", "pll3_sys", 1, 2);
	hws[IMXRT1170_CLK_PLL3_DIV2] = imx_clk_hw_gate("pll3_div2", "pll3_sys", pll_base + 0x210, 3);

	hws[IMXRT1170_CLK_PLL2_PFD0] = imx_clk_hw_pfd("pll2_pfd0", "pll2_sys", pll_base + 0x270, 0);
	hws[IMXRT1170_CLK_PLL2_PFD1] = imx_clk_hw_pfd("pll2_pfd1", "pll2_sys", pll_base + 0x270, 1);
	hws[IMXRT1170_CLK_PLL2_PFD2] = imx_clk_hw_pfd("pll2_pfd2", "pll2_sys", pll_base + 0x270, 2);
	hws[IMXRT1170_CLK_PLL2_PFD3] = imx_clk_hw_pfd("pll2_pfd3", "pll2_sys", pll_base + 0x270, 3);

	/* CCM clocks */
	ccm_base = devm_platform_ioremap_resource(pdev, 0);
	if (WARN_ON(IS_ERR(ccm_base)))
		return PTR_ERR(ccm_base);

	hws[IMXRT1170_CLK_M7_SEL] = imx_clk_hw_mux("m7_sel", ccm_base + (m7 * 0x80), 8, 3, root_clocks[m7], 8);
	hws[IMXRT1170_CLK_M4_SEL] = imx_clk_hw_mux("m4_sel", ccm_base + (m4 * 0x80), 8, 3, root_clocks[m4], 8);
	hws[IMXRT1170_CLK_BUS_SEL] = imx_clk_hw_mux("bus_sel", ccm_base + (bus * 0x80), 8, 3, root_clocks[bus], 8);
	hws[IMXRT1170_CLK_BUS_LPSR_SEL] = imx_clk_hw_mux("bus_lpsr_sel", ccm_base + (bus_lpsr * 0x80), 8, 3, root_clocks[bus_lpsr], 8);
	hws[IMXRT1170_CLK_SEMC_SEL] = imx_clk_hw_mux("semc_sel", ccm_base + (semc * 0x80), 8, 3, root_clocks[semc], 8);
	hws[IMXRT1170_CLK_CSSYS_SEL] = imx_clk_hw_mux("cssys_sel", ccm_base + (cssys * 0x80), 8, 3, root_clocks[cssys], 8);
	hws[IMXRT1170_CLK_CSTRACE_SEL] = imx_clk_hw_mux("cstrace_sel", ccm_base + (cstrace * 0x80), 8, 3, root_clocks[cstrace], 8);
	hws[IMXRT1170_CLK_M4_SYSTICK_SEL] = imx_clk_hw_mux("m4_systick_sel", ccm_base + (m4_systick * 0x80), 8, 3, root_clocks[m4_systick], 8);
	hws[IMXRT1170_CLK_M7_SYSTICK_SEL] = imx_clk_hw_mux("m7_systick_sel", ccm_base + (m7_systick * 0x80), 8, 3, root_clocks[m7_systick], 8);
	hws[IMXRT1170_CLK_ADC1_SEL] = imx_clk_hw_mux("adc1_sel", ccm_base + (adc1 * 0x80), 8, 3, root_clocks[adc1], 8);
	hws[IMXRT1170_CLK_ADC2_SEL] = imx_clk_hw_mux("adc2_sel", ccm_base + (adc2 * 0x80), 8, 3, root_clocks[adc2], 8);
	hws[IMXRT1170_CLK_ACMP_SEL] = imx_clk_hw_mux("acmp_sel", ccm_base + (acmp * 0x80), 8, 3, root_clocks[acmp], 8);
	hws[IMXRT1170_CLK_FLEXIO1_SEL] = imx_clk_hw_mux("flexio1_sel", ccm_base + (flexio1 * 0x80), 8, 3, root_clocks[flexio1], 8);
	hws[IMXRT1170_CLK_FLEXIO2_SEL] = imx_clk_hw_mux("flexio2_sel", ccm_base + (flexio2 * 0x80), 8, 3, root_clocks[flexio2], 8);
	hws[IMXRT1170_CLK_GPT1_SEL] = imx_clk_hw_mux("gpt1_sel", ccm_base + (gpt1 * 0x80), 8, 3, root_clocks[gpt1], 8);
	hws[IMXRT1170_CLK_GPT2_SEL] = imx_clk_hw_mux("gpt2_sel", ccm_base + (gpt2 * 0x80), 8, 3, root_clocks[gpt2], 8);
	hws[IMXRT1170_CLK_GPT3_SEL] = imx_clk_hw_mux("gpt3_sel", ccm_base + (gpt3 * 0x80), 8, 3, root_clocks[gpt3], 8);
	hws[IMXRT1170_CLK_GPT4_SEL] = imx_clk_hw_mux("gpt4_sel", ccm_base + (gpt4 * 0x80), 8, 3, root_clocks[gpt4], 8);
	hws[IMXRT1170_CLK_GPT5_SEL] = imx_clk_hw_mux("gpt5_sel", ccm_base + (gpt5 * 0x80), 8, 3, root_clocks[gpt5], 8);
	hws[IMXRT1170_CLK_GPT6_SEL] = imx_clk_hw_mux("gpt6_sel", ccm_base + (gpt6 * 0x80), 8, 3, root_clocks[gpt6], 8);
	hws[IMXRT1170_CLK_FLEXSPI1_SEL] = imx_clk_hw_mux("flexspi1_sel", ccm_base + (flexspi1 * 0x80), 8, 3, root_clocks[flexspi1], 8);
	hws[IMXRT1170_CLK_FLEXSPI2_SEL] = imx_clk_hw_mux("flexspi2_sel", ccm_base + (flexspi2 * 0x80), 8, 3, root_clocks[flexspi2], 8);
	hws[IMXRT1170_CLK_CAN1_SEL] = imx_clk_hw_mux("can1_sel", ccm_base + (can1 * 0x80), 8, 3, root_clocks[can1], 8);
	hws[IMXRT1170_CLK_CAN2_SEL] = imx_clk_hw_mux("can2_sel", ccm_base + (can2 * 0x80), 8, 3, root_clocks[can2], 8);
	hws[IMXRT1170_CLK_CAN3_SEL] = imx_clk_hw_mux("can3_sel", ccm_base + (can3 * 0x80), 8, 3, root_clocks[can3], 8);
	hws[IMXRT1170_CLK_LPUART1_SEL] = imx_clk_hw_mux("lpuart1_sel", ccm_base + (lpuart1 * 0x80), 8, 3, root_clocks[lpuart1], 8);
	hws[IMXRT1170_CLK_LPUART2_SEL] = imx_clk_hw_mux("lpuart2_sel", ccm_base + (lpuart2 * 0x80), 8, 3, root_clocks[lpuart2], 8);
	hws[IMXRT1170_CLK_LPUART3_SEL] = imx_clk_hw_mux("lpuart3_sel", ccm_base + (lpuart3 * 0x80), 8, 3, root_clocks[lpuart3], 8);
	hws[IMXRT1170_CLK_LPUART4_SEL] = imx_clk_hw_mux("lpuart4_sel", ccm_base + (lpuart4 * 0x80), 8, 3, root_clocks[lpuart4], 8);
	hws[IMXRT1170_CLK_LPUART5_SEL] = imx_clk_hw_mux("lpuart5_sel", ccm_base + (lpuart5 * 0x80), 8, 3, root_clocks[lpuart5], 8);
	hws[IMXRT1170_CLK_LPUART6_SEL] = imx_clk_hw_mux("lpuart6_sel", ccm_base + (lpuart6 * 0x80), 8, 3, root_clocks[lpuart6], 8);
	hws[IMXRT1170_CLK_LPUART7_SEL] = imx_clk_hw_mux("lpuart7_sel", ccm_base + (lpuart7 * 0x80), 8, 3, root_clocks[lpuart7], 8);
	hws[IMXRT1170_CLK_LPUART8_SEL] = imx_clk_hw_mux("lpuart8_sel", ccm_base + (lpuart8 * 0x80), 8, 3, root_clocks[lpuart8], 8);
	hws[IMXRT1170_CLK_LPUART9_SEL] = imx_clk_hw_mux("lpuart9_sel", ccm_base + (lpuart9 * 0x80), 8, 3, root_clocks[lpuart9], 8);
	hws[IMXRT1170_CLK_LPUART10_SEL] = imx_clk_hw_mux("lpuart10_sel", ccm_base + (lpuart10 * 0x80), 8, 3, root_clocks[lpuart10], 8);
	hws[IMXRT1170_CLK_LPUART11_SEL] = imx_clk_hw_mux("lpuart11_sel", ccm_base + (lpuart11 * 0x80), 8, 3, root_clocks[lpuart11], 8);
	hws[IMXRT1170_CLK_LPUART12_SEL] = imx_clk_hw_mux("lpuart12_sel", ccm_base + (lpuart12 * 0x80), 8, 3, root_clocks[lpuart12], 8);
	hws[IMXRT1170_CLK_LPI2C1_SEL] = imx_clk_hw_mux("lpi2c1_sel", ccm_base + (lpi2c1 * 0x80), 8, 3, root_clocks[lpi2c1], 8);
	hws[IMXRT1170_CLK_LPI2C2_SEL] = imx_clk_hw_mux("lpi2c2_sel", ccm_base + (lpi2c2 * 0x80), 8, 3, root_clocks[lpi2c2], 8);
	hws[IMXRT1170_CLK_LPI2C3_SEL] = imx_clk_hw_mux("lpi2c3_sel", ccm_base + (lpi2c3 * 0x80), 8, 3, root_clocks[lpi2c3], 8);
	hws[IMXRT1170_CLK_LPI2C4_SEL] = imx_clk_hw_mux("lpi2c4_sel", ccm_base + (lpi2c4 * 0x80), 8, 3, root_clocks[lpi2c4], 8);
	hws[IMXRT1170_CLK_LPI2C5_SEL] = imx_clk_hw_mux("lpi2c5_sel", ccm_base + (lpi2c5 * 0x80), 8, 3, root_clocks[lpi2c5], 8);
	hws[IMXRT1170_CLK_LPI2C6_SEL] = imx_clk_hw_mux("lpi2c6_sel", ccm_base + (lpi2c6 * 0x80), 8, 3, root_clocks[lpi2c6], 8);
	hws[IMXRT1170_CLK_LPSPI1_SEL] = imx_clk_hw_mux("lpspi1_sel", ccm_base + (lpspi1 * 0x80), 8, 3, root_clocks[lpspi1], 8);
	hws[IMXRT1170_CLK_LPSPI2_SEL] = imx_clk_hw_mux("lpspi2_sel", ccm_base + (lpspi2 * 0x80), 8, 3, root_clocks[lpspi2], 8);
	hws[IMXRT1170_CLK_LPSPI3_SEL] = imx_clk_hw_mux("lpspi3_sel", ccm_base + (lpspi3 * 0x80), 8, 3, root_clocks[lpspi3], 8);
	hws[IMXRT1170_CLK_LPSPI4_SEL] = imx_clk_hw_mux("lpspi4_sel", ccm_base + (lpspi4 * 0x80), 8, 3, root_clocks[lpspi4], 8);
	hws[IMXRT1170_CLK_LPSPI5_SEL] = imx_clk_hw_mux("lpspi5_sel", ccm_base + (lpspi5 * 0x80), 8, 3, root_clocks[lpspi5], 8);
	hws[IMXRT1170_CLK_LPSPI6_SEL] = imx_clk_hw_mux("lpspi6_sel", ccm_base + (lpspi6 * 0x80), 8, 3, root_clocks[lpspi6], 8);
	hws[IMXRT1170_CLK_EMV1_SEL] = imx_clk_hw_mux("emv1_sel", ccm_base + (emv1 * 0x80), 8, 3, root_clocks[emv1], 8);
	hws[IMXRT1170_CLK_EMV2_SEL] = imx_clk_hw_mux("emv2_sel", ccm_base + (emv2 * 0x80), 8, 3, root_clocks[emv2], 8);
	hws[IMXRT1170_CLK_ENET1_SEL] = imx_clk_hw_mux("enet1_sel", ccm_base + (enet1 * 0x80), 8, 3, root_clocks[enet1], 8);
	hws[IMXRT1170_CLK_ENET2_SEL] = imx_clk_hw_mux("enet2_sel", ccm_base + (enet2 * 0x80), 8, 3, root_clocks[enet2], 8);
	hws[IMXRT1170_CLK_ENET_QOS_SEL] = imx_clk_hw_mux("enet_qos_sel", ccm_base + (enet_qos * 0x80), 8, 3, root_clocks[enet_qos], 8);
	hws[IMXRT1170_CLK_ENET_25M_SEL] = imx_clk_hw_mux("enet_25m_sel", ccm_base + (enet_25m * 0x80), 8, 3, root_clocks[enet_25m], 8);
	hws[IMXRT1170_CLK_ENET_TIMER1_SEL] = imx_clk_hw_mux("enet_timer1_sel", ccm_base + (enet_timer1 * 0x80), 8, 3, root_clocks[enet_timer1], 8);
	hws[IMXRT1170_CLK_ENET_TIMER2_SEL] = imx_clk_hw_mux("enet_timer2_sel", ccm_base + (enet_timer2 * 0x80), 8, 3, root_clocks[enet_timer2], 8);
	hws[IMXRT1170_CLK_ENET_TIMER3_SEL] = imx_clk_hw_mux("enet_timer3_sel", ccm_base + (enet_timer3 * 0x80), 8, 3, root_clocks[enet_timer3], 8);
	hws[IMXRT1170_CLK_USDHC1_SEL] = imx_clk_hw_mux("usdhc1_sel", ccm_base + (usdhc1 * 0x80), 8, 3, root_clocks[usdhc1], 8);
	hws[IMXRT1170_CLK_USDHC2_SEL] = imx_clk_hw_mux("usdhc2_sel", ccm_base + (usdhc2 * 0x80), 8, 3, root_clocks[usdhc2], 8);
	hws[IMXRT1170_CLK_ASRC_SEL] = imx_clk_hw_mux("asrc_sel", ccm_base + (asrc * 0x80), 8, 3, root_clocks[asrc], 8);
	hws[IMXRT1170_CLK_MQS_SEL] = imx_clk_hw_mux("mqs_sel", ccm_base + (mqs * 0x80), 8, 3, root_clocks[mqs], 8);
	hws[IMXRT1170_CLK_MIC_SEL] = imx_clk_hw_mux("mic_sel", ccm_base + (mic * 0x80), 8, 3, root_clocks[mic], 8);
	hws[IMXRT1170_CLK_SPDIF_SEL] = imx_clk_hw_mux("spdif_sel", ccm_base + (spdif * 0x80), 8, 3, root_clocks[spdif], 8);
	hws[IMXRT1170_CLK_SAI1_SEL] = imx_clk_hw_mux("sai1_sel", ccm_base + (sai1 * 0x80), 8, 3, root_clocks[sai1], 8);
	hws[IMXRT1170_CLK_SAI2_SEL] = imx_clk_hw_mux("sai2_sel", ccm_base + (sai2 * 0x80), 8, 3, root_clocks[sai2], 8);
	hws[IMXRT1170_CLK_SAI3_SEL] = imx_clk_hw_mux("sai3_sel", ccm_base + (sai3 * 0x80), 8, 3, root_clocks[sai3], 8);
	hws[IMXRT1170_CLK_SAI4_SEL] = imx_clk_hw_mux("sai4_sel", ccm_base + (sai4 * 0x80), 8, 3, root_clocks[sai4], 8);
	hws[IMXRT1170_CLK_GC355_SEL] = imx_clk_hw_mux("gc355_sel", ccm_base + (gc355 * 0x80), 8, 3, root_clocks[gc355], 8);
	hws[IMXRT1170_CLK_LCDIF_SEL] = imx_clk_hw_mux("lcdif_sel", ccm_base + (lcdif * 0x80), 8, 3, root_clocks[lcdif], 8);
	hws[IMXRT1170_CLK_LCDIFV2_SEL] = imx_clk_hw_mux("lcdifv2_sel", ccm_base + (lcdifv2 * 0x80), 8, 3, root_clocks[lcdifv2], 8);
	hws[IMXRT1170_CLK_MIPI_REF_SEL] = imx_clk_hw_mux("mipi_ref_sel", ccm_base + (mipi_ref * 0x80), 8, 3, root_clocks[mipi_ref], 8);
	hws[IMXRT1170_CLK_MIPI_ESC_SEL] = imx_clk_hw_mux("mipi_esc_sel", ccm_base + (mipi_esc * 0x80), 8, 3, root_clocks[mipi_esc], 8);
	hws[IMXRT1170_CLK_CSI2_SEL] = imx_clk_hw_mux("csi2_sel", ccm_base + (csi2 * 0x80), 8, 3, root_clocks[csi2], 8);
	hws[IMXRT1170_CLK_CSI2_ESC_SEL] = imx_clk_hw_mux("csi2_esc_sel", ccm_base + (csi2_esc * 0x80), 8, 3, root_clocks[csi2_esc], 8);
	hws[IMXRT1170_CLK_CSI2_UI_SEL] = imx_clk_hw_mux("csi2_ui_sel", ccm_base + (csi2_ui * 0x80), 8, 3, root_clocks[csi2_ui], 8);
	hws[IMXRT1170_CLK_CSI_SEL] = imx_clk_hw_mux("csi_sel", ccm_base + (csi * 0x80), 8, 3, root_clocks[csi], 8);
	hws[IMXRT1170_CLK_CKO1_SEL] = imx_clk_hw_mux("cko1_sel", ccm_base + (cko1 * 0x80), 8, 3, root_clocks[cko1], 8);
	hws[IMXRT1170_CLK_CKO2_SEL] = imx_clk_hw_mux("cko2_sel", ccm_base + (cko2 * 0x80), 8, 3, root_clocks[cko2], 8);

	hws[IMXRT1170_CLK_M7_GATE] = imx_clk_hw_gate_dis_flags("m7_gate", "m7_sel", ccm_base + (m7 * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_M4_GATE] = imx_clk_hw_gate_dis_flags("m4_gate", "m4_sel", ccm_base + (m4 * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_BUS_GATE] = imx_clk_hw_gate_dis_flags("bus_gate", "bus_sel", ccm_base + (bus * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_BUS_LPSR_GATE] = imx_clk_hw_gate_dis_flags("bus_lpsr_gate", "bus_lpsr_sel", ccm_base + (bus_lpsr * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_SEMC_GATE] = imx_clk_hw_gate_dis_flags("semc_gate", "semc_sel", ccm_base + (semc * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_CSSYS_GATE] = imx_clk_hw_gate_dis_flags("cssys_gate", "cssys_sel", ccm_base + (cssys * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_CSTRACE_GATE] = imx_clk_hw_gate_dis_flags("cstrace_gate", "cstrace_sel", ccm_base + (cstrace * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_M4_SYSTICK_GATE] = imx_clk_hw_gate_dis_flags("m4_systick_gate", "m4_systick_sel", ccm_base + (m4_systick * 0x80), 24, 0);
	hws[IMXRT1170_CLK_M7_SYSTICK_GATE] = imx_clk_hw_gate_dis_flags("m7_systick_gate", "m7_systick_sel", ccm_base + (m7_systick * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ADC1_GATE] = imx_clk_hw_gate_dis_flags("adc1_gate", "adc1_sel", ccm_base + (adc1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ADC2_GATE] = imx_clk_hw_gate_dis_flags("adc2_gate", "adc2_sel", ccm_base + (adc2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ACMP_GATE] = imx_clk_hw_gate_dis_flags("acmp_gate", "acmp_sel", ccm_base + (acmp * 0x80), 24, 0);
	hws[IMXRT1170_CLK_FLEXIO1_GATE] = imx_clk_hw_gate_dis_flags("flexio1_gate", "flexio1_sel", ccm_base + (flexio1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_FLEXIO2_GATE] = imx_clk_hw_gate_dis_flags("flexio2_gate", "flexio2_sel", ccm_base + (flexio2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_GPT1_GATE] = imx_clk_hw_gate_dis_flags("gpt1_gate", "gpt1_sel", ccm_base + (gpt1 * 0x80), 24, CLK_IS_CRITICAL);
	hws[IMXRT1170_CLK_GPT2_GATE] = imx_clk_hw_gate_dis_flags("gpt2_gate", "gpt2_sel", ccm_base + (gpt2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_GPT3_GATE] = imx_clk_hw_gate_dis_flags("gpt3_gate", "gpt3_sel", ccm_base + (gpt3 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_GPT4_GATE] = imx_clk_hw_gate_dis_flags("gpt4_gate", "gpt4_sel", ccm_base + (gpt4 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_GPT5_GATE] = imx_clk_hw_gate_dis_flags("gpt5_gate", "gpt5_sel", ccm_base + (gpt5 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_GPT6_GATE] = imx_clk_hw_gate_dis_flags("gpt6_gate", "gpt6_sel", ccm_base + (gpt6 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_FLEXSPI1_GATE] = imx_clk_hw_gate_dis_flags("flexspi1_gate", "flexspi1_sel", ccm_base + (flexspi1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_FLEXSPI2_GATE] = imx_clk_hw_gate_dis_flags("flexspi2_gate", "flexspi2_sel", ccm_base + (flexspi2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CAN1_GATE] = imx_clk_hw_gate_dis_flags("can1_gate", "can1_sel", ccm_base + (can1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CAN2_GATE] = imx_clk_hw_gate_dis_flags("can2_gate", "can2_sel", ccm_base + (can2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CAN3_GATE] = imx_clk_hw_gate_dis_flags("can3_gate", "can3_sel", ccm_base + (can3 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART1_GATE] = imx_clk_hw_gate_dis_flags("lpuart1_gate", "lpuart1_sel", ccm_base + (lpuart1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART2_GATE] = imx_clk_hw_gate_dis_flags("lpuart2_gate", "lpuart2_sel", ccm_base + (lpuart2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART3_GATE] = imx_clk_hw_gate_dis_flags("lpuart3_gate", "lpuart3_sel", ccm_base + (lpuart3 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART4_GATE] = imx_clk_hw_gate_dis_flags("lpuart4_gate", "lpuart4_sel", ccm_base + (lpuart4 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART5_GATE] = imx_clk_hw_gate_dis_flags("lpuart5_gate", "lpuart5_sel", ccm_base + (lpuart5 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART6_GATE] = imx_clk_hw_gate_dis_flags("lpuart6_gate", "lpuart6_sel", ccm_base + (lpuart6 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART7_GATE] = imx_clk_hw_gate_dis_flags("lpuart7_gate", "lpuart7_sel", ccm_base + (lpuart7 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART8_GATE] = imx_clk_hw_gate_dis_flags("lpuart8_gate", "lpuart8_sel", ccm_base + (lpuart8 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART9_GATE] = imx_clk_hw_gate_dis_flags("lpuart9_gate", "lpuart9_sel", ccm_base + (lpuart9 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART10_GATE] = imx_clk_hw_gate_dis_flags("lpuart10_gate", "lpuart10_sel", ccm_base + (lpuart10 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART11_GATE] = imx_clk_hw_gate_dis_flags("lpuart11_gate", "lpuart11_sel", ccm_base + (lpuart11 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPUART12_GATE] = imx_clk_hw_gate_dis_flags("lpuart12_gate", "lpuart12_sel", ccm_base + (lpuart12 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPI2C1_GATE] = imx_clk_hw_gate_dis_flags("lpi2c1_gate", "lpi2c1_sel", ccm_base + (lpi2c1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPI2C2_GATE] = imx_clk_hw_gate_dis_flags("lpi2c2_gate", "lpi2c2_sel", ccm_base + (lpi2c2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPI2C3_GATE] = imx_clk_hw_gate_dis_flags("lpi2c3_gate", "lpi2c3_sel", ccm_base + (lpi2c3 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPI2C4_GATE] = imx_clk_hw_gate_dis_flags("lpi2c4_gate", "lpi2c4_sel", ccm_base + (lpi2c4 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPI2C5_GATE] = imx_clk_hw_gate_dis_flags("lpi2c5_gate", "lpi2c5_sel", ccm_base + (lpi2c5 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPI2C6_GATE] = imx_clk_hw_gate_dis_flags("lpi2c6_gate", "lpi2c6_sel", ccm_base + (lpi2c6 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPSPI1_GATE] = imx_clk_hw_gate_dis_flags("lpspi1_gate", "lpspi1_sel", ccm_base + (lpspi1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPSPI2_GATE] = imx_clk_hw_gate_dis_flags("lpspi2_gate", "lpspi2_sel", ccm_base + (lpspi2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPSPI3_GATE] = imx_clk_hw_gate_dis_flags("lpspi3_gate", "lpspi3_sel", ccm_base + (lpspi3 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPSPI4_GATE] = imx_clk_hw_gate_dis_flags("lpspi4_gate", "lpspi4_sel", ccm_base + (lpspi4 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPSPI5_GATE] = imx_clk_hw_gate_dis_flags("lpspi5_gate", "lpspi5_sel", ccm_base + (lpspi5 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LPSPI6_GATE] = imx_clk_hw_gate_dis_flags("lpspi6_gate", "lpspi6_sel", ccm_base + (lpspi6 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_EMV1_GATE] = imx_clk_hw_gate_dis_flags("emv1_gate", "emv1_sel", ccm_base + (emv1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_EMV2_GATE] = imx_clk_hw_gate_dis_flags("emv2_gate", "emv2_sel", ccm_base + (emv2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ENET1_GATE] = imx_clk_hw_gate_dis_flags("enet1_gate", "enet1_sel", ccm_base + (enet1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ENET2_GATE] = imx_clk_hw_gate_dis_flags("enet2_gate", "enet2_sel", ccm_base + (enet2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ENET_QOS_GATE] = imx_clk_hw_gate_dis_flags("enet_qos_gate", "enet_qos_sel", ccm_base + (enet_qos * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ENET_25M_GATE] = imx_clk_hw_gate_dis_flags("enet_25m_gate", "enet_25m_sel", ccm_base + (enet_25m * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ENET_TIMER1_GATE] = imx_clk_hw_gate_dis_flags("enet_timer1_gate", "enet_timer1_sel", ccm_base + (enet_timer1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ENET_TIMER2_GATE] = imx_clk_hw_gate_dis_flags("enet_timer2_gate", "enet_timer2_sel", ccm_base + (enet_timer2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ENET_TIMER3_GATE] = imx_clk_hw_gate_dis_flags("enet_timer3_gate", "enet_timer3_sel", ccm_base + (enet_timer3 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_USDHC1_GATE] = imx_clk_hw_gate_dis_flags("usdhc1_gate", "usdhc1_sel", ccm_base + (usdhc1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_USDHC2_GATE] = imx_clk_hw_gate_dis_flags("usdhc2_gate", "usdhc2_sel", ccm_base + (usdhc2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_ASRC_GATE] = imx_clk_hw_gate_dis_flags("asrc_gate", "asrc_sel", ccm_base + (asrc * 0x80), 24, 0);
	hws[IMXRT1170_CLK_MQS_GATE] = imx_clk_hw_gate_dis_flags("mqs_gate", "mqs_sel", ccm_base + (mqs * 0x80), 24, 0);
	hws[IMXRT1170_CLK_MIC_GATE] = imx_clk_hw_gate_dis_flags("mic_gate", "mic_sel", ccm_base + (mic * 0x80), 24, 0);
	hws[IMXRT1170_CLK_SPDIF_GATE] = imx_clk_hw_gate_dis_flags("spdif_gate", "spdif_sel", ccm_base + (spdif * 0x80), 24, 0);
	hws[IMXRT1170_CLK_SAI1_GATE] = imx_clk_hw_gate_dis_flags("sai1_gate", "sai1_sel", ccm_base + (sai1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_SAI2_GATE] = imx_clk_hw_gate_dis_flags("sai2_gate", "sai2_sel", ccm_base + (sai2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_SAI3_GATE] = imx_clk_hw_gate_dis_flags("sai3_gate", "sai3_sel", ccm_base + (sai3 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_SAI4_GATE] = imx_clk_hw_gate_dis_flags("sai4_gate", "sai4_sel", ccm_base + (sai4 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_GC355_GATE] = imx_clk_hw_gate_dis_flags("gc355_gate", "gc355_sel", ccm_base + (gc355 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LCDIF_GATE] = imx_clk_hw_gate_dis_flags("lcdif_gate", "lcdif_sel", ccm_base + (lcdif * 0x80), 24, 0);
	hws[IMXRT1170_CLK_LCDIFV2_GATE] = imx_clk_hw_gate_dis_flags("lcdifv2_gate", "lcdifv2_sel", ccm_base + (lcdifv2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_MIPI_REF_GATE] = imx_clk_hw_gate_dis_flags("mipi_ref_gate", "mipi_ref_sel", ccm_base + (mipi_ref * 0x80), 24, 0);
	hws[IMXRT1170_CLK_MIPI_ESC_GATE] = imx_clk_hw_gate_dis_flags("mipi_esc_gate", "mipi_esc_sel", ccm_base + (mipi_esc * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CSI2_GATE] = imx_clk_hw_gate_dis_flags("csi2_gate", "csi2_sel", ccm_base + (csi2 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CSI2_ESC_GATE] = imx_clk_hw_gate_dis_flags("csi2_esc_gate", "csi2_esc_sel", ccm_base + (csi2_esc * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CSI2_UI_GATE] = imx_clk_hw_gate_dis_flags("csi2_ui_gate", "csi2_ui_sel", ccm_base + (csi2_ui * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CSI_GATE] = imx_clk_hw_gate_dis_flags("csi_gate", "csi_sel", ccm_base + (csi * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CKO1_GATE] = imx_clk_hw_gate_dis_flags("cko1_gate", "cko1_sel", ccm_base + (cko1 * 0x80), 24, 0);
	hws[IMXRT1170_CLK_CKO2_GATE] = imx_clk_hw_gate_dis_flags("cko2_gate", "cko2_sel", ccm_base + (cko2 * 0x80), 24, 0);

	hws[IMXRT1170_CLK_M7] = imx_clk_hw_divider("m7", "m7_gate", ccm_base + (m7 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_M4] = imx_clk_hw_divider("m4", "m4_gate", ccm_base + (m4 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_BUS] = imx_clk_hw_divider("bus", "bus_gate", ccm_base + (bus * 0x80), 0, 8);
	hws[IMXRT1170_CLK_BUS_LPSR] = imx_clk_hw_divider("bus_lpsr", "bus_lpsr_gate", ccm_base + (bus_lpsr * 0x80), 0, 8);
	hws[IMXRT1170_CLK_SEMC] = imx_clk_hw_divider("semc", "semc_gate", ccm_base + (semc * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CSSYS] = imx_clk_hw_divider("cssys", "cssys_gate", ccm_base + (cssys * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CSTRACE] = imx_clk_hw_divider("cstrace", "cstrace_gate", ccm_base + (cstrace * 0x80), 0, 8);
	hws[IMXRT1170_CLK_M4_SYSTICK] = imx_clk_hw_divider("m4_systick", "m4_systick_gate", ccm_base + (m4_systick * 0x80), 0, 8);
	hws[IMXRT1170_CLK_M7_SYSTICK] = imx_clk_hw_divider("m7_systick", "m7_systick_gate", ccm_base + (m7_systick * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ADC1] = imx_clk_hw_divider("adc1", "adc1_gate", ccm_base + (adc1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ADC2] = imx_clk_hw_divider("adc2", "adc2_gate", ccm_base + (adc2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ACMP] = imx_clk_hw_divider("acmp", "acmp_gate", ccm_base + (acmp * 0x80), 0, 8);
	hws[IMXRT1170_CLK_FLEXIO1] = imx_clk_hw_divider("flexio1", "flexio1_gate", ccm_base + (flexio1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_FLEXIO2] = imx_clk_hw_divider("flexio2", "flexio2_gate", ccm_base + (flexio2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_GPT1] = imx_clk_hw_divider("gpt1", "gpt1_gate", ccm_base + (gpt1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_GPT2] = imx_clk_hw_divider("gpt2", "gpt2_gate", ccm_base + (gpt2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_GPT3] = imx_clk_hw_divider("gpt3", "gpt3_gate", ccm_base + (gpt3 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_GPT4] = imx_clk_hw_divider("gpt4", "gpt4_gate", ccm_base + (gpt4 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_GPT5] = imx_clk_hw_divider("gpt5", "gpt5_gate", ccm_base + (gpt5 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_GPT6] = imx_clk_hw_divider("gpt6", "gpt6_gate", ccm_base + (gpt6 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_FLEXSPI1] = imx_clk_hw_divider("flexspi1", "flexspi1_gate", ccm_base + (flexspi1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_FLEXSPI2] = imx_clk_hw_divider("flexspi2", "flexspi2_gate", ccm_base + (flexspi2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CAN1] = imx_clk_hw_divider("can1", "can1_gate", ccm_base + (can1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CAN2] = imx_clk_hw_divider("can2", "can2_gate", ccm_base + (can2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CAN3] = imx_clk_hw_divider("can3", "can3_gate", ccm_base + (can3 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART1] = imx_clk_hw_divider("lpuart1", "lpuart1_gate", ccm_base + (lpuart1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART2] = imx_clk_hw_divider("lpuart2", "lpuart2_gate", ccm_base + (lpuart2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART3] = imx_clk_hw_divider("lpuart3", "lpuart3_gate", ccm_base + (lpuart3 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART4] = imx_clk_hw_divider("lpuart4", "lpuart4_gate", ccm_base + (lpuart4 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART5] = imx_clk_hw_divider("lpuart5", "lpuart5_gate", ccm_base + (lpuart5 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART6] = imx_clk_hw_divider("lpuart6", "lpuart6_gate", ccm_base + (lpuart6 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART7] = imx_clk_hw_divider("lpuart7", "lpuart7_gate", ccm_base + (lpuart7 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART8] = imx_clk_hw_divider("lpuart8", "lpuart8_gate", ccm_base + (lpuart8 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART9] = imx_clk_hw_divider("lpuart9", "lpuart9_gate", ccm_base + (lpuart9 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART10] = imx_clk_hw_divider("lpuart10", "lpuart10_gate", ccm_base + (lpuart10 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART11] = imx_clk_hw_divider("lpuart11", "lpuart11_gate", ccm_base + (lpuart11 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPUART12] = imx_clk_hw_divider("lpuart12", "lpuart12_gate", ccm_base + (lpuart12 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPI2C1] = imx_clk_hw_divider("lpi2c1", "lpi2c1_gate", ccm_base + (lpi2c1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPI2C2] = imx_clk_hw_divider("lpi2c2", "lpi2c2_gate", ccm_base + (lpi2c2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPI2C3] = imx_clk_hw_divider("lpi2c3", "lpi2c3_gate", ccm_base + (lpi2c3 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPI2C4] = imx_clk_hw_divider("lpi2c4", "lpi2c4_gate", ccm_base + (lpi2c4 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPI2C5] = imx_clk_hw_divider("lpi2c5", "lpi2c5_gate", ccm_base + (lpi2c5 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPI2C6] = imx_clk_hw_divider("lpi2c6", "lpi2c6_gate", ccm_base + (lpi2c6 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPSPI1] = imx_clk_hw_divider("lpspi1", "lpspi1_gate", ccm_base + (lpspi1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPSPI2] = imx_clk_hw_divider("lpspi2", "lpspi2_gate", ccm_base + (lpspi2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPSPI3] = imx_clk_hw_divider("lpspi3", "lpspi3_gate", ccm_base + (lpspi3 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPSPI4] = imx_clk_hw_divider("lpspi4", "lpspi4_gate", ccm_base + (lpspi4 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPSPI5] = imx_clk_hw_divider("lpspi5", "lpspi5_gate", ccm_base + (lpspi5 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LPSPI6] = imx_clk_hw_divider("lpspi6", "lpspi6_gate", ccm_base + (lpspi6 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_EMV1] = imx_clk_hw_divider("emv1", "emv1_gate", ccm_base + (emv1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_EMV2] = imx_clk_hw_divider("emv2", "emv2_gate", ccm_base + (emv2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ENET1] = imx_clk_hw_divider("enet1", "enet1_gate", ccm_base + (enet1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ENET2] = imx_clk_hw_divider("enet2", "enet2_gate", ccm_base + (enet2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ENET_QOS] = imx_clk_hw_divider("enet_qos", "enet_qos_gate", ccm_base + (enet_qos * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ENET_25M] = imx_clk_hw_divider("enet_25m", "enet_25m_gate", ccm_base + (enet_25m * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ENET_TIMER1] = imx_clk_hw_divider("enet_timer1", "enet_timer1_gate", ccm_base + (enet_timer1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ENET_TIMER2] = imx_clk_hw_divider("enet_timer2", "enet_timer2_gate", ccm_base + (enet_timer2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ENET_TIMER3] = imx_clk_hw_divider("enet_timer3", "enet_timer3_gate", ccm_base + (enet_timer3 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_USDHC1] = imx_clk_hw_divider("usdhc1", "usdhc1_gate", ccm_base + (usdhc1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_USDHC2] = imx_clk_hw_divider("usdhc2", "usdhc2_gate", ccm_base + (usdhc2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_ASRC] = imx_clk_hw_divider("asrc", "asrc_gate", ccm_base + (asrc * 0x80), 0, 8);
	hws[IMXRT1170_CLK_MQS] = imx_clk_hw_divider("mqs", "mqs_gate", ccm_base + (mqs * 0x80), 0, 8);
	hws[IMXRT1170_CLK_MIC] = imx_clk_hw_divider("mic", "mic_gate", ccm_base + (mic * 0x80), 0, 8);
	hws[IMXRT1170_CLK_SPDIF] = imx_clk_hw_divider("spdif", "spdif_gate", ccm_base + (spdif * 0x80), 0, 8);
	hws[IMXRT1170_CLK_SAI1] = imx_clk_hw_divider("sai1", "sai1_gate", ccm_base + (sai1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_SAI2] = imx_clk_hw_divider("sai2", "sai2_gate", ccm_base + (sai2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_SAI3] = imx_clk_hw_divider("sai3", "sai3_gate", ccm_base + (sai3 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_SAI4] = imx_clk_hw_divider("sai4", "sai4_gate", ccm_base + (sai4 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_GC355] = imx_clk_hw_divider("gc355", "gc355_gate", ccm_base + (gc355 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LCDIF] = imx_clk_hw_divider("lcdif", "lcdif_gate", ccm_base + (lcdif * 0x80), 0, 8);
	hws[IMXRT1170_CLK_LCDIFV2] = imx_clk_hw_divider("lcdifv2", "lcdifv2_gate", ccm_base + (lcdifv2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_MIPI_REF] = imx_clk_hw_divider("mipi_ref", "mipi_ref_gate", ccm_base + (mipi_ref * 0x80), 0, 8);
	hws[IMXRT1170_CLK_MIPI_ESC] = imx_clk_hw_divider("mipi_esc", "mipi_esc_gate", ccm_base + (mipi_esc * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CSI2] = imx_clk_hw_divider("csi2", "csi2_gate", ccm_base + (csi2 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CSI2_ESC] = imx_clk_hw_divider("csi2_esc", "csi2_esc_gate", ccm_base + (csi2_esc * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CSI2_UI] = imx_clk_hw_divider("csi2_ui", "csi2_ui_gate", ccm_base + (csi2_ui * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CSI] = imx_clk_hw_divider("csi", "csi_gate", ccm_base + (csi * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CKO1] = imx_clk_hw_divider("cko1", "cko1_gate", ccm_base + (cko1 * 0x80), 0, 8);
	hws[IMXRT1170_CLK_CKO2] = imx_clk_hw_divider("cko2", "cko2_gate", ccm_base + (cko2 * 0x80), 0, 8);

	hws[IMXRT1170_CLK_USB] = imx_clk_hw_gate("usb", "bus", ccm_base + LPCG_GATE(115), 0);

	imx_check_clk_hws(hws, IMXRT1170_CLK_END);

	ret = of_clk_add_hw_provider(np, of_clk_hw_onecell_get, clk_hw_data);
	if (ret < 0) {
		dev_err(dev, "Failed to register clks for i.MXRT1170.\n");
		imx_unregister_hw_clocks(hws, IMXRT1170_CLK_END);
	}
	return ret;
}

static const struct of_device_id imxrt1170_clk_of_match[] = {
	{ .compatible = "fsl,imxrt1170-ccm" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, imxrt1170_clk_of_match);

static struct platform_driver imxrt1170_clk_driver = {
	.probe = imxrt1170_clocks_probe,
	.driver = {
		.name = "imxrt1170-ccm",
		.of_match_table = imxrt1170_clk_of_match,
	},
};
module_platform_driver(imxrt1170_clk_driver);
