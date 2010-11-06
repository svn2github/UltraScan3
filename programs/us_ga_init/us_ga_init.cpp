//! \file us_ga_init.cpp

#include <QApplication>

#include <uuid/uuid.h>

#include "us_ga_init.h"
#include "us_model_loader.h"
#include "us_license_t.h"
#include "us_license.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_math2.h"
#include "us_matrix.h"
#include "us_constants.h"

// main program
int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );

   #include "main1.inc"

   // License is OK.  Start up.
   
   US_GA_Initialize w;
   w.show();                   //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

// qSort LessThan method for Solute sort
bool distro_lessthan( const Solute &solu1, const Solute &solu2 )
{  // TRUE iff  (s1<s2) || (s1==s2 && k1<k2)
   return ( solu1.s < solu2.s ) ||
          ( ( solu1.s == solu2.s ) && ( solu1.k < solu2.k ) );
}

const double epsilon = 0.005;    // equivalence magnitude ratio radius

// US_GA_Initialize class constructor
US_GA_Initialize::US_GA_Initialize() : US_Widgets()
{
   // set up the GUI

   setWindowTitle( tr( "Genetic Algorithm Initialization Control Window" ) );
   setPalette( US_GuiSettings::frameColor() );

   // primary layouts
   QHBoxLayout* main  = new QHBoxLayout( this );
   QVBoxLayout* left  = new QVBoxLayout();
   QVBoxLayout* rght  = new QVBoxLayout();
   QGridLayout* spec  = new QGridLayout();
   main->setSpacing        ( 2 );
   main->setContentsMargins( 2, 2, 2, 2 );
   left->setSpacing        ( 0 );
   left->setContentsMargins( 0, 1, 0, 1 );
   spec->setSpacing        ( 1 );
   spec->setContentsMargins( 0, 0, 0, 0 );
   rght->setSpacing        ( 0 );
   rght->setContentsMargins( 0, 1, 0, 1 );

   int s_row = 0;

   // series of rows: most of them label on left, counter/box on right
   lb_info1      = us_banner( tr( "Genetic Algorithm Controls" ) );
   spec->addWidget( lb_info1, s_row++, 0, 1, 2 );

   lb_nisols     = us_label( tr( "Number of Initial Solutes:" ) );
   lb_nisols->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_nisols, s_row, 0 );

   ct_nisols     = us_counter( 3, 0.0, 1000.0, 0.0 );
   ct_nisols->setStep( 1 );
   ct_nisols->setEnabled( true );
   spec->addWidget( ct_nisols, s_row++, 1 );
   connect( ct_nisols, SIGNAL( valueChanged(  double ) ),
            this,      SLOT(   update_nisols( double ) ) );

   lb_wsbuck     = us_label( tr( "Width of s Bucket:" ) );
   lb_wsbuck->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_wsbuck, s_row, 0 );

   ct_wsbuck     = us_counter( 3, 0.0, 10.0, 0.0 );
   ct_wsbuck->setStep( 1 );
   spec->addWidget( ct_wsbuck, s_row++, 1 );
   connect( ct_wsbuck, SIGNAL( valueChanged(  double ) ),
            this,      SLOT(   update_wsbuck( double ) ) );

   lb_hfbuck     = us_label( tr( "Height of f/f0 Bucket:" ) );
   lb_hfbuck->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_hfbuck, s_row, 0 );

   ct_hfbuck     = us_counter( 3, 0.0, 1.0, 0.0 );
   ct_hfbuck->setStep( 1 );
   spec->addWidget( ct_hfbuck, s_row++, 1 );
   connect( ct_hfbuck, SIGNAL( valueChanged(  double ) ),
            this,      SLOT(   update_hfbuck( double ) ) );

   lb_info2      = us_banner( tr( "Pseudo-3D Controls" ) );
   spec->addWidget( lb_info2, s_row++, 0, 1, 2 );

   lb_resolu     = us_label( tr( "Pseudo-3D Resolution:" ) );
   lb_resolu->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_resolu, s_row, 0 );

   ct_resolu     = us_counter( 3, 0.0, 100.0, 90.0 );
   ct_resolu->setStep( 1 );
   spec->addWidget( ct_resolu, s_row++, 1 );
   connect( ct_resolu, SIGNAL( valueChanged( double ) ),
            this,      SLOT( update_resolu( double ) ) );

   lb_xreso      = us_label( tr( "X Resolution:" ) );
   lb_xreso->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_xreso, s_row, 0 );

   ct_xreso      = us_counter( 3, 10.0, 1000.0, 0.0 );
   ct_xreso->setStep( 1 );
   spec->addWidget( ct_xreso, s_row++, 1 );
   connect( ct_xreso,  SIGNAL( valueChanged( double ) ),
            this,      SLOT( update_xreso( double ) ) );

   lb_yreso      = us_label( tr( "Y Resolution:" ) );
   lb_yreso->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_yreso, s_row, 0 );

   ct_yreso      = us_counter( 3, 10.0, 1000.0, 0.0 );
   ct_yreso->setStep( 1 );
   spec->addWidget( ct_yreso, s_row++, 1 );
   connect( ct_yreso,  SIGNAL( valueChanged( double ) ),
            this,      SLOT( update_yreso( double ) ) );

   lb_zfloor     = us_label( tr( "Z Floor Percent:" ) );
   lb_zfloor->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_zfloor, s_row, 0 );

   ct_zfloor     = us_counter( 3, 0.0, 50.0, 1.0 );
   ct_zfloor->setStep( 1 );
   spec->addWidget( ct_zfloor, s_row++, 1 );
   connect( ct_zfloor, SIGNAL( valueChanged( double ) ),
            this,      SLOT( update_zfloor( double ) ) );

   lb_autlim     = us_label( tr( "Automatic Plot Limits" ) );
   lb_autlim->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_autlim, s_row, 0 );

   us_checkbox( tr( "(unselect to override)             " ), cb_autlim, true );
   spec->addWidget( cb_autlim, s_row++, 1 );
   connect( cb_autlim, SIGNAL( clicked() ),
            this,       SLOT( select_autolim() ) );

   lb_plfmin     = us_label( tr( "Plot Limit f/f0 Min:" ) );
   lb_plfmin->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_plfmin, s_row, 0 );

   ct_plfmin     = us_counter( 3, 1.0, 50.0, 0.0 );
   ct_plfmin->setStep( 1 );
   spec->addWidget( ct_plfmin, s_row++, 1 );
   connect( ct_plfmin, SIGNAL( valueChanged( double ) ),
            this,        SLOT( update_plfmin( double ) ) );
   
   lb_plfmax     = us_label( tr( "Plot Limit f/f0 Max:" ) );
   lb_plfmax->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_plfmax, s_row, 0 );
   
   ct_plfmax     = us_counter( 3, 1.0, 50.0, 1.0 );
   ct_plfmax->setStep( 1 );
   ct_plfmax->setValue( 1.34567e+01 );
   spec->addWidget( ct_plfmax, s_row++, 1 );
   connect( ct_plfmax, SIGNAL( valueChanged( double ) ),
         this,         SLOT( update_plfmax( double ) ) );

   lb_plsmin     = us_label( tr( "Plot Limit s Min:" ) );
   lb_plsmin->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_plsmin, s_row, 0 );

   ct_plsmin     = us_counter( 3, -10.0, 10000.0, 0.0 );
   ct_plsmin->setStep( 1 );
   spec->addWidget( ct_plsmin, s_row++, 1 );
   connect( ct_plsmin, SIGNAL( valueChanged( double ) ),
            this,        SLOT( update_plsmin( double ) ) );

   lb_plsmax     = us_label( tr( "Plot Limit s Max:" ) );
   lb_plsmax->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
   spec->addWidget( lb_plsmax, s_row, 0 );

   ct_plsmax     = us_counter( 3, 0.0, 10000.0, 0.0 );
   ct_plsmax->setStep( 1 );
   spec->addWidget( ct_plsmax, s_row++, 1 );
   connect( ct_plsmax, SIGNAL( valueChanged( double ) ),
            this,        SLOT( update_plsmax( double ) ) );

   lw_sbin_data = us_listwidget( );
   spec->addWidget( lw_sbin_data, s_row++, 0, 1, 2 );
   lw_sbin_data->installEventFilter( this );
   connect( lw_sbin_data, SIGNAL( clicked(       const QModelIndex& ) ),
            this,         SLOT(   sclick_sbdata( const QModelIndex& ) ) );
   connect( lw_sbin_data, SIGNAL( doubleClicked( const QModelIndex& ) ),
            this,         SLOT(   dclick_sbdata( const QModelIndex& ) ) );

   us_checkbox( tr( "Plot f/f0 VS s" ), cb_plot_s, true );
   spec->addWidget( cb_plot_s, s_row, 0 );
   connect( cb_plot_s,  SIGNAL( clicked() ),
            this,       SLOT( select_plot_s() ) );

   us_checkbox( tr( "Plot f/f0 VS mw" ), cb_plot_mw, false );
   spec->addWidget( cb_plot_mw, s_row++, 1 );
   connect( cb_plot_mw, SIGNAL( clicked() ),
            this,       SLOT( select_plot_mw() ) );

   pb_lddistr    = us_pushbutton( tr( "Load Distribution" ) );
   pb_lddistr->setEnabled( true );
   spec->addWidget( pb_lddistr, s_row, 0 );
   connect( pb_lddistr, SIGNAL( clicked() ),
            this,       SLOT(   load_distro() ) );

   us_checkbox( tr( "1-Dimensional Plot" ), cb_1dplot, true );
   spec->addWidget( cb_1dplot, s_row++, 1 );
   connect( cb_1dplot,  SIGNAL( clicked() ),
            this,       SLOT(   select_plot1d() ) );

   pb_ldcolor    = us_pushbutton( tr( "Load Color File" ) );
   pb_ldcolor->setEnabled( true );
   spec->addWidget( pb_ldcolor, s_row, 0 );
   connect( pb_ldcolor, SIGNAL( clicked() ),
            this,       SLOT(   load_color() ) );

   us_checkbox( tr( "2-Dimensional Plot" ), cb_2dplot, false );
   spec->addWidget( cb_2dplot, s_row++, 1 );
   connect( cb_2dplot, SIGNAL( clicked() ),
            this,       SLOT(  select_plot2d() ) );

   pb_refresh    = us_pushbutton( tr( "Refresh Plot" ) );
   pb_refresh->setEnabled(  false );
   spec->addWidget( pb_refresh, s_row, 0 );
   connect( pb_refresh, SIGNAL( clicked() ),
            this,       SLOT(   replot_data() ) );

   us_checkbox( tr( "Pseudo 3-D Plot" ),    cb_3dplot, false );
   spec->addWidget( cb_3dplot, s_row++, 1 );
   connect( cb_3dplot, SIGNAL( clicked() ),
            this,       SLOT(  select_plot3d() ) );

   pb_mandrsb    = us_pushbutton( tr( "Manually Draw Bins" ) );
   pb_mandrsb->setEnabled( false );
   spec->addWidget( pb_mandrsb, s_row, 0 );
   connect( pb_mandrsb, SIGNAL( clicked() ),
            this,       SLOT(   mandrawsb() ) );

   pb_shrnksb    = us_pushbutton( tr( "Shrink Solute Bins" ) );
   pb_shrnksb->setEnabled( false );
   spec->addWidget( pb_shrnksb, s_row++, 1 );
   connect( pb_shrnksb, SIGNAL( clicked() ),
            this,       SLOT(   shrinksb() ) );

   pb_autassb    = us_pushbutton( tr( "Autoassign Solute Bins" ) );
   pb_autassb->setEnabled( false );
   spec->addWidget( pb_autassb, s_row, 0 );
   connect( pb_autassb, SIGNAL( clicked() ),
            this,       SLOT(   autassignsb() ) );

   pb_resetsb    = us_pushbutton( tr( "Reset Solute Bins" ) );
   pb_resetsb->setEnabled( false );
   spec->addWidget( pb_resetsb, s_row++, 1 );
   connect( pb_resetsb, SIGNAL( clicked() ),
            this,       SLOT( resetsb() ) );

   pb_reset      = us_pushbutton( tr( "Reset" ) );
   pb_reset->setEnabled( false );
   spec->addWidget( pb_reset, s_row, 0 );
   connect( pb_reset,   SIGNAL( clicked() ),
            this,       SLOT(   reset() ) );

   pb_save       = us_pushbutton( tr( "Save" ) );
   pb_save->setEnabled( false );
   spec->addWidget( pb_save, s_row++, 1 );
   connect( pb_save,    SIGNAL( clicked() ),
            this,       SLOT(   save() ) );

   pb_help       = us_pushbutton( tr( "Help" ) );
   pb_help->setEnabled( true );
   spec->addWidget( pb_help, s_row, 0 );
   connect( pb_help,    SIGNAL( clicked() ),
            this,       SLOT(   help() ) );

   pb_close      = us_pushbutton( tr( "Close" ) );
   pb_close->setEnabled( true );
   spec->addWidget( pb_close, s_row++, 1 );
   connect( pb_close,   SIGNAL( clicked() ),
            this,       SLOT(   close() ) );

   QPalette pa( lb_info1->palette() );
   te_status    = us_textedit( );
   te_status->setPalette( pa );
   te_status->setTextBackgroundColor( pa.color( QPalette::Window ) );
   te_status->setTextColor( pa.color( QPalette::WindowText ) );
   dfilname     = "(NONE)";
   stcmline     = tr( "Color Map:  the default w-cyan-magenta-red-black" );
   stdiline     = tr( "The distribution was loaded from the file:" );
   stdfline     = "  " + dfilname;
   stnpline     = tr( "The number of distribution points is 0." );
   te_status->setText( stcmline + "\n" + stdiline + "\n"
         + stdfline + "\n" + stnpline );
   spec->addWidget( te_status, s_row++, 0, 1, 2 );

   // set up plot component window on right side
   xa_title_s  = tr( "Sedimentation Coefficient corrected for water at 20" )
      + DEGC;
   xa_title_mw = tr( "Molecular Weight (Dalton)" );
   xa_title    = xa_title_s;

   QBoxLayout* plot = new US_Plot( data_plot, 
      tr( "Distribution Data" ),
      xa_title,
      tr( "Frictional Ratio f/f0" ) );

   rght->addLayout( plot );
   QBoxLayout* txed = new QHBoxLayout;
   te_pctl_help  = us_textedit( );
   te_pctl_help->setText( tr(
      "Please load a sedimentation coefficient or molecular weight"
      " distribution to initialize the genetic algorithm s-value or vw-value"
      " range. The distribution should have a good resolution over the"
      " sedimentation or weight coefficients. This distribution will be "
      " used to initialize all experiments used in the run, so the"
      " distribution taken from the experiment with the highest speed is"
      " probably the most appropriate distribution. You can use a distribution"
      " from the van Holde - Weischet method, the C(s) method, or 2-D Spectrum"
      " Analysis. You may also load a Monte Carlo distribution." ) );
   te_pctl_help->setReadOnly( true );
   txed->addWidget( te_pctl_help );
   rght->addLayout( txed );
   rght->setStretchFactor( plot, 4 );
   rght->setStretchFactor( txed, 1 );

   data_plot->setAutoDelete( true );
   data_plot->setMinimumSize( 600, 600 );

   data_plot->enableAxis( QwtPlot::xBottom, true );
   data_plot->enableAxis( QwtPlot::yLeft,   true );
   data_plot->enableAxis( QwtPlot::yRight,  true );
   data_plot->setAxisScale( QwtPlot::xBottom, 1.0, 40.0 );
   data_plot->setAxisScale( QwtPlot::yLeft,   1.0,  4.0 );

   data_plot->setCanvasBackground( Qt::darkBlue );

   // put layouts together for overall layout
   left->addLayout( spec );

   main->addLayout( left );
   main->addLayout( rght );
   main->setStretchFactor( left, 2 );
   main->setStretchFactor( rght, 5 );

   // set up variables and initial state of GUI
   soludata   = new US_SoluteData();
   sdistro    = &s_distro;
   plot_dim   = 2;          // default plot dimension
   plot_s     = true;       // default s/MW X type
   rbtn_click = false;      // default right-button clicked
   def_local  = false;      // default local/DB flag
   def_local  = US_Settings::debug_match( "LoadLocal" ) ? true : def_local;
   investig   = "USER";     // default investigator
   mfilter    = "";         // default model list filter

   reset();
}

