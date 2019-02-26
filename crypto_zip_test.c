/**
 * Module parameter:
 *
 * - If not parameter, by default running all test cases.
 * - If setting one_test=id, runing this test case.
 * - If setting range_test=id_start-id_end, running test case in the range.
 * - one_test and range_test can not be setted at same time.
 *
 * - Use echo <case_num> > /sys/kernel/debug/hzip_test_num to trigger specific
 *   test case.
 *
 *   fix me: to do this feature
 * - Use echo 1 > /sys/xxx/show_test_case to list all test cases.
 *
 * Add one test case:
 *
 * - Add your test case function, e.g. int test_case_0(void)
 * - Add register function in test_init, e.g.
 *	hisi_zip_crypto_register_test_case(0, test_case_0);
 */
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/smp.h>
#include <linux/crypto.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/zlib.h>
#include "crypto_zip_test.h"

#define COMP_BUF_SIZE			512
#define TEST_CASE_ARRAY_SIZE		512

static int one_test = -1;
module_param(one_test, int, 0644);

static int range_test[2] = {-1, -1};
module_param_array(range_test, int, NULL, 0644);

static struct dentry *hzip_test_num;
static u32 current_test_num = 0;

struct zip_test_case {
	int case_num;
	int (*fun)(int param);
};

static struct zip_test_case hisi_zip_test_cases[TEST_CASE_ARRAY_SIZE];

enum test_option {
	ALL_TEST,
	ONE_TEST,
	RANGE_TEST,
};

struct test_cmd {
	enum test_option option;

	union {
		/* all test has no parameter */

		/* one test: user tells which one to test */
		u32 case_num;

		/* range test: user tells which range to test */
		struct case_range {
			u32 start;
			u32 end;
		} case_range;
	};
};

/* Please add your test case below. Test cases start. */

/**
 * Test case 0 - This is a an example about test case(brief decription).
 *
 * Prepare condition: NA(please tell us prepare condition here).
 * Test goal: Arch(Please tell us what you want to test).
 * Expected result: Print "zip: test case 0 is an example!"
 *                  (Tell us expected result).
 *
 * Return 0 case is successful, return value < 0 case is failed,
 * return value > 0 case specific return value.
 */
static int test_case_0(int param)
{
	pr_info("zip: test case 0 is an example!\n");

	return 0;
}

struct comp_testvec {
	int inlen, outlen;
	char input[COMP_BUF_SIZE];
	char output[COMP_BUF_SIZE];
};

static const struct comp_testvec zlib_comp = {
	.inlen	= 191,
	.outlen	= 129,
	.input	= "This document describes a compression method based on the DEFLATE"
		  "compression algorithm.  This document defines the application of "
		  "the DEFLATE algorithm to the IP Payload Compression Protocol.",
	.output	= "\x78\x5e\x5d\xce\x41\x0a\xc3\x30"
		  "\x0c\x04\xc0\xaf\xec\x0b\xf2\x87"
		  "\xd2\xa6\x50\xe8\xc1\x07\x7f\x40"
		  "\xb1\x95\x5a\x60\x5b\xc6\x56\x0f"
		  "\xfd\x7d\x93\x1e\x42\xe8\x51\xec"
		  "\xee\x20\x9f\x64\x20\x6a\x78\x17"
		  "\xae\x86\xc8\x23\x74\x59\x78\x80"
		  "\x10\xb4\xb4\xce\x63\x88\x56\x14"
		  "\xb6\xa4\x11\x0b\x0d\x8e\xd8\x6e"
		  "\x4b\x8c\xdb\x7c\x7f\x5e\xfc\x7c"
		  "\xae\x51\x7e\x69\x17\x4b\x65\x02"
		  "\xfc\x1f\xbc\x4a\xdd\xd8\x7d\x48"
		  "\xad\x65\x09\x64\x3b\xac\xeb\xd9"
		  "\xc2\x01\xc0\xf4\x17\x3c\x1c\x1c"
		  "\x7d\xb2\x52\xc4\xf5\xf4\x8f\xeb"
		  "\x6a\x1a\x34\x4f\x5f\x2e\x32\x45"
		  "\x4e",
};

/* this is the gzip compression data which includes FLG.FNAME field */
static const struct comp_testvec gzip_comp_with_name = {
	.inlen	= 191,
	.outlen	= 152,
	.input	= "This document describes a compression method based on the DEFLATE"
		  "compression algorithm.  This document defines the application of "
		  "the DEFLATE algorithm to the IP Payload Compression Protocol.",
	.output = "\x1f\x8b\x08\x08\xbf\x5e\x24\x5c\x00\x03\x67\x7a\x69\x70\x5f\x64"
		  "\x61\x74\x61\x00\x5d\x8d\x31\x0e\xc2\x30\x10\x04\x7b\x5e\xb1\x2f"
		  "\xc8\x1f\x10\x04\x09\x89\xc2\x85\x3f\x70\xb1\x2f\xf8\x24\xdb\x67"
		  "\xd9\x47\xc1\xef\x49\x68\x88\x28\x57\xbb\x33\xeb\x93\x0c\x44\x0d"
		  "\xaf\xc2\xd5\x10\x79\x84\x2e\x0b\x0f\x10\x82\x96\xd6\x79\x0c\xd1"
		  "\x8a\xc2\x96\x34\x62\xa1\xc1\x11\x5b\xb6\xc4\xb8\xce\xb7\xc7\xd9"
		  "\xcf\xc7\x19\xe5\xa7\x76\xb1\x54\x26\xc0\xff\x89\x57\xa9\x9b\x76"
		  "\x07\xa9\xb5\x2c\x81\x6c\x27\x74\x3d\xba\x7e\x02\x98\x7e\x8b\xbb"
		  "\x83\xa3\x77\x56\x8a\xb8\x1c\x8e\x5c\x57\xd3\xa0\x79\x3a\x7d\x00"
		  "\x7f\x5c\xed\xd6\xc0\x00\x00\x00",
};

