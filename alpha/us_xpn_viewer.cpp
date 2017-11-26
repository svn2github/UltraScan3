#include <QApplication>
#include <QtSql>

#include "us_xpn_viewer.h"
#include "us_xpn_run_auc.h"
#include "us_xpn_run_raw.h"
#include "us_tmst_plot.h"
#include "us_license_t.h"
#include "us_license.h"
#include "us_util.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_plot.h"
#include "us_math2.h"
#include "us_db2.h"
#include "us_passwd.h"
#include "us_investigator.h"
#include "us_constants.h"
#include "us_report.h"
#include "us_gui_util.h"
#include "us_util.h"
#include "us_crypto.h"
#include "us_editor.h"
#include "us_images.h"
#include "us_colorgradIO.h"

#if QT_VERSION < 0x050000
#define setSamples(a,b,c)  setData(a,b,c)
#define setMinimum(a)      setMinValue(a)
#define setMaximum(a)      setMaxValue(a)
#define setSymbol(a)       setSymbol(*a)
#define AXISSCALEDIV(a)    data_plot->axisScaleDiv(a)
#define dPlotClearAll(a) a->clear()
#else
#include "qwt_picker_machine.h"
#define AXISSCALEDIV(a)    (QwtScaleDiv*)&data_plot->axisScaleDiv(a)
#define dPlotClearAll(a) a->detachItems(QwtPlotItem::Rtti_PlotItem,true)
#endif

#ifdef WIN32
#include <float.h>
#define isnan _isnan
#endif

#ifndef DbgLv
#define DbgLv(a) if(dbg_level>=a)qDebug()
#endif

int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );

   #include "main1.inc"

   // License is OK.  Start up.
   
   US_XpnDataViewer ww;
   ww.show();                  //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

US_XpnDataViewer::US_XpnDataViewer() : US_Widgets()
{
   const QChar chlamb( 955 );

   setWindowTitle( tr( "Beckman Optima Data Viewer" ) );
   setPalette( US_GuiSettings::frameColor() );

   QGridLayout* settings = new QGridLayout;

   navgrec      = 10;
   dbg_level    = US_Settings::us_debug();
   QFont sfont( US_GuiSettings::fontFamily(), US_GuiSettings::fontSize() - 1 );
   QFontMetrics fmet( sfont );
   int fwid     = fmet.maxWidth();
   int lwid     = fwid * 4;
   int swid     = lwid + fwid;
   isMWL        = false;
   isRaw        = true;
   haveData     = false;
   haveTmst     = false;
   xpn_data     = NULL;
   runID        = "";
   runType      = "RI";
   rlt_id       = 0;
   currentDir   = "";
   in_reload    = false;
   QStringList xpnentr = US_Settings::defaultXpnHost();
DbgLv(1) << "xpnentr count" << xpnentr.count();

   if ( xpnentr.count() == 0 )
   {
      xpnentr << "test-host" << "bcf.uthscsa.edu" << "5432";

      QMessageBox::warning( this,
            tr( "No Optima Host Entry" ),
            tr( "A default Optima Host entry is being used.\n"
                "You should add entries via Preferences:Optima Host Preferences\n"
                "as soon as possible" ) );
   }
else
 DbgLv(1) << "xpnentr ..." << xpnentr;

   QString encpw;
   QString decpw;
   QString encpw0;
   QString encpw1;
   QString masterpw;
   US_Passwd pw;
   xpndesc      = xpnentr.at( 0 );
   xpnhost      = xpnentr.at( 1 );
   xpnport      = xpnentr.at( 2 );
   xpnname      = xpnentr.at( 3 );
   xpnuser      = xpnentr.at( 4 );
   encpw        = xpnentr.at( 5 );
   encpw0       = encpw.section( "^", 0, 0 );
   encpw1       = encpw.section( "^", 1, 1 );
   masterpw     = pw.getPasswd();
   xpnpasw      = US_Crypto::decrypt( encpw0, masterpw, encpw1 );

   // Load controls     
   QLabel*      lb_run      = us_banner( tr( "Load the Run" ) );

                pb_loadXpn  = us_pushbutton( tr( "Load Raw Optima Data" ) );
                pb_loadAUC  = us_pushbutton( tr( "Load US3 AUC Data" ) );
                pb_reset    = us_pushbutton( tr( "Reset Data" ) );
                pb_details  = us_pushbutton( tr( "Data Details" ), true  );
                pb_plot2d   = us_pushbutton( tr( "Refresh Plot" ) );
                pb_saveauc  = us_pushbutton( tr( "Export openAUC" )  );
                pb_showtmst = us_pushbutton( tr( "Show Time State" )  );
                pb_reload   = us_pushbutton( tr( "Update Data" )      );
                pb_colmap   = us_pushbutton( tr( "Color Map" )        );

   QLabel*      lb_dir      = us_label( tr( "Directory" ), -1 );
                le_dir      = us_lineedit( "", -1, true );

   QLabel*      lb_dbhost   = us_label( tr( "DB Host" ), -1 );
                le_dbhost   = us_lineedit( "", -1, true );

   QLabel*      lb_runID    = us_label( tr( "Run ID:" ), -1 );
                le_runID    = us_lineedit( "", -1, false );

   QLabel*      lb_cellchn  = us_label( tr( "Cell/Channel:" ), -1 );
                cb_cellchn  = us_comboBox();

                le_colmap   = us_lineedit( "cm-rainbow", -1, true );

   int rhgt     = le_runID->height();
   ptype_mw     = tr( "Plot %1:"    ).arg( chlamb );
   ptype_tr     = tr( "Plot Triple:" );
   prectype     = ptype_tr;

   // Plot controls     
   QLabel*      lb_prcntls  = us_banner( tr( "Plot Controls" ) );
   QLabel*      lb_rstart   = us_label( tr( "Radius Start:"   ), -1 );
                cb_rstart   = us_comboBox();
   QLabel*      lb_rend     = us_label( tr( "Radius End:"     ), -1 );
                cb_rend     = us_comboBox();
   QLabel*      lb_lrange   = us_label( tr( "%1 Range:"   ).arg( chlamb ), -1 );
                le_lrange   = us_lineedit( "280 only", -1, true );
                lb_pltrec   = us_label( prectype, -1 );
                cb_pltrec   = us_comboBox();
   QLabel*      lb_optsys   = us_label( tr( "Optical System:" ), -1 );
                cb_optsys   = us_comboBox();

                pb_prev     = us_pushbutton( tr( "Previous" ) );
                pb_next     = us_pushbutton( tr( "Next" ) );
   us_checkbox( tr( "Always Auto-scale Y Axis" ), ck_autoscy, true );
   us_checkbox( tr( "Auto Update" ),              ck_autorld, false );
   pb_prev->setIcon( US_Images::getIcon( US_Images::ARROW_LEFT  ) );
   pb_next->setIcon( US_Images::getIcon( US_Images::ARROW_RIGHT ) );
   QLabel*      lb_rinterv  = us_label( tr( "Update Interval Seconds:" ), -1 );
                ct_rinterv  = us_counter( 2, 10, 3600, 1 );
   ct_rinterv->setFont( sfont );
   ct_rinterv->setMinimumWidth( lwid );
   ct_rinterv->resize( rhgt, swid );
   ct_rinterv->setMinimum   (   10 );
   ct_rinterv->setMaximum   ( 3600 );
   ct_rinterv->setValue     (   60 );
   ct_rinterv->setSingleStep(    1 );

   // Scan controls     
   QLabel*      lb_scanctl  = us_banner( tr( "Scan Control" ) );
   QLabel*      lb_from     = us_label( tr( "From:" ) );
   QLabel*      lb_to       = us_label( tr( "To:" ) );
                ct_from     = us_counter( 2, 0, 500, 1 );
                ct_to       = us_counter( 2, 0, 500, 1 );
                pb_exclude  = us_pushbutton( tr( "Exclude Scan Range" ) );
                pb_include  = us_pushbutton( tr( "Include All Scans"  ) );
   ct_from  ->setFont( sfont );
   ct_from  ->setMinimumWidth( lwid );
   ct_from  ->resize( rhgt, swid );
   ct_to    ->setFont( sfont );
   ct_to    ->setMinimumWidth( lwid );
   ct_to    ->resize( rhgt, swid );
   ct_from  ->setValue( 0 );
   ct_to    ->setValue( 0 );
   ct_from  ->setSingleStep( 1 );
   ct_to    ->setSingleStep( 1 );

   // Status and standard pushbuttons
   QLabel*      lb_status   = us_banner( tr( "Status" ) );
                le_status   = us_lineedit( tr( "(no data loaded)" ), -1, true );
   QPalette stpal;
   stpal.setColor( QPalette::Text, Qt::white );
   stpal.setColor( QPalette::Base, Qt::blue  );
   le_status->setPalette( stpal );

   QPushButton* pb_help     = us_pushbutton( tr( "Help" ) );
   QPushButton* pb_close    = us_pushbutton( tr( "Close" ) );

   // Default scan curve color list and count
   QString cmfpath          = US_Settings::etcDir() + "/cm-rainbow.xml";
 DbgLv(1) << "cmfpath" << cmfpath;
   US_ColorGradIO::read_color_gradient( cmfpath, mcolors );
   mcknt                    = mcolors.count();
DbgLv(1) << "mcolors count" << mcknt;
if(mcknt>0)
 DbgLv(1) << "mcolors c0,cn" << mcolors[0] << mcolors[mcknt-1];

   // Signals and Slots
   connect( pb_loadXpn,   SIGNAL( clicked()      ),
            this,         SLOT  ( load_xpn_raw() ) );
   connect( pb_loadAUC,   SIGNAL( clicked()      ),
            this,         SLOT  ( load_auc_xpn() ) );
   connect( pb_reset,     SIGNAL( clicked()      ),
            this,         SLOT  ( resetAll()     ) );
   connect( pb_details,   SIGNAL( clicked()      ),
            this,         SLOT  ( runDetails()   ) );
   connect( pb_saveauc,   SIGNAL( clicked()      ),
            this,         SLOT  ( export_auc()   ) );
   connect( pb_reload,    SIGNAL( clicked()      ),
            this,         SLOT  ( reloadData()   ) );
   connect( ck_autorld,   SIGNAL( clicked()      ),
            this,         SLOT  ( changeReload()             ) );
   connect( cb_cellchn,   SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeCellCh( )            ) );
   connect( cb_rstart,    SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeRadius( )            ) );
   connect( cb_rend,      SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeRadius( )            ) );
   connect( cb_pltrec,    SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeRecord( )            ) );
   connect( pb_prev,      SIGNAL( clicked()  ),
            this,         SLOT  ( prevPlot() ) );
   connect( pb_next,      SIGNAL( clicked()  ),
            this,         SLOT  ( nextPlot() ) );
   connect( ct_from,      SIGNAL( valueChanged( double ) ),
            this,         SLOT  ( exclude_from( double ) ) );
   connect( ct_to,        SIGNAL( valueChanged( double ) ),
            this,         SLOT  ( exclude_to  ( double ) ) );
   connect( pb_exclude,   SIGNAL( clicked()       ),
            this,         SLOT  ( exclude_scans() ) );
   connect( pb_include,   SIGNAL( clicked()       ),
            this,         SLOT  ( include_scans() ) );
   connect( pb_plot2d,    SIGNAL( clicked()       ),
            this,         SLOT  ( changeCellCh()  ) );
   connect( pb_showtmst,  SIGNAL( clicked()       ),
            this,         SLOT  ( showTimeState() ) );
   connect( pb_colmap,    SIGNAL( clicked()        ),
            this,         SLOT  ( selectColorMap() ) );
   connect( ct_rinterv,   SIGNAL( valueChanged( double ) ),
            this,         SLOT  ( changeInterval()       ) );
   connect( pb_help,      SIGNAL( clicked()  ),
            this,         SLOT  ( help()     ) );
   connect( pb_close,     SIGNAL( clicked()  ),
            this,         SLOT  ( close()    ) );

   // Do the left-side layout
   int row = 0;
   settings->addWidget( lb_run,        row++, 0, 1, 8 );
   settings->addWidget( lb_dir,        row++, 0, 1, 8 );
   settings->addWidget( le_dir,        row++, 0, 1, 8 );
   settings->addWidget( lb_dbhost,     row,   0, 1, 2 );
   settings->addWidget( le_dbhost,     row++, 2, 1, 6 );
   settings->addWidget( lb_runID,      row,   0, 1, 2 );
   settings->addWidget( le_runID,      row++, 2, 1, 6 );
   settings->addWidget( pb_loadXpn,    row,   0, 1, 4 );
   settings->addWidget( pb_loadAUC,    row++, 4, 1, 4 );
   settings->addWidget( pb_reset,      row,   0, 1, 4 );
   settings->addWidget( pb_details,    row++, 4, 1, 4 );
   settings->addWidget( pb_plot2d,     row,   0, 1, 4 );
   settings->addWidget( pb_saveauc,    row++, 4, 1, 4 );
   settings->addWidget( pb_reload,     row,   0, 1, 4 );
   settings->addWidget( ck_autorld,    row++, 4, 1, 4 );
   settings->addWidget( lb_rinterv,    row,   0, 1, 4 );
   settings->addWidget( ct_rinterv,    row++, 4, 1, 4 );
   settings->addWidget( lb_prcntls,    row++, 0, 1, 8 );
   settings->addWidget( lb_rstart,     row,   0, 1, 2 );
   settings->addWidget( cb_rstart,     row,   2, 1, 2 );
   settings->addWidget( lb_rend,       row,   4, 1, 2 );
   settings->addWidget( cb_rend,       row++, 6, 1, 2 );
   settings->addWidget( lb_optsys,     row,   0, 1, 4 );
   settings->addWidget( cb_optsys,     row++, 4, 1, 4 );
   settings->addWidget( lb_cellchn,    row,   0, 1, 2 );
   settings->addWidget( cb_cellchn,    row,   2, 1, 2 );
   settings->addWidget( lb_lrange,     row,   4, 1, 2 );
   settings->addWidget( le_lrange,     row++, 6, 1, 2 );
   settings->addWidget( lb_pltrec,     row,   0, 1, 2 );
   settings->addWidget( cb_pltrec,     row,   2, 1, 2 );
   settings->addWidget( pb_prev,       row,   4, 1, 2 );
   settings->addWidget( pb_next,       row++, 6, 1, 2 );
   settings->addWidget( ck_autoscy,    row,   0, 1, 4 );
   settings->addWidget( pb_showtmst,   row++, 4, 1, 4 );
   settings->addWidget( pb_colmap,     row,   0, 1, 2 );
   settings->addWidget( le_colmap,     row++, 2, 1, 6 );
   settings->addWidget( lb_scanctl,    row++, 0, 1, 8 );
   settings->addWidget( lb_from,       row,   0, 1, 1 );
   settings->addWidget( ct_from,       row,   1, 1, 3 );
   settings->addWidget( lb_to,         row,   4, 1, 1 );
   settings->addWidget( ct_to,         row++, 5, 1, 3 );
   settings->addWidget( pb_exclude,    row,   0, 1, 4 );
   settings->addWidget( pb_include,    row++, 4, 1, 4 );
   settings->addWidget( lb_status,     row++, 0, 1, 8 );
   settings->addWidget( le_status,     row++, 0, 1, 8 );
   settings->addWidget( pb_help,       row,   0, 1, 4 );
   settings->addWidget( pb_close,      row++, 4, 1, 4 );

   // Plot layout for the right side of window
