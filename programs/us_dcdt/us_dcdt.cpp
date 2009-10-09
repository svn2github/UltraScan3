//! \file us_dcdt.cpp

#include <QApplication>
#include <QtSvg>

#include "us_dcdt.h"
#include "us_license_t.h"
#include "us_license.h"
#include "us_settings.h"
#include "us_gui_settings.h"

//! \brief Main program for us_dcdt. Loads translators and starts
//         the class US_Dcdt.

int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );

   #include "main1.inc"

   // License is OK.  Start up.
   
   US_Dcdt w;
   w.show();                   //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

US_Dcdt::US_Dcdt() : US_AnalysisBase()
{
   setWindowTitle( tr( "Time Derivitive - dC/dt Analysis" ) );

   te_results = NULL;

   QLabel*       lb_aux    = us_banner( tr( "dC/dt Auxiliary Controls" ) );
   QLabel*       lb_sValue = us_label( tr( "S-value Cutoff:" ) );
   QwtCounter*   ct_sValue = us_counter( 3, 0, 1000, 20 );
   
   QLabel*       lb_graph  = us_label( tr( "Graph Selection" ) );
   lb_graph->setAlignment ( Qt::AlignCenter );

   QRadioButton* rb_radius;
   QGridLayout*  rb_layout1 = us_radiobutton( tr( "x:radius" ) , rb_radius, true );
   
   QRadioButton* rb_sed;
   QGridLayout*  rb_layout2 = us_radiobutton( tr( "x:S" ) , rb_sed, false );

   QRadioButton* rb_avg;
   QGridLayout*  rb_layout3 = us_radiobutton( tr( "Average S" ) , rb_avg, false );

   QButtonGroup* group = new QButtonGroup( this );
   group->addButton( rb_radius, 0 );
   group->addButton( rb_sed   , 1 );
   group->addButton( rb_avg   , 2 );
   graphType = 0;
   connect( group, SIGNAL( buttonClicked( int ) ), SLOT( set_graph( int ) ) );

   QBoxLayout* rb_layout0 = new QHBoxLayout();
   rb_layout0->addLayout( rb_layout1 );
   rb_layout0->addLayout( rb_layout2 );
   rb_layout0->addLayout( rb_layout3 );

   int row = 7;

   controlsLayout->addWidget( lb_aux,     row++, 0, 1, 4 );

   controlsLayout->addWidget( lb_sValue,  row,   0, 1, 2 );
   controlsLayout->addWidget( ct_sValue,  row++, 2, 1, 2 );
   
   controlsLayout->addWidget( lb_graph,   row++, 0, 1, 4 );
   controlsLayout->addLayout( rb_layout0, row++, 0, 1, 4 );


   connect( pb_help,  SIGNAL( clicked() ), SLOT( help() ) );
   connect( pb_view,  SIGNAL( clicked() ), SLOT( view() ) );
   connect( pb_save,  SIGNAL( clicked() ), SLOT( save() ) );
}

void US_Dcdt::set_graph( int button )
{
   if ( graphType == button ) return;
qDebug() << button;
   graphType = button;
   //data_plot();
}

