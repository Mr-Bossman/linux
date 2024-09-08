// SPDX-License-Identifier: GPL-2.0
/*
 * Simple External Interrupt Controller driver
 *
 * Copyright (C)
 *
 * Author: Jesse Taube <Mr.Bossman075@gmail.com>
 */

#define pr_fmt(fmt) "irq-simple: " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/reset.h>
#include <linux/spinlock.h>

#define MAX_IRQ_NUM	512

#define CSR_MEINEXT 0xbe4
#define CSR_MEIEA 0xbe0

struct simple_irq_chip {
	void __iomem		*base;
	struct irq_domain	*domain;
	raw_spinlock_t		lock;
};

static void simple_intc_unmask(struct irq_data *d)
{
	struct simple_irq_chip *irqc = irq_data_get_irq_chip_data(d);
	uint32_t index = d->hwirq / 16;
	uint32_t mask = BIT(d->hwirq % 16);

	raw_spin_lock(&irqc->lock);
	csr_set(CSR_MEIEA, index | (mask << 16));
	raw_spin_unlock(&irqc->lock);
}

static void simple_intc_mask(struct irq_data *d)
{
	struct simple_irq_chip *irqc = irq_data_get_irq_chip_data(d);
	uint32_t index = d->hwirq / 16;
	uint32_t mask = BIT(d->hwirq % 16);

	raw_spin_lock(&irqc->lock);
	csr_clear(CSR_MEIEA, index | (mask << 16));
	raw_spin_unlock(&irqc->lock);
}

static struct irq_chip intc_dev = {
	.name		= "Simple INTC",
	.irq_unmask	= simple_intc_unmask,
	.irq_mask	= simple_intc_mask,
};

static int simple_intc_map(struct irq_domain *d, unsigned int irq,
			     irq_hw_number_t hwirq)
{
	irq_domain_set_info(d, irq, hwirq, &intc_dev, d->host_data,
			    handle_level_irq, NULL, NULL);

	return 0;
}

static const struct irq_domain_ops simple_intc_domain_ops = {
	.xlate	= irq_domain_xlate_onecell,
	.map	= simple_intc_map,
};

static void simple_intc_irq_handler(struct irq_desc *desc)
{
	struct simple_irq_chip *irqc = irq_data_get_irq_handler_data(&desc->irq_data);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	int hwirq;

	chained_irq_enter(chip, desc);
	hwirq = (csr_read_set(CSR_MEINEXT, 0x1) >> 2) & 0x1ff;

	generic_handle_domain_irq(irqc->domain, hwirq);

	chained_irq_exit(chip, desc);
}

static int __init simple_intc_init(struct device_node *intc,
				     struct device_node *parent)
{
	struct simple_irq_chip *irqc;
	int parent_irq;
	int ret;

	irqc = kzalloc(sizeof(*irqc), GFP_KERNEL);
	if (!irqc)
		return -ENOMEM;

	raw_spin_lock_init(&irqc->lock);

	irqc->domain = irq_domain_add_linear(intc, MAX_IRQ_NUM,
					     &simple_intc_domain_ops, irqc);
	if (!irqc->domain) {
		pr_err("Unable to create IRQ domain\n");
		ret = -EINVAL;
		goto err_free;
	}

	parent_irq = of_irq_get(intc, 0);
	if (parent_irq < 0) {
		pr_err("Failed to get main IRQ: %d\n", parent_irq);
		ret = parent_irq;
		goto err_remove_domain;
	}

	irq_set_chained_handler_and_data(parent_irq, simple_intc_irq_handler, irqc);

	pr_info("Interrupt controller register, nr_irqs %d\n", MAX_IRQ_NUM);

	return 0;

err_remove_domain:
	irq_domain_remove(irqc->domain);
err_free:
	kfree(irqc);
	return ret;
}

IRQCHIP_PLATFORM_DRIVER_BEGIN(simple_intc)
IRQCHIP_MATCH("generic,simple-intc", simple_intc_init)
IRQCHIP_PLATFORM_DRIVER_END(simple_intc)

MODULE_DESCRIPTION("Simple External Interrupt Controller driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jesse Taube <Mr.Bossman075@gmail.com>");