// reset the GUI
void US_GA_Initialize::reset( void )
{
   data_plot->detachItems( );
   data_plot->replot();
   pick       = new US_PlotPicker( data_plot );
 
   lw_sbin_data->clear();
   soludata->clearBuckets();
   sxset      = 0;

   minmax     = false;
   zoom       = false;
   cb_1dplot->setChecked(  plot_dim == 1 );  
   cb_2dplot->setChecked(  plot_dim == 2 );
   cb_3dplot->setChecked(  plot_dim == 3 );
   cb_plot_s->setChecked(  plot_s );
   cb_plot_mw->setChecked( !plot_s );

   nisols     = 0;
   nibuks     = 0;
   wsbuck     = 0.0;
   hfbuck     = 0.0;
   ct_nisols->setValue( (double)nisols );
   ct_wsbuck->setRange( 0, 200, 0.1 );
   ct_hfbuck->setRange( 0, 50, 0.01 );
   ct_wsbuck->setValue( wsbuck );
   ct_hfbuck->setValue( hfbuck );

   resolu     = 90.0;
   ct_resolu->setRange( 1, 100, 1 );
   ct_resolu->setValue( resolu );  

   xreso      = 300.0;
   yreso      = 300.0;
   ct_xreso->setRange( 10.0, 1000.0, 1.0 );
   ct_xreso->setValue( (double)xreso );
   ct_yreso->setRange( 10, 1000, 1 );
   ct_yreso->setValue( (double)yreso );

   zfloor     = 5.0;
   ct_zfloor->setRange( 0, 50, 1 );
   ct_zfloor->setValue( (double)zfloor );

   auto_lim   = true;
   cb_autlim->setChecked( auto_lim );

   plfmin     = 1.0;
   plfmax     = 4.0;
   ct_plfmin->setRange( 0, 50, 0.01 );
   ct_plfmin->setValue( plfmin );
   ct_plfmin->setEnabled( false );
   ct_plfmax->setRange( 1, 50, 0.01 );
   ct_plfmax->setValue( plfmax );
   ct_plfmax->setEnabled( false );

   plsmin     = 1.0;
   plsmax     = 10.0;
   ct_plsmin->setRange( -10.0, 10000.0, 0.01 );
   ct_plsmin->setValue( plsmin );
   ct_plsmin->setEnabled( false );
   ct_plsmax->setRange( 0.0, 10000.0, 0.01 );
   ct_plsmax->setValue( plsmax );
   ct_plsmax->setEnabled( false );

   // default to white-cyan-magenta-red-black color map
   colormap   = new QwtLinearColorMap( Qt::white, Qt::black );
   colormap->addColorStop( 0.10, Qt::cyan );
   colormap->addColorStop( 0.50, Qt::magenta );
   colormap->addColorStop( 0.80, Qt::red );
   cmapname   = tr( "Default Color Map: w-cyan-magenta-red-black" );

   monte_carlo = false;
   pb_reset->setEnabled(   false );
   cb_plot_s->setEnabled(  true );
   cb_plot_mw->setEnabled( true );
}