//   QBoxLayout* plot = new US_Plot( data_plot,
   plot             = new US_Plot( data_plot,
                                   tr( "Intensity Data" ),
                                   tr( "Radius (in cm)" ), 
                                   tr( "Intensity" ) );

   data_plot->setMinimumSize( 600, 400 );

   data_plot->enableAxis( QwtPlot::xBottom, true );
   data_plot->enableAxis( QwtPlot::yLeft  , true );

   data_plot->setAxisScale( QwtPlot::xBottom, 5.8,  7.2 );
   data_plot->setAxisScale( QwtPlot::yLeft  , 0.0, 5e+4 );

   picker = new US_PlotPicker( data_plot );
   picker->setRubberBand     ( QwtPicker::VLineRubberBand );
   picker->setMousePattern   ( QwtEventPattern::MouseSelect1,
                               Qt::LeftButton, Qt::ControlModifier );

   connect( plot, SIGNAL( zoomedCorners( QRectF ) ),
            this, SLOT  ( currentRectf ( QRectF ) ) );

   // Now let's assemble the page
   
   QVBoxLayout* left     = new QVBoxLayout;

   left->addLayout( settings );
   
   QVBoxLayout* right    = new QVBoxLayout;
   
   right->addLayout( plot );

   QHBoxLayout* main = new QHBoxLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   main->addLayout( left );
   main->addLayout( right );

   main->setStretch( 0, 2 );
   main->setStretch( 1, 4 );

   reset();
   adjustSize();
}

void US_XpnDataViewer::reset( void )
{
   runID         = "";
   currentDir    = US_Settings::importDir() + "/" + runID;
   cb_cellchn ->disconnect();
   cb_cellchn ->clear();
   le_dir     ->setText( currentDir );
   le_runID   ->setText( runID );
   le_dbhost  ->setText( xpnhost + ":" + xpnport + "   (" + xpndesc + ")" );

   pb_loadXpn ->setEnabled( true );
   pb_loadAUC ->setEnabled( true );
   pb_details ->setEnabled( false );
   pb_reload  ->setEnabled( false );
   ck_autorld ->setEnabled( true  );
   cb_cellchn ->setEnabled( false );
   cb_rstart  ->setEnabled( false );
   cb_rend    ->setEnabled( false );
   cb_pltrec  ->setEnabled( false );
   pb_prev    ->setEnabled( false );
   pb_next    ->setEnabled( false );
   ct_from    ->setEnabled( false );
   ct_to      ->setEnabled( false );
   pb_exclude ->setEnabled( false );
   pb_include ->setEnabled( false );
   pb_reset   ->setEnabled( false );
   ct_from    ->setEnabled( false );
   ct_to      ->setEnabled( false );
   pb_exclude ->setEnabled( false );
   pb_include ->setEnabled( false );
   pb_plot2d  ->setEnabled( false );
   pb_saveauc ->setEnabled( false );
   pb_showtmst->setEnabled( false );
//   pb_movie2d->setEnabled( false );

   // Clear any data structures
   allData   .clear();
   lambdas   .clear();
   r_radii   .clear();
   excludes  .clear();
   runInfo   .clear();
   cellchans .clear();
   triples   .clear();
   haveData      = false;

   dPlotClearAll( data_plot );
   picker   ->disconnect();
   data_plot->setAxisScale( QwtPlot::xBottom, 5.8,  7.2 );
   data_plot->setAxisScale( QwtPlot::yLeft  , 0.0, 5e+4 );
   grid          = us_grid( data_plot );
   data_plot->replot();
   connect( cb_cellchn,   SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeCellCh(            ) ) );
//   connect( plot, SIGNAL( zoomedCorners( QRectF ) ),
//            this, SLOT  ( currentRectf ( QRectF ) ) );

   last_xmin     = -1.0;
   last_xmax     = -1.0;
   last_ymin     = -1.0;
   last_ymax     = -1.0;
   xpn_data      = ( xpn_data == NULL ) ? new US_XpnData() : xpn_data;

   connect( xpn_data, SIGNAL( status_text  ( QString ) ),
            this,     SLOT  ( status_report( QString ) ) );

   xpn_data->clear();
   le_status->setText( tr( "(no data loaded)" ) );

}

