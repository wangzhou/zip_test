// SPDX-License-Identifier: GPL-2.0+
#include <asm/sysreg.h>
#include <linux/cpu.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/pci.h>
#include "../zip.h"

/*
 * ==== Usage ====
 * mount -t debugfs none /sys/kernel/debug
 * cd sys/kernel/debug/error_inject_0000:75:00.0
 * cat available_error_type -> List supported zip hw errors with error id
 * echo <error id> > error_type -> inject selected hw error
 */

/* Definitions */
/* lspci -kn - xx:00.0 Class 1200: 19e5:a250 hisi_zip */
#define HISI_ZIP_VENDOR_ID			0x19e5
#define HISI_ZIP_PCI_DEVICE_ID			0xa250

#define ZIP_CORE_INT_MASK_REG_OFFSET		0x3010A4
#define ZIP_CORE_INT_SET_REG_OFFSET		0x3010A8
#define QM_PF_ABNORMAL_INT_MASK_REG_OFFSET	0x100004
#define QM_PF_ABNORMAL_INT_SET_REG_OFFSET	0x10000C

#define HISI_ZIP_PF_NUM				2

static struct dentry *error_inject_root[HISI_ZIP_PF_NUM];

struct hisi_zip_hw_error {
	u32 val;
	u32 reg_ecc_einj_en_val;
	const char *type;
	u64 reg_offset;
};

static struct hisi_zip_hw_error zip_errors[] = {
	{
		.val = 0x1,
		.reg_ecc_einj_en_val = BIT(0),
		.type = "zip_1bit_ecc_err\n",
	},
	{
		.val = 0x2,
		.reg_ecc_einj_en_val = BIT(1),
		.type = "zip_2bit_ecc_err\n",
	},
	{
		.val = 0x3,
		.reg_ecc_einj_en_val = BIT(2),
		.type = "zip_axi_rresp_err\n",
	},
	{
		.val = 0x4,
		.reg_ecc_einj_en_val = BIT(3),
		.type = "zip_axi_bresp_err\n",
	},
	{
		.val = 0x5,
		.reg_ecc_einj_en_val = BIT(4),
		.type = "zip_src_addr_parse_int\n",
	},
	{
		.val = 0x6,
		.reg_ecc_einj_en_val = BIT(5),
		.type = "zip_dst_addr_parse_int\n",
	},
	{
		.val = 0x7,
		.reg_ecc_einj_en_val = BIT(6),
		.type = "zip_pre_in_addr_int\n",
	},
	{
		.val = 0x8,
		.reg_ecc_einj_en_val = BIT(7),
		.type = "zip_pre_in_data_int\n",
	},
	{
		.val = 0x9,
		.reg_ecc_einj_en_val = BIT(8),
		.type = "zip_com_inf_int\n",
	},
	{
		.val = 0xA,
		.reg_ecc_einj_en_val = BIT(9),
		.type = "zip_enc_inf_int\n",
	},
	{
		.val = 0xB,
		.reg_ecc_einj_en_val = BIT(10),
		.type = "zip_pre_out_int\n",
	},
	{
		.val = 0xC,
		.reg_ecc_einj_en_val = BIT(0),
		.type = "qm_axi_rresp_err\n",
	},
	{
		.val = 0xD,
		.reg_ecc_einj_en_val = BIT(1),
		.type = "qm_axi_bresp_err\n",
	},
	{
		.val = 0xE,
		.reg_ecc_einj_en_val = BIT(2),
		.type = "qm_2bit_ecc_err\n",
	},
	{
		.val = 0xF,
		.reg_ecc_einj_en_val = BIT(3),
		.type = "qm_1bit_ecc_err\n",
	},
	{
		.val = 0x10,
		.reg_ecc_einj_en_val = BIT(4),
		.type = "qm_acc_get_task_timeout_rint\n",
	},
	{
		.val = 0x11,
		.reg_ecc_einj_en_val = BIT(5),
		.type = "qm_acc_do_task_timeout_rint\n",
	},
	{
		.val = 0x12,
		.reg_ecc_einj_en_val = BIT(6),
		.type = "qm_acc_wb_not_rdy_timeout_rint\n",
	},
	{
		.val = 0x13,
		.reg_ecc_einj_en_val = BIT(7),
		.type = "qm_sq_cq_vf_invalid_rint\n",
	},
	{
		.val = 0x14,
		.reg_ecc_einj_en_val = BIT(8),
		.type = "qm_cq_vf_invalid_rint\n",
	},
	{
		.val = 0x15,
		.reg_ecc_einj_en_val = BIT(9),
		.type = "qm_sq_vf_invalid_rint\n",
	},
	{
		.val = 0x16,
		.reg_ecc_einj_en_val = BIT(10),
		.type = "qm_db_timeout_rint\n",
	},
	{
		.val = 0x17,
		.reg_ecc_einj_en_val = BIT(11),
		.type = "qm_of_fifo_of_rint\n",
	},
	{
		.val = 0x18,
		.reg_ecc_einj_en_val = BIT(12),
		.type = "qm_new_db_invalid_rint\n",
	},
};

