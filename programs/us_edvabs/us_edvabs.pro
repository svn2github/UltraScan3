include( ../../gui.pri )

TARGET        = us_edvabs
QT           += xml

HEADERS       = us_edvabs.h          \
                us_exclude_profile.h \
                us_ri_noise.h        \
                us_edit_scan.h

SOURCES       = us_edvabs.cpp          \
                us_exclude_profile.cpp \
                us_ri_noise.cpp        \
                us_edit_scan.cpp