void US_XpnDataViewer::resetAll( void )
{
   if ( allData.size() > 0 )
   {
      int status = QMessageBox::information( this,
               tr( "New Data Warning" ),
               tr( "This will erase all data currently on the screen, and " 
                   "reset the program to its starting condition. No hard-drive "
                   "data or database information will be affected. Proceed? " ),
               tr( "&OK" ), tr( "&Cancel" ),
               0, 0, 1 );
      if ( status != 0 ) return;
   }

   reset();

   runID           = "";
   data_plot->setTitle( tr( "Intensity Data" ) );
}


// Enable the common dialog controls based on the presence of data
void US_XpnDataViewer::enableControls( void )
{
   const QChar chlamb( 955 );

   if ( allData.size() == 0 )
   {  // If no data yet, just reset
      reset();
      return;
   }

   // Enable and disable controls now
   pb_loadXpn ->setEnabled( false );
   pb_loadAUC ->setEnabled( false );
   pb_reset   ->setEnabled( true );
   pb_details ->setEnabled( true );
   pb_reload  ->setEnabled( true );
   cb_cellchn ->setEnabled( true );
   cb_rstart  ->setEnabled( true );
   cb_rend    ->setEnabled( true );
   cb_pltrec  ->setEnabled( true );
   pb_prev    ->setEnabled( true );
   pb_next    ->setEnabled( true );
   pb_plot2d  ->setEnabled( true );
   pb_saveauc ->setEnabled( isRaw );
   pb_showtmst->setEnabled( haveTmst );
//   pb_movie2d->setEnabled( true );
   ct_from    ->setEnabled( true );
   ct_to      ->setEnabled( true );
   pb_exclude ->setEnabled( true );

   ncellch     = cellchans.count();
   nlambda     = lambdas  .count();
   ntriple     = triples  .count();
   nscan       = allData[ 0 ].scanCount();
   npoint      = allData[ 0 ].pointCount();
   ntpoint     = nscan * npoint;
   isMWL       = ( nlambda > 2 );
   if ( isMWL )
   {  // If apparently MWL, test for all wavelengths each channel
      int ktrip   = ncellch * nlambda;
      if ( ntriple != ktrip )
      {  // Possibly to be treated on triple basis (not MWL)
         if ( ntriple <= ( nlambda * 2 )  ||  ntriple < 16 )
         {
            isMWL       = false;
            cb_cellchn ->setEnabled( false );
         }
      }
   }
DbgLv(1) << "ec: ncc nwl nsc npt ntpt" << ncellch << nlambda << nscan
 << npoint << ntpoint << "Mwl" << isMWL;
   QStringList slrads;
   QStringList sllmbs;
   QStringList plrecs;
DbgLv(1) << "ec: npoint" << npoint << "radsize" << r_radii.count();
DbgLv(1) << "ec: nlambda" << nlambda << "lmbsize" << lambdas.count();
DbgLv(1) << "ec: ntriple" << ntriple << "trpsize" << triples.count();

   for ( int jj = 0; jj < npoint; jj++ )
      slrads << QString().sprintf( "%.3f", r_radii[ jj ] );

   for ( int jj = 0; jj < nlambda; jj++ )
      sllmbs << QString::number( lambdas[ jj ] );

   if ( isMWL )
   {
      prectype    = ptype_mw;
      plrecs      = sllmbs;
   }
   else
   {
      prectype    = ptype_tr;
      for ( int jj = 0; jj < ntriple; jj++ )
         plrecs << QString( triples[ jj ] ).replace( " ", "" );
   }

   lb_pltrec->setText( prectype );
   connect_ranges( false );
   cb_cellchn->clear();
   cb_rstart ->clear();
   cb_rend   ->clear();
   cb_pltrec ->clear();

   cb_cellchn->addItems( cellchans );
   cb_rstart ->addItems( slrads );
   cb_rend   ->addItems( slrads );
   cb_pltrec ->addItems( plrecs );

   if ( nlambda == 1 )
      le_lrange ->setText( sllmbs[ 0 ] + tr( " only" ) );
   else if ( nlambda > 1 )
      le_lrange ->setText( sllmbs[ 0 ] + tr( " to " ) 
                         + sllmbs[ nlambda - 1 ] );

   cb_cellchn->setCurrentIndex( 0 );
   cb_rstart ->setCurrentIndex( 0 );
   cb_rend   ->setCurrentIndex( npoint - 1 );
   connect_ranges( true );

   have_rngs  = false;
   compute_ranges( );

   ct_from   ->setMaximum( nscan );
   ct_to     ->setMaximum( nscan );
   cb_pltrec ->setCurrentIndex( nlambda / 2 );
   qApp->processEvents();

DbgLv(1) << "ec: call changeCellCh";
   changeCellCh();                          // Force a plot initialize
}

// Load Optima raw (.postgres) data
void US_XpnDataViewer::load_xpn_raw( )
{
   // Ask for data directory
   QString dbhost    = xpnhost;
   int     dbport    = xpnport.toInt();
DbgLv(1) << "RDr: call connect_data  dbname h p u w"
 << xpnname << dbhost << dbport << xpnuser << xpnpasw;
   if ( xpn_data->connect_data( dbhost, dbport, xpnname, xpnuser, xpnpasw ) )
   {
if ( dbg_level > 0 ) xpn_data->dump_tables();
      xpn_data->scan_runs( runInfo );
      xpn_data->filter_runs( runInfo );
DbgLv(1) << "RDr:  rtn fr import_data";
   }
   else
   {
DbgLv(1) << "RDr:  connection failed";
      runInfo.clear();
   }

   QString drDesc    = "";
   US_XpnRunRaw* lddiag = new US_XpnRunRaw( drDesc, runInfo );
   if ( lddiag->exec() == QDialog::Rejected )
      return;

   // Restore area beneath dialog
   qApp->processEvents();
DbgLv(1) << "RDr:  rtn fr XpnRunRaw dialog";
DbgLv(1) << "RDr:   drDesc" << drDesc;

   // See if we need to fix the runID
   QString delim     = QString( drDesc ).left( 1 );
   QString fRunId    = QString( drDesc ).section( delim, 1, 1 );
   QString fExpNm    = QString( drDesc ).section( delim, 5, 5 );
   QString new_runID = fExpNm + "-run" + fRunId;
   runType           = "RI";
   QRegExp rx( "[^A-Za-z0-9_-]" );

   int pos            = 0;

   bool runID_changed = false;
   while ( ( pos = rx.indexIn( new_runID ) ) != -1 )
   {
      new_runID.replace( pos, 1, "_" );         // Replace 1 char at pos
      runID_changed = true;
   }

   // Let the user know if the runID name has changed
   if ( runID_changed )
   {
      QMessageBox::warning( this,
            tr( "RunId Name Changed" ),
            tr( "The runId name has been changed.\nIt may consist only "
                "of alphanumeric characters,\nthe underscore, and the "
                "hyphen.\nNew runId:\n  " ) + new_runID );
   }

   // Set the runID and directory
   runID       = new_runID;
   le_runID->setText( runID );
   currentDir  = US_Settings::importDir() + "/" + runID;
   le_dir  ->setText( currentDir );
   qApp->processEvents();

   // Read the data
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor) );
   le_status->setText( tr( "Reading Raw Optima data ..." ) );
   qApp->processEvents();
QDateTime sttime=QDateTime::currentDateTime();

   int iRunId         = fRunId.toInt();
   QString sMasks     = QString( drDesc ).section( delim, 7, 10 );
   int scanmask       = QString( sMasks ).mid( 0, 1 ) == "1" ? 1 : 0;
   scanmask          += QString( sMasks ).mid( 2, 1 ) == "1" ? 2 : 0;
   scanmask          += QString( sMasks ).mid( 4, 1 ) == "1" ? 4 : 0;
   scanmask          += QString( sMasks ).mid( 6, 1 ) == "1" ? 8 : 0;
DbgLv(1) << "RDr:     iRId" << iRunId << "sMsks scnmask" << sMasks << scanmask;

   xpn_data->import_data( iRunId, scanmask );
   le_status->setText( tr( "Initial Raw Optima data import complete." ) );
   qApp->processEvents();
double tm1=(double)sttime.msecsTo(QDateTime::currentDateTime())/1000.0;
   QStringList opsys;

   // Infer and report on type of data to eventually export
   runType            = "RI";
   int optndx         = 0;

   if ( scanmask == 1  ||  scanmask == 2  ||
        scanmask == 4  ||  scanmask == 8 )
   {
      runType            = ( scanmask == 2 ) ? "FI" : runType;
      runType            = ( scanmask == 4 ) ? "IP" : runType;
      runType            = ( scanmask == 8 ) ? "WI" : runType;
      if ( scanmask == 1 )
         opsys << "Absorbance";
      else if ( scanmask == 2 )
         opsys << "Fluorescence";
      else if ( scanmask == 4 )
         opsys << "Interference";
      else if ( scanmask == 8 )
         opsys << "Wavelength";
   }

   else if ( ( scanmask & 1 ) != 0 )
   {
      QApplication::restoreOverrideCursor();
      QApplication::restoreOverrideCursor();
      QString runType2( "IP" );
      runType2           = ( ( scanmask & 2 ) != 0 ) ? "FI" : runType2;
      runType2           = ( ( scanmask & 8 ) != 0 ) ? "WI" : runType2;
      QString drtype1    = "Absorbance";
      QString drtype2    = "Interference";
      drtype1            = ( runType  == "FI" ) ? "Fluorescence" : drtype1;
      drtype1            = ( runType  == "IP" ) ? "Interference" : drtype1;
      drtype1            = ( runType  == "WI" ) ? "Wavelength"   : drtype1;
      drtype2            = ( runType2 == "RI" ) ? "Absorbance"   : drtype2;
      drtype2            = ( runType2 == "FI" ) ? "Fluorescence" : drtype2;
      drtype2            = ( runType2 == "WI" ) ? "Wavelength"   : drtype2;
      opsys << drtype1 << drtype2;

      QString msg        = tr( "Multiple scan data types are present:\n" )
                           +   "'" + drtype1 + "'\n or \n"
                           +   "'" + drtype2 + "' .\n\n"
                           + tr( "Choose one for initial display." );
      QMessageBox mbox;
      mbox.setWindowTitle( tr( "Scan Data Type to Process" ) );
      mbox.setText( msg );
      QPushButton* pb_opt1 = mbox.addButton( drtype1, QMessageBox::AcceptRole );
      QPushButton* pb_opt2 = mbox.addButton( drtype2, QMessageBox::RejectRole );
      mbox.setEscapeButton ( pb_opt2 );
      mbox.setDefaultButton( pb_opt1 );

      mbox.exec();
      if ( mbox.clickedButton() == pb_opt2 )
      {
         runType            = runType2;
         optndx             = 1;
      }
   }

   cb_optsys->disconnect();
   cb_optsys->clear();
   cb_optsys->addItems( opsys );
   cb_optsys->setCurrentIndex( optndx );
   connect( cb_optsys,    SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeOptics( )            ) );

   runID         = new_runID;
