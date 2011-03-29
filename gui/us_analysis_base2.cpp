//! \file us_analysis_base2.cpp

#include <QtSvg>

#include "us_analysis_base2.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_run_details2.h"
#include "us_analyte_gui.h"
#include "us_buffer_gui.h"
#include "us_data_loader.h"
#include "us_noise_loader.h"
#include "us_loadable_noise.h"
#include "us_db2.h"
#include "us_passwd.h"
#include "us_solution_vals.h"

US_AnalysisBase2::US_AnalysisBase2() : US_Widgets()
{
   setPalette( US_GuiSettings::frameColor() );

   mainLayout      = new QGridLayout( this );
   mainLayout->setSpacing        ( 2 );
   mainLayout->setContentsMargins( 2, 2, 2, 2 );

   leftLayout      = new QVBoxLayout();
   rightLayout     = new QVBoxLayout();
   
   analysisLayout  = new QGridLayout();
   runInfoLayout   = new QGridLayout();
   parameterLayout = new QGridLayout();
   controlsLayout  = new QGridLayout();
   buttonLayout    = new QHBoxLayout();

   leftLayout->addLayout( analysisLayout  );
   leftLayout->addLayout( runInfoLayout   );
   leftLayout->addLayout( parameterLayout );
   leftLayout->addLayout( controlsLayout  );
   leftLayout->addStretch();
   leftLayout->addLayout( buttonLayout    );

   // Plots
   plotLayout1 = new US_Plot( data_plot1,
            tr( "Plot 1 Title" ),
            tr( "X-Axis Title" ),
            tr( "Y-Axis Title" ) );

   data_plot1->setMinimumSize( 600, 150 );

   plotLayout2 = new US_Plot( data_plot2,
            tr( "Plot 2 Title" ),
            tr( "X-Axis Title" ),
            tr( "Y-Axis Title" ) );

   data_plot2->setMinimumSize( 600, 150 );

   rightLayout->addLayout( plotLayout1 );
   rightLayout->addLayout( plotLayout2 );

   mainLayout->addLayout( leftLayout,  0, 0 );
   mainLayout->addLayout( rightLayout, 0, 1 );
   mainLayout->setColumnStretch( 0, 0 );
   mainLayout->setColumnStretch( 1, 99 );

   // Analysis buttons
   pb_load    = us_pushbutton( tr( "Load Experiment" ) );
   connect( pb_load, SIGNAL( clicked() ), SLOT( load() ) );
   pb_details = us_pushbutton( tr( "Run Details" ) );
   QLayout* lo_edlast =
                us_checkbox(   tr( "Latest Data Edit" ), ck_edlast, true ); 
   connect( pb_details, SIGNAL( clicked() ), SLOT( details() ) );

   disk_controls = new US_Disk_DB_Controls;

   pb_view    = us_pushbutton( tr( "View Data Report" ) );
   pb_save    = us_pushbutton( tr( "Save Data" ) );

   pb_details->setEnabled( false );
   pb_view   ->setEnabled( false );
   pb_save   ->setEnabled( false );

   int row = 0;
   analysisLayout->addWidget( pb_load,       row,   0, 1, 1 );
   analysisLayout->addWidget( pb_details,    row++, 1, 1, 1 );
   analysisLayout->addLayout( lo_edlast,     row,   0, 1, 1 );
   analysisLayout->addLayout( disk_controls, row++, 1, 1, 1 );
   analysisLayout->addWidget( pb_view,       row,   0, 1, 1 );
   analysisLayout->addWidget( pb_save,       row++, 1, 1, 1 );

   // Standard buttons
   pb_reset = us_pushbutton( tr( "Reset" ) );
   pb_help  = us_pushbutton( tr( "Help"  ) );
   pb_close = us_pushbutton( tr( "Close" ) );

   buttonLayout->addWidget( pb_reset );
   connect( pb_reset, SIGNAL( clicked() ), SLOT( reset() ) );
   buttonLayout->addWidget( pb_help  );
   buttonLayout->addWidget( pb_close );
   connect( pb_close, SIGNAL( clicked() ), SLOT( close() ) );

   // Run info
   QLabel* lb_info    = us_banner( tr( "Information for this Run" ) );
   QLabel* lb_triples = us_banner( tr( "Cell / Channel / Wavelength" ) );
   QLabel* lb_id      = us_label ( tr( "Run ID / Edit ID:" ) );
   QLabel* lb_temp    = us_label ( tr( "Avg Temperature:" ) );

   le_id      = us_lineedit();
   le_temp    = us_lineedit();

   te_desc    = us_textedit();
   lw_triples = us_listwidget();

   QFont        font( US_GuiSettings::fontFamily(), 
                      US_GuiSettings::fontSize() );
   QFontMetrics fm  ( font );

   int fontHeight = fm.lineSpacing();

   te_desc   ->setMaximumHeight( fontHeight * 2 + 12 );  // Add for border
   lw_triples->setMaximumHeight( fontHeight * 6 + 12 );

   le_id     ->setReadOnly( true );
   le_temp   ->setReadOnly( true );
   te_desc   ->setReadOnly( true );
   QPalette gray = US_GuiSettings::editColor();
   gray.setColor( QPalette::Base, QColor( 0xe0, 0xe0, 0xe0 ) );
   le_id     ->setPalette( gray );
   le_temp   ->setPalette( gray );
   te_desc   ->setPalette( gray );

   row = 0;
   runInfoLayout->addWidget( lb_info   , row++, 0, 1, 2 );
   runInfoLayout->addWidget( lb_id     , row,   0 );
   runInfoLayout->addWidget( le_id     , row++, 1 );
   runInfoLayout->addWidget( lb_temp   , row,   0 );
   runInfoLayout->addWidget( le_temp   , row++, 1 );
   runInfoLayout->addWidget( te_desc   , row,   0, 2, 2 );
   row += 2;
   runInfoLayout->addWidget( lb_triples, row++, 0, 1, 2 );
   runInfoLayout->addWidget( lw_triples, row++, 0, 4, 2 );

   // Parameters

   QPushButton* pb_density   = us_pushbutton( tr( "Density"   ) );
   connect( pb_density,   SIGNAL( clicked() ), SLOT( get_buffer() ) );
   
   QPushButton* pb_viscosity = us_pushbutton( tr( "Viscosity" ) );
   connect( pb_viscosity, SIGNAL( clicked() ), SLOT( get_buffer() ) );
   
   QPushButton* pb_vbar      = us_pushbutton( tr( "Vbar"   ) );
   connect( pb_vbar,      SIGNAL( clicked() ), SLOT( get_vbar()   ) );
   
   QLabel* lb_skipped   = us_label     ( tr( "Skipped:"  ) );

   density      = DENS_20W;
   viscosity    = VISC_20W;
   vbar         = TYPICAL_VBAR;

   le_density   = us_lineedit( QString::number( density,   'f', 6 ) );
   le_viscosity = us_lineedit( QString::number( viscosity, 'f', 5 ) );
   le_vbar      = us_lineedit( QString::number( vbar,      'f', 5 ) );
   le_skipped   = us_lineedit( "0" );
   le_skipped->setReadOnly( true );
   le_skipped->setPalette ( gray );

   row = 0;
   parameterLayout->addWidget( pb_density  , row,   0 );
   parameterLayout->addWidget( le_density  , row,   1 );
   parameterLayout->addWidget( pb_viscosity, row,   2 );
   parameterLayout->addWidget( le_viscosity, row++, 3 );
   parameterLayout->addWidget( pb_vbar     , row,   0 );
   parameterLayout->addWidget( le_vbar     , row,   1 );
   parameterLayout->addWidget( lb_skipped  , row,   2 );
   parameterLayout->addWidget( le_skipped  , row++, 3 );

   // Analysis Controls
   QLabel* lb_analysis     = us_banner( tr( "Analysis Controls"  ) ); 
   QLabel* lb_scan         = us_banner( tr( "Scan Control"       ) ); 
   QLabel* lb_smoothing    = us_label ( tr( "Data Smoothing:"    ) ); 
   QLabel* lb_boundPercent = us_label ( tr( "% of Boundary:"     ) ); 
   QLabel* lb_boundPos     = us_label ( tr( "Boundary Pos. (%):" ) ); 

   QLabel* lb_from         = us_label ( tr( "From:" ) );
   QLabel* lb_to           = us_label ( tr( "to:"   ) );

   pb_exclude = us_pushbutton( tr( "Exclude Scan Range" ) );
   pb_exclude->setEnabled( false );
   connect( pb_exclude, SIGNAL( clicked() ), SLOT( exclude() ) );

   pb_reset_exclude = us_pushbutton( tr( "Reset Scan Range" ) );
   pb_reset_exclude->setEnabled( false );
   connect( pb_reset_exclude, SIGNAL( clicked() ), SLOT( reset_excludes() ) );

   ct_smoothing = us_counter( 2,  1,  50,  1 );
   ct_smoothing->setStep( 1.0 );
   connect( ct_smoothing, SIGNAL( valueChanged( double ) ),
                          SLOT  ( smoothing   ( double ) ) );

   ct_boundaryPercent = us_counter( 3, 10, 100, 90 );
   ct_boundaryPos     = us_counter( 3,  0,  10,  5 );
   ct_boundaryPercent->setStep( 0.1 );
   ct_boundaryPos    ->setStep( 0.1 );
   connect( ct_boundaryPercent, SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pct( double ) ) );
   connect( ct_boundaryPos,     SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pos( double ) ) );
   
   ct_from            = us_counter( 2, 0, 0 );
   ct_to              = us_counter( 2, 0, 0 );
   
   connect( ct_from, SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_from( double ) ) );
   connect( ct_to,   SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_to  ( double ) ) );

   row = 0;
   controlsLayout->addWidget( lb_scan           , row++, 0, 1, 4 );
   controlsLayout->addWidget( lb_from           , row,   0 );
   controlsLayout->addWidget( ct_from           , row,   1 );
   controlsLayout->addWidget( lb_to             , row,   2 );
   controlsLayout->addWidget( ct_to             , row++, 3 );
   controlsLayout->addWidget( pb_exclude        , row,   0, 1, 2 );
   controlsLayout->addWidget( pb_reset_exclude  , row++, 2, 1, 2 );
   controlsLayout->addWidget( lb_analysis       , row++, 0, 1, 4 );
   controlsLayout->addWidget( lb_smoothing      , row,   0, 1, 2 );
   controlsLayout->addWidget( ct_smoothing      , row++, 2, 1, 2 );
   controlsLayout->addWidget( lb_boundPercent   , row,   0, 1, 2 );
   controlsLayout->addWidget( ct_boundaryPercent, row++, 2, 1, 2 );
   controlsLayout->addWidget( lb_boundPos       , row,   0, 1, 2 );
   controlsLayout->addWidget( ct_boundaryPos    , row++, 2, 1, 2 );

   dataLoaded = false;
   buffLoaded = false;

   dfilter    = "";

   connect( le_density,   SIGNAL( returnPressed() ),
            this,         SLOT(   buffer_text()     ) );
   connect( le_viscosity, SIGNAL( returnPressed() ),
            this,         SLOT(   buffer_text()     ) );
   connect( le_vbar,      SIGNAL( returnPressed() ),
            this,         SLOT(   vbar_text()       ) );
}

