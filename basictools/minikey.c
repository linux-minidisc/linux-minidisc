#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>

#include <stdio.h>

#define HIMD_KEY_HANDSHAKE_PKT1 0x30 /* to MD */
#define HIMD_KEY_HANDSHAKE_PKT2 0x31 /* from MD */
#define HIMD_KEY_HANDSHAKE_PKT3 0x32 /* to MD */
#define HIMD_KEY_HANDSHAKE_PKT4 0x33 /* from MD */
#define HIMD_KEY_UNIQUEID 0x39
#define HIMD_KEY_LEAFID 0x3B
#define HIMD_KEY_DISCID 0x3D

int main(int argc, char ** argv)
{
    int fd;
    int res;
    int i;
    struct sg_io_hdr sg;
    unsigned char command[12];
    unsigned char reply[200];
    unsigned char sense[16];
    const unsigned int index = 0;
    
    if(argc < 2)
    {
        fputs("Please specify the path to the sg device\n",stderr);
        return 1;
    }
    
    fd = open(argv[1],O_RDWR);
    if(fd < 0)
    {
        perror("Cannot open device");
        return 1;
    }
    if(ioctl(fd,SG_GET_NUM_WAITING,&i) < 0)
    {
        perror("ioctl SG_GET_ACCESS_COUNT failed. Is it really sg? Error");
        return 1;
    }
    command[0] = 0xA4;	/* SENSE KEYDATA (HiMD-specific)  */
    command[1] = 0; /* LUN 0 */
    command[2] = index >> 24; /* 32 */
    command[3] = index >> 16; /* bits */
    command[4] = index >> 8; /* of */
    command[5] = index; /* index */
    command[6] = 0;
    command[7] = 0xBD; /* magic byte, no idea what it means */
    command[8] = 0; /* high byte reply size */
    command[9] = 0x12; /* low byte reply size */
    command[10] = HIMD_KEY_DISCID;
    command[11] = 0;
    
    sg.interface_id = 'S';
    sg.dxfer_direction = SG_DXFER_FROM_DEV;
    sg.cmd_len = 12;
    sg.mx_sb_len = 16;
    sg.iovec_count = 0;
    sg.dxfer_len = 0x12;
    sg.dxferp = reply;
    sg.cmdp = command;
    sg.sbp = sense;
    sg.timeout = 10000000;
    sg.flags = 0;
    sg.pack_id = 0;
    sg.usr_ptr = NULL;
    
    res = write(fd,&sg,sizeof sg);
    if(res < 0)
    {
        perror("sending SCSI command");
        return 1;
    }
    
    res = read(fd,&sg,sizeof sg);
    if(res < 0)
    {
        perror("receiving SCSI answer");
        return 1;
    }

    if(sg.sb_len_wr)
    {
        printf("Getting Disc ID FAILED! Sense data: ");
        for(i = 0;i < sg.sb_len_wr;i++)
            printf("%02X ",(unsigned char)sense[i]);
        putchar('\n');
        return 1;
    }

    printf("Disc ID: ");
    for(i = 0;i < 16;++i)
        printf("%02X",(unsigned char)reply[i+2]);
    putchar('\n');

    close(fd);
    return 0;
}