static const struct comp_testvec gzip_comp = {
	.inlen	= 191,
	.outlen	= 142,
	.input	= "This document describes a compression method based on the DEFLATE"
		  "compression algorithm.  This document defines the application of "
		  "the DEFLATE algorithm to the IP Payload Compression Protocol.",
	.output = "\x1f\x8b\x08\x00\xbf\x5e\x24\x5c\x00\x03"
		  "\x5d\x8d\x31\x0e\xc2\x30\x10\x04\x7b\x5e\xb1\x2f"
		  "\xc8\x1f\x10\x04\x09\x89\xc2\x85\x3f\x70\xb1\x2f\xf8\x24\xdb\x67"
		  "\xd9\x47\xc1\xef\x49\x68\x88\x28\x57\xbb\x33\xeb\x93\x0c\x44\x0d"
		  "\xaf\xc2\xd5\x10\x79\x84\x2e\x0b\x0f\x10\x82\x96\xd6\x79\x0c\xd1"
		  "\x8a\xc2\x96\x34\x62\xa1\xc1\x11\x5b\xb6\xc4\xb8\xce\xb7\xc7\xd9"
		  "\xcf\xc7\x19\xe5\xa7\x76\xb1\x54\x26\xc0\xff\x89\x57\xa9\x9b\x76"
		  "\x07\xa9\xb5\x2c\x81\x6c\x27\x74\x3d\xba\x7e\x02\x98\x7e\x8b\xbb"
		  "\x83\xa3\x77\x56\x8a\xb8\x1c\x8e\x5c\x57\xd3\xa0\x79\x3a\x7d\x00"
		  "\x7f\x5c\xed\xd6\xc0\x00\x00\x00",
};

/**
 * Test case 1 - Basic zlib hardware compression and hardware decompression test.
 *
 * Prepare condition: input data is input of zlib_comp above.
 * Test goal: test if hardware zlib compression/decompression is right.
 * Expected result: Print "zip: test case 1 is end and successful"
 */
static int test_case_1(int param)
{
	struct crypto_comp *tfm;
	char *out_buf;
	int out_size = 512;
	char *decomp_out_buf;
	int decomp_out_size = 512;
	const char *alg;
	int ret = 0;

	switch (param) {
	case 0:
		alg = "zlib-deflate";
		break;
	case 1:
		alg = "gzip";
		break;
	default:
		pr_info("zip: unknown algorithm\n");
		return -EINVAL;
	}

	out_buf = kmalloc(out_size, GFP_KERNEL);
	if (!out_buf) {
		pr_info("zip: fail to allocate out buffer\n");
		return -1;
	}

	decomp_out_buf = kmalloc(out_size, GFP_KERNEL);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		ret = -1;
		goto err_free;
	}

	tfm = crypto_alloc_comp(alg, 0, 0);
	if (IS_ERR(tfm)) {
		pr_info("zip: fail to create comp tfm\n");
		ret = PTR_ERR(tfm);
		goto err_free_decomp;
	}

	ret = crypto_comp_compress(tfm, zlib_comp.input, zlib_comp.inlen,
				   out_buf, &out_size);
	if (ret) {
		pr_info("zip: failed to compress, ret = %d\n", ret);
		goto err_free_tfm;
	}

	ret = crypto_comp_decompress(tfm, out_buf, out_size, decomp_out_buf,
				     &decomp_out_size);
	if (ret) {
		pr_info("zip: failed to decompress, ret = %d\n", ret);
		goto err_free_tfm;
	}

	if (memcmp(decomp_out_buf, zlib_comp.input, zlib_comp.inlen)) {
		pr_info("zip: test case 1 fails");
		ret = -1;
		goto err_free_tfm;
	}

err_free_tfm:
	crypto_free_comp(tfm);
err_free_decomp:
	kfree(decomp_out_buf);
err_free:
	kfree(out_buf);

	return ret;
}

/**
 * Test case 2 - Basic zlib hardware compression and software decompression.
 *
 * Prepare condition: input data is input of zlib_comp above.
 *                    software decompression done by kernel deflate APIs.
 * Test goal: test if hardware zlib compression and software decompression is
 *            right.
 * Expected result: Print "zip: test case 2 is end and successful"
 */