void US_AnalysisBase2::update_disk_db( bool db )
{
   ( db ) ? disk_controls->set_db() : disk_controls->set_disk();
}

void US_AnalysisBase2::load( void )
{
   // Determine the edit ID
   dataLoaded = false;
   dataList     .clear();
   rawList      .clear();
   excludedScans.clear();
   triples      .clear();

   reset();

   bool edlast = ck_edlast->isChecked();
   int  dbdisk = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                         : US_Disk_DB_Controls::Disk;
   QString description;

   US_DataLoader* dialog = new US_DataLoader(
         edlast, dbdisk, rawList, dataList, triples, description );

   connect( dialog, SIGNAL( changed( bool ) ), SLOT( update_disk_db( bool ) ) );
   connect( dialog, SIGNAL( progress    ( const QString& ) ), 
                    SLOT  ( set_progress( const QString& ) ) );

   if ( dialog->exec() != QDialog::Accepted ) return;

   if ( disk_controls->db() )
      directory = tr( "(database)" );

   else
   {
      directory = description.section( description.left( 1 ), 4, 4 );
      directory = directory.left( directory.lastIndexOf( "/" ) );
   }

   lw_triples->disconnect();
   lw_triples->clear();

   for ( int ii=0; ii < triples.size(); ii++ )
      lw_triples->addItem( triples.at( ii ) );

   savedValues.clear();

   for ( int i = 0; i < dataList[ 0 ].scanData.size(); i++ )
   {
      US_DataIO2::Scan* s = &dataList[ 0 ].scanData[ i ];
      int points = s->readings.size();

      QVector< double > v;
      v.resize( points );

      for ( int j = 0; j < points; j++ ) v[ j ] = s->readings[ j ].value;

      savedValues << v;
   }

   noiflags.fill( -1, dataList.size() );

   connect( lw_triples, SIGNAL( currentRowChanged( int ) ), 
                        SLOT  ( new_triple       ( int ) ) );
   lw_triples->setCurrentRow( 0 );

   // Enable other buttons
   pb_details->setEnabled( true );
   pb_view   ->setEnabled( true );
   pb_save   ->setEnabled( true );
   pb_exclude->setEnabled( true );

   ct_from->disconnect();
   ct_from->setValue( 0 );

   connect( ct_from, SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_from( double ) ) );

   // Set up solution/buffer values implied from experimental data
   QString solID;
   QString bufID;
   QString bguid;
   QString bdesc;
   QString bdens  = le_density  ->text();
   QString bvisc  = le_viscosity->text();
   QString svbar  = le_vbar     ->text();
   QString bcomp  = "";
   QString errmsg = "";
   dbdisk         = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                            : US_Disk_DB_Controls::Disk;
   US_Passwd pw;
   US_DB2*   dbP  = ( dbdisk == US_Disk_DB_Controls::DB ) ?
                    new US_DB2( pw.getPasswd() ) : 0;

   bool    bufin  = US_SolutionVals::values( dbP, &dataList[ 0 ], solID, svbar,
                                             bdens, bvisc, bcomp, errmsg );

   if ( bufin )
   {
      buffLoaded  = false;
      le_density  ->setText( bdens );
      le_viscosity->setText( bvisc );
      le_vbar     ->setText( svbar );
      density     = bdens.toDouble();
      viscosity   = bvisc.toDouble();
      vbar        = svbar.toDouble();
      buffLoaded  = true;
qDebug() << "SolID" << solID;

      if ( solID.isEmpty() )
      {
         QMessageBox::warning( this, tr( "Solution/Buffer Values Fetch" ),
            tr( "Empty solution ID value!" ) );
      }

      else if ( solID.length() < 36  &&  dbP != NULL )
      {  // Have DB solution ID
         solution_rec.readFromDB( solID.toInt(), dbP );
qDebug() << "Sol-from-DB: #analytes" << solution_rec.analytes.size();
      }

      else
      {  // Have Local solution GUID
         solution_rec.readFromDisk( solID );
qDebug() << "Sol-from-Local: #analytes" << solution_rec.analytes.size();
      }
   }

   else
   {
      QMessageBox::warning( this, tr( "Solution/Buffer Values Fetch" ),
         errmsg );
   }

   dataLoaded = true;
   qApp->processEvents();
}

