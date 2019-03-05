obj-m := crypto_zip_test.o
#obj-m += zip_test_einj.o
clean:
	rm crypto_zip_test.ko crypto_zip_test.o crypto_zip_test.mod.c crypto_zip_test.mod.o \
	zip_test_einj.o zip_test_einj.mod.c zip_test_einj.mod.o \
	modules.order Module.symvers zip_test_einj.ko
