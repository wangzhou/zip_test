#!/bin/bash
#
# This script is the autotest script for HiSilicon Hip08/Hip09 QM/ZIP modules.
#
# How to use this script:
#  -h show help descriptions
#  -v show all test cases and their numbers
#  -o <case_num> do specific test case
#  Note: by default do all test cases
#
# Test cases are planned as below:
#  - modules insmod/rmmod
#  - module parameter
#  - crypto driver basic functions
#  - uacce driver basic functions
#  - VF related tests
#  - RAS related tests
#  - DFX related tests
#  - Reset related tests
#
# Author: Zhou Wang <wangzhou1@hisilicon.com>

# global varials
test_case_array=(0)

# test cases start
test_case_example()
{
	echo "This is an example for test case!"
}

insmod_and_rmmod_qm()
{
	insmod qm.ko &> /dev/null
	if [ $? -ne 0 ]; then
		echo "fail to insmod qm.ko"
		exit 1
	fi
	rmmod qm.ko &> /dev/null
	if [ $? -ne 0 ]; then
		echo "fail to rmmod qm.ko"
		exit 1
	fi
}

insmod_and_rmmod_qm_multi_times()
{
	local i

	for i in `seq 10000`
	do
		insmod_and_rmmod_qm
	done
}

insmod_and_rmmod_zip()
{
	insmod qm.ko &> /dev/null
	insmod hisi_zip.ko &> /dev/null
	if [ $? -ne 0 ]; then
		echo "fail to insmod hisi_zip.ko"
		rmmod qm.ko
		exit 1
	fi
	rmmod hisi_zip.ko &> /dev/null
	if [ $? -ne 0 ]; then
		echo "fail to rmmod hisi_zip.ko"
		rmmod qm.ko
		exit 1
	fi
	rmmod qm.ko &> /dev/null
}

insmod_and_rmmod_zip_multi_times()
{
	local i

	for i in `seq 10000`
	do
		insmod_and_rmmod_zip
	done
}

set_pf_q_num_as_100()
{
	insmod qm.ko
	insmod hisi_zip.ko pf_q_num=100 &> /dev/null
	# fix me: run crypto_zip_test
}

set_pf_q_num_as_max_value()
{
	# fix me: v1 is 4096
	local max_q_num=1024
	insmod qm.ko
	insmod hisi_zip.ko pf_q_num=${max_q_num}
	# fix me: run crypto_zip_test
}

set_pf_q_num_as_0_err1()
{
	insmod qm.ko &> /dev/null
	insmod hisi_zip.ko pf_q_num=0 &> /dev/null
	if [ $? -eq 0 ]; then
		echo "pf_q_num should not be 0, case fails"
	fi
}

set_pf_q_num_gt_max_err2()
{
	insmod qm.ko &> /dev/null
	insmod hisi_zip.ko pf_q_num=4097 &> /dev/null
	if [ $? -eq 0 ]; then
		echo "pf_q_num should not be 4097, case fails"
	fi
}

crypto_zip_test_all()
{
	insmod qm.ko
	insmod hisi_zip.ko
	insmod crypto_zip_test.ko
	if [ $? -ne 0 ]; then
		echo "crypto_zip_test fails"
		exit 1
	fi
	rmmod crypto_zip_test.ko
	rmmod hisi_zip.ko
	rmmod qm.ko
}

uacce_zip_test_all()
{
	echo "Fix me: please add this test case!"
}

sriov_numvfs_path_host_zip="/sys/devices/pci0000:74/0000:74:00.0/0000:75:00.0/sriov_numvfs"
enable_and_disable_vf()
{
	local i

	insmod qm.ko
	insmod hisi_zip.ko

	for i in `seq 10000`
	do
		echo 3 > $sriov_numvfs_path_host_zip
		echo 0 > $sriov_numvfs_path_host_zip
	done

	rmmod hisi_zip.ko
	rmmod qm.ko
}

enable_and_disable_all_vf()
{
	local i

	insmod qm.ko
	insmod hisi_zip.ko

	for i in `seq 10000`
	do
		echo 63 > $sriov_numvfs_path_host_zip
		echo 0 > $sriov_numvfs_path_host_zip
	done

	rmmod hisi_zip.ko
	rmmod qm.ko
}

enable_and_disable_gt_all_vf_err1()
{
	insmod qm.ko
	insmod hisi_zip.ko

	echo 64 > $sriov_numvfs_path_host_zip

	rmmod hisi_zip.ko
	rmmod qm.ko
}

use_vf_in_host_with_crypto()
{
	insmod qm.ko
	insmod hisi_zip.ko

	echo 3 > $sriov_numvfs_path_host_zip
	# Fix me: how to use vf in host?

	rmmod hisi_zip.ko
	rmmod qm.ko
}