DbgLv(1) << "RDr:  runID" << runID << "runType" << runType;
   xpn_data->set_run_values( runID, runType );

   // Build the AUC equivalent
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor) );
   le_status->setText( tr( "Building AUC data ..." ) );
   qApp->processEvents();

   xpn_data->build_rawData( allData );
double tm2=(double)sttime.msecsTo(QDateTime::currentDateTime())/1000.0;
DbgLv(1) << "RDr:      build-raw done: tm1 tm2" << tm1 << tm2;

   QApplication::restoreOverrideCursor();
   QApplication::restoreOverrideCursor();
   isRaw         = true;
   haveData      = true;
   ncellch       = xpn_data->cellchannels( cellchans );
   r_radii.clear();
   r_radii << allData[ 0 ].xvalues;
   nscan         = allData[ 0 ].scanCount();
   npoint        = allData[ 0 ].pointCount();

DbgLv(1) << "RDr: mwr ntriple" << ntriple;
DbgLv(1) << "RDr: ncellch" << ncellch << cellchans.count();
DbgLv(1) << "RDr: nscan" << nscan << "npoint" << npoint;
DbgLv(1) << "RDr:   rvS rvE" << r_radii[0] << r_radii[npoint-1];
   cb_cellchn->disconnect();
   cb_cellchn->clear();
   cb_cellchn->addItems( cellchans );
   connect( cb_cellchn,   SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeCellCh(            ) ) );

   nlambda      = xpn_data->lambdas_raw( lambdas );
   int wvlo     = lambdas[ 0 ];
   int wvhi     = lambdas[ nlambda - 1 ];
#if 0
   ntriple      = nlambda * ncellch;  // Number triples
   ntpoint      = npoint  * nscan;    // Number radius points per triple
DbgLv(1) << "RDr: nwl wvlo wvhi" << nlambda << wvlo << wvhi
   << "ncellch" << ncellch << "nlambda" << nlambda << "ntriple" << ntriple;
   triples.clear();

   for ( int jj = 0; jj < ncellch; jj++ )
   {
      QString celchn  = cellchans[ jj ];

      for ( int kk = 0; kk < nlambda; kk++ )
         triples << celchn + " / " + QString::number( lambdas[ kk] );
   }
#endif
#if 1
   ntriple      = xpn_data->data_triples( triples );
DbgLv(1) << "RDr: nwl wvlo wvhi" << nlambda << wvlo << wvhi
   << "ncellch" << ncellch << "nlambda" << nlambda << "ntriple" << ntriple
   << triples.count();
#endif

DbgLv(1) << "RDr: allData size" << allData.size();
   QApplication::restoreOverrideCursor();
   QString tspath = currentDir + "/" + runID + ".time_state.tmst";
   haveTmst       = QFile( tspath ).exists();

   // Ok to enable some buttons now
   enableControls();
}

// Load US3 AUC Optima-derived data
void US_XpnDataViewer::load_auc_xpn( )
{
   int status        = 0;
   QStringList ifpaths;

   resetAll();

   QString dir       = "";
   US_XpnRunAuc lddiag( dir );
   if ( lddiag.exec() == QDialog::Rejected )
      return;
   
   // Restore area beneath dialog
   qApp->processEvents();

   if ( dir.isEmpty() ) return; 
   
   dir.replace( "\\", "/" );                 // WIN32 issue
   if ( dir.right( 1 ) != "/" ) dir += "/";  // Ensure trailing '/'
   
   // Check the runID
   QString new_runID = dir.section( "/", -2, -2 ).simplified();
DbgLv(1) << "RDa: runID" << new_runID;
DbgLv(1) << "RDa: dir" << dir;
      
   QRegExp rx( "^[A-Za-z0-9_-]{1,80}$" );
   if ( rx.indexIn( new_runID ) < 0 )
   {
      QMessageBox::warning( this,
            tr( "Bad runID Name" ),
            tr( "The runID name may consist only of alphanumeric\n"  
                "characters, the underscore, and the hyphen." ) );
      return;
   }

   // Set the runID and directory
   runID             = new_runID;
   currentDir        = dir;
   le_runID ->setText( runID );
   le_dir   ->setText( currentDir );
DbgLv(1) << "RDa: runID" << runID;
DbgLv(1) << "RDa: dir" << currentDir;

   // Error reporting 
   if ( status == US_DB2::NO_PROJECT ) 
   { 
      QMessageBox::information( this, 
            tr( "Attention" ), 
            tr( "The project was not found.\n" 
                "Please select an existing project and try again.\n" ) ); 
   } 
   
   else if ( status != US_DB2::OK ) 
   { 
      QMessageBox::information( this, 
            tr( "Disk Read Problem" ), 
            tr( "Could not read data from the disk.\n" 
                "Disk status: " ) + QString::number( status ) ); 
   }

   // Load the AUC data
   le_status->setText( tr( "Reading AUC Xpn data ..." ) );
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor) );
   QDir dirdir( dir, "*", QDir::Name, QDir::Files | QDir::Readable );
   dirdir.makeAbsolute();
   if ( dir.right( 1 ) != "/" ) dir += "/"; // Ensure trailing /
   xpn_fnames       = dirdir.entryList( QStringList( "*.auc" ),
                                        QDir::Files, QDir::Name );
   xpn_fnames.sort();
   ntriple           = xpn_fnames.size();
   cellchans.clear();
   lambdas  .clear();
   triples  .clear();
   int missm         = 0;
DbgLv(1) << "RDa: mwr ntriple" << ntriple;

   for ( int ii = 0; ii < ntriple; ii++ )
   {
      QString mwrfname  = xpn_fnames.at( ii );
      QString mwrfpath  = currentDir + mwrfname;
      ifpaths << mwrfpath;
      US_DataIO::RawData  rdata;

      US_DataIO::readRawData( mwrfpath, rdata );

      allData << rdata;

      QString celchn    = QString::number( rdata.cell ) + " / " +
                          QString( rdata.channel );
      QString ccnode    = mwrfname.section( ".", -4, -3 );
      QString celchnf   = ccnode.replace( ".", " / " );

      if ( celchn != celchnf )
      {
DbgLv(1) << "RDa:   **F/D MISMATCH** " << celchn << celchnf << ii;
         celchn            = celchnf;
         missm++;
      }

      int lambda        = qRound( rdata.scanData[ 0 ].wavelength );
      QString triple    = celchn + " / " + QString::number( lambda );

      if ( ! cellchans.contains( celchn ) )
         cellchans << celchn;

      if ( ! lambdas.contains( lambda ) )
         lambdas   << lambda;

      if ( ! triples.contains( triple ) )
         triples   << triple;

      le_status->setText( tr( "Data in for triple %1 of %2" )
                          .arg( ii + 1 ).arg( ntriple ) );
      qApp->processEvents();
   }

   if ( missm > 0 )
   {
      QMessageBox::warning( this,
            tr( "Data/File Cell/Channel Mismatches" ),
            tr( "%1 mismatch(es) occurred in the Cell/Channel setting\n"
               " of AUC data versus File name nodes.\n\n"
               "The Cell/Channel settings  have been corrected,\n"
               " but input data should be reviewed!!" )
               .arg( missm ) );
   }

   r_radii.clear();
   r_radii << allData[ 0 ].xvalues;

   isRaw        = false;
   haveData     = true;
   ncellch      = cellchans .size();
   nlambda      = lambdas   .size();
   nscan        = allData[ 0 ].scanCount();
   npoint       = allData[ 0 ].pointCount();
   ntpoint      = nscan * npoint;
DbgLv(1) << "RDa: mwr ncellch nlambda nscan npoint"
 << ncellch << nlambda << nscan << npoint;
DbgLv(1) << "RDa:   rvS rvE" << r_radii[0] << r_radii[npoint-1];
   le_status->setText( tr( "All %1 raw AUCs have been loaded." )
                       .arg( ntriple ) );
   QApplication::restoreOverrideCursor();
   QApplication::restoreOverrideCursor();
   qApp->processEvents();

   ct_from->setMaximum( nscan );
   ct_to  ->setMaximum( nscan );

   xpn_data->load_auc( allData, ifpaths );

   QString tspath = currentDir + "/" + runID + ".time_state.tmst";
   haveTmst       = QFile( tspath ).exists();
DbgLv(1) << "RDa: load_auc complete";

   // Ok to enable some buttons now
   enableControls();
DbgLv(1) << "RDa: enableControls complete";
}

// Display detailed information about the data
void US_XpnDataViewer::runDetails( void )
{
   // Use the data object to compose details text
   QString msg = xpn_data->runDetails();

   // Open the dialog and display the report text
   US_Editor* editd = new US_Editor( US_Editor::DEFAULT, true );

   editd->setWindowTitle( tr( "Optima Raw Data Statistics" ) );
   editd->move( pos() + QPoint( 200, 200 ) );
   editd->resize( 600, 500 );
   editd->e->setFont( QFont( US_Widgets::fixedFont().family(),
                             US_GuiSettings::fontSize() ) );
   editd->e->setText( msg );
   editd->show();
}