// save the GA data
void US_GA_Initialize::save( void )
{ 
   QString filter = tr( "GA data files (*.gadistro.dat);;" )
      + tr( "Any files (*)" );
   QString fsufx = "." + cell + wavelength + ".gadistro.dat";
   QString fname = run_name + fsufx;
   fname         = QFileDialog::getSaveFileName( this,
      tr( "Save GA Data File" ),
      US_Settings::resultDir() + "/" + fname,
      filter,
      0, 0 );
   soludata->saveGAdata( fname );

   if ( monte_carlo )
   {  // if Monte Carlo, build up and analyze data, then report

      soludata->buildDataMC( plot_s, &s_distro, &w_distro );  // build it

      int jj        = fname.lastIndexOf( fsufx );
      fsufx         = ".ga_stats." + cell + wavelength;
      fname         = ( ( jj > 0 ) ? fname.left( jj ) : fname ) + fsufx;

      soludata->reportDataMC( fname, mc_iters );              // report it
   }
}

// manually draw solute bins
void US_GA_Initialize::mandrawsb( void )
{
   QColor cblack( Qt::black );
   QColor cwhite( Qt::white );

   // create a new plot picker to draw rectangles around solute points
   delete pick;
   pick      = new US_PlotPicker( data_plot );

   // make sure rubber band and tracker show up against background
   QColor bg = data_plot->canvasBackground();
   int csum  = bg.red() + bg.green() + bg.blue();
   pickpen   = new QPen( ( csum > 600 ) ? cblack : cwhite );

   pick->setRubberBandPen( *pickpen );
   pick->setTrackerPen(    *pickpen );
   pick->setRubberBand(     QwtPicker::RectRubberBand );
   pick->setSelectionFlags( QwtPicker::RectSelection
                          | QwtPicker::DragSelection );

   // set up to capture position and dimensions of solute bin
   connect( pick, SIGNAL(  mouseDown( const QwtDoublePoint& ) ),
            this, SLOT( getMouseDown( const QwtDoublePoint& ) ) );
   connect( pick, SIGNAL( mouseUp(    const QwtDoublePoint& ) ),
            this, SLOT( getMouseUp(   const QwtDoublePoint& ) ) );

   //pb_shrnksb->setEnabled( true );
   pb_shrnksb->setEnabled( false );

   wsbuck       = ( plsmax - plsmin ) / 20.0;
   hfbuck       = ( plfmax - plfmin ) / 20.0;
   double rmax  = wsbuck * 10.0;
   double rinc  = pow( 10.0, (double)( (int)( log10( rmax ) - 3.0 ) ) );
   ct_wsbuck->disconnect( );
   ct_wsbuck->setRange( 0.0, rmax, rinc );
   ct_wsbuck->setValue( wsbuck );
   ct_hfbuck->setValue( hfbuck );
   connect( ct_wsbuck, SIGNAL( valueChanged(  double ) ),
            this,      SLOT(   update_wsbuck( double ) ) );
}

// shrink solute bins
void US_GA_Initialize::shrinksb( void )
{
   pb_shrnksb->setEnabled( false );
}

// auto assign solute bins
void US_GA_Initialize::autassignsb( void )
{
   nisols      = ( nisols == 0 ) ? sdistro->size() : nisols;
   pc1         = NULL;
   lw_sbin_data->clear();

   nibuks      = soludata->autoCalcBins( nisols, wsbuck, hfbuck );

   for ( int jj = 0; jj < nibuks; jj++ )
   {  // draw the auto-assigned buckets and add lines to list widget
      QRectF rect = soludata->bucketRect( jj );
      pc1         = drawBucketRect( jj, rect );

      lw_sbin_data->addItem( soludata->bucketLine( jj ) );
   }

   data_plot->replot();
   pb_resetsb->setEnabled( true );
   pb_save->setEnabled(    true );
}

// reset solute bins
void US_GA_Initialize::resetsb( void )
{
   ct_nisols->setValue( 0.0 );
   lw_sbin_data->clear();     // clear solute bucket data
   soludata->clearBuckets();
   sxset    = 0;

   erase_buckets( true );     // erase bucket rectangles from plot and delete

   nibuks   = 0;

   data_plot->replot();
}

// (re)plot data
void US_GA_Initialize::replot_data()
{
   if ( sdistro->isEmpty()  || sdistro->size() == 0 )
      return;

   resetsb();

   if ( plot_dim == 1 )
   {
      plot_1dim();
   }
   else if ( plot_dim == 2 )
   {
      plot_2dim();
   }
   else
   {
      plot_3dim();
   }

   setBucketPens();
}

// plot data 1-D
void US_GA_Initialize::plot_1dim( void )
{
   data_plot->detachItems();

   data_plot->setCanvasBackground( Qt::black );

   sdistro       = plot_s ? &s_distro : &w_distro;

   int     dsize = sdistro->size();
   double* x     = new double[ dsize ];
   double* y     = new double[ dsize ];
   double  sval  = sdistro->at( 0 ).s;
   double  smin  = sval;
   double  smax  = sval;
   double  cval  = sdistro->at( 0 ).c;
   double  cmin  = cval;
   double  cmax  = cval;
   int     nn    = 1;
   x[ 0 ]        = sval;
   y[ 0 ]        = cval;

   for ( int jj = 1; jj < dsize; jj++ )
   {
      double svpr = sval;
      double cvpr = cval;
      sval        = sdistro->at( jj ).s;
      cval        = sdistro->at( jj ).c;

      if ( equivalent( sval, svpr, epsilon ) )
      {  // effectively equal s values: sum c values
         cval       += cvpr;
         x[ nn - 1 ] = ( svpr + sval ) * 0.5;
         y[ nn - 1 ] = cval;
      }

      else
      {  // new s value:  save c value and bump count
         x[ nn   ]   = sval;
         y[ nn++ ]   = cval;
      }

      smin        = ( smin < sval ) ? smin : sval;
      smax        = ( smax > sval ) ? smax : sval;
      cmin        = ( cmin < cval ) ? cmin : cval;
      cmax        = ( cmax > cval ) ? cmax : cval;
   }

   double rdif = ( smax - smin ) / 20.0;
   smin       -= rdif;
   smax       += rdif;
   rdif        = ( cmax - cmin ) / 20.0;
   cmin       -= rdif;
   cmax       += rdif;
   smin        = ( smin > 0.0 ) ? smin : 0.0;
   cmin        = ( cmin > 0.0 ) ? cmin : 0.0;

   QwtPlotGrid* data_grid = us_grid( data_plot );
   data_grid->enableYMin( true );
   data_grid->enableY( true );
   data_grid->setMajPen( QPen( US_GuiSettings::plotMajGrid(),
      0, Qt::DashLine ) );
   data_grid->attach( data_plot );

   QwtPlotCurve *data_curv = us_curve( data_plot, "distro" );
   data_curv->setData( x, y, nn );
   data_curv->setPen( QPen( Qt::yellow, 3, Qt::SolidLine ) );
   data_curv->setStyle( QwtPlotCurve::Sticks );

   delete [] x;
   delete [] y;
   data_plot->setAxisAutoScale( QwtPlot::xBottom );
   data_plot->setAxisAutoScale( QwtPlot::yLeft );
   data_plot->enableAxis( QwtPlot::yRight, false );
   data_plot->setAxisTitle( QwtPlot::xBottom, xa_title );
   data_plot->setAxisTitle( QwtPlot::yLeft,   tr( "Relative Frequency" ) );
   data_plot->setAxisTitle( QwtPlot::yRight,  tr( "Frequency" ) );
   data_plot->axisTitle( QwtPlot::yRight ).setFont(
         data_plot->axisTitle( QwtPlot::yLeft ).font() );
   data_plot->setAxisScale( QwtPlot::xBottom, smin, smax );
   data_plot->setAxisScale( QwtPlot::yLeft,   cmin, cmax );

   data_plot->replot();

   pb_reset->setEnabled( true );
   pb_autassb->setEnabled( false );
}

