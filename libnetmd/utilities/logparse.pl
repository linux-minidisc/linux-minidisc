#!/usr/bin/perl -w

use strict;
use Getopt::Std;

# Global variable for line info to avoid passing it all around
my $LineNo;		# Line number in log file
my $LineTime;		# Time elapsed when this line was written
my $Line;		# Rest of line
my $PeekFlag = 0;	# Lookahead flag (ugh!)
my $UrbTime = 0;	# Time elapsed when this URB was sent
my $LastUrbTime = 0;	# Time elapsed when previous URB was sent
my $FreeStyle = 0;	# Allow lines with no obvious timestamp or line tag

my $LastVendor= '';	# Last vendor request

my %UrbTypeMap = (
   'URB_FUNCTION_CONTROL_TRANSFER'		=>	'Control Transfer',
   'URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE'	=>	'Get Descriptor',
   'URB_FUNCTION_SELECT_CONFIGURATION'		=>	'Select Config',
   'URB_FUNCTION_VENDOR_INTERFACE'		=>	'Vendor I/F',
);

#############################################################################
# Argument processing

my %Opt;
getopts( 'qstv', \%Opt )
    or Usage();
my $Quiet = $Opt{'q'};		# Very quiet output
my $Verbose = $Opt{'v'};	# Verbose output
my $ShowSetup = $Opt{'s'};	# Show setup packet data too
my $ShowTime = $Opt{'t'};	# Show time information

#############################################################################
# Map of opcode decode functions

my %Decoders = (
		'001808-10100000' =>	\&DecodeSetFlags,
		'001808-10100001' =>	\&DecodeSetFlags,
		'001808-10100100' =>	\&DecodeSetFlags,
		'001808-10100101' =>	\&DecodeSetFlags,
		'001808-10180100' =>	\&DecodeSetFlags,
		'001808-10180101' =>	\&DecodeSetFlags,
		'001808-80000000' =>	\&DecodeSetFlags,
		'001808-80000100' =>	\&DecodeSetFlags,
		'091808-10100000' =>	\&DecodeSetFlags,
		'091808-10100001' =>	\&DecodeSetFlags,
		'091808-10100100' =>	\&DecodeSetFlags,
		'091808-10100101' =>	\&DecodeSetFlags,
		'091808-10180100' =>	\&DecodeSetFlags,
		'091808-10180101' =>	\&DecodeSetFlags,
		'091808-80000000' =>	\&DecodeSetFlags,
		'091808-80000100' =>	\&DecodeSetFlags,
		'001806-01101000' =>	\&DecodeDiscFlagsRequest,
		'091806-01101000' =>	\&DecodeDiscFlagsReply,
		'001809-80010230' =>	\&DecodePlayerStatusRequest,
		'091809-80010230' =>	\&DecodePlayerStatusReply,
		'001809-80010330' =>	\&DecodeDiscStatusRequest,
		'091809-80010330' =>	\&DecodeDiscStatusReply,
		'001809-80010430' =>	\&DecodePlaybackStatusRequest,
		'091809-80010430' =>	\&DecodePlaybackStatusReply,
		'001806-01201001' =>	\&DecodeCheckoutStatusRequest,
		'091806-01201001' =>	\&DecodeCheckoutStatusReply,
		'001806-02201801' =>	\&DecodeDiscInfoRequest,
		'091806-02201801' =>	\&DecodeDiscInfoReply,
		'001806-02201802' =>	\&DecodeTrackTitleRequest,
		'091806-02201802' =>	\&DecodeTrackTitleReply,
		'001806-02101000' =>	\&DecodeDiscCapacityRequest,
		'091806-02101000' =>	\&DecodeDiscCapacityReply,
		'001806-02201001' =>	\&DecodeTrackInfoRequest,
		'091806-02201001' =>	\&DecodeTrackInfoReply,
		'001806-02101001' =>	\&DecodeTrackCountRequest,
		'091806-02101001' =>	\&DecodeTrackCountReply,
		'001840-ff010020' =>	\&DecodeDeleteTrack,
		'001850-ff000000' =>	\&DecodeSetPosition,
		'001850-ff010000' =>	\&DecodeSetTrack,
		'0018c3-ff390000' =>	\&DecodeFastForward,
		'0018c3-ff490000' =>	\&DecodeRewind,
		'0018c3-ff750000' =>	\&DecodeStartPlayback,
		'0018c3-ff7d0000' =>	\&DecodePause,
		'0018c5-ff000000' =>	\&DecodeStopPlayback,
		'091840-00010020' =>	\&DecodeDeleteTrack,
		'091850-00000000' =>	\&DecodeSetPosition,
		'091850-00010000' =>	\&DecodeSetTrack,
		'0918c3-00390000' =>	\&DecodeFastForward,
		'0918c3-00490000' =>	\&DecodeRewind,
		'0918c3-00750000' =>	\&DecodeStartPlayback,
		'0918c3-007d0000' =>	\&DecodePause,
		'0918c5-00000000' =>	\&DecodeStopPlayback,
		'001840-ff0000' =>	\&DecodeInitialiseDisc,
		'091840-000000' =>	\&DecodeInitialiseDisc,
);