// Plot the current data record
void US_XpnDataViewer::plot_current( void )
{
   if ( allData.size() == 0 )
      return;

   plot_titles();     // Set the titles

   plot_all();        // Plot the data
}

// Compose plot titles for the current record
void US_XpnDataViewer::plot_titles( void )
{
DbgLv(1) << "pTit: prectype" << prectype;
   QString prec     = cb_pltrec->currentText();
   QString cell     = prec.section( "/", 0, 0 ).simplified();
   QString chan     = prec.section( "/", 1, 1 ).simplified();
   QString wvln     = prec.section( "/", 2, 2 ).simplified();

   if ( isMWL )
   {
      QString cellch   = cb_cellchn ->currentText();
      cell             = cellch.section( "/", 0, 0 ).simplified();
      chan             = cellch.section( "/", 1, 1 ).simplified();
      wvln             = prec;
   }
DbgLv(1) << "pTit: prec" << prec << "isMWL" << isMWL << "wvln" << wvln;

   // Plot Title and legends
   QString title    = "Radial Intensity Data\nRun ID: " + runID +
                      "\n    Cell: " + cell + "  Channel: " + chan +
                      "  Wavelength: " + wvln;
   QString xLegend  = QString( "Radius (in cm)" );
   QString yLegend  = "Radial Intensity at " + wvln + " nm";

   if ( runType == "IP" )
   {
      title            = "Interference Data\nRun ID: " + runID +
                         "\n    Cell: " + cell + "  Channel: " + chan +
                         "  Wavelength: " + wvln;
      yLegend          = "Interference at " + wvln + " nm";
   }
   else if ( runType == "FI" )
   {
      title            = "Fluorescence Intensity Data\nRun ID: " + runID +
                         "\n    Cell: " + cell + "  Channel: " + chan +
                         "  Wavelength: " + wvln;
      yLegend          = "Fluorescence at " + wvln + " nm";
   }
   else if ( runType == "WI" )
   {
      title            = "Wavelength Intensity Data\nRun ID: " + runID +
                         "\n    Cell: " + cell + "  Channel: " + chan +
                         "  Radius: " + wvln;
      xLegend          = QString( "Wavelength (in nm)" );
      yLegend          = "Radial Intensity at " + wvln;
   }

   data_plot->setTitle( title );
   data_plot->setAxisTitle( QwtPlot::yLeft,   yLegend );
   data_plot->setAxisTitle( QwtPlot::xBottom, xLegend );
}

// Draw scan curves for the current plot record
void US_XpnDataViewer::plot_all( void )
{
   dPlotClearAll( data_plot );
   grid           = us_grid( data_plot );

   // Make sure ranges are set up, then build an averaged data vector
   compute_ranges();

//DbgLv(1) << "PltA: kpoint" << kpoint << "datsize" << curr_adata.size();
DbgLv(1) << "PltA: kpoint" << kpoint << "trpxs" << trpxs
 << "nscan" << nscan << allData[trpxs].scanCount();
   // Build the X,Y vectors
   QVector< double > rvec( kpoint );
   QVector< double > vvec( kpoint );
   double* rr     = rvec.data();
   double* vv     = vvec.data();

   int     scan_from = (int)ct_from->value();
   int     scan_to   = (int)ct_to  ->value();
   int     scan_nbr  = 0;
   QPen    pen_red ( Qt::red );
   QPen    pen_plot( US_GuiSettings::plotCurve() );
   int     rdx       = radxs;
   int     colx      = -1;
   US_DataIO::RawData* rdata = &allData[ trpxs ];
   int     scan_knt  = rdata->scanCount();

   for ( int ptx = 0; ptx < kpoint; ptx++, rdx++ )
   {  // One-time build of X vector
      rr[ ptx ]       = rdata->xvalues[ rdx ];
   }
DbgLv(1) << "PltA:   rr[n]" << rr[kpoint-1];

//   for ( int scnx = 0; scnx < nscan; scnx++ )
   for ( int scnx = 0; scnx < scan_knt; scnx++ )
   {  // Build Y vector for each scan
      if ( excludes.contains( scnx ) )  continue;

      for ( int ptx = 0, rdx = radxs; ptx < kpoint; ptx++, rdx++ )
      {
         vv[ ptx ]       = rdata->value( scnx, rdx );
      }

      if ( mcknt > 0 )
      {
         colx                = scan_nbr % mcknt;
         pen_plot            = QPen( mcolors[ colx ] );
      }

      scan_nbr++;
if(scnx<3 || (scnx+4)>nscan)
DbgLv(1) << "PltA:   vv[n]" << vv[kpoint-1] << "scnx" << scnx
 << "scan_nbr" << scan_nbr << "colx" << colx;
      QString       title = tr( "Raw Data at scan " )
                            + QString::number( scan_nbr );
      QwtPlotCurve* curv  = us_curve( data_plot, title );

      if ( scan_nbr > scan_to  ||  scan_nbr < scan_from )
         curv->setPen( pen_plot );            // Normal pen
      else
         curv->setPen( pen_red  );            // Scan-focus pen

      curv->setSamples( rr, vv, kpoint );     // Build a scan curve
//DbgLv(1) << "PltA:   scx" << scx << "rr0 vv0 rrn vvn"
// << rr[0] << rr[kpoint-1] << vv[0] << vv[kpoint-1];
   }

DbgLv(1) << "PltA: last_xmin" << last_xmin;
   if ( last_xmin < 0.0 )
   {  // If first time, use auto scale to set plot ranges
      data_plot->setAxisAutoScale( QwtPlot::yLeft   );
      data_plot->setAxisAutoScale( QwtPlot::xBottom );
   }

   else
   {  // After first time, use the same plot ranges as set before
      data_plot->setAxisScale( QwtPlot::xBottom, last_xmin, last_xmax );
      if ( ck_autoscy->isChecked() )
         data_plot->setAxisAutoScale( QwtPlot::yLeft   );
      else
         data_plot->setAxisScale( QwtPlot::yLeft  , last_ymin, last_ymax );
   }

   // Draw the plot
//   connect( plot, SIGNAL( zoomedCorners( QRectF ) ),
//            this, SLOT  ( currentRectf ( QRectF ) ) );
   data_plot->replot();

   // Pick up the actual bounds plotted (including any Config changes)
   QwtScaleDiv* sdx = AXISSCALEDIV( QwtPlot::xBottom );
   QwtScaleDiv* sdy = AXISSCALEDIV( QwtPlot::yLeft   );
   last_xmin      = sdx->lowerBound();
   last_xmax      = sdx->upperBound();
   last_ymin      = sdy->lowerBound();
   last_ymax      = sdy->upperBound();
DbgLv(1) << "PltA: xlo xhi" << last_xmin << last_xmax
 << "ylo yhi" << last_ymin << last_ymax;
}

// Slot to handle a change in start or end radius
void US_XpnDataViewer::changeRadius()
{
DbgLv(1) << "chgRadius";
   have_rngs  = false;
   compute_ranges();                        // Recompute ranges
   last_xmin      = -1;
   last_xmax      = -1;

   plot_current();                      // Re-plot with adjusted range
}

void US_XpnDataViewer::changeCellCh( void )
{
   // Match the description to find the correct triple in memory
   QString cellch = cb_cellchn->currentText();
DbgLv(1) << "chgCC:  cellch" << cellch << "last_xmin" << last_xmin;
   if ( allData.size() < 1 )
      return;

   if ( last_xmin < 0.0 )
   {  // If first time plotting any data, detect actual data bounds
      have_rngs     = false;
      compute_ranges();

      int trxs      = trpxs + lmbxs;
      int trxe      = trpxs + lmbxe;

      last_xmin     = allData[ trxs ].xvalues[ radxs ];
      last_xmax     = last_xmin;
      US_DataIO::RawData* edata = &allData[ trxs ];

      for ( int rdx = radxs; rdx < radxe; rdx++ )
      {
         last_xmin  = qMin( last_xmin, edata->xvalues[ rdx ] );
         last_xmax  = qMax( last_xmax, edata->xvalues[ rdx ] );
      }

      last_ymin     = allData[ trxs ].reading( 0, 0 );
      last_ymax     = last_ymin;

      for ( int trx = trxs; trx < trxe; trx++ )
      {  // Accumulate Y bounds, the amplitude bounds
         US_DataIO::RawData* edata = &allData[ trx ];

         for ( int scx = 0; scx < nscan; scx++ )
         {
            for ( int rdx = radxs; rdx < radxe; rdx++ )
            {
               last_ymin  = qMin( last_ymin, edata->reading( scx, rdx ) );
               last_ymax  = qMax( last_ymax, edata->reading( scx, rdx ) );
            }
         }
      }
DbgLv(1) << "chgCC: trxs trxe" << trxs << trxe;
   }

   else
   {  // After first time, detect what has been already set
      QwtScaleDiv* sdx = AXISSCALEDIV( QwtPlot::xBottom );
      QwtScaleDiv* sdy = AXISSCALEDIV( QwtPlot::yLeft   );
      last_xmin      = sdx->lowerBound();
      last_xmax      = sdx->upperBound();
      last_ymin      = sdy->lowerBound();
      last_ymax      = sdy->upperBound();
   }
DbgLv(1) << "chgCC:  xmin xmax ymin ymax"
 << last_xmin << last_xmax << last_ymin << last_ymax;

   have_rngs      = false;
   int lplrec     = ntriple - 1;

   // Make sure cell/channel and plot record are in sync
   if ( ! isMWL )
   {  // If plot record ranges are triples, point to first of cell/channel
      QString ccval  = cb_cellchn->currentText().replace( " / ", "/" );
      QString ccplr  = cb_pltrec->currentText().section( "/", 0, 1 );
DbgLv(1) << "chgCC: ccval" << ccval << "ccplr" << ccplr;

      if ( ccval != ccplr )
      {
         for ( int jj = 0; jj < ntriple; jj++ )
         {
            ccplr          = triples[ jj ].section( "/", 0, 1 ).simplified()
                                          .replace( " / ", "/" );
DbgLv(1) << "chgCC:   jj" << jj << "ccplr" << ccplr;
            if ( ccplr == ccval )
            {  // Set plot record to first triple from cell/channel
               connect_ranges( false );
DbgLv(1) << "chgCC:     set pltrec index" << jj;
               cb_pltrec->setCurrentIndex( jj );
               connect_ranges( true );
               break;
            }
         }
      }
   }
   else
   {  // If plot record ranges are wavelengths, point to first wavelength
DbgLv(1) << "chgCC: isMWL" << isMWL;
      lplrec         = nlambda - 1;
//      connect_ranges( false );
//      cb_pltrec->setCurrentIndex( 0 );
//      connect_ranges( true );
   }

   compute_ranges();
   pb_prev  ->setEnabled( ( recx > 0 )      );
   pb_next  ->setEnabled( ( recx < lplrec ) );

//   changeRecord();
   plot_current();
}

