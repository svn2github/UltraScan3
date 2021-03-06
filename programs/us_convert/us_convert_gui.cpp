#include <QApplication>

#include "us_license_t.h"
#include "us_license.h"
#include "us_util.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_gui_util.h"
#include "us_run_details2.h"
#include "us_plot.h"
#include "us_math2.h"
#include "us_convert_gui.h"
#include "us_convertio.h"
#include "us_experiment_gui.h"
#include "us_select_triples.h"
#include "us_solution_gui.h"
#include "us_db2.h"
#include "us_passwd.h"
#include "us_intensity.h"
#include "us_get_run.h"
#include "us_investigator.h"
#include "us_constants.h"
#include "us_time_state.h"
#include "us_report.h"
#include "us_gui_util.h"
#include "us_util.h"
#include "us_images.h"
#include "us_tmst_plot.h"
#include "us_astfem_math.h"

#if QT_VERSION < 0x050000
#define setSamples(a,b,c)  setData(a,b,c)
#define setMinimum(a)      setMinValue(a)
#define setMaximum(a)      setMaxValue(a)
#endif

#ifdef Q_OS_WIN
#include <float.h>
#ifndef isnan
#define isnan _isnan
#endif
#endif

int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );

   #include "main1.inc"

   // License is OK.  Start up.

   US_ConvertGui w;
   w.show();                   //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

US_ConvertGui::US_ConvertGui() : US_Widgets()
{
   ExpData.invID = US_Settings::us_inv_ID();

   // Ensure data directories are there
   QDir dir;
   dir.mkpath( US_Settings::workBaseDir() );
   dir.mkpath( US_Settings::importDir()   );
   dir.mkpath( US_Settings::tmpDir()      );
   dir.mkpath( US_Settings::dataDir()     );
   dir.mkpath( US_Settings::archiveDir()  );
   dir.mkpath( US_Settings::resultDir()   );
   dir.mkpath( US_Settings::reportDir()   );
   dir.mkpath( US_Settings::etcDir()      );

   setWindowTitle( tr( "Import Experimental Data (Beckman-XLA/XLI,"
                       " Aviv Fluorescence, OpenAUC Multiwavelength)" ) );
   setPalette( US_GuiSettings::frameColor() );

   isMwl        = false;
   tmst_fnamei  = QString( "" );
   dbg_level    = US_Settings::us_debug();
DbgLv(0) << "CGui: dbg_level" << dbg_level;

   QGridLayout* settings = new QGridLayout;

   int  row     = 0;

   // First row
   QStringList DB   = US_Settings::defaultDB();
   if ( DB.isEmpty() ) DB << "Undefined";
   QLabel* lb_DB    = us_banner( tr( "Database: " ) + DB.at( 0 ) );
   lb_DB->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

   // Investigator
   bool isadmin     = ( US_Settings::us_inv_level() > 2 );
   QWidget* wg_investigator;
   QPushButton* pb_investigator = us_pushbutton( tr( "Select Investigator"));

   if ( isadmin )
   {  // Admin gets investigator button
      wg_investigator         = (QWidget*)pb_investigator;
   }
   else
   {  // Non-admin gets only investigator label
      QLabel* lb_investigator = us_label( tr( "Investigator:" ) );
      wg_investigator         = (QWidget*)lb_investigator;
      pb_investigator->setVisible( false );
   }

   le_investigator   = us_lineedit(   tr( "Not Selected" ), 0, true );

   // Radio buttons
   disk_controls     = new US_Disk_DB_Controls( US_Disk_DB_Controls::Default );
   save_diskDB       = US_Disk_DB_Controls::Default;

   // Display status
   QLabel* lb_status = us_label(      tr( "Status:" ) );
   le_status         = us_lineedit(   tr( "(no data loaded)" ), 1, true );
   QPalette stpal;
   stpal.setColor( QPalette::Text, Qt::white );
   stpal.setColor( QPalette::Base, Qt::blue  );
   le_status->setPalette( stpal );

   // Display Run ID
   QLabel* lb_runID  = us_label(      tr( "Run ID:" ) );
   // Add this later, after tabs:settings->addWidget( lb_runID, row, 0 );
   lb_runID->setVisible( false ); // for now

   le_runID          = us_lineedit(   "", 1, true );
   //le_runID ->setMinimumWidth( 280 );
   // Add this later, after tabs: settings->addWidget( le_runID, row++, 1 );
   le_runID ->setVisible ( false );  // for now

   // Load the run
   QLabel* lb_run    = us_banner(     tr( "Load the Run" ) );

   // Pushbuttons to load and reload data
   pb_import         = us_pushbutton( tr( "Import Experimental Data" ) );

   // External program to enter experiment information
   pb_editRuninfo    = us_pushbutton( tr( "Edit Run Information" ) );
   pb_editRuninfo->setEnabled( false );

   // Load US3 data ( that perhaps has been done offline )
   pb_loadUS3        = us_pushbutton( tr( "Load US3 OpenAUC Run" ), true );

   // Show TimeState
   pb_showTmst       = us_pushbutton( tr( "Show TimeState" ), false );

   // Run details
   pb_details        = us_pushbutton( tr( "Run Details" ), false );

   // Set the wavelength tolerance for c/c/w determination
   QLabel* lb_tolerance = us_label(   tr( "Separation Tolerance:" ) );
   ct_tolerance      = us_counter ( 2, 0.0, 100.0, 5.0 );
   ct_tolerance->setSingleStep( 1 );

   // Set up MWL controls
   QFont font( US_GuiSettings::fontFamily(), US_GuiSettings::fontSize() - 1 );
   QFontMetrics fmet( font );
   static QChar clambda( 955 );   // Lambda character

   lb_mwlctrl   = us_banner  ( tr( "Multi-Wavelength Lambda Controls" ) );
   lb_lambstrt  = us_label   ( tr( "%1 Start:"    ).arg( clambda ) );
   lb_lambstop  = us_label   ( tr( "%1 End:"      ).arg( clambda ) );
   lb_lambplot  = us_label   ( tr( "Plot %1:"     ).arg( clambda ) );
   cb_lambstrt  = us_comboBox();
   cb_lambstop  = us_comboBox();
   cb_lambplot  = us_comboBox();
   pb_lambprev  = us_pushbutton( "previous", true, -2 );
   pb_lambnext  = us_pushbutton( "next",     true, -2 );
   pb_lambprev->setIcon( US_Images::getIcon( US_Images::ARROW_LEFT  ) );
   pb_lambnext->setIcon( US_Images::getIcon( US_Images::ARROW_RIGHT ) );

   mwl_connect( true );

   QLabel* lb_runinfo  = us_banner(   tr( "Run Information" ) );

   // Change Run ID
   QLabel* lb_runID2   = us_label(    tr( "Run ID:" ) );
   le_runID2           = us_lineedit( "", 1 );
   //le_runID2 ->setMinimumWidth( 225 );

   // Directory
   QLabel* lb_dir      = us_label(    tr( "Directory:" ) );
   le_dir              = us_lineedit( "", 1, true );

   // Description
   lb_description      = us_label(    tr( "Description:" ), 0 );
   //lb_description ->setMaximumWidth( 175 );
   le_description      = us_lineedit( "", 1 );

   // Cell / Channel / Wavelength
   QGridLayout* ccw    = new QGridLayout();

   lb_triple           = us_banner(
                            tr( "Cell / Channel / Wavelength" ), -1 );
   lw_triple           = us_listwidget();
   QLabel* lb_ccwinfo  = us_label(
                            tr( "Enter Associated Triple (c/c/w) Info:" ) );

   // Set up centerpiece drop-down
   cb_centerpiece      = new US_SelectBox( this );
   centerpieceInfo();
   cb_centerpiece -> load();

   // External program to enter solution information
   pb_solution         = us_pushbutton( tr( "Manage Solutions"       ), false );
   pb_applyAll         = us_pushbutton( tr( "Apply to All"           ), false );
   // Defining data subsets
   pb_define           = us_pushbutton( tr( "Define Subsets"         ), false );
   pb_process          = us_pushbutton( tr( "Process Subsets"        ), false );
   // Choosing reference channel
   pb_reference        = us_pushbutton( tr( "Define Reference Scans" ), false );
   pb_cancelref        = us_pushbutton( tr( "Undo Reference Scans"   ), false );
   // Define intensity profile
   pb_intensity        = us_pushbutton( tr( "Show Intensity Profile" ), false );
   // Drop Triples or Channel or Cell/Channel
   pb_dropTrips        = us_pushbutton( tr( "Drop Selected Triples"  ), false );
   pb_dropCelch        = us_pushbutton( tr( "Drop Selected Data"     ), false );
   pb_dropChan         = us_pushbutton( tr( "Drop All Channel 'A's"  ), false );
   // Document solutio
   QLabel* lb_solution = us_label(      tr( "Solution:" ) );
   le_solutionDesc     = us_lineedit(   "", 1, true );
   // Scan Controls
   lb_scan             = us_banner(     tr( "Scan Controls" ) );
   // Scan focus from
   lb_from             = us_label(      tr( "Scan Focus from:" ), 0 );
   lb_from->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
   ct_from             = us_counter ( 3, 0.0, 0.0 ); // Update range upon load
   ct_from->setSingleStep( 1 );
   // Scan focus to
   lb_to               = us_label(      tr( "Scan Focus to:"   ), 0 );
   lb_to->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
   ct_to = us_counter ( 3, 0.0, 0.0 ); // Update range upon load
   ct_to->setSingleStep( 1 );

   // Exclude and Include pushbuttons
   pb_exclude          = us_pushbutton( tr( "Exclude Scan(s)" ), false );
   pb_include          = us_pushbutton( tr( "Include All"     ), false );
   pb_include ->setEnabled( false );
   // Standard pushbuttons
   QPushButton* pb_reset  = us_pushbutton( tr( "Reset" ) );
   QPushButton* pb_help   = us_pushbutton( tr( "Help" ) );
   pb_saveUS3             = us_pushbutton( tr( "Save" ) );
   QPushButton* pb_close  = us_pushbutton( tr( "Close" ) );

   // Add widgets to layouts
   settings ->addWidget( lb_DB,           row++, 0, 1, 4 );
   settings ->addWidget( wg_investigator, row,   0, 1, 2 );
   settings ->addWidget( le_investigator, row++, 2, 1, 2 );
   settings ->addLayout( disk_controls,   row++, 0, 1, 4 );
   settings ->addWidget( lb_run,          row++, 0, 1, 4 );
   settings ->addWidget( pb_import,       row,   0, 1, 2 );
   settings ->addWidget( pb_editRuninfo,  row++, 2, 1, 2 );
   settings ->addWidget( pb_loadUS3,      row,   0, 1, 2 );
   settings ->addWidget( pb_showTmst,     row,   2, 1, 1 );
   settings ->addWidget( pb_details,      row++, 3, 1, 1 );
   settings ->addWidget( lb_tolerance,    row,   0, 1, 2 );
   settings ->addWidget( ct_tolerance,    row++, 2, 1, 2 );
   settings ->addWidget( lb_mwlctrl,      row++, 0, 1, 4 );
   settings ->addWidget( lb_lambstrt,     row,   0, 1, 1 );
   settings ->addWidget( cb_lambstrt,     row,   1, 1, 1 );
   settings ->addWidget( lb_lambstop,     row,   2, 1, 1 );
   settings ->addWidget( cb_lambstop,     row++, 3, 1, 1 );
   settings ->addWidget( lb_lambplot,     row,   0, 1, 1 );
   settings ->addWidget( cb_lambplot,     row,   1, 1, 1 );
   settings ->addWidget( pb_lambprev,     row,   2, 1, 1 );
   settings ->addWidget( pb_lambnext,     row++, 3, 1, 1 );

   ccw      ->addWidget( lb_runinfo,      row++, 0, 1, 12 );
   ccw      ->addWidget( lb_runID2,       row,   0, 1,  3 );
   ccw      ->addWidget( le_runID2,       row++, 3, 1,  9 );
   ccw      ->addWidget( lb_dir,          row,   0, 1,  3 );
   ccw      ->addWidget( le_dir,          row++, 3, 1,  9 );
   ccw      ->addWidget( lb_triple,       row++, 0, 1, 12 );
   ccw      ->addWidget( lb_description,  row,   0, 1,  3 );
   ccw      ->addWidget( le_description,  row++, 3, 1,  9 );
   ccw      ->addWidget( lw_triple,       row,   0, 7,  4 );
   ccw      ->addWidget( lb_ccwinfo,      row++, 4, 1,  8 );
   ccw      ->addWidget( cb_centerpiece,  row++, 4, 1,  8 );
   ccw      ->addWidget( pb_solution,     row,   4, 1,  4 );
   ccw      ->addWidget( pb_applyAll,     row++, 8, 1,  4 );
   ccw      ->addWidget( pb_define,       row,   4, 1,  4 );
   ccw      ->addWidget( pb_process,      row++, 8, 1,  4 );
   ccw      ->addWidget( pb_reference,    row,   4, 1,  4 );
   ccw      ->addWidget( pb_cancelref,    row++, 8, 1,  4 );
   ccw      ->addWidget( pb_intensity,    row,   4, 1,  4 );
   ccw      ->addWidget( pb_dropTrips,    row++, 8, 1,  4 );
   ccw      ->addWidget( pb_dropCelch,    row,   4, 1,  4 );
   ccw      ->addWidget( pb_dropChan,     row++, 8, 1,  4 );
   ccw      ->addWidget( lb_solution,     row,   0, 1,  3 );
   ccw      ->addWidget( le_solutionDesc, row++, 3, 1,  9 );
   ccw      ->addWidget( lb_status,       row,   0, 1,  2 );
   ccw      ->addWidget( le_status,       row++, 2, 1, 10 );

   settings ->addLayout( ccw,             row++, 0, 1, 4 );

   settings ->addWidget( pb_reset,        row,   0, 1, 1 );
   settings ->addWidget( pb_help,         row,   1, 1, 1 );
   settings ->addWidget( pb_saveUS3,      row,   2, 1, 1 );
   settings ->addWidget( pb_close,        row++, 3, 1, 1 );

   // Plot layout for the right side of window
   QBoxLayout* plot = new US_Plot( data_plot,
                                   tr( "Absorbance Data" ),
                                   tr( "Radius (in cm)" ),
                                   tr( "Absorbance" ) );

   data_plot->setMinimumSize( 500, 300 );

   data_plot->enableAxis( QwtPlot::xBottom, true );
   data_plot->enableAxis( QwtPlot::yLeft  , true );

   data_plot->setAxisScale( QwtPlot::xBottom, 5.7, 7.3 );
   data_plot->setAxisScale( QwtPlot::yLeft  , 0.0, 1.5 );

   picker = new US_PlotPicker( data_plot );
   picker ->setRubberBand    ( QwtPicker::VLineRubberBand );
   picker->setMousePattern   ( QwtEventPattern::MouseSelect1,
                               Qt::LeftButton, Qt::ControlModifier );

   QGridLayout* todo = new QGridLayout();

   // Instructions ( missing to do items )
   QLabel* lb_todoinfo = us_banner( tr( "Instructions ( to do list )" ), 0 );
   lw_todoinfo = us_listwidget();
   lw_todoinfo->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
   lw_todoinfo->setMaximumHeight ( 90 );
   lw_todoinfo->setSelectionMode( QAbstractItemView::NoSelection );

   // Scan controls:
   todo->addWidget( lb_scan,         row++, 0, 1, 4 );
   todo->addWidget( lb_from,         row,   0, 1, 2 );
   todo->addWidget( ct_from,         row++, 2, 1, 2 );
   todo->addWidget( lb_to,           row,   0, 1, 2 );
   todo->addWidget( ct_to,           row++, 2, 1, 2 );
   todo->addWidget( pb_exclude,      row,   0, 1, 2 );
   todo->addWidget( pb_include,      row++, 2, 1, 2 );
   todo->addWidget( lb_todoinfo,     row++, 0, 1, 4 );
   todo->addWidget( lw_todoinfo,     row++, 0, 1, 4 );

   // Connect signals and slots
   if ( isadmin )
      connect( pb_investigator, SIGNAL( clicked()          ),
                                SLOT(   sel_investigator() ) );
   connect( disk_controls,  SIGNAL( changed       ( bool ) ),
                            SLOT  ( source_changed( bool ) ) );
   connect( pb_import,      SIGNAL( clicked()       ),
                            SLOT(   import()        ) );
   connect( pb_editRuninfo, SIGNAL( clicked()       ),
                            SLOT(   editRuninfo()   ) );
   connect( pb_loadUS3,     SIGNAL( clicked()       ),
                            SLOT(   loadUS3()       ) );
   connect( pb_showTmst,    SIGNAL( clicked()       ),
                            SLOT(   showTimeState() ) );
   connect( pb_details,     SIGNAL( clicked()       ),
                            SLOT(   runDetails()    ) );
   connect( ct_tolerance,   SIGNAL( valueChanged         ( double ) ),
                            SLOT  ( toleranceValueChanged( double ) ) );
   connect( le_description, SIGNAL( textEdited( QString )      ),
                            SLOT  ( changeDescription()        ) );
   connect( lw_triple,      SIGNAL( itemSelectionChanged()     ),
                            SLOT  ( changeTriple()             ) );
   connect( cb_centerpiece, SIGNAL( activated          ( int ) ),
                            SLOT  ( getCenterpieceIndex( int ) ) );
   connect( pb_solution,    SIGNAL( clicked()          ),
                            SLOT(   getSolutionInfo()  ) );
   connect( pb_applyAll,    SIGNAL( clicked()          ),
                            SLOT(   tripleApplyAll()   ) );
   connect( pb_define,      SIGNAL( clicked()          ),
                            SLOT(   define_subsets()   ) );
   connect( pb_process,     SIGNAL( clicked()          ),
                            SLOT(   process_subsets()  ) );
   connect( pb_reference,   SIGNAL( clicked()          ),
                            SLOT(   define_reference() ) );
   connect( pb_cancelref,   SIGNAL( clicked()          ),
                            SLOT(   cancel_reference() ) );
   connect( pb_intensity,   SIGNAL( clicked()          ),
                            SLOT(   show_intensity()   ) );
   connect( pb_dropTrips,   SIGNAL( clicked()          ),
                            SLOT(   drop_reference()   ) );
   connect( pb_dropCelch,   SIGNAL( clicked()          ),
                            SLOT(   drop_cellchan()    ) );
   connect( pb_dropChan,    SIGNAL( clicked()          ),
                            SLOT(   drop_channel()     ) );
   connect( pb_exclude,     SIGNAL( clicked()          ),
                            SLOT(   exclude_scans()    ) );
   connect( pb_include,     SIGNAL( clicked()  ),
                            SLOT(   include()  ) );
   connect( pb_reset,       SIGNAL( clicked()  ),
                            SLOT(   resetAll() ) );
   connect( pb_help,        SIGNAL( clicked()  ),
                            SLOT(   help()     ) );
   connect( pb_saveUS3,     SIGNAL( clicked()  ),
                            SLOT(   saveUS3()  ) );
   connect( pb_close,       SIGNAL( clicked()  ),
                            SLOT(   close() )  );

   // Now let's assemble the page

   QVBoxLayout* left     = new QVBoxLayout;

   left->addLayout( settings );

   QVBoxLayout* right    = new QVBoxLayout;

   right->addLayout( plot );
   right->addLayout( todo );

   QHBoxLayout* main = new QHBoxLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   main->addLayout( left );
   main->addLayout( right );
   main->setStretchFactor( left,  3 );
   main->setStretchFactor( right, 5 );

DbgLv(1) << "CGui: GUI setup complete";
   reset();
DbgLv(1) << "CGui: reset complete";
}