void US_AnalysisBase2::update( int selection )
{
   US_DataIO2::EditedData* d = &dataList[ selection ];
   int scanCount = d->scanData.size();
   runID         = d->runID;
   le_id->setText( runID + " / " + d->editID );

   double avTemp = d->average_temperature();

   le_temp->setText( QString::number( avTemp, 'f', 1 ) + " " + DEGC );

   te_desc->setText( d->description );

   ct_smoothing      ->disconnect();
   ct_boundaryPercent->disconnect();
   ct_boundaryPos    ->disconnect();

   ct_smoothing      ->setValue( 1  );  // Signals?
   ct_boundaryPercent->setValue( 90 );
   ct_boundaryPos    ->setValue( 5  );

   connect( ct_smoothing,       SIGNAL( valueChanged( double ) ),
                                SLOT  ( smoothing   ( double ) ) );
   connect( ct_boundaryPercent, SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pct( double ) ) );
   connect( ct_boundaryPos,     SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pos( double ) ) );

   ct_from->setMaxValue( scanCount - excludedScans.size() );
   ct_from->setStep( 1.0 );
   ct_to  ->setMaxValue( scanCount - excludedScans.size() );
   ct_to  ->setStep( 1.0 );

   data_plot();
}


void US_AnalysisBase2::details( void )
{
   US_RunDetails2* dialog
      = new US_RunDetails2( rawList, runID, directory, triples );
   dialog->exec();
   qApp->processEvents();
   delete dialog;
}

void US_AnalysisBase2::get_vbar( void )
{
   int dbdisk = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                        : US_Disk_DB_Controls::Disk;
   double avgTemp = le_temp->text().section( " ", 0, 0 ).toDouble();
   avgTemp        = ( avgTemp > 0.0 ) ? avgTemp : NORMAL_TEMP;
qDebug() << "Average Temp" << avgTemp;
qDebug() << "Average Temp" << le_temp->text();

   US_AnalyteGui* dialog = new US_AnalyteGui( true, QString(), dbdisk,
                                              avgTemp );

   connect( dialog, SIGNAL( valueChanged( US_Analyte ) ),
                    SLOT  ( update_vbar ( US_Analyte ) ) );

   connect( dialog, SIGNAL( use_db( bool ) ), SLOT( update_disk_db( bool ) ) );
   dialog->exec();
   qApp->processEvents();
}

void US_AnalysisBase2::update_vbar( US_Analyte analyte )
{
   bool changed = true;

   if ( buffLoaded )
      changed = verify_vbar();

   if ( changed )
   {
      vbar = analyte.vbar20;

      buffLoaded = false;
      le_vbar->setText( QString::number( vbar, 'f', 5 ) );
      qApp->processEvents();

      if ( dataLoaded ) data_plot();
   }
}

void US_AnalysisBase2::get_buffer( void )
{
   int dbdisk = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                        : US_Disk_DB_Controls::Disk;

   US_BufferGui* dialog = new US_BufferGui( true, buff, dbdisk );

   connect( dialog, SIGNAL( valueChanged ( double, double ) ),
                    SLOT  ( update_buffer( double, double ) ) );

   connect( dialog, SIGNAL( use_db( bool ) ), SLOT( update_disk_db( bool ) ) );
   dialog->exec();
   qApp->processEvents();
}