// plot data 2-D
void US_GA_Initialize::plot_2dim( void )
{
   data_plot->detachItems();

   data_plot->setCanvasBackground( Qt::black );

   sdistro       = plot_s ? &s_distro : &w_distro;

   int     dsize = sdistro->size();
   double* x     = new double[ dsize ];
   double* y     = new double[ dsize ];
   double  smin  = 1.0e30;
   double  smax  = -1.0e30;
   double  fmin  = 1.0e30;
   double  fmax  = -1.0e30;

   for ( int jj = 0; jj < dsize; jj++ )
   {
      double sval = sdistro->at( jj ).s;
      double fval = sdistro->at( jj ).k;
      x[ jj ]     = sval;
      y[ jj ]     = fval;
      smin        = ( smin < sval ) ? smin : sval;
      smax        = ( smax > sval ) ? smax : sval;
      fmin        = ( fmin < fval ) ? fmin : fval;
      fmax        = ( fmax > fval ) ? fmax : fval;
   }

   double rdif = ( smax - smin ) / 20.0;
   smin       -= rdif;
   smax       += rdif;
   rdif        = ( fmax - fmin ) / 20.0;
   fmin       -= rdif;
   fmax       += rdif;
   smin        = ( smin > 0.0 ) ? smin : 0.0;
   fmin        = ( fmin > 0.0 ) ? fmin : 0.0;

   QwtPlotGrid* data_grid = us_grid( data_plot );
   data_grid->enableYMin( true );
   data_grid->enableY( true );
   data_grid->setMajPen( QPen( US_GuiSettings::plotMajGrid(),
      0, Qt::DashLine ) );
   data_grid->attach( data_plot );

   QwtPlotCurve *data_curv = us_curve( data_plot, "distro" );
   QwtSymbol symbol;

   symbol.setStyle( QwtSymbol::Ellipse );
   symbol.setPen( QPen( Qt::red ) );
   symbol.setBrush( QBrush( Qt::yellow ) );
   if ( dsize < 100  &&  dsize > 50 )
      symbol.setSize( 8 );
   else if ( dsize < 50  &&  dsize > 20 )
      symbol.setSize( 10 );
   else if ( dsize < 21 )
      symbol.setSize( 12 );
   else if ( dsize > 100 )
      symbol.setSize( 6 );

   data_curv->setStyle( QwtPlotCurve::NoCurve );
   data_curv->setSymbol( symbol );
   data_curv->setData( x, y, dsize );

   delete [] x;
   delete [] y;
   data_plot->setAxisAutoScale( QwtPlot::xBottom );
   data_plot->setAxisAutoScale( QwtPlot::yLeft );
   data_plot->enableAxis( QwtPlot::yRight, false );
   data_plot->setAxisTitle( QwtPlot::xBottom, xa_title );
   data_plot->setAxisTitle( QwtPlot::yLeft,   tr( "Frictional Ratio f/f0" ) );
   data_plot->setAxisTitle( QwtPlot::yRight,  tr( "Frequency" ) );
   data_plot->axisTitle( QwtPlot::yRight ).setFont(
         data_plot->axisTitle( QwtPlot::yLeft ).font() );
   data_plot->setAxisScale( QwtPlot::xBottom, smin, smax );
   data_plot->setAxisScale( QwtPlot::yLeft,   fmin, fmax );

   data_plot->replot();

   pb_reset->setEnabled( true );
   pb_autassb->setEnabled( !monte_carlo );
}

// plot data 3-D
void US_GA_Initialize::plot_3dim( void )
{
   data_plot->detachItems();

   data_plot->setCanvasBackground( colormap->color1() ); 
   QString tstr = run_name + "." + cell + wavelength + "\n" + method;
   data_plot->setTitle( tstr );

   // set up spectrogram data
   d_spectrogram = new QwtPlotSpectrogram();
   d_spectrogram->setData( US_SpectrogramData() );
   d_spectrogram->setColorMap( *colormap );

   US_SpectrogramData& spec_dat = (US_SpectrogramData&)d_spectrogram->data();

   sdistro       = plot_s ? &s_distro : &w_distro;
   spec_dat.setRastRanges( xreso, yreso, resolu, zfloor );
   spec_dat.setRaster( sdistro );

   d_spectrogram->attach( data_plot );

   // set color map and axis settings
   QwtScaleWidget *rightAxis = data_plot->axisWidget( QwtPlot::yRight );
   rightAxis->setColorBarEnabled( true );
   rightAxis->setColorMap( spec_dat.range(), d_spectrogram->colorMap() );
   data_plot->setAxisTitle( QwtPlot::xBottom, xa_title );
   data_plot->setAxisTitle( QwtPlot::yLeft,   tr( "Frictional Ratio f/f0" ) );
   data_plot->setAxisTitle( QwtPlot::yRight,  tr( "Frequency" ) );
   data_plot->axisTitle( QwtPlot::yRight ).setFont(
         data_plot->axisTitle( QwtPlot::yLeft ).font() );
   data_plot->setAxisScale( QwtPlot::yRight,
      spec_dat.range().minValue(), spec_dat.range().maxValue() );
   data_plot->enableAxis( QwtPlot::yRight );

   if ( auto_lim )
   {   // auto limits
      data_plot->setAxisScale( QwtPlot::yLeft,
         spec_dat.yrange().minValue(), spec_dat.yrange().maxValue() );
      data_plot->setAxisScale( QwtPlot::xBottom,
         spec_dat.xrange().minValue(), spec_dat.xrange().maxValue() );
   }
   else
   {   // manual limits
      data_plot->setAxisScale( QwtPlot::xBottom, plsmin, plsmax );
      data_plot->setAxisScale( QwtPlot::yLeft,   plfmin, plfmax );
   }

   data_plot->replot();

   pb_reset->setEnabled( true );
   pb_autassb->setEnabled( !monte_carlo );
}

// update pseudo-3d resolution factor
void US_GA_Initialize::update_resolu( double dval )
{
   resolu = dval;
}

// update raster x resolution
void US_GA_Initialize::update_xreso( double dval )
{
   xreso  = dval;
}

// update raster y resolution
void US_GA_Initialize::update_yreso( double dval )
{
   yreso  = dval;
}

// update Z (frequency) floor percent
void US_GA_Initialize::update_zfloor( double dval )
{
   zfloor = dval;
}

// update number of initial solutes
void US_GA_Initialize::update_nisols( double dval )
{
   nisols    = qRound( dval );
}

// update width in s of buckets
void US_GA_Initialize::update_wsbuck( double dval )
{
   wsbuck    = dval;
}

// update height in f/f0 of buckets
void US_GA_Initialize::update_hfbuck( double dval )
{
   hfbuck    = dval;
}

// update plot limit s min
void US_GA_Initialize::update_plsmin( double dval )
{
   plsmin    = dval;
}

// update plot limit s max
void US_GA_Initialize::update_plsmax( double dval )
{
   plsmax    = dval;
}

// update plot limit f/f0 min
void US_GA_Initialize::update_plfmin( double dval )
{
   plfmin    = dval;
}

// update plot limit f/f0 max
void US_GA_Initialize::update_plfmax( double dval )
{
   plfmax    = dval;
}

