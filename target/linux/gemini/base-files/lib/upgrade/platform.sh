REQUIRE_IMAGE_METADATA=1

platform_check_image() {
	local board=$(board_name)

	case "$board" in
	dlink,dir-685)
		return 0
		;;
	raidsonic,ib-4220-b)
		local productline=`tar -xzOf $1 ImageInfo | grep productName`
		[ "$productline" == "" ] && return 1

		eval $productline
		[ "$productName" == "" ] && return 1

		return 0
		;;	
	esac

	echo "Sysupgrade is not yet supported on $board."
	return 1
}

platform_do_upgrade() {
	local board=$(board_name)

	case "$board" in
	dlink,dir-685)
		PART_NAME=firmware
		default_do_upgrade "$1"
		;;
	raidsonic,ib-4220-b)
		tar -C /tmp -xzf $1
		rm $1
		dd if=/tmp/rd.gz of=/tmp/rootfs bs=6M count=1 >/dev/null 2>&1
		rm /tmp/rd.gz
		dd if=/tmp/hddapp.tgz of=/tmp/rootfs bs=6M count=1 seek=1 >/dev/null 2>&1
		rm /tmp/hddapp.tgz

		local line

		line=`grep Distribution /tmp/ImageInfo`
		eval $line
		line=`grep Layout /tmp/ImageInfo`
		eval $line

		local mode=0
		if [ "$Layout" == "Compact" ]; then
			mode=1
		fi 

		local append=""
		[ -f "$CONF_TAR" -a "$SAVE_CONFIG" -eq 1 ] && append="-j $CONF_TAR"

		mtd -q write /tmp/zImage Kern
		mtd -q -r $append write /tmp/rootfs rootfs
		;;
	esac
}
