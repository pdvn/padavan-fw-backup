#!/bin/sh

self_name="Entware"

# check /opt mounted
if ! grep -q /opt /proc/mounts ; then
	echo "ERROR! Directory \"/opt\" not mounted!"
	exit 1
fi

export PATH=/opt/sbin:/opt/bin:/usr/sbin:/usr/bin:/sbin:/bin


dl () {
	# $1 - URL to download
	# $2 - place to store
	# $3 - 'x' if should be executable
	logger -t "${self_name}" "Downloading $2..."
	wget -q $1 -O $2
	if [ $? -eq 0 ] ; then
		logger -t "${self_name}" "SUCCESS!"
	else
		logger -t "${self_name}" "FAILED!"
		exit 1
	fi
	[ -z "$3" ] || chmod +x $2
}

install_link () {
	if [ -f /etc/$1 ] ; then
		ln -sf /etc/$1 /opt/etc/$1
	else
		cp /opt/etc/$1.1 /opt/etc/$1
	fi
}

# check opkg installed
if [ ! -f /opt/bin/opkg ] ; then
	logger -t "${self_name}" "Installing entware opkg...."

	logger -t "${self_name}" "Creating folders..."
	for folder in bin etc lib/opkg tmp var/lock ; do
		if [ -d "/opt/$folder" ] ; then
			logger -t "${self_name}" "Warning: Folder /opt/$folder exists! If something goes wrong please clean /opt folder and try again."
		else
			mkdir -p /opt/$folder
		fi
	done

	URL=http://bin.entware.net/mipselsf-k3.4/installer
	dl $URL/opkg /opt/bin/opkg x
	dl $URL/opkg.conf /opt/etc/opkg.conf
	dl $URL/ld-2.27.so /opt/lib/ld-2.27.so x
	dl $URL/libc-2.27.so /opt/lib/libc-2.27.so
	dl $URL/libgcc_s.so.1 /opt/lib/libgcc_s.so.1
	dl $URL/libpthread-2.27.so /opt/lib/libpthread-2.27.so
	cd /opt/lib
	ln -s ld-2.27.so ld.so.1
	ln -s libc-2.27.so libc.so.6
	ln -s libpthread-2.27.so libpthread.so.0

	logger -t "${self_name}" "Updating opkg packages list..."
	opkg update
	if [ $? -eq 0 ] ; then
		logger -t "${self_name}" "SUCCESS!"
	else
		logger -t "${self_name}" "FAILED!"
		exit 1
	fi
	logger -t "${self_name}" "Basic packages installation..."
	opkg install entware-opt
	if [ $? -eq 0 ] ; then
		logger -t "${self_name}" "SUCCESS!"
	else
		logger -t "${self_name}" "FAILED!"
		exit 1
	fi

	chmod 777 /opt/tmp

	install_link passwd
	install_link group
	install_link shells

	if [ -f /etc/shadow ] ; then
		ln -sf /etc/shadow /opt/etc/shadow
	fi
	if [ -f /etc/gshadow ] ; then
		ln -sf /etc/gshadow /opt/etc/gshadow
	fi
	if [ -f /etc/localtime ] ; then
		ln -sf /etc/localtime /opt/etc/localtime
	fi

	logger -t "${self_name}" "Congratulations!"
	logger -t "${self_name}" "If there are no errors above then Entware successfully initialized."
	logger -t "${self_name}" "Found a Bug? Please report at https://github.com/Entware/Entware/issues"
fi