void US_Dcdt::data_plot( void )
{
   US_AnalysisBase::data_plot();
/*
   time_correction = US_Math::time_correction( dataList );

   int                    index  = lw_triples->currentRow();
   US_DataIO::editedData* d      = &dataList[ index ];

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

   data_plot1->setTitle( tr( "Run " ) + d->runID + tr( ": Cell " ) + d->cell
             + " (" + d->wavelength + tr( " nm) - Second Moment Plot" ) );

   data_plot1->setAxisTitle( QwtPlot::xBottom, tr( "Scan Number" ) );
   data_plot1->setAxisTitle( QwtPlot::yLeft  , 
         tr( "Corrected Sed. Coeff. (1e-13 s)" ) );

   // Calculate solution parameters
   solution.density   = le_density  ->text().toDouble();
   solution.viscosity = le_viscosity->text().toDouble();
   solution.vbar      = le_vbar     ->text().toDouble();
   
   double sum = 0;
   int    count;
   
   for ( int i = 0; i < scanCount; i++ ) sum += d->scanData[ i ].temperature;
   
   double avgTemp  = sum / scanCount;
   solution.vbar20 = US_Math::adjust_vbar( solution.vbar, avgTemp );
   US_Math::data_correction( avgTemp, solution );

   if ( smPoints  != NULL ) delete [] smPoints;
   if ( smSeconds != NULL ) delete [] smSeconds;

   smPoints  = new double[ scanCount ];
   smSeconds = new double[ scanCount ];

   // Calculate the 2nd moment
   for ( int i = 0; i < scanCount; i++ )
   {
      double sum1  = 0.0;
      double sum2  = 0.0;
      count        = 0;

      // The span is the boundary portion that is going to be analyzed (in
      // percent)
qDebug() << d->scanData[ i ].plateau - baseline;

      double range  = ( d->scanData[ i ].plateau - baseline ) * boundaryPct;
      double test_y = range * positionPct;

      while ( d->scanData[ i ].readings[ count ].value - baseline < test_y ) 
         count++;

      int points = d->scanData[ i ].readings.size();

      if ( count == 0 ) count = 1;

      while ( count < points )
      {   
         double value  = d->scanData[ i ].readings[ count ].value - baseline;
         double radius = d->scanData[ i ].readings[ count ].d.radius;

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
qDebug() << smPoints [ i ] << smSeconds[ i ];      
   }

   double* x = new double[ scanCount ];
   double* y = new double[ scanCount ];
   
   // Sedimentation coefficients from all scans that have not cleared the
   // meniscus form a separate plot that will be plotted in red, and will not
   // be included in the line fit:
    
   QwtPlotCurve* curve;
   QwtSymbol     sym;
   
   count = 0;

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
   */
}

void US_Dcdt::view( void )
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

   // Add results to window
   QString s = "<html><head>\n"
               "<style>td { padding-right: 1em;}</style>\n"
               "</head><body>\n" +
               results() + 
               "</body></html>\n";

   te_results->e->setHtml( s );
   te_results->show();
}

void US_Dcdt::save( void )
{
   int                    index  = lw_triples->currentRow();
   US_DataIO::editedData* d      = &dataList[ index ];

   QString dir = US_Settings::reportDir();
   QDir    folder( dir );
   
   if ( ! folder.exists( d->runID ) )
   {
      if ( ! folder.mkdir( d->runID ) )
      {
         QMessageBox::warning( this,
               tr( "File error" ),
               tr( "Could not create the directory:\n" ) +
               dir + "/" + d->runID );
         return;
      }
   }

   // Note: d->runID is both directory and first segment of file name
   QString filebase = dir + "/" + d->runID + "/" + d->runID + "." + d->cell + 
       + "." + d->channel + "." + d->wavelength;
   
   QString plot1File = filebase + ".dcdt_plot1.svg";
   QString plot2File = filebase + ".dcdt_plot2.svg";
   QString textFile  = filebase + ".dcdt_data.txt";
   QString htmlFile  = filebase + ".dcdt_report.html";


   // Write plots
   QSvgGenerator generator1;
   generator1.setSize( data_plot1->size() );
   generator1.setFileName( plot1File );
   data_plot1->print( generator1 );

   QSvgGenerator generator2;
   generator2.setSize( data_plot2->size() );
   generator2.setFileName( plot2File );
   data_plot2->print( generator2 );

   // Write dcdt data
   QFile dcdt_data( textFile );
   if ( ! dcdt_data.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
   {
      QMessageBox::warning( this,
            tr( "IO Error" ),
            tr( "Could not open\n" ) + textFile + "\n" +
                tr( "\nfor writing" ) );
      return;
   }

   QTextStream ts_data( &dcdt_data );

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

 //        ts_data << count + 1 << "\t" << smPoints[ i ] 
 //                << "\t" << smSeconds[ i ] << "\n";
         count++;
      }
   }

   dcdt_data.close();

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

   // Remove directory from string
   filebase = d->runID + "." + d->cell + "." + d->channel + "." + d->wavelength;

   QTextStream report_ts( &report );

   report_ts << "<html><head>\n"
         "<style>td { padding-right: 1em;}</style>\n"
         "</head><body>\n" +
         results() + 
         "<h3><a href='" + filebase + ".dcdt_data.txt'>" 
                         "Text File of Second Moment Plot Data</a></h3>\n"

         "<div><h3>Second Moment Plot</h3>\n"
         "<object data='" + filebase + ".dcdt_plot1.svg' type='image/svg+xml' "
         "width='"  + QString::number( data_plot1->size().width()  * 1.4 ) + "' "
         "height='" + QString::number( data_plot1->size().height() * 1.4 ) + 
         "'></object></div>\n"
         
         "<div><h3>Velocity Plot</h3>\n"
         "<object data='" + filebase + ".dcdt_plot2.svg' type='image/svg+xml' "
         "width='"  + QString::number( data_plot2->size().width()  * 1.4 ) + "' "
         "height='" + QString::number( data_plot2->size().height() * 1.4 ) + 
         "'></object></div>\n"

         "</body></html>\n";

   report.close();

   // Tell user
   QMessageBox::warning( this,
         tr( "Success" ),
         tr( "Wrote:\n" ) 
         + htmlFile  + "\n" 
         + plot1File + "\n" 
         + plot2File + "\n" 
         + textFile  + "\n" );
}