void US_AnalysisBase2::update_buffer( double new_density, double new_viscosity )
{
   bool changed = true;

   if ( buffLoaded )
      changed = verify_buffer();

   if ( changed )
   {
      density    = new_density;
      viscosity  = new_viscosity;

      buffLoaded = false;
      le_density  ->setText( QString::number( density,   'f', 6 ) );
      le_viscosity->setText( QString::number( viscosity, 'f', 5 ) );
      qApp->processEvents();

      if ( dataLoaded )
         data_plot();
   }
}

void US_AnalysisBase2::data_plot( void )
{
   int                     row  = lw_triples->currentRow();
   US_DataIO2::EditedData* d    = &dataList[ row ];

   QString header = tr( "Velocity Data for ") + d->runID;
   data_plot2->setTitle( header );

   header = tr( "Absorbance at " ) + d->wavelength + tr( " nm" );
   data_plot2->setAxisTitle( QwtPlot::yLeft, header );

   header = tr( "Radius (cm) " );
   data_plot2->setAxisTitle( QwtPlot::xBottom, header );

   data_plot2->clear();
   us_grid( data_plot2 );

   int     scan_number = 0;
   int     from        = (int)ct_from->value();
   int     to          = (int)ct_to  ->value();

   int     scanCount   = d->scanData.size();
   int     points      = d->scanData[ 0 ].readings.size();
   double  boundaryPct = ct_boundaryPercent->value() / 100.0;
   double  positionPct = ct_boundaryPos    ->value() / 100.0;
   double  baseline    = calc_baseline();

   QVector< double > rvec( points );
   QVector< double > vvec( points );
   double* r           = rvec.data();
   double* v           = vvec.data();

   // Calculate basic parameters for other functions
   time_correction    = US_Math2::time_correction( dataList );

   solution.density   = le_density  ->text().toDouble();
   solution.viscosity = le_viscosity->text().toDouble();
   solution.vbar20    = le_vbar     ->text().toDouble();
   double avgTemp     = d->average_temperature();
   solution.vbar      = US_Math2::calcCommonVbar( solution_rec, avgTemp );

   US_Math2::data_correction( avgTemp, solution );

   // Draw curves
   for ( int i = 0; i < scanCount; i++ )
   {
      if ( excludedScans.contains( i ) ) continue;

      scan_number++;
      bool highlight = scan_number >= from  &&  scan_number <= to;

      US_DataIO2::Scan* s = &d->scanData[ i ];

      double range       = s->plateau - baseline;
      double lower_limit = baseline    + range * positionPct;
      double upper_limit = lower_limit + range * boundaryPct;

      int j     = 0;
      int count = 0;

      // Plot each scan in (up to) three segments: below, in, and above
      // the specified boundaries
      while (  j < points  &&  s->readings[ j ].value < lower_limit )
      {
         r[ count ] = d->x       [ j ].radius;
         v[ count ] = s->readings[ j ].value;
         j++;
         count++;
      }

      QString       title; 
      QwtPlotCurve* c;

      if ( count > 1 )
      {
         title = tr( "Curve " ) + QString::number( i ) + tr( " below range" );
         c     = us_curve( data_plot2, title );

         if ( highlight )
            c->setPen( QPen( Qt::red ) );
         else
            c->setPen( QPen( Qt::cyan ) );
         
         c->setData( r, v, count );
      }

      count = 0;

      while (   j < points && s->readings[ j ].value < upper_limit )
      {
         r[ count ] = d->x       [ j ].radius;
         v[ count ] = s->readings[ j ].value;
         j++;
         count++;
      }

      if ( count > 1 )
      {
         title = tr( "Curve " ) + QString::number( i ) + tr( " in range" );
         c = us_curve( data_plot2, title );

         if ( highlight )
            c->setPen( QPen( Qt::red ) );
         else
            c->setPen( QPen( US_GuiSettings::plotCurve() ) );
         
         c->setData( r, v, count );
      }

      count = 0;

      while ( j < points )
      {
         r[ count ] = d->x       [ j ].radius;
         v[ count ] = s->readings[ j ].value;
         j++;
         count++;
      }

      if ( count > 1 )
      {
         title = tr( "Curve " ) + QString::number( i ) + tr( " above range" );
         c = us_curve( data_plot2, title );

         if ( highlight )
            c->setPen( QPen( Qt::red ) );
         else
            c->setPen( QPen( Qt::cyan ) );
        
         c->setData( r, v, count );
      }
   }

   data_plot2->replot();

   return;
}

void US_AnalysisBase2::boundary_pct( double percent )
{
   ct_boundaryPos->disconnect();
   ct_boundaryPos->setMaxValue( 100 - percent );

   if ( ct_boundaryPos->value() >  100 - percent )
      ct_boundaryPos->setValue( 100.0 - percent );

   connect( ct_boundaryPos, SIGNAL( valueChanged( double ) ),
                            SLOT  ( boundary_pos( double ) ) );
   data_plot();
}

void US_AnalysisBase2::boundary_pos( double /* position */ )
{
   data_plot();
}

void US_AnalysisBase2::exclude_from( double from )
{
   double to = ct_to->value();

   if ( to < from )
   {
      ct_to->disconnect();
      ct_to->setValue( from );

      connect( ct_to,   SIGNAL( valueChanged( double ) ),
                        SLOT  ( exclude_to  ( double ) ) );
   }

   data_plot();
}

void US_AnalysisBase2::exclude_to( double to )
{
   double from = ct_from->value();

   if ( from > to )
   {
      ct_from->disconnect();
      ct_from->setValue( to );

      connect( ct_from, SIGNAL( valueChanged( double ) ),
                        SLOT  ( exclude_from( double ) ) );
   }

   data_plot();
}

