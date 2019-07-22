#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <proc.h>
#include "sl811.h"
#include "usb.h"
#include "usb_hid.h"

#ifdef SL811

static bool device_present;
#define printf(...)                     kprintf(__VA_ARGS__)

void sl811_write(unsigned char reg, unsigned char data) {
    __nop
    __nop
    __nop
    outb(SL811, reg);
    __nop
    __nop
    __nop
    outb(SL811+4, data);
}
unsigned char sl811_read(unsigned char reg) {
    outb(SL811, reg);
    __nop
    __nop
    __nop
    return inb(SL811+4);
}


void sl811_write_buf(int base, const char *buf, int n) {
    int i;
    for (i = 0; i < n; ++i) 
      sl811_write(base + i, buf[i]);
}
void sl811_read_buf(int base, char *buf, int n) {
    int i;
    for (i = 0; i < n; ++i) 
      buf[i] = sl811_read(base + i);
}

#ifdef USERLAND_TEST
void print_mem(const char *start, int count) {
    int x = 0;
    int i = 0, j;
    for (; ; ++i) {
        printf("0x%08x:  ", (unsigned int)(start + i * 16));
        for (j = 0; j < 16; ++j) {
            printf("%02x ", start[i * 16 + j]);
            x += 1;
            if (x == count) {
                printf("\n");
                return;
            }
        }
        printf("\n");
    }
}

int hex2i(const char *hex) {
    int len = strlen(hex);
    int i;
    int sum = 0;
    char c;
    int base = 1;
    for (i = 0; i < len; ++i) {
        c = hex[len - i - 1];
        if ('0' <= c && c <= '9') {
            sum += (c - '0') * base;
        }
        else if ('a' <= c && c <= 'f') {
            sum += (c - 'a' + 10) * base;
        }
        else if ('A' <= c && c <= 'F') {
            sum += (c - 'A' + 10) * base;
        }
        base <<= 4;
    }
    return sum;
}
#endif

void reset_sl811() {
    sl811_write(0xf, 0xae);
    sl811_write(0x5, 0x8);
    do_sleep(50);
    sl811_write(0x5, 0x0);
    sl811_write(0xd, 0xff);
    int st = sl811_read(0xd);
    if ((st & 0x40) == 0) {
        if ((st & 0x80) != 0) {
            printf("12Mbps USB device detected\n");
        }
        else {
            printf("1.5Mbps USB device detected\n");
        }
        device_present = 1;
    }
    else {
        printf("No USB device detected\n");
        device_present = 0;
    }


    sl811_write(0xa, 0);
    sl811_write(0xb, 0x50);
    sl811_write(0xc, 0x0);
    sl811_write(0xe, 0xe0);

    sl811_write(0x5, 8);
    do_sleep(50);
    sl811_write(0xf, 0xae);
    sl811_write(0x5, 0x1);
    sl811_write(0x8, 0x1);
    sl811_write(0xd, 0xff);
    /* sl811_write(0x6, 0x1); */

}

#ifdef USERLAND_TEST
void print_sl811(int start, int count) {
    int x = 0;
    int i = 0, j;
    for (; i < 8; ++i) {
        printf("0x%02x:  ", start + i * 16);
        for (j = 0; j < 16; ++j) {
            printf("%02x ", sl811_read(start + i * 16 + j));
            x += 1;
            if (x == count) {
                printf("\n");
                return;
            }
        }
        printf("\n");
    }
}

int isset(unsigned x, int bit) {
    return (x & (1 << bit)) == 0 ? 0 : 1;
}

