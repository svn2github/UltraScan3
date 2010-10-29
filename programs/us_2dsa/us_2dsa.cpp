//! \file us_2dsa.cpp

#include <QApplication>

#include "us_2dsa.h"
#include "us_license_t.h"
#include "us_license.h"
#include "us_settings.h"
#include "us_gui_settings.h"

//! \brief Main program for us_2dsa. Loads translators and starts
//         the class US_2dsa.

int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );

   #include "main1.inc"

   // License is OK.  Start up.
   
   US_2dsa w;
   w.show();                   //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

US_2dsa::US_2dsa() : US_AnalysisBase2()
{
   setWindowTitle( tr( "2-Dimensional Spectrum Analysis" ) );

   te_results = NULL;

   QLabel* lb_analysis = us_banner( tr( "Analysis Controls" ) );
   QLabel* lb_scan     = us_banner( tr( "Scan Control"       ) );

   QLabel* lb_status   = us_label ( tr( "Status\nInfo:" ) );
   te_status           = us_textedit();
   QLabel* lb_from     = us_label ( tr( "From:" ) );
   QLabel* lb_to       = us_label ( tr( "to:"   ) );

   pb_exclude = us_pushbutton( tr( "Exclude Scan Range" ) );
   pb_exclude->setEnabled( false );
   connect( pb_exclude, SIGNAL( clicked() ), SLOT( exclude() ) );

   ct_from            = us_counter( 2, 0, 0 );
   ct_to              = us_counter( 2, 0, 0 );

   connect( ct_from, SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_from( double ) ) );
   connect( ct_to,   SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_to  ( double ) ) );
   QPushButton* pb_fitcntl = us_pushbutton( tr( "Fit Control"   ) );
   QPushButton* pb_loadfit = us_pushbutton( tr( "Load Fit"      ) );
   QPushButton* pb_plt3d   = us_pushbutton( tr( "3-D Plot"      ) );
   QPushButton* pb_pltres  = us_pushbutton( tr( "Residual Plot" ) );

   controlsLayout->addWidget( lb_analysis,  0, 0, 1, 4 );
   controlsLayout->addWidget( lb_status,    1, 0, 4, 1 );
   controlsLayout->addWidget( te_status,    1, 1, 4, 3 );
   controlsLayout->addWidget( pb_fitcntl,   5, 0, 1, 2 );
   controlsLayout->addWidget( pb_loadfit,   5, 2, 1, 2 );
   controlsLayout->addWidget( pb_plt3d,     6, 0, 1, 2 );
   controlsLayout->addWidget( pb_pltres,    6, 2, 1, 2 );
   controlsLayout->addWidget( lb_scan,      7, 0, 1, 4 );
   controlsLayout->addWidget( lb_from,      8, 0, 1, 1 );
   controlsLayout->addWidget( ct_from,      8, 1, 1, 1 );
   controlsLayout->addWidget( lb_to,        8, 2, 1, 1 );
   controlsLayout->addWidget( ct_to,        8, 3, 1, 1 );
   controlsLayout->addWidget( pb_exclude,   9, 0, 1, 4 );

   te_status->setAlignment( Qt::AlignCenter | Qt::AlignVCenter );
   te_status->setText( tr(
       "Solution not initiated...\n"
       "RMSD:  0.000000,\n"
       "Variance: 0.000000e-05 .\n"
       "Iterations:  0" ) );

   progressLayout   = new QGridLayout();
   QLabel* lb_progress = us_label( tr( "Status:" ) );
   b_progress          = us_progressBar( 0, 100, 0 );
   progressLayout->addWidget( lb_progress,  0, 0, 1, 1 );
   progressLayout->addWidget( b_progress,   0, 1, 1, 3 );
   leftLayout->addLayout( progressLayout );

   connect( pb_help,  SIGNAL( clicked() ), SLOT( help() ) );
   connect( pb_view,  SIGNAL( clicked() ), SLOT( view() ) );
   connect( pb_save,  SIGNAL( clicked() ), SLOT( save() ) );
}

