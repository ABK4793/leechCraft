cmake_minimum_required (VERSION 3.10)
project (lmp_httstream)
include (InitLCPlugin NO_POLICY_SCOPE)

LC_DEFINE_PLUGIN (
	SRCS
		httstream.cpp
		httpserver.cpp
		httpstreamfilter.cpp
		filterconfigurator.cpp
	SETTINGS lmphttstreamfiltersettings.xml
	QT_COMPONENTS Gui Network
	LINK_LIBRARIES PkgConfig::GStreamer leechcraft-lmp-util
	)