// Slot to handle a change in the plot record
void US_XpnDataViewer::changeRecord( void )
{
   recx           = cb_pltrec->currentIndex();
   int lplrec     = ntriple - 1;

   if ( isMWL )
   {  // For MWL, limit record range to lambda range
      lplrec         = nlambda - 1;
   }

   else
   {  // For non-MWL, change cell/channel if appropriate
      QString new_cc = QString( cb_pltrec->currentText() )
                       .section( "/", 0, 1 ).replace( "/", " / " );
      QString celchn = cb_cellchn->currentText();

      if ( new_cc != celchn )
      {
         int iccx       = cellchans.indexOf( new_cc );
         connect_ranges( false );
         cb_cellchn->setCurrentIndex( iccx );
         QString new_wl = QString( cb_pltrec->currentText() )
                          .section( "/", 2, 2 );
         le_lrange ->setText( new_wl + tr( " only" ) );
         connect_ranges( true );
      }
   }

DbgLv(1) << "chgRec: recx" << recx;

   // Plot what we have
   plot_current();

   // Update status text (if not part of movie save) and set prev/next arrows
   le_status->setText( lb_pltrec->text() + "  " + cb_pltrec->currentText() );
   pb_prev  ->setEnabled( ( recx > 0 )      );
   pb_next  ->setEnabled( ( recx < lplrec ) );
}

// Slot to handle a change in the optical system
void US_XpnDataViewer::changeOptics( void )
{
   // Determine the new run type
   int optrx      = cb_optsys->currentIndex();
   QString ostyp  = cb_optsys ->currentText();
   QString rtype( "RI" );
   if ( ostyp == "Interference" )
      rtype          = "IP";
   else if ( ostyp == "Fluorescence" )
      rtype          = "FI";
   else if ( ostyp == "Wavelength" )
      rtype          = "WI";
DbgLv(1) << "chgOpt: optrx" << optrx << "ostyp" << ostyp
 << "rtype" << rtype;

  // If simply re-choosing the same optics, bale out now
   if ( rtype == runType )
      return;

QDateTime sttime=QDateTime::currentDateTime();
   runType       = rtype;    // Set the new run type (RI, IP, ...)

   // Turn off auto-reload if on
   if ( rlt_id != 0 )
   {
      ck_autorld->setChecked( false );
      QMessageBox::warning( this,
            tr( "Auto-Reload Stopped" ),
            tr( "Auto-Reload has been stopped and its box unchecked,"
                "\n since the optical system has been changed." ) );
   }

   // Set up for a new run type
   xpn_data->set_run_values( runID, runType );

   // For new optics, rebuild internal arrays and all AUC
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor) );
   le_status->setText( tr( "Rebuilding AUC data ..." ) );
   qApp->processEvents();

   xpn_data->build_rawData( allData );
double tm2=(double)sttime.msecsTo(QDateTime::currentDateTime())/1000.0;
DbgLv(1) << "chgOpt:   build-raw done: tm2" << tm2;

   QApplication::restoreOverrideCursor();
   QApplication::restoreOverrideCursor();

   // Reset various flags, counts, and lists
   isRaw         = true;
   haveData      = true;
   ncellch       = xpn_data->cellchannels( cellchans );
   r_radii.clear();
   r_radii << allData[ 0 ].xvalues;
   nscan         = allData[ 0 ].scanCount();
   npoint        = allData[ 0 ].pointCount();

DbgLv(1) << "chgOpt: mwr ntriple" << ntriple;
DbgLv(1) << "chgOpt: ncellch" << ncellch << cellchans.count();
DbgLv(1) << "chgOpt: nscan" << nscan << "npoint" << npoint;
DbgLv(1) << "chgOpt:   rvS rvE" << r_radii[0] << r_radii[npoint-1];
   cb_cellchn->disconnect();
   cb_cellchn->clear();
   cb_cellchn->addItems( cellchans );
   connect( cb_cellchn,   SIGNAL( currentIndexChanged( int ) ),
            this,         SLOT  ( changeCellCh(            ) ) );

   nlambda      = xpn_data->lambdas_raw( lambdas );
   int wvlo     = lambdas[ 0 ];
   int wvhi     = lambdas[ nlambda - 1 ];
   ntriple      = xpn_data->data_triples( triples );
DbgLv(1) << "chgOpt: nwl wvlo wvhi" << nlambda << wvlo << wvhi
   << "ncellch" << ncellch << "nlambda" << nlambda << "ntriple" << ntriple
   << triples.count();

DbgLv(1) << "chgOpt: allData size" << allData.size();
   QApplication::restoreOverrideCursor();
   QString tspath = currentDir + "/" + runID + ".time_state.tmst";
   haveTmst       = QFile( tspath ).exists();

   // Ok to reenable some buttons now
   enableControls();
}

// Slot to handle a click to go to the previous record
void US_XpnDataViewer::prevPlot( void )
{
   int pltrx      = cb_pltrec->currentIndex() - 1;

   if ( pltrx < 1 )
   {
      pltrx          = 0;
      pb_prev->setEnabled( false );
   }

   QwtScaleDiv* sdx = AXISSCALEDIV( QwtPlot::xBottom );
   QwtScaleDiv* sdy = AXISSCALEDIV( QwtPlot::yLeft   );
   last_xmin      = sdx->lowerBound();
   last_xmax      = sdx->upperBound();
   last_ymin      = sdy->lowerBound();
   last_ymax      = sdy->upperBound();

   cb_pltrec->setCurrentIndex( pltrx );
}

// Slot to handle a click to go to the next record
void US_XpnDataViewer::nextPlot( void )
{
   int pltrx      = cb_pltrec->currentIndex() + 1;
   int nitems     = cb_pltrec->count();

   if ( ( pltrx + 2 ) > nitems )
   {
      pltrx          = nitems - 1;
      pb_next->setEnabled( false );
   }

   QwtScaleDiv* sdx = AXISSCALEDIV( QwtPlot::xBottom );
   QwtScaleDiv* sdy = AXISSCALEDIV( QwtPlot::yLeft   );
   last_xmin      = sdx->lowerBound();
   last_xmax      = sdx->upperBound();
   last_ymin      = sdy->lowerBound();
   last_ymax      = sdy->upperBound();

   cb_pltrec->setCurrentIndex( pltrx );
}

// Compute the plot range indexes implied by current settings
void US_XpnDataViewer::compute_ranges()
{
   if ( have_rngs )   // If we just did this computation, return now
      return;

   ccx        = cb_cellchn->currentIndex();         // Cell/Channel index
   rad_start  = cb_rstart ->currentText().toDouble();  // Radius start
   rad_end    = cb_rend   ->currentText().toDouble();  // Radius end
   lmb_start  = lambdas[ 0 ];                       // Lambda start
   lmb_end    = lambdas[ nlambda - 1 ];             // Lambda end
   recx       = cb_pltrec ->currentIndex();         // Plot record index
   lmbxs      = lambdas.indexOf( lmb_start );       // Lambda start index
   lmbxe      = lambdas.indexOf( lmb_end   ) + 1;   // Lambda end index
   radxs      = dvec_index( r_radii, rad_start );     // Radius start index
   radxe      = dvec_index( r_radii, rad_end   ) + 1; // Radius end index
DbgLv(1) << "cmpR:  rS rE rxS rxE" << rad_start << rad_end << radxs << radxe;
DbgLv(1) << "cmpR:   rvS rvE" << r_radii[radxs] << r_radii[radxe-1];
   klambda    = lmbxe - lmbxs;                      // Count of plot lambdas
   kradii     = radxe - radxs;                      // Count of plot radii
   kscan      = nscan - excludes.size();            // Count included scans
   trpxs      = ccx * nlambda;                      // Start triple index
   kpoint     = kradii;                             // Count of plot points
   ktpoint    = kscan * kpoint;                     // Total plot data points

   if ( isMWL )
   {
      trpxs     += recx;                            // Triple-to-plot index
   }
   else
   {
      trpxs      = recx;                            // Triple-to-plot index
   }
//   have_rngs  = true;                               // Mark ranges computed
DbgLv(1) << "cmpR:  isMWL" << isMWL << "kpoint" << kpoint << "trpxs" << trpxs;
}

// Connect or Disconnect plot-range related controls
void US_XpnDataViewer::connect_ranges( bool conn )
{
   if ( conn )
   {  // Connect the range-related controls
      connect( cb_cellchn, SIGNAL( currentIndexChanged( int ) ),
               this,       SLOT  ( changeCellCh(            ) ) );
      connect( cb_rstart,  SIGNAL( currentIndexChanged( int ) ),
               this,       SLOT  ( changeRadius(            ) ) );
      connect( cb_rend,    SIGNAL( currentIndexChanged( int ) ),
               this,       SLOT  ( changeRadius(            ) ) );
      connect( cb_pltrec,  SIGNAL( currentIndexChanged( int ) ),
               this,       SLOT  ( changeRecord(            ) ) );
   }

   else
   {  // Disconnect the range-related controls
      cb_cellchn->disconnect();
      cb_rstart ->disconnect();
      cb_rend   ->disconnect();
      cb_pltrec ->disconnect();
   }
}

