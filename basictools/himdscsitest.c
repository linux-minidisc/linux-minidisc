/*
 *   himdscsitest.c - send various SCSI commands to HiMD Walkman
 *
 * - see Wiki: <https://wiki.physik.fu-berlin.de/linux-minidisc/doku.php?id=himdscsi>
 * - requires cdrtools to be installed
 *
 * - compile with: gcc -D__LINUX_X86_GCC32 himdscsitest.c -I/opt/schily/include -L/opt/schily/lib  -lscg -lscgcmd -lschily -o himdscsitest (on 32 bit Linux with gcc)
 * - compile with: gcc -D__LINUX_X86_GCC64 himdscsitest.c -I/opt/schily/include -L/opt/schily/lib  -lscg -lscgcmd -lschily -o himdscsitest (on 64 bit Linux with gcc)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <schily/schily.h>
#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#define TRUE 1   /* use return values of boolean type in some static functions */
#define FALSE 0

#define SONY_SPECIFIC_COMMAND 0xC2
#define FORMAT_ERASE_FLAGS 3
#define FORMAT_HIMD 1
#define ERASE_HIMD 0
#define GET_TIME 0x50
#define SET_TIME 0x90
#define ALLOW_REMOVAL 0
#define PREVENT_REMOVAL 1

/*#define MEDIUM_60_MIN: can anyone test this, I have no 60 min MD*/
#define MEDIUM_74_MIN 283
#define MEDIUM_80_MIN 305
#define MEDIUM_HIMD_1GB 1011

#define MAX_DEVICE_LEN 256
#define SCSI_TIMEOUT 20

static void read_capacity(SCSI * scgp)
{
  struct scg_cmd * scmd = scgp->scmd;
  struct scsi_capacity * cap = scgp->cap;

  memset(scmd, 0, sizeof(struct scg_cmd));
  scmd->addr = (caddr_t)cap;
  scmd->size = sizeof(struct scsi_capacity);
  scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
  scmd->cdb_len = SC_G1_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->cdb.g1_cdb.cmd = 0x25;	/* Read Capacity */
  scmd->cdb.g1_cdb.lun = scg_lun(scgp);
  g1_cdblen(&scmd->cdb.g1_cdb, 0); /* Full Media */

  scgp->cmdname = "read capacity";

  if (scg_cmd(scgp) < 0)
    {
      cap->c_bsize = 0;
      cap->c_baddr = 0;
    }
  else
    {
      cap->c_bsize = a_to_4_byte(&cap->c_bsize);
      cap->c_baddr = a_to_4_byte(&cap->c_baddr);
    }
}

static int capacity_mb(SCSI * scgp)
{
  double dkb = (scgp->cap->c_baddr+1.0) * (scgp->cap->c_bsize/1024.0);
  int mb = (dkb / 1000.0 * 1.024);
  return mb;
}

static int prevent_removal(SCSI * scgp, int prevent)
{
  struct scg_cmd * scmd = scgp->scmd;

  memset(scmd, 0, sizeof(struct scg_cmd));
  scmd->addr = (caddr_t)0;
  scmd->size = 0;
  scmd->cdb_len = SC_G0_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->flags = SCG_DISRE_ENA;
  scmd->cdb.g0_cdb.cmd = 0x1E;
  scmd->cdb.g0_cdb.lun = scg_lun(scgp);
  scmd->cdb.g0_cdb.count = prevent & 1;

  scgp->cmdname = "allow / prevent removal";

  return scg_cmd(scgp);
}

static int test_unit_ready(SCSI * scgp)
{
  struct scg_cmd * scmd = scgp->scmd;

  memset(scmd, 0, sizeof(struct scg_cmd));
  scmd->addr = (caddr_t)0;
  scmd->size = 0;
  scmd->flags = SCG_DISRE_ENA;
  scmd->cdb_len = SC_G0_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
  scmd->cdb.g0_cdb.lun = scg_lun(scgp);

  scgp->cmdname = "test unit ready";

  return scg_cmd(scgp);
}

static int send_command(SCSI * scgp, const char * command, int len, char * name)
{
  struct scg_cmd * scmd = scgp->scmd;

  scgp->cmdname = name;

  memset(scmd, 0, sizeof(struct scg_cmd));
  scmd->addr = (caddr_t)0;
  scmd->size = 0;
  scmd->cdb_len = len;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->flags = SCG_DISRE_ENA;
  memcpy(scmd->cdb.cmd_cdb, command, scmd->cdb_len);

  return scg_cmd(scgp);
}

static int send_command_with_buffer(SCSI * scgp, const char * command, int len, char * name,
                                    char * outbuf, int outlen, int mode)
{
  struct scg_cmd * scmd = scgp->scmd;

  scgp->cmdname = name;

  memset(scmd, 0, sizeof(struct scg_cmd));
  scmd->addr = (caddr_t)outbuf;
  scmd->size = outlen;
  scmd->cdb_len = len;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->flags = mode | SCG_DISRE_ENA;
  memcpy(scmd->cdb.cmd_cdb, command, scmd->cdb_len);

  return scg_cmd(scgp);
}

