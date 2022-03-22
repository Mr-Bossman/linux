// SPDX-License-Identifier: (GPL-2.0+ OR BSD-3-Clause)
/*
 * Copyright (C) 2022
 * Author(s):
 * Jesse Taube <Mr.Bossman075@gmail.com>
 */
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <dt-bindings/clock/imxrt1170-clock.h>

#include "clk.h"

#define CLOCK_MUX_DEFAULT "rcosc48M_div2", "osc", "rcosc400M", "rcosc16M"

#define LPCG_GATE(gate) (0x6000 + (gate * 0x20))

#define DEF_CLOCK(flags, macro, name) \
do { \
	hws[macro##_SEL] = imx_clk_hw_mux(#name"_sel", ccm_base + (name * 0x80), \
		8, 3, root_clocks[name], 8); \
	hws[macro##_GATE] = imx_clk_hw_gate_dis_flags(#name"_gate", #name"_sel", \
		ccm_base + (name * 0x80), 24, flags); \
	hws[macro] = imx_clk_hw_divider(#name, #name"_gate", ccm_base + (name * 0x80), 0, 8); \
} while (0)

enum root_clock_names {
	m7,		/* root clock m7. */
	m4,		/* root clock m4. */
	bus,		/* root clock bus. */
	bus_lpsr,	/* root clock bus lpsr. */
	semc,		/* root clock semc. */
	cssys,		/* root clock cssys. */
	cstrace,	/* root clock cstrace. */
	m4_systick,	/* root clock m4 systick. */
	m7_systick,	/* root clock m7 systick. */
	adc1,		/* root clock adc1. */
	adc2,		/* root clock adc2. */
	acmp,		/* root clock acmp. */
	flexio1,	/* root clock flexio1. */
	flexio2,	/* root clock flexio2. */
	gpt1,		/* root clock gpt1. */
	gpt2,		/* root clock gpt2. */
	gpt3,		/* root clock gpt3. */
	gpt4,		/* root clock gpt4. */
	gpt5,		/* root clock gpt5. */
	gpt6,		/* root clock gpt6. */
	flexspi1,	/* root clock flexspi1. */
	flexspi2,	/* root clock flexspi2. */
	can1,		/* root clock can1. */
	can2,		/* root clock can2. */
	can3,		/* root clock can3. */
	lpuart1,	/* root clock lpuart1. */
	lpuart2,	/* root clock lpuart2. */
	lpuart3,	/* root clock lpuart3. */
	lpuart4,	/* root clock lpuart4. */
	lpuart5,	/* root clock lpuart5. */
	lpuart6,	/* root clock lpuart6. */
	lpuart7,	/* root clock lpuart7. */
	lpuart8,	/* root clock lpuart8. */
	lpuart9,	/* root clock lpuart9. */
	lpuart10,	/* root clock lpuart10. */
	lpuart11,	/* root clock lpuart11. */
	lpuart12,	/* root clock lpuart12. */
	lpi2c1,		/* root clock lpi2c1. */
	lpi2c2,		/* root clock lpi2c2. */
	lpi2c3,		/* root clock lpi2c3. */
	lpi2c4,		/* root clock lpi2c4. */
	lpi2c5,		/* root clock lpi2c5. */
	lpi2c6,		/* root clock lpi2c6. */
	lpspi1,		/* root clock lpspi1. */
	lpspi2,		/* root clock lpspi2. */
	lpspi3,		/* root clock lpspi3. */
	lpspi4,		/* root clock lpspi4. */
	lpspi5,		/* root clock lpspi5. */
	lpspi6,		/* root clock lpspi6. */
	emv1,		/* root clock emv1. */
	emv2,		/* root clock emv2. */
	enet1,		/* root clock enet1. */
	enet2,		/* root clock enet2. */
	enet_qos,	/* root clock enet qos. */
	enet_25m,	/* root clock enet 25m. */
	enet_timer1,	/* root clock enet timer1. */
	enet_timer2,	/* root clock enet timer2. */
	enet_timer3,	/* root clock enet timer3. */
	usdhc1,		/* root clock usdhc1. */
	usdhc2,		/* root clock usdhc2. */
	asrc,		/* root clock asrc. */
	mqs,		/* root clock mqs. */
	mic,		/* root clock mic. */
	spdif,		/* root clock spdif */
	sai1,		/* root clock sai1. */
	sai2,		/* root clock sai2. */
	sai3,		/* root clock sai3. */
	sai4,		/* root clock sai4. */
	gc355,		/* root clock gc355. */
	lcdif,		/* root clock lcdif. */
	lcdifv2,	/* root clock lcdifv2. */
	mipi_ref,	/* root clock mipi ref. */
	mipi_esc,	/* root clock mipi esc. */
	csi2,		/* root clock csi2. */
	csi2_esc,	/* root clock csi2 esc. */
	csi2_ui,	/* root clock csi2 ui. */
	csi,		/* root clock csi. */
	cko1,		/* root clock cko1. */
	cko2,		/* root clock cko2. */
	end,		/* root clock end. */
};

static const char * const root_clocks[79][8] = {
	{CLOCK_MUX_DEFAULT, "pll_arm", "pll1_sys", "pll3_sys", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_sys", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_sys", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_sys", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll1_div5", "pll2_sys", "pll2_pfd1", "pll3_pfd0"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_pfd1", "pll2_sys"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd0", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll2_sys", "pll3_div2", "pll1_div5", "pll2_pfd0"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_sys", "pll1_div5", "pll_audio", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll_audio", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll_audio", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll3_pfd2", "pll3_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd0", "pll2_sys", "pll2_pfd2", "pll3_sys"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd0", "pll2_sys", "pll2_pfd2", "pll3_sys"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll2_pfd3", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll3_pfd2", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll3_pfd2", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_div2", "pll1_div5", "pll2_sys", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1"},
	{CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1"},
	{CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1"},
	{CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1"},
	{CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1"},
	{CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1"},
	{CLOCK_MUX_DEFAULT, "pll1_div2", "pll_audio", "pll1_div5", "pll2_pfd1"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll2_pfd0", "pll1_div5", "pll_arm"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll2_pfd0", "pll1_div5", "pll_arm"},
	{CLOCK_MUX_DEFAULT, "pll1_div5", "pll3_div2", "pll_audio", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll1_div5", "pll3_div2", "pll_audio", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll_audio", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll_audio", "pll3_sys", "pll3_pfd2", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll_audio", "pll3_pfd2", "pll1_div5", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll_audio", "pll3_pfd2", "pll1_div5", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll_audio", "pll3_pfd2", "pll1_div5", "pll2_pfd3"},
	{CLOCK_MUX_DEFAULT, "pll3_pfd3", "pll3_sys", "pll_audio", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd1", "pll3_sys", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd2", "pll3_pfd0", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd2", "pll3_pfd0", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd0", "pll3_pfd0", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_sys", "pll2_pfd0", "pll3_pfd0", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll2_pfd0", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll2_pfd0", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll2_pfd0", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll3_sys", "pll3_pfd1", "pll_video"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd2", "pll2_sys", "pll3_pfd1", "pll1_div5"},
	{CLOCK_MUX_DEFAULT, "pll2_pfd3", "rcosc48M", "pll3_pfd1", "pll_audio"}
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

static struct clk_hw **hws;
static struct clk_hw_onecell_data *clk_hw_data;

static int imxrt1170_clocks_probe(struct platform_device *pdev)
{
	void __iomem *ccm_base;
	void __iomem *pll_base;
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

	hws[IMXRT1170_CLK_PLL_ARM_PRE] = imx_clk_hw_pllv3(IMX_PLLV3_SYSV2,
		"pll_arm_pre", "osc", pll_base + 0x200, 0xff);
	imx_clk_hw_pll3_powerbit(hws[IMXRT1170_CLK_PLL_ARM_PRE], 13);
	hws[IMXRT1170_CLK_PLL_ARM_BYPASS] = imx_clk_hw_mux("pll_arm_bypass",
		pll_base + 0x200, 17, 1, pll_arm_mux, 2);
	hws[IMXRT1170_CLK_PLL_ARM_DIV] = clk_hw_register_divider_table(NULL, "pll_arm_div",
		"pll_arm_bypass", CLK_SET_RATE_PARENT | CLK_SET_RATE_GATE,
		pll_base + 0x200, 15, 2, 0, post_div_table, &imx_ccm_lock);
	hws[IMXRT1170_CLK_PLL_ARM] = imx_clk_hw_gate("pll_arm", "pll_arm_div", pll_base + 0x200, 14);

	hws[IMXRT1170_CLK_PLL3_PRE] = imx_clk_hw_pllv3(IMX_PLLV3_GENERICV2,
		"pll3_pre", "osc", pll_base + 0x210, 0x1);
	imx_clk_hw_pll3_powerbit(hws[IMXRT1170_CLK_PLL3_PRE], 21);
	hws[IMXRT1170_CLK_PLL3_BYPASS] = imx_clk_hw_mux("pll3_bypass",
		pll_base + 0x210, 16, 1, pll3_mux, 2);
	hws[IMXRT1170_CLK_PLL3] = imx_clk_hw_gate("pll3_sys", "pll3_bypass", pll_base + 0x210, 13);

	hws[IMXRT1170_CLK_PLL2_PRE] = imx_clk_hw_pllv3(IMX_PLLV3_GENERICV2,
		"pll2_pre", "osc", pll_base + 0x240, 0x1);
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

	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_M7, m7);
	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_M4, m4);
	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_BUS, bus);
	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_BUS_LPSR, bus_lpsr);
	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_SEMC, semc);
	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_CSSYS, cssys);
	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_CSTRACE, cstrace);
	DEF_CLOCK(0, IMXRT1170_CLK_M4_SYSTICK, m4_systick);
	DEF_CLOCK(0, IMXRT1170_CLK_M7_SYSTICK, m7_systick);
	DEF_CLOCK(0, IMXRT1170_CLK_ADC1, adc1);
	DEF_CLOCK(0, IMXRT1170_CLK_ADC2, adc2);
	DEF_CLOCK(0, IMXRT1170_CLK_ACMP, acmp);
	DEF_CLOCK(0, IMXRT1170_CLK_FLEXIO1, flexio1);
	DEF_CLOCK(0, IMXRT1170_CLK_FLEXIO2, flexio2);
	DEF_CLOCK(CLK_IS_CRITICAL, IMXRT1170_CLK_GPT1, gpt1);
	DEF_CLOCK(0, IMXRT1170_CLK_GPT2, gpt2);
	DEF_CLOCK(0, IMXRT1170_CLK_GPT3, gpt3);
	DEF_CLOCK(0, IMXRT1170_CLK_GPT4, gpt4);
	DEF_CLOCK(0, IMXRT1170_CLK_GPT5, gpt5);
	DEF_CLOCK(0, IMXRT1170_CLK_GPT6, gpt6);
	DEF_CLOCK(0, IMXRT1170_CLK_FLEXSPI1, flexspi1);
	DEF_CLOCK(0, IMXRT1170_CLK_FLEXSPI2, flexspi2);
	DEF_CLOCK(0, IMXRT1170_CLK_CAN1, can1);
	DEF_CLOCK(0, IMXRT1170_CLK_CAN2, can2);
	DEF_CLOCK(0, IMXRT1170_CLK_CAN3, can3);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART1, lpuart1);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART2, lpuart2);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART3, lpuart3);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART4, lpuart4);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART5, lpuart5);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART6, lpuart6);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART7, lpuart7);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART8, lpuart8);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART9, lpuart9);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART10, lpuart10);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART11, lpuart11);
	DEF_CLOCK(0, IMXRT1170_CLK_LPUART12, lpuart12);
	DEF_CLOCK(0, IMXRT1170_CLK_LPI2C1, lpi2c1);
	DEF_CLOCK(0, IMXRT1170_CLK_LPI2C2, lpi2c2);
	DEF_CLOCK(0, IMXRT1170_CLK_LPI2C3, lpi2c3);
	DEF_CLOCK(0, IMXRT1170_CLK_LPI2C4, lpi2c4);
	DEF_CLOCK(0, IMXRT1170_CLK_LPI2C5, lpi2c5);
	DEF_CLOCK(0, IMXRT1170_CLK_LPI2C6, lpi2c6);
	DEF_CLOCK(0, IMXRT1170_CLK_LPSPI1, lpspi1);
	DEF_CLOCK(0, IMXRT1170_CLK_LPSPI2, lpspi2);
	DEF_CLOCK(0, IMXRT1170_CLK_LPSPI3, lpspi3);
	DEF_CLOCK(0, IMXRT1170_CLK_LPSPI4, lpspi4);
	DEF_CLOCK(0, IMXRT1170_CLK_LPSPI5, lpspi5);
	DEF_CLOCK(0, IMXRT1170_CLK_LPSPI6, lpspi6);
	DEF_CLOCK(0, IMXRT1170_CLK_EMV1, emv1);
	DEF_CLOCK(0, IMXRT1170_CLK_EMV2, emv2);
	DEF_CLOCK(0, IMXRT1170_CLK_ENET1, enet1);
	DEF_CLOCK(0, IMXRT1170_CLK_ENET2, enet2);
	DEF_CLOCK(0, IMXRT1170_CLK_ENET_QOS, enet_qos);
	DEF_CLOCK(0, IMXRT1170_CLK_ENET_25M, enet_25m);
	DEF_CLOCK(0, IMXRT1170_CLK_ENET_TIMER1, enet_timer1);
	DEF_CLOCK(0, IMXRT1170_CLK_ENET_TIMER2, enet_timer2);
	DEF_CLOCK(0, IMXRT1170_CLK_ENET_TIMER3, enet_timer3);
	DEF_CLOCK(0, IMXRT1170_CLK_USDHC1, usdhc1);
	DEF_CLOCK(0, IMXRT1170_CLK_USDHC2, usdhc2);
	DEF_CLOCK(0, IMXRT1170_CLK_ASRC, asrc);
	DEF_CLOCK(0, IMXRT1170_CLK_MQS, mqs);
	DEF_CLOCK(0, IMXRT1170_CLK_MIC, mic);
	DEF_CLOCK(0, IMXRT1170_CLK_SPDIF, spdif);
	DEF_CLOCK(0, IMXRT1170_CLK_SAI1, sai1);
	DEF_CLOCK(0, IMXRT1170_CLK_SAI2, sai2);
	DEF_CLOCK(0, IMXRT1170_CLK_SAI3, sai3);
	DEF_CLOCK(0, IMXRT1170_CLK_SAI4, sai4);
	DEF_CLOCK(0, IMXRT1170_CLK_GC355, gc355);
	DEF_CLOCK(0, IMXRT1170_CLK_LCDIF, lcdif);
	DEF_CLOCK(0, IMXRT1170_CLK_LCDIFV2, lcdifv2);
	DEF_CLOCK(0, IMXRT1170_CLK_MIPI_REF, mipi_ref);
	DEF_CLOCK(0, IMXRT1170_CLK_MIPI_ESC, mipi_esc);
	DEF_CLOCK(0, IMXRT1170_CLK_CSI2, csi2);
	DEF_CLOCK(0, IMXRT1170_CLK_CSI2_ESC, csi2_esc);
	DEF_CLOCK(0, IMXRT1170_CLK_CSI2_UI, csi2_ui);
	DEF_CLOCK(0, IMXRT1170_CLK_CSI, csi);
	DEF_CLOCK(0, IMXRT1170_CLK_CKO1, cko1);
	DEF_CLOCK(0, IMXRT1170_CLK_CKO2, cko2);

	hws[IMXRT1170_CLK_USB] = imx_clk_hw_gate("usb", "bus", ccm_base + LPCG_GATE(115), 0);

	clk_set_rate(hws[IMXRT1170_CLK_PLL_ARM]->clk, 90000000);

	imx_check_clk_hws(hws, IMXRT1170_CLK_END);

	ret = of_clk_add_hw_provider(np, of_clk_hw_onecell_get, clk_hw_data);
	if (ret < 0) {
		dev_err(dev, "Failed to register clks for i.MXRT1170.\n");
		imx_unregister_hw_clocks(hws, IMXRT1170_CLK_END);
	}
	return ret;

	return 0;
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