// Slot to export to openAUC
void US_XpnDataViewer::export_auc()
{
   if ( ! isRaw  ||  allData.size() == 0 )
      return;

   QString runIDt = le_runID->text();              // User given run ID text

   if ( runIDt != runID )
   {  // Set runID to new entry given by user
      QRegExp rx( "[^A-Za-z0-9_-]" );              // Possibly modify entry
      QString new_runID  = runIDt;
      int pos            = 0;

      while ( ( pos = rx.indexIn( new_runID ) ) != -1 )
      {  // Loop thru any invalid characters given (not alphanum.,'_','-')
         new_runID.replace( pos, 1, "_" );         // Replace 1 char at pos
      }

      // Let the user know that the runID name has changed
      QMessageBox::warning( this,
         tr( "RunId Name Changed" ),
         tr( "The runId name has been changed."
             "\nNew runId:\n  " ) + new_runID );

      runID          = new_runID;                  // Show new run ID, dir.
      le_runID->setText( runID );
      currentDir     = US_Settings::importDir() + "/" + runID;
      le_dir  ->setText( currentDir );
      qApp->processEvents();

      xpn_data->set_run_values( runID, runType );  // Set run ID for export
   }

   // Export the AUC data to a local directory and build TMST

   correct_radii();      // Perform chromatic aberration radius corrections

   int nfiles     = xpn_data->export_auc( allData );

   QString tspath = currentDir + "/" + runID + ".time_state.tmst";
   haveTmst       = QFile( tspath ).exists();

   le_status  ->setText( tr( "%1 AUC/TMST files written ..." ).arg( nfiles ) );
   pb_showtmst->setEnabled( haveTmst );
   qApp->processEvents();
DbgLv(1) << "ExpAuc: haveTmst" << haveTmst << "tmst file" << tspath;
}

// Slot to handle a change in scan exclude "from" value
void US_XpnDataViewer::exclude_from( double sfr )
{
   int scan_from  = (int)sfr;
   int scan_to    = (int)ct_to  ->value();

   if ( scan_to < scan_from )
   {
      ct_to  ->disconnect();
      ct_to  ->setValue( scan_from );

      connect( ct_to,        SIGNAL( valueChanged( double ) ),
               this,         SLOT  ( exclude_to  ( double ) ) );
   }

   plot_current();
}

// Slot to handle a change in scan exclude "to" value
void US_XpnDataViewer::exclude_to( double sto )
{
   int scan_to    = (int)sto;
   int scan_from  = (int)ct_from->value();

   if ( scan_from > scan_to )
   {
      ct_from->disconnect();
      ct_from->setValue( scan_to );

      connect( ct_from,      SIGNAL( valueChanged( double ) ),
               this,         SLOT  ( exclude_from( double ) ) );
   }

   plot_current();
}

// Slot to handle click of Exclude Scan Range
void US_XpnDataViewer::exclude_scans()
{
   int scan_from  = (int)ct_from->value();
   int scan_to    = (int)ct_to  ->value();
   int scan_knt   = 1;

   for ( int scnx = 0; scnx < nscan; scnx++ )
   {
      if ( excludes.contains( scnx ) )  continue;

      if ( scan_knt >= scan_from  &&  scan_knt <= scan_to )
         excludes << scnx;

      scan_knt++;
   }

   qSort( excludes );
   kscan      = nscan - excludes.count();
DbgLv(1) << "Excl: kscan" << kscan;
   ct_from   ->disconnect();
   ct_to     ->disconnect();
   ct_from   ->setMaximum( kscan );
   ct_to     ->setMaximum( kscan );
   connect( ct_from,      SIGNAL( valueChanged( double ) ),
            this,         SLOT  ( exclude_from( double ) ) );
   connect( ct_to,        SIGNAL( valueChanged( double ) ),
            this,         SLOT  ( exclude_to  ( double ) ) );
   ct_to     ->setValue( 0 );
   pb_include->setEnabled( true );
}

// Slot to handle click of Include All (restore of all scans)
void US_XpnDataViewer::include_scans()
{
   excludes.clear();

   kscan      = nscan;
   ktpoint    = kscan * kpoint;
DbgLv(1) << "Incl: nscan" << nscan << "kscn ecnt" << kscan << excludes.count();
   ct_to     ->setValue( 0 );
//   changeRecord();                     // Force replot
   plot_current();                     // Force replot

   ct_from   ->disconnect();
   ct_to     ->disconnect();
   ct_from   ->setMaximum( kscan );
   ct_to     ->setMaximum( kscan );
   connect( ct_from,      SIGNAL( valueChanged( double ) ),
            this,         SLOT  ( exclude_from( double ) ) );
   connect( ct_to,        SIGNAL( valueChanged( double ) ),
            this,         SLOT  ( exclude_to  ( double ) ) );
   ct_to     ->setValue( 0 );
   pb_include->setEnabled( false );
}

// Utility to find an index in a QVector<double> to a value epsilon match
int US_XpnDataViewer::dvec_index( QVector< double >& dvec, const double dval )
{
   const double eps = 1.e-4;
   double dmag     = qAbs( dval);
//DbgLv(1) << "dvix: dval dvknt" << dval << dvec.count();

   int indx        = dvec.indexOf( dval );   // Try to find an exact match
//DbgLv(1) << "dvix: dvec0 dval indx" << dvec[0] << dval << indx;

   if ( indx < 0 )
   {  // If no exact match was found, look for a match within epsilon

      for ( int jj = 0; jj < dvec.size(); jj++ )
      {  // Search doubles vector
         double ddif     = qAbs( dvec[ jj ] - dval ) / dmag;
//if(jj<3||(jj+4)>dvec.size())
//DbgLv(1) << "dvix:   jj" << jj << "dvj" << dvec[jj] << "ddif" << ddif;

         if ( ddif < eps )
         {  // If vector value matches within epsilon, break and return
            indx            = jj;
            break;
         }
      }
   }

//DbgLv(1) << "dvix:   dval" << dval << "indx" << indx;
   return indx;
}

// Slot to pop up dialog to plot TMST values
void US_XpnDataViewer::showTimeState()
{
   QString tspath = currentDir + "/" + runID + ".time_state.tmst";
   US_TmstPlot* tsdiag = new US_TmstPlot( this, tspath );

DbgLv(1) << "sTS: tsdiag exec()";
   tsdiag->exec();
DbgLv(1) << "sTS: tsdiag DONE";
}

// Slot to report updated status text
void US_XpnDataViewer::status_report( QString stat_text )
{
   qApp->processEvents();

   le_status->setText( stat_text );

   qApp->processEvents();
}

// Slot to reload data
void US_XpnDataViewer::reloadData()
{
   if ( in_reload )             // If already doing a reload,
      return;                   //  skip starting a new one

   in_reload   = true;          // Flag in the midst of a reload
   int runix          = runID.lastIndexOf( "-run" ) + 4;
   QString fRunId     = runID.mid( runix );
   int iRunId         = fRunId.toInt();
DbgLv(1) << "RLd:  runID" << runID << "runix" << runix << "iRunId" << iRunId;
   int scanmask       = 1;
   scanmask           = ( runType == "FI" ) ? 2 : scanmask;
   scanmask           = ( runType == "IP" ) ? 4 : scanmask;
   scanmask           = ( runType == "WI" ) ? 8 : scanmask;
DbgLv(1) << "RLd:     iRunId" << iRunId << "runType scanmask" << runType << scanmask;

QDateTime sttime=QDateTime::currentDateTime();
   QString smsg       = le_status->text();
   le_status->setText( tr( "Scanning Optima DB for any data updates..." ) );
   qApp->processEvents();

   // Import any newly added Scan Data records
   bool upd_ok        =  xpn_data->reimport_data( iRunId, scanmask );

   if ( ! upd_ok )
   {  // No change in data scans:  report inability to update
      nscan       = allData[ trpxs ].scanCount();
      if ( rlt_id == 0 )    // Output message only if not auto-reload
      {
         QMessageBox::warning( this,
               tr( "Reload Data Not Possible" ),
               tr( "The \"Reload Data\" action had no effect.\n"
                   "No additional data has been recorded." ) );
      }

      if ( ! smsg.endsWith( tr( " scans)" ) ) )
      {  // If need be, add scan count to the status message
         smsg        = smsg + tr( "  (%1 scans)" ).arg( nscan );
         le_status->setText( smsg );
         qApp->processEvents();
      }
      return;     // Return with no change in AUC data
   }
double tm1=(double)sttime.msecsTo(QDateTime::currentDateTime())/1000.0;

   // Otherwise, report updated raw data import
   le_status->setText( tr( "Update of Raw Optima data import complete." ) );
   qApp->processEvents();

   // Now, update the AUC data with new scans
   xpn_data->rebuild_rawData( allData );

double tm2=(double)sttime.msecsTo(QDateTime::currentDateTime())/1000.0;
DbgLv(1) << "RLd:      build-raw done: tm1 tm2" << tm1 << tm2
 << "tm2i" << (tm2-tm1);
   // Reset scan counter maximum and report update complete
   nscan       = allData[ trpxs ].scanCount();
   npoint      = allData[ trpxs ].pointCount();
   ntpoint     = nscan * npoint;
   ct_from->setMaximum( nscan );
   ct_to  ->setMaximum( nscan );
   le_status->setText( tr( "Update of AUC data complete -- now %1 scans." )
                       .arg( nscan ) );
   qApp->processEvents();

   // Do resets and re-plot the current triple
   changeCellCh();
   in_reload   = false;         // Flag no longer in the midst of reload
}