static int test_case_2(int param)
{
	struct crypto_comp *tfm;
	const char *alg;
	char *out_buf;
	int out_size = 512;
	char *decomp_out_buf;
	int decomp_out_size = 512;
	struct z_stream_s stream;
	int ret = 0;
	u8 head_size;

	switch (param) {
	case 0:
		alg = "zlib-deflate";
		head_size = 2;
		break;
	case 1:
		alg = "gzip";
		head_size = 10;
		break;
	default:
		pr_info("zip: unknown algorithm\n");
		return -EINVAL;
	}

	out_buf = kmalloc(out_size, GFP_KERNEL);
	if (!out_buf) {
		pr_info("zip: fail to allocate out buffer\n");
		return -1;
	}

	decomp_out_buf = kmalloc(decomp_out_size, GFP_KERNEL);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		ret = -1;
		goto err_free;
	}

	tfm = crypto_alloc_comp(alg, CRYPTO_ALG_TYPE_COMPRESS, 0xf);
	if (IS_ERR(tfm)) {
		pr_info("zip: fail to create comp tfm\n");
		ret = -1;
		goto err_free_decomp;
	}

	ret = crypto_comp_compress(tfm, zlib_comp.input, zlib_comp.inlen,
				   out_buf, &out_size);
	if (ret) {
		pr_info("zip: failed to compress, ret = %d\n", ret);
		goto err_free_tfm;
	}

	/* software decompression starts */
	stream.next_in = out_buf + head_size;
	stream.avail_in = out_size - head_size;
	stream.total_in = 0;
	stream.next_out = decomp_out_buf;
	stream.avail_out = decomp_out_size;
	stream.total_out = 0;
	stream.workspace = kmalloc(zlib_inflate_workspacesize(), GFP_KERNEL);
	if (!stream.workspace) {
		ret = -ENOMEM;
		goto err_free_tfm;
	}

	ret = zlib_inflateInit2(&stream, -15);
	if (ret != Z_OK) {
		pr_info("zip: inflate init fails, ret = %d\n", ret);
		goto err_free_workspace;
	}

	ret = zlib_inflate(&stream, Z_FINISH);
	if (ret != Z_OK && ret != Z_STREAM_END) {
		pr_info("zip: inflate fails, ret = %d\n", ret);
		goto err_end_stream;
	}

	if (memcmp(decomp_out_buf, zlib_comp.input, zlib_comp.inlen)) {
		pr_info("zip: test case 2 fails");
		ret = -1;
		goto err_end_stream;
	}

err_end_stream:
	zlib_deflateEnd(&stream);
err_free_workspace:
	kfree(stream.workspace);
err_free_tfm:
	crypto_free_comp(tfm);
err_free_decomp:
	kfree(decomp_out_buf);
err_free:
	kfree(out_buf);

	return ret;
}

/**
 * Test case 3 - Basic zlib hardware compression and software decompression.
 *               Keep running multiple times(10000 times)
 *
 * Prepare condition: input data is input of zlib_comp above.
 *                    software decompression done by kernel deflate APIs.
 * Test goal: test if hardware zlib compression and software decompression is
 *            right when keeping running.
 * Expected result: Print "zip: test case 3 is end and successful"
 */
static int test_case_3(int param)
{
	struct crypto_comp *tfm;
	char *out_buf;
	int out_size = 512;
	char *decomp_out_buf;
	int decomp_out_size = 512;
	struct z_stream_s stream;
	int ret = 0;
	u32 run_times = 10000;
	int i;

	out_buf = kmalloc(out_size, GFP_KERNEL);
	if (!out_buf) {
		pr_info("zip: fail to allocate out buffer\n");
		return -1;
	}

	decomp_out_buf = kmalloc(out_size, GFP_KERNEL);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		ret = -1;
		goto err_free;
	}

	tfm = crypto_alloc_comp("zlib-deflate", CRYPTO_ALG_TYPE_COMPRESS, 0xf);
	if (IS_ERR(tfm)) {
		pr_info("zip: fail to create comp tfm\n");
		ret = -1;
		goto err_free_decomp;
	}

	for (i = 0; i < run_times; i++) {
		out_size = 512;
		ret = crypto_comp_compress(tfm, zlib_comp.input, zlib_comp.inlen,
					   out_buf, &out_size);
		if (ret) {
			pr_info("zip: failed to compress, ret = %d\n", ret);
			goto err_free_tfm;
		}

		/* software decompression starts */
		stream.next_in = out_buf;
		stream.avail_in = out_size;
		stream.total_in = 0;
		stream.next_out = decomp_out_buf;
		stream.avail_out = decomp_out_size;
		stream.total_out = 0;
		stream.workspace = kmalloc(zlib_inflate_workspacesize(), GFP_KERNEL);
		if (!stream.workspace) {
			ret = -ENOMEM;
			goto err_free_tfm;
		}

		if (Z_OK != zlib_inflateInit(&stream)) {
			pr_info("zip: inflate init fails, ret = %d\n", ret);
			ret = -1;
			goto err_free_workspace;
		}

		ret = zlib_inflate(&stream, Z_FINISH);
		if (ret != Z_OK && ret != Z_STREAM_END) {
			pr_info("zip: inflate fails, ret = %d\n", ret);
			goto err_end_stream;
		}

		zlib_deflateEnd(&stream);
		kfree(stream.workspace);

		if (memcmp(decomp_out_buf, zlib_comp.input, zlib_comp.inlen)) {
			pr_info("zip: test case 3 fails");
			ret = -1;
			goto err_free_tfm;
		}
	}

	goto err_free_tfm;

err_end_stream:
	zlib_deflateEnd(&stream);
err_free_workspace:
	kfree(stream.workspace);
err_free_tfm:
	crypto_free_comp(tfm);
err_free_decomp:
	kfree(decomp_out_buf);
err_free:
	kfree(out_buf);

	return ret;
}

/**
 * Test case 4 - Basic zlib software compression and hardware decompression.
 *
 * Prepare condition: input data is output of zlib_comp above.
 * Test goal: test if hardware zlib hardware decompression is right.
 * Expected result: Print "zip: test case 4 is end and successful"
 */
