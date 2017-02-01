#!/bin/sh -e

 case $ACTION in
         add)
                 ACT=AddMDDevice
                 ;;
         remove)
                 ACT=RemoveMDDevice
                 ;;
         *)
                 ;;
 esac

# run as current user, else dbus message cannot be recieved
username=`who | head -n 1 | cut -d " " -f 1`
export HOME=/home/$username
export XAUTHORITY=$HOME/.Xauthority
export DISPLAY=:0.0

# convert vendor and product id to int
vid=$(echo $((0x$ID_VENDOR_ID)))
pid=$(echo $((0x$ID_MODEL_ID)))

# send message to session bus, do not wait for a reply !!!
su $username -c "qdbus --session org.linux-minidisc.qhimdtransfer /QHiMDUnixDetection $ACT $DEVNAME $vid $pid &" -m