void US_2dsa::data_plot( void )
{
   US_AnalysisBase2::data_plot();

#if 0
   //time_correction = US_Math::time_correction( dataList );

   int                     index  = lw_triples->currentRow();
   US_DataIO2::EditedData* d      = &dataList[ index ];

   int     scanCount   = d->scanData.size();
   int     exclude     = 0;
   double  boundaryPct = ct_boundaryPercent->value() / 100.0;
   double  positionPct = ct_boundaryPos    ->value() / 100.0;
   double  baseline    = calc_baseline();

   for ( int i = 0; i < scanCount; i++ )
   {
      if ( excludedScans.contains( i ) ) continue;
      
      double range  = d->scanData[ i ].plateau - baseline;
      double test_y = baseline + range * positionPct;
      
      if ( d->scanData[ i ].readings[ 0 ].value > test_y ) exclude++;
   }

   le_skipped->setText( QString::number( exclude ) );

   // Draw plot
   data_plot1->clear();
   us_grid( data_plot1 );

   QString triptitl = lw_triples->item( index )->text();
   data_plot1->setTitle( triptitl + tr( " - 2-D Spectrum Plot" ) );

   data_plot1->setAxisTitle( QwtPlot::xBottom, tr( "Scan Number" ) );
   data_plot1->setAxisTitle( QwtPlot::yLeft  , 
         tr( "Correc. Sed. Coeff. (1e-13 s)" ) );

   if ( smPoints  != NULL ) delete [] smPoints;
   if ( smSeconds != NULL ) delete [] smSeconds;

   smPoints  = new double[ scanCount ];
   smSeconds = new double[ scanCount ];

   // Calculate the 2nd moment
   for ( int i = 0; i < scanCount; i++ )
   {
      double sum1  = 0.0;
      double sum2  = 0.0;
      int    count = 0;

      // The span is the boundary portion that is going to be analyzed (in
      // percent)

      double range  = ( d->scanData[ i ].plateau - baseline ) * boundaryPct;
      double test_y = range * positionPct;

      while ( d->scanData[ i ].readings[ count ].value - baseline < test_y ) 
         count++;

      int points = d->scanData[ i ].readings.size();

      if ( count == 0 ) count = 1;

      while ( count < points )
      {   
         double value  = d->scanData[ i ].readings[ count ].value - baseline;
         double radius = d->x[ count ].radius;

         if ( value >= test_y + range ) break;
      
         double v0 = d->scanData[ i ].readings[ count - 1 ].value - baseline;
         double dC = value - v0;

         sum1 += dC * sq( radius );
         sum2 += dC;
         count++;
      }

      smPoints [ i ] = sqrt( sum1 / sum2 ); // second moment points in cm

      double omega = d->scanData[ i ].rpm * M_PI / 30.0;

      // second moment s
      smSeconds[ i ] = 
         1.0e13 * solution.correction * log( smPoints[ i ] / d->meniscus ) /
         ( sq( omega ) * ( d->scanData[ i ].seconds - time_correction ) );
   }

   double* x = new double[ scanCount ];
   double* y = new double[ scanCount ];
   
   // Sedimentation coefficients from all scans that have not cleared the
   // meniscus form a separate plot that will be plotted in red, and will not
   // be included in the line fit:
    
   QwtPlotCurve* curve;
   QwtSymbol     sym;
   
   int count = 0;

   // Curve 1
   for ( int i = 0; i < exclude; i++ )
   {
      if ( excludedScans.contains( i ) ) continue;
      
      x[ count ] = (double)( count + 1 );
      y[ count ] = smSeconds[ i ];
      count++;
   }

   curve = us_curve( data_plot1, tr( "Non-cleared Sedimentation Coefficients" ) );

   sym.setStyle( QwtSymbol::Ellipse );
   sym.setPen  ( QPen( Qt::white ) );
   sym.setBrush( QBrush( Qt::red ) );
   sym.setSize ( 8 );
   
   curve->setStyle ( QwtPlotCurve::NoCurve );
   curve->setSymbol( sym );
   curve->setData  ( x, y, count );

   // Curve 2
   count          = 0;
   double average = 0.0;

   for ( int i = exclude; i < scanCount; i++ )
   {
      if ( excludedScans.contains( i ) ) continue;
      
      x[ count ] = (double)( count + 1 + exclude );
      y[ count ] = smSeconds[ i ];
      average   += smSeconds[ i ];
      count++;
   }

   average_2nd = (count > 0 ) ? average / count : 0.0;

   sym.setPen  ( QPen  ( Qt::blue  ) );
   sym.setBrush( QBrush( Qt::white ) );
   
   curve = us_curve( data_plot1, tr( "Cleared Sedimentation Coefficients" ) );
   curve->setSymbol( sym );
   curve->setData( x, y, count );
   
   // Curve 3

   x[ 0 ] = 0.0;
   x[ 1 ] = (double)( scanCount - excludedScans.size() );
   y[ 0 ] = average_2nd;
   y[ 1 ] = average_2nd;

   if ( count > 0 )
   {
      curve = us_curve( data_plot1, tr( "Average" ) );
      curve->setPen( QPen( Qt::green ) );
      curve->setData( x, y, 2 );
   }

   data_plot1->setAxisScale   ( QwtPlot::xBottom, 0.0, x[ 1 ] + 0.25, 1.0 );
   data_plot1->setAxisMaxMinor( QwtPlot::xBottom, 0 );
   data_plot1->replot();

   delete [] x;
   delete [] y;
#endif
}