void print_sl811_info() {
    int cr1 = sl811_read(SL11H_CTLREG1);
    int ien = sl811_read(SL11H_IRQ_ENABLE);
    int ist = sl811_read(SL11H_IRQ_STATUS);
    int rev = sl811_read(SL11H_HWREVREG) >> 4;

    int hcr = sl811_read(SL11H_HOSTCTLREG);
    int addr = sl811_read(SL11H_BUFADDRREG);
    int len = sl811_read(SL11H_BUFLNTHREG);
    int st = sl811_read(SL11H_PKTSTATREG);
    int n_tx = sl811_read(SL11H_XFERCNTREG);

    printf("Ctrl Regs:\n");
    printf("  CR1: (Sus,Speed,JK,RST,SOF)\n");
    printf("       (%d,%d,%d,%d,%d)\n", 
        isset(cr1, 6), isset(cr1, 5), isset(cr1,4), isset(cr1,3),isset(cr1,0));

    printf("  IRQ_EN: (Detect,I/R,SOFTimer,B-Done,A-Done)\n");
    printf("       (%d,%d,%d,%d,%d)\n", 
        isset(ien, 6), isset(ien, 5), isset(ien,4), isset(ien,1),isset(ien,0));

    printf("  IRQ_ST: (Detect,I/R,SOFTimer,B-Done,A-Done)\n");
    printf("       (%d,%d,%d,%d,%d)\n", 
        isset(ist, 6), isset(ist, 5), isset(ist,4), isset(ist,1),isset(ist,0));
    printf("  REV: %x\n", rev);

    printf("USB Regs:\n");
    printf("  HCR: (Pre,D0/1,SyncSOF,ISO,Direction,En,Arm)\n");
    printf("       (%d,%d,%d,%d,%d,%d,%d)\n", 
        isset(hcr, 7), isset(hcr,6),isset(hcr,5),isset(hcr,4), isset(hcr,2),isset(hcr,1),isset(hcr,0));

    printf("  Addr/Len: (%x, %x)\n", addr, len);
    printf("  Status: (Stall,NAK,Overflow,Setup,Seq,Timeout,Err,ACK)\n");
    printf("          (%d,%d,%d,%d,%d,%d,%d,%d)\n",
        isset(st,7),isset(st,6),isset(st,5),isset(st,4),isset(st,3),isset(st,2),isset(st,1),isset(st,0));
    printf("  N Transmitted %x\n", n_tx);
}
#endif

int last_status = 0;

int wait_transfer() {
    volatile unsigned int st, ctl, irq;
    int i;
    for (i = 0; i < 100; ++i) {
        /* ctl = sl811_read(SL11H_HOSTCTLREG); */
        schedule();
        irq = sl811_read(SL11H_IRQ_STATUS);
        /* if ((ctl & 1) == 0) { */
        if ((irq & 1) != 0) {
            sl811_write(SL11H_IRQ_STATUS, 0xff);
            st = sl811_read(SL11H_PKTSTATREG);
            last_status = st;
            // usleep(wait_transfer_time);
            return 0;
            /* if ((st & 0xC7) == 1) { */
            /*     [> return i + 1; <] */
            /*     return 0; */
            /* } */
            /* else if ((st & 0xC7) != 0) { */
            /*     printf("st: %02x\n", st); */
            /*     return -1; */
            /* } */
        }
    }
    return -2;
}

int setup_packet(const struct usb_setup_pkt* ppkt,
                 int ep, 
                 int addr){
    int buf_addr = 0x10;
    sl811_write_buf(buf_addr, (unsigned char*)ppkt, sizeof(struct usb_setup_pkt));
    /* printf("write_buf %x, %x\n", buf_addr, sizeof(struct usb_setup_pkt)); */
    sl811_write(SL11H_BUFADDRREG, buf_addr);
    /* printf("bufaddr %x\n", buf_addr); */
    sl811_write(SL11H_BUFLNTHREG, sizeof(struct usb_setup_pkt));
    /* printf("buflen %x\n", sizeof(struct usb_setup_pkt)); */
    sl811_write(SL11H_PIDEPREG, SL_SETUP | ep);
    /* printf("pidep %x\n", SL_SETUP | ep); */
    sl811_write(SL11H_DEVADDRREG, addr);
    /* printf("devaddr %x\n", addr); */
    int v = SL11H_HCTLMASK_ARM | SL11H_HCTLMASK_ENABLE | SL11H_HCTLMASK_OUT | SL11H_HCTLMASK_AFTERSOF;
    /* print_sl811_info(); */
    /* printf("hostctlreg: %x\n", v); */
    sl811_write(SL11H_HOSTCTLREG, v);
    /* v = sl811_read(SL11H_HOSTCTLREG); */
    /* printf("hostctlreg: %x\n", v); */
    int p = wait_transfer();
    if (p != 0) {
        // print_sl811_info();
        if (p == -2)
            printf("setup_packet timeout\n");
        else
            printf("setup_packet error\n");
        return -1;
    }
    sl811_write(SL11H_IRQ_STATUS, 0xff);
    /* printf("setup_packet done\n"); */
    return 0;
}