void US_ConvertGui::reset( void )
{
   lw_triple       ->clear();

   le_dir          ->setText( "" );

   le_description  ->setText( "" );
   le_runID        ->setText( "" );
   le_runID2       ->setText( "" );
   le_solutionDesc ->setText( "" );

   pb_import     ->setEnabled( true  );
   pb_loadUS3    ->setEnabled( true  );
   pb_exclude    ->setEnabled( false );
   pb_include    ->setEnabled( false );
   pb_showTmst   ->setEnabled( false );
   pb_details    ->setEnabled( false );
   pb_intensity  ->setEnabled( false );
   pb_cancelref  ->setEnabled( false );
   pb_dropTrips  ->setEnabled( false );
   pb_dropCelch  ->setEnabled( false );
   pb_dropChan   ->setEnabled( false );
   pb_solution   ->setEnabled( false );
   pb_editRuninfo->setEnabled( false );
   pb_applyAll   ->setEnabled( false );
   pb_saveUS3    ->setEnabled( false );

   ct_tolerance  ->setEnabled( true  );

   cb_centerpiece->setEnabled( false );

   ct_from       ->disconnect();
   ct_from       ->setMinimum ( 0 );
   ct_from       ->setMaximum ( 0 );
   ct_from       ->setValue   ( 0 );

   ct_to         ->disconnect();
   ct_to         ->setMinimum ( 0 );
   ct_to         ->setMaximum ( 0 );
   ct_to         ->setValue   ( 0 );

   // Clear any data structures
   legacyData  .clear();
   allExcludes .clear();
   all_tripinfo.clear();
   all_chaninfo.clear();
   all_triples .clear();
   all_channels.clear();
   out_tripinfo.clear();
   out_chaninfo.clear();
   out_triples .clear();
   out_channels.clear();
   allData     .clear();
   outData     .clear();
   ExpData     .clear();
   if ( isMwl )
      mwl_data.clear();
   show_plot_progress = true;

   // Erase the todo list
   lw_todoinfo->clear();
   lw_todoinfo->addItem( "Load or import some AUC data" );

   dataPlotClear( data_plot );
   picker        ->disconnect();
   data_plot     ->setAxisScale( QwtPlot::xBottom, 5.7, 7.3 );
   data_plot     ->setAxisScale( QwtPlot::yLeft  , 0.0, 1.5 );
   grid          = us_grid( data_plot );
   data_plot     ->replot();

   pb_define     ->setEnabled( false );
   pb_process    ->setEnabled( false );
   step          = NONE;

   enableRunIDControl( true );

   toleranceChanged = false;
   saveStatus       = NOT_SAVED;
   isPseudo         = false;
   isMwl            = false;

   pb_reference   ->setEnabled( false );
   referenceDefined = false;
DbgLv(1) << "CGui: (1)referDef=" << referenceDefined;

   // Display investigator
   ExpData.invID = US_Settings::us_inv_ID();

   QString number = ( ExpData.invID > 0 )
      ?  QString::number( ExpData.invID ) + ": "
      : "";

   le_investigator->setText( number + US_Settings::us_inv_name() );
   show_mwl_control( false );
}

void US_ConvertGui::resetAll( void )
{
   QApplication::restoreOverrideCursor();
   QApplication::restoreOverrideCursor();

   if ( allData.size() > 0 )
   {  // Output warning when resetting (but only if we have data)
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

   le_status->setText( tr( "(no data loaded)" ) );
   subsets.clear();
   reference_start = 0;
   reference_end   = 0;

   connectTolerance( false );
   ct_tolerance    ->setMinimum (   0.0 );
   ct_tolerance    ->setMaximum ( 100.0 );
   ct_tolerance    ->setValue   (   5.0 );
   ct_tolerance    ->setSingleStep( 1 );
   connectTolerance( true );
   scanTolerance   = 5.0;
   runID           = "";
   data_plot->setTitle( tr( "Absorbance Data" ) );
}

// Function to select the current investigator
void US_ConvertGui::sel_investigator( void )
{
   US_Investigator* inv_dialog = new US_Investigator( true, ExpData.invID );

   connect( inv_dialog,
      SIGNAL( investigator_accepted( int ) ),
      SLOT  ( assign_investigator  ( int ) ) );

   inv_dialog->exec();
}

// Function to assign the selected investigator as current
void US_ConvertGui::assign_investigator( int invID )
{
   ExpData.invID = invID;

   QString number = ( invID > 0 )
                    ? QString::number( invID ) + ": "
                    : "";

   le_investigator->setText( number + US_Settings::us_inv_name() );

}

// Function to change the data source (disk/db)
void US_ConvertGui::source_changed( bool )
{
   QStringList DB = US_Settings::defaultDB();

   if ( DB.size() < 5 )
   {
      QMessageBox::warning( this,
         tr( "Attention" ),
         tr( "There is no default database set." ) );
   }

   // Did we switch from disk to db?
   if ( disk_controls->db() && ! save_diskDB )
   {
      // Make sure these are explicitly chosen
      ExpData.operatorID = 0;
      ExpData.rotorID    = 0;
      ExpData.calibrationID = 0;
   }

   save_diskDB = disk_controls->db();

   // Reinvestigate whether to enable the buttons
   enableControls();
}

void US_ConvertGui::update_disk_db( bool db )
{
   save_diskDB = disk_controls->db();

   ( db ) ? disk_controls->set_db() : disk_controls->set_disk();
}

// User changed the dataset separation tolerance
void US_ConvertGui::toleranceValueChanged( double )
{
   toleranceChanged = true;
   scanTolerance    = ct_tolerance->value();
   reimport();
}

// User pressed the import data button
void US_ConvertGui::import()
{
   int impType = getImports();

   if ( impType == 1 )
   {
      importMWL();
      return;
   }

   else if ( impType == 2 )
   {
      importAUC();
      return;
   }

DbgLv(1) << "CGui:IMP: IN";
   bool success = false;
   le_status->setText( tr( "Importing experimental data ..." ) );

   success = read();                // Read the legacy data

   if ( ! success ) return;

   // Define default tolerances before converting
   scanTolerance = ( runType == "WA" ) ? 0.1 : 5.0;
   connectTolerance( false );
   ct_tolerance->setValue( scanTolerance );
   connectTolerance( true );

   // Figure out all the triple combinations and convert data
   success = convert();

   if ( ! success ) return;

   // Initialize export data pointers vector
   success = init_output_data();

   if ( ! success ) return;

   setTripleInfo();

   checkTemperature();          // Check to see if temperature varied too much

   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   // Initialize exclude list
DbgLv(1) << "CGui:IMP: init_excludes CALL";
   init_excludes();

   plot_current();

   QApplication::restoreOverrideCursor();

   connect( ct_from, SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_from   ( double ) ) );

   connect( ct_to  , SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_to     ( double ) ) );

   saveStatus = NOT_SAVED;

   // Ok to enable some buttons now
DbgLv(1) << "CGui:IMP: enableControls CALL";
   enableControls();

   if ( runType == "RI" )
   {
      referenceDefined = false;
      pb_reference->setEnabled( true );
DbgLv(1) << "CGui: (2)referDef=" << referenceDefined;
   }
DbgLv(1) << "CGui: import: RTN";
   le_status->setText( tr( "Legacy data has been imported." ) );
}

// User pressed the reimport data button
// Legacy data is already supposed to be present
void US_ConvertGui::reimport( void )
{
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   if ( toleranceChanged )
   {
      // If the tolerance has changed, we need to reconvert the data
      toleranceChanged = false;
      bool success = false;

      all_tripinfo.clear();

      le_solutionDesc ->setText( "" );

      // Figure out all the triple combinations and convert data
      success = convert();

      if ( success )
         success = init_output_data();

      if ( ! success )
      {
         QApplication::restoreOverrideCursor();
         return;
      }

      setTripleInfo();
   }

   // Initialize exclude list
   init_excludes();

   // In this case the runType is not changing
   oldRunType = runType;

   pb_include->setEnabled( false );
   pb_exclude->setEnabled( false );
   plot_current();
   QApplication::restoreOverrideCursor();

   connect( ct_from, SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_from   ( double ) ) );

   connect( ct_to  , SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_to     ( double ) ) );

   // Ok to enable some buttons now
   enableControls();

   if ( runType == "RI" )
   {
      referenceDefined = false;
      pb_reference->setEnabled( true );
DbgLv(1) << "CGui: (3)referDef=" << referenceDefined;
   }
}

// Import sub-directory contains MWL data
void US_ConvertGui::importMWL( void )
{
DbgLv(1) << "CGui:iM: IN";
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   // Read the data
   le_status->setText( tr( "Importing MWL data ..." ) );
   qApp->processEvents();
DbgLv(1) << "CGui:iM: import_data";
   isMwl       = mwl_data.import_data( currentDir, le_status );

   if ( !isMwl  &&  le_status->text().contains( tr( "duplicate" ) ) )
   {  // If import problem was duplicate wavelengths, report it.
      QMessageBox::warning( this,
            tr( "Duplicate Wavelengths" ),
            tr( "Duplicate wavelengths were encountered in the import"
                " of the Multi-WaveLength data.\n\n"
                "All duplicate triples were skipped." ) );
      isMwl       = true;
   }

   if ( !isMwl )
   {  // Return immediately if there were MWL import problems
      qApp->processEvents();
      QApplication::restoreOverrideCursor();
      QApplication::restoreOverrideCursor();
      le_status->setText( tr( "MWL import error (no data loaded)" ) );
      qApp->processEvents();
      return;
   }

DbgLv(1) << "CGui:iM:  RTN: import_data";
   isMwl       = true;
   runType     = "RI";
   mwl_data.run_values( runID, runType );
DbgLv(1) << "CGui:iM:  runID runType" << runID << runType;
   QApplication::restoreOverrideCursor();

   // if runType has changed, let's clear out xml data too
//   if ( oldRunType != runType ) ExpData.clear();
   le_runID2->setText( runID );
   le_runID ->setText( runID );
   le_dir   ->setText( currentDir );
   ExpData.runID = runID;

   // Define default tolerances before converting
   connectTolerance( false );
   scanTolerance = ( runType == "WA" ) ? 0.1 : 5.0;
   ct_tolerance->setValue( scanTolerance );
   connectTolerance( true );

DbgLv(1) << "CGui:iM: show_mwl_control";
   show_mwl_control( true );
DbgLv(1) << "CGui:iM:  RTN:show_mwl_control";
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   // Set initial lambda range; build the output data
   le_status->setText( tr( "Duplicating wavelengths ..." ) );
   qApp->processEvents();
   slambda       = mwl_data.countOf( "slambda" );
   elambda       = mwl_data.countOf( "elambda" );
DbgLv(1) << "CGui:iM: set_lambd sl el" << slambda << elambda;
   mwl_data.set_lambdas   ( slambda, elambda );

   le_status->setText( tr( "Building raw data AUCs ..." ) );
   qApp->processEvents();
DbgLv(1) << "CGui:iM: buildraw";
   mwl_data.build_rawData ( allData );
DbgLv(1) << "CGui:iM: init_out";
   if ( ! init_output_data() )
      return;

   le_status->setText( tr( "Building Lambda list ..." ) );
   qApp->processEvents();

   // Set up MWL data object after import
DbgLv(1) << "CGui:iM: mwlsetup";
   mwl_setup();

   // Point to any existing time state file
   QDir ddir( currentDir );
   QStringList tmsfs = ddir.entryList( QStringList( "*.time_state.*" ),
         QDir::Files, QDir::Name );
   QString defs_fnamei;
   QString tmst_dname  = currentDir;
   if ( tmst_dname.right( 1 ) != "/" )
      tmst_dname   += "/";

   if ( tmsfs.count() == 2 )
   {  // Looks like we have a TMST and corresponding Defs XML
      tmst_fnamei   = tmsfs[ 0 ];
      defs_fnamei   = tmsfs[ 1 ];

      if ( tmst_fnamei.contains( ".tmst" )  &&
           defs_fnamei.contains( ".xml" ) )
      {  // Have both files, so save full path to TMST
         tmst_fnamei   = tmst_dname + tmst_fnamei;
      }

      else if ( tmsfs[ 0 ].contains( ".xml" )  &&
                tmsfs[ 1 ].contains( ".tmst" ) )
      {  // Have both files (in opposite order), so save full path to TMST
         tmst_fnamei   = tmst_dname + tmsfs[ 1 ];
      }

      else
      {  // Do not have both files, so clear TMST file path
         tmst_fnamei.clear();
      }
   }

   else  // If file does not exist, clear name
   {
      tmst_fnamei.clear();
   }

//   le_status->setText( tr( "MWL Import IS COMPLETE." ) );
   qApp->processEvents();
   QApplication::restoreOverrideCursor();
DbgLv(1) << "CGui:iM:  DONE";
}

// Import simulation data in AUC form
void US_ConvertGui::importAUC( void )
{
   QString importDir = currentDir;
   QDir readDir( importDir );

   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   le_status->setText( tr( "Loading AUC simulation data ..." ) );
   QStringList nameFilters = QStringList( "*.auc" );
   QStringList files =  readDir.entryList( nameFilters,
         QDir::Files | QDir::Readable, QDir::Name );
   allData     .clear();
   all_tripinfo.clear();

   for ( int trx = 0; trx < files.size(); trx++ )
   {
      QString fname  = files[ trx ];
      QString fpath  = importDir + fname;
      US_DataIO::RawData     rdata;
      US_Convert::TripleInfo tripinfo;

      // Get a raw data from an auc file
      US_DataIO::readRawData( fpath, rdata );

      // Give it a new unique GUID, since we are making a copy
      QString uuidst = US_Util::new_guid();
      US_Util::uuid_parse( uuidst, (unsigned char*)&rdata.rawGUID );
DbgLv(1) << "rIA: trx" << trx << "uuid" << uuidst << importDir;

      // Save the raw data for this triple
      allData << rdata;

      // Save triple information
      QString triple   = QString::number( rdata.cell ) + " / "
         + QString( rdata.channel ) + " / "
         + QString::number( qRound( rdata.scanData[ 0 ].wavelength ) );
      tripinfo.tripleID    = trx + 1;
      tripinfo.tripleDesc  = triple;
      tripinfo.description = rdata.description;
      tripinfo.channelID   = trx + 1;
      tripinfo.excluded    = false;

      all_tripinfo << tripinfo;
   }
   qApp->processEvents();
   QApplication::restoreOverrideCursor();

   QString fname = files[ 0 ];
   runType       = QString( fname ).section( ".", -5, -5 );
   runID         = QString( fname ).section( ".",  0, -6 );
   le_runID2->setText( runID );
   le_runID ->setText( runID );
   scanTolerance = 5.0;

   if ( ! init_output_data() )
      return;

   setTripleInfo();
   le_description -> setText( allData[ 0 ].description );
   init_excludes();
   plot_current();
   saveStatus    = NOT_SAVED;
   enableControls();

   // Point to any existing time state file
   QDir ddir( currentDir );
   QStringList tmsfs = ddir.entryList( QStringList( "*.time_state.*" ),
         QDir::Files, QDir::Name );
   QString defs_fnamei;
   QString tmst_dname  = currentDir;
   if ( tmst_dname.right( 1 ) != "/" )
      tmst_dname   += "/";
DbgLv(1) << "rTS: tmst_dname" << tmst_dname << "tmsfs count" << tmsfs.count();

   if ( tmsfs.count() == 2 )
   {  // Looks like we have a TMST and corresponding Defs XML
      tmst_fnamei   = tmsfs[ 0 ];
      defs_fnamei   = tmsfs[ 1 ];
DbgLv(1) << "rTS: tmsfs" << tmst_fnamei << defs_fnamei;

      if ( tmst_fnamei.contains( ".tmst" )  &&
           defs_fnamei.contains( ".xml" ) )
      {  // Have both files, so save full path to TMST
         tmst_fnamei   = tmst_dname + tmst_fnamei;
      }

      else if ( tmsfs[ 0 ].contains( ".xml" )  &&
                tmsfs[ 1 ].contains( ".tmst" ) )
      {  // Have both files (in opposite order), so save full path to TMST
         tmst_fnamei   = tmst_dname + tmsfs[ 1 ];
      }

      else
      {  // Do not have both files, so clear TMST file path
         tmst_fnamei.clear();
      }
DbgLv(1) << "rTS: tmst,defs fnamei" << tmst_fnamei << defs_fnamei;
   }

   else  // If file does not exist, clear name
   {
DbgLv(1) << "rTS: NON_EXIST:" << tmst_fnamei;
      tmst_fnamei.clear();
   }

   le_status->setText( tr( "AUC simulation data import IS COMPLETE." ) );
   pb_showTmst->setEnabled( ! tmst_fnamei.isEmpty() );
}

// Enable the common dialog controls when there is data
void US_ConvertGui::enableControls( void )
{
   if ( allData.size() == 0 )
      reset();

   else
   {
DbgLv(1) << "CGui: enabCtl: have-data" << allData.size() << all_tripinfo.size();
      // Ok to enable some buttons now
      pb_showTmst    ->setEnabled( ! tmst_fnamei.isEmpty() );
      pb_details     ->setEnabled( true );
      pb_solution    ->setEnabled( true );
      cb_centerpiece ->setEnabled( true );
      pb_editRuninfo ->setEnabled( true );

      // Ok to drop scan if not the only one
      int currentScanCount = 0;

      for ( int i = 0; i < all_tripinfo.size(); i++ )
         if ( ! all_tripinfo[ i ].excluded ) currentScanCount++;

      bool drops      = ( currentScanCount > 1 );
      pb_dropTrips   ->setEnabled( drops );
      pb_dropCelch   ->setEnabled( drops && isMwl );
      pb_dropChan    ->setEnabled( drops && isMwl );

      if ( runType == "RI" )
         pb_reference->setEnabled( ! referenceDefined );

      if ( subsets.size() < 1 )
      {
         // Allow user to define subsets, if he hasn't already
         pb_define   ->setEnabled( true );
      }

      // Disable load buttons if there is data
      pb_import ->setEnabled( false );
      pb_loadUS3->setEnabled( false );

      // Most triples are ccw
      lb_triple   ->setText( tr( "Cell / Channel / Wavelength" ) );
      connectTolerance( false );
      ct_tolerance->setNumButtons  ( 2 );
      ct_tolerance->setRange       ( 0.0, 100.0 );
      ct_tolerance->setSingleStep  ( 1.0 );
      ct_tolerance->setValue       ( scanTolerance );

      // Default tolerances are different for wavelength data
      if ( runType == "WA" )
      {
         // First of all, wavelength triples are ccr.
         lb_triple   ->setText( tr( "Cell / Channel / Radius" ) );
         ct_tolerance->setNumButtons  ( 3 );
         ct_tolerance->setRange       ( 0.0, 10.0 );
         ct_tolerance->setSingleStep  ( 0.001 );
         ct_tolerance->setValue       ( scanTolerance );
      }

      connectTolerance( true );

      // Let's calculate if we're eligible to copy this triple info to all
      // or to save it
      // We have to check against GUID's, because solutions won't have
      // solutionID's yet if they are created as needed offline
      QRegExp rx( "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$" );

      pb_applyAll     -> setEnabled( false );
//      if ( all_tripinfo.size() > 1  &&
//           rx.exactMatch( all_tripinfo[ tripListx ].solution.solutionGUID ) )
      if ( out_chaninfo.size() > 1  &&
           rx.exactMatch( out_chaninfo[ tripListx ].solution.solutionGUID ) )
      {
         pb_applyAll  -> setEnabled( true );
      }

      enableRunIDControl( saveStatus == NOT_SAVED );

      enableSaveBtn();
DbgLv(1) << "CGui: enabCtl: enabSvBtn complete";
   }
}

// Enable or disable the runID control
void US_ConvertGui::enableRunIDControl( bool setEnable )
{
   if ( setEnable )
   {
      us_setReadOnly( le_runID2, false );
      connect( le_runID2, SIGNAL( textEdited( QString ) ),
                          SLOT  ( runIDChanged(  )      ) );
   }

   else
   {
      le_runID2->disconnect();
      us_setReadOnly( le_runID2, true );
   }

}

// Reset the boundaries on the scan controls
void US_ConvertGui::enableScanControls( void )
{
DbgLv(1) << "CGui:enabScContr: isMwl" << isMwl;
//   if ( isMwl )  return;
   triple_index();
DbgLv(1) << "CGui:enabScContr: trx dax" << tripListx << tripDatax;

   ct_from->disconnect();
   ct_from->setMinimum ( 0.0 );
   ct_from->setMaximum (  outData[ tripDatax ]->scanData.size()
                        - allExcludes[ tripDatax ].size() );
   ct_from->setValue   ( 0 );

   ct_to  ->disconnect();
   ct_to  ->setMinimum ( 0.0 );
   ct_to  ->setMaximum (  outData[ tripDatax ]->scanData.size()
                        - allExcludes[ tripDatax ].size() );
   ct_to  ->setValue   ( 0 );

   connect( ct_from, SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_from   ( double ) ) );

   connect( ct_to  , SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_to     ( double ) ) );

}