// select automatic plot limits
void US_GA_Initialize::select_autolim()
{
   auto_lim   = cb_autlim->isChecked();
   ct_plfmin->setEnabled( !auto_lim );
   ct_plfmax->setEnabled( !auto_lim );
   ct_plsmin->setEnabled( !auto_lim );
   ct_plsmax->setEnabled( !auto_lim );
   if ( auto_lim )
   {
      set_limits();
   }

   else if ( plot_s )
   {
      ct_plsmin->setRange( -10.0, 10000.0, 0.01 );
      ct_plsmax->setRange( 0.0, 10000.0, 0.01 );
   }

   else
   {
      ct_plsmin->setRange( -10.0, 1.0E+08, 1.0E+05 );
      ct_plsmax->setRange( 0.0, 1.0E+080, 1.0E+05 );
   }

   wsbuck       = ( plsmax - plsmin ) / 20.0;
   hfbuck       = ( plfmax - plfmin ) / 20.0;
   double rmax  = wsbuck * 10.0;
   double rinc  = pow( 10.0, (double)( (int)( log10( rmax ) - 3.0 ) ) );
   ct_wsbuck->disconnect( );
   ct_wsbuck->setRange( 0.0, rmax, rinc );
   ct_wsbuck->setValue( wsbuck );
   ct_hfbuck->setValue( hfbuck );
   connect( ct_wsbuck, SIGNAL( valueChanged(  double ) ),
            this,      SLOT(   update_wsbuck( double ) ) );
}

// select 1-dimensional plot
void US_GA_Initialize::select_plot1d()
{
   plot_dim   = 1;
   cb_2dplot->disconnect();
   cb_3dplot->disconnect();
   cb_2dplot->setChecked(  false );
   cb_3dplot->setChecked(  false );

   cb_1dplot->setEnabled(  false );
   cb_2dplot->setEnabled(  true );
   cb_3dplot->setEnabled(  true );

   connect( cb_2dplot,  SIGNAL( clicked() ),
            this,       SLOT( select_plot2d() ) );
   connect( cb_3dplot,  SIGNAL( clicked() ),
            this,       SLOT( select_plot3d() ) );

   replot_data();

   pb_mandrsb->setEnabled( false );
   pb_autassb->setEnabled( false );
}

// select 2-dimensional plot
void US_GA_Initialize::select_plot2d()
{
   plot_dim   = 2;
   cb_1dplot->disconnect();
   cb_3dplot->disconnect();
   cb_1dplot->setChecked( false );
   cb_3dplot->setChecked( false );

   cb_1dplot->setEnabled( true );
   cb_2dplot->setEnabled( false );
   cb_3dplot->setEnabled( true );

   connect( cb_1dplot,  SIGNAL( clicked() ),
            this,       SLOT( select_plot1d() ) );
   connect( cb_3dplot,  SIGNAL( clicked() ),
            this,       SLOT( select_plot3d() ) );

   replot_data();

   pb_mandrsb->setEnabled( true );
   pb_autassb->setEnabled( !monte_carlo );
}

// select 3-dimensional plot
void US_GA_Initialize::select_plot3d()
{
   plot_dim   = 3;
   cb_1dplot->disconnect();
   cb_2dplot->disconnect();
   cb_3dplot->disconnect();
   cb_1dplot->setChecked( false );
   cb_2dplot->setChecked( false );

   cb_1dplot->setEnabled( true );
   cb_2dplot->setEnabled( true );
   cb_3dplot->setEnabled( false );

   connect( cb_1dplot,  SIGNAL( clicked() ),
            this,       SLOT( select_plot1d() ) );
   connect( cb_2dplot,  SIGNAL( clicked() ),
            this,       SLOT( select_plot2d() ) );
   connect( cb_3dplot,  SIGNAL( clicked() ),
            this,       SLOT( select_plot3d() ) );

   replot_data();

   pb_mandrsb->setEnabled( true );
   pb_autassb->setEnabled( !monte_carlo );
}

// select S (sed. coeff.) for horizontal axis
void US_GA_Initialize::select_plot_s()
{
   plot_s    = cb_plot_s->isChecked();
   cb_plot_mw->setChecked( !plot_s );
   xa_title  = plot_s ? xa_title_s : xa_title_mw;
   data_plot->setAxisTitle( QwtPlot::xBottom, xa_title );

   set_limits();

   replot_data();

   lb_wsbuck->setText( tr( "Width of s Bucket:" ) );
   lb_plsmin->setText( tr( "Plot Limit s Min:" ) );
   lb_plsmax->setText( tr( "Plot Limit s Max:" ) );

   wsbuck       = ( plsmax - plsmin ) / 20.0;
   hfbuck       = ( plfmax - plfmin ) / 20.0;
   double rmax  = wsbuck * 10.0;
   double rinc  = pow( 10.0, (double)( (int)( log10( rmax ) - 3.0 ) ) );
   ct_wsbuck->disconnect();
   ct_wsbuck->setRange( 0.0, rmax, rinc );
   ct_wsbuck->setValue( wsbuck );
   ct_hfbuck->setValue( hfbuck );
   connect( ct_wsbuck, SIGNAL( valueChanged(  double ) ),
            this,      SLOT(   update_wsbuck( double ) ) );
}

// select MW (mol.wt.) for horizontal axis
void US_GA_Initialize::select_plot_mw()
{
   plot_s    = !cb_plot_mw->isChecked();
   cb_plot_s->setChecked( plot_s );
   xa_title  = plot_s ? xa_title_s : xa_title_mw;
   data_plot->setAxisTitle( QwtPlot::xBottom, xa_title );

   set_limits();

   replot_data();

   lb_wsbuck->setText( tr( "Width of mw Bucket:" ) );
   lb_plsmin->setText( tr( "Plot Limit mw Min:" ) );
   lb_plsmax->setText( tr( "Plot Limit mw Max:" ) );

   wsbuck       = ( plsmax - plsmin ) / 20.0;
   hfbuck       = ( plfmax - plfmin ) / 20.0;
   double rmax  = wsbuck * 10.0;
   double rinc  = pow( 10.0, (double)( (int)( log10( rmax ) - 3.0 ) ) );
   ct_wsbuck->disconnect();
   ct_wsbuck->setRange( 0.0, rmax, rinc );
   ct_wsbuck->setValue( wsbuck );
   ct_hfbuck->setValue( hfbuck );
   connect( ct_wsbuck, SIGNAL( valueChanged(  double ) ),
            this,      SLOT(   update_wsbuck( double ) ) );
}

