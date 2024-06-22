// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023-2024 SpacemiT (Hangzhou) Technology Co. Ltd
 * Copyright (c) 2024 Yixun Lan <dlan@gentoo.org>
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/irqdomain.h>

/* register offset */
#define GPLR		0x00
#define GPDR		0x0c
#define GPSR		0x18
#define GPCR		0x24
#define GRER		0x30
#define GFER		0x3c
#define GEDR		0x48
#define GSDR		0x54
#define GCDR		0x60
#define GSRER		0x6c
#define GCRER		0x78
#define GSFER		0x84
#define GCFER		0x90
#define GAPMASK		0x9c
#define GCPMASK		0xa8

#define K1_BANK_GPIO_NUMBER	(32)
#define BANK_GPIO_MASK		(K1_BANK_GPIO_NUMBER - 1)

#define spacemit_gpio_to_bank_idx(gpio)		((gpio) / K1_BANK_GPIO_NUMBER)
#define spacemit_gpio_to_bank_offset(gpio)	((gpio) & BANK_GPIO_MASK)
#define spacemit_bank_to_gpio(idx, offset)	\
	(((idx) * K1_BANK_GPIO_NUMBER) | ((offset) & BANK_GPIO_MASK))

static u32 k1_gpio_bank_offset[] = { 0x0, 0x4, 0x8, 0x100 };

struct spacemit_gpio_bank {
	void __iomem			*reg_bank;
	u32				irq_mask;
	u32				irq_rising_edge;
	u32				irq_falling_edge;
};

struct spacemit_gpio_chip {
	struct gpio_chip		chip;
	struct irq_domain		*domain;
	struct spacemit_gpio_bank	*banks;
	void __iomem			*reg_base;
	unsigned int			ngpio;
	unsigned int			nbank;
	int				irq;
};

static inline void spacemit_clear_edge_detection(struct spacemit_gpio_bank *bank,
						 u32 bit)
{
	writel(bit, bank->reg_bank + GCRER);
	writel(bit, bank->reg_bank + GCFER);
}

static inline void spacemit_set_edge_detection(struct spacemit_gpio_bank *bank,
					       u32 bit)
{
	writel(bit & bank->irq_rising_edge, bank->reg_bank + GSRER);
	writel(bit & bank->irq_falling_edge, bank->reg_bank + GSFER);
}

static void spacemit_reset_edge_detection(struct spacemit_gpio_chip *schip)
{
	struct spacemit_gpio_bank *bank;
	unsigned int i;

	for (i = 0; i < schip->nbank; i++) {
		bank = &schip->banks[i];

		writel(0xffffffff, bank->reg_bank + GCFER);
		writel(0xffffffff, bank->reg_bank + GCRER);
		writel(0xffffffff, bank->reg_bank + GAPMASK);
	}
}

static int spacemit_gpio_to_irq(struct gpio_chip *chip, unsigned int offset)
{
	struct spacemit_gpio_chip *schip;

	schip =	container_of(chip, struct spacemit_gpio_chip, chip);

	return irq_create_mapping(schip->domain, offset);
}

static int spacemit_gpio_direction_input(struct gpio_chip *chip,
					 unsigned int offset)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	u32 bit;

	schip =	container_of(chip, struct spacemit_gpio_chip, chip);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(offset)];
	bit = BIT(spacemit_gpio_to_bank_offset(offset));

	writel(bit, bank->reg_bank + GCDR);

	return 0;
}

static int spacemit_gpio_direction_output(struct gpio_chip *chip,
					  unsigned int offset, int value)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	u32 bit;

	schip =	container_of(chip, struct spacemit_gpio_chip, chip);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(offset)];
	bit = BIT(spacemit_gpio_to_bank_offset(offset));

	writel(bit, bank->reg_bank + (value ? GPSR : GPCR));
	writel(bit, bank->reg_bank + GSDR);

	return 0;
}

static int spacemit_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	u32 bit, gplr;

	schip = container_of(chip, struct spacemit_gpio_chip, chip);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(offset)];

	bit = BIT(spacemit_gpio_to_bank_offset(offset));
	gplr = readl(bank->reg_bank + GPLR);

	return !!(gplr & bit);
}