// Enable the "save" button, if appropriate
// Let's use the same logic to populate the todo list too
void US_ConvertGui::enableSaveBtn( void )
{
   lw_todoinfo->clear();
   int count = 0;
   bool completed = true;
DbgLv(1) << " enabCtl: tLx infsz" << tripListx << out_chaninfo.count();
   cb_centerpiece->setLogicalIndex( out_chaninfo[ tripListx ].centerpiece );

   if ( allData.size() == 0 )
   {
      count++;
      lw_todoinfo->addItem( QString::number( count )
            + tr( ": Load or import some AUC data" ) );
      completed = false;
   }

   // Do we have any triples?
   if ( all_tripinfo.size() == 0 )
   {
      count++;
      lw_todoinfo->addItem( QString::number( count )
            + tr( ": Load or import some AUC data" ) );
      completed = false;
   }

   // Is the run info defined?
   QRegExp rx( "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$" );

   // Not checking operator on disk -- defined as "Local"
   if ( ( ExpData.rotorID == 0 )              ||
        ( ExpData.calibrationID == 0 )        ||
        ( ExpData.labID == 0 )                ||
        ( ExpData.instrumentID == 0 )         ||
        ( ExpData.label.isEmpty() )           ||
        ( ! rx.exactMatch( ExpData.project.projectGUID ) ) )
   {
      if ( ! referenceDefined )
      {
         count++;
         lw_todoinfo->addItem( QString::number( count )
               + tr( ": Modify run ID (optional)" ) );
      }
      count++;
      lw_todoinfo->addItem( QString::number( count )
            + tr( ": Edit run information" ) );
      completed = false;
   }

   // Have we filled out all the c/c/w info?
   // Check GUIDs, because solutionID's may not be present yet.
   foreach ( US_Convert::TripleInfo tripinfo, out_chaninfo )
   {
      if ( ! rx.exactMatch( tripinfo.solution.solutionGUID ) )
      {
         count++;
         lw_todoinfo->addItem( QString::number( count ) +
                               tr( ": Select solution for triple " ) +
                               tripinfo.tripleDesc );
         completed = false;
      }
   }

   foreach ( US_Convert::TripleInfo tripinfo, out_chaninfo )
   {
      if ( tripinfo.centerpiece == 0 )
      {
         count++;
         lw_todoinfo->addItem( QString::number( count ) +
                               tr( ": Select centerpiece for triple " ) +
                               tripinfo.tripleDesc );
         completed = false;
      }
   }

   if ( disk_controls->db() )
   {
      // Verify connectivity
      US_Passwd pw;
      QString masterPW = pw.getPasswd();
      US_DB2 db( masterPW );

      if ( db.lastErrno() != US_DB2::OK )
      {
         count++;
         lw_todoinfo->addItem( QString::number( count ) +
                               tr( ": Verify database connectivity" ) );
         completed = false;
      }

      // Information is there, but we need to see if the runID exists in the
      // DB. If we didn't load it from there, then we shouldn't be able to sync
      int recStatus = ExpData.checkRunID( &db );

      // if a record is found but saveStatus==BOTH,
      //  then we are editing that record
      if ( ( recStatus == US_DB2::OK ) && ( saveStatus != BOTH ) ) // ||
           // ( ! ExpData.syncOK ) )
      {
         count++;
         lw_todoinfo->addItem( QString::number( count ) +
                               tr( ": Select a different runID" ) );
         completed = false;
      }

      // Not checking operator on disk -- defined as "Local"
      if ( ExpData.operatorID == 0 )
      {
         count++;
         lw_todoinfo->addItem( QString::number( count )
               + tr( ": Select operator in run information" ) );
         completed = false;
      }

   }

   // This can go on the todo list, but should not prevent user from saving
   if ( ( runType == "RI" ) && ( ! referenceDefined ) )
   {
      count++;
      lw_todoinfo->addItem( QString::number( count ) +
                            tr( ": Define reference scans" ) );

      if ( count == 1 )
         pb_reference->setEnabled( true );
   }

   // If we made it here, user can save
   pb_saveUS3 ->setEnabled( completed );
}

// Process when the user changes the runID
void US_ConvertGui::runIDChanged( void )
{
   // See if we need to update the runID
   QRegExp rx( "^[A-Za-z0-9_-]{1,80}$" );
   QString new_runID = le_runID2->text();

   if ( rx.indexIn( new_runID ) >= 0 )
   {
      runID = new_runID;
      if ( runID.length() > 50 )
      {
         QMessageBox::warning( this,
               tr( "RunID Name Too Long" ),
               tr( "The runID name may be at most\n"
                   "50 characters in length." ) );
         runID = runID.left( 50 );
      }
      plot_titles();
   }

   // If the runID has changed, a number of other things need to change too,
   // for instance GUID's.
   if ( referenceDefined )
      cancel_reference();
   ExpData.clear();
   foreach( US_Convert::TripleInfo tripinfo, all_tripinfo )
      tripinfo.clear();
   le_runID2->setText( runID );
   le_runID ->setText( runID );

   saveStatus = NOT_SAVED;

   // Set the directory too
   QDir resultDir( US_Settings::resultDir() );
   currentDir = resultDir.absolutePath() + "/" + runID + "/";
   le_dir ->setText( currentDir );
}

// Function to generate a new guid for experiment, and associate with DB
void US_ConvertGui::editRuninfo( void )
{
DbgLv(1) << "CGui: edRuninfo: IN";
   if ( saveStatus == NOT_SAVED )
   {
      // Create a new GUID for the experiment as a whole
      ExpData.expGUID = US_Util::new_guid();

   }

   getExpInfo( );
DbgLv(1) << "CGui: edRuninfo: getExpInfo complete";
}

// Function to load US3 data
void US_ConvertGui::loadUS3( )
{
   // Open a dialog to get the RunID from DB or Disk
   US_GetRun dialog( runID, disk_controls->db() );

   connect( &dialog, SIGNAL( dkdb_changed  ( bool ) ),
            this,    SLOT  ( update_disk_db( bool ) ) );

   if ( dialog.exec() == QDialog::Rejected )
      return;

   if ( runID == QString( "" ) )
      return;

   // Restore area beneath dialog
   qApp->processEvents();

DbgLv(1) << "CGui: ldUS3: IN";
   // Load the data
   if ( disk_controls->db() )
      loadUS3DB();

   else
      loadUS3Disk();

   checkTemperature();          // Check to see if temperature varied too much
DbgLv(1) << "CGui: ldUS3: RTN";

}

// Load AUC data from disk
void US_ConvertGui::loadUS3Disk( void )
{
   // Construct the full path to the run directory
   QString dir = US_Settings::resultDir() + "/" + runID;

   dir.replace( "\\", "/" );  // WIN32 issue
   if ( dir.right( 1 ) != "/" ) dir += "/"; // Ensure trailing /

   // Load data from disk
   loadUS3Disk( dir );
}

// Load AUC data from a specified directory on local disk
void US_ConvertGui::loadUS3Disk( QString dir )
{
   resetAll();
   le_status->setText( tr( "Loading data from local disk ..." ) );
   qApp->processEvents();

   // Check the runID
   QStringList components =  dir.split( "/", QString::SkipEmptyParts );
   QString new_runID      = components.last();

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
   runID       = new_runID;
   le_runID ->setText( runID );
   le_runID2->setText( runID );
   le_dir   ->setText( dir );
   currentDir  = QString( dir );

   // Reload the AUC data
   le_status->setText( tr( "Loading data from Disk (raw data) ..." ) );
   qApp->processEvents();
DbgLv(1) << "CGui: ldUS3Dk: call read";
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   int status = US_Convert::readUS3Disk( dir, allData, all_tripinfo, runType );
   QApplication::restoreOverrideCursor();

   if ( status == US_Convert::NODATA )
   {
      QMessageBox::warning( this,
            tr( "No Files Found" ),
            tr( "There were no files of the form *.auc\n"
                "found in the specified directory." ) );
      return;
   }

   if ( status == US_Convert::NOAUC )
   {
      QMessageBox::warning( this,
         tr( "UltraScan Error" ),
         tr( "Could not read data file.\n" ) );
      return;
   }

   // Now try to read the xml file
DbgLv(1) << "CGui: ldUS3Dk: call rdExp  sz(trinfo)" << all_tripinfo.count();
   le_status->setText( tr( "Loading data from Disk (experiment) ..." ) );
   qApp->processEvents();
   ExpData.clear();
   status = ExpData.readFromDisk( all_tripinfo, runType, runID, dir );

   if ( status == US_Convert::CANTOPEN )
   {
      QString readFile = runID      + "."
                       + runType    + ".xml";
      QMessageBox::information( this,
         tr( "Error" ),
         tr( "US3 run data ok, but unable to assocate run with DB.\n " ) +
         tr( "Cannot open read file: " ) + dir + readFile );
   }

   else if ( status == US_Convert::BADXML )
   {
      QString readFile = runID      + "."
                       + runType    + ".xml";
      QMessageBox::information( this,
         tr( "Error" ),
         tr( "US3 run data ok, but there is an error in association with DB.\n"
             "Improper XML in read file: " ) + dir + readFile );
   }

   else if ( status != US_Convert::OK )
   {
      QMessageBox::information( this,
         tr( "Error" ),
         tr( "Unknown error: " ) + status );
   }

   // Now that we have the experiment, let's read the rest of the
   //  solution and project information
   le_status->setText( tr( "Loading data from Disk (project) ..." ) );
   qApp->processEvents();
DbgLv(1) << "CGui: ldUS3Dk: call prj-rDk";
   status = ExpData.project.readFromDisk( ExpData.project.projectGUID );

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

   // and clear it out
   if ( status != US_DB2::OK )
      ExpData.project.clear();

   le_status->setText( tr( "Project and experiment are loaded." ) );
   qApp->processEvents();
   QString psolGUID = "";
DbgLv(1) << "CGui: SOLCHK:loop";

   // Now the solutions
   for ( int ii = 0; ii < all_tripinfo.size(); ii++ )
   {
      QString csolGUID = all_tripinfo[ ii ].solution.solutionGUID;

      if ( csolGUID == psolGUID )
      {
         all_tripinfo[ ii ].solution = all_tripinfo[ ii - 1 ].solution;
         status   = US_DB2::OK;
         continue;
      }
      else
      {
         if ( csolGUID.isEmpty() )
         {
DbgLv(1) << "SOLCHK:  ii csolGUID EMPTY" << ii << csolGUID;
            if ( ii > 0 )
            {
               csolGUID = psolGUID;
               all_tripinfo[ ii ].solution.solutionGUID = csolGUID;
            }
         }
         status   = all_tripinfo[ ii ].solution.readFromDisk( csolGUID );
         psolGUID = csolGUID;
      }

      // Error reporting
      if ( status == US_DB2::NO_SOLUTION )
      {
         QMessageBox::information( this,
            tr( "Attention" ),
            tr( "A solution this run refers to was not found,"
                " or could not be read.\n"
                "Please select an existing solution and try again.\n" ) );
DbgLv(1) << "SOLERR: ii psolGUID csolGUI" << ii << psolGUID << csolGUID;
      }

      else if ( status == US_DB2::NO_BUFFER )
      {
         QMessageBox::information( this,
            tr( "Attention" ),
            tr( "The buffer this solution refers to was not found.\n"
                "Please restore and try again.\n" ) );
      }

      else if ( status == US_DB2::NO_ANALYTE )
      {
         QMessageBox::information( this,
            tr( "Attention" ),
            tr( "One of the analytes this solution refers to was not found.\n"
                "Please restore and try again.\n" ) );
      }

      else if ( status != US_DB2::OK )
      {
         QMessageBox::information( this,
            tr( "Disk Read Problem" ),
            tr( "Could not read data from the disk.\n"
                "Disk status: " ) + QString::number( status ) );
      }

      // Just clear it out
      if ( status != US_DB2::OK )
         all_tripinfo[ ii ].solution.clear();
   }
DbgLv(1) << "CGui: SOLCHK:loop-END";

   le_status->setText( tr( "Solutions are now loaded." ) );
   qApp->processEvents();

   // Now read the RI Profile, if it exists
   if ( runType == "RI" )
   {
      referenceDefined = false;
      pb_reference->setEnabled( true );
DbgLv(1) << "CGui: (4)referDef=" << referenceDefined;

DbgLv(1) << "CGui: call rdRIDk";
      status = ExpData.readRIDisk( runID, dir );

      if ( status == US_Convert::CANTOPEN )
      {
         QString readFile = runID      + "."
                          + "RIProfile.xml";
         QMessageBox::information( this,
            tr( "Error" ),
            tr( "US3 run data ok, but unable to read intensity profile.\n " ) +
            tr( "Cannot open read file: " ) + dir + readFile );
      }

      else if ( status == US_Convert::BADXML )
      {
         QString readFile = runID      + "."
                          + "RIProfile.xml";
         QMessageBox::information( this,
            tr( "Error" ),
            tr( "US3 run data ok, but unable to read intensity profile.\n " ) +
            tr( "Improper XML in read file: " ) + dir + readFile );
      }

      else if ( status != US_Convert::OK )
      {
         QMessageBox::information( this,
            tr( "Error" ),
            tr( "Unknown error: " ) + status );
      }

      else
      {
         // Enable intensity plot
         pb_intensity->setEnabled( true );

         referenceDefined = true;
         isPseudo         = true;
DbgLv(1) << "CGui: (5)referDef=" << referenceDefined;
         pb_reference->setEnabled( false );
         pb_cancelref->setEnabled( true );
      }

   }

   if ( allData.size() == 0 ) return;

   le_status->setText( tr( "%1 data triples are NOW LOADED." )
                       .arg( allData.size() ) );
   qApp->processEvents();

   // Initialize export data pointers vector
   if ( ! init_output_data() )
      return;

DbgLv(1) << "CGui: ldUS3Dk: fromIOD: sz(trinfo)" << all_tripinfo.count()
 << "sz(chinfo)" << all_chaninfo.size();

   // Copy solution and centerpiece info to channel vectors
   int         cCenterpiece = all_tripinfo[ 0 ].centerpiece;
   US_Solution cSolution    = all_tripinfo[ 0 ].solution;
   int         idax         = 0;

   for ( int ii = 0; ii < all_chaninfo.size(); ii++ )
   {
      if ( idax != out_chandatx[ ii ] )
      {
         idax         = out_chandatx[ ii ];
         cCenterpiece = all_tripinfo[ idax ].centerpiece;
         cSolution    = all_tripinfo[ idax ].solution;
      }

      all_chaninfo[ ii ].centerpiece  = cCenterpiece;
      all_chaninfo[ ii ].solution     = cSolution;
DbgLv(1) << "CGui:     chan trip" << ii << idax
 << "centp sol" << cCenterpiece << cSolution.solutionGUID;
   }

   if ( isMwl )
   {  // If need be, load MWL data object
      mwl_data.load_mwl( allData );

      mwl_setup();
   }

   // Update triple information on screen
DbgLv(1) << "CGui: call setTripleInfo";
   setTripleInfo();
DbgLv(1) << "CGui: call init_excludes";
   init_excludes();

DbgLv(1) << "CGui:  setDesc trLx" << tripListx << out_chaninfo.size();
   le_solutionDesc->setText( all_tripinfo[ 0 ].solution.solutionDesc );

   // Restore description
   le_description->setText( allData[ 0 ].description );
   saveDescription = QString( allData[ 0 ].description );

   // Reset maximum scan control values
DbgLv(1) << "CGui: call enableScanControls";
   enableScanControls();

   // The centerpiece combo box
   cb_centerpiece->setLogicalIndex( all_tripinfo[ 0 ].centerpiece );

   // Redo plot
DbgLv(1) << "CGui: call plot_current";
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   plot_current();
   QApplication::restoreOverrideCursor();

   connect( ct_from, SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_from   ( double ) ) );

   connect( ct_to  , SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_to     ( double ) ) );

   // Ok to enable some buttons now
   enableControls();
   if ( ! disk_controls->db() )
      pb_saveUS3  ->setEnabled( true );

   pb_details     ->setEnabled( true );
   pb_solution    ->setEnabled( true );
   cb_centerpiece ->setEnabled( true );
   pb_editRuninfo ->setEnabled( true );

   // some things one can't do from here
   ct_tolerance   ->setEnabled( false );

   enableRunIDControl( false );

   if ( runType == "RA" && subsets.size() < 1 )
   {
      // Allow user to define subsets, if he hasn't already
      pb_define   ->setEnabled( true );
   }

   saveStatus = ( ExpData.expID == 0 ) ? HD_ONLY : BOTH;
   //pb_editRuninfo  ->setEnabled ( saveStatus == HD_ONLY );

   // Point to any existing time state file
   QDir ddir( currentDir );
   QStringList tmsfs = ddir.entryList( QStringList( "*.time_state.*" ),
         QDir::Files, QDir::Name );
   QString defs_fnamei;
   QString tmst_dname  = currentDir;
   if ( tmst_dname.right( 1 ) != "/" )
      tmst_dname   += "/";
DbgLv(1) << "lTS: tmst_dname" << tmst_dname << "tmsfs count" << tmsfs.count();

   if ( tmsfs.count() == 2 )
   {  // Looks like we have a TMST and corresponding Defs XML
      tmst_fnamei   = tmsfs[ 0 ];
      defs_fnamei   = tmsfs[ 1 ];
DbgLv(1) << "lTS: tmsfs" << tmst_fnamei << defs_fnamei;

      if ( tmst_fnamei.contains( ".tmst" )  &&
           defs_fnamei.contains( ".xml" ) )
      {  // Have both files, so save full path to TMST
         tmst_fnamei   = tmst_dname + tmst_fnamei;
      }

      else if ( tmsfs[ 0 ].contains( ".xml" )  &&
                tmsfs[ 1 ].contains( ".tmst" ) )
      {  // Have both files (in opposite order), so save full path to TMST
         tmst_fnamei   = tmst_dname + tmsfs[ 1 ];
      }

      else
      {  // Do not have both files, so clear TMST file path
         tmst_fnamei.clear();
      }
DbgLv(1) << "lTS: tmst,defs fnamei" << tmst_fnamei << defs_fnamei;
   }

   else  // If file does not exist, clear name
   {
DbgLv(1) << "lTS: NON_EXIST:" << tmst_fnamei;
      tmst_fnamei.clear();
   }

   // Read it in ok, so ok to sync with DB
   ExpData.syncOK = true;

   enableSaveBtn();
   le_status->setText( tr( "Local disk data load IS COMPLETE." ) );
   qApp->processEvents();
DbgLv(1) << "CGui: ldUS3Dk: RTN";
}

// Function to load an experiment from the DB
void US_ConvertGui:: loadUS3DB( void )
{
DbgLv(1) << "CGui: ldUS3DB: IN";
   le_status->setText( tr( "Loading data from DB ..." ) );
   qApp->processEvents();

   // Verify connectivity
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() != US_DB2::OK )
   {
      QMessageBox::information( this,
          tr( "Error" ),
          tr( "Error making the DB connection.\n" ) );
      return;
   }

   // We have runID from a call to a load dialog; let's copy the DB info to HD
   QDir        readDir( US_Settings::resultDir() );
   QString     dirname = readDir.absolutePath() + "/" + runID + "/";

DbgLv(1) << "CGui: ldUS3DB: call rdDBExp";
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   le_status->setText( tr( "Loading data from DB (Experiment) ..." ) );
   qApp->processEvents();
   QString status = US_ConvertIO::readDBExperiment( runID, dirname, &db,
                                                    speedsteps );
   QApplication::restoreOverrideCursor();

   if ( status  != QString( "" ) )
   {
      QMessageBox::information( this, tr( "Error" ), status + "\n" );
      return;
   }

   // And load it
DbgLv(1) << "CGui: ldUS3DB: call ldUS3Dk";
   le_status->setText( tr( "Loading data from DB (Disk data) ..." ) );
   qApp->processEvents();
   loadUS3Disk( dirname );

   saveStatus = BOTH;         // override from loadUS3Disk()
   ExpData.syncOK = true;     // since we just read it from there

   enableControls();

   le_status->setText( tr( "%1 data triples are NOW LOADED from DB." )
                       .arg( allData.size() ) );
   qApp->processEvents();

DbgLv(1) << "CGui: ldUS3DB: RTN";
}

void US_ConvertGui::getExpInfo( void )
{
DbgLv(1) << "CGui: gExpInf: IN";
   ExpData.runID = le_runID -> text();

   if ( disk_controls->db() )
   {
      // Then we're working in DB, so verify connectivity
      US_Passwd pw;
      QString masterPW = pw.getPasswd();
      US_DB2 db( masterPW );

      if ( db.lastErrno() != US_DB2::OK )
      {
         QMessageBox::information( this,
                tr( "Error" ),
                tr( "Error making the DB connection.\n" ) );
         return;
      }

      // Check if the run ID already exists in the DB
      int recStatus = ExpData.checkRunID( &db );

      // if saveStatus == BOTH, then we are editing the record from the database
      if ( ( recStatus == US_DB2::OK ) && ( saveStatus != BOTH ) )
      {
         QMessageBox::information( this,
                tr( "Error" ),
                tr( "The current runID already exists in the database.\n"
                    "To edit that information, load it from the database\n"
                    "to start with." ) );
         return;
      }
   }

   // OK, proceed

   // Calculate average temperature
   double sum   = 0.0;
   double count = 0.0;
   for ( int ii = 0; ii < allData.size(); ii++ )
   {
      US_DataIO::RawData* raw = &allData[ ii ];
      int kscan    = raw->scanData.size();
      for ( int jj = 0; jj < kscan; jj++ )
      {
         sum         += raw->scanData[ jj ].temperature;
      }

      count       += kscan;
   }

   ExpData.runTemp       = QString::number( sum / count );

   // Load information we're passing
   char* optSysPtr       = ExpData.opticalSystem.data();
   strncpy( optSysPtr, allData[ 0 ].type, 2 );
   optSysPtr[ 2 ] = '\0';

   // A list of unique rpms
   ExpData.rpms.clear();
   for ( int ii = 0; ii < allData.size(); ii++ )
   {
      US_DataIO::RawData* raw = &allData[ ii ];
      for ( int jj = 0; jj < raw->scanData.size(); jj++ )
      {
         if ( ! ExpData.rpms.contains( raw->scanData[ jj ].rpm ) )
            ExpData.rpms << raw->scanData[ jj ].rpm;
      }
   }

   // Now sort the rpm list.  Use a modified bubble sort;
   // the list might already be almost ordered
   bool done = false;
   while ( !done )
   {
      done = true;
      for ( int i = 0; i < ExpData.rpms.size() - 1; i++ )
      {
         if ( ExpData.rpms[ i ] > ExpData.rpms[ i + 1 ] )
         {
            ExpData.rpms.swap( i, i + 1 );
            done = false;
         }
      }
   }

   int dbdisk = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                        : US_Disk_DB_Controls::Disk;

   US_ExperimentGui* expInfo = new US_ExperimentGui( true,    // signal_wanted
                                                     ExpData,
                                                     dbdisk );

   connect( expInfo, SIGNAL( updateExpInfoSelection( US_Experiment& ) ),
            this   , SLOT  ( updateExpInfo         ( US_Experiment& ) ) );

   connect( expInfo, SIGNAL( cancelExpInfoSelection() ),
            this   , SLOT  ( cancelExpInfo         () ) );

   connect( expInfo, SIGNAL( use_db        ( bool ) ),
                     SLOT  ( update_disk_db( bool ) ) );

   expInfo->exec();
DbgLv(1) << "CGui: gExpInf: RTN";
}