// load the solute distribution from a file
void US_GA_Initialize::load_distro()
{
   Solute      sol_s;
   Solute      sol_w;

   // model type table:  DescPartialName, Method, MonteCarlo
   const char* cdtyp[] =
   {
      "cofs_dis",        "C(s)",                               "F",
      "fe_dis",          "FE",                                 "F",
      "sa2d_dis",        "2DSA",                               "F",
      "ga_mc_dis",       "GA-MC",                              "T",
      "sa2d_mc_dis",     "2DSA-MC",                            "T",
      "ga_dis",          "GA",                                 "F",
      "global_dis",      "Global",                             "F",
      "sa2d_mw_dis",     "2DSA, MW Constrained",               "F",
      "ga_mw_dis",       "GA, MW Constrained",                 "F",
      "sa2d_mw_mc_dis",  "2DSA, MW Constrained, Monte Carlo",  "T",
      "ga_mw_mc_dis",    "GA, MW Constrained, Monte Carlo",    "T",
      "global_dis",      "Global Distro",                      "T",
      "global_mc_dis",   "Global MC Distro",                   "T",
      "model",           "2DSA",                               "T"
   };
   int ncdte = sizeof( cdtyp ) / sizeof( char* );

   US_Model model;

   US_ModelLoader* dialog = new US_ModelLoader( false, def_local,
      mfilter, investig );
   dialog->move( this->pos() + QPoint( 200, 200 ) );

   QString         mdesc;
   QString         mfnam;
   QString         sep;

   if ( dialog->exec() == QDialog::Accepted )
   {
      dialog->load_model( model, 0 );

      mdesc     = dialog->description( 0 );
      mfilter   = dialog->search_filter();
      investig  = dialog->investigator_text();
//qDebug() << "LOAD ACCEPT  Description:\n " << mdesc;
      sep       = mdesc.left( 1 );
      mfnam     = mdesc.section( sep, 2, 2 );

      if ( mfnam.isEmpty() )
      {  // from db:  make ID the "filename"; default to db next time
         mfnam     = "db ID " + mdesc.section( sep, 4, 4 );
         def_local = false;
      }

      else
      {  // from disk:  use base file name; default to disk next time
         mfnam     = QFileInfo( mfnam ).baseName();
         def_local = true;
      }

      delete dialog;

      if ( model.components.size() < 1 )
      {
qDebug() << "  NO Model components";
         return;
      }
   }

   else
   {
      delete dialog;
      return;
   }

//*DEBUG*
#if 0
   double i_s    = model.components[0].s;
   double i_D    = model.components[0].D;
   double i_mw   = model.components[0].mw;
   double i_f    = model.components[0].f;
   double i_ff0  = model.components[0].f_f0;
   double i_c    = model.components[0].signal_concentration;
   qDebug() << QString().sprintf(
      "    INPUT     s/D/mw/f/ff0/c %13.4e%13.4e%13.4e%13.4e%13.4e%13.4e"
      ,i_s,i_D,i_mw,i_f,i_ff0,i_c);
   for ( int ii = 0; ii < 10; ii++ )
   {
      double c_s    = i_s;
      double c_D    = i_D;
      double c_mw   = i_mw;
      double c_f    = i_f;
      double c_ff0  = i_ff0;
      QString  oper="";
      switch ( ii )
      {
         case 0:
            oper="s+D   ";
            c_mw = c_ff0 = 0.0;
            break;
         case 1:
            oper="s+mw  ";
            c_D = c_ff0 = 0.0;
            break;
         case 2:
            oper="s+ff0 ";
            c_mw = c_D = 0.0;
            break;
         case 3:
            oper="s+f   ";
            c_mw = c_D = c_ff0 = 0.0;
            break;
         case 4:
            oper="mw+D  ";
            c_s = c_ff0 = 0.0;
            break;
         case 5:
            oper="mw+ff0";
            c_s = c_D = 0.0;
            break;
         case 6:
            oper="mw+f  ";
            c_s = c_D = c_ff0 = 0.0;
            break;
         case 7:
            oper="D+ff0 ";
            c_s = c_mw = 0.0;
            break;
         case 8:
            oper="D+f   ";
            c_s = c_mw = c_ff0 = 0.0;
            break;
         case 9:
         default:
            oper="f+ff0 ";
            c_s = c_mw = c_D = 0.0;
            break;
      }
      model.components[0].s    = c_s;
      model.components[0].D    = c_D;
      model.components[0].mw   = c_mw;
      model.components[0].f    = c_f;
      model.components[0].f_f0 = c_ff0;

      bool uok = model.update_coefficients();

      qDebug() << oper << " OUTp s/D/mw/f/ff0/c"
         << QString().sprintf("%13.4e%13.4e%13.4e%13.4e%13.4e%13.4e",
         model.components[ 0 ].s,model.components[ 0 ].D,model.components[ 0 ].mw,
         model.components[ 0 ].f,model.components[ 0 ].f_f0,
         model.components[ 0 ].signal_concentration) << uok;
      if ( ii == 0 )
      {
         US_Math2::SolutionData d;
         d.vbar20 = model.components[0].vbar20;
         //d.vbar = model.components[0].vbar20;
         d.density = DENS_20W;
         d.viscosity = VISC_20W;
         US_Math2::data_correction( 20.0, d );

         //qDebug() << "fIN fOUT  ratioOI" << i_f << model.components[0].f << " " << model.components[0].f/i_f;
         //qDebug() << " V20W vTB  bB bW" << VISC_20W << d.viscosity_tb << " " << d.buoyancyb << d.buoyancyw;
      }
   }
   model.components[0].s    = i_s;
   model.components[0].D    = i_D;
   model.components[0].mw   = i_mw;
   model.components[0].f    = i_f;
   model.components[0].f_f0 = i_ff0;
   model.components[0].signal_concentration = i_c;
#endif
//*DEBUG*
   // insure all model coefficient properties are set
   if ( ! model.update_coefficients() )
   {
      qDebug() << "*** Unable to recalculate coefficient values ***";
   }

   // parse model information from its description
   mdesc        = mdesc.section( sep, 1, 1 );

   run_name     = mdesc.section( ".", 0, 0 );
   int jj       = mdesc.lastIndexOf( "." );
   int kk       = mdesc.length();

   if ( jj < 0 )
   {  // for model not really a distribution, fake it
      jj           = kk;
      mdesc        = mdesc + ".model.11";
      kk           = mdesc.length();
   }

   QString tstr = mdesc.right( kk - jj - 1 );

   cell         = tstr.left( 1 );
   tstr         = mdesc.right( kk - jj - 2 );
   wavelength   = tstr;
   distro_type  = 0;

   // find type in table and set values accordingly
   for ( jj = 0; jj < ncdte; jj += 3 )
   {
      QString fnp( cdtyp[ jj ] );

      if ( mdesc.contains( fnp, Qt::CaseInsensitive ) )
      {
         distro_type = jj / 3 + 1;
         monte_carlo = QString( cdtyp[ jj+2 ] ).contains( "T" );
         method      = QString( cdtyp[ jj+1 ] );
         break;
      }
   }
   s_distro.clear();
   w_distro.clear();

   // pick up number of monte carlo iterations in case needed
   //mc_iters  = ( model.iterations > 1 ) ? model.iterations : 1;
   mc_iters  = 1;

   tstr      = run_name + "." + cell + wavelength + "\n" + method;
   data_plot->setTitle( tstr );

   // read in and set distribution s,c,k,d values
   if ( distro_type > 0 )
   {
      for ( int jj = 0; jj < model.components.size(); jj++ )
      {
         sol_s.s  = model.components[ jj ].s * 1.0e13;
         sol_s.c  = model.components[ jj ].signal_concentration;
         sol_s.k  = model.components[ jj ].f_f0;
         sol_s.d  = model.components[ jj ].D;
         sol_w.s  = model.components[ jj ].mw;
         sol_w.c  = sol_s.c;
         sol_w.k  = sol_s.k;
         sol_w.d  = sol_s.d;
//qDebug() << "Solute jj s c k d" << jj << sol_s.s << sol_s.c << sol_s.k << sol_s.d;

         s_distro.append( sol_s );
         w_distro.append( sol_w );
      }

      // sort and reduce distributions
      psdsiz    = s_distro.size();
      sort_distro( s_distro, true );
      sort_distro( w_distro, true );
   }

   if ( auto_lim )
   {
      set_limits();

      ct_plfmin->setEnabled( false );
      ct_plfmax->setEnabled( false );
      ct_plsmin->setEnabled( false );
      ct_plsmax->setEnabled( false );
   }
   else
   {
      plsmin    = ct_plsmin->value();
      plsmax    = ct_plsmax->value();
      plfmin    = ct_plfmin->value();
      plfmax    = ct_plfmax->value();
      if ( plot_s )
      {
         ct_plsmin->setRange( -10.0, 10000.0, 0.01 );
         ct_plsmax->setRange( 0.0, 10000.0, 0.01 );
      }
      else
      {
         ct_plsmin->setRange( -10.0, 1.0E+08, 1.0E+05 );
         ct_plsmax->setRange( 0.0, 1.0E+080, 1.0E+05 );
      }
   }
   data_plot->setAxisScale( QwtPlot::xBottom, plsmin, plsmax );
   data_plot->setAxisScale( QwtPlot::yLeft,   plfmin, plfmax );

   pb_resetsb->setEnabled( true );

   kk           = s_distro.size();
   dfilname     = "(" + mfnam + ") " + mdesc;
   stdfline     = "  " + dfilname;
   stnpline     = tr( "The number of distribution points is %1" ).arg( kk );

   if ( kk != psdsiz )
      stnpline    += tr( "\n  (reduced from %1)" ).arg( psdsiz );

   te_status->setText( stcmline + "\n" + stdiline + "\n"
         + stdfline + "\n" + stnpline );

   replot_data();

   soludata->setDistro( sdistro );

   nisols       = kk;
   nibuks       = 0;
   wsbuck       = ( plsmax - plsmin ) / 20.0;
   hfbuck       = ( plfmax - plfmin ) / 20.0;
   ct_wsbuck->setValue( wsbuck );
   ct_hfbuck->setValue( hfbuck );
   ct_nisols->setValue( double( nisols ) );
   ct_wsbuck->setEnabled(  true );
   ct_hfbuck->setEnabled(  true );
   pb_refresh->setEnabled( true );
   pb_mandrsb->setEnabled( plot_dim != 1 );
}

