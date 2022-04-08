@echo off
ppu-lv2-gcc -O3 -c main.c -o main.o
ppu-lv2-gcc -O3 -c tests.c -o tests.o

ppu-lv2-gcc -O3 main.o tests.o -lsysutil_stub -lio_stub -lm -lsysmodule_stub -lnet_stub -lnetctl_stub -o sys_net_test.elf
make_fself sys_net_test.elf sys_net_test.self