static void read_time(struct tm * tm, char * time)
{
  tm->tm_isdst = time[0] & 0xff;
  tm->tm_sec = time[1] & 0xff;
  tm->tm_min = time[2] & 0xff;
  tm->tm_hour = time[3] & 0xff;
  tm->tm_mday = time[4] & 0xff;
  tm->tm_mon = time[5] & 0xff;
  tm->tm_year = time[6] & 0xff;

  mktime(tm); /* this will set tm_wday and tm_yday correctly*/
}

static void make_settime_cmd(struct tm * tm, char * cmd)
{
  memset(cmd, 0, 12);
  cmd[0] = SONY_SPECIFIC_COMMAND;
  cmd[3] = SET_TIME;
  cmd[5] = tm->tm_sec & 0xff;
  cmd[6] = tm->tm_min & 0xff;
  cmd[7] = tm->tm_hour & 0xff;
  cmd[8] = tm->tm_mday & 0xff;
  cmd[9] = tm->tm_mon & 0xff;
  cmd[10] = tm->tm_year & 0xff;
}

int unit_ready(SCSI * scgp)
{
  if(test_unit_ready(scgp) < 0)
    return FALSE;
  return TRUE;
}

int wait_for_unit_ready(SCSI *scgp)
{
  int c, k;
  int ret = 0;

  if (test_unit_ready(scgp) >= 0)
    return TRUE;

  while((ret = test_unit_ready(scgp)) < 0)
    {
      if (!scgp->scmd->scb.busy)
        {
	  c = scg_sense_code(scgp);  /* Abort if it does not make sense to wait. */
	  k = scg_sense_key(scgp);   /* 0x30 == Cannot read medium ; 0x3A == Medium not present*/
	  if ((k == SC_NOT_READY && (c == 0x3A || c == 0x30)) ||(k == SC_MEDIUM_ERROR))
	    break;
        }
    }

  if(ret < 0)
    return FALSE;
  return TRUE;
}

void format_medium(SCSI * scgp, char * command, char * errstr)
{
  int type;
  command[0] = SONY_SPECIFIC_COMMAND;
  command[3] = FORMAT_HIMD;
  command[4] = FORMAT_ERASE_FLAGS;

  type = capacity_mb(scgp);
  if(type == 0)
    printf("Error: Cannot read capacity\n");
  else
    {
      if(send_command(scgp, command, 12, "himd format") < 0)
	fprintf(stderr,"Error formatting medium:\n%s",
		scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
			     NULL, errstr, sizeof(errstr)));
      else
	{
	  printf("Please wait, formatting device.\n");
	  if(!wait_for_unit_ready(scgp))
	    fprintf(stderr,"Error waiting for unit ready:\n%s",
		    scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
				 NULL, errstr, sizeof(errstr)));
	  else
	    printf("HiMD successfully formatted.\n");
	}
    }
}

void erase_medium(SCSI * scgp, char * command, char * errstr)
{
  int type;
  command[0] = SONY_SPECIFIC_COMMAND;
  command[3] = ERASE_HIMD;
  command[4] = FORMAT_ERASE_FLAGS;

  type = capacity_mb(scgp);
  if(type == 0)
    printf("Error: Cannot read capacity.\n");
  else if(type == MEDIUM_HIMD_1GB)
    printf("Error: Erasing 1GB HiMD medium is not supported\nPlease use format to delete all tracks.\n");
  else
    {
      if(send_command(scgp, command, 12, "erase format") < 0)
	fprintf(stderr,"Error erasing medium:\n%s",
		scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
			     NULL, errstr, sizeof(errstr)));
      else
	{
	  printf("Please wait, while erasing medium.\n");
	  if(!wait_for_unit_ready(scgp))
	    fprintf(stderr,"Error waiting for unit ready:\n%s",
		    scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
				 NULL, errstr, sizeof(errstr)));
	  else
	    printf("HiMD successfully erased.\n");
	}
    }
}

void get_disc_id(SCSI * scgp, char * command, char * errstr)
{
  char buffer[18];
  unsigned char * discid = buffer;
  int i;
  command[0] = 0xA4;
  command[7] = 0xBD;
  command[9] = sizeof(buffer);
  command[10] = 0x3D;

  memset(buffer, 0, sizeof(buffer));

  if(send_command_with_buffer(scgp, command, 12, "get disc id", buffer,
			      sizeof(buffer), SCG_RECV_DATA) < 0)
    fprintf(stderr, "Cannot read disc id:\n%s",
	    scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
			 NULL, errstr, sizeof(errstr)));
  else
    {
      printf("Disc ID: ");
      for(i = 2;i < 0x12;++i)
	fprintf(stdout, "%02x",discid[i]);
      puts("");
    }
}