use_vf_in_guest_with_crypto_by_passthrough()
{
	echo "Fix me: please add this test case!"
}

qm_ce_error_test()
{
	echo "Fix me: please add this test case!"
}

qm_nfe_error_test()
{
	echo "Fix me: please add this test case!"
}

qm_misc_error_test()
{
	echo "Fix me: please add this test case!"
}

qm_aeq_error_test()
{
	echo "Fix me: please add this test case!"
}

zip_ce_error_test()
{
	echo "Fix me: please add this test case!"
}

zip_nfe_error_test()
{
	echo "Fix me: please add this test case!"
}

zip_misc_error_test()
{
	echo "Fix me: please add this test case!"
}

qm_dfx_dump_in_pf()
{
	echo "Fix me: please add this test case!"
}

qm_dfx_dump_in_vf()
{
	echo "Fix me: please add this test case!"
}

qm_dfx_current_qm_test()
{
	echo "Fix me: please add this test case!"
}

qm_dfx_current_q_test()
{
	echo "Fix me: please add this test case!"
}

qm_dfx_clean_enable_test()
{
	echo "Fix me: please add this test case!"
}

zip_dfx_dump_test()
{
	echo "Fix me: please add this test case!"
}

zip_dfx_clean_enable_test()
{
	echo "Fix me: please add this test case!"
}

zip_flr_test()
{
	echo "Fix me: please add this test case!"
}

zip_software_reset_test()
{
	echo "Fix me: please add this test case!"
}
# test cases end

test_case_index=0
add_test_case()
{
	test_case_array[${test_case_index}]=$1
	let test_case_index+=1
}

show_help()
{
	echo "This script is the autotest script for HiSilicon"
	echo "Hip08/Hip09 QM/ZIP modules."
	echo
	echo "Options:"
	echo "  -h             show help descriptions"
	echo "  -v             show all test cases and their numbers"
	echo "  -o <case_num>  do specific test case"
	echo
	echo "  Note: by default do all test cases"
	echo
}

show_all_test_cases()
{
	local i=0

	for test_case in ${test_case_array[@]}
	do
		printf "test_case_%.3d: %s\n" ${i} ${test_case}
		let i+=1
	done
}

run_one_test_case()
{
	${test_case_array[$1]}
}

input_parse()
{
	while getopts "ho:v" opt; do
		case $opt in
		h)
			show_help; exit 0 ;;
		v)
			show_all_test_cases; exit 0 ;;
		o)
			run_one_test_case ${OPTARG}; exit 0 ;;
		\?)
			echo "Invalid option: -${OPTARG}" >&2
			exit 1
			;;
		esac
	done
}

run_all_test_case()
{
	for test_case in ${test_case_array[@]}
	do
		${test_case}
	done
}

main()
{
	# Add test cases here firstly
	add_test_case  "test_case_example"
	add_test_case  "insmod_and_rmmod_qm"
	add_test_case  "insmod_and_rmmod_qm_multi_times"
	add_test_case  "insmod_and_rmmod_zip"
	add_test_case  "insmod_and_rmmod_zip_multi_times"
	add_test_case  "set_pf_q_num_as_100"
	add_test_case  "set_pf_q_num_as_max_value"
	add_test_case  "set_pf_q_num_as_0_err1"
	add_test_case  "set_pf_q_num_gt_max_err2"
	add_test_case  "crypto_zip_test_all"
	add_test_case  "uacce_zip_test_all"
	add_test_case  "enable_and_disable_vf"
	add_test_case  "enable_and_disable_all_vf"
	add_test_case  "enable_and_disable_gt_all_vf_err1"
	add_test_case  "use_vf_in_host_with_crypto"
	add_test_case  "use_vf_in_guest_with_crypto_by_passthrough"
	add_test_case  "qm_ce_error_test"
	add_test_case  "qm_nfe_error_test"
	add_test_case  "qm_misc_error_test"
	add_test_case  "qm_aeq_error_test"
	add_test_case  "zip_ce_error_test"
	add_test_case  "zip_nfe_error_test"
	add_test_case  "zip_misc_error_test"
	add_test_case  "qm_dfx_dump_in_pf"
	add_test_case  "qm_dfx_dump_in_vf"
	add_test_case  "qm_dfx_current_qm_test"
	add_test_case  "qm_dfx_current_q_test"
	add_test_case  "qm_dfx_clean_enable_test"
	add_test_case  "zip_dfx_dump_test"
	add_test_case  "zip_dfx_clean_enable_test"
	add_test_case  "zip_flr_test"
	add_test_case  "zip_software_reset_test"

	input_parse "$@"

	run_all_test_case
}
main "$@"