// Slot to respond to a timer event (auto-reload)
void US_XpnDataViewer::timerEvent( QTimerEvent *event )
{
   int tim_id  = event->timerId();
DbgLv(1) << "            timerEvent:   tim_id" << tim_id << "    "
 << QDateTime::currentDateTime().toString( "hh:mm:ss" );

   if ( tim_id != rlt_id )
   {  // if other than auto-reload event, pass on to normal handler
      QWidget::timerEvent( event );
      return;
   }

   // Do a reload of data (if none currently underway)
   reloadData();
}

// Slot to handle change in auto-reload check
void US_XpnDataViewer::changeReload()
{
DbgLv(1) << "chgRld: checked" << ck_autorld->isChecked();
   if ( ck_autorld->isChecked() )
   {  // Changing to checked:  start the timer
      data_plot->replot();

      rlt_dlay    = qRound( ct_rinterv->value() * 1000.0 );
      rlt_id      = startTimer( rlt_dlay );
DbgLv(1) << "chgRld:  rlt_dlay" << rlt_dlay << "rlt_id" << rlt_id;
   }

   else
   {  // Just been unchecked:  stop the timer if need be
DbgLv(1) << "chgRld:  rlt_id" << rlt_id;
      if ( rlt_id != 0 )
      {
         killTimer( rlt_id );
DbgLv(1) << "chgRld:    STOPPED";
      }
      rlt_id      = 0;
   }
}

// Slot to handle change in auto-reload interval
void US_XpnDataViewer::changeInterval()
{
DbgLv(1) << "chgInt: checked" << ck_autorld->isChecked();
   if ( ! ck_autorld->isChecked() )
   {  // If no auto reload checked, ignore this interval change for now
      rlt_id      = 0;
      return;
   }

DbgLv(1) << "chgInt:  rlt_id" << rlt_id << "newInt" << ct_rinterv->value()
 << "    " << QDateTime::currentDateTime().toString( "hh:mm:ss" );
   // Otherwise, stop the timer and then restart it with a new delay
   if ( rlt_id != 0 )
   {
      killTimer( rlt_id );
   }

   changeReload();
}

// Slot to use a file dialog to select a new scan curve color map
void US_XpnDataViewer::selectColorMap()
{
   QString cmapname( "cm-rainbow" );
   QString filter  = tr( "Color Map files (*cm-*.xml);;"
                         "Any XML files (*.xml);;"
                         "Any files (*)" );

   // get an xml file name for the color map
   QString cmfpath = QFileDialog::getOpenFileName( this,
       tr( "Load Color Map File" ),
       US_Settings::etcDir(), filter, 0, 0 );

   if ( cmfpath.isEmpty() )
        return;

DbgLv(1) << "sCM: cmfpath" << cmfpath;
   US_ColorGradIO::read_color_gradient( cmfpath, mcolors );
   mcknt           = mcolors.count();
DbgLv(1) << "sCM: mcolors count" << mcknt;
   cmapname        = QFileInfo( cmfpath ).baseName().replace( ".xml$", "" );
   le_colmap->setText( cmapname );

   if ( allData.size() > 0 )
      plot_all();
}

// Apply a chromatic aberration correction to auc data radius values
void US_XpnDataViewer::correct_radii()
{
   const double wl_max  = 800.0;    // Max wavelength for computed correction
   const double rad_inc = 1e-5;     // Radius precision
   double a_coef     = 0.0;         // Default 4th order polynomial coefficents
   double b1_coef    = 0.0;     
   double b2_coef    = 0.0;
   double b3_coef    = 0.0;
   double b4_coef    = 0.0;
   double b5_coef    = 0.0;
   int ntripl        = allData.count();
   int npoint        = allData[ 0 ].pointCount();

   // If coefficient values are in an */etc file, use them
   QString cofname   = US_Settings::etcDir() + "/chromo-aberration-coeffs.dat";
   QFile cofile( cofname );

   if ( cofile.open( QIODevice::ReadOnly | QIODevice::Text ) )
   {
      QTextStream cotxti( &cofile );
      QString fline;
      while ( ! cotxti.atEnd() )
      {
         fline             = cotxti.readLine().simplified();
         if ( ! fline.isEmpty()  &&  ! fline.startsWith( "#" ) )
         {  // Get values from first non-empty, non-comment line
DbgLv(1) << "c_r: file-read line" << fline;
            a_coef            = QString( fline ).section( " ", 0, 0 )
                                   .simplified().toDouble();
            b1_coef           = QString( fline ).section( " ", 1, 1 )
                                   .simplified().toDouble();
            b2_coef           = QString( fline ).section( " ", 2, 2 )
                                   .simplified().toDouble();
            b3_coef           = QString( fline ).section( " ", 3, 3 )
                                   .simplified().toDouble();
            b4_coef           = QString( fline ).section( " ", 4, 4 )
                                   .simplified().toDouble();
            b5_coef           = QString( fline ).section( " ", 5, 5 )
                                   .simplified().toDouble();
            break;
         }
      }

      cofile.close();
DbgLv(1) << "c_r: file-read ca coeffs:" << a_coef << b1_coef << b2_coef
 << b3_coef << b4_coef << b5_coef;
   }
	else
	{
DbgLv(1) << US_Settings::etcDir() + "/chromo-aberration-coeffs.dat not found";
	}


   // For each triple, get the wavelength; then compute and apply a correction
   for ( int jd = 0; jd < ntripl; jd++ )
   {
      double wavelen    = allData[ jd ].scanData[ 0 ].wavelength; // Wavelength
      double wl_used    = qMin( wavelen, wl_max );  // WL used
      double wl_p2      = sq( wl_used );            // Squared
      double wl_p3      = wl_p2 * wl_used;          // Cubed
      double wl_p4      = wl_p2 * wl_p2;            // To 4th power
      double wl_p5      = wl_p4 * wl_used;          // To 5th power

      // Wavelength-dependent correction
      double wl_corr    = a_coef + wl_used * b1_coef
                                 + wl_p2   * b2_coef
                                 + wl_p3   * b3_coef
                                 + wl_p4   * b4_coef
											+ wl_p5   * b5_coef;
if (jd<3 || (jd+4)>ntripl )
DbgLv(1) << "c_r: jd" << jd << "wavelen" << wavelen << "wl_corr" << wl_corr;

      if ( wl_corr != 0.0 )
      {
         for ( int jr = 0; jr < npoint; jr++ )
         {  // Correct each radial point
            double radval     = r_radii[ jr ] - wl_corr;   // Corrected radius
            radval            = qRound( radval / rad_inc ) * rad_inc; // Rounded
            allData[ jd ].xvalues[ jr ] = radval;          // Replace radius
         }
      }
if (jd<3 || (jd+4)>ntripl )
DbgLv(1) << "c_r:  ri0 ro0" << r_radii[0] << allData[jd].xvalues[0]
 << "rin ron" << r_radii[npoint-1] << allData[jd].xvalues[npoint-1];
   }
}

// Capture X range of latest Zoom
void US_XpnDataViewer::currentRectf( QRectF rectf )
{
   QVector< double >  ascdat;
   double rad1      = qRound( rectf.left()  * 10000.0 ) * 0.0001;
   double rad2      = qRound( rectf.right() * 10000.0 ) * 0.0001;
   if ( rad2 > 7.0 )
      return;           // Skip further processing if not reasonable zoom
   int irx1         = 0;
   int irx2         = 0;
   double kpoint    = 0;
   QString cellch   = cb_cellchn ->currentText();
   QString cech     = QString( cellch ).replace( " / ", "" );

   QString impath   = US_Settings::importDir() + "/" + runID;
   QDir dir;
   if ( ! dir.exists( impath ) )
      dir.mkpath( impath );
   QString dapath   = impath + "/" + cech + ".wavelen.radpos.dat";
   QFile dafile( dapath );
   if ( !dafile.open( QIODevice::WriteOnly | QIODevice::Text ) )
   {
      return;
   }
   QTextStream datxto( &dafile );

   QString msg      = tr( "%1, %2-to-%3 Radial adjustment scan..." )
                      .arg( cellch ).arg( rad1 ).arg( rad2 );
   le_status->setText( msg );
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor) );
   qApp->processEvents();
DbgLv(1) << "cRect" << msg;

   // Determine meniscus position for each wavelength of this channel
   for ( int jd = 0; jd < allData.count(); jd++ )
   {
      US_DataIO::RawData *rdata = &allData[ jd ];

      QString dchann    = QString::number( rdata->cell )
                          + QString( rdata->channel );
      if ( dchann != cech )
         continue;

      if ( irx2 == 0 )
      {
         irx1             = US_DataIO::index( rdata, rad1 );
         irx2             = US_DataIO::index( rdata, rad2 );
         kpoint           = irx2 - irx1 + 1;
      }

      double wavelen    = rdata->scanData[ 0 ].wavelength; // Wavelength

      // Create an average scan value vector for the search range
      int nscan         = rdata->scanCount();
      double afact      = 1.0 / (double)nscan;
      ascdat.fill( 0.0, kpoint );
      for ( int js = 0; js < nscan; js++ )
      {
         US_DataIO::Scan* dscan = &rdata->scanData[ js ];

         for ( int jr = 0; jr < kpoint; jr++ )
         {
            ascdat[ jr ] += dscan->rvalues[ jr + irx1 ] * afact;
         }
      }

      // Find the position of the minimum value in the range
      double rvmin      = 1.0e+99;
      int irpos         = -1;
      for ( int jr = 0; jr < kpoint; jr++ )
      {
         double rval       = ascdat[ jr ];
         if ( rval < rvmin )
         {
            rvmin             = rval;
            irpos             = jr;
         }
      }

      double radiusw    = rdata->xvalues[ irpos + irx1 ];
DbgLv(1) << "  wavelen/radpos:  " << wavelen << " / " << radiusw;

      QString outline   = QString::number( wavelen ) + ","
                        + QString::number( radiusw ) + "\n";
      datxto << outline;
   }

   dafile.close();

   le_status->setText( tr( "%1  Radial adjustment scan complete." )
                      .arg( cech ) );
   QApplication::restoreOverrideCursor();
   qApp->processEvents();
}

