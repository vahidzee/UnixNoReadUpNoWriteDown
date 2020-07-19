obj-m += pid_tracker.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

test:
	-sudo rmmod pid_tracker
	-sudo rm /dev/pid_tracker
	sudo dmesg -C
	sudo insmod pid_tracker.ko
	dmesg
	sudo su