#############################################################################
# Main loop

while( PeekLine() ) {
    if( $Line =~ /^>>>/ ) {
	ParseOutgoing();
    }
    elsif( $Line =~ /^<<</ ) {
       ParseIncoming();
    }
    else {
	print "State error: Unexpected line '$Line' at $LineNo !\n";
	ReadLine();
    }
}
print "Done.\n";
exit 0;
#############################################################################

sub ParseOutgoing {
    my $UrbDir = '->';
    my ( $UrbId, $UrbType );

    # Read and parse the URB direction & number header
    ReadLine() or die "Read error\n";
    if( $Line =~ />> +URB (\d+) going down/ ) {
	$UrbId = $1;			# Save the ID number
	# Read and parse the URB type
	ReadLine() or die "Read error\n";
	if( $Line =~ /^-- ?(\S+):/ ) {
	    $UrbType = $1;
	}
	else {
	    die "State error: Unexpected line '$Line' at $LineNo !\n";
	}
	# Time this outgoing URB as now, save the previous time
	$LastUrbTime = $UrbTime;
	$UrbTime = $LineTime;
	# Print time info if desired
	if( $ShowTime ) {
	    printf "+ %.3f (%.3f)\n", $UrbTime, $UrbTime - $LastUrbTime;
	}

	ReadUrb( $UrbDir, $UrbId, $UrbType );
    }
    else {
	die "State error: Unexpected line '$Line' at $LineNo !\n";
    }
}

sub ParseIncoming {
    my $UrbDir = '<-';
    my ( $UrbId, $UrbType );

    # Read and parse the URB direction & number header
    ReadLine() or die "Read error\n";
    if( $Line =~ /<< +URB (\d+) coming back/ ) {
	$UrbId = $1;			# Save the ID number
	# Read and parse the URB type
	ReadLine() or die "Read error\n";
	if( $Line =~ /^-- ?(\S+):/ ) {
	    $UrbType = $1;
	}
	else {
	    die "State error: Unexpected line '$Line' at $LineNo !\n";
	}
	ReadUrb( $UrbDir, $UrbId, $UrbType );
    }
    else {
	die "State error: Unexpected line '$Line' at $LineNo !\n";
    }
}