static int test_case_4(int param)
{
	struct comp_testvec *test_vec;
	struct crypto_comp *tfm;
	const char *alg;
	char *decomp_out_buf;
	int decomp_out_size = 512;
	int ret = 0;

	switch (param) {
	case 0:
		alg = "zlib-deflate";
		test_vec = &zlib_comp;
		break;
	case 1:
		alg = "gzip";
		test_vec = &gzip_comp;
		break;
	case 2:
		alg = "gzip";
		test_vec = &gzip_comp_with_name;
		break;
	default:
		pr_info("zip: unknown algorithm\n");
		return -EINVAL;
	}

	decomp_out_buf = kmalloc(decomp_out_size, GFP_KERNEL);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		return -1;
	}

	tfm = crypto_alloc_comp(alg, CRYPTO_ALG_TYPE_COMPRESS, 0xf);
	if (IS_ERR(tfm)) {
		pr_info("zip: fail to create comp tfm\n");
		ret = -1;
		goto err_free_decomp;
	}

	ret = crypto_comp_decompress(tfm, test_vec->output, test_vec->outlen,
				     decomp_out_buf, &decomp_out_size);
	if (ret) {
		pr_info("zip: failed to compress, ret = %d\n", ret);
		goto err_free_tfm;
	}

	if (memcmp(decomp_out_buf, test_vec->input, test_vec->inlen)) {
		pr_info("zip: test case 4 fails");
		ret = -1;
		goto err_free_tfm;
	}

err_free_tfm:
	crypto_free_comp(tfm);
err_free_decomp:
	kfree(decomp_out_buf);

	return ret;
}

/**
 * Test case 5 - Zlib tfm allocate/free multiple times, = pf_q_num / 2.
 *
 * Prepare condition: NA.
 * Test goal: test if QM allocate qp and free qp are OK.
 * Expected result: Print "zip: test case 5 is end and successful"
 */
static int test_case_5(int param)
{
#define DEFAULT_PF_Q_NUM		64
#define DEFAULT_TFM			(DEFAULT_PF_Q_NUM / 2)
	struct crypto_comp *tfm;
	struct crypto_comp *tfm_array[DEFAULT_PF_Q_NUM];
	int ret = 0, i, j;

	/* case_5_1 */
	for (i = 0; i < 10000; i++) {
		tfm = crypto_alloc_comp("zlib-deflate",
					CRYPTO_ALG_TYPE_COMPRESS, 0xf);
		if (IS_ERR(tfm)) {
			pr_info("zip: fail to create comp tfm: %u-tfm\n", i);
			return -1;
		}

		crypto_free_comp(tfm);
	}

	/* case_5_2 */
	for (i = 0; i < DEFAULT_TFM; i++) {
		tfm_array[i] = crypto_alloc_comp("zlib-deflate",
					CRYPTO_ALG_TYPE_COMPRESS, 0xf);
		if (IS_ERR(tfm_array[i])) {
			pr_info("zip: fail to create comp tfm: %u-tfm\n", i);
			for (j = 0; j < i; j++)
				crypto_free_comp(tfm_array[j]);

			return -2;
		}
	}

	for (i = 0; i < DEFAULT_TFM; i++) {
		crypto_free_comp(tfm_array[i]);
	}

	return ret;
}

/**
 * Test case 6 - Zlib tfm allocate/free multiple times, > pf_q_num / 2.
 *
 * Prepare condition: NA.
 * Test goal: if having no resource for tfm, should return error.
 * Expected result: Print "zip: test case 6 is end and successful"
 */
static int test_case_6(int param)
{
#define DEFAULT_PF_Q_NUM		64
#define DEFAULT_TFM			(DEFAULT_PF_Q_NUM / 2)
	struct crypto_comp *tfm_array[DEFAULT_TFM + 1];
	int ret = 0, i, j;

	for (i = 0; i < DEFAULT_TFM + 1; i++) {
		tfm_array[i] = crypto_alloc_comp("zlib-deflate",
						 CRYPTO_ALG_TYPE_COMPRESS, 0xf);
		if (IS_ERR(tfm_array[i])) {
			pr_info("zip: fail to create comp tfm: %u-tfm\n", i);
			for (j = 0; j < i; j++)
				crypto_free_comp(tfm_array[j]);

			if (i != DEFAULT_TFM)
				return -1;
		}
	}

	return ret;
}

/* this is the thread function for case 7 */
static int allocate_tfm_c7(void *date)
{
	struct crypto_comp *tfm;
	int i;

	for (i = 0; i < 10000; i++) {
		tfm = crypto_alloc_comp("zlib-deflate",
					CRYPTO_ALG_TYPE_COMPRESS, 0xf);
		if (IS_ERR(tfm)) {
			pr_info("zip: fail to create comp tfm: %u-tfm\n", i);
			do_exit(-1);
		}

		crypto_free_comp(tfm);
	}

	do_exit(0);
}

/**
 * Test case 7 - Zlib tfm allocate/free multiple times, multiple threads
 *               request tfm at same time.
 * Prepare condition: NA.
 * Test goal: request tfms at same time should ok.
 * Expected result: Print "zip: test case 7 is end and successful"
 */
