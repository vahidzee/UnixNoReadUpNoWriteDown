obj-m += phase2.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

test:
	-sudo rmmod phase2
	-sudo rm /dev/phase2
	sudo dmesg -C
	sudo insmod phase2.ko
	dmesg
	sudo su