static void spacemit_gpio_set(struct gpio_chip *chip,
			      unsigned int offset, int value)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	u32 bit, gpdr;

	schip = container_of(chip, struct spacemit_gpio_chip, chip);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(offset)];

	bit = BIT(spacemit_gpio_to_bank_offset(offset));
	gpdr = readl(bank->reg_bank + GPDR);

	/* Is it configured as output? */
	if (gpdr & bit)
		writel(bit, bank->reg_bank + (value ? GPSR : GPCR));
}

#ifdef CONFIG_OF_GPIO
static int spacemit_gpio_of_xlate(struct gpio_chip *chip,
				  const struct of_phandle_args *gpiospec,
				  u32 *flags)
{
	struct spacemit_gpio_chip *schip =
			container_of(chip, struct spacemit_gpio_chip, chip);

	/* GPIO index start from 0. */
	if (gpiospec->args[0] >= schip->ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0];
}
#endif

static int spacemit_gpio_irq_type(struct irq_data *d, unsigned int type)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	int gpio = irqd_to_hwirq(d);
	u32 bit;

	schip = irq_data_get_irq_chip_data(d);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(gpio)];

	bit = BIT(spacemit_gpio_to_bank_offset(gpio));

	if (type & IRQ_TYPE_EDGE_RISING) {
		bank->irq_rising_edge |= bit;
		writel(bit, bank->reg_bank + GSRER);
	} else {
		bank->irq_rising_edge &= ~bit;
		writel(bit, bank->reg_bank + GCRER);
	}

	if (type & IRQ_TYPE_EDGE_FALLING) {
		bank->irq_falling_edge |= bit;
		writel(bit, bank->reg_bank + GSFER);
	} else {
		bank->irq_falling_edge &= ~bit;
		writel(bit, bank->reg_bank + GCFER);
	}

	return 0;
}

static irqreturn_t spacemit_gpio_irq_handler(int irq, void *data)
{
	struct spacemit_gpio_chip *schip = data;
	struct spacemit_gpio_bank *bank;
	unsigned int irqs_handled = 0;
	unsigned long pending = 0;
	int i, n;
	u32 gedr, girq;

	for (i = 0; i < schip->nbank; i++) {
		bank = &schip->banks[i];

		gedr = readl(bank->reg_bank + GEDR);
		if (!gedr)
			continue;

		writel(gedr, bank->reg_bank + GEDR);
		gedr = gedr & bank->irq_mask;

		if (!gedr)
			continue;

		pending = gedr;
		for_each_set_bit(n, &pending, BITS_PER_LONG) {
			girq = irq_find_mapping(schip->domain,
						spacemit_bank_to_gpio(i, n));
			generic_handle_irq(girq);
		}

		irqs_handled++;
	}

	return irqs_handled ? IRQ_HANDLED : IRQ_NONE;
}

static void spacemit_ack_muxed_gpio(struct irq_data *d)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	int gpio = irqd_to_hwirq(d);
	u32 bit;

	schip = irq_data_get_irq_chip_data(d);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(gpio)];

	bit = BIT(spacemit_gpio_to_bank_offset(gpio));

	writel(bit, bank->reg_bank + GEDR);
}

static void spacemit_mask_muxed_gpio(struct irq_data *d)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	int gpio = irqd_to_hwirq(d);
	u32 bit;

	schip = irq_data_get_irq_chip_data(d);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(gpio)];

	bit = BIT(spacemit_gpio_to_bank_offset(gpio));

	bank->irq_mask &= ~bit;

	spacemit_clear_edge_detection(bank, bit);
}

static void spacemit_unmask_muxed_gpio(struct irq_data *d)
{
	struct spacemit_gpio_chip *schip;
	struct spacemit_gpio_bank *bank;
	int gpio = irqd_to_hwirq(d);
	u32 bit;

	schip = irq_data_get_irq_chip_data(d);
	bank = &schip->banks[spacemit_gpio_to_bank_idx(gpio)];

	bit = BIT(spacemit_gpio_to_bank_offset(gpio));

	bank->irq_mask |= bit;

	spacemit_set_edge_detection(bank, bit);
}

static struct irq_chip spacemit_muxed_gpio_chip = {
	.name		= "k1-gpio-irqchip",
	.irq_ack	= spacemit_ack_muxed_gpio,
	.irq_mask	= spacemit_mask_muxed_gpio,
	.irq_unmask	= spacemit_unmask_muxed_gpio,
	.irq_set_type	= spacemit_gpio_irq_type,
	.flags		= IRQCHIP_SKIP_SET_WAKE,
	GPIOCHIP_IRQ_RESOURCE_HELPERS,
};

