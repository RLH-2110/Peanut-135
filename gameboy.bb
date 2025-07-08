# Recipe created by recipetool
# This is the basis of a recipe and may need further editing in order to be fully functional.
# (Feel free to remove these comments when editing.)

# WARNING: the following LICENSE and LIC_FILES_CHKSUM values are best guesses - it is
# your responsibility to verify that the values are complete and correct.
#
# The following license files were not able to be identified and are
# represented as "Unknown" below, you will need to check them yourself:
#   LICENSE

FILESEXTRAPATHS:prepend := "${THISDIR}:"
SRC_URI = "file://gameboy/"
S = "${WORKDIR}/gameboy"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=aa4afe57b8479d0d348a3559717ed15c"

inherit pkgconfig 
DEPENDS += "libdrm util-linux"

do_configure() {
	# Specify any needed configure commands here
	:
}



do_compile() {

	# Specify compilation commands here
	oe_runmake all 

}

FILES:${PN} += "/home/root/calc.gb"

do_install() {
	# Specify install commands here
	install -d ${D}${bindir}
	install -m 0755 peanut135 ${D}${bindir}

	install -d ${D}/home/root
	install -m 0644 calc.gb ${D}/home/root

	install -d ${D}${sysconfdir}/peanut135
	install -m 0644 default.ini ${D}${sysconfdir}/peanut135/config.ini
}
 
