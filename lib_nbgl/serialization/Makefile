
INC := -I . -I ../include -I ../../main/stubbed_api
DEFINES := -DLINUX_SIMU -DHAVE_LANGUAGE_PACK -DNBGL_GENERATE_DATA_TEST -DUSE_IMAGE_ID

ifdef TARGET_STAX
DEFINES += -DHAVE_SE_TOUCH
DEFINES += -DSCREEN_SIZE_WALLET
DEFINES += -DTARGET_STAX
endif

default: generate_data_test.c
	gcc -g $(INC) generate_data_test.c $(DEFINES) ../src/nbgl_serialize.c -o generate_data_test

run_test: default
	./generate_data_test > data_test.txt
	pytest -s -vv
	rm data_test.txt

clean:
	rm generate_data_test
