
INC := -I . -I ../include -I ../../include -I ../../lib_ux_nbgl
DEFINES := -DLINUX_SIMU -DHAVE_LANGUAGE_PACK -DNBGL_GENERATE_DATA_TEST -DHAVE_NBGL -DHAVE_BOLOS

ifdef TARGET_STAX
DEFINES += -DHAVE_SE_TOUCH
DEFINES += -DSCREEN_SIZE_WALLET
INC += -I ../../target/stax/include
else
INC += -I ../../target/nanox/include
endif

default: generate_data_test.c
	gcc $(INC) generate_data_test.c $(DEFINES) ../src/nbgl_serialize.c -o generate_data_test

run_test: default
	./generate_data_test > data_test.txt
	pytest -s -vv
	rm data_test.txt

clean:
	rm generate_data_test