// IN DATA1
int in_packet(char *buf,
              int len, 
              int ep, 
              int addr,
              int wait_ack) {
    int buf_addr = 0x20;
    int toggle = 0;

    while (1) {
        sl811_write(SL11H_BUFADDRREG, buf_addr);
        sl811_write(SL11H_BUFLNTHREG, len);
        sl811_write(SL11H_PIDEPREG, SL_IN | ep);
        sl811_write(SL11H_DEVADDRREG, addr);
        sl811_write(SL11H_IRQ_STATUS, 0xff);
        sl811_write(SL11H_HOSTCTLREG, SL11H_HCTLMASK_ARM | SL11H_HCTLMASK_ENABLE | SL11H_HCTLMASK_IN | SL11H_HCTLMASK_AFTERSOF);
        int p = wait_transfer();
        sl811_write(SL11H_IRQ_STATUS, 0xff);
        if (p != 0) {
            // print_sl811_info();
            if (p == -2)
                printf("in_packet timeout\n");
            else
                printf("in packet error\n");
            return -1;
        }
        if ((last_status & SL11H_STATMASK_ACK)) {
            sl811_read_buf(buf_addr, buf, len);
            return 0;
        }
        else if ((last_status & SL11H_STATMASK_TMOUT))
            return -1;
        else if ((last_status & SL11H_STATMASK_STALL))
            return -1;
        else if ((last_status & SL11H_STATMASK_NAK) && !wait_ack)
            return -1;
    }
}

// DATA1
int status_packet(int ep, 
                  int addr) {
    int buf_addr = 0x20;
    sl811_write(SL11H_BUFADDRREG, buf_addr);
    sl811_write(SL11H_BUFLNTHREG, 0);
    sl811_write(SL11H_PIDEPREG, SL_OUT | ep);
    sl811_write(SL11H_DEVADDRREG, addr);
    sl811_write(SL11H_HOSTCTLREG, SL11H_HCTLMASK_ARM | SL11H_HCTLMASK_ENABLE | SL11H_HCTLMASK_OUT | SL11H_HCTLMASK_TOGGLE | SL11H_HCTLMASK_AFTERSOF);

    int p = wait_transfer();
    if (p != 0) {
        if (p == -2)
            printf("status_packet timeout\n");
        else
            printf("status_packet error\n");
        // print_sl811_info();
        return -1;
    }
    return 0;
}

static char g_buf[512];

#define SET(ptr, member, val) \
      struct_set((ptr), \
                (unsigned int)(&(ptr)->member) - (unsigned int)(ptr), \
                sizeof((ptr)->member), \
                (val))

void
struct_set(void *ptr, int offset, int len, unsigned int val) {
    unsigned int addr = (unsigned int)ptr + offset;
    unsigned int aligned_addr = (addr & 0xFFFFFFFC);
    unsigned int off = addr - aligned_addr;
    unsigned int cell_data = *((unsigned int*)aligned_addr);
    unsigned int cell_zeroed = cell_data & ~(((1<<(len*8))-1) << (off*8));
    unsigned int data_shifted = (val & ((1<<(len*8))-1)) << (off*8);
    *((unsigned int*)aligned_addr) = cell_zeroed | data_shifted;
}

