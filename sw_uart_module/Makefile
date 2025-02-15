KERNEL_SRC=/lib/modules/$(shell uname -r)/build

obj-m := led_module.o

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules
	gcc -o main main.c

clean:
	rm *.mod.* *.order *.symvers *.mod *.o