static int test_case_7(int param)
{
#define DEFAULT_THREAD_NUM		5
	struct task_struct *thread_array[DEFAULT_THREAD_NUM];
	int ret = 0, t;

	for (t = 0; t < DEFAULT_THREAD_NUM; t++) {
		thread_array[t] =
		kthread_create_on_node(allocate_tfm_c7, NULL,
				       cpu_to_node(smp_processor_id()),
				       "zip_tc7_t%u", t);
		if (IS_ERR(thread_array[t])) {
			pr_info("zip: fail to create kthread %d\n", t);
			ret = -EPERM;
			goto err_return;
		}

		wake_up_process(thread_array[t]);
	}

err_return:
	return ret;
}

/* this is the thread function for case 8 */
static int do_compress_c8(void *date)
{
	struct crypto_comp *tfm;
	char *decomp_out_buf;
	char self_name[TASK_COMM_LEN];
	int decomp_out_size = 512;
	int ret = 0, i;

	get_task_comm(self_name, current);
	self_name[TASK_COMM_LEN - 1] = 0;

	pr_info("%s is running on cpu-%d\n", self_name, smp_processor_id());

	decomp_out_buf = kmalloc(decomp_out_size, GFP_KERNEL);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		return -1;
	}

	tfm = crypto_alloc_comp("zlib-deflate", CRYPTO_ALG_TYPE_COMPRESS, 0xf);
	if (IS_ERR(tfm)) {
		pr_info("zip: fail to create comp tfm\n");
		ret = -1;
		goto err_free_decomp;
	}

	for (i = 0; i < 1000; i++) {
		decomp_out_size = 512;
		memset(decomp_out_buf, 0, decomp_out_size);
		ret = crypto_comp_decompress(tfm, zlib_comp.output,
					     zlib_comp.outlen, decomp_out_buf,
					     &decomp_out_size);
		if (ret) {
			pr_info("zip: failed to compress, ret = %d\n", ret);
			goto err_free_tfm;
		}

		if (memcmp(decomp_out_buf, zlib_comp.input, zlib_comp.inlen)) {
			pr_info("zip: memcmp failed!");
			ret = -1;
			goto err_free_tfm;
		}
	}

err_free_tfm:
	crypto_free_comp(tfm);
err_free_decomp:
	kfree(decomp_out_buf);

	return ret;
}

/**
 * Test case 8 - Zlib hardware decompression, multiple threads do at same time.
 * Prepare condition: NA.
 * Test goal: zlib hardware decompression at same time should ok.
 * Expected result: Print "zip: test case 8 is end and successful"
 */
static int test_case_8(int param)
{
#define DEFAULT_THREAD_NUM		32
	struct task_struct *thread_array[DEFAULT_THREAD_NUM];
	int ret = 0, t;

	for (t = 0; t < DEFAULT_THREAD_NUM; t++) {
		pr_info("thread-%d is on numa node %d\n", t, cpu_to_node(smp_processor_id()));
		thread_array[t] =
		kthread_create_on_node(do_compress_c8, NULL,
				       cpu_to_node(smp_processor_id()),
				       "zip_tc8_t%u", t);
		if (IS_ERR(thread_array[t])) {
			pr_info("zip: fail to create kthread %d\n", t);
			ret = -EPERM;
			goto err_return;
		}

		wake_up_process(thread_array[t]);
	}

err_return:
	return ret;
}

/**
 * Test case 9 - Basic gzip hardware compression and hardware decompression test.
 *
 * Note: mostly same with case 1, but algorithm is gzip.
 */
static int test_case_9(int param)
{
	/* 0: zlib, 1: gzip */
	return test_case_1(1);
}

/**
 * Test case 10 - Basic gzip hardware compression and software decompression test.
 *
 * Note: mostly same with case 2, but algorithm is gzip.
 */
static int test_case_10(int param)
{
	/* 0: zlib, 1: gzip */
	return test_case_2(1);
}

/**
 * Test case 11 - Basic gzip software compression and hardware decompression.
 *
 * Note: mostly same with case 4, but algorithm is gzip.
 *
 * Fix me: currently this case pass, however, we fixed gzip head and context.
 *         Driver should consider the size of gzip head according to the flag
 *         field in gzip head.
 */
static int test_case_11(int param)
{
	/* 0: zlib, 1: gzip */
	return test_case_4(1);
}

/**
 * Test case 12 - Basic gzip software compression and hardware decompression.
 *
 * Note: mostly same with case 4, but algorithm is gzip.
 *
 * gzip compressed head with name field.
 */
static int test_case_12(int param)
{
	/* 0: zlib, 1: gzip, 2: gzip with name field */
	return test_case_4(2);
}

/**
 * Test case 13 - Zlib tfm allocate multiple times, > pf_q_num / 2.
 *		  And run task on a queue which number > 64.
 *
 * Prepare condition: insmod hisi_zip.ko pf_q_num=<num>, here num should > 74.
 * Test goal: IT reported queue which number > 64 can not run, let's test it.
 * Expected result: Print "zip: test case 13 is end and successful"
 */
