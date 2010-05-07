/*
 * himdformat_scg.c
 * Format HiMDs using the generic SCSI abstraction library
 * from cdrtools (libscg)
 */

#include <stdio.h>
#include <string.h>

#include <schily/schily.h>
#include <scg/scgcmd.h>
#include <scg/scsitransp.h>

#define SONY_SPECIFIC_COMMAND 0xC2
#define HIMD_FORMAT 3

#define MAX_DEVICE_LEN 256
#define SCSI_TIMEOUT 20

int main(int argc, char ** argv)
{
	SCSI * scgp = NULL;
	char command[12];
	int err = 0;
	int ret;
	char dev[MAX_DEVICE_LEN];
	char errstr[80];
	char cmdname[] = "himd_format";
	struct scg_cmd * scmd;

	if(argc < 2)
	{
	    fputs("Please specify the path to the scsi device\n",stderr);
	    return -1;
	}

	memset(dev, 0, MAX_DEVICE_LEN);
	memcpy(dev, argv[1], sizeof(argv[1]));

	// open scsi driver
	scgp = scg_open(dev, errstr, sizeof(errstr), 0, NULL);
	if(!scgp)
	{
		fputs("Cannot open scsi driver", stderr);
		return -2;
	}

	if(scgp->addr.scsibus == -2 && scgp->addr.target == -2)   // scsi device not found, search by devicename;
	{                                                         // this is nessessary on Windows when drive letters
	                                                          // are used
		ret = scg__open(scgp, dev);
		if(!ret)
		{
			fprintf(stderr, "Cannot open SCSI device for %d\n", dev);
			err = -3;
			goto clean;
		}
	}

	scg_settimeout(scgp, SCSI_TIMEOUT);
	scgp->cmdname = cmdname;

	// prepare SCSI command
	scmd = scgp->scmd;
	memset(scmd, 0, sizeof(struct scg_cmd));
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->cdb_len = sizeof(command);
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->flags = SCG_DISRE_ENA;

	memset(command, 0, 12);
	command[0] = SONY_SPECIFIC_COMMAND;
	command[4] = HIMD_FORMAT;
	memcpy(scmd->cdb.cmd_cdb, command, 12);

	// send SCSI command
	if(scg_cmd(scgp) < 0)
	{
		fputs("Cannot send scsi command.", stderr);
		err = -4;
		goto clean;
	}
	else
		fprintf(stderr, "SCSI command sent successfully.");

clean:
	scg__close(scgp);

	return err;
}
