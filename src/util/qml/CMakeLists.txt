if (WITH_QWT)
	find_package (PkgConfig)
	pkg_search_module (Qwt REQUIRED IMPORTED_TARGET Qt${LC_QT_VERSION}Qwt6 Qt${LC_QT_VERSION}Qwt)
endif ()

add_util_library (util-qml
	SRCS
		colorthemeproxy.cpp
		settableiconprovider.cpp
		standardnamfactory.cpp
		themeimageprovider.cpp
		tooltipitem.cpp
		unhidelistmodel.cpp
		unhidelistviewbase.cpp
		util.cpp
		widthiconprovider.cpp
		$<$<BOOL:${WITH_QWT}>:plotitem.cpp>
	DEPENDS
		leechcraft-util-gui
		leechcraft-util-network
		leechcraft-util-sll
		leechcraft-util-sys
		$<$<BOOL:${WITH_QWT}>:PkgConfig::Qwt>
	USES Network Quick QuickWidgets
	DEFINES $<$<BOOL:${WITH_QWT}>:WITH_QWT>
	)
