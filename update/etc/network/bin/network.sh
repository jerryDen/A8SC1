#!/bin/sh

PUBLIC=/etc/network
export PATH=/bin:/sbin:/usr/bin:/usr/sbin:$PUBLIC/bin
export LD_LIBRARY_PATH=/lib:/usr/lib:$PUBLIC/lib
chmod -R a+x $PUBLIC/bin

## /*enable ethernet device.*/
ifconfig lo up
ifconfig lo 127.0.0.1

NET_CONF=$PUBLIC/net.cfg
UDHCP_CONF=$PUBLIC/udhcpc.conf
#
# "key=value" in net.cfg are as follows:
# IPAddressMode=DHCP
# IPAddress=192.168.1.234
# DNSIPAddress1=192.168.1.1
# DNSIPAddress2=192.168.1.1
# SubnetMask=255.255.255.0
# ConfigNet=20
# Port=5000
# Gateway=192.168.1.1
# DDNSServer=192.168.1.1
# MacAddress=00:11:22:33:44:55
# 

if [  -f $NET_CONF ] ## net.cfg is existed ?
then                 ## existed 
    . $NET_CONF ##open file to get "key=value".
	
    if [ "$IPAddressMode" == "DHCP" ]
    then
	ifconfig eth0 down
        /sbin/ifconfig eth0 hw ether $MacAddress
        ifconfig eth0 up
       	while true
       	do
       	    #ret=` ifconfig eth0 | grep 'inet ' | sed s/^.*addr://g | sed s/Bcast.*$//g `
       	    ret=` ping 114.114.114.114  -c 3 -W 2| grep "ttl="  `
       	    if [ -z $ret   ]; then
       	    	
        	ifconfig eth0 down
        	ifconfig eth0 up
       	    	killall -9 udhcpc
            	#/sbin/udhcpc -b -i eth0 -n -s $UDHCP_CONF &
            	/sbin/udhcpc -b -i eth0 &
            	
        	echo "...start......DHCP..."
            fi
            sleep 30
       	done
    else
        if [ -n $IPAddress ] ##length > 0.
        then
          #  /sbin/ifconfig eth0 hw ether $MacAddress
            /sbin/ifconfig eth0 $IPAddress netmask $SubnetMask up
            route add default gw "$Gateway"
            echo $DNSIPAddress1 > /etc/resolv.conf
            echo "...wire_connect......FixedIP..."
        else
            ifconfig eth0 up
            /sbin/udhcpc -b -i eth0 -n -s $UDHCP_CONF &
            echo "...wire_connect......DHCP..."
        fi
    fi
else ## net.cfg is not existed.
    echo ".......$CONF_NET is not existed ......"
    /sbin/ifconfig eth0 192.168.1.254 netmask 255.255.255.0 up
    /sbin/udhcpc -b -i eth0 -n -s $UDHCP_CONF &
fi