// Updating after user has selected info from experiment dialog
void US_ConvertGui::updateExpInfo( US_Experiment& d )
{
   // Update local copy
   ExpData = d;

   if ( this->saveStatus == NOT_SAVED )
      this->saveStatus = EDITING;        // don't delete the data!

   enableControls();
}

void US_ConvertGui::cancelExpInfo( void )
{
   // Don't clear out the data, just don't save anything new

   enableControls();
}

void US_ConvertGui::getSolutionInfo( void )
{
   const int chanID = 1;

   triple_index();
   ExpData.runID = le_runID -> text();

   int dbdisk = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                        : US_Disk_DB_Controls::Disk;

   US_Solution solution = out_chaninfo[ tripListx ].solution;

   US_SolutionGui* solutionInfo = new US_SolutionGui(
                                     ExpData.expID,
                                     chanID, // channelID
                                     true,   // signal wanted
                                     dbdisk, // data source
                                     solution );

   connect( solutionInfo, SIGNAL( updateSolutionGuiSelection( US_Solution ) ),
            this,         SLOT  ( updateSolutionInfo        ( US_Solution ) ) );

   connect( solutionInfo, SIGNAL( cancelSolutionGuiSelection() ),
            this,         SLOT  ( cancelSolutionInfo        () ) );

   connect( solutionInfo, SIGNAL( use_db        ( bool ) ),
                          SLOT  ( update_disk_db( bool ) ) );

   solutionInfo->exec();
}

// Updating after user has selected info from experiment dialog
void US_ConvertGui::updateSolutionInfo( US_Solution s )
{
   triple_index();

   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   // Update local copy
   out_chaninfo[ tripListx ].solution = s;
   out_tripinfo[ tripDatax ].solution = s;

   le_solutionDesc->setText( out_chaninfo[ tripListx ].solution.solutionDesc );

   // For MWL, duplicate solution to all triples of the channel
   if ( isMwl )
   {
      int idax   = out_chandatx[ tripListx ];
      int ldax   = tripListx + 1;
      ldax       = ldax < out_chandatx.size()
                 ? out_chandatx[ ldax ]
                 : out_tripinfo.size();

      while ( idax < ldax )
      {
         out_tripinfo[ idax++ ].solution = s;
DbgLv(0) << "CGui: updSol: dax" << idax
 << "s.desc s.id" << s.solutionDesc << s.solutionID << s.solutionGUID;
      }
   }

   // Re-plot
   plot_current();

   QApplication::restoreOverrideCursor();
   QApplication::restoreOverrideCursor();

   enableControls();
}

void US_ConvertGui::cancelSolutionInfo( void )
{
   enableControls();
}

// Function to copy the current triple's data to all triples
void US_ConvertGui::tripleApplyAll( void )
{
   US_Convert::TripleInfo tripinfo = out_chaninfo[ tripListx ];

   // Copy selected fields only
   for ( int ii = 0; ii < out_tripinfo.size(); ii++ )
   {
      out_tripinfo[ ii ].centerpiece  = tripinfo.centerpiece;
      out_tripinfo[ ii ].solution     = tripinfo.solution;
   }

   for ( int ii = 0; ii < out_chaninfo.size(); ii++ )
   {
      out_chaninfo[ ii ].centerpiece  = tripinfo.centerpiece;
      out_chaninfo[ ii ].solution     = tripinfo.solution;
   }

   for ( int ii = 0; ii < all_tripinfo.size(); ii++ )
   {
      all_tripinfo[ ii ].centerpiece  = tripinfo.centerpiece;
      all_tripinfo[ ii ].solution     = tripinfo.solution;
   }

   for ( int ii = 0; ii < all_chaninfo.size(); ii++ )
   {
      all_chaninfo[ ii ].centerpiece  = tripinfo.centerpiece;
      all_chaninfo[ ii ].solution     = tripinfo.solution;
   }

   le_status->setText( tr( "The current c/c/w information has been"
                           " copied to all." ) );
   qApp->processEvents();

   plot_current();

   enableControls();
}

// Slot to handle click of Show TimeState
void US_ConvertGui::showTimeState( void )
{
DbgLv(1) << "showTimeState: Time State file path" << tmst_fnamei;
   US_TmstPlot* tsdiag = new US_TmstPlot( this, tmst_fnamei );

   tsdiag->exec();
}

void US_ConvertGui::runDetails( void )
{
   // Create data structures for US_RunDetails2
   QStringList tripleDescriptions;
   QVector< US_DataIO::RawData >  currentData;

   if ( isMwl )
   {  // For MWL, only pass the 1st data set of each cell/channel
      for ( int ii = 0; ii < out_chaninfo.size(); ii++ )
      {
         currentData        << *outData[ out_chandatx[ ii ] ];
         QString celchn      = out_chaninfo[ ii ].tripleDesc;
         celchn              = celchn.section( " / ", 0, 1 );
         tripleDescriptions << celchn;
      }
   }

   else
   {  // For most data, pass all (non-excluded) triples
      for ( int ii = 0; ii < out_tripinfo.size(); ii++ )
      {
         currentData        << *outData[ ii ];
         tripleDescriptions << out_tripinfo[ ii ].tripleDesc;
      }
   }

   US_RunDetails2* dialog = new US_RunDetails2( currentData, runID, currentDir,
                                                tripleDescriptions );
   dialog->exec();
   qApp->processEvents();
//   delete dialog;
}

void US_ConvertGui::changeDescription( void )
{
   if ( le_description->text().size() < 1 )
      le_description->setText( outData[ tripDatax ]->description );

   else
   {
      QString chdesc  = le_description->text().trimmed();
      outData[ tripDatax ]->description = chdesc;

      if ( isMwl )
      {  // Propagate description to all triples of channel
         out_chaninfo[ tripListx ].description   = chdesc;
         int trxs        = out_chandatx[ tripListx ];
         int trxe        = trxs + nlambda;
         for ( int trx = trxs; trx < trxe; trx++ )
            outData[ trx ]->description = chdesc;
      }
   }
}

void US_ConvertGui::changeTriple()
{
   QString chann  = lw_triple->currentItem()->text()
                    .section( "/", 1, 1 ).simplified();
   pb_dropChan    ->setText( tr( "Drop All Channel '%1's" ).arg( chann ) );

   triple_index( );
DbgLv(1) << "chgTrp: trDx trLx" << tripDatax << tripListx
 << "outDsz chinfsz" << outData.size() << out_chaninfo.size();

   le_dir         ->setText( currentDir );
   le_description ->setText( outData[ tripDatax ]->description );
   le_solutionDesc->setText( out_chaninfo[ tripListx ].solution.solutionDesc );

   // If MWL, set the cell/channel index and get the lambda range
   if ( isMwl )
   {
      QVector< int > wvs;
      nlambda     = mwl_data.lambdas( wvs, tripListx );
      if ( slambda != wvs[ 0 ]  ||  elambda != wvs[ nlambda - 1 ] )
      {  // A change in lambda range requires a general reset
         setTripleInfo();
      }
   }

   // Reset maximum scan control values
   enableScanControls();

   // The centerpiece combo box
   cb_centerpiece->setLogicalIndex( out_chaninfo[ tripListx ].centerpiece );

   // Redo plot
   plot_current();
}

// Reset triple controls after a change
void US_ConvertGui::setTripleInfo( void )
{
   // Load them into the list box
   int trListSave = lw_triple->currentRow();
   int nchans     = out_channels.count();
   int ntrips     = out_triples .count();
   lw_triple->disconnect();
   lw_triple->clear();
DbgLv(1) << " sTI: nch ntr" << nchans << ntrips << "trLSv" << trListSave;

   if ( isMwl )
   {  // For MWL, wavelength lists need rebuilding
      if ( mwl_data.countOf( "lambda" ) < 1 )
      {  // If no lambdas yet, we need to build from scratch
         // Load MWL data from AUCs
         mwl_data.load_mwl( allData );

         // Build lambda lists and controls
         mwl_connect( false );
         nlamb_i         = mwl_data.lambdas_raw( all_lambdas );
         cb_lambstrt->clear();
         cb_lambstop->clear();
         for ( int ii = 0; ii < nlamb_i; ii++ )
         {
            QString clamb = QString::number( all_lambdas[ ii ] );
            cb_lambstrt->addItem( clamb );
            cb_lambstop->addItem( clamb );
         }

         cb_lambstrt->setCurrentIndex( 0 );
         cb_lambstop->setCurrentIndex( nlamb_i - 1 );
         nlambda         = mwl_data.lambdas( exp_lambdas );
         cb_lambplot->clear();

         for ( int ii = 0; ii < nlambda; ii++ )
         {
            QString clamb = QString::number( exp_lambdas[ ii ] );
            cb_lambplot->addItem( clamb );
         }

         cb_lambplot->setCurrentIndex( nlambda / 2 );

         show_mwl_control( true );
         mwl_connect( true );
      }

      for ( int ccx = 0; ccx < nchans; ccx++ )
      {  // Reformat the triples entries
         nlambda        = mwl_data.lambdas( exp_lambdas, ccx );
         if ( nlambda < 1 )
         {
            break;
         }
         slambda        = exp_lambdas[ 0 ];
         elambda        = exp_lambdas[ nlambda - 1 ];
         lw_triple->addItem( out_channels[ ccx ]
                           + QString( " / %1-%2 (%3)" )
                           .arg( slambda ).arg( elambda ).arg( nlambda ) );
      }

      // Get wavelengths for the currently selected cell/channel
      tripListx      = qMax( 0, qMin( trListSave, ( nchans - 1 ) ) );
      nlambda        = mwl_data.lambdas( exp_lambdas, tripListx );
      if ( nlambda < 1 )
      {
         return;
      }
      slambda        = exp_lambdas[ 0 ];
      elambda        = exp_lambdas[ nlambda - 1 ];
      int plambda    = cb_lambplot->currentText().toInt();
      int pltx       = nlambda / 2;
      tripDatax      = out_chandatx[ tripListx ] + pltx;

      mwl_connect( false );
      cb_lambplot->clear();

      for ( int wvx = 0; wvx < nlambda; wvx++ )
      {  // Rebuild the plot lambda list
         int ilamb      = exp_lambdas[ wvx ];
         cb_lambplot->addItem( QString::number( ilamb ) );

         if ( ilamb == plambda )  pltx = wvx;
      }

      // Re-do selections for lambda start,stop,plot
      cb_lambstrt->setCurrentIndex( all_lambdas.indexOf( slambda ) );
      cb_lambstop->setCurrentIndex( all_lambdas.indexOf( elambda ) );
      cb_lambplot->setCurrentIndex( pltx );
      mwl_connect( true );
   }

   else
   {  // For non-MWL, just re-do triples
      for ( int trx = 0; trx < ntrips; trx++ )
      {  // Rebuild the triples list
         lw_triple->addItem( out_triples[ trx ] );
      }

      tripListx      = qMax( 0, qMin( trListSave, ( ntrips - 1 ) ) );
      tripDatax      = tripListx;
   }

   lw_triple->setCurrentRow( tripListx );
   connect( lw_triple, SIGNAL( itemSelectionChanged() ),
                       SLOT  ( changeTriple        () ) );

   if ( ntrips > 0 )
   {
      QString chann  = lw_triple->currentItem()->text()
                       .section( "/", 1, 1 ).simplified();
      pb_dropChan    ->setText( tr( "Drop All Channel '%1's" ).arg( chann ) );
   }
}

void US_ConvertGui::checkTemperature( void )
{
   // Temperature check
   double dt = 0.0;

   foreach( US_DataIO::RawData triple, allData )
   {
       double temp_spread = triple.temperature_spread();
       dt = ( temp_spread > dt ) ? temp_spread : dt;
   }

   if ( dt > US_Settings::tempTolerance() )
   {
      QMessageBox::warning( this,
            tr( "Temperature Problem" ),
            tr( "The temperature in this run varied over the course\n"
                "of the run to a larger extent than allowed by the\n"
                "current threshold (" )
                + QString::number( US_Settings::tempTolerance(), 'f', 1 )
                + " " + DEGC + tr( "). The accuracy of experimental\n"
                "results may be affected significantly." ) );
   }
}

void US_ConvertGui::getCenterpieceIndex( int )
{
   int cpID      = cb_centerpiece->getLogicalID();
   out_chaninfo[ tripListx ].centerpiece = cpID;
DbgLv(1) << "getCenterpieceIndex " << out_chaninfo[tripListx].centerpiece;

   // For MWL, duplicate centerpiece to all triples of the channel
   if ( isMwl )
   {
      int idax   = out_chandatx[ tripListx ];
      int ldax   = tripListx + 1;
      ldax       = ldax < out_chandatx.size()
                 ? out_chandatx[ ldax ]
                 : out_tripinfo.size();

      while ( idax < ldax )
      {
         out_tripinfo[ idax++ ].centerpiece = cpID;
      }
   }
   else
      out_tripinfo[ tripListx ].centerpiece = cpID;

   enableSaveBtn();
}

void US_ConvertGui::focus_from( double scan )
{
   int from = (int)scan;
   int to   = (int)ct_to->value();

   if ( from > to )
   {
      ct_to->disconnect();
      ct_to->setValue( scan );
      to = from;

      connect( ct_to, SIGNAL( valueChanged ( double ) ),
                      SLOT  ( focus_to     ( double ) ) );
   }

   focus( from, to );
}

void US_ConvertGui::focus_to( double scan )
{
   int to   = (int)scan;
   int from = (int)ct_from->value();

   if ( from > to )
   {
      ct_from->disconnect();
      ct_from->setValue( scan );
      from = to;

      connect( ct_from, SIGNAL( valueChanged ( double ) ),
                        SLOT  ( focus_from   ( double ) ) );
   }

   focus( from, to );
}

void US_ConvertGui::focus( int from, int to )
{
   if ( ( from == 0 && to == 0 )  ||  isMwl )
   {
      pb_exclude->setEnabled( false );
      pb_include->setEnabled( false );
   }
   else
   {
      pb_exclude->setEnabled( true );
      pb_include->setEnabled( true );
   }

   QList< int > focus;  // We don't care if -1 is in the list
   for ( int i = from - 1; i <= to - 1; i++ ) focus << i;

   set_colors( focus );

}

// Function to initialize the excluded scan count
void US_ConvertGui::init_excludes( void )
{
   allExcludes.clear();
   US_Convert::Excludes x;
   for ( int i = 0; i < allData.size(); i++ )
      allExcludes << x;
}

// Function to exclude user-selected scans
void US_ConvertGui::exclude_scans( void )
{
   US_Convert::Excludes excludes = allExcludes[ tripDatax ];

   // Scans actually start at index 0
   int scanStart = (int)ct_from->value() - 1;
   int scanEnd   = (int)ct_to  ->value() - 1;

   // Sometimes the user leaves this at 0
   scanStart = ( scanStart < 0 ) ? 0 : scanStart;
   int scanCount = scanEnd - scanStart + 1;

   // Find the first scan to exclude, but account for the already excluded
   int scanNdx = 0;
   while ( scanNdx < scanStart )
   {
      if ( excludes.contains( scanNdx ) )
         scanStart++;

      scanNdx++;
   }

   // Now we have the starting point, so exclude some scans
   int excluded = 0;
   while ( excluded < scanCount )
   {
      if ( ! excludes.contains( scanNdx ) )
      {
         excluded++;
         allExcludes[ tripDatax ] << scanNdx;
      }

      scanNdx++;
   }

   enableScanControls();

   replot();
}

void US_ConvertGui::include( void )
{
   allExcludes[ tripDatax ].excludes.clear();
   enableScanControls();

   pb_include->setEnabled( false );
   pb_exclude->setEnabled( false );

   replot();
}

// User pressed the define subsets button while processing equil-abs data
void US_ConvertGui::define_subsets( void )
{
   subsets.clear();
   countSubsets = 1;      // We start with one big subset

   pb_define  ->setEnabled( false );
   pb_process ->setEnabled( true );

   connect( picker, SIGNAL( cMouseUp( const QwtDoublePoint& ) ),
                    SLOT  ( cClick  ( const QwtDoublePoint& ) ) );

   step = SPLIT;
}

void US_ConvertGui::process_subsets( void )
{
   if ( subsets.size() < 1 )
   {
      // Not enough clicks to work with
      subsets.clear();
      pb_process ->setEnabled( true );
      return;
   }

   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   // Add the top and bottom boundaries
   subsets << 5.7
           << 7.3;

   // Let's make sure the points are in sorted order
   for ( int i = 0; i < subsets.size() - 1; i++ )
      for ( int j = i + 1; j < subsets.size(); j++ )
         if ( subsets[ i ] > subsets[ j ] )
         {
            double temp  = subsets[ i ];
            subsets[ i ] = subsets[ j ];
            subsets[ j ] = temp;
         }

   pb_process ->setEnabled( false );
   picker   ->disconnect();

DbgLv(1) << "CGui:pSS: split CALL";
   // Now let's split the triple
   US_Convert::splitRAData( allData, all_tripinfo, tripDatax, subsets );

   // We don't need this any more
   subsets.clear();

   // Reinitialize some things
   if ( ! init_output_data() )
      return;
   setTripleInfo();
   init_excludes();
   enableControls();

   plot_current();
   QApplication::restoreOverrideCursor();
}

// User pressed the Define reference button while analyzing intensity data
void US_ConvertGui::define_reference( void )
{
   if ( isMwl )
   {  // First insure that the full range of wavelengths are available
      //  for defining reference scans
      int kdiff       = 0;
DbgLv(1) << "CGui: (1)dRef: isMwl=true";

      for ( int ccx = 0; ccx < out_channels.size(); ccx++ )
      {  // Review the wavelength ranges for each channel
         QVector< int > wvs;
         int klamb       = mwl_data.lambdas( wvs, ccx );
DbgLv(1) << "CGui: (2)dRef:  kl nl kwvs" << klamb << nlamb_i << wvs.count();

         if ( klamb != nlamb_i )
         {  // Count of wavelengths differs from the full input range
            kdiff++;
         }

         else
         {  // Test each wavelength in this channel's range
            for ( int jj = 0; jj < klamb; jj++ )
            {  // Record any differences
               if ( wvs[ jj ] != all_lambdas[ jj ] )
                  kdiff++;
DbgLv(1) << "CGui: (2)dRef:   jj wj aj kd" << jj << wvs[jj] << all_lambdas[jj]
 << kdiff;
            }
         }
      }

      if ( kdiff != 0 )
      {  // Report dangerous differences and allow a reference define cancel
         int status = QMessageBox::information( this,
            tr( "Inconsistent Reference Wavelengths" ),
            tr( "Differences in wavelength ranges exist for the channels.\n"
                "The safest course is to always open the full range of\n"
                "input wavelengths for the Reference channel.\n\n"
                "If you are sure that Reference wavelengths will cover\n"
                "all other channels, you may \"Proceed\".\n\n"
                "Otherwise, you should \"Cancel\".\n" ),
            tr( "&Cancel" ), tr( "&Proceed" ) );

         if ( status == 0 ) return;
      }
   }

   connect( picker, SIGNAL( cMouseUp( const QwtDoublePoint& ) ),
                    SLOT  ( cClick  ( const QwtDoublePoint& ) ) );

   reference_start = 0.0;
   reference_end   = 0.0;
   pb_reference->setEnabled( false );
   pb_cancelref->setEnabled( true );

   step = REFERENCE;
}

// Select starting point of reference scan in intensity data
void US_ConvertGui::start_reference( const QwtDoublePoint& p )
{
   reference_start   = p.x();

   draw_vline( reference_start );
   data_plot->replot();
}