void US_AnalysisBase2::exclude( void )
{
   double from = ct_from->value();
   double to   = ct_to  ->value();

   int                     displayedScan = 1; 
   int                     index         = lw_triples->currentRow();
   US_DataIO2::EditedData* d             = &dataList[ index ];
   int                     totalScans    = d->scanData.size();
   
   for( int i = 0; i < totalScans; i++ )
   {
      if ( excludedScans.contains( i ) ) continue;
      
      if ( displayedScan >= from  &&  displayedScan <= to ) excludedScans << i;
   
      displayedScan++;
   }

   ct_to->setValue( 0 );  // Resets both counters and replots

   ct_from->setMaxValue( totalScans - excludedScans.size() );
   ct_to  ->setMaxValue( totalScans - excludedScans.size() );
}

void US_AnalysisBase2::reset_excludes( void )
{
   int                     index      = lw_triples->currentRow();
   US_DataIO2::EditedData* d          = &dataList[ index ];
   int                     totalScans = d->scanData.size();

   excludedScans.clear();
   le_skipped->setText( "0" );

   ct_from->setMaxValue( totalScans );
   ct_to  ->setMaxValue( totalScans );

   if ( ct_to->value() != 0 )
      ct_to ->setValue( 0 );
   else
      data_plot();

   pb_reset_exclude->setEnabled( false );
}

void US_AnalysisBase2::smoothing( double smoothCount )
{
   if ( ! dataLoaded ) return;

   int smoothPoints = (int) smoothCount;

   // Restore saved data
   int                    index  = lw_triples->currentRow();
   US_DataIO2::EditedData* d      = &dataList[ index ];

   for ( int i = 0; i < d->scanData.size(); i++ )
   {
      US_DataIO2::Scan* s = &d->scanData[ i ];

      for ( int j = 0; j < s->readings.size(); j++ )
         s->readings[ j ].value = savedValues[ i ][ j ];
   }
   
   // Smooth the data
   if ( smoothPoints > 1 )
   {
      x_weights = new double [ smoothPoints ];
      y_weights = new double [ smoothPoints ];
                  
      // Divide the count into 2 standard deviations
      double increment = 2.0 / smoothCount;

      // Only calculate half a Gaussian curve, since the other side is symmetric
      for ( int i = 0; i < smoothPoints; i++ ) 
      {
         x_weights[ i ] = increment * i;

         // Use a standard deviation of 0.7 to narrow the spread and multiply
         // by 0.7 to scale the result as an empirical weighting factor
         
         // Standard deviation = 0.7, mean = 0.0, point = 0.0;
         y_weights[ i ] = 
            0.7 * US_Math2::normal_distribution( 0.7, 0.0, x_weights[ i ] );
      }

      // For each scan
      for ( int i = 0; i < d->scanData.size(); i++ )
      {
         US_DataIO2::Scan* s          = &d->scanData[ i ];
         int              scanPoints = s->readings.size();
         
         // Loop over all border point centers
         for ( int j = 0; j < smoothPoints; j++ )
         {
            s->readings[ j ].value = smooth_point( i, j, -1, smoothPoints );
         }
      
         // Now deal with all non-border points
         for ( int j = smoothPoints; j < scanPoints - smoothPoints - 1; j++ )
         {
            s->readings[ j ].value = smooth_point( i, j,  0, smoothPoints );
         }

         // Finally the points on the right border
         for ( int j = scanPoints - smoothPoints - 1; j < scanPoints; j++ )
         {
            s->readings[ j ].value 
               = smooth_point( i, j,  1, smoothPoints, scanPoints );
         }
      }
   
      delete [] x_weights;
      delete [] y_weights;
   }
   
   data_plot();
}

double US_AnalysisBase2::smooth_point( 
      int scan, int point, int type, int smoothPoints, int scanPoints )
{
   // type ==  0 means no reflection
   // type ==  1 means to reflect on the right
   // type == -1 means to reflect on the left

   double  sum      = 0.0;
   double  sum_y    = 0.0;
   int     start;
   int     stop;
   int     direction;

   // Sum all applicable points left of center
   if ( type == -1 ) // reflect left
   {
      start     = point + 1;
      stop      = point + smoothPoints;
      direction = 1;
   }
   else
   {
      start     = point - 1;
      stop      = point - smoothPoints;
      direction = -1;
   }

   // This is a bit complex because the test for leaving the loop
   // is different if we are incrementing or decrementing.
   
   int position = 0;
   int k        = start;
   
   while ( true )
   {
      position++;
      double value = savedValues[ scan ][ k ];

      if ( type ==  -1 )
      {
         if ( point - position < 0 ) // we need a reflected value
         {
            double dy = savedValues[ scan ][ k ] - savedValues[ scan ][ point ];
            value     = savedValues[ scan ][ point ] - dy;
         }
         else
            value     = savedValues[ scan ][ point - position ];
      }
      
      sum   += value * y_weights[ position ];
      sum_y +=         y_weights[ position ];

      if ( type == -1 )
      {
         if ( k > stop ) break;
      }
      else
      {
         if ( k <= stop ) break;
      }

      k += direction;
   }

   // Add the center point
   sum   += savedValues[ scan ][ point ] * y_weights[ 0 ];
   sum_y +=                                y_weights[ 0 ];
   
   // Sum all applicable points right of center
   if ( type == 1 ) // reflect right
   {
      start     = point - 1;
      stop      = point - smoothPoints;
      direction = -1;
   }
   else
   {
      start     = point + 1;
      stop      = point + smoothPoints;
      direction = 1;
   }

   position = 0;
   k        = start;

   while( true )
   {
      position++;
      double value = savedValues[ scan ][ k ];

      if ( type == 1 )
      {
         if ( point + position >= scanPoints ) // Need reflection
         {
            double dy = savedValues[ scan ][ k ] 
                      - savedValues[ scan ][ point ];
            value     = savedValues[ scan ][ point ] - dy;
         }
         else
            value     = savedValues[ scan ][ point + position ];
      }

      sum   += value * y_weights[ position ];
      sum_y +=         y_weights[ position ];

      if ( type == 1 )
      {
         if ( k <= stop ) break;
      }
      else
      {
         if ( k > stop ) break;
      }

      k += direction;
   }

   // Normalize by the sum of all weights that were used 
   return sum / sum_y;
}