// load the color map from a file
void US_GA_Initialize::load_color()
{
   QString filter = tr( "Color Map files (cm*.xml);;" )
         + tr( "Any XML files (*.xml);;" )
         + tr( "Any files (*)" );

   // get an xml file name for the color map
   QString fname = QFileDialog::getOpenFileName( this,
      tr( "Load Color Map File" ),
      US_Settings::appBaseDir() + "/etc",
      filter,
      0, 0 );

   if ( fname.isEmpty() )
      return;

   // get the map from the file
   QList< QColor > cmcolor;
   QList< double > cmvalue;

   US_ColorGradIO::read_color_steps( fname, cmcolor, cmvalue );
   colormap  = new QwtLinearColorMap( cmcolor.first(), cmcolor.last() );

   for ( int jj = 1; jj < cmvalue.size() - 1; jj++ )
   {
      colormap->addColorStop( cmvalue.at( jj ), cmcolor.at( jj ) );
   }
   QFileInfo fi( fname );
   cmapname  = tr( "Color Map: " ) + fi.baseName();

   stcmline  = cmapname;
   te_status->setText( stcmline + "\n" + stdiline + "\n"
         + stdfline + "\n" + stnpline );
}

// set plot x,y limits
void US_GA_Initialize::set_limits()
{
   double fmin = 1.0e30;
   double fmax = -1.0e30;
   double smin = 1.0e30;
   double smax = -1.0e30;
   double rdif;

   resetsb();

   if ( plot_s )
   {
      sdistro     = &s_distro;
      xa_title    = xa_title_s;
   }

   else
   {
      sdistro     = &w_distro;
      xa_title    = xa_title_mw;
   }

   soludata->setDistro( sdistro );

   data_plot->setAxisTitle( QwtPlot::xBottom, xa_title );

   // find min,max for S distributions
   for ( int jj = 0; jj < sdistro->size(); jj++ )
   {
      double sval = sdistro->at( jj ).s;
      double fval = sdistro->at( jj ).k;
      smin        = ( smin < sval ) ? smin : sval;
      smax        = ( smax > sval ) ? smax : sval;
      fmin        = ( fmin < fval ) ? fmin : fval;
      fmax        = ( fmax > fval ) ? fmax : fval;
   }

   // adjust minima, maxima
   rdif      = ( smax - smin ) / 10.0;
   smin     -= rdif;
   smax     += rdif;
   smin      = ( smin < 0.0 ) ? 0.0 : smin;
   rdif      = ( fmax - fmin ) / 10.0;
   fmin     -= rdif;
   fmax     += rdif;

   if ( sdistro->size() > 0 )
   {
      double rmax  = smax * 10.0;
      double rinc  = pow( 10.0, (double)( (int)( log10( rmax ) - 3.0 ) ) );

      ct_plsmax->setRange( 0.0, rmax, rinc );
      ct_plsmin->setRange( -( smax / 50.0 ), rmax, rinc );
   }

   if ( auto_lim )
   {
      // set auto limits
      smax       += ( ( smax - smin ) / 20.0 );
      smin       -= ( ( smax - smin ) / 20.0 );
      fmax       += ( ( fmax - fmin ) / 20.0 );
      fmin       -= ( ( fmax - fmin ) / 20.0 );
      fmin        = ( fmin < 0.0 ) ? 0.0 : fmin;

      if ( ( fmax - fmin ) < 1.0e-3 )
         fmax       += ( fmax / 10.0 );

      if ( ( smax - smin ) < 1.0e-100 )
      {
         smin       -= ( smin / 30.0 );
         smax       += ( smax / 30.0 );
      }

      ct_plsmin->setValue( smin );
      ct_plsmax->setValue( smax );
      ct_plfmin->setValue( fmin );
      ct_plfmax->setValue( fmax );

      plsmin    = smin;
      plsmax    = smax;
      plfmin    = fmin;
      plfmax    = fmax;
   }
   else
   {
      plsmin    = ct_plsmin->value();
      plsmax    = ct_plsmax->value();
      plfmin    = ct_plfmin->value();
      plfmax    = ct_plfmax->value();
   }
   te_pctl_help->setText( tr(
      "Now either auto-assign the solute bins, or manually select bins"
      " by clicking on a bin vertex, then moving and releasing on the"
      " other vertex. If you auto-assign the bins you should first"
      " select the number of solute bins you want to use. UltraScan will"
      " space the bins proportional to the integral value of each peak,"
      " such that each bin contains the same integral value."
      " You can select each solute bin from the listbox on the left and"
      " modify its size by first changing the bucket dimensions with the"
      " respective counters, then double-click on the listbox item."
      " You may remove a bin by right-mouse-button clicking on the listbox"
      " item and responding/defaulting Yes in the resulting dialog." ) );
}

// Sort distribution solute list by s,k values and optionally reduce
void US_GA_Initialize::sort_distro( QList< Solute >& listsols,
      bool reduce )
{
   int sizi = listsols.size();

   if ( sizi < 2 )
      return;        // nothing need be done for 1-element list

   // sort distro solute list by s,k values

   qSort( listsols.begin(), listsols.end(), distro_lessthan );

   // check reduce flag

   if ( reduce )
   {     // skip any duplicates in sorted list
      Solute sol1;
      Solute sol2;
      QList< Solute > reduced;
      QList< Solute >::iterator jj = listsols.begin();
      sol1     = *jj;
      reduced.append( *jj );     // output first entry

      while ( (++jj) != listsols.end() )
      {     // loop to compare each entry to previous
          sol2    = *jj;         // solute entry

          if ( !equivalent( sol1.s, sol2.s, epsilon )  ||
               !equivalent( sol1.k, sol2.k, epsilon ) )
          {   // not a duplicate, so output to temporary list
             reduced.append( sol2 );
          }

          else
          {   // duplicate:  sum c value
             sol2.c += sol1.c;   // sum c value
             sol2.s  = ( sol1.s + sol2.s ) * 0.5;  // average s,k
             sol2.k  = ( sol1.k + sol2.k ) * 0.5;
             reduced.replace( reduced.size()-1, sol2 );
          }

          sol1    = sol2;        // save entry for next iteration
      }

      if ( reduced.size() < sizi )
      {   // if some reduction happened, replace list with reduced version
         listsols = reduced;
      }
   }
   return;
}

// set bucket pens for previous and current bin
void US_GA_Initialize::setBucketPens()
{
   QPen penCR( QColor( Qt::red ),   1, Qt::SolidLine );
   QPen penCY( QColor( Qt::yellow), 1, Qt::SolidLine );
   QPen penCB( QColor( Qt::blue),   1, Qt::SolidLine );

   // current is always red
   cbukpen   = penCR;

   // previous is blue for light background, yellow for dark
   QColor bg = data_plot->canvasBackground();
   int csum  = bg.red() + bg.green() + bg.blue();
   pbukpen   = ( csum > 600 ) ? penCB : penCY;

   return;
}

// highlight solute bin rectangle in red; previous in yellow or blue
void US_GA_Initialize::highlight_solute( QwtPlotCurve* bc1 )
{
   if ( bc1 == NULL )
      return;

   if ( nibuks > 0  &&  pc1 != NULL )
   {  // re-color previous bucket yellow or blue
      pc1->setPen( pbukpen );
   }

   // current bucket borders drawn in red
   bc1->setPen( cbukpen );

   pc1       = bc1;         // save previous bucket curve
   return;
}

// find bucket curve by solute index, then highlight
void US_GA_Initialize::highlight_solute( int sx )
{
   highlight_solute( bucketCurveAt( sx ) );
   return;
}

// mouse down:  save of first point
void US_GA_Initialize::getMouseDown( const QwtDoublePoint& p )
{
   p1     = p;              // save the first rubberband point
}

// mouse up:  draw bucket rectangle
void US_GA_Initialize::getMouseUp( const QwtDoublePoint& p )
{
   double        tx[2];
   double        ty[2];
   QwtPlotCurve* bc1;

   p2     = p;              // save the second rubberband point

   // draw the bucket rectangle
   bc1    = drawBucketRect( nibuks, p1, p2 );

   // highlight it (and turn off highlight for previous)
   highlight_solute( bc1 );

   data_plot->replot();

   // construct and save a bucket entry
   tx[0]  = p1.x();         // upper,left and lower,right points
   ty[0]  = p1.y();
   tx[1]  = p2.x();
   ty[1]  = p2.y();

   if ( tx[0] > tx[1] )     // insure properly ordered
   {
      tx[0]  = p2.x();
      tx[1]  = p1.x();
   }

   if ( ty[0] > ty[1] )
   {
      ty[0]  = p2.y();
      ty[1]  = p1.y();
   }

   // create bucket rectangle, solute point, and concentration value
   QRectF bucr( QPointF( tx[0], ty[1] ), QPointF( tx[1], ty[0] ) );
   QPointF tpt( ( tx[0] + tx[1] ) / 2.0, ( ty[0] + ty[1] ) / 2.0 );
   QPointF& bucp = tpt;
   int sx        = soludata->findNearestPoint( bucp );

   if ( sx >= 0 )
   {  // for solute point nearest to rectangle midpoint
      Solute sol  = sdistro->at( sx );
      qreal bucc  = sol.c;  // get concentrate value

      // add the bucket entry and add a text box entry
      soludata->appendBucket( bucr, bucp, bucc, 2 );

      QString txt = soludata->bucketLine( -3 );
      lw_sbin_data->addItem( txt );
   }

   // bump solute bins count
   nibuks++;
   ct_nisols->setValue( (double)nibuks );
   if ( nibuks > 2 )
      pb_save->setEnabled(    true );
}