void US_2dsa::view( void )
{
   // Create US_Editor
   if ( te_results == NULL )
   {
      te_results = new US_Editor( US_Editor::DEFAULT, true, QString(), this );
      te_results->resize( 500, 400 );
      QPoint p = g.global_position();
      te_results->move( p.x() + 30, p.y() + 30 );
      te_results->e->setFont( QFont( US_GuiSettings::fontFamily(),
                                     US_GuiSettings::fontSize() ) );
   }

   int                     index  = lw_triples->currentRow();
   US_DataIO2::EditedData* d      = &dataList[ index ];

   QString sm_results = 
        table_row( tr( "Average Second Moment S: " ),
                   QString::number( index,       'f', 5 ) + " s * 10e-13" );

   // Add results to window
   QString s = 
      "<html><head>\n"
      "<style>td { padding-right: 1em;}</style>\n"
      "</head><body>\n" +
      tr( "<h1>Second Moment Analysis</h1>\n" )   +
      tr( "<h2>Data Report for Run \"" ) + d->runID + tr( "\", Cell " ) + d->cell +
      tr(  ", Wavelength " ) + d->wavelength + "</h2>\n";
   
   s += run_details();
   s += hydrodynamics();
   s += analysis( sm_results );
   s += scan_info();
   s += "</body></html>\n";

   te_results->e->setHtml( s );
   te_results->show();
}

void US_2dsa::save( void )
{
   int                     index  = lw_triples->currentRow();
   US_DataIO2::EditedData* d      = &dataList[ index ];
   QString                 dir    = US_Settings::reportDir();

   if ( ! mkdir( dir, d->runID ) ) return;

   // Note: d->runID is both directory and first segment of file name
   QString filebase = dir + "/" + d->runID + "/" + d->runID + "." + d->cell + 
       + "." + d->channel + "." + d->wavelength;
   
   QString plot1File = filebase + ".sm_plot1.svg";
   QString plot2File = filebase + ".sm_plot2.svg";
   QString textFile  = filebase + ".sm_data.txt";
   QString htmlFile  = filebase + ".sm_report.html";


   // Write plots
   write_plot( plot1File, data_plot1 );
   write_plot( plot2File, data_plot2 );
   
   // Write moment data
   QFile sm_data( textFile );
   if ( ! sm_data.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
   {
      QMessageBox::warning( this,
            tr( "IO Error" ),
            tr( "Could not open\n" ) + textFile + "\n" +
                tr( "\nfor writing" ) );
      return;
   }

   QTextStream ts_data( &sm_data );

#if 0
   int scanCount = d->scanData.size();
   int excludes  = le_skipped->text().toInt();
   
   if ( excludes == scanCount )
      ts_data << "No valid scans\n";
   else
   {
      int count = 0;
      for ( int i = excludes; i < scanCount; i++ )
      {
         if ( excludedScans.contains( i ) ) continue;

         ts_data << count + 1 << "\t" << smPoints[ i ] 
                 << "\t" << smSeconds[ i ] << "\n";
         count++;
      }
   }

   sm_data.close();

   // Write report
   QFile report( htmlFile );

   if ( ! report.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
   {
      QMessageBox::warning( this,
            tr( "IO Error" ),
            tr( "Could not open\n" ) + htmlFile + "\n" +
                tr( "\nfor writing" ) );
      return;
   }

   QTextStream report_ts( &report );

   report_ts << "<html><head>\n"
         "<style>td { padding-right: 1em;}</style>\n"
         "</head><body>\n";

   report_ts << tr( "<h1>Second Moment Analysis</h1>\n" ) 
             << tr( "<h2>Data Report for Run \"" ) + d->runID 
                    + tr( "\", Cell " ) + d->cell 
                    + tr(  ", Wavelength " ) + d->wavelength + "</h2>\n";

   QString sm_results = 
        table_row( tr( "Average Second Moment S: " ),
                   QString::number( average_2nd, 'f', 5 ) + " s * 10e-13" );

   report_ts << run_details()
             << hydrodynamics()
             << analysis( sm_results )
             << scan_info();

   // Remove directory from string
   filebase = d->runID + "." + d->cell + "." + d->channel + "." + d->wavelength;

   report_ts << "<h3><a href='" + filebase + ".sm_data.txt'>" 
                 "Text File of Second Moment Plot Data</a></h3>\n"

         "<div><h3>Second Moment Plot</h3>\n"
         "<object data='" + filebase + ".sm_plot1.svg' type='image/svg+xml' "
         "width='"  + QString::number( data_plot1->size().width()  * 1.4 ) + "' "
         "height='" + QString::number( data_plot1->size().height() * 1.4 ) + 
         "'></object></div>\n"
         
         "<div><h3>Velocity Plot</h3>\n"
         "<object data='" + filebase + ".sm_plot2.svg' type='image/svg+xml' "
         "width='"  + QString::number( data_plot2->size().width()  * 1.4 ) + "' "
         "height='" + QString::number( data_plot2->size().height() * 1.4 ) + 
         "'></object></div>\n"

         "</body></html>\n";

   report.close();
#endif

   // Tell user
   QMessageBox::warning( this,
         tr( "Success" ),
         tr( "Wrote:\n" ) 
         + htmlFile  + "\n" 
         + plot1File + "\n" 
         + plot2File + "\n" 
         + textFile  + "\n" );
}