static int spacemit_irq_domain_map(struct irq_domain *d, unsigned int irq,
				   irq_hw_number_t hw)
{
	irq_set_chip_and_handler(irq, &spacemit_muxed_gpio_chip,
				 handle_edge_irq);
	irq_set_chip_data(irq, d->host_data);

	return 0;
}

static const struct irq_domain_ops spacemit_gpio_irq_domain_ops = {
	.map	= spacemit_irq_domain_map,
	.xlate	= irq_domain_xlate_twocell,
};

static int spacemit_gpio_probe_dt(struct platform_device *pdev,
				  struct spacemit_gpio_chip *schip)
{
	void __iomem *reg;
	int i, nbank;

	nbank = ARRAY_SIZE(k1_gpio_bank_offset);

	schip->banks = devm_kzalloc(&pdev->dev,
				    sizeof(*schip->banks) * nbank,
				    GFP_KERNEL);
	if (!schip->banks)
		return -ENOMEM;

	for (i = 0; i < nbank; i++) {
		reg = schip->reg_base + k1_gpio_bank_offset[i];
		schip->banks[i].reg_bank = reg;
	}

	schip->nbank = nbank;
	schip->ngpio = nbank * K1_BANK_GPIO_NUMBER;

	return 0;
}

static void spacemit_gpio_remove_irq_domain(void *data)
{
	struct irq_domain *domain = data;

	irq_domain_remove(domain);
}

static int spacemit_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np;
	struct spacemit_gpio_chip *schip;
	struct irq_domain *domain;
	struct resource *res;
	void __iomem *base;
	int ret;

	np = pdev->dev.of_node;
	if (!np)
		return -EINVAL;

	schip = devm_kzalloc(dev, sizeof(*schip), GFP_KERNEL);
	if (!schip)
		return -ENOMEM;

	schip->irq = platform_get_irq(pdev, 0);
	if (schip->irq < 0)
		return schip->irq;

	base = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	schip->reg_base = base;

	ret = spacemit_gpio_probe_dt(pdev, schip);
	if (ret)
		return dev_err_probe(dev, ret, "Fail to initialize gpio unit\n");

	domain = irq_domain_add_linear(np, schip->ngpio,
				       &spacemit_gpio_irq_domain_ops,
				       schip);
	if (domain == NULL)
		return -EINVAL;

	ret = devm_add_action_or_reset(dev, spacemit_gpio_remove_irq_domain,
				       domain);
	if (ret)
		return ret;

	schip->domain			= domain;
	schip->chip.label		= "k1-gpio";
	schip->chip.parent		= dev;
	schip->chip.request		= gpiochip_generic_request;
	schip->chip.free		= gpiochip_generic_free;
	schip->chip.direction_input	= spacemit_gpio_direction_input;
	schip->chip.direction_output	= spacemit_gpio_direction_output;
	schip->chip.get			= spacemit_gpio_get;
	schip->chip.set			= spacemit_gpio_set;
	schip->chip.to_irq		= spacemit_gpio_to_irq;
#ifdef CONFIG_OF_GPIO
	schip->chip.of_xlate		= spacemit_gpio_of_xlate;
	schip->chip.of_gpio_n_cells	= 2;
#endif
	schip->chip.ngpio		= schip->ngpio;
	schip->chip.base		= -1;

	ret = devm_request_threaded_irq(dev, schip->irq,
					spacemit_gpio_irq_handler,
					NULL, IRQF_ONESHOT,
					schip->chip.label, schip);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to request high IRQ\n");

	ret = devm_gpiochip_add_data(dev, &schip->chip, schip);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to add gpiochip\n");

	spacemit_reset_edge_detection(schip);

	return 0;
}

static const struct of_device_id spacemit_gpio_dt_ids[] = {
	{ .compatible = "spacemit,k1-gpio" },
	{ /* sentinel */ },
};

static struct platform_driver spacemit_gpio_driver = {
	.probe		= spacemit_gpio_probe,
	.driver		= {
		.name	= "k1-gpio",
		.of_match_table = spacemit_gpio_dt_ids,
	},
};
module_platform_driver(spacemit_gpio_driver);

MODULE_DESCRIPTION("GPIO driver for SpacemiT K1 SoC");
MODULE_LICENSE("GPL");