void US_AnalysisBase2::reset( void )
{
   if ( ! dataLoaded ) return;

   excludedScans.clear();

   density      = DENS_20W;
   viscosity    = VISC_20W;
   vbar         = TYPICAL_VBAR;

   le_density  ->setText( QString::number( density,   'f', 6 ) );
   le_viscosity->setText( QString::number( viscosity, 'f', 5 ) );
   le_vbar     ->setText( QString::number( vbar,      'f', 5 ) );
   le_skipped  ->setText( "0" );

   // Restore saved data
   if ( dataList.size() > 0 )
   {
      int                     index  = lw_triples->currentRow();
      US_DataIO2::EditedData* d      = &dataList[ index ];
   
      for ( int i = 0; i < d->scanData.size(); i++ )
      {
         US_DataIO2::Scan* s = &d->scanData[ i ];

         for ( int j = 0; j < s->readings.size(); j++ )
            s->readings[ j ].value = savedValues[ i ][ j ];
      }
   }

   ct_from           ->disconnect();
   ct_to             ->disconnect();
   ct_smoothing      ->disconnect();
   ct_boundaryPercent->disconnect();
   ct_boundaryPos    ->disconnect();
   
   ct_from           ->setValue( 0 );
   ct_to             ->setValue( 0 );
   ct_smoothing      ->setValue( 1 );
   ct_boundaryPercent->setValue( 90 );
   ct_boundaryPos    ->setValue( 5 );

   connect( ct_from,            SIGNAL( valueChanged( double ) ),
                                SLOT  ( exclude_from( double ) ) );
                                
   connect( ct_to,              SIGNAL( valueChanged( double ) ),
                                SLOT  ( exclude_to  ( double ) ) );

   connect( ct_boundaryPercent, SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pct( double ) ) );

   connect( ct_boundaryPos,     SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pos( double ) ) );
   
   connect( ct_smoothing,       SIGNAL( valueChanged( double ) ),
                                SLOT  ( smoothing   ( double ) ) );
   data_plot();
   
}

void US_AnalysisBase2::new_triple( int index )
{
qDebug() << "NewTr: index" << index;
   // Save the data for the new triple
   US_DataIO2::EditedData* d = &dataList[ index ];
 
   savedValues.clear();

   for ( int i = 0; i < d->scanData.size(); i++ )
   {
      US_DataIO2::Scan* s = &d->scanData[ i ];
      int points = s->readings.size();

      QVector< double > v;
      v.resize( points );

      for ( int j = 0; j < points; j++ ) v[ j ] = s->readings[ j ].value;

      savedValues << v;
   }

   // Test for noise data to substract from the experiment; apply if any
   load_noise( index );

   // Update GUI elements and plot for selected triple
   update( index );
qDebug() << "NewTr: d00 s00" << d->value(0,0) << savedValues[0][0];
}

double US_AnalysisBase2::calc_baseline( void ) const
{
   int    row   = lw_triples->currentRow();
   const US_DataIO2::Scan*
          scan  = &dataList[ row ].scanData.last();
   int    point = US_DataIO2::index( *scan, dataList[ row ].x, 
                                      dataList[ row ].baseline );
          point = ( point < 5 ) ? 5 : point;
   double sum   = 0.0;
   
   for ( int j = point - 5;  j < point + 6; j++ )
      sum += scan->readings[ j ].value;

   return sum / 11.0;
}

// String to accomplish line identation
QString US_AnalysisBase2::indent( const int spaces ) const
{
   return QString( "            " ).left( spaces );
}

// Table row HTML with 2 columns
QString US_AnalysisBase2::table_row( const QString& s1, const QString& s2 ) const
{
   return ( indent( 6 ) + "<tr><td>" + s1 + "</td><td>" + s2 + "</td></tr>\n" );
}

// Table row HTML with 3 columns
QString US_AnalysisBase2::table_row( const QString& s1, const QString& s2, 
                                     const QString& s3 ) const
{
   return ( indent( 6 ) + "<tr><td>" + s1 + "</td><td>" + s2 + "</td><td>"
            + s3 + "</td></tr>\n" );
}

// Table row HTML with 5 columns
QString US_AnalysisBase2::table_row( const QString& s1, const QString& s2, 
                                     const QString& s3, const QString& s4, 
                                     const QString& s5 ) const
{
   return ( indent( 6 ) + "<tr><td>" + s1 + "</td><td>" + s2 + "</td><td>"
            + s3 + "</td><td>" + s4 + "</td><td>" + s5 + "</td></tr>\n" );
}

// Compose HTML header string
QString US_AnalysisBase2::html_header( const QString& title,
      const QString& head1, US_DataIO2::EditedData* edata ) const
{
   QString ss = QString( "<?xml version=\"1.0\"?>\n" );
   ss  += "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
   ss  += "                      \"http://www.w3.org/TR/xhtml1/DTD"
          "/xhtml1-strict.dtd\">\n";
   ss  += "<html xmlns=\"http://www.w3.org/1999/xhtml\""
          " xml:lang=\"en\" lang=\"en\">\n";
   ss  += "  <head>\n";
   ss  += "    <title> " + title + " </title>\n";
   ss  += "    <meta http-equiv=\"Content-Type\" content="
          "\"text/html; charset=iso-8859-1\"/>\n";
   ss  += "    <style type=\"text/css\" >\n";
   ss  += "      td { padding-right: 1em; }\n";
   ss  += "      body { background-color: white; }\n";
   ss  += "    </style>\n";
   ss  += "  </head>\n  <body>\n";
   ss  += "    <h1>" + head1 + "</h1>\n";
   ss  += indent( 4 ) + tr( "<h2>Data Report for Run \"" ) + edata->runID;
   ss  += "\",<br/>\n" + indent( 4 ) + "&nbsp;" + tr( " Cell " ) + edata->cell;
   ss  += tr( ", Channel " ) + edata->channel;
   ss  += tr( ", Wavelength " ) + edata->wavelength;
   ss  += ",<br/>\n" + indent( 4 ) + "&nbsp;" + tr( " Edited Dataset " );
   ss  += edata->editID + "</h2>\n";
 
   return ss;
}