QString US_Dcdt::results( void )
{
   /*
   int                    index  = lw_triples->currentRow();
   US_DataIO::editedData* d      = &dataList[ index ];

   QString s = 
      tr( "<h1>Second Moment Analysis</h1>\n" )   +
      tr( "<h2>Data Report for Run \"" ) + d->runID + tr( "\", Cell " ) + d->cell +
      tr(  ", Wavelength " ) + d->wavelength + "</h2>\n";

   s += tr( "<h3>Detailed Run Information:</h3>\n" ) + "<table>\n" +
        table_row( tr( "Cell Description:" ), d->description )     +
        table_row( tr( "Data Directory:"   ), directory )          +
        table_row( tr( "Rotor Speed:"      ),  
            QString::number( (int)d->scanData[ 0 ].rpm ) + " rpm" );

   // Temperature data
   double sum     =  0.0;
   double maxTemp = -1.0e99;
   double minTemp =  1.0e99;

   for ( int i = 0; i < d->scanData.size(); i++ )
   {
      double t = d->scanData[ i ].temperature;
      sum += t;
      maxTemp = max( maxTemp, t );
      minTemp = min( minTemp, t );
   }

   QString average = QString::number( sum / d->scanData.size(), 'f', 1 );

   s += table_row( tr( "Average Temperature:" ), average + " &deg;C" );

   if ( maxTemp - minTemp <= US_Settings::tempTolerance() )
      s += table_row( tr( "Temperature Variation:" ), tr( "Within tolerance" ) );
   else 
      s += table_row( tr( "Temperature Variation:" ), 
                      tr( "(!) OUTSIDE TOLERANCE (!)" ) );

   // Time data
   int minutes = (int)time_correction / 60;
   int seconds = (int)time_correction % 60;

   QString m   = ( minutes == 1 ) ? tr( " minute " ) : tr( " minutes " );
   QString sec = ( seconds == 1 ) ? tr( " second"  ) : tr( " seconds"  );
   s += table_row( tr( "Time Correction:" ), 
                   QString::number( minutes ) + m +
                   QString::number( seconds ) + sec );

   double duration = rawList.last().scanData.last().seconds;

   int hours = (int) duration / 3600;
   minutes   = (int) duration / 60 - hours * 60;
   seconds   = (int) duration % 60;

   QString h;
   h   = ( hours   == 1 ) ? tr( " hour "   ) : tr( " hours " );
   m   = ( minutes == 1 ) ? tr( " minute " ) : tr( " minutes " );
   sec = ( seconds == 1 ) ? tr( " second" ) : tr( " seconds" );

   s += table_row( tr( "Run Duration:" ),
                   QString::number( hours   ) + h + 
                   QString::number( minutes ) + m + 
                   QString::number( seconds ) + sec );

   // Wavelength, baseline, meniscus, range
   s += table_row( tr( "Wavelength:" ), d->wavelength + " nm" )  +
        table_row( tr( "Baseline Absorbance:" ),
                   QString::number( calc_baseline(), 'f', 6 ) + " OD" ) + 
        table_row( tr( "Meniscus Position:     " ),           
                   QString::number( d->meniscus, 'f', 3 ) + " cm" );

   double left  =  d->scanData[ 0 ].readings[ 0 ]  .d.radius;
   double right =  d->scanData[ 0 ].readings.last().d.radius;

   s += table_row( tr( "Edited Data starts at: " ), 
                   QString::number( left,  'f', 3 ) + " cm" ) +
        table_row( tr( "Edited Data stops at:  " ), 
                   QString::number( right, 'f', 3 ) + " cm" ) + "</table>\n";

   // Settings

   s += tr( "<h3>Hydrodynamic Settings:</h3>\n" ) + 
        "<table>\n";
  
   s += table_row( tr( "Viscosity corrected:" ), 
                   QString::number( solution.viscosity, 'f', 5 ) ) +
        table_row( tr( "Viscosity (absolute):" ),
                   QString::number( solution.viscosity_tb, 'f', 5 ) ) +
        table_row( tr( "Density corrected:" ),
                   QString::number( solution.density, 'f', 6 ) + " g/ccm" ) +
        table_row( tr( "Density (absolute):" ),
                   QString::number( solution.density_tb, 'f', 6 ) + " g/ccm" ) +
        table_row( tr( "Vbar:" ), 
                   QString::number( solution.vbar, 'f', 4 ) + " ccm/g" ) +
        table_row( tr( "Vbar corrected for 20 &deg;C:" ),
                   QString::number( solution.vbar20, 'f', 4 ) + " ccm/g" ) +
        table_row( tr( "Buoyancy (Water, 20 &deg;C): " ),
                   QString::number( solution.buoyancyw, 'f', 6 ) ) +
        table_row( tr( "Buoyancy (absolute)" ),
                   QString::number( solution.buoyancyb, 'f', 6 ) ) +
        table_row( tr( "Correction Factor:" ),
                   QString::number( solution.correction, 'f', 6 ) ) + 
        "</table>\n";

   s += tr( "<h3>Data Analysis Settings:</h3>\n" ) +
        "<table>\n" +

        table_row( tr( "Smoothing Frame:" ),
                   QString::number( (int)ct_smoothing->value() ) ) + 
        table_row( tr( "Analyzed Boundary:" ),
                   QString::number( (int)ct_boundaryPercent->value() ) + " %" )+
        table_row( tr( "Boundary Position:" ),
                   QString::number( (int)ct_boundaryPos->value() ) + " %" ) +
        table_row( tr( "Early Scans skipped:" ),
                   le_skipped->text() + " scans" ) +
        table_row( tr( "Average Second Moment S: " ),
                   QString::number( average_2nd, 'f', 5 ) + " s * 10e-13" ) +
        "</table>";

   s += tr( "<h3>Scan Information:</h3>\n" ) +
        "<table>\n"; 
         
   s += table_row( tr( "Scan" ), tr( "Corrected Time" ), 
                   tr( "Plateau Concentration" ) );

   for ( int i = 0; i < d->scanData.size(); i++ )
   {
      QString s1;
      QString s2;
      QString s3;

      double od   = d->scanData[ i ].plateau;
      int    time = (int)( d->scanData[ i ].seconds - time_correction ); 

      s1 = s1.sprintf( "%4d",             i + 1 );
      s2 = s2.sprintf( "%4d min %2d sec", time / 60, time % 60 );
      s3 = s3.sprintf( "%.6f OD",         od ); 

      s += table_row( s1, s2, s3 );
   }

   s += "</table>";
*/
   QString s = "<p>TBD</p>";
   return s;
}