// Select end point of reference scan in intensity data
void US_ConvertGui::process_reference( const QwtDoublePoint& p )
{
   // Just in case we get a second click message right away
   if ( fabs( p.x() - reference_start ) < 0.005 ) return;

   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   le_status->setText( tr( "The reference scans are being defined..." ) );
   qApp->processEvents();
   reference_end = p.x();
   draw_vline( reference_end );
   data_plot->replot();
   picker        ->disconnect();

   // Double check if min < max
   if ( reference_start > reference_end )
   {
      double temp     = reference_start;
      reference_start = reference_end;
      reference_end   = temp;
   }

   // Calculate the averages for all triples
   PseudoCalcAvg();

   // Now that we have the averages, let's replot
   Pseudo_reference_triple = tripListx;

   // Default to displaying the first non-reference triple
   for ( int trx = 0; trx < outData.size(); trx++ )
   {
      if ( trx != Pseudo_reference_triple )
      {
         tripListx = trx;
         break;
      }
   }

   lw_triple->setCurrentRow( tripListx );
   plot_current();
   QApplication::restoreOverrideCursor();

   pb_reference  ->setEnabled( false );
   referenceDefined = true;
DbgLv(1) << "CGui: (6)referDef=" << referenceDefined;
   enableSaveBtn();
   enableRunIDControl( false );
   le_status->setText( tr( "The reference scans have been defined." ) );
   qApp->processEvents();
}

// Process a control-click on the plot window
void US_ConvertGui::cClick( const QwtDoublePoint& p )
{
   switch ( step )
   {
      case SPLIT :
         if ( countSubsets < 4 )
         {
             // process equil-abs data
             draw_vline( p.x() );
             subsets << p.x();
             countSubsets++;
         }

         break;

      case REFERENCE :
         // process reference scan
         if ( reference_start == 0.0 )
            start_reference( p );

         else
            process_reference( p );

      default :
         break;

   }
}

// Reference calculation for pseudo-absorbance
void US_ConvertGui::PseudoCalcAvg( void )
{
   if ( referenceDefined ) return;  // Average calculation has already been done

   if ( isMwl )
   {  // Do calculations for each wavelength, if MWL
      PseudoCalcAvgMWL();
      return;
   }

   US_DataIO::RawData* referenceData = outData[ tripDatax ];
   int ref_size = referenceData->pointCount();

   for ( int ss = 0; ss < referenceData->scanCount(); ss++ )
   {
      US_DataIO::Scan* scan = &referenceData->scanData[ ss ];

      int    rr     = 0;
      int    count  = 0;
      double sum    = 0.0;

      while ( referenceData->radius( rr ) < reference_start  &&
              rr < ref_size )
         rr++;

      while ( referenceData->radius( rr ) < reference_end  &&
              rr < ref_size )
      {
         sum += scan->rvalues[ rr ];
         count++;
         rr++;
      }

      if ( count > 0 )
         ExpData.RIProfile << sum / count;

      else
         ExpData.RIProfile << 1.0;    // See the log10 function, later

   }

   // Now average around excluded values
   int lastGood  = 0;
   int countBad  = 0;
   for ( int ss = 0; ss < ExpData.RIProfile.size(); ss++ )
   {
      // In case there are adjacent excluded scans...
      if ( allExcludes[ tripDatax ].contains( ss ) )
         countBad++;

      // Calculate average of before and after for intervening values
      else if ( countBad > 0 )
      {
         double newAvg = ( ExpData.RIProfile[ lastGood ]
                         + ExpData.RIProfile[ ss ] ) / 2.0;

         for ( int rr = lastGood + 1; rr < ss; rr++ )
            ExpData.RIProfile[ rr ] = newAvg;

         countBad = 0;
      }

      // Normal situation -- value is not excluded
      else
         lastGood = ss;

   }

   // Now calculate the pseudo-absorbance
   int ndxmax    = ExpData.RIProfile.size() - 1;

   for ( int trx = 0; trx < outData.size(); trx++ )
   {
      US_DataIO::RawData* currentData = outData[ trx ];
      int kcpoint   = currentData->pointCount();

      for ( int ss = 0; ss < currentData->scanCount(); ss++ )
      {
         US_DataIO::Scan* scan = &currentData->scanData[ ss ];

         for ( int rr = 0; rr < kcpoint; rr++ )
         {
            double rvalue = scan->rvalues[ rr ];

            // Protect against possible inf's and nan's, if a reading
            // evaluates to 0 or wherever log function is undefined or -inf
            if ( rvalue < 1.0 ) rvalue = 1.0;

            // Check for boundary condition
            int ndx        = qMin( ss, ndxmax );
            double prval   = ExpData.RIProfile[ ndx ];
            scan->rvalues[ rr ] = ( prval != 0.0 ) ?
                                  log10( prval / rvalue ) : 0.0;
         }
      }

      // Let's mark pseudo-absorbance as different from RI data,
      //  since it needs some different processing in some places
      isPseudo = true;
   }

   // Enable intensity plot
   referenceDefined = true;
DbgLv(1) << "CGui:PCA: (7)referDef=" << referenceDefined;
   pb_intensity->setEnabled( true );
   pb_reference->setEnabled( false );
   pb_cancelref->setEnabled( true );
}

// Bring up a graph window showing the intensity profile
void US_ConvertGui::show_intensity( void )
{
   QString triple = out_triples[ 0 ];
   QVector< double > scan_nbrs;

   if ( isMwl )
   {  // For MWL, set special triple string and build scan numbers
      int riscans    = ExpData.RI_nscans;
      int riwvlns    = ExpData.RI_nwvlns;
      triple      = out_channels[ 0 ] + " / "
         + QString::number( ExpData.RIwvlns[ 0 ] ) + "-"
         + QString::number( ExpData.RIwvlns[ riwvlns - 1 ] );

      // Scan numbers are composite wavelength.scan
      double scndiv  = 0.1;
      double scnmax  = (double)riscans;
      double scnfra  = scnmax * scndiv;
      while ( scnfra > 0.999 )
      {
         scndiv        *= 0.1;
         scnfra         = scnmax * scndiv;
      }
DbgLv(1) << "CGui: show_intensity  scndiv scnfra" << scndiv << scnfra;

      for ( int ii = 0; ii < riwvlns; ii++ )
      {
         double wvbase  = (double)ExpData.RIwvlns[ ii ];

         for ( int jj = 0; jj < riscans; jj++ )
         {
            double scnnbr  = wvbase + scndiv * (double)( jj + 1 );
            scan_nbrs << scnnbr;
         }
      }
   }

   else
   {  // For non-MWL, set scan numbers vector
      for ( int ii = 0; ii < ExpData.RIProfile.size(); ii++ )
         scan_nbrs << (double)( ii + 1 );
   }
DbgLv(1) << "CGui: show_intensity  scn1 scnn" << scan_nbrs[0]
 << scan_nbrs[ scan_nbrs.size() - 1 ] << "isMwl" << isMwl;

   US_Intensity* dialog
      = new US_Intensity( runID, triple,
                        ( const QVector< double > ) ExpData.RIProfile,
                        ( const QVector< double > ) scan_nbrs );
   dialog->exec();
   qApp->processEvents();
}

// Un-do reference scans apply
void US_ConvertGui::cancel_reference( void )
{
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   le_status->setText( tr( "The reference scans are being canceled..." ) );
   qApp->processEvents();
   int wvoff    = 0;
   int rscans   = ExpData.RI_nscans;


   // Do the inverse operation and retrieve raw intensity data
   for ( int ii = 0; ii < outData.size(); ii++ )
   {
      US_DataIO::RawData* currentData = outData[ ii ];

      if ( isMwl )
      {  // For MWL, profile is offset by wavelength
         int iwavl    = out_triples[ ii ].section( " / ", 2, 2 ).toInt();
         wvoff        = ExpData.RIwvlns.indexOf( iwavl );

         if ( wvoff < 0 )
         {
DbgLv(1) << "Triple " << out_triples[ ii ]
 << "has NO CORRESPONDING RI PROFILE POINT!!!";
            wvoff        = 0;
            QMessageBox::information( this,
              tr( "Error" ),
              tr( "Triple %1 has NO CORRESPONDING RI PROFILE POINT!!!" )
              .arg( out_triples[ ii ] ) );
            int kwavl    = 99999;

            for ( int jj = 0; jj < ExpData.RI_nwvlns; jj++ )
            {  // Find index of nearest wavelength
               int jwavl    = qAbs( ExpData.RIwvlns[ jj ] - iwavl );

               if ( jwavl < kwavl )
               {
                  kwavl        = jwavl;
                  wvoff        = jj;
               }
            }
         }
      }

      wvoff       *= rscans;
      int kpoint   = currentData->pointCount();
      int kscan    = currentData->scanCount();


      if ( ExpData.RIProfile.size() )  //ALEXEY
	{
	  for ( int jj = 0; jj < kscan; jj++ )
	    {
	      US_DataIO::Scan* scan  = &currentData->scanData[ jj ];
	      qDebug() << "jj: " << jj << ", Size of ExpData.RIProfile: " << ExpData.RIProfile.size() << ", wvoff: " << wvoff << ", jj+wvoff: " << jj+wvoff ;
	      
	      double           rppro = ExpData.RIProfile[ jj + wvoff ];
	      
	      for ( int kk = 0; kk < kpoint; kk++ )
		{
		  double rvalue = scan->rvalues[ kk ];
		  
		  scan->rvalues[ kk ] = rppro / pow( 10, rvalue );
		}
	    }
	}

      qDebug() << "Test 2 After cycle";
   }
   qDebug() << "Test 3";

   referenceDefined = false;
   isPseudo         = false;
   ExpData.RIProfile.clear();
   reference_start  = 0.0;
   reference_end    = 0.0;
DbgLv(1) << "CGui: (8)referDef=" << referenceDefined;

   for ( int ii = 0; ii < all_tripinfo.size(); ii++ )
      all_tripinfo[ ii ].excluded = false;

   setTripleInfo();

   pb_reference  ->setEnabled( true );
   pb_cancelref  ->setEnabled( false );
   pb_intensity  ->setEnabled( false );
   tripListx = 0;
   lw_triple->setCurrentRow( tripListx );

   plot_current();

   enableSaveBtn();

   le_status->setText( tr( "The reference scans have been canceled." ) );
   QApplication::restoreOverrideCursor();
   qApp->processEvents();
}

// Drop selected triples
void US_ConvertGui::drop_reference( void )
{
   QStringList selected;
   QStringList celchns;

   // Prepare the list of currently selected triples
   selected = out_triples;

   // Pop up and execute a dialog to select triples to delete
   US_SelectTriples* seldiag = new US_SelectTriples( selected );

   if ( seldiag->exec() != QDialog::Accepted )
   {  // If Accept was not clicked, make sure selection list is empty
      selected.clear();
   }

   int selsiz      = selected.size();
DbgLv(1) << "DelTrip: selected size" << selsiz;

   // Modify triples or cell/channels and wavelengths for specified excludes
   if ( selsiz > 0 )
   {
      for ( int ss = 0; ss < selsiz; ss++ )
      {  // Mark selected triples as excluded
         int trx    = all_triples.indexOf( selected[ ss ] );

         if ( trx >= 0 )
         {
            all_tripinfo[ trx ].excluded = true;
DbgLv(1) << "DelTrip:  EXCLUDED ss trx sel" << ss << trx << selected[ss];
         }
      }

      // Rebuild the output data controls
DbgLv(1) << "DelTrip: bldout call";
      build_output_data();
DbgLv(1) << "DelTrip: bldout  RTN";

      if ( isMwl )
      {  // For MWL, rebuild wavelength controls
         build_lambda_ctrl();
      }

      setTripleInfo();         // Review new triple information
   }

   plot_titles();              // Output a plot of the current data
   plot_all();
}

// Drop the triples for the selected channel
void US_ConvertGui::drop_channel()
{
   QString chann  = lw_triple->currentItem()->text()
                    .section( "/", 1, 1 ).simplified();
   int status     = QMessageBox::information( this,
      tr( "Drop Triples with Selected Channel" ),
      tr( "You have selected a list item that implies you wish to"
          " drop triples that have channel '%1'\n\n"
          "If that is what you intend, click \"Proceed\".\n\n"
          "Otherwise, you should \"Cancel\".\n" ).arg( chann ),
      tr( "&Proceed" ), tr( "&Cancel" ) );

   if ( status != 0 ) return;

   for ( int trx = 0; trx < all_triples.size(); trx++ )
   {  // Mark matching triples as excluded
      QString tchan = QString( all_triples[ trx ] )
                      .section( "/", 1, 1 ).simplified();

      if ( tchan == chann )
      {
         all_tripinfo[ trx ].excluded = true;
DbgLv(1) << "DelChan:  EXCLUDED chn trx" << chann << trx;
      }
   }

   build_output_data();        // Rebuild the output data controls

   build_lambda_ctrl();        // Rebuild lambda controls

   setTripleInfo();            // Review new triple information

   plot_titles();              // Output a plot of the current data
   plot_all();
}

// Drop the triples for the selected cell/channel
void US_ConvertGui::drop_cellchan()
{
   QString celchn = lw_triple->currentItem()->text()
                    .section( "/", 0, 1 ).simplified();
   int status     = QMessageBox::information( this,
      tr( "Drop Triples of Selected Cell/Channel" ),
      tr( "You have selected a list item that implies you wish to"
          " drop all triples from cell/channel \"%1\"\n\n"
          "If that is what you intend, click \"Proceed\".\n\n"
          "Otherwise, you should \"Cancel\".\n" ).arg( celchn ),
      tr( "&Proceed" ), tr( "&Cancel" ) );

   if ( status != 0 ) return;

   for ( int trx = 0; trx < all_triples.size(); trx++ )
   {  // Mark matching triples as excluded
      QString tcelchn = QString( all_triples[ trx ] )
                        .section( "/", 0, 1 ).simplified();

      if ( tcelchn == celchn )
      {
         all_tripinfo[ trx ].excluded = true;
DbgLv(1) << "DelChan:  EXCLUDED cc trx" << celchn << trx;
      }
   }

   build_output_data();        // Rebuild the output data controls

   build_lambda_ctrl();        // Rebuild lambda controls

   setTripleInfo();            // Review new triple information

   plot_titles();              // Output a plot of the current data
   plot_all();
}

// Function to save US3 data
void US_ConvertGui::saveUS3( void )
{
   // Test to see if this is multi-speed data
   QVector< int > speeds;
   int notrips  = 0;
   int nspeeds  = countSpeeds( speeds, &notrips );
   int nitrips  = allData.count();
   int kotrips  = nitrips * nspeeds;
   bool is_buoy = ExpData.expType.contains( "buoyancy", Qt::CaseInsensitive );
   bool is_cali = ExpData.expType.contains( "calibrat", Qt::CaseInsensitive );
DbgLv(1) << "SV: ExpData.expType" << ExpData.expType << "is_buoy" << is_buoy
 << "is_cali" << is_cali;
DbgLv(1) << "SV: nspeeds,itrips,otrips" << nspeeds << nitrips << notrips;

   if ( nspeeds < 2  ||  notrips != kotrips  ||  is_buoy  ||  is_cali )
   {  // For single-speed data, save current triples
      //  and also save single run if Buoyancy or Calibration experiment type
      if ( disk_controls->db() )
      {
         saveUS3DB();          // Save AUCs to disk then DB
         writeTimeStateDisk(); // Get TimeState in results
         writeTimeStateDB();   // Save TimeState to database
      }
      else
      {
         saveUS3Disk();        // Save AUCs to disk
         writeTimeStateDisk(); // Save TimeState to disk
      }
   }

   else
   {  // For multi-speed data, report and split data into speed-runIDs
      QMessageBox::information( this,
          tr( "Multi-Speed Data to be Saved" ),
          tr( "%1 speeds are present in the data to be saved\n"
              "It will be split into single-speed runs.\n"
              "The %2 input triples will result in\n"
              "    %3 total output triples." )
              .arg( nspeeds ).arg( nitrips ).arg( notrips ) );

      QString runIDsave = runID;                        // Save run ID
      QString runIDbase = runIDsave + "-";              // Base new run ID
      QVector< US_DataIO::RawData > oriData = allData;  // Save original data
      QVector< double > oriRIPro = ExpData.RIProfile;   // Save original RI Pro
      int     nripro    = ExpData.RIProfile.count();
DbgLv(1) << "nspeeds=" << nspeeds << "nripro=" << nripro;
//if(nripro>(-2)) return;
      uchar uuid[ 16 ];
//      saveStatus        = NOT_SAVED;

      for ( int spx = 0; spx < nspeeds; spx++ )
      {
         int ispeed        = speeds[ spx ];
DbgLv(1) << "us_convert: ispeed=" << ispeed << "spx=" << spx << "nspeeds=" << nspeeds;
         runID             = runIDbase + QString().sprintf( "%05d", ispeed );
         double speed      = (double)ispeed;
         ExpData.runID     = runID;
         ExpData.expGUID.clear();
DbgLv(1) << "SV:   runID" << runID;
         le_runID2->setText( runID );

         // Create triple datasets with scans at the current speed
         for ( int trx = 0; trx < nitrips; trx++ )
         {
            allData[ trx ].scanData.clear();

            if ( trx == 0 )
               ExpData.RIProfile.clear();

            for ( int ss = 0; ss < oriData[ trx ].scanData.count(); ss++ )
            {
               if ( oriData[ trx ].scanData[ ss ].rpm == speed )
               {  // Save scan (+RI profile?) for scan where speed matches
                  allData[ trx ].scanData << oriData[ trx ].scanData[ ss ];

                  if ( trx == 0  &&  ss < nripro )
                     ExpData.RIProfile << oriRIPro[ ss ];
               }
            }
DbgLv(1) << "SV:    trx" << trx << "scans" << allData[ trx ].scanData.count();

            QString fname     = out_tripinfo[ trx ].tripleFilename;
            QString uuidst    = US_Util::new_guid();
            US_Util::uuid_parse( uuidst, uuid );
            out_tripinfo[ trx ].tripleFilename
                              = runID + "." + fname.section( ".", -5, -1 );
            out_tripinfo[ trx ].tripleID
                              = trx + 1;
            memcpy( (unsigned char*)out_tripinfo[ trx ].tripleGUID, uuid, 16 );
            memcpy( (unsigned char*)allData     [ trx ].rawGUID   , uuid, 16 );
            outData[ trx ]    = &allData[ trx ];
         }// End of 'trx' loop

         // Save the triples of the current speed-run
         if ( disk_controls->db() )
         {
            saveUS3DB();          // Save AUCs to disk then DB
            writeTimeStateDisk(); // Get TimeState in results
            writeTimeStateDB();   // Save TimeState to database
DbgLv(1) << "Writing to database";
         }
         else
         {
            saveUS3Disk();        // Save AUCs to disk
            writeTimeStateDisk(); // Save TimeState to disk
DbgLv(1) << "Writing to disk";
         }
      } // End of 'spx' for loop (counts for each speed step)
   } // End of 'else' loop for multispeed case

// x  x  x  x  x
   QMessageBox::information( this,
       tr( "Save is Complete" ),
       tr( "The save of all data and reports is complete." ) );
}