static int test_case_13(int param)
{
#define DEFAULT_PF_Q_NUM		64
#define TFM_NUM				((DEFAULT_PF_Q_NUM / 2) + 5)
	struct crypto_comp *tfm_array[TFM_NUM];
	char *decomp_out_buf;
	int decomp_out_size = 512;
	int ret = 0, i, j;

	for (i = 0; i < TFM_NUM; i++) {
//	for (i = 0; i < 32; i++) {
		tfm_array[i] = crypto_alloc_comp("zlib-deflate",
						 CRYPTO_ALG_TYPE_COMPRESS, 0xf);
		if (IS_ERR(tfm_array[i])) {
			pr_info("zip: fail to create comp tfm: %u-tfm\n", i);
			ret = -1;
			goto err_free_tfm;
		}
	}

	decomp_out_buf = kmalloc(decomp_out_size, GFP_KERNEL);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		ret = -ENOMEM;
		goto err_free_tfm;
	}

	ret = crypto_comp_decompress(tfm_array[33], zlib_comp.output,
				     zlib_comp.outlen, decomp_out_buf,
				     &decomp_out_size);
	if (ret) {
		pr_info("zip: failed to compress, ret = %d\n", ret);
		goto err_free_decomp;
	}

	if (memcmp(decomp_out_buf, zlib_comp.input, zlib_comp.inlen)) {
		ret = -2;
		goto err_free_decomp;
	}

err_free_decomp:
	kfree(decomp_out_buf);
err_free_tfm:
	for (j = 0; j < i; j++)
		crypto_free_comp(tfm_array[j]);

	return ret;
}

/* use "char data_1k[SZ_1K]" 1K data to create big data, e.g. 2M, 4M... */
static void create_data(char *buf, u32 size)
{
	u32 times = size / SZ_1K;
	u32 i, p = 0;

	for (i = 0; i < times; i++) {
		memcpy(buf + p, data_1k, SZ_1K);
		p += SZ_1K;
	}
}

/**
 * Test case 14 - Zlib hardware compression and hardware compression.
 *                Input data is 4M.
 *
 * Prepare condition: NA
 * Test goal: Let's make input data big.
 * Expected result: Print "zip: test case 14 is end and successful"
 */
static int test_case_14(int param)
{
	struct crypto_comp *tfm;
	char *in_buf;
	int in_size;
	char *out_buf;
	int out_size;
	char *decomp_out_buf;
	int decomp_out_size;
	int data_size;
	const char *alg;
	int ret = 0;

	switch (param) {
	case 0:
		alg = "zlib-deflate";
		in_size = out_size = decomp_out_size = data_size = SZ_4M;
		break;
	case 1:
		alg = "gzip";
		in_size = out_size = decomp_out_size = data_size = SZ_4M;
		break;
	case 2:
		alg = "zlib-deflate";
		in_size = out_size = decomp_out_size = data_size = SZ_2M;
		break;
	case 3:
		alg = "gzip";
		in_size = out_size = decomp_out_size = data_size = SZ_2M;
		break;
	case 4:
		alg = "zlib-deflate";
		in_size = out_size = decomp_out_size = data_size = SZ_1M;
		break;
	case 5:
		alg = "gzip";
		in_size = out_size = decomp_out_size = data_size = SZ_1M;
		break;
	default:
		pr_info("zip: unknown algorithm\n");
		return -EINVAL;
	}

	in_buf = vmalloc(in_size);
	if (!in_buf) {
		pr_info("zip: fail to allocate in buffer\n");
		return -1;
	}

	create_data(in_buf, data_size);

	out_buf = vmalloc(out_size);
	if (!out_buf) {
		pr_info("zip: fail to allocate out buffer\n");
		ret = -1;
		goto err_free_in;
	}

	decomp_out_buf = vmalloc(decomp_out_size);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		ret = -1;
		goto err_free;
	}

	tfm = crypto_alloc_comp(alg, CRYPTO_ALG_TYPE_COMPRESS, 0xf);
	if (IS_ERR(tfm)) {
		pr_info("zip: fail to create comp tfm\n");
		ret = -1;
		goto err_free_decomp;
	}

	ret = crypto_comp_compress(tfm, in_buf, in_size, out_buf, &out_size);
	pr_info("zip: compress out size %d, ret = %d\n", out_size, ret);
	if (ret) {
		pr_info("zip: failed to compress, ret = %d\n", ret);
		goto err_free_tfm;
	}

	ret = crypto_comp_decompress(tfm, out_buf, out_size, decomp_out_buf,
				     &decomp_out_size);
	pr_info("zip: decompress out size %d, ret = %d\n", decomp_out_size, ret);
	if (ret) {
		pr_info("zip: failed to decompress, ret = %d\n", ret);
		goto err_free_tfm;
	}

	if (memcmp(decomp_out_buf, in_buf, in_size)) {
		pr_info("zip: data comparing fails");
		ret = -1;
		goto err_free_tfm;
	}

err_free_tfm:
	crypto_free_comp(tfm);
err_free_decomp:
	vfree(decomp_out_buf);
err_free:
	vfree(out_buf);
err_free_in:
	vfree(in_buf);

	return ret;
}

/**
 * Test case 15 - Gzip hardware compression and hardware compression.
 *                Input data is 4M.
 *
 * Prepare condition: NA
 * Test goal: Let's make input data big.
 * Expected result: Print "zip: test case 15 is end and successful"
 */
static int test_case_15(int param)
{
	/* 0: zlib, 1: gzip */
	return test_case_14(1);
}

static int test_case_16(int param)
{
	/* zlib 2M */
	return test_case_14(2);
}

static int test_case_17(int param)
{
	/* gzip 2M */
	return test_case_14(3);
}

static int test_case_18(int param)
{
	/* zlib 1M */
	return test_case_14(4);
}

static int test_case_19(int param)
{
	/* gzip 1M */
	return test_case_14(5);
}