// draw a bucket rectangle by index and top-left,bottom-right points
QwtPlotCurve* US_GA_Initialize::drawBucketRect( int sx,
      QPointF pt1, QPointF pt2 )
{
   double        tx[5];
   double        ty[5];
   QwtPlotCurve* bc1;

   tx[0]  = pt1.x();        // set 5 points needed to draw rectangle
   ty[0]  = pt1.y();
   tx[1]  = pt2.x();
   ty[1]  = pt1.y();
   tx[2]  = pt2.x();
   ty[2]  = pt2.y();
   tx[3]  = pt1.x();
   ty[3]  = pt2.y();
   tx[4]  = pt1.x();
   ty[4]  = pt1.y();

   // create the bucket rectangle curve
   bc1    = us_curve( data_plot, QString( "bucket border %1" ).arg( sx ) );
   bc1->setPen(   pbukpen );
   bc1->setStyle( QwtPlotCurve::Lines );
   bc1->setData(  tx, ty, 5 );

   return bc1;
}

// draw a bucket rectangle by index and rectangle
QwtPlotCurve* US_GA_Initialize::drawBucketRect( int sx, QRectF rect )
{
   return drawBucketRect( sx, rect.topLeft(), rect.bottomRight() );
}

// solute bin list row clicked: highlight bucket
void US_GA_Initialize::sclick_sbdata( const QModelIndex& mx )
{
   int sx      = mx.row();

   highlight_solute( sx );
   data_plot->replot();

   if ( rbtn_click )
   {
      int binx    = sx + 1;
      QMessageBox msgBox;
      QString msg = tr( "Are you sure you want to delete solute bin %1 ?" )
         .arg( binx );
      msgBox.setWindowTitle( tr( "GA_Init Solute Bin Delete" ) );
      msgBox.setText( msg );
      msgBox.setStandardButtons( QMessageBox::No | QMessageBox::Yes );
      msgBox.setDefaultButton( QMessageBox::Yes );
      if ( msgBox.exec() == QMessageBox::Yes )
      {
         removeSoluteBin( sx );
      }
   }

   else if ( monte_carlo  &&  sx != sxset )
   {
      QRectF rect  = soludata->bucketRect( sx );
      ct_wsbuck->setValue( rect.width() );
      ct_hfbuck->setValue( rect.height() );
   }
   sxset        = sx;
   rbtn_click   = false;

}

// solute bin list row double-clicked:  change bucket values
void US_GA_Initialize::dclick_sbdata( const QModelIndex& mx )
{
   int sx      = mx.row();
   QwtPlotCurve* bc1;
   QPointF pt0;

   if ( !monte_carlo )
   {
      pt0         = soludata->bucketPoint( sx, false );
   }
   else
   {
      QRectF rect = soludata->bucketRect( sx );
      QPointF ptl = rect.topLeft();
      QPointF pbr = rect.bottomRight();
      pt0         = QPointF( ( ptl.x() + pbr.x() ) / 2.0, 
                             ( ptl.y() + pbr.y() ) / 2.0 );
   }
   sxset       = sx;
   qreal x1    = pt0.x() - wsbuck / 2.0;
   qreal y1    = pt0.y() + hfbuck / 2.0;
   qreal x2    = pt0.x() + wsbuck / 2.0;
   qreal y2    = pt0.y() - hfbuck / 2.0;
   QPointF pt1( x1, y1 );
   QPointF pt2( x2, y2 );
   QRectF  brect( pt1, pt2 );

   pc1->detach();                  // erase old rectangle for this bucket

   changeBucketRect( sx, brect );  // change bucket rectangle

   bc1         = drawBucketRect( sx, pt1, pt2 );  // draw a new rectangle

   pc1         = bc1;              // save previous bucket curve
   rbtn_click  = false;

   data_plot->replot();

   return;
}

// change the rectangle (vertices) for a bucket
void US_GA_Initialize::changeBucketRect( int sx, QRectF& rect )
{
   bucket abuck  = soludata->bucketAt( sx );

   QPointF bpnt  = soludata->bucketPoint( sx, true );
   qreal bconc   = soludata->bucketAt( sx ).conc;
   int bstat     = 0;

   soludata->setBucket( sx, rect, bpnt, bconc, bstat );

   QString line  = soludata->bucketLine( sx );
   lw_sbin_data->item( sx )->setText( line );

   return;
}

// find the plot curve in the list of curves
QwtPlotCurve* US_GA_Initialize::bucketCurveAt( int sx )
{
   // get title of desired bucket curve and list of all items
   QString ctext         = QString( "bucket border %1" ).arg( sx );
   QwtPlotItemList ilist = data_plot->itemList();

   for ( int jj = 0; jj < ilist.size(); jj++ )
   {  // test each item for Curve type and matching title
      QwtPlotCurve* bc1 = (QwtPlotCurve*)ilist.at( jj );

      if ( bc1->rtti() == QwtPlotItem::Rtti_PlotCurve )
      {  // right type,  so check title
         QString itext = bc1->title().text();

         if ( itext.compare( ctext ) == 0 )
         {  // this is the one we want, return a pointer to the curve
            return bc1;
         }
      }
   }

   return (QwtPlotCurve*)NULL;
}

// erase all bucket curves (option to completely delete )
void US_GA_Initialize::erase_buckets( bool delflag )
{

   for ( int jj = 0; jj < nibuks; jj++ )
   {
      QwtPlotCurve* bc1 = bucketCurveAt( jj );
      if ( bc1 != NULL )
      {
         bc1->detach();

         if ( delflag )
            delete bc1;
      }
   }
   nibuks   = 0;

   data_plot->replot();

   return;
}
// erase all bucket curves (from plot only)
void US_GA_Initialize::erase_buckets( )
{
   erase_buckets( false );
   return;
}

// filter events to catch right-mouse-button-click on list widget
bool US_GA_Initialize::eventFilter( QObject *obj, QEvent *e )
{
   if ( obj == lw_sbin_data  &&
        e->type() == QEvent::ContextMenu )
   {
      rbtn_click = true;
      return false;
   }
   else
   {
      return US_Widgets::eventFilter( obj, e );
   }
}

// remove a solute bin from data, plot, and list
void US_GA_Initialize::removeSoluteBin( int sx )
{
   QList< QString > lines;
   QList< QwtPlotCurve* > curves;
   QwtPlotCurve* bc1;

   int bsize = soludata->bucketsCount();

   // remove the solute bin
   soludata->removeBucketAt( sx );

   // create a new set of lines for the List Widget and new list of curves

   for ( int jj = 0; jj < bsize; jj++ )
   {
      pc1    = bucketCurveAt( jj );
      if ( jj < sx )
      {  // before removed item, just save the old line and curve
         lines.append( lw_sbin_data->item( jj )->text() );
         curves.append( pc1 );
      }

      else if ( jj > sx )
      {  // after removed item, compose new line,title from bucket data
         int kk = jj - 1;
         lines.append( soludata->bucketLine( kk ) );
         pc1->setTitle( QString( "bucket border %1" ).arg( kk ) );
         curves.append( pc1 );
      }

      else
      {  // completely remove the curve itself
         delete pc1;
         pc1    = bucketCurveAt( ( jj > 0 ) ? (jj-1) : 0 );
      }
   }

   // replace the List Widget contents and redraw bin rectangles
   lw_sbin_data->clear();
   erase_buckets();
   nibuks    = bsize - 1;
   ct_nisols->setValue( (double)nibuks );

   for ( int jj = 0; jj < nibuks; jj++ )
   {
      // add the Solute Bin line back in the List Widget
      lw_sbin_data->addItem( lines.at( jj ) );

      // redraw the bin rectangle
      bc1    = curves.at( jj );
      bc1->attach( data_plot );
      highlight_solute( jj );
      pc1    = bc1;
   }

   if ( nibuks > 0 )
   {  // highlight the next bucket if there is one
      highlight_solute( ( sx < nibuks ) ? sx : ( nibuks - 1) );
   }

   data_plot->replot();

   return;
}

// flag whether two values are effectively equal within a given epsilon
bool US_GA_Initialize::equivalent( double a, double b, double eps )
{
   return ( qAbs( ( a - b ) / a ) <= eps );
}

