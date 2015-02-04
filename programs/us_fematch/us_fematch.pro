include( ../../gui.pri )

QT           += xml svg opengl

TARGET        = us_fematch

HEADERS       = us_fematch.h          \
                us_plot_control_fem.h \
                us_resplot_fem.h      \
                us_advanced_fem.h     \
                us_adv_dmgamc.h       \
                us_dmga_mc_stats.h    \
                us_thread_worker.h

SOURCES       = us_fematch.cpp          \
                us_plot_control_fem.cpp \
                us_resplot_fem.cpp      \
                us_advanced_fem.cpp     \
                us_adv_dmgamc.cpp       \
                us_dmga_mc_stats.cpp    \
                us_thread_worker.cpp