QString US_AnalysisBase2::run_details( void ) const
{
   int                           index  = lw_triples->currentRow();
   const US_DataIO2::EditedData* d      = &dataList[ index ];

   QString s = "\n" + indent( 4 )
        + tr( "<h3>Detailed Run Information:</h3>\n" )
        + indent( 4 ) + "<table>\n"
        + table_row( tr( "Cell Description:" ), d->description )
        + table_row( tr( "Data Directory:"   ), directory )
        + table_row( tr( "Rotor Speed:"      ),  
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

   s += table_row( tr( "Average Temperature:" ), average + " " + MLDEGC );

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

   int    rrx   =  d->x.size() - 1;
   double left  =  d->x[ 0   ].radius;
   double right =  d->x[ rrx ].radius;

   s += table_row( tr( "Edited Data starts at: " ), 
                   QString::number( left,  'f', 3 ) + " cm" ) +
        table_row( tr( "Edited Data stops at:  " ), 
                   QString::number( right, 'f', 3 ) + " cm" );
   s += indent( 4 ) + "</table>\n";
   return s;
}

QString US_AnalysisBase2::hydrodynamics( void ) const
{
   // Set up hydrodynamics values
   US_Math2::SolutionData solution = this->solution;
   solution.vbar20    = le_vbar     ->text().toDouble();
   solution.density   = le_density  ->text().toDouble();
   solution.viscosity = le_viscosity->text().toDouble();
   double avgTemp     = le_temp     ->text().section( " ", 0, 0 ).toDouble();
   solution.vbar      = US_Math2::calcCommonVbar( (US_Solution&)solution_rec,
                                                  avgTemp );
   US_Math2::data_correction( avgTemp, solution );

   QString s = "\n" + indent( 4 ) + tr( "<h3>Hydrodynamic Settings:</h3>\n" )
               + indent( 4 ) + "<table>\n";
  
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
        table_row( tr( "Vbar corrected for 20 " ) + MLDEGC + ":",
                   QString::number( solution.vbar20, 'f', 4 ) + " ccm/g" ) +
        table_row( tr( "Buoyancy (Water, 20 " ) + MLDEGC + "): ",
                   QString::number( solution.buoyancyw, 'f', 6 ) ) +
        table_row( tr( "Buoyancy (absolute)" ),
                   QString::number( solution.buoyancyb, 'f', 6 ) ) +
        table_row( tr( "Correction Factor (s):" ),
                   QString::number( solution.s20w_correction, 'f', 6 ) ) + 
        table_row( tr( "Correction Factor (D):" ),
                   QString::number( solution.D20w_correction, 'f', 6 ) ) + 
        indent( 4 ) + "</table>\n";

   return s;
}

QString US_AnalysisBase2::analysis( const QString& extra ) const
{
   QString s = "\n" + indent( 4 ) + tr( "<h3>Data Analysis Settings:</h3>\n" )
               + indent( 4 ) + "<table>\n";

   s += table_row( tr( "Smoothing Frame:" ),
                   QString::number( (int)ct_smoothing->value() ) );
   s += table_row( tr( "Analyzed Boundary:" ),
                   QString::number( (int)ct_boundaryPercent->value() ) + " %" );
   s += table_row( tr( "Boundary Position:" ),
                   QString::number( (int)ct_boundaryPos->value() ) + " %" );
   s += table_row( tr( "Early Scans skipped:" ),
                   le_skipped->text() + " scans" );
   s += extra;
   
   s += indent( 4 ) + "</table>\n";

   return s;
}

QString US_AnalysisBase2::scan_info( void ) const
{
   int                           index  = lw_triples->currentRow();
   const US_DataIO2::EditedData* d      = &dataList[ index ];

   QString s = "\n" + indent( 4 ) + tr( "<h3>Scan Information:</h3>\n" )
               + indent( 4 ) + "<table>\n"; 
         
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

   s += indent( 4 ) + "</table>\n";
   
   return s;
}

void US_AnalysisBase2::write_plot( const QString& fname, const QwtPlot* plot )
{
    QSvgGenerator generator;
    // Set resolution to screen resolution
    double px  = (double)qApp->desktop()->width();
    double in  = (double)qApp->desktop()->widthMM() / 25.4;
    int    res = qRound( px / in );
    int    pw  = plot->width()  + res;
    int    ph  = plot->height() + res;
qDebug() << "write_plot res" << res;

    generator.setResolution( res );
    generator.setFileName( fname );
    generator.setSize( plot->size() );
    generator.setViewBox( QRect( QPoint( 0, 0 ), QPoint( pw, ph ) ) );
    plot->print( generator );
}

bool US_AnalysisBase2::mkdir( const QString& baseDir, const QString& subdir )
{
   QDir folder( baseDir );

   if ( folder.exists( subdir ) ) return true;
           
   if ( folder.mkdir( subdir ) ) return true;
   
   QMessageBox::warning( this,
      tr( "File error" ),
      tr( "Could not create the directory:\n" ) + baseDir + "/" + subdir );
   
   return false;
}

// Use dialogs to alert user to change in experiment buffer
bool US_AnalysisBase2::verify_buffer( void )
{
   bool changed = true;

   if ( buffLoaded )
   {  // Only need verify buffer change while experiment values are loaded
      if ( QMessageBox::No == QMessageBox::warning( this,
               tr( "Warning" ),
               tr( "Attention:\n"
                   "You are attempting to override buffer parameters\n"
                   "that have been set from the experimental data!\n\n"
                   "Do you really want to override them?" ),
               QMessageBox::Yes, QMessageBox::No ) )
      {  // "No":  retain loaded values, mark unchanged
         QMessageBox::information( this,
            tr( "Buffer Retained" ),
            tr( "Buffer parameters from the experiment will be retained" ) );
         changed = false;
      }

      else
      {  // "Yes":  change values,  mark experiment values no longer used
         QMessageBox::information( this,
            tr( "Buffer Overridden" ),
            tr( "Buffer parameters from the experiment will be overridden" ) );
         buffLoaded = false;
      }
   }

   qApp->processEvents();
   return changed;
}

// Slot to respond to text box change to buffer parameter
void US_AnalysisBase2::buffer_text( void )
{
   if ( buffLoaded )
   {  // Only need verify desire to change while experiment values are loaded
      bool changed = verify_buffer();
      buffLoaded   = false;
      le_skipped->setFocus( Qt::OtherFocusReason );

      if ( changed )
      {  // "Yes" to change: use values as entered and leave loaded flag off
         density      = le_density  ->text().toDouble();
         viscosity    = le_viscosity->text().toDouble();
      }

      else
      {  // "No" to change:  restore text and insure loaded flag turned on
         buffLoaded   = false;
         le_density  ->setText( QString::number( density,   'f', 6 ) );
         le_viscosity->setText( QString::number( viscosity, 'f', 5 ) );
         qApp->processEvents();
         buffLoaded   = true;
      }
   }

   if ( dataLoaded )
      data_plot();
}

// Use dialogs to alert user to change in experiment solution common vbar
bool US_AnalysisBase2::verify_vbar( void )
{
   bool changed = true;

   if ( buffLoaded )
   {  // Only need verify vbar change while experiment values are loaded
      if ( QMessageBox::No == QMessageBox::warning( this,
               tr( "Warning" ),
               tr( "Attention:\n"
                   "You are attempting to override the vbar parameter\n"
                   "that has been set from the experimental data!\n\n"
                   "Do you really want to override it?" ),
               QMessageBox::Yes, QMessageBox::No ) )
      {  // "No":  retain loaded values, mark unchanged
         QMessageBox::information( this,
            tr( "Vbar Retained" ),
            tr( "Vbar parameter from the experiment will be retained" ) );
         changed    = false;
      }

      else
      {  // "Yes":  change values,  mark experiment values no longer used
         QMessageBox::information( this,
            tr( "Vbar Overridden" ),
            tr( "Vbar parameter from the experiment will be overridden" ) );
         buffLoaded = false;
      }
   }

   qApp->processEvents();
   return changed;
}

// Slot to respond to text box change to vbar parameter
void US_AnalysisBase2::vbar_text( void )
{
   if ( buffLoaded )
   {  // Only need verify desire to change while experiment values are loaded
      bool changed = verify_vbar();
      buffLoaded   = false;

      if ( changed )
      {  // "Yes" to change: use value as entered and leave loaded flag off
         vbar = le_vbar->text().toDouble();
      }

      else
      {  // "No" to change:  restore text and insure loaded flag still on
         le_vbar->setText( QString::number( vbar, 'f', 4 ) );
         qApp->processEvents();
         buffLoaded = true;
      }
   }

   if ( dataLoaded ) data_plot();
}

// Slot to give load-data progress feedback
void US_AnalysisBase2::set_progress( const QString& message )
{
   te_desc->setText( "<b>" + message + " ...</b>" );
   qApp->processEvents();
}

// Load noise record(s) if there are any and user so chooses, then apply
void US_AnalysisBase2::load_noise( int index )
{
   US_DataIO2::EditedData* edata = &dataList[ index ];

   if ( noiflags[ index ] >= 0 )
      return;             // Do nothing if noise already applied, this triple

   noiflags[ index ] = 0; // Initially flag no noise to subtract

   QStringList mieGUIDs;  // List of GUIDs of models-in-edit
   QStringList nieGUIDs;  // List of GUIDS:type:index of noises-in-edit

   US_LoadableNoise lnoise;
   bool loadDB = disk_controls->db();
   bool local  = ! loadDB;
   int  nenois = lnoise.count_noise( local, edata, NULL, mieGUIDs, nieGUIDs );

for (int jj=0;jj<nenois;jj++)
 qDebug() << " jj nieG" << jj << nieGUIDs.at(jj);

   if ( nenois > 0 )
   {  // There is/are noise(s):  ask user if she wants to load
      US_Passwd pw;
      US_DB2* dbP  = local ? NULL : new US_DB2( pw.getPasswd() );

      if ( nenois > 1 )
      {  // more than 1:  get choice from noise loader dialog
         US_NoiseLoader* nldiag = new US_NoiseLoader( dbP,
            mieGUIDs, nieGUIDs, ti_noise, ri_noise );
         nldiag->move( this->pos() + QPoint( 200, 200 ) );
         nldiag->exec();
         qApp->processEvents();

         delete nldiag;
      }

      else
      {  // only 1:  just load it
         QString noiID = nieGUIDs.at( 0 );
         QString typen = noiID.section( ":", 1, 1 );
         noiID         = noiID.section( ":", 0, 0 );

         if ( typen == "ti" )
            ti_noise.load( loadDB, noiID, dbP );

         else
            ri_noise.load( loadDB, noiID, dbP );
      }

      // noise loaded:  insure that counts jive with data
      int ntinois = ti_noise.values.size();
      int nrinois = ri_noise.values.size();
      int nscans  = edata->scanData.size();
      int npoints = edata->x.size();
      int npadded = 0;

      if ( ntinois > 0  &&  ntinois < npoints )
      {  // pad out ti noise values to radius count
         int jj      = ntinois;
         while ( jj++ < npoints )
            ti_noise.values << 0.0;
         ti_noise.count = ti_noise.values.size();
         npadded++;
      }

      if ( nrinois > 0  &&  nrinois < nscans )
      {  // pad out ri noise values to scan count
         int jj      = nrinois;
         while ( jj++ < nscans )
            ri_noise.values << 0.0;
         ri_noise.count = ri_noise.values.size();
         npadded++;
      }

      if ( npadded  > 0 )
      {  // let user know that padding occurred
         QString pmsg;

         if ( npadded == 1 )
            pmsg = tr( "The noise file was padded out with zeroes\n"
                       "in order to match the data range." );
         else
            pmsg = tr( "The noise files were padded out with zeroes\n"
                       "in order to match the data ranges." );

         QMessageBox::information( this, tr( "Noise Padded Out" ), pmsg );
      }

      // Subtract noise from the experiment

      for ( int ii = 0; ii < nscans; ii++ )
      {
         int    iin    = min( ii, ( nrinois - 1 ) );
         double rinoi  = ( nrinois > 0 ) ? ri_noise.values[ iin ] : 0.0;
         US_DataIO2::Scan* escan = &edata->scanData[ ii ];

         for ( int jj = 0; jj < npoints; jj++ )
         {
            int    jjn    = min( jj, ( ntinois - 1 ) );
            double tinoi  = ( ntinois > 0 ) ? ti_noise.values[ jjn ] : 0.0;

            escan->readings[ jj ] =
               US_DataIO2::Reading( edata->value( ii, jj ) - rinoi - tinoi );
         }
      }

      // Keep track of noise applied to this triple
      noiflags[ index ] = min( nrinois, 1 ) + 2 * min( ntinois, 1 );

   }
}

