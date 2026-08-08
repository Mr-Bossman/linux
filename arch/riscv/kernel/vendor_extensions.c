// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2024 Rivos, Inc
 */

#include <asm/vendorid_list.h>
#include <asm/vendor_extensions.h>
#include <asm/vendor_extensions/andes.h>
#include <asm/vendor_extensions/mips.h>
#include <asm/vendor_extensions/sifive.h>
#include <asm/vendor_extensions/thead.h>

#include <linux/array_size.h>
#include <linux/types.h>

struct riscv_isa_vendor_ext_data_list *riscv_isa_vendor_ext_list[] = {
#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_ANDES
	&riscv_isa_vendor_ext_list_andes,
#endif
#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_MIPS
	&riscv_isa_vendor_ext_list_mips,
#endif
#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_SIFIVE
	&riscv_isa_vendor_ext_list_sifive,
#endif
#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_THEAD
	&riscv_isa_vendor_ext_list_thead,
#endif
};

const size_t riscv_isa_vendor_ext_list_size = ARRAY_SIZE(riscv_isa_vendor_ext_list);

/**
 * __riscv_isa_vendor_extension_available() - Check whether given vendor
 * extension is available or not.
 *
 * @cpu: check if extension is available on this cpu
 * @vendor: vendor that the extension is a member of
 * @bit: bit position of the desired extension
 * Return: true or false
 *
 * NOTE: When cpu is -1, will check if extension is available on all cpus
 */
bool __riscv_isa_vendor_extension_available(int cpu, unsigned long vendor, unsigned int bit)
{
	struct riscv_isavendorinfo *bmap;
	struct riscv_isavendorinfo *cpu_bmap;

	switch (vendor) {
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_ANDES
	case ANDES_VENDOR_ID:
		bmap = &riscv_isa_vendor_ext_list_andes.all_harts_isa_bitmap;
		cpu_bmap = riscv_isa_vendor_ext_list_andes.per_hart_isa_bitmap;
		break;
	#endif
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_MIPS
	case MIPS_VENDOR_ID:
		bmap = &riscv_isa_vendor_ext_list_mips.all_harts_isa_bitmap;
		cpu_bmap = riscv_isa_vendor_ext_list_mips.per_hart_isa_bitmap;
		break;
	#endif
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_SIFIVE
	case SIFIVE_VENDOR_ID:
		bmap = &riscv_isa_vendor_ext_list_sifive.all_harts_isa_bitmap;
		cpu_bmap = riscv_isa_vendor_ext_list_sifive.per_hart_isa_bitmap;
		break;
	#endif
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_THEAD
	case THEAD_VENDOR_ID:
		bmap = &riscv_isa_vendor_ext_list_thead.all_harts_isa_bitmap;
		cpu_bmap = riscv_isa_vendor_ext_list_thead.per_hart_isa_bitmap;
		break;
	#endif
	default:
		return false;
	}

	if (cpu != -1)
		bmap = &cpu_bmap[cpu];

	if (bit >= RISCV_ISA_VENDOR_EXT_MAX)
		return false;

	return test_bit(bit, bmap->isa);
}
EXPORT_SYMBOL_GPL(__riscv_isa_vendor_extension_available);


static const riscv_isa_vendor_ext_data_list* riscv_get_vendor_alt_early_boot(unsigned long vendor)
{
	switch (vendor) {
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_ANDES
	case ANDES_VENDOR_ID:
		return &riscv_alt_early_boot_andes;
	#endif
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_MIPS
	case MIPS_VENDOR_ID:
		return &riscv_alt_early_boot_mips;
	#endif
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_SIFIVE
	case SIFIVE_VENDOR_ID:
		return &riscv_alt_early_boot_sifive;
	#endif
	#ifdef CONFIG_RISCV_ISA_VENDOR_EXT_THEAD
	case THEAD_VENDOR_ID:
		return &riscv_alt_early_boot_thead;
	#endif
	default:
		return NULL;
	}
}

bool riscv_has_vendor_alt_early_boot(unsigned long vendor, unsigned int ext_id)
{
	struct riscv_isa_vendor_ext_data_list *ext_list;

	if (!IS_ENABLED(CONFIG_RISCV_ISA_VENDOR_EXT))
		return false;

	ext_list = riscv_get_vendor_alt_early_boot(vendor);
	if (!ext_list)
		return false;

	for (int j = 0; j < ext_list->ext_data_count; j++) {
		 if (ext_list->ext_data[j].id == ext_id)
			 return true;
	}

	return false;
}