// Save to disk (default directory)
int US_ConvertGui::saveUS3Disk( void )
{
   if ( allData[ 0 ].scanData.empty() ) return US_Convert::NODATA;

   QDir     writeDir( US_Settings::resultDir() );
   QString  dirname = writeDir.absolutePath() + "/" + runID + "/";

   if ( saveStatus == NOT_SAVED  &&
        writeDir.exists( runID ) )
   {
        QMessageBox::information( this,
           tr( "Error" ),
           tr( "The write directory,  " ) + dirname +
           tr( " already exists. Please change run ID to a unique value." ) );
        return US_Convert::DUP_RUNID;
   }

   if ( ! writeDir.exists( runID ) )
   {
      if ( ! writeDir.mkpath( dirname ) )
      {
         QMessageBox::information( this,
            tr( "Error" ),
            tr( "Cannot write to " ) + writeDir.absolutePath() );
         return US_Convert::CANTOPEN;
      }
   }

   int status;

   // Check to see if we have all the data to write
   if ( ! ExpData.syncOK )
   {
      status = QMessageBox::information( this,
                  tr( "Warning" ),
                  tr( "The run has not yet been associated with the database."
                      " Click 'OK' to proceed anyway, or click 'Cancel'"
                      " and then click on the 'Edit Run Information'"
                      " button to enter this information first.\n" ),
                  tr( "&OK" ), tr( "&Cancel" ),
                  0, 0, 1 );
      if ( status != 0 ) return US_Convert::NOT_WRITTEN;
   }

   // Write the data
   le_status->setText( tr( "Saving data to disk ..." ) );
   qApp->processEvents();
   bool saveGUIDs = saveStatus != NOT_SAVED;
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   status = US_Convert::saveToDisk( outData, out_tripinfo, allExcludes,
                                    runType, runID, dirname, saveGUIDs );

   QApplication::restoreOverrideCursor();

   // Now try to write the xml file
   status = ExpData.saveToDisk( out_tripinfo, runType, runID, dirname,
                                speedsteps );

   // How many files should have been written?
   int fileCount = out_tripinfo.size();
DbgLv(1) << "SV:   fileCount" << fileCount;

   // Now try to communicate status
   if ( status == US_Convert::CANTOPEN )
   {
      QString writeFile = runID      + "."
                        + runType    + ".xml";
      QMessageBox::information( this,
            tr( "Error" ),
            tr( "Cannot open write file: " ) + dirname + writeFile );
   }

   else if ( status == US_Convert::NOXML )
   {
      // Main xml data is missing
      QMessageBox::information( this,
            tr( "Warning" ),
            tr( "The run information file was not written. "
                "Please click on the "
                "'Associate Run with DB' button \n\n " ) +
            QString::number( fileCount ) + " " +
            runID + tr( " files written." ) );
      return( status );
   }

   else if ( status == US_Convert::PARTIAL_XML )
   {
      // xml data is missing for one or more triples
      QMessageBox::information( this,
            tr( "Warning" ),
            tr( "Solution information is incomplete. Please click on the "
                "'Manage Solutions' button for each "
                "cell, channel, and wavelength combination \n\n " ) +
            QString::number( fileCount ) + " "
            + runID + tr( " files written." ) );
      return( status );
   }

   else if ( status != US_Convert::OK )
   {
      QMessageBox::information( this,
            tr( "Error" ),
            tr( "Error: " ) + status );
      return( status );
   }

   // Save or delete RI intensity profile
   if ( runType == "RI" )
   {
      if ( referenceDefined )
      {
         status = ExpData.saveRIDisk( runID, dirname );
DbgLv(1) << "SV:   saveRIDisk status" << status;

         if ( status == US_Convert::CANTOPEN )
         {
            QString writeFile = runID      + "."
                              + "RIProfile.xml";
            QMessageBox::information( this,
                  tr( "Error" ),
                  tr( "Cannot open write file: " ) + dirname + writeFile );
         }

         else if ( status != US_Convert::OK )
         {
            QMessageBox::information( this,
                  tr( "Error" ),
                  tr( "Error: " ) + status );
            return( status );

         }
      }
else
DbgLv(1) << "SV:   NO saveRIDisk : refDef" << referenceDefined;

      else
      {
         // Maybe the profile has been deleted, so let's
         // delete the xml file and avoid confusion later
         QDir d( dirname );
         QString filename = runID      + "."
                          + "RIProfile.xml";
         if ( d.exists( filename ) && ! d.remove( filename ) )
            qDebug() << "Unable to remove file" << filename;

      }
   }
else
DbgLv(1) << "SV:   NO saveRIDisk : runType" << runType;

   // Insure that we have a TimeState record locally for this run
   le_status->setText( tr( "Writing Time State to disk..." ) );
   if ( writeTimeStateDisk() == 0 )
   {  // Abort if unable to open TimeState file
      return US_Convert::CANTOPEN;
   }
   fileCount   += 2;

   // Status is OK
   le_status->setText( tr( "%1 %2 files were written to disk." )
                       .arg( fileCount ).arg( runID ) );
   qApp->processEvents();

   if ( saveStatus == NOT_SAVED )
      saveStatus = HD_ONLY;

   // If we are here, it's ok to sync with DB
   ExpData.syncOK = true;

   enableRunIDControl( false );

   if ( isMwl )
   {  // Where triples were expanded for MWL, skip plot save
      return( US_Convert::OK );
   }

   // Save the main plots
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   le_status->setText( tr( "Saving plot and report files ..." ) );
   qApp->processEvents();
   QString dir    = US_Settings::reportDir() + "/" + runID;
   if ( ! QDir( dir ).exists() )      // make sure the directory exists
      QDir().mkdir( dir );
   data_plot->setVisible( false );
   int save_tripListx = tripListx;

   // Make sure directory is empty
   QDir d( dir );
   QStringList rmvfilt;
   rmvfilt << "*.svgz" << "*.png" << "*.svg";
   QStringList rmvfiles = d.entryList( rmvfilt, QDir::Files, QDir::Name );

   for ( int ii = 0; ii < rmvfiles.size(); ii++ )
      if ( ! d.remove( rmvfiles[ ii ] ) )
DbgLv(1) << "Unable to remove file" << rmvfiles[ ii ];

   for ( int ii = 0; ii < out_tripinfo.size(); ii++ )
   {
      tripListx = ii;

      QString t              = out_tripinfo[ ii ].tripleDesc;
      QStringList parts      = t.split(" / ");

      QString     cell       = parts[ 0 ];
      QString     channel    = parts[ 1 ];
      QString     wl         = parts[ 2 ];

      QString     triple     = "";

      if ( runType == "WA" )
      {
         double r       = wl.toDouble() * 1000.0;
         QString radius = QString::number( (int) round( r ) );
         triple         = cell + channel + radius;
      }

      else
      {
         triple         = cell + channel + wl;
      }

      QString filename = dir + "/cnvt." + triple + ".raw.svgz";

      // Redo current plot and write it to a file
      lw_triple->setCurrentRow( tripListx );
      plot_current();
      int status = US_GuiUtil::save_plot( filename, data_plot );
      if ( status != 0 )
         qDebug() << filename << "plot not saved";
   }

   // Restore original plot
   tripListx = save_tripListx;
   lw_triple->setCurrentRow( tripListx );
   plot_current();
   data_plot->setVisible( true );
   le_status->setText( tr( "Saving of plot files IS COMPLETE." ) );
   qApp->processEvents();
   QApplication::restoreOverrideCursor();
   QApplication::restoreOverrideCursor();

   return( US_Convert::OK );
}

// Save to Database
void US_ConvertGui::saveUS3DB( void )
{
   // Verify connectivity
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() != US_DB2::OK )
   {
      QMessageBox::information( this,
             tr( "Error" ),
             tr( "Database connectivity error" ) );

      return;
   }
DbgLv(1) << "DBSv:  (1)trip0tripFilename" << out_tripinfo[0].tripleFilename;
DbgLv(1) << "DBSv:     tripleGUID       "
 << US_Util::uuid_unparse((uchar*)out_tripinfo[0].tripleGUID);

   // Ok, let's make sure they know what'll happen
   if ( saveStatus == BOTH )
   {
     int status = QMessageBox::information( this,
              tr( "Warning" ),
              tr( "This will overwrite the raw data currently in the " ) +
              tr( "database, and all existing edit profiles, models "  ) +
              tr( "and noise data will be deleted too. Proceed? "      ),
              tr( "&OK" ), tr( "&Cancel" ),
              0, 0, 1 );
     if ( status != 0 ) return;
   }

   else
   {
     int status = QMessageBox::information( this,
              tr( "Warning" ),
              tr( "Once this data is written to the DB you will not "  ) +
              tr( "be able to make changes to it without erasing the " ) +
              tr( "edit profiles, models and noise files too. Proceed? " ),
              tr( "&OK" ), tr( "&Cancel" ),
              0, 0, 1 );
     if ( status != 0 ) return;
   }

   // First check some of the data with the DB
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   le_status->setText( tr( "Preparing Save with DB check ..." ) );
   qApp->processEvents();

   int status = US_ConvertIO::checkDiskData( ExpData, out_tripinfo, &db );
DbgLv(1) << "Status from SaveUs3DB" << status;
   // Save a flag for need to repeat the disk write later
   bool repeat_disk = ( status == US_DB2::NO_RAWDATA );

   QApplication::restoreOverrideCursor();
DbgLv(1) << "DBSv:  (2)trip0tripFilename" << out_tripinfo[0].tripleFilename;
DbgLv(1) << "DBSv:     dset tripleID    " << out_tripinfo[0].tripleID;

   if ( status == US_DB2::NO_PERSON )  // Investigator or operator doesn't exist
   {
      QMessageBox::information( this,
            tr( "Error" ),
            tr( "This investigator or instrument operator was not found\n"
                "in the database.\n" ) );
      return;
   }

   if ( status == US_DB2::BADGUID )
   {
      QMessageBox::information( this,
            tr( "Error" ),
            tr( "Bad GUID format.\n"
                "Please click on Edit Run Information"
                " and re-select hardware.\n" ) );
      return;
   }

   if ( status == US_DB2::NO_ROTOR )
   {
      QMessageBox::information( this,
            tr( "Error" ),
            tr( "Don't recognize the rotor configuration.\n"
                "Please click on Edit Run Information"
                " and re-select hardware.\n") );
      return;
   }

   if ( status != US_DB2::OK && status != US_DB2::NO_RAWDATA )
   {
      // US_DB2::OK means we're updating; US_DB2::NO_RAWDATA means it's new
      QMessageBox::information( this,
            tr( "Error" ),
            db.lastError() + " (" + db.lastErrno() + ")\n" );
      return;
   }

   // Save updated files and prepare to transfer to DB
   le_status->setText( tr( "Preparing Save with Disk write ..." ) );
   qApp->processEvents();
DbgLv(1) << "DBSv:  (2)dset tripleID    " << out_tripinfo[0].tripleID;

   status = saveUS3Disk();
DbgLv(1) << "DBSv: Status after saveUS3Disk()" << status;

   if ( status != US_Convert::OK )
      return;
DbgLv(1) << "DBSv:  local files saved";

   QString error = QString( "" );

   // Get the directory where the auc files are
   QDir        resultDir( US_Settings::resultDir() );
   QString     dir = resultDir.absolutePath() + "/" + ExpData.runID + "/";

   if ( ! resultDir.exists( ExpData.runID ) )
   {
      QMessageBox::information( this,
            tr( "Error" ),
            tr( "Cannot read from " ) + dir ); // aucDir.absolutePath() );
      return;
   }

   QStringList nameFilters = QStringList( "*.auc" );

   QDir readDir( dir );

   QStringList files =  readDir.entryList( nameFilters,
         QDir::Files | QDir::Readable, QDir::Name );
DbgLv(1) << "DBSv:  files count" << files.size();

   if ( files.size() == 0 )
   {
      QMessageBox::warning( this,
            tr( "No Files Found" ),
            tr( "There were no files of the form *.auc\n"
                "found in the specified directory." ) );
      return;
   }

   if ( ExpData.checkRunID( &db ) == US_DB2::OK && ( saveStatus != BOTH ) )
   {
      // Then the user is trying to overwrite a runID that is already in the DB
      QMessageBox::warning( this,
            tr( "Duplicate runID" ),
            tr( "This runID already exists in the database. To edit that "
                "run information, load it from there to begin with.\n" ) );
      return;
   }

   // If saveStatus == BOTH already, then it came from the db to begin with
   // and it should be updated. Otherwise, there shouldn't be any database
   // records with this runID found
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   le_status->setText( tr( "Saving Experiment to DB ..." ) );
   qApp->processEvents();
   status = ExpData.saveToDB( ( saveStatus == BOTH ), &db, speedsteps );
   QApplication::restoreOverrideCursor();

   if ( status == US_DB2::NO_PROJECT )
   {
      QMessageBox::warning( this,
            tr( "Project missing" ),
            tr( "The project associated with this experiment could not be "
                "updated or added to the database.\n" ) );
      return;
   }

   else if ( status == US_DB2::DUPFIELD )
   {
      QMessageBox::warning( this,
            tr( "Duplicate runID" ),
            tr( "The runID already exists in the database.\n" ) );
      return;
   }

   else if ( status == US_DB2::ERROR )
   {
      // This is what happens in the case of RI data, and the xml is bad
      QMessageBox::warning( this,
         tr( "Bad RI XML" ),
         tr( "There was a problem with the xml data"
             " read from the database.\n" ) );
   }

   else if ( status != US_DB2::OK )
   {
      QMessageBox::warning( this,
         tr( "Problem saving experiment information" ),
         tr( "MySQL Error : " ) + db.lastError() + " ("
         + QString::number( status ) + ")" );
      return;
   }

   // If the data came from the database in the first place,
   // then this function erases all the edit profiles, models
   // and noise files in the database too. However, if one
   // changes most of the things here ( solution, rotor, etc. )
   // it would invalidate the data anyway.
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   le_status->setText( tr( "Writing raw data to DB ..." ) );
   qApp->processEvents();

   QString writeStatus =
      US_ConvertIO::writeRawDataToDB( ExpData, out_tripinfo, dir, &db );

   QApplication::restoreOverrideCursor();

   if ( ! writeStatus.isEmpty() )
   {
      QMessageBox::warning( this,
            tr( "Problem saving experiment" ),
            tr( "Unspecified database error: " ) + writeStatus );
      le_status->setText( tr( "*ERROR* Problem saving experiment." ) );
      return;
   }

   if ( repeat_disk )
   {  // Last disk save did not have tripleID values, so update files
      QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
      le_status->setText( tr( "Writing updated Raw records to disk..." ) );

      status = saveUS3Disk();

      QApplication::restoreOverrideCursor();
      qApp->processEvents();
DbgLv(1) << "DBSv:  REPEAT disk save for raw IDs";
   }

   // Insure that we have a TimeState record in the DB for this run
   le_status->setText( tr( "Writing Time State to DB..." ) );
   writeTimeStateDB();

   le_status->setText( tr( "DB data save complete. Saving reports..." ) );
   qApp->processEvents();
   saveStatus = BOTH;
   enableRunIDControl( false );

   saveReportsToDB();
   QApplication::restoreOverrideCursor();
   QApplication::restoreOverrideCursor();

   le_status->setText( tr( "DB data and reports save IS COMPLETE." ) );
   qApp->processEvents();
}

void US_ConvertGui::saveReportsToDB( void )
{
   if ( isMwl )
      return;

   // Verify connectivity
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() != US_DB2::OK )
   {
      QMessageBox::information( this,
             tr( "Error" ),
             tr( "Database connectivity error" ) );

      return;
   }

   // Get a list of report files produced by us_convert
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   QString dir = US_Settings::reportDir() + "/" + runID;
   QDir d( dir, "cnvt*", QDir::Name, QDir::Files | QDir::Readable );
   d.makeAbsolute();
   QStringList files = d.entryList( QDir::Files );

   // Add any run details files to it
   QDir d2( dir, "rundetail*", QDir::Name, QDir::Files | QDir::Readable );
   d2.makeAbsolute();
   files << d2.entryList( QDir::Files );

   // Create US_Report object
   QString now = QDateTime::currentDateTime().toString();
   US_Report myReport;
   myReport.reset();
   myReport.runID = runID;
   myReport.title = runID + " Report";
   myReport.html  = "<p>Report created " + now + "</p>";

   // Save all us_convert report files to the DB
   QString errorMsg = "";
   foreach( QString file, files )
   {
      // Get description info for the correct triple
      QStringList parts = file.split( "." );
      QString fileTriple = US_Util::expanded_triple( parts[ 1 ], true );

      // Match the triple to find the correct description in memory
      QString description = QString( "" );
      for ( int ii = 0; ii < out_tripinfo.size(); ii++ )
      {
         if ( fileTriple == out_tripinfo[ ii ].tripleDesc )
         {
            description = outData[ ii ]->description;
            break;
         }
      }

      // Edit data ID is not known yet, so use 1. It goes in the report document
      //   table itself, so we're not overwriting anything.
      US_Report::Status status = myReport.saveDocumentFromFile(
            dir, file, &db, 1, description );
      if ( status != US_Report::REPORT_OK )
      {
         errorMsg += file + " was not saved to report database; error code: "
                          + QString::number( status ) + "\n";
         qDebug() << "US_ConvertGui.saveDocumentFromFile error: "
                  << db.lastError() << db.lastErrno();
      }
   }
   QApplication::restoreOverrideCursor();

   if ( ! errorMsg.isEmpty() )
   {
      QMessageBox::warning( this,
            tr( "Problem saving reports to DB" ),
            errorMsg );
   }

}

int US_ConvertGui::getImports()
{
   QString dir;
   int impType  = 0;

   dir = QFileDialog::getExistingDirectory( this,
         tr( "Raw Data Directory" ),
         US_Settings::importDir(),
         QFileDialog::DontResolveSymlinks );

   dir.replace( "\\", "/" );

   if ( dir.isEmpty() )      // If no directory chosen, return now
      return -1;

   QDir readDir( dir, "*", QDir::Name, QDir::Files | QDir::Readable );
   readDir.makeAbsolute();
   if ( dir.right( 1 ) != "/" ) dir += "/";  // Ensure trailing "/"

   // See if we need to fix the runID
   QString new_runID = dir.section( "/", -2, -2 );
   QRegExp rx( "[^A-Za-z0-9_-]" );

   int pos = 0;
   bool runID_changed = false;
   while ( ( pos = rx.indexIn( new_runID ) ) != -1 )
   {
      new_runID.replace( pos, 1, "_" );      // Replace 1 char at position pos
      runID_changed = true;
   }
DbgLv(1) << "CGui:gI: dir" << dir << "new_runID" << new_runID
 << "rID_chgd" << runID_changed;

   // Let the user know if the runID name has changed
   if ( runID_changed )
   {
      QMessageBox::warning( this,
            tr( "RunID Name Changed" ),
            tr( "The runID name has been changed. It may consist only"
                "of alphanumeric \n"
                " characters, the underscore, and the hyphen. New runID: " )
            + new_runID );
   }

   // Set the runID and directory
   runID       = new_runID;
   le_runID ->setText( runID );
   le_runID2->setText( runID );
   le_dir   ->setText( dir );
   currentDir  = QString( dir );

   // Get the list of files in the chosen directory
   QStringList nameFilters = QStringList( "*" );
   QStringList files =  readDir.entryList( nameFilters,
         QDir::Files | QDir::Readable, QDir::Name );

   QString tmstFname;
   QString txmlFname;
   int nauc    = 0;
   int nmwrs   = 0;
   int nother  = 0;
   int ntmst   = 0;
   int ntxml   = 0;
   int ntotf   = files.size();
DbgLv(1) << "CGui:gI: ntotf" << ntotf;

   for ( int jj = 0; jj < ntotf; jj++ )
   {
      QString fname = files[ jj ];

      if (      fname.endsWith( ".mwrs" ) )
         nmwrs++;

      else if ( fname.endsWith( ".auc" ) )
         nauc++;

      else if ( fname.endsWith( "time_state.xml" ) )
      {
         ntxml++;
         txmlFname     = fname;
      }
      else if ( fname.endsWith( "time_state.tmst" ) )
      {
         ntmst++;
         tmstFname     = fname;
      }
      else
         nother++;
   }
DbgLv(1) << "CGui:gI: nmwrs nauc ntxml ntmst nother"
 << nmwrs << nauc << ntxml << ntmst << nother;

   if ( nmwrs > 0 )
      impType     = 1;             // Flag import of MWL data

   else if ( nauc > 0 )
      impType     = 2;             // Flag import of AUC data

   else
      impType     = 0;             // Flag import of Beckman Raw

DbgLv(1) << "CGui:gI:  RTN impType" << impType;
   return impType;
}

bool US_ConvertGui::read( void )
{
   oldRunType = runType;            // let's see if the runType changes

   // Read the legacy data
DbgLv(1) << "CGui:RD:  rdLegDat CALL";
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   US_Convert::readLegacyData( currentDir, legacyData, runType );
   QApplication::restoreOverrideCursor();
DbgLv(1) << "CGui:RD:   rdLegDat RTN  lDsz" << legacyData.size();

   if ( legacyData.size() == 0 ) return( false );

   // if runType has changed, let's clear out xml data too
   if ( oldRunType != runType ) ExpData.clear();

   return( true );
}

bool US_ConvertGui::convert( void )
{
DbgLv(1) << "CGui:CV: IN";
   // Set tolerance to stay between wl numbers
   double tolerance = (double)ct_tolerance->value();

   // Convert the data
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
   US_Convert::convertLegacyData( legacyData, allData, all_tripinfo,
                                  runType, tolerance );
   int kadata       = allData     .size();
   int katrip       = all_tripinfo.size();
DbgLv(1) << "CGui:CV: kadata katrip runType" << kadata << katrip << runType;
   QApplication::restoreOverrideCursor();

   if ( kadata == 0 ) return( false );

   le_description->setText( allData[ 0 ].description );
   saveDescription = QString( allData[ 0 ].description );

   // Now let's show the user the first one
   tripListx = -1;
   for ( int ii = 0; ii < katrip; ii++ )
   {
      if ( all_tripinfo[ ii ].excluded ) continue;

      if ( tripListx == -1 ) tripListx = ii;
DbgLv(1) << "CGui:CV:  ii, trip" << ii << all_tripinfo[ii].tripleDesc;
   }

DbgLv(1) << "CGui:CV: RTN tLx" << tripListx << "katrip" << katrip;
   return( true );
}

bool US_ConvertGui::centerpieceInfo( void )
{
DbgLv(1) << "CGui: centpInfo: db" << disk_controls->db();
   if ( disk_controls->db() )
      return centerpieceInfoDB();

   else
      return centerpieceInfoDisk();
}