void get_time(SCSI * scgp, char * command, char * errstr)
{
  char buffer[7];
  struct tm * tm;

  memset(buffer, 0x0, sizeof(buffer));
  command[0] = SONY_SPECIFIC_COMMAND;
  command[3] = GET_TIME;
  command[8] = sizeof(buffer);

  if(send_command_with_buffer(scgp, command, 12, "get time", buffer,
			      7, SCG_RECV_DATA) < 0)
    fprintf(stderr,"Cannot read device time:\n%s",
	    scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
			 NULL, errstr, sizeof(errstr)));
  else
    {
      read_time(tm, buffer);
      fprintf(stderr, "Device time is: %s\n", asctime(tm));
    }
}

void set_localtime(SCSI * scgp, char * command, char * errstr)
{
  time_t t = time(NULL);
  struct tm * tme;

  if(t == -1)
    {
      printf("Error: Cannot get local time.\n");
      return;
    }

  tme = localtime(&t);
  fprintf(stderr, "Reading local computer time: %s\n", asctime(tme));
  make_settime_cmd(tme, command);

  if(send_command(scgp, command, 12, "set time") < 0)
    fprintf(stderr,"Cannot set device time:\n%s",
	    scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
			 NULL, errstr, sizeof(errstr)));
  else
    printf("Time successfully set.\n");
}

void eject_medium(SCSI * scgp, char * errstr)
{
  static const char cmd_loadunload[6] = {0x1B,0,0,0,2,0};  /* use load/unload scsi command */

  prevent_removal(scgp, ALLOW_REMOVAL);

  if(send_command(scgp, cmd_loadunload, 6, "eject") < 0)
    fprintf(stderr,"Cannot eject medium:\n%s",
	    scg_sensemsg(0, scg_sense_code(scgp), scg_sense_qual(scgp),
			 NULL, errstr, sizeof(errstr)));
}

void usage(char * cmdname)
{
  printf("Usage: %s <devicename> <command>, where <command> is either of:\n\n\
          format       - erases all tracks on disc\n\
          erase        - erases all tracks and himd file system (not om 1GB medium)\n\
          discid       - reads the disc id of the inserted medium\n\
          gettime      - reads the time of device internal clock\n\
          setlocaltime - syncs device internal clock with your local computer time\n\
          capacity     - shows the capacity of inserted medium in MB\n\
          eject        - ejects the medium (same as pressing stop button on device)\n", cmdname);
}

int main(int argc, char ** argv)
{

  SCSI * scgp = NULL;
  char command[12];
  char dev[MAX_DEVICE_LEN];
  char errstr[80];

  if (argc == 2 && (strcmp (argv[1], "help") == 0)) {
    usage(argv[0]);
    return 0;
  }

  if (argc != 3) {
    printf("Please specify device name and command to be sent. Use \"%s help\" to display a help.\n", argv[0]);
    return 0;
  }

  memset(dev, 0, MAX_DEVICE_LEN);
  strcpy(dev, argv[1]);

  // open scsi driver
  scgp = scg_open(dev, errstr, sizeof(errstr), 0, 1);

  if(!scgp)
    {
      fputs("Cannot open scsi driver.\n", stderr);
      return 1;
    }

  if(scgp->addr.scsibus == -2 && scgp->addr.target == -2)   // scsi device not found, search by devicename
    {                                                         // this is nessessary on windows when driveletter is used
      ret = scg__open(scgp, dev);
      if(!ret || (scgp->addr.scsibus == -2 && scgp->addr.target == -2))
	{
	  fprintf(stderr, "Cannot open SCSI device for %s\n", dev);
	  scg__close(scgp);
	  return 1;
	}
    }

  if(!unit_ready)
    {
      printf("Error: device not ready.\n");
      scg__close(scgp);
      return 1;
    }

  scgp->silent++;                             /* do not print debug messages from libscg */
  scg_settimeout(scgp, SCSI_TIMEOUT);
  read_capacity(scgp);
  prevent_removal(scgp, PREVENT_REMOVAL);
  memset(command, 0, 12);

  if(strcmp(argv[2],"format") == 0)         /* aka "Erase All", deletes all tracks but leaves */
    format_medium(scgp, command, errstr);   /* himd file system on disc                       */

  else if(strcmp(argv[2],"erase") == 0)          /* deletes all incl. himd file system, */
    erase_medium(scgp, command, errstr);    /* will not work on 1GB himd media  */

  else if(strcmp(argv[2],"discid") == 0)
    get_disc_id(scgp, command, errstr);

  else if(strcmp(argv[2],"gettime") == 0)
    get_time(scgp, command, errstr);

  else if(strcmp(argv[2],"setlocaltime") == 0)
    set_localtime(scgp, command, errstr);

  else if(strcmp(argv[2],"capacity") == 0)
    {
      int capacity = capacity_mb(scgp);
      if(capacity == 0)
	fputs("Error: Cannot read capacity.\n", stderr);
      else
	fprintf(stderr, "Capacity: %d MB\n", capacity);
    }
  else if(strcmp(argv[2],"eject") == 0)
    eject_medium(scgp, errstr);
  else
    {
      usage (argv[0]);
      return 1;
    }

  prevent_removal(scgp, ALLOW_REMOVAL);
  scg__close(scgp);

  return 0;
}