sub ReadUrb {
    my( $UrbDir, $UrbId, $UrbType ) = @_;

    my %Params;
    my %TerseParams;
    my $Data;

    while( PeekLine() ) {
	# Handle 'key = value' lines;
	if( $Line =~ /^(.*\S)\s*=\s+(.*)/ ) {
	    my $key = $1;
	    my $value = $2;
	    $Params{$key} = $value;
	    # Strip off flag explanations etc for terse version
	    if( $value =~ /^[0-9a-f]+ \(/ ) {
		$value =~ s/ .*//;
	    }
	    if( $value =~ /^00+/ ) {
		$value =~ s/^00+/0/;
	    }
	    $TerseParams{$key} = $value;
	}
	# Handle hex data (eg transfers)
	elsif( $Line =~ /^([0-9a-f ]+)$/ ) {
	    if( $Data ) {
		$Data = "$Data $Line";
	    }
	    else {
		$Data = $Line;
	    }
	}
	# Anything else must be the end of the URB data
	else {
	    last;		# Break out of the loop, ie cleanup and return
	}
	# We only peeked at the line... Now read it so the next peek gets the
	# next one
	ReadLine() or die "Read error!\n";
    }
    # Remap URB type name to something nicer if we can
    my $TypeName = $UrbType;
    if( $UrbTypeMap{$TypeName} ) {
	$TypeName = $UrbTypeMap{$TypeName};
    }

    if( $UrbType eq 'URB_FUNCTION_VENDOR_INTERFACE' ) {
	# Save vendor IDs as we print them out
	$LastVendor = $TerseParams{'Request'};
	# Put a seperator before request 0x80's to seperate things a bit
	if( $LastVendor eq '80' ) {
	    print "----------------------------------------------\n";
	}
    }

    # Decide whether or not to print anything
    my $DoPrint = 1;
    if( $Data ) {
	my $rc = DecodeNetMD( $Data );
	if( $Quiet && $rc ) {
	    $DoPrint = 0;
	}
    }

    if( $Quiet && $LastVendor ) {
	if( $UrbType eq 'URB_FUNCTION_VENDOR_INTERFACE' ) {
	    if( ($LastVendor eq '01') || ($LastVendor eq '81' )) {
		$DoPrint = 0;
	    }
	}
	if( $UrbType eq 'URB_FUNCTION_CONTROL_TRANSFER' ) {
	    if( ($LastVendor eq '01') || ($LastVendor eq '80' )) {
		$DoPrint = 0;
	    }
	}
    }

    if ( $DoPrint ) {
	# Start of line is common
	print "$UrbDir $UrbId $TypeName: ";

	if ( $UrbType eq 'URB_FUNCTION_CONTROL_TRANSFER' ) {
	    print "Buflen=$TerseParams{'TransferBufferLength'}  ";
	    unless( $Quiet ) {
		print "Flags=$TerseParams{'TransferFlags'}  ";
		print "Pipe=$TerseParams{'PipeHandle'}  ";
	    }
	} elsif ( $UrbType eq 'URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE' ) {
	    print "Index=$TerseParams{'Index'}  ";
	    print "Type=$TerseParams{'DescriptorType'}  ";
	    print "Lang=$TerseParams{'LanguageId'}  ";
	} elsif ( $UrbType eq 'URB_FUNCTION_SELECT_CONFIGURATION' ) {
	    # Nothing here yet :)
	} elsif ( $UrbType eq 'URB_FUNCTION_VENDOR_INTERFACE' ) {
	    print "Req=$TerseParams{'Request'}  ";
	    unless( $Quiet ) {
		print "Value=$TerseParams{'Value'}  ";
		print "Index=$TerseParams{'Index'}  ";
		print "Res Bits=$TerseParams{'RequestTypeReservedBits'}  ";
		print "Pipe=$TerseParams{'PipeHandle'}  " if( $Params{'PipeHandle'});
	    }
	}
	print "\n";

	# Hexdump any setup packets
	if ( $ShowSetup && $Params{'SetupPacket'} ) {
	    HexDump( "   Setup: ", 8, $Params{'SetupPacket'}, 0 );
	}

	# Hexdump any data transferred
	if ( $Data ) {
	    HexDump( "   ", 16, $Data, 1 );
	}
	# In verbose mode print out all the key/values
	if ( $Verbose ) {
	    foreach my $k (sort keys %Params) {
		print "  $k = $Params{$k}\n";
	    }
	    print "------------------\n"
	}
	if( $UrbDir eq '<-' ) {
	    print "\n";		# Extra blank line for readbility
	}
    }
#    else {
#	print "--\n";
#    }
}

#############################################################################

sub DecodeNetMD {
    my ($data) = @_;

    my @byte = split( / /, $data );

    my ( $direction, $type, $groupcode, $selector );
    if( $byte[0] eq '00' && $byte[1] eq '18' ) {
	$type = 'NetMD Request';
	$direction = 'send';
    }
    elsif( $byte[0] eq '09' && $byte[1] eq '18' ) {
	$type = 'NetMD Response';
	$direction = 'receive';
    }
    else {
	return 0;
    }
    $groupcode = $byte[2];
    if( $#byte == 5 ) {
	$selector = "$byte[0]$byte[1]$byte[2]-$byte[3]$byte[4]$byte[5]";
    }
    elsif( $#byte >= 6 ) {
	$selector = "$byte[0]$byte[1]$byte[2]-$byte[3]$byte[4]$byte[5]$byte[6]";
    }
    else {
	print "*** Short NetMD block! $data\n";
	return 0;
    }

    my $decoder = $Decoders{$selector};
    if( $decoder ) {
	return $decoder->( @byte );
    }
    else {
	print "$type: $selector\n";
	return 0;
    }
}

#############################################################################

sub CompareConst {
    my( $expected, @byte ) = @_;

    my $actual = shift( @byte );
    while( (length $actual < length $expected )) {
	last unless( @byte );
	$actual = $actual . ' ' . shift( @byte );
    }
    if( $expected ne $actual ) {
	print "Const mismatch: Expected '$expected' Got '$actual'\n";
	return 0;
    }
    return 1;
}

sub ShiftBy {
    my( $amt, @data ) = @_;

    for( my $i = 0; $i < $amt; $i++ ) {
	shift( @data );
    }
    return @data;
}

#############################################################################

sub DecodePlayerStatusRequest {
    my( @data ) = @_;

    print "-> ?? Player Status?";
    my $rc = CompareConst( '00 18 09 80 01 02 30 88 00 00 30 88 04 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}

sub DecodePlayerStatusReply {
    my( @data ) = @_;

    print "<- ?? Player Status: ";
    my $rc = CompareConst( '09 18 09 80 01 02 30 88 00 00 30 88 04 00 10 00 00 09 00 00 00 07', @data );
    if( $rc ) {
	@data = ShiftBy( 22, @data );
	print join( ' ', @data );
    }
    print "\n";
    return $rc;
}

#############################################################################

sub DecodeDiscCapacityRequest {
    my( @data ) = @_;

    print "-> Disc Capacity?";
    my $rc = CompareConst( '00 18 06 02 10 10 00 30 80 03 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}

sub DecodeDiscCapacityReply {
    my( @data ) = @_;

    my @time;

    print "<- Disc Capacity: ";
    my $rc = CompareConst( '09 18 06 02 10 10 00 30 80 03 00 10 00 00 1d 00 00 00 1b 80 03 00 17 80 00', @data );
    if( $rc ) {
	@data = ShiftBy( 25, @data );
	# Decode all 3 time parts
	for( my $i = 0; $i < 3; $i++ ) {
	    my $b = shift( @data );
	    if( $b ne '00' ) {
		print "Time decode error\n";
		return 0;
	    }
	    $b = shift( @data );
	    if( $b ne '05' ) {
		print "Expecting 5 byte timecodes (got $b)?\n";
		return 0;
	    }
	    shift( @data );	# Ignore leading 00
	    my $t = shift( @data) . ':' . shift( @data) . ':' . 
		shift( @data) . '.' . shift( @data);
	    $time[$i] = $t;
	}
	print "Max: $time[1], Used: $time[0], Avail: $time[2]";
    }
    print "\n";
    return $rc;
}

#############################################################################

sub DecodeDiscStatusRequest {
    my( @data ) = @_;

    print "-> ?? Disc Status? ";
    my $rc;
    if ( $data[8] eq '01' ) {
	print "(stopped) ";
	$rc = CompareConst( '00 18 09 80 01 03 30 88 01 00 30 88 05 00 30 88 07 00 ff 00 00 00 00 00', @data );
    }
    elsif( $data[8] eq '02' ) {
	print "(playing) ";
	$rc = CompareConst( '00 18 09 80 01 03 30 88 02 00 30 88 05 00 30 88 06 00 ff 00 00 00 00 00', @data );
    }
    else {
	print "(unknown state) ";
	$rc = 0;
    }
    print "\n";
    return $rc;
}

sub DecodeDiscStatusReply {
    my( @data ) = @_;

    print "<- ?? Disc Status: ";
    my $status = "";
    if( $data[8] eq '02' && $data[30] eq 'c3' ) {
	if( $data[31] eq '75' ) {
	    $status = '(Normal Playback)';
	}
	elsif( $data[31] eq '7d' ) {
	    $status = '(Paused)';
	}
	elsif( $data[31] eq '3f' ) {
	    $status = '(FF)';
	}
	elsif( $data[31] eq '4f' ) {
	    $status = '(Rewind)';
	}
    }
    @data = ShiftBy( 26, @data);
    print join( ' ', @data );
    print " $status\n";
    return 1;
}

#############################################################################

sub DecodePlaybackStatusRequest {
    my( @data ) = @_;

    print "-> ?? Playback Status? ";
    my $rc = CompareConst( '00 18 09 80 01 04 30 88 02 00 30 88 05 00 30 00 03 00 30 00 02 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}

sub DecodePlaybackStatusReply {
    my( @data ) = @_;

    print "<- ?? Playback Status: ";
    my $rc = CompareConst( '09 18 09 80 01 04 30 88 02 00 30 88 05 00 30 00 03 00 30 00 02 00 10 00 00 0d 00 00 00 0b 00 02 00 07 00 00', @data );
    if( $rc ) {
	@data = ShiftBy( 36, @data );
	my $track = shift( @data );
	my $time = join( ':', @data );
	print "Track $track, Time $time";
    }
    print "\n";
    return $rc;
}

#############################################################################

sub DecodeDiscFlagsRequest {
    my( @data ) = @_;

    print "-> Disc Flags? ";
    my $rc = CompareConst( '00 18 06 01 10 10 00 ff 00 00 01 00 0b', @data );
    print "\n";
    return $rc;

}


sub DecodeDiscFlagsReply {
    my( @data ) = @_;

    print "<- Disc Flags: ";
    my $rc = CompareConst( '09 18 06 01 10 10 00 10 00 00 01 00 0b', @data );
    if( $rc ) {
	@data = ShiftBy( 13, @data );
	my $flags = shift( @data );
	print "$flags ";
	if( $flags eq 50 ) {
	    print "(Write Protected)";
	}
	elsif( $flags eq 10 ) {
	    print "('Normal'?)";
	}
    }
    print "\n";
    return $rc;
}


#############################################################################

sub DecodeDeleteTrack {
    my( @data ) = @_;

    my $str;
    my $dir = shift( @data );
    if( $dir eq '00' ) {
	$str = '->';
    }
    elsif( $dir eq '09' ) {
	$str = '<-';
    }
    else {
	print "*** Invalid control control msg\n";
	return 0;
    }
    @data = ShiftBy( 9, @data );
    print "$str Delete Track: $data[0]\n";
    return 1;
}

#############################################################################

sub DecodeInitialiseDisc {
    my( @data ) = @_;

    my $dir = shift( @data );
    if( $dir eq '00' ) {
	print "-> Initialise Disc\n";
    }
    elsif( $dir eq '09' ) {
	print "<- Initialise Disc\n";
    }
    return 1;
}

#############################################################################

sub DecodeSimple {
    my( $tag, @data ) = @_;

    my $str;
    my $dir = shift( @data );
    if( $dir eq '00' ) {
	$str = '->';
    }
    elsif( $dir eq '09' ) {
	$str = '<-';
    }
    else {
	print "*** Invalid control control msg\n";
	return 0;
    }
    @data = ShiftBy( 6, @data );
    print "$str $tag: ", join( ' ', @data ), "\n";
    return 1;
}


#############################################################################

sub DecodeStartPlayback {
    my( @data ) = @_;

    return DecodeSimple( "Start playback", @data );
}

sub DecodeStopPlayback {
    my( @data ) = @_;

    return DecodeSimple( "Stop playback", @data );
}

sub DecodePause {
    my( @data ) = @_;

    return DecodeSimple( "Pause", @data );
}

sub DecodeFastForward {
    my( @data ) = @_;

    return DecodeSimple( "Start Fast Forward Mode", @data );
}

sub DecodeRewind {
    my( @data ) = @_;

    return DecodeSimple( "Start Rewind Mode", @data );
}

#############################################################################

sub DecodeSetPosition {
    my( @data ) = @_;

    if( $data[0] eq '00' ) {
	print '-> Set Position: ';
    }
    else {
	print '<- Set Position: ';
    }
    # Skip opcode and leading zeroes
    @data = ShiftBy( 10, @data );
    my $track = shift( @data );
    my $position = "$data[0]$data[1]:$data[2].$data[3]";
    $position =~ s/^0//g;
    print "Track $track - $position\n";

    return 1;
}

sub DecodeSetTrack {
    my( @data ) = @_;

    if( $data[0] eq '00' ) {
	print '-> Set Track: ';
    }
    else {
	print '<- Set Track: ';
    }
    # Skip opcode and leading zeroes
    @data = ShiftBy( 10, @data );
    my $track = shift( @data );
    print "$track\n";

    return 1;
}


#############################################################################

sub DecodeSetFlags {
    my( @data ) = @_;

    my $str;
    my $dir = shift( @data );
    if( $dir eq '00' ) {
	$str = '->';
    }
    elsif( $dir eq '09' ) {
	$str = '<-';
    }
    else {
	print "*** Invalid control msg\n";
	return 0;
    }
    shift( @data );
    shift( @data );
    print "$str ?? Set Flags ??: ", join( ' ', @data ), "\n";
    return 1;
}

#############################################################################

sub DecodeCheckoutStatusRequest {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Checkout Status Request: Track $track ";
    my $rc = CompareConst( 'ff 00 00 01 00 08', @data );
    print "\n";
    return $rc;
}

sub DecodeCheckoutStatusReply {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Checkout Status Reply: Track $track, ";
    my $rc = CompareConst( '10 00 00 01 00 08', @data );
    @data = ShiftBy( 6, @data );
    my $status = $data[0];
    print "Status $status ";
    if( $status ne '00' ) {
	print '(Locked?)';
    }
    print "\n";
    return $rc;
}

#############################################################################

sub DecodeTrackCountRequest {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    
    print "Track Count Request";
    my $rc = CompareConst( '30 00 10 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}

sub DecodeTrackCountReply {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    print "Track Count Reply: ";
    my $rc = CompareConst( '30 00 10 00 10 00 00 08 00 00 00 06 00 10 00 02 00', @data );
    @data = ShiftBy( 17, @data );
    print shift( @data ), "\n";
    return $rc;
}

#############################################################################

sub DecodeDiscTitleRequest {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    
    print "Disc Title request ";
    my $rc = CompareConst( '00 00 30 00 0a 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}

sub DecodeDiscTitleReply {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    
    print "Disc Title reply: ";
    my $rc = CompareConst( '00 00 30 00 0a 00 10 00 00', @data );
    @data = ShiftBy( 18, @data );
    my $title = '';
    while( @data ) {
	$title = $title . chr( hex( shift( @data )));
    }
    print "Title: '$title'\n";
    return $rc;
}


#############################################################################

sub DecodeDiscInfoRequest {
    my( @data ) = @_;

    if( $data[11] eq '0a' ) {
	return DecodeDiscTitleRequest( @data );
    }
    else {
	return 0;
    }
}

sub DecodeDiscInfoReply {
    my( @data ) = @_;

    if( $data[11] eq '0a' ) {
	return DecodeDiscTitleReply( @data );
    }
    else {
	return 0;
    }
}

#############################################################################

sub DecodeTrackBitrateRequest {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Bitrate request: Track $track ";
    my $rc = CompareConst( '30 80 07 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}


sub DecodeTrackBitrateReply {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Bitrate reply: Track $track ";
    my $rc = CompareConst( '30 80 07 00 10 00 00 0a 00 00 00 08 80 07 00 04 01 10', @data );
    if ( $rc ) {
	@data = ShiftBy( 18, @data );
	my $bitrate = $data[0];
	if ( $bitrate eq '90' ) {
	    print 'SP ';
	} elsif ( $bitrate eq '92' ) {
	    print 'LP2 ';
	} elsif ( $bitrate eq '93' ) {
	    print 'LP4 ';
	} else {
	    print "Unknown bitrate $bitrate ";
	}
	if ( $data[1] eq '00' ) {
	    print "Stereo";
	} else {
	    print "Mono";
	}
    }
    print "\n";
    return $rc;
}

sub DecodeTrackLengthRequest {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Length request: Track $track ";
    my $rc = CompareConst( '30 00 01 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}


sub DecodeTrackLengthReply {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Length reply: Track $track ";
    my $rc = CompareConst( '30 00 01 00 10 00 00 0c 00 00 00 0a 00 01 00 06 00 00', @data );
    if ( $rc ) {
	@data = ShiftBy( 18, @data );
	my $length = "$data[0]$data[1]:$data[2].$data[3]";
	$length =~ s/^0//g;
	print "Length $length";
    }
    print "\n";
    return $rc;
}


sub DecodeTrackInfoRequest {
    my( @data ) = @_;

    if( $data[10] eq '80' && $data[11] eq '07' ) {
	return DecodeTrackBitrateRequest( @data );
    }
    elsif( $data[10] eq '00' && $data[11] eq '01' ) {
	return DecodeTrackLengthRequest( @data );
    }
    else {
	return 0;
    }
}

sub DecodeTrackInfoReply {
    my( @data ) = @_;

    if( $data[10] eq '80' && $data[11] eq '07' ) {
	return DecodeTrackBitrateReply( @data );
    }
    elsif( $data[10] eq '00' && $data[11] eq '01' ) {
	return DecodeTrackLengthReply( @data );
    }
    else {
	return 0;
    }
}



#############################################################################

sub DecodeTrackTitleRequest {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Track Title request: Track $track ";
    my $rc = CompareConst( '30 00 0a 00 ff 00 00 00 00 00', @data );
    print "\n";
    return $rc;
}

sub DecodeTrackTitleReply {
    my( @data ) = @_;

    @data = ShiftBy( 7, @data ); # Shift past opcode
    my $track = shift( @data ) . shift( @data );
    
    print "Track Title reply: Track $track ";
    my $rc = CompareConst( '30 00 0a 00 10 00 00', @data );
    @data = ShiftBy( 16, @data );
    my $title = '';
    while( @data ) {
	$title = $title . chr( hex( shift( @data )));
    }
    print "Title: '$title'\n";
    return $rc;
}

#############################################################################
# ReadLine() - Read the next line from the file, skipping cruft and tidying
# up.  
sub ReadLine {
    # If some function peeked ahead, we don't need to actually read the line
    if( $PeekFlag ) {
	$PeekFlag = 0;
	return 1;
    }
    while( <> ) {
	# split out the line number and time fields
	if( /^(\d+)\s+([0-9.]+)\s+(.*)$/ ) {	# "old" usbsnoopy format
	    $LineNo = $1;
	    $LineTime = $2;
	    $Line = $3;
	    $Line =~ s/ : / = /g;		# Convert for easier parsing later
	}
	elsif( /^\[(\d+) ms\] (.*)$/ ) {	# 'new' usbsnoop format
	    $LineTime = $1;
	    $Line = $2;
	    if( $LineNo ) {			# Could line numbers ourself
		$LineNo++;
	    }
	    else {
		$LineNo = 1;			# Count line number ourself
		$FreeStyle = 1;			# Allow 'freestyle' lines
	    }
	}
	elsif( $FreeStyle ) {
	    s/^\s*//;
	    s/^[0-9a-f]+: //;	# Nasty hack to get rid of markers in hex data
	    $Line = $_;
	    if( $Line =~ /^SetupPacket/ ) {
		# Even nastier hack for the way setup packets are dumped
		my $t = <>;
		if( $t ) {
		    $t =~ s/^\s*//;
		    $t =~ s/\s+$//;
		    $t =~ s/^[0-9a-f]+: //;
		    $Line = "$Line $t";
		}
	    }
	    $LineNo++;
	}
	else {
	    die "Parse error!";
	}
	next unless $Line;			# SKIP blank lines
	$Line =~ s/^\s*//;			# Trim leading whitespace
	$Line =~ s/\s+$//;			# Trim trailing whitespace
	next if( $Line =~ /^UsbSnoop/ );	# Skip UsbSnoop internal stuff
	next if( $Line =~ /^fido=/ );
	next if( $Line =~ /^fdo=/ );
	next if( $Line =~ /^pdx=/ );
	next if( $Line =~ /^\d+:\s*$/ );	# Skip hexdump 'count' lines
	$Line =~ s/^[0-9a-f]+: //;	# Nasty hack to get rid of markers in hex data
	#print "$LineNo/$LineTime '$Line'\n";
	return 1;
    }
    $Line = undef;
    return undef;
}

#############################################################################
# PeekLine() - Peek at the next line, but leave it available for reading.
# makes the parsing mess a bit simpler.  NB This overwrite the global $Line
# etc variables.
sub PeekLine {
    my $result = 1;

    # If we already peeked we don't need to peek again
    if( $PeekFlag ) {
	# Handle the case where a previous peek found EOF
	$result = undef unless $Line;
    }
    else {
	$result = ReadLine();
	$PeekFlag = 1;
    }
    return $result;
}


sub HexDump {
    my ($prefix, $linemax, $str, $bool) = @_;

    my @bytes = split( / /, $str );

    my $offset = 0;
    my $size = $#bytes + 1;
    #print "** $str\n";
    while( $size > 0 ) {
	my $rowsize = ($size > $linemax ) ? $linemax : $size;
	printf "%s%04x: ", $prefix, $offset;
	my $i;
	for( $i = 0; $i < $rowsize; $i++ ) {
	    print "$bytes[$i + $offset] ";
	}
	for( ; $i < $linemax; $i++ ) {
	    print "   ";
	}
	print "  ";
	if($bool) {
	  for( $i = 0; $i < $rowsize; $i++ ) {
	    my $c = '.';
	    my $b = $bytes[$i + $offset];
	    if( $b ) {
		  $b = hex $b;	# Convert to decimal
		  if( $b >= 32 && $b < 127 ) {
		    $c = chr $b;
		  }
	    }
	    print $c;
	  }
	}
	print "\n";
	$size -= $rowsize;
	$offset += $rowsize;
    }
}

#############################################################################

sub Usage {
    print<<EOS;

logparse - Try to simplify USBSnoopy logs

 Usage:  logparse [opts] <logfile>\n

Options include:-
    -v		Verbose output
    -q		Be extra quiet
    -s		Show setup packets too
    -t		Show packet times too
EOS
    exit 1;
}