// Function to get abstractCenterpiece names from DB
bool US_ConvertGui::centerpieceInfoDB( void )
{
   // Verify connectivity
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() != US_DB2::OK )
   {
      QMessageBox::information( this,
             tr( "Error" ),
             tr( "Database connectivity error" ) );

      return( false );
   }

   QStringList q( "get_abstractCenterpiece_names" );
   db.query( q );

   QList<listInfo> options;
   while ( db.next() )
   {
      struct listInfo option;
      option.ID      = db.value( 0 ).toString();
      option.text    = db.value( 1 ).toString();
      options << option;
   }

   cb_centerpiece->clear();
   if ( options.size() > 0 )
      cb_centerpiece->addOptions( options );

DbgLv(1) << "CGui: centpInfoDB RTN";
   return true;
}

// Function to get abstractCenterpiece names from disk
bool US_ConvertGui::centerpieceInfoDisk( void )
{
   // First figure out the xml file name, and try to open it
   QFile f( US_Settings::appBaseDir() + "/etc/abstractCenterpieces.xml" );

   if ( ! f.open( QIODevice::ReadOnly ) ) return false;

   QList<listInfo> options;
   QXmlStreamReader xml( &f );
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "abstractCenterpiece" )
         {
            struct listInfo option;
            QXmlStreamAttributes a = xml.attributes();
            option.ID   = a.value("id").toString();
            option.text = a.value("name").toString();
            options << option;
         }
      }
   }

   bool error = xml.hasError();
   f.close();

   if ( error ) return false;

   cb_centerpiece->clear();
   if ( options.size() > 0 )
   {
      // Let's sort them so they come up like the DB
      for ( int i = 0; i < options.size() - 1; i++ )
         for ( int j = i + 1; j < options.size(); j++ )
            if ( options[ i ].text > options[ j ].text )
               options.swap( i, j );

      cb_centerpiece->addOptions( options );
   }

DbgLv(1) << "CGui: centpInfoDk RTN";
   return true;
}

void US_ConvertGui::plot_current( void )
{
   triple_index();
DbgLv(1) << " PlCur:  tDx szoutD" << tripDatax << outData.count();

   US_DataIO::RawData currentData = *outData[ tripDatax ];

   if ( currentData.scanData.empty() ) return;

   plot_titles();

   // Plot current data for cell / channel / wavelength triple
   plot_all();

   // Set the Scan spin boxes
   enableScanControls();
}

void US_ConvertGui::plot_titles( void )
{
DbgLv(1) << "  PlTit: tDx outsz otrisz" << tripDatax << outData.size()
 << out_tripinfo.size();
   char chtype[ 3 ] = { 'R', 'A', '\0' };

   strncpy( chtype, outData[ tripDatax ]->type, 2 );
   QString dataType    = QString( chtype ).left( 2 );
   QString triple      = out_tripinfo[ tripDatax ].tripleDesc;
   QStringList parts   = triple.split(" / ");
DbgLv(1) << "  PlTit: triple" << triple << "parts" << parts;

   QString  cell       = parts[ 0 ];
   QString  channel    = parts[ 1 ];
   QString  wavelen    = isMwl ? cb_lambplot->currentText() : parts[ 2 ];

   // Plot Title and legends
   QString title;
   QString xLegend     = tr( "Radius (in cm)" );
   QString yLegend     = tr( "Absorbance" );
   QString ccwlong     = runID + tr( "\nCell: "       ) + cell
                               + tr( "  Channel: "    ) + channel
                               + tr( "  Wavelength: " ) + wavelen;
   QString ccrlong     = QString( ccwlong ).replace( "Wavelength:", "Radius:" );
DbgLv(1) << "  PlTit: dataType" << dataType;

   if      ( dataType == "RA" )
   {
      title = tr( "Radial Absorbance Data\nRun ID: " ) + ccwlong;
   }

   else if ( dataType == "RI"  &&  isPseudo )
   {
      title = tr( "Pseudo Absorbance Data\nRun ID: " ) + ccwlong;
   }

   else if ( dataType == "RI" )
   {
      title = tr( "Radial Intensity Data\nRun ID: " ) + ccwlong;
      yLegend = tr( "Radial Intensity at " ) + wavelen + " nm";
   }

   else if ( dataType == "IP" )
   {
      title = tr( "Interference Data\nRun ID: " ) + ccwlong;
      yLegend = tr( "Fringes" );
   }

   else if ( dataType == "FI" )
   {
      title = tr( "Fluorescence Intensity Data\nRun ID: " ) + ccwlong;
      yLegend = tr( "Fluorescence Intensity" );
   }

   else if ( dataType == "WA" )
   {
      title = tr( "Wavelength Data\nRun ID: " ) + ccrlong;
      xLegend = tr( "Wavelength" );
      yLegend = tr( "Absorbance" );
   }

   else if ( dataType == "WI" )
   {
      title = tr( "Wavelength Intensity Data\nRun ID: " ) + ccrlong;
      xLegend = tr( "Wavelength" );
      yLegend = tr( "Intensity" );
   }

   else
      title = tr( "File type not recognized" );

   data_plot->setTitle( title );
   data_plot->setAxisTitle( QwtPlot::yLeft, yLegend );
   data_plot->setAxisTitle( QwtPlot::xBottom, xLegend );

}

void US_ConvertGui::plot_all( void )
{
   US_DataIO::RawData* currentData = outData[ tripDatax ];

   dataPlotClear( data_plot );
   grid        = us_grid( data_plot );
   int kcpoint = currentData->pointCount();
   int kcscan  = currentData->scanCount();

   QVector< double > rvec( kcpoint );
   QVector< double > vvec( kcpoint );

   double* rr  = rvec.data();
   double* vv  = vvec.data();

   double maxR = -1.0e99;
   double minR =  1.0e99;
   double maxV = -1.0e99;
   double minV =  1.0e99;

   for ( int ii = 0; ii < kcscan; ii++ )
   {
      US_Convert::Excludes currentExcludes = allExcludes[ tripDatax ];
      if ( currentExcludes.contains( ii ) ) continue;
      US_DataIO::Scan* scan = &currentData->scanData[ ii ];

      for ( int jj = 0; jj < kcpoint; jj++ )
      {
         rr[ jj ]    = currentData->radius( jj );
         vv[ jj ]    = scan->rvalues[ jj ];

#if QT_VERSION < 0x050000
         if ( vv[ jj ] > 1.0e99 || isnan( vv[ jj ] ) )
#else
         if ( vv[ jj ] > 1.0e99  ||  vv[ jj ] != vv[ jj ] )
#endif
         {
            // For some reason vv[jj] is going off the scale
            // Don't know why, but filter out for now
            if ( jj < 5  ||  ( jj + 5 ) > kcpoint )
               qDebug() << "(rr, vv) = ( " << rr[jj] << ", " << vv[jj] << ")"
                        << " (minR, maxR) = ( " << minR << ", " << maxR << ")" << endl
                        << " (minV, maxV) = ( " << minV << ", " << maxV << ")" << endl;
            continue;
         }

         maxR = qMax( maxR, rr[ jj ] );
         minR = qMin( minR, rr[ jj ] );
         maxV = qMax( maxV, vv[ jj ] );
         minV = qMin( minV, vv[ jj ] );
      }

      QString title = tr( "Raw Data at " )
         + QString::number( scan->seconds ) + tr( " seconds" );

      QwtPlotCurve* curv = us_curve( data_plot, title );
      curv->setSamples( rr, vv, kcpoint );

   }

   // Reset the scan curves within the new limits
   double padR = ( maxR - minR ) / 30.0;
   double padV = ( maxV - minV ) / 30.0;

   data_plot->setAxisScale( QwtPlot::yLeft  , minV - padV, maxV + padV );
   data_plot->setAxisScale( QwtPlot::xBottom, minR - padR, maxR + padR );

   show_plot_progress = false;
   data_plot->replot();
}

void US_ConvertGui::replot( void )
{
   plot_all();
}

void US_ConvertGui::set_colors( const QList< int >& focus )
{
   // Get pointers to curves
   QwtPlotItemList        list = data_plot->itemList();
   QList< QwtPlotCurve* > curves;

   for ( int i = 0; i < list.size(); i++ )
   {
      if ( list[ i ]->title().text().contains( "Raw" ) )
         curves << dynamic_cast< QwtPlotCurve* >( list[ i ] );
   }

   QPen   p   = curves[ 0 ]->pen();
   QBrush b   = curves[ 0 ]->brush();
   QColor std = US_GuiSettings::plotCurve();

   // Mark these scans in red
   for ( int i = 0; i < curves.size(); i++ )
   {
      if ( focus.contains( i ) )
      {
         p.setColor( Qt::red );
      }
      else
      {
         p.setColor( std );
         b.setColor( std );
      }

      curves[ i ]->setPen  ( p );
      curves[ i ]->setBrush( b );
   }

   data_plot->replot();
}

void US_ConvertGui::draw_vline( double radius )
{
   double r[ 2 ];

   r[ 0 ] = radius;
   r[ 1 ] = radius;
#if QT_VERSION > 0x050000
   QwtScaleDiv* y_axis = (QwtScaleDiv*)&data_plot->axisScaleDiv( QwtPlot::yLeft );
#else
   QwtScaleDiv* y_axis = data_plot->axisScaleDiv( QwtPlot::yLeft );
#endif

   double padding = ( y_axis->upperBound() - y_axis->lowerBound() ) / 30.0;

   double v[ 2 ];
   v [ 0 ] = y_axis->upperBound() - padding;
   v [ 1 ] = y_axis->lowerBound() + padding;

   QwtPlotCurve* v_line = us_curve( data_plot, "V-Line" );
   v_line->setSamples( r, v, 2 );

   QPen pen = QPen( QBrush( Qt::white ), 2.0 );
   v_line->setPen( pen );

   data_plot->replot();
}

void US_ConvertGui::db_error( const QString& error )
{
   QMessageBox::warning( this, tr( "Database Problem" ),
         tr( "Database returned the following error: \n" ) + error );
}

// User changed the Lambda Start value
void US_ConvertGui::lambdaStartChanged( int value )
{
DbgLv(1) << "lambdaStartChanged" << value;
   slambda       = cb_lambstrt->itemText( value ).toInt();
   elambda       = cb_lambstop->currentText()    .toInt();
   tripListx     = lw_triple->currentRow();
   int currChan  = out_chaninfo[ tripListx ].channelID;
DbgLv(1) << "lambdaStartChanged" << value << "sl el tLx cCh"
 << slambda << elambda << tripListx << currChan;

   for ( int trx = 0; trx < all_tripinfo.count(); trx++ )
   {
      int iwavl     = all_tripinfo[ trx ]
                      .tripleDesc.section( " / ", 2, 2 ).toInt();

      if ( all_tripinfo[ trx ].channelID == currChan )
         all_tripinfo[ trx ].excluded = ( iwavl < slambda );
DbgLv(1) << "lStChg:  trx iwavl chnID excl" << trx << iwavl
 << all_tripinfo[trx].channelID << all_tripinfo[trx].excluded;
   }

   build_output_data();
DbgLv(1) << "lStChg: BlOutDa RTN";
   reset_lambdas();
DbgLv(1) << "lStChg: RsLambd RTN";
}

// User changed the Lambda End value
void US_ConvertGui::lambdaEndChanged( int value )
{
DbgLv(1) << "lambdaEndChanged" << value;
   slambda       = cb_lambstrt->currentText()    .toInt();
   elambda       = cb_lambstop->itemText( value ).toInt();
   tripListx     = lw_triple->currentRow();
   int currChan  = out_chaninfo[ tripListx ].channelID;
DbgLv(1) << "lEnChg:  val" << value << "sl el tLx cCh"
 << slambda << elambda << tripListx << currChan;

   for ( int trx = 0; trx < all_tripinfo.count(); trx++ )
   {
      int iwavl     = all_tripinfo[ trx ]
                      .tripleDesc.section( " / ", 2, 2 ).toInt();

      if ( all_tripinfo[ trx ].channelID == currChan  &&  iwavl > elambda )
         all_tripinfo[ trx ].excluded = ( iwavl > elambda );
   }

   build_output_data();
DbgLv(1) << "lEnChg: BlOutDa RTN";
   reset_lambdas();
DbgLv(1) << "lEnChg: RsLambd RTN";
}

// User changed the Lambda Plot value
void US_ConvertGui::lambdaPlotChanged( int value )
{
   triple_index();

   plot_current();
   pb_lambprev->setEnabled( value > 0 );
   pb_lambnext->setEnabled( ( value + 1 ) < cb_lambplot->count() );
}

// User clicked the Previous Lambda Plot button
void US_ConvertGui::lambdaPrevClicked( )
{
   int wvx     = qMax( 0, cb_lambplot->currentIndex() - 1 );

   cb_lambplot->setCurrentIndex( wvx );
}

// User clicked the Next Lambda Plot button
void US_ConvertGui::lambdaNextClicked( )
{
   int wvxmax  = cb_lambplot->count() - 1;
   int wvx     = qMin( wvxmax, cb_lambplot->currentIndex() + 1 );

   cb_lambplot->setCurrentIndex( wvx );
}

// Show or hide MWL controls
void US_ConvertGui::show_mwl_control( bool show )
{
   lb_mwlctrl  ->setVisible( show );
   lb_lambstrt ->setVisible( show );
   cb_lambstrt ->setVisible( show );
   lb_lambstop ->setVisible( show );
   cb_lambstop ->setVisible( show );
   lb_lambplot ->setVisible( show );
   cb_lambplot ->setVisible( show );
   pb_lambprev ->setVisible( show );
   pb_lambnext ->setVisible( show );
   pb_dropCelch->setVisible( show );
   pb_dropChan ->setVisible( show );

   pb_exclude  ->setVisible( !show );
   pb_include  ->setVisible( !show );

   adjustSize();
}

// Determine triple index (separate list,data for MWL)
void US_ConvertGui::triple_index()
{
   tripListx      = lw_triple->currentRow();
   tripDatax      = isMwl
                  ? out_chandatx[ tripListx ] + cb_lambplot->currentIndex()
                  : tripListx;
DbgLv(1) << " triple_index  trLx trDx" << tripListx << tripDatax
 << "  triple" << lw_triple->currentItem()->text();
}

// Turn MWL connections on or off
void US_ConvertGui::mwl_connect( bool connect_on )
{
   if ( connect_on )
   {
      connect( cb_lambstrt,  SIGNAL( currentIndexChanged( int    ) ),
               this,         SLOT  ( lambdaStartChanged ( int    ) ) );
      connect( cb_lambstop,  SIGNAL( currentIndexChanged( int    ) ),
               this,         SLOT  ( lambdaEndChanged   ( int    ) ) );
      connect( cb_lambplot,  SIGNAL( currentIndexChanged( int    ) ),
               this,         SLOT  ( lambdaPlotChanged  ( int    ) ) );
      connect( pb_lambprev,  SIGNAL( clicked            (        ) ),
               this,         SLOT  ( lambdaPrevClicked  (        ) ) );
      connect( pb_lambnext,  SIGNAL( clicked            (        ) ),
               this,         SLOT  ( lambdaNextClicked  (        ) ) );
   }

   else
   {
      cb_lambstrt->disconnect();
      cb_lambstop->disconnect();
      cb_lambplot->disconnect();
      pb_lambprev->disconnect();
      pb_lambnext->disconnect();
   }
}

// Reset with lambda delta,range changes
void US_ConvertGui::reset_lambdas()
{
   if ( ! isMwl )
      return;

   int plambda   = cb_lambplot->currentText().toInt();
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

DbgLv(1) << "rsL: slambda elambda" << slambda << elambda;
   mwl_data.set_lambdas   ( slambda, elambda, tripListx );
   mwl_connect( false );

   nlambda       = mwl_data.lambdas( exp_lambdas );
   int plx       = nlambda / 2;
   cb_lambplot->clear();
DbgLv(1) << "rsL:  nlambda" << nlambda;

   for ( int wvx = 0; wvx < nlambda; wvx++ )
   {  // Add to plot-lambda list and possibly detect old selection in new list
      int ilamb     = exp_lambdas[ wvx ];
      cb_lambplot->addItem( QString::number( ilamb ) );

      if ( ilamb == plambda )  plx = wvx;
   }

DbgLv(1) << "rsL:  plambda" << plambda << "plx" << plx;
   cb_lambplot->setCurrentIndex( plx );

   // Rebuild list of triples
   QStringList celchns;
   QString     pwvln = " / " + QString::number( exp_lambdas[ 0           ] )
                     + "-"   + QString::number( exp_lambdas[ nlambda - 1 ] )
                     + " ("  + QString::number( nlambda ) + ")";

   int ncelchn   = mwl_data.cellchannels( celchns );
   nlambda       = mwl_data.countOf( "lambda" );
DbgLv(1) << "rsL: ncc nl szcc szci" << ncelchn << nlambda << celchns.count()
 << all_chaninfo.count();

   for ( int ii = 0; ii < ncelchn; ii++ )
   {  // Redo internal channel information description field
      all_chaninfo[ ii ].tripleDesc  = celchns[ ii ] + pwvln;
   }

   mwl_connect( true );
   QApplication::restoreOverrideCursor();

   setTripleInfo();
DbgLv(1) << "rsL: sTrInf RTN";

   init_excludes();
DbgLv(1) << "rsL: InExcl RTN";

   plot_current();
DbgLv(1) << "rsL: PlCurr RTN";
}

// Do pseudo-absorbance calculation and apply for MultiWaveLength case
void US_ConvertGui::PseudoCalcAvgMWL( void )
{
   QVector< double >  ri_prof;      // RI profile for a single wavelength
   ExpData.RIProfile.clear();       // Composite RI profile
   ExpData.RIwvlns  .clear();       // RI profile wavelengths

   if ( referenceDefined ) return;  // Average calculation is already done

   US_DataIO::RawData* refData = outData[ tripDatax ];
   int ref_size = refData->xvalues.size();
   int ccx      = tripListx;
   int tripx    = out_chandatx[ ccx ];
   nlambda      = mwl_data.lambdas( exp_lambdas, tripListx );
   ExpData.RI_nscans = refData->scanData.size();
   ExpData.RI_nwvlns = nlambda;
DbgLv(1) << "PseCalcAvgMWL: ccx tripx nlambda" << ccx << tripx << nlambda;

   // Loop to calculate reference data for each wavelength,
   //  then apply it to all triples with that same wavelength.
   for ( int wvx = 0; wvx < nlambda; wvx++ )
   {
      refData      = outData[ tripx ];
      ref_size     = refData->xvalues.size();
      int nscan    = refData->scanData.size();
      ri_prof.clear();

      // Get the reference profile for the current wavelength
      for ( int ss = 0; ss < nscan; ss++ )
      {
         US_DataIO::Scan* scan = &refData->scanData[ ss ];

         int    rr    = 0;
         int    count = 0;
         double sum   = 0.0;

         while ( refData->radius( rr ) < reference_start  &&  rr < ref_size )
            rr++;

         while ( refData->radius( rr ) < reference_end  &&
                 rr < ref_size )
         {
            sum         += scan->rvalues[ rr ];
            count++;
            rr++;
         }

         double rip_avg  = count > 0 ? ( sum / (double)count ) : 1.0;
         ri_prof           << rip_avg;
         ExpData.RIProfile << rip_avg;

      } // END: scan loop for reference triple

      int rip_size = ri_prof.size();
      int iwavl    = exp_lambdas[ wvx ];
DbgLv(1) << "PseCalcAvgMWL:  wvx" << wvx << "rsiz wavl" << rip_size << iwavl;
      ExpData.RIwvlns << iwavl;

      // Now calculate the pseudo-absorbance for all cell/channel/this-lambda
      for ( int trx = 0; trx < outData.size(); trx++ )
      {
         int jwavl    = out_triples[ trx ].section( " / ", -1, -1 ).toInt();

         if ( jwavl != iwavl )  continue;

         // Triple data has the current wavelength value
         US_DataIO::RawData* currentData = outData[ trx ];

         for ( int ss = 0; ss < currentData->scanData.size(); ss++ )
         {
            US_DataIO::Scan* scan = &currentData->scanData[ ss ];
            // Check for boundary condition
            int    ndx    = ( ss < rip_size ) ? ss : rip_size - 1;
            double ripval = ri_prof[ ndx ];

            for ( int rr = 0; rr < scan->rvalues.size(); rr++ )
            {
               // Protect against possible inf's and nan's, if a reading
               // evaluates to 0 or wherever log function is undefined or -inf
               double rvalue       = qMax( 1.0, scan->rvalues[ rr ] );

               scan->rvalues[ rr ] = log10( ripval / rvalue );
            } // END: readings loop for a scan

         } // END: scan loop for a specific triple

      } // END: cell-channel apply loop for one wavelength

      tripx++;
DbgLv(1) << "PseCalcAvgMWL:   tripx" << tripx;
   } // END: WaveLength loop

   isPseudo         = true;
   referenceDefined = true;
DbgLv(1) << "CGui: (9)referDef=" << referenceDefined;
   pb_intensity->setEnabled( true );
   pb_reference->setEnabled( false );
   pb_cancelref->setEnabled( true );
}

