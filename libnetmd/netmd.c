#include "libnetmd.h"

void print_disc_info(usb_dev_handle* devh, minidisc *md);
void print_current_track_info(usb_dev_handle* devh);
void print_syntax();
void import_m3u_playlist(usb_dev_handle* devh, const char *file);
/* Max line length we support in M3U files... should match MD TOC max */
#define M3U_LINE_MAX	128

int main(int argc, char* argv[])
{
	struct usb_device* netmd;
	usb_dev_handle* devh;
	minidisc my_minidisc, *md = &my_minidisc;
	int i = 0;
	int j = 0;
	char name[16];
	
	netmd = init_netmd();

	printf("\n\n");
	if(!netmd)
	{
		printf( "No NetMD devices detected.\n" );
		return -1;
	}

	printf("Found a NetMD device!\n");

	devh = open_netmd(netmd);
	if(!devh)
	{
		printf("Error opening netmd\n%s\n", strerror(errno));
		return -1;
	}

	get_devname(devh, name, 16);
	printf("%s\n", name);

	initialize_disc_info(devh, md);
	printf("Disc Title: %s\n\n", md->groups[0].name);

	if(argc > 1)
	{
		if(strcmp("rename", argv[1]) == 0)
		{
			i = strtol(argv[2], NULL, 10);
 			set_title(devh, i, argv[3], strlen(argv[3]));
		}
		else if(strcmp("move", argv[1]) == 0)
		{
			i = strtol(argv[2], NULL, 10);
			j = strtol(argv[3], NULL, 10);
			move_track(devh, i, j);
		}
		else if(strcmp("groupmove", argv[1]) == 0)
		{
			i = strtol(argv[2], NULL, 10);
			j = strtol(argv[3], NULL, 10);
			move_group(devh, md, j, i);
		}
		else if(strcmp("write", argv[1]) == 0)
		{
			if(write_track(devh, argv[2]) < 0)
			{
				fprintf(stderr, "Error writing track %i\n", errno);
			}
		}
		else if(strcmp("newgroup", argv[1]) == 0)
		{
			create_group(devh, argv[2]);
		}
		else if(strcmp("group", argv[1]) == 0)
		{
			i = strtol(argv[2], NULL, 10);
			j = strtol(argv[3], NULL, 10);
			if(!put_track_in_group(devh, md, i, j))
			{
				printf("Something screwy happened\n");
			}
		}
		else if(strcmp("retitle", argv[1]) == 0)
		{
			i = strtol(argv[2], NULL, 10);
			set_group_title(devh, md, i, argv[3]);
		}
		else if(strcmp("writeheader", argv[1]) == 0)
		{
			/* edit group cause my logic fucked it up. */
			i = strtol(argv[2], NULL, 10);
			md->groups[i].start = strtol(argv[3], NULL, 10);
			md->groups[i].finish = strtol(argv[4], NULL, 10);
			write_disc_header(devh, md);
		}
		else if(strcmp("play", argv[1]) == 0)
		{
			if( argc > 2 ) {
				i = strtol(argv[2],NULL, 10);
				netmd_set_track( devh, i );
			}
			netmd_play(devh);
		}
		else if(strcmp("stop", argv[1]) == 0)
		{
			netmd_stop(devh);
		}
		else if(strcmp("pause", argv[1]) == 0)
		{
			netmd_pause(devh);
		}
		else if(strcmp("fforward", argv[1]) == 0)
		{
			netmd_fast_forward(devh);
		}
		else if(strcmp("rewind", argv[1]) == 0)
		{
			netmd_rewind(devh);
		}
		else if(strcmp("test", argv[1]) == 0)
		{
			test(devh);
		}
		else if(strcmp("m3uimport", argv[1]) == 0)
		{
			import_m3u_playlist(devh, argv[2]);
		}
		else if(strcmp("delete", argv[1]) == 0)
		{
			i = strtol(argv[2], NULL, 10);
			netmd_delete_track(devh, i);
		}
		else if(strcmp("deletegroup", argv[1]) == 0)
		{
			i = strtol(argv[2], NULL, 10);
			delete_group(devh, md, i);
		}
		else if(strcmp("status", argv[1]) == 0) {
			print_current_track_info(devh);	
		}
		else if(strcmp("help", argv[1]) == 0)
		{
			print_syntax();
		}
		else
		{
			print_disc_info(devh, md);
			print_syntax();
		}
	}
	else
		print_disc_info(devh, md);

	clean_disc_info(md);
	clean_netmd(devh);

	return 0;
}

void print_current_track_info(usb_dev_handle* devh) {
	float f = 0;
	int i = 0;
	int size = 0;
	char buffer[256];
	
	
	f = netmd_get_playback_position(devh);
	i = netmd_get_current_track(devh);

	size = request_title(devh, i, buffer, 256);

	if(size < 0)
	{
		printf("something really really nasty just happened.");
	}
	else {	
		printf("Current track: %s \n", buffer);
	}

	printf("Current playback position: %f \n", f);

}