static int zip_available_error_type_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(zip_errors); i++)
		seq_printf(m, "0x%x\t%s",
			   zip_errors[i].val,
			   zip_errors[i].type);
		return 0;
}

static int zip_available_error_type_open(struct inode *inode, struct file *file)
{
	return single_open(file, zip_available_error_type_show, NULL);
}

static const struct file_operations zip_avl_err_type_fops = {
	.open           = zip_available_error_type_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static int zip_error_type_set(void *data, u64 val)
{
	struct hisi_zip *hisi_zip = data;
	u64 reg_val;

	if (!hisi_zip) {
		pr_err("%s: invalid hisi_zip", __func__);
		return -EINVAL;
	}

	if (val < 0x1 || val > ARRAY_SIZE(zip_errors)) {
		pr_err("%s: invalid error type set", __func__);
		return -EINVAL;
	}

	pr_info("injecting %s", zip_errors[val - 1].type);

	reg_val = zip_errors[val - 1].reg_ecc_einj_en_val;
	if (val <= 0xB) {
		writel(0x00, hisi_zip->qm.io_base +
		       ZIP_CORE_INT_MASK_REG_OFFSET);
		writel(reg_val, hisi_zip->qm.io_base +
		       ZIP_CORE_INT_SET_REG_OFFSET);
	} else if (val <= 0x18) {
		writel(0x00, hisi_zip->qm.io_base +
		       QM_PF_ABNORMAL_INT_MASK_REG_OFFSET);
		writel(reg_val, hisi_zip->qm.io_base +
		       QM_PF_ABNORMAL_INT_SET_REG_OFFSET);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(zip_error_type_fops, NULL,
			zip_error_type_set, "%llu\n");

static int __init init_hzip_einj(void)
{
	struct hisi_zip *hisi_zip = NULL;
	struct pci_dev *pdev = NULL;
	struct dentry *fentry, *dir;
	char dir_name[40];
	int i, j;

	for (i = 0; (pdev = pci_get_device(HISI_ZIP_VENDOR_ID,
		     HISI_ZIP_PCI_DEVICE_ID, pdev)); i++) {
		dev_info(&pdev->dev, "pci dev id = 0x%x\n", pdev->device);
		hisi_zip = pci_get_drvdata(pdev);
		if (!hisi_zip)
			pr_err("%s ZIP HW device not found\n", __func__);

		snprintf(dir_name, sizeof(dir_name), "error_inject_%s",
			 dev_name(&pdev->dev));
		dir = debugfs_create_dir(dir_name, NULL);
		if (!dir)
			pr_err("%s: debugfs_create_dir failed\n", __func__);
		error_inject_root[i] = dir;

		fentry = debugfs_create_file("available_error_type", 0400,
					     dir, hisi_zip,
					     &zip_avl_err_type_fops);
		if (!fentry)
			goto cleanup;

		fentry = debugfs_create_file("error_type", 0600,
					     dir, hisi_zip,
					     &zip_error_type_fops);
		if (!fentry)
			goto cleanup;
	}

	return 0;

cleanup:
	pr_err("%s failed\n", __func__);
	for (j = 0; j <= i; j++)
		debugfs_remove_recursive(error_inject_root[j]);
	return -ENOMEM;
}

static void __exit exit_hzip_einj(void)
{
	int i;

	for (i = 0; i < HISI_ZIP_PF_NUM; i++) {
		debugfs_remove_recursive(error_inject_root[i]);
		error_inject_root[i] = NULL;
	}
}

module_init(init_hzip_einj);
module_exit(exit_hzip_einj);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shiju Jose <shiju.jose@huawei.com>");
MODULE_AUTHOR("Zhou Wang <wangzhou1@hisilicon.com>");
MODULE_DESCRIPTION("HISI HIP08 ZIP error injection driver for testing");