// Do MWL Gui setup after import or load of MWL data
void US_ConvertGui::mwl_setup()
{
   mwl_connect( false );

   // Propagate initial lists of Lambdas
   nlamb_i         = mwl_data.lambdas_raw( all_lambdas );
   cb_lambstrt->clear();
   cb_lambstop->clear();

   for ( int ii = 0; ii < nlamb_i; ii++ )
   {
      QString clamb = QString::number( all_lambdas[ ii ] );
      cb_lambstrt->addItem( clamb );
      cb_lambstop->addItem( clamb );
   }

   cb_lambstrt->setCurrentIndex( 0 );
   cb_lambstop->setCurrentIndex( nlamb_i - 1 );
   nlambda         = mwl_data.lambdas( exp_lambdas );
   cb_lambplot->clear();

   for ( int ii = 0; ii < nlambda; ii++ )
   {
      QString clamb = QString::number( exp_lambdas[ ii ] );
      cb_lambplot->addItem( clamb );
   }

   cb_lambplot->setCurrentIndex( nlambda / 2 );

   show_mwl_control( true );
   mwl_connect( true );

   setTripleInfo();
   le_description -> setText( allData[ 0 ].description );

   // Initialize exclude list
   init_excludes();

   plot_current();

   saveStatus = NOT_SAVED;

   // Ok to enable some buttons now
   enableControls();

   int    rlamb_s  = all_lambdas[ 0 ];
   int    rlamb_e  = all_lambdas[ nlamb_i - 1 ];
   int    ntriple  = all_tripinfo.size();
   static QChar clambda( 955 );   // Lambda character
   QString lambmsg = tr( "MWL imported : %1 triples :  %2 %3 to %4" )
      .arg( ntriple ).arg( clambda ).arg( rlamb_s ).arg( rlamb_e );
//   le_lambraw->setText( lambmsg );
   le_status->setText( lambmsg );
   qApp->processEvents();
   adjustSize();
}

// Initialize output data pointers and lists
bool US_ConvertGui::init_output_data()
{
   bool success   = true;
   bool have_trip = ( all_tripinfo.size() == allData.size() );
DbgLv(1) << "CGui:IOD: have_trip" << have_trip;
   if ( ! have_trip )
      all_tripinfo.clear();
   outData     .clear();
   all_chaninfo.clear();
   all_triples .clear();
   all_channels.clear();
   out_tripinfo.clear();
   out_chaninfo.clear();
   out_triples .clear();
   out_channels.clear();
   out_chandatx.clear();

   // Set up initial export-data pointers list and all/out lists
   for ( int trx = 0; trx < allData.size(); trx++ )
   {
      US_DataIO::RawData*    edata    = &allData[ trx ];
      US_Convert::TripleInfo tripinfo;
      US_Convert::TripleInfo chaninfo;

      outData      << edata;

      if ( have_trip )
         tripinfo = all_tripinfo[ trx ];

      QString triple   = QString::number( edata->cell ) + " / "
         + QString( edata->channel ) + " / "
         + ( ! runType.contains( "W" )
             ? QString::number( qRound( edata->scanData[ 0 ].wavelength ) )
             : tripinfo.tripleDesc.section( " / ", 2, 2 ) );
      QString celchn   = triple.section( " / ", 0, 1 );
      int     chanID   = all_channels.indexOf( celchn ) + 1;
      chanID           = ( chanID < 1 ) ? ( all_channels.count() + 1 ) : chanID;
      tripinfo.tripleID    = trx + 1;
      tripinfo.tripleDesc  = triple;
      tripinfo.description = edata->description;
      tripinfo.channelID   = chanID;
      tripinfo.excluded    = false;
      chaninfo             = tripinfo;
      chaninfo.tripleDesc  = celchn;
DbgLv(1) << "CGui:IOD: trx" << trx << "triple" << triple;

      if ( have_trip )
         all_tripinfo[ trx ] = tripinfo;
      else
         all_tripinfo << tripinfo;
      out_tripinfo << tripinfo;
      all_triples  << triple;
      out_triples  << triple;

      if ( ! all_channels.contains( celchn )  ||  runType.contains( "W" ) )
      {
         all_channels << celchn;
         all_chaninfo << chaninfo;
         out_channels << celchn;
         out_chaninfo << chaninfo;
         out_chandatx << trx;
DbgLv(1) << "CGui:IOD:  ochx" << trx << "celchn cID" << celchn << chanID;
      }
   }

   // Save a vector of speed steps read or computed from the data scans
   char chtype[ 3 ] = { 'R', 'A', '\0' };
   strncpy( chtype, allData[ 0 ].type, 2 );
   QString dataType = QString( chtype ).left( 2 );
   int     nspeed   = US_SimulationParameters::readSpeedSteps( runID, dataType,
                                                               speedsteps );
   int     nspeedc  = 0;
DbgLv(1) << "CGui:IOD:   rSS nspeed" << nspeed;

   if ( nspeed == 0 )
   {  // Compute speed steps from all raw auc data
      US_SimulationParameters::computeSpeedSteps( allData, speedsteps );
      nspeedc          = speedsteps.size();
      nspeed           = nspeedc;
   }
DbgLv(1) << "CGui:IOD:   cSS nspeed" << speedsteps.size();

   // MultiWaveLength if channels and triples counts differ
   isMwl            = ( all_chaninfo.count() != all_tripinfo.count() );
   if ( isMwl  &&  ( all_tripinfo.count() / all_chaninfo.count() ) < 4 )
   {  // If less than 4 wavelengths, treat as non-MWL
      isMwl            = false;
      all_chaninfo     = all_tripinfo;
      out_chaninfo     = out_tripinfo;
      out_chandatx.clear();

      for ( int jj = 0; jj < out_tripinfo.count(); jj++ )
         out_chandatx << jj;
DbgLv(1) << "CGui:IOD:    isMwl" << isMwl << "ac.count at.count oc.count"
 << all_chaninfo.count() << all_tripinfo.count() << out_chaninfo.count();
   }

   if ( isMwl )
   {  // If MWL, update speed steps
DbgLv(1) << "CGui:IOD:   updSS call";
      QVector< SP_SPEEDPROFILE > testss = speedsteps;
      int nstest    = mwl_data.update_speedsteps( testss );
      if ( nstest > 0 )
      {
         nspeed        = nstest;
         speedsteps    = testss;
      }
      else if ( nstest < 0 )
      {
         QMessageBox::critical( this,
            tr( "Invalid MWL Speedsteps" ),
            tr( "The \"set_speed\" values in MWRS files have"
                " resulted in too many speed steps or steps"
                " where speeds decrease.\n\nImport is ABORTED!!" ) );
         success       = false;
         resetAll();
      }
DbgLv(1) << "CGui:IOD:    updSS nspeed nstest nspeedc"
 << nspeed << nstest << nspeedc << "ss0  timf omgf"
 << speedsteps[0].time_first << speedsteps[0].w2t_first;
   }

   else
   {  // Else complete channel lists
      for ( int trx = 0; trx < all_tripinfo.count(); trx++ )
      {
         QString triple = all_tripinfo[ trx ].tripleDesc;
DbgLv(1) << "CGui:IOD:     trx" << trx << "triple" << triple;
         all_chaninfo[ trx ].tripleDesc = triple;
         out_chaninfo[ trx ].tripleDesc = triple;
      }
DbgLv(1) << "CGui:IOD:   nspeed" << nspeed << "sp0.rspeed sp0.avspeed"
 << nspeed << speedsteps[0].rotorspeed << speedsteps[0].avg_speed;
   }

DbgLv(1) << "CGui:IOD: RETURN";
   return success;
}

// Build output data pointers and lists after new exclusions
void US_ConvertGui::build_output_data()
{
DbgLv(1) << "CGui: BOD: allsz" << allData.size() << "outsz" << outData.size();
   outData     .clear();   // Pointers to output data
   out_tripinfo.clear();   // Output triple information objects
   out_triples .clear();   // Output triple strings ("1 / A / 250")
   out_chaninfo.clear();   // Output channel information objects
   out_channels.clear();   // Output channel strings ("2 / B")
   out_chandatx.clear();   // Triple start indexes for each channel
   int outx       = 0;     // Output triple data index

   // Set up updated export-data pointers list and supporting output lists
   for ( int trx = 0; trx < allData.size(); trx++ )
   {
      US_Convert::TripleInfo* tripinfo = &all_tripinfo[ trx ];

      if ( tripinfo->excluded )  continue;

      outData      << &allData[ trx ];

      QString triple = tripinfo->tripleDesc;
      QString celchn = triple.section( " / ", 0, 1 );

      out_tripinfo << *tripinfo;
      out_triples  << triple;

      if ( !isMwl  ||  ! out_channels.contains( celchn ) )
      {
         out_channels << celchn;
         out_chaninfo << *tripinfo;
         out_chandatx << outx;
      }
DbgLv(1) << "CGui: BOD:  trx" << trx << "add to out" << outx << triple;

      outx++;
   }
DbgLv(1) << "CGui: BOD: outsz" << outData.size();
}

// Turn ct_tolerance connection on or off
void US_ConvertGui::connectTolerance( bool setConnect )
{
   if ( setConnect )
   {
      connect( ct_tolerance,   SIGNAL( valueChanged         ( double ) ),
                               SLOT  ( toleranceValueChanged( double ) ) );
   }
   else
   {
      ct_tolerance->disconnect();
   }
}

// Function to count speeds present in all data
int US_ConvertGui::countSpeeds( QVector< int >& speeds, int* pNotrips  )
{
   speeds.clear();                      // Initial list of unique speeds
   int nspeed        = 0;               // Total unique speeds encountered
   int notrips       = 0;               // Count of total output triples
   int nitrips       = allData.size();  // Count of input triples
   int pspeed        = 0;               // Speed at previous scan

   if ( isMwl )
   {  // Skip the speed counting for MWL
      return nspeed;
   }

   for ( int trx = 0; trx < nitrips; trx++ )
   {  // Examine the speeds within each triple
      US_DataIO::RawData* edata  = &allData[ trx ];

      for ( int ss = 0; ss < edata->scanData.size(); ss++ )
      {  // Test each scan's speed
         int speed      = qRound( edata->scanData[ ss ].rpm );

         if ( ! speeds.contains( speed ) )
         {  // New speed:  add to list and bump unique-speed count
            speeds << speed;            // Newly encountered speed
            nspeed++;                   // Count of unique speeds
         }

         if ( speed != pspeed )
         {  // Not the same speed as previous scan:  bump out-triple count
            pspeed         = speed;     // Previous speed for next iteration
            notrips++;                  // Count of output triples
         }
      }
DbgLv(1) << "cS: trx nspeed notrips" << nspeed << notrips;
   }

   if ( pNotrips != NULL )
      *pNotrips      = notrips;         // Return number of output triples

   return nspeed;                       // Return number of unique speeds
DbgLv(1) << "No of speeds from US_CONVERT=" << nspeed;
}
//------------------------------------------------
// Function to write TimeState files to local disk
//------------------------------------------------
int US_ConvertGui::writeTimeStateDisk()
{
   US_TimeState timestate;
   QString tmst_fbase   = runID + ".time_state.tmst";
   QString defs_fbase   = runID + ".time_state.xml";
   QDir     writeDir( US_Settings::resultDir() ); // Writes timestate to results directory
   QString  dirname     = writeDir.absolutePath() + "/" + runID + "/";

   //QString tmst_fdir    = currentDir;
   QString tmst_fdir    = dirname;


   if ( tmst_fdir.right( 1 ) != "/" )   tmst_fdir + "/";
   QString tmst_fname   = tmst_fdir + tmst_fbase;
   QString defs_fname   = tmst_fdir + defs_fbase;
   QString defs_fnamei  = QString( tmst_fnamei ).section( ".", 0, -2 ) + ".xml";
   int     nspeed       = speedsteps.size();
   QVector< double > rrpms;
   int     nmscans      = isMwl ? mwl_data.raw_speeds( rrpms ) : 0;

//   QDir     writeDir( US_Settings::resultDir() );
//   QString  dirname = writeDir.absolutePath() + "/" + runID + "/";

DbgLv(1) << "wTS: runid=" << runID << " Current directory=" << currentDir
 << "Time state file name=" << tmst_fname
 << "tmst_fnamei" << tmst_fnamei << "nspeed" << nspeed;

   // Determine if TimeState of right character already exists
   if ( ! tmst_fnamei.isEmpty() )
   {  // An input TMST exists
      timestate.open_read_data( tmst_fnamei );
      bool constti;
      double timeinc;
      double ftime;
      QStringList fkeys;
      QStringList ffmts;
      int  ntimes     = timestate.time_range( &constti, &timeinc, &ftime );
      int  nvalues    = timestate.field_keys( &fkeys, &ffmts );
      timestate.close_read_data();
DbgLv(1) << "wTS: TMST file exists:" << tmst_fnamei;

      if ( tmst_fnamei == tmst_fname  &&  defs_fnamei == defs_fname )
      {  // Same paths,  so we are done
DbgLv(1) << "wTS: existing TMST in run directory - NO UPDATE.";
         //return ntimes;
      }

      // See if all file characteristics match what is now needed
      bool use_exstsf = ( ntimes >= nmscans );  // Use existing TMST file?
      use_exstsf      = ( fkeys.contains( "RawSpeed" ) ) ? use_exstsf : false;
      use_exstsf      = ( nvalues > 2 )      ? use_exstsf : false;
      if ( constti )
      {  // Constant time increment
         use_exstsf   = ( ntimes > nmscans ) ? use_exstsf : false;
         use_exstsf   = ( timeinc > 0.0 )    ? use_exstsf : false;
      }
      else
      {  // Non-constant time increment
         use_exstsf   = ( ntimes >= nmscans ) ? use_exstsf : false;
         use_exstsf   = ( timeinc == 0.0    ) ? use_exstsf : false;
         use_exstsf   = ( fkeys.contains( "Time" ) ) ? use_exstsf : false;
      }
DbgLv(1) << "wTS: use_exstsf" << use_exstsf << "ntimes nmscans nvalues"
 << ntimes << nmscans << nvalues;

      if ( use_exstsf )
      {  // Existing files have enough: we can use them;  need to copy?

         if ( tmst_fnamei.contains( "imports" ) )
         {  // Existing are in */imports, so copy to current (results/run)
            QFile::copy( tmst_fnamei, tmst_fname );
            QFile::copy( defs_fnamei, defs_fname );
DbgLv(1) << "wTS:  COPY from IMPORTS";
         }

         else if ( tmst_fnamei.startsWith( tmst_fdir ) )
         {  // In current directory, so only a name change  (move)
            if ( tmst_fname != tmst_fnamei )
               QFile::rename( tmst_fnamei, tmst_fname );
            if ( defs_fname != defs_fnamei )
               QFile::rename( defs_fnamei, defs_fname );
DbgLv(1) << "wTS:  RENAME in RESULTS";
         }

         else
         {  // They are somewhere else, so copy to current
            QFile::copy( tmst_fnamei, tmst_fname );
            QFile::copy( defs_fnamei, defs_fname );
DbgLv(1) << "wTS:  COPY from OTHER:" << tmst_fnamei;
         }

         // Flag destination file as the input file (in case we re-enter)
         tmst_fnamei  = tmst_fname;
         return ntimes;
      }

      else if ( tmst_fnamei.startsWith( tmst_fdir ) )
      {  // Set is in current directory but not useable as is, so delete
         QFile::remove( tmst_fnamei );
         QFile::remove( defs_fnamei );
         tmst_fnamei.clear();
DbgLv(1) << "wTS:  DELETE from RESULTS";
      }
   }
else
DbgLv(1) << "wTS: EMPTY: tmst_fnamei";

   US_DataIO::RawData* rdata    = outData[ 0 ];

   simparams.speed_step        = speedsteps;

   simparams.sim  = ( rdata->channel == 'S' );
   int n_times    = US_AstfemMath::writetimestate( tmst_fname, simparams, *rdata );

DbgLv(1) << "number of times in file" << n_times;
   // Flag destination file as the input file (in case we re-enter)
   QFile::copy( tmst_fnamei, tmst_fname );
   tmst_fnamei  = tmst_fname;
   return n_times;
}

// Function to write TimeState files to the database
int US_ConvertGui::writeTimeStateDB()
{
   int status           = 0;
   US_Passwd pw;
   US_DB2 db( pw.getPasswd() );

   if ( db.lastErrno() != US_DB2::OK )
   {
DbgLv(1) << "wTSdb: connect" << db.lastErrno() << db.lastError();
      return -99;
   }
//DbgLv(1) << "wTSdb: connect" << db.lastErrno() << db.lastError();

   QDir     writeDir( US_Settings::resultDir() ); // Writes timestate to results directory
   QString  dirname     = writeDir.absolutePath() + "/" + runID + "/";

   QString tmst_fdir    = dirname;
   QString tmst_fbase   = runID + ".time_state.tmst";
   QString tmst_fname   = tmst_fdir + tmst_fbase;
   QString tmst_cksm    = US_Util::md5sum_file( tmst_fname );
DbgLv(1) << "wTSdb: fname " << tmst_fname << "cksm" << tmst_cksm<< "fbase" << tmst_fbase;

   // Create a new TimeState DB record, if need be. If one already exists,
   //  it may be updated or it may be deleted and recreated.
   int timestateID = US_TimeState::dbCreate( &db, ExpData.expID, tmst_fname );
DbgLv(1) << "wTSdb: post-create timestateID" << timestateID;

   // If a TimeState DB record now exists, check for binary data in it.
   //  If need be, upload binary TMST data
   if ( timestateID > 0 )
   {  // A record has been created/updated/affirmed, so check for data blob
      QString cksum;
      status            = US_TimeState::dbExamine( &db, &timestateID, NULL,
                                                   NULL, NULL, &cksum, NULL );
DbgLv(1) << "wTSdb:  examine status" << status << "cksum-db" << cksum;

      if ( status == US_DB2::OK  &&  cksum != tmst_cksm )
      {  // No or mismatched blob in DB, so upload it
         status            = US_TimeState::dbUpload( &db, timestateID,
                                                     tmst_fname );
DbgLv(1) << "wTSdb:   upload status" << status;
      }
   }
DbgLv(1) << "wTSdb:  Timestate from DB is used";
   return status;
}

// Build lambda controls
void US_ConvertGui::build_lambda_ctrl()
{
   if ( ! isMwl )
      return;

   // For MWL, rebuild wavelength controls
   int ntrip    = out_tripinfo.count();
DbgLv(1) << "BldLCtr: ntrip" << ntrip;
   int ccx      = out_tripinfo[ 0 ].channelID - 1;
DbgLv(1) << "BldLCtr:  st ccx" << ccx;
   int wvx      = 0;
   int ccxo     = 0;
   exp_lambdas.clear();

   for ( int trx = 0; trx < ntrip; trx++ )
   {  // Loop to add non-excluded wavelengths
      int ichan    = out_tripinfo[ trx ].channelID - 1;
      int iwavl    = out_triples [ trx ].section( " / ", 2, 2 ).toInt();
DbgLv(1) << "BldLCtr: trx ichan ccx iwavl" << trx << ichan << ccx << iwavl;

      if ( ichan != ccx )
      {  // If a new channel, store the previous channel's lambda list
         mwl_data.set_lambdas( exp_lambdas, ccxo );
DbgLv(1) << "BldLCtr:   trx ichan ccx ccxo iwavl nlam" << trx << ichan << ccx
 << ccxo << iwavl << exp_lambdas.count() << wvx;
         wvx       = 0;
         exp_lambdas.clear();

         ccx       = ichan;
         ccxo++;
      }

      exp_lambdas << iwavl;
      wvx++;
   }

   // Store the last channel's lambdas
   mwl_data.set_lambdas( exp_lambdas, ccxo );
DbgLv(1) << "BldLCtr:    ccxo nlam" << ccxo << exp_lambdas.count();
   mwl_connect( false );

   // Set controls for the currently selected cell/channel in the list
   tripListx    = lw_triple->currentRow();
   nlambda      = mwl_data.lambdas( exp_lambdas, tripListx );
   slambda      = exp_lambdas[ 0 ];
   elambda      = exp_lambdas[ nlambda - 1 ];

DbgLv(1) << "BldLCtr:     ccx nlam" << 0 << exp_lambdas.count();
   cb_lambstrt->setCurrentIndex( all_lambdas.indexOf( slambda ) );
   cb_lambstop->setCurrentIndex( all_lambdas.indexOf( elambda ) );
   cb_lambplot->setCurrentIndex( nlambda / 2 );
   mwl_connect( true );

   reset_lambdas();      // Make sure internal MWL lambdas are right

}