/**
 * Test case 20 - Zlib tfm allocate multiple times, ES 4096, CS 1024
 *
 * Prepare condition: insmod hisi_zip.ko pf_q_num=<num>, here num should be all
 * queue.
 * Test goal: Test guy reported can not run all queue together, so let's see.
 * Expected result: Print "zip: test case 20 is end and successful"
 *
 * Note: this is same with case 13, but let's use all queue in this case.
 */
static int test_case_20(int param)
{
#define DEFAULT_PF_Q_NUM		512
#define TFM_NUM				(DEFAULT_PF_Q_NUM / 2)
	struct crypto_comp **tfm_array;
	char *decomp_out_buf;
	int decomp_out_size = 512;
	int ret = 0, i, j;

	tfm_array = kmalloc(sizeof(struct crypto_comp *) * TFM_NUM, GFP_KERNEL);
	if (!tfm_array) {
		pr_info("zip: fail to allocate tfm_array\n");
		return -ENOMEM;
	}
	
	for (i = 0; i < TFM_NUM; i++) {
		pr_info("allocate %d comp tfm\n", i);
		*(tfm_array + i) = crypto_alloc_comp("zlib-deflate",
						CRYPTO_ALG_TYPE_COMPRESS, 0xf);
		if (IS_ERR(*(tfm_array + i))) {
			pr_info("zip: fail to create comp tfm: %u-tfm\n", i);
			ret = -1;
			goto err_free_tfm;
		}
	}

	decomp_out_buf = kmalloc(decomp_out_size, GFP_KERNEL);
	if (!decomp_out_buf) {
		pr_info("zip: fail to allocate decomp out buffer\n");
		ret = -ENOMEM;
		goto err_free_tfm;
	}

	ret = crypto_comp_decompress(tfm_array[33], zlib_comp.output,
				     zlib_comp.outlen, decomp_out_buf,
				     &decomp_out_size);
	if (ret) {
		pr_info("zip: failed to compress, ret = %d\n", ret);
		goto err_free_decomp;
	}

	if (memcmp(decomp_out_buf, zlib_comp.input, zlib_comp.inlen)) {
		ret = -2;
		goto err_free_decomp;
	}

err_free_decomp:
	kfree(decomp_out_buf);
err_free_tfm:
	for (j = 0; j < i; j++)
		crypto_free_comp(tfm_array[j]);
	kfree(tfm_array);

	return ret;
}

/*
 * to do: test cases will be added:
 *
 * Test case 5 - Basic zlib hardware decompression, keeping running 10000 times.
 * Test case 6 - Basic gzip hardware compression, software decompression.
 * Test case 7 - Basic gzip hardware decompression.
 * Test case 8 - Zlib hardware compression, input date 4M.
 * Test case 9 - Zlib hardware compression, input date > 4M.
 * Test case 10 - Zlib hardware compression, input date 0.
 * Test case 11 - Zlib hardware compression, input date is random.
 * Test case 12, 13, 14, 15 - as above, but alg is gzip.
 * Test case 16 - Zlib hardware compression, multiple tfms work at same time.
 * Test case 17 - Zlib hardware compression, multiple tfms = pf_q_num / 2.
 * Test case 18 - Zlib hardware compression, multiple tfms > pf_q_num / 2.
 * Test case 19 - Zlib tfm allocate/free multiple times.
 * Test case 20 - Zlib tfm allocate/free multiple times, = pf_q_num / 2.
 * Test case 21 - Zlib tfm allocate/free multiple times, > pf_q_num / 2.
 * Test case 30 - Zlib tfm allocate/free multiple times, multiple threads request at same time.
 * Test case 31 - Zlib hardware compression performance.
 * Test case 32 - Zlib hardware compression performance, delay.
 * Test case 32 - Zlib hardware compression performance, throughput.
 * Test case 32 - Zlib hardware compression performance, multiple threads.
 *
 */

/* Test cases end. */

static void hisi_zip_crypto_unregister_test_case(int n)
{
	hisi_zip_test_cases[n].fun = NULL;
	hisi_zip_test_cases[n].case_num = -1;
}

static int hisi_zip_crypto_register_test_case(int n, int (*fn)(int))
{
	if (hisi_zip_test_cases[n].fun) {
		pr_info("zip: fail to register test case\n");
		return -1;
	}

	hisi_zip_test_cases[n].fun = fn;
	hisi_zip_test_cases[n].case_num = n;

	return 0;
}

static int parse_module_param(struct test_cmd *cmd)
{
	bool one_test_flag = 0;
	bool range_test_flag = 0;

	if (one_test >= 0)
		one_test_flag = 1;

	if (range_test[0] >= 0 && range_test[1] >= 0)
		range_test_flag = 1;

	if (one_test_flag && range_test_flag) {
		one_test_flag = 0;
		range_test_flag = 0;
		pr_info("zip: do not set one_test and range_test together\n");
		return	-EINVAL;
	}

	if (one_test_flag) {
		cmd->option = ONE_TEST;
		cmd->case_num = one_test;
	} else if (range_test_flag) {
		if (range_test[1] < range_test[0]) {
			pr_info("zip: range end should >= start\n");
			return	-EINVAL;
		}
		cmd->option = RANGE_TEST;
		cmd->case_range.start = range_test[0];
		cmd->case_range.end   = range_test[1];
	} else {
		cmd->option = ALL_TEST;
	}

	return 0;
}

