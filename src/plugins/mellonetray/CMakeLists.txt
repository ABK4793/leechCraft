cmake_minimum_required (VERSION 3.10)
project (mellonetray)
include (InitLCPlugin NO_POLICY_SCOPE)

find_package (PkgConfig)
pkg_check_modules (MellonetrayX11Parts REQUIRED IMPORTED_TARGET x11 xdamage xrender)

LC_DEFINE_PLUGIN (
	SRCS
		mellonetray.cpp
		traymodel.cpp
		iconhandler.cpp
	QT_COMPONENTS Quick Widgets
	LINK_LIBRARIES PkgConfig::MellonetrayX11Parts
	INSTALL_SHARE
	)