int
usb_get_dev_desc(struct usb_dev_desc *desc, int ep, int addr) {
    int a, b, c;
    struct usb_setup_pkt pkt;
    SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE);
    SET(&pkt, req, GET_DESCRIPTOR);
    SET(&pkt, val, (USB_DESC_TYPE_DEVICE) << 8 | 0);
    SET(&pkt, idx, 0);
    SET(&pkt, cnt, sizeof(struct usb_dev_desc));
    a = setup_packet(&pkt, ep, addr);
    b = in_packet((char*)desc, sizeof(struct usb_dev_desc), ep, addr, 1); 
    c = status_packet(ep, addr);
    // printf("Cycles: %d, %d, %d\n", a, b, c);
    // printf("DeviceDescriptor returned:\n");
    // print_mem((const char*)desc, sizeof(*desc));
    if(a || b || c){
        printf("usb_get_dev_desc failed\n");
        return -1;
    }
    return 0;
}

int
usb_set_address(int ep, int addr, int new_addr) {
    int a, b; char buf[10];
    struct usb_setup_pkt pkt;
    SET(&pkt, req_type, USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE);
    SET(&pkt, req, SET_ADDRESS);
    SET(&pkt, val, new_addr);
    SET(&pkt, idx, 0);
    SET(&pkt, cnt, 0);
    a = setup_packet(&pkt, ep, addr);
    b = in_packet(buf, 0, ep, addr, 1);
    // printf("Cycles: %d, %d\n", a, b);
    // printf("Address set: %x\n", new_addr);
    if(a || b){
        printf("usb_set_address failed\n");
        return -1;
    }
    return 0;
}

int
usb_get_conf_desc(char *buf, int len, int ep, int addr) {
    int a, b, c, n_read = 0, n_max = 120, rest = len, offset = 0, pkt_size = 0;
    struct usb_setup_pkt pkt;
    SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE);
    SET(&pkt, req, GET_DESCRIPTOR);
    SET(&pkt, val, (USB_DESC_TYPE_CONF) << 8 | 0);
    SET(&pkt, idx, 0);
    SET(&pkt, cnt, len);
    a = setup_packet(&pkt, ep, addr);
    while (rest > 0) {
        offset = len - rest;
        if (rest > n_max)
            pkt_size = n_max;
        else
            pkt_size = rest;
        rest -= pkt_size;
        b = in_packet(((char*)g_buf + offset), pkt_size, ep, addr, 1); 
        if(b)
            break;
    }
    c = status_packet(ep, addr);
    // printf("Cycles: %d, %d, %d\n", a, b, c);
    // printf("ConfigurationDescriptor returned:\n");
    // print_mem((const char*)buf, len);
    if(a || b || c){
        printf("usb_get_conf_desc failed\n");
        return -1;
    }
    return 0;
}
int
usb_get_str_desc(char *buf, int *plen, int ep, int addr, int idx) {
    int a, b, c, max_packet_size = 0x3C; // 60Bytes
    struct usb_setup_pkt pkt;
    SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE);
    SET(&pkt, req, GET_DESCRIPTOR);
    SET(&pkt, val, (USB_DESC_TYPE_STR) << 8 | idx);
    SET(&pkt, idx, 0);
    SET(&pkt, cnt, max_packet_size);
    a = setup_packet(&pkt, ep, addr);
    b = in_packet((char*)buf, max_packet_size, ep, addr, 1); 
    c = status_packet(ep, addr);
    *plen = sl811_read(0x20);
    // printf("Cycles: %d, %d, %d\n", a, b, c);
    // printf("StringDescriptor returned, len %d:\n", *plen);
    // print_mem((const char*)buf, *plen);
    if(a || b || c){
        printf("usb_get_str_desc failed\n");
        return -1;
    }
    return 0;
}
int
usb_set_conf(int ep, int addr, int idx) {
    int a, b; char buf[10];
    struct usb_setup_pkt pkt;
    SET(&pkt, req_type, USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE);
    SET(&pkt, req, SET_CONF);
    SET(&pkt, val, idx);
    SET(&pkt, idx, 0);
    SET(&pkt, cnt, 0);
    a = setup_packet(&pkt, ep, addr);
    b = in_packet(buf, 0, ep, addr, 1);
    /* c = status_packet(ep, addr); */
    // printf("Cycles: %d, %d\n", a, b);
    // printf("Configuration set: %x\n", idx);
    if(a || b){
        printf("usb_set_conf failed\n");
        return -1;
    }
    return 0;
}

