#!/bin/sh

qemu-system-aarch64 -cpu cortex-a57 -machine type=virt,gic-version=2 	\
		-append "rdinit=/linuxrc console=ttyAMA0 earlycon=115200 rodata=nofull"	\
		-kernel ../../arch/arm64/boot/Image	\
		-device virtio-scsi-device	\
		-smp cores=4,threads=2	\
		-m 4G	\
        -object memory-backend-ram,id=mem0,size=1G \
        -object memory-backend-ram,id=mem1,size=512M \
        -object memory-backend-ram,id=mem2,size=256M \
        -object memory-backend-ram,id=mem3,size=2G \
		-object memory-backend-ram,id=mem4,size=256M \
		-numa node,memdev=mem0,cpus=5-6,nodeid=0 \
        -numa node,memdev=mem1,cpus=3,cpus=7,nodeid=1 \
		-numa node,memdev=mem2,cpus=4,nodeid=2 \
        -numa node,memdev=mem3,cpus=0-1,nodeid=3 \
		-numa node,memdev=mem4,cpus=2,nodeid=4 \
		-numa dist,src=0,dst=0,val=10	\
		-numa dist,src=0,dst=1,val=15	\
		-numa dist,src=0,dst=2,val=20	\
		-numa dist,src=0,dst=3,val=15	\
		-numa dist,src=0,dst=4,val=20	\
		-numa dist,src=1,dst=1,val=10	\
		-numa dist,src=1,dst=2,val=25	\
		-numa dist,src=1,dst=3,val=15	\
		-numa dist,src=1,dst=4,val=15	\
		-numa dist,src=2,dst=2,val=10	\
		-numa dist,src=2,dst=3,val=30	\
		-numa dist,src=2,dst=4,val=25	\
		-numa dist,src=3,dst=3,val=10	\
		-numa dist,src=3,dst=4,val=15	\
		-numa dist,src=4,dst=4,val=10	\
		-nographic
