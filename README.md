# Start

```bash
orangefs_install/sbin/pvfs2-server orangefs-server.conf wb_info.conf -f
orangefs_install/sbin/pvfs2-server orangefs-server.conf wb_info.conf
```

# Sample wb_info.conf

```bash
buff_path_1 : /mnt/ssd1

buff_path_2 : /mnt/ssd2

buff_threshold : 8GB

high_watermark : 0.45
low_watermark : 0.3

rs_length : 128

```

# Compile

chage `src/io/trove/trove-handle-mgmt/module.mk` to:

```bash
DIR := src/io/trove/trove-handle-mgmt
SERVERSRC += \
	$(DIR)/avltree.c \
	$(DIR)/my_avltree.c \
	$(DIR)/trove-extentlist.c \
	$(DIR)/trove-ledger.c \
	$(DIR)/trove-handle-mgmt.c


```

Other operations remain the same as orangefs

If you want to know more about SSDUP, please refer to the ICS paper:
Xuanhua Shi, Ming Li, Wei Liu, Hai Jin, Chen Yu, and Yong Chen, "SSDUP: A Traffic-Aware SSD Burst Buffer for HPC Systems". in Proceedings of the ACM International Conference on Supercomputing (ICS), Chicago, Illinois, USA, June 13-16, 2017.