#ifdef USERLAND_TEST
void
usbhub_get_status(char *buf, int ep, int addr, int port) {
    int a, b, c;
    struct usb_setup_pkt pkt;
    int status_len = 4;
    SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_OTHER);
    SET(&pkt, req, GET_STATUS);
    SET(&pkt, val, 0);
    SET(&pkt, idx, port);
    SET(&pkt, cnt, status_len);
    a = setup_packet(&pkt, ep, addr);
    b = in_packet(buf, status_len, ep, addr, 1); 
    c = status_packet(ep, addr);
    printf("Cycles: %d, %d, %d\n", a, b, c);
    printf("USB Hub GetStatus returned:\n");
    print_mem(buf, status_len);
}
void
usbhub_set_feature(int ep, int addr, int port, int feature) {
    int a, b; char buf[10];
    struct usb_setup_pkt pkt;
    SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_OTHER);
    SET(&pkt, req, SET_FEATURE);
    SET(&pkt, val, feature);
    SET(&pkt, idx, port);
    SET(&pkt, cnt, 0);
    a = setup_packet(&pkt, ep, addr);
    b = in_packet(buf, 0, ep, addr, 1);
    /* c = status_packet(ep, addr); */
    printf("Cycles: %d, %d\n", a, b);
    printf("USB Hub Feature set: %x\n", feature);
}
void
usbhub_clear_feature(int ep, int addr, int port, int feature) {
    int a, b; char buf[10];
    struct usb_setup_pkt pkt;
    SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_OTHER);
    SET(&pkt, req, CLEAR_FEATURE);
    SET(&pkt, val, feature);
    SET(&pkt, idx, port);
    SET(&pkt, cnt, 0);
    a = setup_packet(&pkt, ep, addr);
    b = in_packet(buf, 0, ep, addr, 1);
    printf("Cycles: %d, %d\n", a, b);
    printf("USB Hub Feature cleared: %x\n", feature);
}
#endif

int
usb_int_in(char *buf, int len, int ep, int addr) {
    int a, b, i;
    char scan;
    a = in_packet(buf, len, ep, addr, 0); 
    b = status_packet(ep, addr);
    return a;
}

static int
kthread_sl811(void *arg) {
    bool pressed[128] = {0};
    int addr = 1, ep = 1, i;
    printf("sl811 kthread started\n");
    while(1){
        // kprintf("kthread_sl811 running\n");
        // do_sleep(1000);

        if (usb_int_in(g_buf, 8, ep, addr) == 0){
            bool pressed_now[128] = {0};
            int scan, ascii;
            for (i = 2; i < 8; ++i) {
                scan = g_buf[i];
                if(scan < 0 || scan >= 128)
                    continue;
                if((g_buf[0] & KEY_MOD_LSHIFT) || (g_buf[0] & KEY_MOD_RSHIFT))
                    ascii = usb_hid_usage_table_shift[scan];
                else
                    ascii = usb_hid_usage_table[scan];
                // printf("%d ", scan);
                pressed_now[ascii] = 1;
            }
            // printf("\n");
            
            for(i = 1; i < 128; i++) {
                if(!pressed[i] && pressed_now[i]) {
                    char ascii = i;
                    extern void dev_stdin_write(char c);
                    dev_stdin_write(ascii);
                }
                pressed[i] = pressed_now[i];
            }
        }
        do_sleep(8);
    }
    return 0;
}