static int hisi_zip_run_test_case(struct zip_test_case *test_case)
{
	int ret;

	pr_info("zip: test case %d is starting\n", test_case->case_num);

	ret = test_case->fun(0);

	if (ret < 0)
		pr_info("zip: test case %d is failed, ret=%d\n", test_case->case_num, ret);
	else
		pr_info("zip: test case %d is successful\n", test_case->case_num);

	return ret;
}

static int hisi_zip_crypto_test_all(void)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(hisi_zip_test_cases); i++) {
		if (hisi_zip_test_cases[i].fun) {
			ret = hisi_zip_run_test_case(hisi_zip_test_cases + i);
			/* when testing all, one fails, whole test returns */
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int hisi_zip_crypto_test_one(struct test_cmd cmd)
{
	int i = cmd.case_num;

	if (!hisi_zip_test_cases[i].fun) {
		pr_info("zip: test case %d does not exist in test one\n", i);
		return -1;
	}

	return hisi_zip_run_test_case(hisi_zip_test_cases + i);
}

static int hisi_zip_crypto_test_range(struct test_cmd cmd)
{
	int start = cmd.case_range.start;
	int end = cmd.case_range.end;
	int ret, i;

	for (i = start; i < end + 1; i++) {
		if (hisi_zip_test_cases[i].fun) {
			ret = hisi_zip_run_test_case(hisi_zip_test_cases + i);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static void hisi_zip_crypto_test_error(int ret)
{
	/* fix me: print more error info */
	pr_info("zip: case fails: %d!\n", ret);
}

static void hisi_zip_crypto_test_main(struct test_cmd cmd)
{
	enum test_option option = cmd.option;
	int ret;

	switch (option) {
	case ALL_TEST:
		ret = hisi_zip_crypto_test_all();
		if (ret < 0)
			goto test_fail;
		break;
	case ONE_TEST:
		ret = hisi_zip_crypto_test_one(cmd);
		if (ret < 0)
			goto test_fail;
		break;
	case RANGE_TEST:
		ret = hisi_zip_crypto_test_range(cmd);
		if (ret < 0)
			goto test_fail;
		break;
	default:
		pr_info("zip: wrong test option\n");
		return;
	}

	pr_info("zip: test success\n");

	return;

test_fail:
	hisi_zip_crypto_test_error(ret);
}

static ssize_t test_num_read(struct file *filp, char __user *buf,
			     size_t count, loff_t *pos)
{
	char tbuf[20];
	int ret;

	ret = sprintf(tbuf, "%u\n", current_test_num);
	return simple_read_from_buffer(buf, count, pos, tbuf, ret);
}

/* do not consider lock currently */
static ssize_t test_num_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *pos)
{
	int len;
	unsigned long val;
	char tbuf[20];

	len = simple_write_to_buffer(tbuf, 20 - 1, pos, buf, count);
	if (len < 0)
		return len;

	tbuf[len] = '\0';
	if (kstrtoul(tbuf, 0, &val))
		return -EFAULT;

	current_test_num = val;

	hisi_zip_run_test_case(hisi_zip_test_cases + current_test_num);

	return count;
}

static const struct file_operations test_num_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = test_num_read,
	.write = test_num_write,
};

static int hisi_zip_test_control_register(void)
{
	struct dentry *tmp;

	tmp = debugfs_create_file("hzip_test_num", 0666, NULL, NULL,
				  &test_num_fops);
	if (!tmp)
		return -ENOENT;

	hzip_test_num = tmp;

	return 0;
}

static void hisi_zip_test_control_unregister(void)
{
	debugfs_remove_recursive(hzip_test_num);
}

static int __init test_init(void)
{
	struct test_cmd cmd;
	int ret;

	pr_info("zip: module init\n");

	ret = parse_module_param(&cmd);
	if (ret) {
		pr_info("zip: module parameter wrong\n");
		return ret;
	}

	/* add register function here one by one */
	hisi_zip_crypto_register_test_case(0, test_case_0);
	hisi_zip_crypto_register_test_case(1, test_case_1);
	hisi_zip_crypto_register_test_case(2, test_case_2);
	hisi_zip_crypto_register_test_case(3, test_case_3);
	hisi_zip_crypto_register_test_case(4, test_case_4);
	hisi_zip_crypto_register_test_case(5, test_case_5);
	hisi_zip_crypto_register_test_case(6, test_case_6);
	hisi_zip_crypto_register_test_case(7, test_case_7);
	hisi_zip_crypto_register_test_case(8, test_case_8);
	hisi_zip_crypto_register_test_case(9, test_case_9);
	hisi_zip_crypto_register_test_case(10, test_case_10);
	hisi_zip_crypto_register_test_case(11, test_case_11);
	hisi_zip_crypto_register_test_case(12, test_case_12);
	hisi_zip_crypto_register_test_case(13, test_case_13);
	hisi_zip_crypto_register_test_case(14, test_case_14);
	hisi_zip_crypto_register_test_case(15, test_case_15);
	hisi_zip_crypto_register_test_case(16, test_case_16);
	hisi_zip_crypto_register_test_case(17, test_case_17);
	hisi_zip_crypto_register_test_case(18, test_case_18);
	hisi_zip_crypto_register_test_case(19, test_case_19);
	hisi_zip_crypto_register_test_case(20, test_case_20);

	hisi_zip_crypto_test_main(cmd);

	/* export a file hzip_test_num to user space by debugfs */
	hisi_zip_test_control_register();

	return 0;
}

static void __exit test_exit(void)
{
	pr_info("zip: module exit\n");

	hisi_zip_test_control_unregister();
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wagnzhou89@126.com");