void print_disc_info(usb_dev_handle* devh, minidisc* md)
{
	int i = 0;
	int size = 1;
	int g, group = 0, lastgroup = 0;
	unsigned char codec_id, bitrate_id;
	char *name, buffer[256];
	struct netmd_track time;
	struct netmd_pair const *codec, *bitrate;

	codec = bitrate = 0;

	for(i = 0; size >= 0; i++)
	{
		size = request_title(devh, i, buffer, 256);

		if(size < 0)
		{
			break;
		}

		/* Figure out which group this track is in */
		for( group = 0, g = 1; g < md->group_count; g++ )
		{
			if( (md->groups[g].start <= i+1) && (md->groups[g].finish >= i+1 ))
			{
				group = g;
				break;
			}
		}
		/* Different to the last group? */
		if( group != lastgroup )
		{
			lastgroup = group;
			if( group )			/* Group 0 is 'no group' */
			{
				printf("Group: %s\n", md->groups[group].name);
			}
		}
		/* Indent tracks which are in a group */
		if( group )
		{
			printf("  ");
		}

		request_track_time(devh, i, &time);
		request_track_codec(devh, i, &codec_id);
		request_track_bitrate(devh, i, &bitrate_id);

		codec = find_pair(codec_id, codecs);
		bitrate = find_pair(bitrate_id, bitrates);

		/* Skip 'LP:' prefix... the codec type shows up in the list anyway*/
		if( strncmp( buffer, "LP:", 3 ))
		{
			name = buffer;
		}
		else
		{
			name = buffer + 3;
		}
		
		printf("Track %2i: %-6s %6s - %02i:%02i:%02i - %s\n", 
			   i, codec->name, bitrate->name, time.minute, 
			   time.second, time.tenth, name);
	}

	/* XXX - This needs a rethink with the above method */
	/* groups may not have tracks, print the rest. */
	printf("\n--Empty Groups--\n");
	for(group=1; group < md->group_count; group++)
	{
		if(md->groups[group].start == 0 && md->groups[group].finish == 0) {
			printf("Group: %s\n", md->groups[group].name);
		}

	}
	
	printf("\n\n");
}

void import_m3u_playlist(usb_dev_handle* devh, const char *file)
{
    FILE *fp;
    char buffer[M3U_LINE_MAX + 1];
    char *s;
    int track, discard;
    
    if( file == NULL )
    {
		printf( "No filename specified\n" );
		print_syntax();
		return;
    }

    if( (fp = fopen( file, "r" )) == NULL )
    {
		printf( "Unable to open file %s: %s\n", file, strerror( errno ));
		return;
    }

    if( ! fgets( buffer, M3U_LINE_MAX, fp )) {
		printf( "File Read error\n" );
		return;
    }
    if( strcmp( buffer, "#EXTM3U\n" )) {
		printf( "Invalid M3U playlist\n" );
		return;
    }

    track = 0;
    discard = 0;
    while( fgets( buffer, M3U_LINE_MAX, fp) != NULL ) {
		/* Chomp newlines */
		s = strchr( buffer, '\n' );
		if( s )
			*s = '\0';
	
		if( buffer[0] == '#' )
		{
			/* comment, ext3inf etc... we only care about ext3inf */
			if( strncmp( buffer, "#EXTINF:", 8 ))
			{
				printf( "Skip: %s\n", buffer );
			}
			else
			{
				s = strchr( buffer, ',' );
				if( !s )
				{
					printf( "M3U Syntax error! %s\n", buffer );
				}
				else
				{
					s++;
					printf( "Title track %d - %s\n", track, s );
					set_title(devh, track, s, strlen( s )); /* XXX Handle errors */
					discard = 1;	/* don't fallback to titling by filename */
				}
			}
		}
		else
		{
			/* Filename line */
			if( discard )
			{
				/* printf( "Discard: %s\n", buffer ); */
				discard = 0;
			}
			else
			{
				/* Try and generate a title from the track name */
				s = strrchr( buffer, '.' ); /* Isolate extension */
				if( s )
					*s = 0;
				s = strrchr( buffer, '/' ); /* Isolate basename */
				if( !s )
					s = strrchr( buffer, '\\' ); /* Handle DOS paths? */
				if( !s )
					s = buffer;
				else
					s++;

				printf( "Title track %d - %s\n", track, s );
				set_title(devh, track, s, strlen( s )); /* XXX Handle errors */
			}
			track++;
		}
    }
}




void print_syntax()
{
	printf("\nNetMD test suite usage syntax\n");
	printf("rename # <string> - rename track # to <string> track numbers are off by one (ie track 1 is 0)\n");
	printf("move #1 #2 - make track #1 track #2\n");
	printf("groupmove #1 #2 - make group #1 start at track #2 !BUGGY!\n");
	printf("deletegroup #1 - delete a group, but not the tracks in it\n");
	printf("write <string> - write omg file to netmd unit !DOES NOT WORK!\n");
	printf("group #1 #2 - Stick track #1 into group #2\n");
	printf("retitle #1 <string> - rename group number #1 to <string>\n");
	printf("play #1 - play track #\n");
	printf("fforward - start fast forwarding\n");
	printf("rewind - start rewinding\n");
	printf("pause - pause the unit\n");
	printf("stop - stop the unit\n");
	printf("delete #1 - delete track\n");
	printf("m3uimport - import playlist - and title current disc using it.\n");
	printf("newgroup <string> - create a new group named <string>\n");
	printf("help - print this stuff\n");
}

