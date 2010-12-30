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
#include "us_matrix.h"
#include "us_editor.h"

struct Average
{
   double top, bottom;
   int cell, rpm, channel, top_count, bottom_count;
};

struct Limit
{
   // this structure contains 2 limits:
   //    [0] = top of channel
   //    [1] = bottom of channel
   QwtDoubleRect  rect[2];
   bool           used[2];
   int            cell;
   QString        channel;
};

class US_EXTERN US_RotorCalibration : public US_Widgets
{
	Q_OBJECT

	public:
		US_RotorCalibration();
		~US_RotorCalibration();

	private:

      US_DataIO2::RawData            data;
      QVector< US_DataIO2::RawData > allData;
      QVector <Average> avg;
      QVector <QVector <double> > reading;
      QVector <double> stretch_factors, std_dev;
      QVector <Limit> limit;

      double             left, right, top, bottom, coef[3];
      double             *x, *y, *sd1, *sd2;
      int                maxcell, current_triple, current_cell;
      bool               top_of_cell, newlimit;
      QString            rotor, fileText, current_channel;
      
      US_Help            showHelp;

      QIcon              check;

      QPushButton*       pb_reset;
      QPushButton*       pb_accept;
      QPushButton*       pb_calculate;
      QPushButton*       pb_save;
      QPushButton*       pb_load;
      QPushButton*       pb_loadRotor;
      QPushButton*       pb_view;
            
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
      QwtCounter*        ct_cell;
      QwtCounter*        ct_channel;
      

      US_PlotPicker*     pick;
      US_Plot*           plot;
      
//      QLabel*            lbl_instructions;
//      QLabel*            lbl_spacer;

      QLineEdit*         le_instructions;
      QLineEdit*         le_rotorInfo;
                        
      QRadioButton*      rb_channel;
      QRadioButton*      rb_top;
      QRadioButton*      rb_bottom;

      QCheckBox*         cb_assigned;
      
   public slots:
      void help (void)
      {
         showHelp.show_help( "manual/rotor_calibration.html" );
      };
      void reset(void);
      void load(void);
      void plotAll(void);
      void currentRect(QwtDoubleRect);
      void findTriple(void);
      void next(void);
      void calculate(void);
      double findAverage(QwtDoubleRect, US_DataIO2::RawData, int);
      void save(void);
      void view(void);
      void loadRotor(void);
      void update_used();
      void update_cell(double);
      void update_channel(double);
      void update_position();
      void update_plot();
};
#endif
