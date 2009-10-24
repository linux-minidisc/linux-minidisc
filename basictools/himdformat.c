#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <string.h>

#include <stdio.h>

int main(int argc, char ** argv)
{
    int fd;
    int res;
    int i;
    struct sg_io_hdr sg;
    unsigned char command[12];
    unsigned char reply[200];
    unsigned char sense[16];
    
    if(argc < 2)
    {
        fputs("Please specify the path to the scsi device (sgN or sdX)\n",stderr);
        return 1;
    }
    
    fd = open(argv[1],O_RDWR);
    if(fd < 0)
    {
        perror("Cannot open device");
        return 1;
    }
    if(ioctl(fd,SG_GET_VERSION_NUM,&i) < 0)
    {
        perror("ioctl SG_GET_VERSTION_NUM failed. Missing sg support?");
        return 1;
    }
    
    memset(command,0,12);
    command[0] = 0xC2;	/* sony special commands */
    command[4] = 3;     /* subcommand: format */
    
    sg.interface_id = 'S';
    sg.dxfer_direction = SG_DXFER_NONE;
    sg.cmd_len = 12;
    sg.mx_sb_len = 16;
    sg.iovec_count = 0;
    sg.dxfer_len = 0;
    sg.dxferp = reply;
    sg.cmdp = command;
    sg.sbp = sense;
    sg.timeout = 10000000;
    sg.flags = 0;
    sg.pack_id = 0;
    sg.usr_ptr = NULL;
    
    res = ioctl(fd,SG_IO,&sg);
    if(res < 0)
    {
        perror("performing SCSI command");
        return 1;
    }
    
    if(sg.sb_len_wr)
    {
        printf("Formatting failed! Sense data: ");
        for(i = 0;i < sg.sb_len_wr;i++)
            printf("%02X ",(unsigned char)sense[i]);
        putchar('\n');
        return 1;
    }

    close(fd);
    return 0;
}
