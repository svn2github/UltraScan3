#ifndef US_EDIT_H
#define US_EDIT_H

#include <uuid/uuid.h>

#include <QtGui>
#include <QApplication>
#include <QDomDocument>

#include <qwt_plot_marker.h>

#include "us_extern.h"
#include "us_widgets.h"
#include "us_help.h"
#include "us_plot.h"
#include "us_dataIO2.h"

class US_EXTERN US_RotorCalibration : public US_Widgets
{
	Q_OBJECT

	public:
		US_RotorCalibration();

	private:

      US_DataIO2::RawData            data;
      QVector< US_DataIO2::RawData > allData;

      double            left, right, top, bottom;
      int               step;
      
      US_Help            showHelp;

      QIcon              check;

      QPushButton*      pb_reset;
      QPushButton*      pb_leftCells;
      QPushButton*      pb_leftCounterbalance;
      QPushButton*      pb_rightCells;
      QPushButton*      pb_rightCounterbalance;
      
      QString            workingDir;
      QString            runID;
      QString            editID;
      QString            dataType;
      QStringList        files;
      QStringList        triples;
                      
      QwtPlot*           data_plot;
      QwtPlotCurve*      fit_curve;
      QwtPlotCurve*      v_line;
      QwtPlotCurve*      minimum_curve;
      QwtPlotGrid*       grid;
      QwtPlotMarker*     marker;
      QwtDoubleRect      limits[4];

      US_PlotPicker*     pick;
      US_Plot*           plot;
      
      QLabel*            lbl_instructions;
      QLabel*            lbl_spacer;

      QLineEdit*         le_instructions;
                        
      QRadioButton*      rb_counterbalance;
      QRadioButton*      rb_cells;
      QRadioButton*      rb_all;

   public slots:
      void help (void)
      {
         showHelp.show_help( "manual/rotor_calibration.html" );
      };
      void reset (void);
      void load (void);
      void plot_all (void);
      void currentRect (QwtDoubleRect);
      void leftCounterbalance (void);
      void rightCounterbalance (void);
      void leftCells (void);
      void rightCells (void);
      void showAll (void);
      void showCells (void);
      void showCounterbalance (void);
      void next(void);
      void accept(void);
      void calculate(void);
};
#endif