void
usb_sl811_init(void)
{
    int addr = 0, len; 
    int ep = 0;
    struct usb_dev_desc desc;
    reset_sl811();
    if(!device_present)
        return;
    if(usb_get_dev_desc(&desc, ep, addr) != 0)
        return;
    if(usb_set_address(ep, addr, 1) != 0)
        return;
    addr = 1; 
    // usb_get_conf_desc(g_buf, 9, ep, addr);
    usb_set_conf(ep, addr, 1);

    int pid = kernel_thread(kthread_sl811, NULL, 0);
    // printf("sl811 kthread started pid %d\n", pid);

}

#ifdef USERLAND_TEST

void usage() {
    printf("sl811 [args]\n");
    printf("  p [addr] [size], default, addr=0,size=0x10, in hex, print regs\n");
    printf("  i, print info\n");
    printf("  w reg val, write reg\n");
    printf("  r reg, read reg\n");
    printf("  e <ops_str>, send packets, ops can be s,i,t\n");
    printf("  get_desc, get descriptor\n");
}
int main(int argc, char **argv) {
    const char *cmd;
    int reg, data;

    if (argc >= 2) {
        cmd = argv[1];
    }
    else {
        usage();
        return 1;
    }

    if (strcmp(cmd, "rst") == 0) {
        reset_sl811();
    }
    else if (strcmp(cmd, "i") == 0) {
        print_sl811_info();
    }
    else if (strcmp(cmd, "p") == 0) {
        int start = 0, count = 0x10;
        if (argc >= 3)
            start = hex2i(argv[2]);
        if (argc >= 4)
            count = hex2i(argv[3]);
        print_sl811(start, count);
    }
    else if (strcmp(cmd, "r") == 0 && argc == 3) {
        reg = hex2i(argv[2]);
        printf("%2x\n", sl811_read(reg));
    }
    else if (strcmp(cmd, "w") == 0 && argc == 4) {
        reg = hex2i(argv[2]);
        data = hex2i(argv[3]);
        sl811_write(reg, data);
    }
    else if (strcmp(cmd, "setup") == 0) {
        struct usb_setup_pkt pkt;
        SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE);
        SET(&pkt, req, GET_DESCRIPTOR);
        SET(&pkt, val, (USB_DESC_TYPE_DEVICE) << 8 | 0);
        SET(&pkt, idx, 0);
        SET(&pkt, cnt, sizeof(struct usb_dev_desc));
        setup_packet(&pkt, 0, 0);
    }
    else if (strcmp(cmd, "sofsetup") == 0) {
        /* sl811_write(0xf, 0xae); */
        /* sl811_write(0xe, 0xe0); */
        /* sl811_write(0xd, 0xff); */
        /* sl811_write(0x5, 0x01); */
        int i, v;
        for (i = 0; i < (65536 * 50); ++i) {
            v = sl811_read(SL11H_SOFTMRREG);
            if ((unsigned int)(v & 0xff) > 0x80) 
                break;
            __nop;
        }
        struct usb_setup_pkt pkt;
        SET(&pkt, req_type, USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE);
        SET(&pkt, req, GET_DESCRIPTOR);
        SET(&pkt, val, (USB_DESC_TYPE_DEVICE) << 8 | 0);
        SET(&pkt, idx, 0);
        SET(&pkt, cnt, sizeof(struct usb_dev_desc));
        setup_packet(&pkt, 0, 0);
    }
    else if (strcmp(cmd, "getdesc") == 0 && argc >= 2) {
        int addr = 0;
        struct usb_dev_desc desc;
        if (argc >= 3)
            addr = hex2i(argv[2]);
        if (argc >= 4)
            wait_transfer_time = hex2i(argv[3]);
        usb_get_dev_desc(&desc, 0, addr);
    }
    else if (strcmp(cmd, "setaddr") == 0 && argc >= 2) {
        int addr = 0, new_addr = 1;
        if (argc >= 3)
            addr = hex2i(argv[2]);
        if (argc >= 4)
            new_addr = hex2i(argv[3]);
        usb_set_address(0, addr, new_addr);
    }
    else if (strcmp(cmd, "getconf") == 0 && argc >= 2) {
        int addr = 1, len = sizeof(struct usb_conf_desc);
        if (argc >= 3)
            addr = hex2i(argv[2]);
        if (argc >= 4)
            len = hex2i(argv[3]);
        usb_get_conf_desc(g_buf, len, 0, addr);
    }
    else if (strcmp(cmd, "setconf") == 0 && argc >= 2) {
        int addr = 1, idx = 1;
        if (argc >= 3)
            addr = hex2i(argv[2]);
        if (argc >= 4)
            idx = hex2i(argv[3]);
        usb_set_conf(0, addr, idx);
    }
    else if (strcmp(cmd, "getstr") == 0 && argc >= 4) {
        int addr, idx, len;
        addr = hex2i(argv[2]);
        idx = hex2i(argv[3]);
        usb_get_str_desc(g_buf, &len, 0, addr, idx);
        g_buf[len] = 0;
        printf("StringDescriptor: '%s'", g_buf);
    }
    else if (strcmp(cmd, "getstatus") == 0 && argc >= 4) {
        int addr, port;
        addr = hex2i(argv[2]);
        port = hex2i(argv[3]);
        usbhub_get_status(g_buf, 0, addr, port);
        printf("Status : '%x'", *(unsigned int*)g_buf);
    }
    else if (strcmp(cmd, "setfeat") == 0 && argc >= 5) {
        int addr = 1, port = 1, feature = 0;
        addr = hex2i(argv[2]);
        port = hex2i(argv[3]);
        feature = hex2i(argv[4]);
        usbhub_set_feature(0, addr, port, feature);
    }
    else if (strcmp(cmd, "clearfeat") == 0 && argc >= 5) {
        int addr = 1, port = 1, feature = 0;
        addr = hex2i(argv[2]);
        port = hex2i(argv[3]);
        feature = hex2i(argv[4]);
        usbhub_clear_feature(0, addr, port, feature);
    }
    else if (strcmp(cmd, "intin") == 0 && argc >= 4) {
        int addr = 1, ep = 1;
        addr = hex2i(argv[2]);
        ep = hex2i(argv[3]);
        usb_int_in(g_buf, ep, addr);
    }
    else if (strcmp(cmd, "msleep") == 0 && argc == 3) {
        msleep(hex2i(argv[2]));
    }
    else if (strcmp(cmd, "forkloop") == 0 && argc == 2) {
        int pid;
        if ((pid = fork()) == 0) {
            printf("sl811 daemon started, from child\n");
            int addr = 1, ep = 1;
            while (1) {
                if (usb_int_in(g_buf, ep, addr) < 0)
                break;
            }
        }
        else {
            printf("sl811 daemon started pid %d\n", pid);
        }
    }
    else if (strcmp(cmd, "up") == 0 && argc == 2) {
        reset_sl811();

        int addr = 0, len; 
        int ep = 0, idx = 1;
        struct usb_dev_desc desc;
        usb_get_dev_desc(&desc, ep, addr);
        usb_set_address(ep, addr, 1);
        addr = 1; 
        len = 9;
        usb_get_conf_desc(g_buf, len, ep, addr);
        usb_set_conf(ep, addr, idx);

        int pid;
        if ((pid = fork()) == 0) {
            printf("sl811 daemon started, from child\n");
            int addr = 1, ep = 1;
            while (1) {
                if (usb_int_in(g_buf, ep, addr) < 0)
                break;
            }
        }
        else {
            printf("sl811 daemon started pid %d\n", pid);
        }
    }
    else if (strcmp(cmd, "kb") == 0) {
        int addr = 0, len; 
        int ep = 0, idx = 1;
        struct usb_dev_desc desc;
        usb_get_dev_desc(&desc, ep, addr);
        usb_set_address(ep, addr, 1);
        addr = 1; 
        len = 9;
        usb_get_conf_desc(g_buf, len, ep, addr);
        usb_set_conf(ep, addr, idx);
    }
    return 0;
}

#endif

#else
void usb_sl811_init(){}
#endif