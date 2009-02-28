//! \file us_astfem_sim.cpp

#include <QApplication>

#include "us_license.h"
#include "us_license_t.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_model_editor.h"
#include "us_model_selection.h"
#include "us_astfem_sim.h"
#include "us_simulationparameters.h"
#include "us_math.h"
#include "us_defines.h"
#include "us_clipdata.h"

/*! \brief Main program for US_Astfem_Sim.  Loads translators and starts
    the class US_Astfem_Sim.
*/
int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );
  
   #include "main1.inc"

   // License is OK.  Start up.
   US_Astfem_Sim w;
   w.show();                   //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

US_Astfem_Sim::US_Astfem_Sim( QWidget* p, Qt::WindowFlags f ) 
   : US_Widgets( p, f )
{
   setWindowTitle( "ASTFEM Simulation Module" );
   setPalette( US_GuiSettings::frameColor() );
   init_simparams();
   
   stopFlag            = false;
   movieFlag           = true;
   time_correctionFlag = true;

   astfem_rsa = new US_Astfem_RSA( system, simparams );

   connect( astfem_rsa, SIGNAL( new_scan         ( QList< double >&, double* ) ), 
            this      , SLOT(   update_movie_plot( QList< double >&, double* ) ) );
   
   connect( astfem_rsa, SIGNAL( current_component( int ) ), 
            this      , SLOT  ( update_progress  ( int ) ) );

   connect( astfem_rsa, SIGNAL( new_time   ( float ) ), 
            this      , SLOT  ( update_time( float ) ) );
   
   connect( astfem_rsa, SIGNAL( current_speed( unsigned int ) ), 
            this      , SLOT  ( update_speed ( unsigned int ) ) );

   connect( astfem_rsa, SIGNAL( calc_start( unsigned int ) ), 
            this      , SLOT  ( start_calc( unsigned int ) ) );

   connect( astfem_rsa, SIGNAL( calc_progress( unsigned int ) ), 
            this      , SLOT  ( show_progress( unsigned int ) ) );

   connect( astfem_rsa, SIGNAL( calc_done( void ) ), 
            this      , SLOT  ( calc_over( void ) ) );

   QGridLayout* main = new QGridLayout( this );
   main->setSpacing( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );
   main->setColumnStretch( 1, 1 );
   
   QBoxLayout* buttonbox = new QVBoxLayout;

   QPushButton* pb_loadExperiment = us_pushbutton( tr( "Load Experiment") );
   connect ( pb_loadExperiment, SIGNAL( clicked() ), SLOT( load_experiment() ) );
   buttonbox->addWidget( pb_loadExperiment );

   pb_saveExp = us_pushbutton( tr( "Save Experiment"), false );
   buttonbox->addWidget( pb_saveExp );

   QPushButton* pb_new = us_pushbutton( tr( "New Model") );
   connect( pb_new, SIGNAL( clicked() ), SLOT( new_model() ) );
   buttonbox->addWidget( pb_new );
   
   QPushButton* pb_loadModel = us_pushbutton( tr( "Load Model") );
   connect( pb_loadModel, SIGNAL( clicked() ), SLOT( load_model() ) );
   buttonbox->addWidget( pb_loadModel );

   pb_changeModel = us_pushbutton( tr( "Change/Review Model"), false );
   connect ( pb_changeModel, SIGNAL( clicked() ) , SLOT( change_model() ) );
   buttonbox->addWidget( pb_changeModel );
   
   pb_simParms = us_pushbutton( tr( "Simulation Parameters"), false );
   connect ( pb_simParms, SIGNAL( clicked() ) , SLOT( sim_parameters() ) );
   buttonbox->addWidget( pb_simParms );

   cb_movie = us_checkbox( "Show Movie", movieFlag );
   buttonbox->addWidget( cb_movie );

   cb_timeCorr = us_checkbox( "Use Time Correction", time_correctionFlag );
   connect( cb_timeCorr, SIGNAL( clicked() ), SLOT( update_time_corr() ) );
   buttonbox->addWidget( cb_timeCorr );

   pb_start = us_pushbutton( tr( "Start Simulation"), false );
   connect( pb_start, SIGNAL( clicked() ), SLOT( start_simulation() ) );
   buttonbox->addWidget( pb_start );

   pb_stop = us_pushbutton( tr( "Stop Simulation"), false );
   connect( pb_stop, SIGNAL( clicked() ), SLOT( stop_simulation() ) );
   buttonbox->addWidget( pb_stop );

   //QPushButton* pb_dcdt = us_pushbutton( tr( "dC/dt Window"), false );
   //buttonbox->addWidget( pb_dcdt );

   pb_saveSim = us_pushbutton( tr( "Save Simulation"), false );
   connect( pb_saveSim, SIGNAL( clicked() ), SLOT( save_scans() ) );
   buttonbox->addWidget( pb_saveSim );

   QPushButton* pb_help = us_pushbutton( tr( "Help") );
   connect( pb_help, SIGNAL( clicked() ), SLOT( help()) );
   buttonbox->addWidget( pb_help );

   QPushButton* pb_close = us_pushbutton( tr( "Close") );
   buttonbox->addWidget( pb_close );
   connect( pb_close, SIGNAL( clicked() ), SLOT( close()) );

   buttonbox->addStretch();
   main->addLayout( buttonbox, 0, 0 );

   // Right Column
   QBoxLayout* plot = new QVBoxLayout;

   // Simulation Plot
   moviePlot = new US_Plot( tr( "Simulation Window" ), tr( "Radius (cm)" ),
                            tr( "Concentration" ) );
   us_grid  ( moviePlot );
   moviePlot->setMinimumSize( 600, 275);
   moviePlot->setAxisScale( QwtPlot::yLeft, 0.0, 2.0 );
   moviePlot->setAxisScale( QwtPlot::xBottom, 5.8, 7.2 );

   plot->addWidget( moviePlot );

   QBoxLayout* timeSpeed = new QHBoxLayout;

   QLabel* lb_time = us_label( tr( "Time( in seconds):" ) );
   lb_time->setAlignment ( Qt::AlignCenter );
   timeSpeed->addWidget( lb_time );

   lcd_time = us_lcd( 5, 0 );
   timeSpeed->addWidget( lcd_time );
   
   QLabel* lb_speed = us_label( tr( "Current Speed:" ) );
   lb_speed->setAlignment ( Qt::AlignCenter );
   timeSpeed->addWidget( lb_speed );

   lcd_speed = us_lcd( 5, 0 );
   timeSpeed->addWidget( lcd_speed );
   
   plot->addLayout( timeSpeed );

   // Saved Scans
   scanPlot = new US_Plot( tr( "Saved Scans" ), tr( "Radius (cm)" ),
                           tr( "Concentration" ) );
   us_grid  ( scanPlot );
   scanPlot->setMinimumSize( 600, 275);
   scanPlot->setAxisScale( QwtPlot::yLeft, 0.0, 2.0 );
   scanPlot->setAxisScale( QwtPlot::xBottom, 5.8, 7.2 );
   plot->addWidget( scanPlot );

   QBoxLayout* completion = new QHBoxLayout;

   QLabel* lb_component = us_label( tr( "Component:" ) );
   lb_component->setAlignment ( Qt::AlignCenter );
   completion->addWidget( lb_component );

   lcd_component = us_lcd( 2, 0 );
   completion->addWidget( lcd_component );

   lb_progress = us_label( tr( "% Completed:" ) );
   lb_progress->setAlignment ( Qt::AlignCenter );
   completion->addWidget( lb_progress );

   progress = us_progressBar( 0, 100, 0 );
   completion->addWidget( progress );

   plot->addLayout( completion );

   main->addLayout( plot, 0, 1 );
}

void US_Astfem_Sim::init_simparams( void )
{
   struct SpeedProfile sp;

   simparams.speed_step.clear();
   simparams.speed_step << sp;

   simparams.speed_step[ 0 ].duration_hours    = 5;
   simparams.speed_step[ 0 ].duration_minutes  = 30;
   simparams.speed_step[ 0 ].delay_hours       = 0;
   simparams.speed_step[ 0 ].delay_minutes     = 20;
   simparams.speed_step[ 0 ].rotorspeed        = 45000;
   simparams.speed_step[ 0 ].scans             = 30;
   simparams.speed_step[ 0 ].acceleration      = 400;
   simparams.speed_step[ 0 ].acceleration_flag = true;

   simparams.simpoints         = 100;
   simparams.radial_resolution = 0.001;
   simparams.mesh              = 0;
   simparams.moving_grid       = 1;
   simparams.meniscus          = 5.8;
   simparams.bottom            = 7.2;
   simparams.rnoise            = 0.0;
   simparams.tinoise           = 0.0;
   simparams.rinoise           = 0.0;
   simparams.band_volume       = 0.015;
   simparams.rotor             = 0;
   simparams.band_forming      = false;
}

void US_Astfem_Sim::load_experiment( void )
{
   QString fn = QFileDialog::getOpenFileName( this, 
         tr( "Select Experiment File" ),
         US_Settings::resultDir(), "*.us_system" );

   if ( ! fn.isEmpty() )
   {
      int error_code = US_FemGlobal::read_experiment( system, simparams, fn );
      
      if ( error_code < 0 )
      {
         QMessageBox::information( this, 
               tr( "Simulation Module" ), 
               tr( "Unable to load System: ") + fn + tr( "\n\nError code: " ) +
               QString::number( error_code ) ); 
         return;
      }
      else
      {
         pb_simParms   ->setEnabled( true );
         pb_changeModel->setEnabled( true );
         pb_start      ->setEnabled( true );

         QMessageBox::information( this, 
               tr( "Simulation Module" ), 
               tr( "Successfully loaded System:\n\n" ) + system.description );
      }
   }
}

void US_Astfem_Sim::save_experiment( void )
{
   QString fn = QFileDialog::getSaveFileName( this, 
         tr( "Save Experiment File" ), US_Settings::resultDir(), "*.us_system" );
   
   if ( ! fn.isEmpty() )
   {
      int error_code = US_FemGlobal::write_experiment( system, simparams, fn );

      if ( error_code < 0 )
      {
         QMessageBox::information( this, 
            tr( "Simulation Module "), 
            tr( "Unable to save System: " ) + fn + "\n\nError code: " +
            QString::number( error_code ) );
         return;
      }
      else
         QMessageBox::information( this, 
              tr( "Simulation Module" ), 
              tr( "Successfully saved System:\n\n") + system.description );
   }
}

void US_Astfem_Sim::new_model( void )
{
   US_ModelSelection::selectModel( system );

   if ( system.model >= 0 )
   {
      // Will be deleted when closed
      US_ModelEditor* component_dialog = new US_ModelEditor( system );
      if ( component_dialog->exec() )
      {
         pb_simParms   ->setEnabled( true );
         pb_changeModel->setEnabled( true );
      }
   }
}

void US_Astfem_Sim::change_model( void )
{
   US_ModelEditor* component_dialog = new US_ModelEditor( system );
   component_dialog->exec();
}

void US_Astfem_Sim::load_model( void )
{
   QString fn = QFileDialog::getOpenFileName( this, tr( "Select the model to load" ),
         US_Settings::resultDir(), "*.model.?? *.model-?.?? *model-??.??" );
   
   if ( fn.isEmpty() ) return;

   int error_code = US_FemGlobal::read_modelSystem( system, fn );
   if ( error_code < 0 )
   {
      QMessageBox::information( this, 
            tr( "Simulation Module" ), 
            tr( "Unable to load model: ") + fn + tr( "\n\nError code: " ) +
            QString::number( error_code ) ); 
      return;
   }
   else
   {
      pb_simParms   ->setEnabled( true );
      pb_changeModel->setEnabled( true );

      QMessageBox::information( this, 
            tr( "Simulation Module" ), 
            tr( "Successfully loaded System:\n\n" ) + system.description );
   }
}

void US_Astfem_Sim::sim_parameters( void )
{
   US_SimulationParameters* sp = new US_SimulationParameters( simparams );
   
   if ( sp->exec() )
   {
      pb_start  ->setEnabled( true );
      pb_saveExp->setEnabled( true );
   }
}

void US_Astfem_Sim::stop_simulation( void )
{
   stopFlag = ! stopFlag;
   astfem_rsa->setStopFlag( stopFlag );
}

void US_Astfem_Sim::start_simulation( void )
{
   moviePlot->clear();
   moviePlot->replot();
   curve_count = 0;

   scanPlot->clear();
   scanPlot->replot();

   pb_stop   ->setEnabled( true  );
   pb_start  ->setEnabled( false );
   pb_saveExp->setEnabled( false );
   pb_saveSim->setEnabled( false );

   astfem_rsa->set_movie_flag( cb_movie->isChecked() );
   
 
   // The astfem simulation routine expects a dataset structure that is
   // initialized with a time and radius grid, and all concentration points
   // need to be set to zero.  Each speed is a separate mfem_data set.

   astfem_data.clear();

   int ss_size =  simparams.speed_step.size();

   for ( int i = 0; i < ss_size; i++ )
   {
      struct SpeedProfile* sp = &simparams.speed_step[ i ];
      struct mfem_data     data;
      
      astfem_data << data;

      astfem_data[ i ].scan.clear();
      astfem_data[ i ].radius.clear();

      astfem_data[ i ].cell            = 1;
      astfem_data[ i ].channel         = 1;
      astfem_data[ i ].wavelength      = 1;
      astfem_data[ i ].avg_temperature = 20;
      astfem_data[ i ].rpm             = sp->rotorspeed;
      astfem_data[ i ].vbar            = 0.72;
      astfem_data[ i ].vbar20          = 0.72;
      astfem_data[ i ].viscosity       = 1.0;
      astfem_data[ i ].density         = 1.0;
      astfem_data[ i ].meniscus        = simparams.meniscus;
      astfem_data[ i ].bottom          = simparams.bottom;
      astfem_data[ i ].s20w_correction = 1.0;
      astfem_data[ i ].D20w_correction = 1.0;


      astfem_data[ i ].id              = "Simulated data set " +
         QString::number( i + 1 ) + " (of " + 
         QString::number( ss_size ) + ") for speed " +
         QString::number( sp->rotorspeed );

      // To get the right time, we need to take the last scan time from the
      // previous speed step and add the delays to it

      // First time point of this speed step in secs
      double current_time = sp->delay_hours * 3600 + sp->delay_minutes * 60; 

      astfem_data[ i ].wavelength = 999; // Simulation wavelength

      // Assign radius for the i'th dataset:
      double r = simparams.meniscus;
      
      while  ( r <= simparams.bottom )
      {
         astfem_data[ i ].radius << r;
         r += simparams.radial_resolution;
      }

      // Calculate the elapsed time between scans (total time - delay)/(scans-1)
      double increment = (   sp->duration_hours * 3600 + sp->duration_minutes*60
                           - sp->delay_hours    * 3600 - sp->delay_minutes   *60 
                         ) / sp->scans;

      mfem_scan temp_scan;

      for ( int j = 0; j < (int) sp->scans; j++ )
      {
         temp_scan.conc.clear();
         current_time  += increment;
         temp_scan.time = current_time;

         for ( int k = 0; k < astfem_data[ i ].radius.size(); k++ ) 
            temp_scan.conc << 0.0;

         astfem_data[ i ].scan << temp_scan;
      }
   }

   lb_progress->setText( tr( "% Completed:" ) );
   progress->reset();
   progress->setMaximum( system.component_vector.size() );

   // Interpolate simulation onto desired grid based on time, not based on
   // omega-square-t integral
   
   astfem_rsa->setTimeInterpolation( true ); 
   astfem_rsa->setTimeCorrection   ( time_correctionFlag );
   
   stopFlag = false;
   astfem_rsa->setStopFlag( stopFlag );
   
   simparams.band_firstScanIsConcentration = false;

   // Run the simulation
   astfem_rsa->calculate( astfem_data );

   float maxconc = 0.0;
   
   for ( int i = 0; i < system.component_vector.size(); i++ )
      maxconc += system.component_vector[ i ].concentration;
   
   if ( simparams.rinoise != 0.0 )
   {
      float rinoise;
      
      for ( int i = 0; i < ss_size; i++ )
      {
         for ( int j = 0; j < astfem_data[ i ].scan.size(); j++ )
         {
            rinoise = US_Math::box_muller( 0, maxconc * simparams.rinoise / 100 );
            for ( int k = 0; k < astfem_data[ i ].radius.size(); k++ )
               astfem_data[ i ].scan[ j ].conc[ k ] += rinoise;
         }
      }
   }
   
   if ( simparams.rnoise != 0.0 )
   {
      for ( int i = 0; i < ss_size; i++ )
      {
         for ( int j = 0; j < astfem_data[ i ].scan.size(); j++ )
         {
            for ( int k = 0; k < astfem_data[ i ].radius.size(); k++ )
            {
               astfem_data[ i ].scan[ j ].conc[ k ] 
                  += US_Math::box_muller( 0, maxconc * simparams.rnoise / 100 );
            }
         }
      }
   }

   if ( simparams.tinoise != 0.0 )
   {
      QList< float > tinoise;
      
      float val = US_Math::box_muller( 0, maxconc * simparams.tinoise / 100 );
      
      tinoise.clear();
      
      for ( int k = 0; k < astfem_data[ 0 ].radius.size(); k++ )
      {
         val += US_Math::box_muller( 0, maxconc * simparams.tinoise / 100 );
         tinoise << val;
      }

      for ( int i = 0; i < ss_size; i++ )
      {
         for ( int j = 0; j < astfem_data[ i ].scan.size(); j++ )
         {
            for ( int k = 0; k < astfem_data[ i ].radius.size(); k++ )
            {
               astfem_data[ i ].scan[ j ].conc[ k ] += tinoise[ k ];
            }
         }
      }
   }

   if ( ! stopFlag )
   {
      // If we didn't interrupt, we need to set to 100 % complete at end of run
      progress->setValue( system.component_vector.size() ); 
   }

   pb_stop   ->setEnabled( false  );
   pb_start  ->setEnabled( true );
   pb_saveExp->setEnabled( true );
   pb_saveSim->setEnabled( true );

   stopFlag = false;
   
   int total_scans = 0;

   for ( int i = 0; i < ss_size; i++ )
      total_scans += simparams.speed_step[ i ].scans;
   
   total_conc = 0.0;

   for ( int i = 0; i < system.component_vector.size(); i++ )
      total_conc += system.component_vector[ i ].concentration;
   
   if (simparams.band_forming)
      scanPlot->setAxisScale( QwtPlot::yLeft, 0, total_conc );
   else
      scanPlot->setAxisScale( QwtPlot::yLeft, 0, total_conc * 2.0 );

   if ( ! stopFlag )
   {
      unsigned int* curve = new unsigned int[ total_scans ];
   
      double*  x;
      double** y;

      for ( int i = 0; i < ss_size; i++ )
      {
         int radius_count = astfem_data[ i ].radius.size();
         int scan_count   = astfem_data[ i ].scan.size();

         x = new double  [ radius_count ];
         y = new double* [ scan_count   ];

         for ( int j = 0; j < radius_count; j++ )
            x[ j ] = astfem_data[ i ].radius[ j ];

         for ( int j = 0; j < scan_count; j++ )
            y[ j ] = new double [ radius_count ];
         
         for ( int j = 0; j < scan_count; j++ )
         {
            for ( int k = 0; k < radius_count; k++ )
               y[ j ][ k ] = astfem_data[ i ].scan[ j ].conc[ k ];
         }


         for ( int j = 0; j < scan_count; j++ )
         {
            QString title = "Concentration" + QString::number( j );
            QwtPlotCurve* plotCurve = new QwtPlotCurve( title );
            
            plotCurve->setData( x, y[ j ], radius_count );
            plotCurve->setPen( QPen( Qt::yellow ) );
            plotCurve->attach( scanPlot );
         }

         delete [] x;
         
         for ( int j = 0; j < scan_count; j++ ) delete [] y[ j ];
         delete [] y;
      }

      delete [] curve;
      scanPlot->replot();
   }
}

void US_Astfem_Sim::save_scans( void )
{
   QMessageBox mb( QMessageBox::Question, 
         tr( "UltraScan" ), 
         tr( "Please choose an export format:" ), 0, this );

   QPushButton* us  = mb.addButton( tr( "UltraScan"), QMessageBox::YesRole    );
   QPushButton* xla = mb.addButton( tr( "XLA")      , QMessageBox::AcceptRole );
                      mb.addButton( tr( "Cancel")   , QMessageBox::RejectRole );

   mb.exec();
   
   if ( mb.clickedButton() == us ) // Save in UltraScan format
   {
      QString fn = QFileDialog::getSaveFileName( this,
            tr( "Select or input a filename" ),
            US_Settings::resultDir(), "*.us.v" );
      
      if ( ! fn.isEmpty() )
      {
         // If an extension was given, strip it
         fn = fn.left( fn.indexOf( "." ) ); 

         // The user gave a file name, save in UltraScan format
         save_ultrascan( fn ); 
      }
   }
   else if ( mb.clickedButton() == xla ) // Save in XL-A format
   {
      QString fn = QFileDialog::getExistingDirectory( this,
            tr( "Select a directory for the simulated data files:" ),
            US_Settings::dataDir() );

      // The user gave a directory name, save in Beckman/XLA format
      if ( ! fn.isEmpty() ) save_xla( fn ); 
   }
}

void US_Astfem_Sim::save_xla( const QString& dirname )
{
   double b         = simparams.bottom;
   double m         = simparams.meniscus;
   double grid_res  = simparams.radial_resolution;
   int    step_size = simparams.speed_step.size();

   // Add 30 points in front of meniscus                                                               
   uint points = (uint)( ( b - m )/ grid_res ) + 30; 
   
   double maxc = 0.0;
   uint  total_scans = 0;
   
   for ( int k = 0; k < step_size; k++ )
   {
      int last_scan = astfem_data[ k ].scan.size() - 1;
      
      maxc = max( maxc, 
            (float) astfem_data[ k ].scan[ last_scan - 1 ].
              conc[ astfem_data[ k ].scan[ last_scan ].conc.size() - 1 ] );
      
      total_scans += astfem_data[ k ].scan.size();
   }

   US_ClipData* cd = new US_ClipData( maxc, b, m, total_conc );
   if ( ! cd->exec() ) return;
   
   progress->setMaximum( total_scans );
   progress->reset();
   
   double* temp_radius = new double [ points ];
   temp_radius[ 0 ] = m - 30 * grid_res;
   
   for ( int i = 1; i < 30; i++ )
      temp_radius[ i ] = temp_radius[ i - 1 ] + grid_res;
   
   for ( uint i = 30; i < points; i++ )
      temp_radius[ i ] = m + ( grid_res * ( i - 30 ) );
   
   lb_progress->setText( "Writing..." );
   
   int current_scan = 1;
   
   for ( int k = 0; k < step_size; k++ )
   {
      for ( int i = 0; i < astfem_data[ k ].scan.size(); i++ )
      {
         QString s;
         s.sprintf( "/%5.5d.ra1", current_scan );

         QString fn = dirname + s;
 
         QFile f( fn );

         if ( f.open( QIODevice::WriteOnly | QIODevice::Text ) )
         {
            QTextStream ts( &f );
            ts << "Simulated Velocity Data" << endl;
            ts << "R 1 20.0 " << s.sprintf( "%5u %7ld %1.5e %d %d\n", 
                  astfem_data[ k ].rpm, 
                  (long int)astfem_data[ k ].scan[ i ].time, 
                  astfem_data[ k ].scan[ i ].omega_s_t, 999, 1 );
            
            for ( int j = 0; j < 30; j++ ) // The region in front of meniscus
            {
               ts << s.sprintf( "%9.4f %12.5e %12.5e", temp_radius[ j ], 
                     0.0, 0.0 ) << endl;
            }
            
            // One high point for the meniscus:
            ts << s.sprintf( "%9.4f %12.5e %12.5e", temp_radius[ 30 ], 
                  total_conc * 1.3, 0.0 ) << endl;
            
            uint j = 31;
            
            if ( maxc == 0 ) // Ff 0 then use the entire range
            {
               while ( j < points && temp_radius[ j ] <= b )
               {
                  ts << s.sprintf( "%9.4f %12.5e %12.5e", temp_radius[ j ], 
                        astfem_data[ k ].scan[ i ].conc[ j - 30 ], 0.0 ) 
                     << endl;
                  j++;
               }
            }
            else
            {
               while ( j                                        <  points && 
                       astfem_data[ k ].scan[ i].conc[ j - 30 ] <= maxc   && 
                       temp_radius[ j ]                         <= b )
               {
                  ts << s.sprintf( "%9.4f %12.5e %12.5e", temp_radius[ j ], 
                        astfem_data[ k ].scan[ i ].conc[ j - 30 ], 0.0 ) 
                     << endl;
                  j++;
               }
            }
         }

         current_scan++;
         progress->setValue( current_scan );
         qApp->processEvents();
         //f.close();
      }
   }

   progress->setValue( total_scans );
   lb_progress->setText( tr( "Completed" ) );
   
   delete [] temp_radius;
}

void US_Astfem_Sim::save_ultrascan( const QString& filename )
{
   double b           = simparams.bottom;
   double m           = simparams.meniscus;
   double grid_res    = simparams.radial_resolution;
   int    step_size   = simparams.speed_step.size();

   uint   total_scans = 0;
   double maxc        = 0.0;

   for ( int k = 0; k < step_size; k++ )
   {
      // astfem_data[ k ].scan is a struct mfem_scan[]
      int last_scan = astfem_data[ k ].scan.size() - 1;
      
      //  mfem_data[].scan[].conc is a double[]
      int last_conc = astfem_data[ k ].scan[ last_scan ].conc.size() - 1;

      maxc = max( maxc, astfem_data[ k ].scan[ last_scan ].conc[ last_conc] );
      
      total_scans += astfem_data[ k ].scan.size();
   }

   double maxrad = b;
   US_ClipData* cd = new US_ClipData( maxc, maxrad, m, total_conc );
   if ( ! cd->exec() ) return;

   progress->setMaximum( total_scans );
   progress->reset();

   lb_progress->setText( tr("Writing..." ) );
   
   int new_points = astfem_data[ 0 ].scan[ 0 ].conc.size();
   
   for ( int k = 0; k < step_size; k++ )
   {
      // Find the radius from the last scan where the
      // concentration is higher than the threshold (if at all)
      int i = 0;
      
      int last_scan = astfem_data[ k ].scan.size() - 1;

      while ( i < astfem_data[ 0 ].scan[ 0 ].conc.size() && 
                  astfem_data[ k ].scan[ last_scan ].conc[ i ] < maxc )
      {         
         i++;    
      }

      if ( i < new_points ) new_points = i; 
   }
   
   // Then check to see if this radius is larger than the maxrad.
   // If it is, then decrease the point count until the max radius matches.
   
   while ( maxrad < astfem_data[ 0 ].radius[ new_points ] ) --new_points;

   double new_bottom = astfem_data[0].radius[new_points];
   
   QString temp_str  = filename;
   QString run_file  = temp_str;
   QString data_file = temp_str;
   QString scan_file = temp_str;
   QString temp      = temp_str;
   
   int position = temp.lastIndexOf( "/" );
   
   int i = temp.length();
   
   // -1 because we dont want to count the null terminating character
   int j = i - position - 1; 

   QString run_name = temp.right( j );   // Run name without leading path
   run_file.append ( ".us.v"     );
   data_file.append( ".veloc.11" );
   scan_file.append( ".scn"      );
   
   QFile f1( run_file );
   
   if ( f1.exists() )
   {
      if ( ! QMessageBox::question( this, 
               tr( "Warning" ), 
               tr( "Attention:\n"
                   "This file exists already!\n\n"
                   "Do you want to overwrite it?" ), tr( "Yes" ), tr( "No" ) ) )
      {
         f1.close();
         return;
      }
   }

   if ( f1.open( QIODevice::WriteOnly ) ) // Write the binary run file *.us.v
   {
      QDataStream ds( &f1 );
      
      ds << US_Version;
      ds << US_Settings::dataDir() + "/simulation"; // data directory
      
      temp_str = temp_str.right( temp_str.lastIndexOf( "/" ) );

      ds << temp_str;
      ds << (float) 20.0;                    // average temperature
      ds << (int) 1;                         // run_inf.temperature_check;
      ds << (float) 0.0;                     // run_inf.time_correction;
      
      // Run_inf.duration;
      ds << (float) ( astfem_data[astfem_data.size() - 1 ].
            scan[ astfem_data[ astfem_data.size() - 1 ].scan.size() - 1 ].time ); 
       
      // for simulated data, total scans = scans for simulation, 
      // since only one "cell" is simulated
      
      ds << (uint)  total_scans;  
      ds << (float) grid_res;                // run_inf.delta_r
      ds << (int)   -1;                      // experimental data ID
      ds << (int)   -1;                      // Investigator ID
      
      QDate today = QDate::currentDate();
      QString current_date;
      current_date.sprintf( "%d/%d/%d", today.month(), today.day(), today.year() );
      
      ds << current_date;
      ds << (QString) "Simulated Velocity Data";
      ds << (QString) "ultrascan";
      ds << (QString) "192.168.0.1";
      ds << (QString) "QMYSQL3";
      
      ds << (int) 1; // run_inf.exp_type.velocity;
      ds << (int) 0; // run_inf.exp_type.equilibrium;
      ds << (int) 0; // run_inf.exp_type.diffusion;
      ds << (int) 1; // run_inf.exp_type.simulation;
      ds << (int) 0; // run_inf.exp_type.interference;
      ds << (int) 1; // run_inf.exp_type.absorbance;
      ds << (int) 0; // run_inf.exp_type.fluorescence;
      ds << (int) 0; // run_inf.exp_type.intensity;
      ds << (int) 0; // run_inf.exp_type.wavelength;
      
      for ( int i = 0; i < 8; i++ )
      {
         if ( i == 0)
         {
            ds << (int) 0; //centerpiece = simulation cell is zero
            ds << (float) simparams.meniscus;
            
            // Data discription
            ds << QString( tr( "Simulated Data - see corresponding model" ) );
            ds << (unsigned int) 1; // How many wavelengths?
         }
         else
         {
            ds << (unsigned int) 0; // Centerpiece = conventional 2 channel epon
            ds << (float) 0.0;
            ds << QString();
            ds << (unsigned int) 0;
         }
      }

      int serial_number = -1;
      
      for ( int i = 0; i < 8; i++ )
      {
         for ( int j = 0; j < 4; j++ )
         {
            ds << serial_number; //buffer serial number
            
            for( int k = 0; k < 3; k++ )
            {
               ds << serial_number; //peptide serial number
               ds << serial_number; //DNA serial number
            }
         }

         for ( int j = 0; j < 3; j++ )
         {
            if ( i == 0 && j == 0 )
            {
               ds << (uint)  999;  // run_inf.wavelength[i][j]
               ds << (uint)  total_scans;
               ds << (float) 0.0;   // baseline
               ds << (float) simparams.meniscus;
               ds << (float) new_bottom;
               ds << (uint)  new_points;
               ds << (float) ( new_bottom - m ) / new_points;
            }
            else
            {
               ds << (uint)  0;
               ds << (uint)  0;
               ds << (float) 0.0;   // baseline
               ds << (float) 0.0;
               ds << (float) 0.0;
               ds << (uint)  0;
               ds << (float) 0.0;
            }
         }
      }

      for ( int i = 0; i < 8; i++ )
      {
         for ( int k = 0; k < simparams.speed_step.size(); k++ )
         {
            for ( int j = 0; j < astfem_data[ k ].scan.size(); j++ )
            {
               float plateau = 0.0;

               for ( int n = 0; n < system.component_vector.size(); n++ )
               {
                  // This is the equation for radial dilution:
                  plateau += system.component_vector[ n ].concentration * 
                             exp( - 2.0 * system.component_vector[ n ].s *
                                    astfem_data[ k ].scan[ j ].omega_s_t );
               }

               if ( i == 0)
               {
                  ds << (uint)  astfem_data[ k ].rpm;
                  ds << (float) 20.0;     // temperature
                  ds << (uint)  astfem_data[ k ].scan[ j ].time;
                  ds << (float) astfem_data[ k ].scan[ j ].omega_s_t;
                  ds << (float) 1.0;      //plateau;
               }
               else
               {
                  ds << (uint)  0;
                  ds << (float) 0.0;
                  ds << (uint)  0;
                  ds << (float) 0.0;
                  ds << (float) 0.0;
               }
            }
         }
      }

      ds << (int) -1;   // run_inf.rotor;
      f1.flush();
      f1.close();
   }

   QFile f2( data_file );

   // Write the binary scan data file *.veloc.11
   if ( f2.open( QIODevice::WriteOnly ) ) 
   {
      QDataStream ds2( &f2 );
      
      for ( int k = 0; k < simparams.speed_step.size(); k++ )
      {
         for ( int j = 0; j < astfem_data[ k ].scan.size(); j++ )
         {
            for ( int i = 0; i < new_points; i++ )
            {
               ds2 << (float) astfem_data[ k ].scan[ j ].conc[ i ];
            }
         }
      }

      f2.flush();
      f2.close();
   }
}

void US_Astfem_Sim::update_progress( int component )
{
   if ( component == -1 )
   {
      progress->setValue( system.component_vector.size() );
      lcd_component->setMode( QLCDNumber::Hex );
      lcd_component->display( "rA " );
   }
   else
   {
      progress->setValue( component - 1 );
      lcd_component->setMode( QLCDNumber::Dec );
      lcd_component->display( component );
   }
}

void US_Astfem_Sim::show_progress( unsigned int time_step )
{
   progress->setValue( time_step );
}

void US_Astfem_Sim::start_calc( unsigned int steps )
{
   progress_text    = lb_progress->text();
   progress_maximum = progress->maximum();
   progress_value   = progress->value();

   progress   ->setMaximum( steps );
   progress   ->reset();
   lb_progress->setText( tr( "Calculating..." ) );
}

void US_Astfem_Sim::calc_over( void )
{
   progress   ->setMaximum( progress_maximum );
   progress   ->setValue  ( progress_value );
   lb_progress->setText( progress_text );
}

void US_Astfem_Sim::update_movie_plot( QList< double >& x, double* c )
{
   moviePlot->clear();
   double total_c = 0.0;
   
   for ( int i = 0; i < system.component_vector.size(); i++ )
      total_c += system.component_vector[ i ].concentration;

   moviePlot->setAxisScale( QwtPlot::yLeft, 0, total_c * 2.0 );
   
   double* r = new double [ x.size() ];
   
   for ( int i = 0; i < x.size(); i++ ) r[ i ] = x[ i ]; 

   QwtPlotCurve* curve = 
      new QwtPlotCurve( "Scan Number " + QString::number( curve_count++ ) );

   curve->setPen( QPen( Qt::yellow ) );
   curve->setData( r, c, x.size() );
   curve->attach( moviePlot );
   
   moviePlot->replot();
   qApp->processEvents();
   
   delete [] r;
}

void US_Astfem_Sim::dump_system( void )
{
   //struct ModelSystem
   qDebug() << "description" <<system.description;
   qDebug() << "model" << system.model;
   qDebug() << "component vector size" << system.component_vector.size();
   for ( int i = 0; i < system.component_vector.size(); i++ ) 
   {
      qDebug() << "component vector " << i;
      dump_simComponent( system.component_vector[ i ] );
   }
   qDebug() << "association vector size" << system.assoc_vector.size();
   for ( int i = 0; i < system.assoc_vector.size(); i++ )
   {
      qDebug() << "Association vector " << i;
      dump_association( system.assoc_vector[ i ] );
   }
}

void US_Astfem_Sim::dump_simComponent( struct SimulationComponent& sc )
{
   qDebug() << "vbar20" << sc.vbar20;
   qDebug() << "mw" << sc.mw;
   qDebug() << "s" << sc.s;
   qDebug() << "D" << sc.D;
   qDebug() << "sigma" << sc.sigma;
   qDebug() << "delta" << sc.delta;
   qDebug() << "extinction" << sc.extinction;
   qDebug() << "concentration" << sc.concentration;
   qDebug() << "f_f0" << sc.f_f0;
   qDebug() << "show_conc" << sc.show_conc;
   qDebug() << "show_keq" << sc.show_keq;
   qDebug() << "show_koff" << sc.show_koff;
   qDebug() << "show_stoich" << sc.show_stoich;
   qDebug() << "QList show_component" << sc.show_component;
   qDebug() << "shape" << sc.shape;
   qDebug() << "name" << sc.name;
   qDebug() << "mfem_initial:";
   dump_mfem_initial( sc.c0 );
}

void US_Astfem_Sim::dump_mfem_initial( struct mfem_initial& mfem )
{
   qDebug() << "radius list size " << mfem.radius.size();
   qDebug() << "radius list" << mfem.radius;
   qDebug() << "concentration list size " << mfem.concentration.size();
   qDebug() << "concentration list" << mfem.concentration;
}

void US_Astfem_Sim::dump_association( struct Association& as )
{
   qDebug() << "keq" << as.keq;
   qDebug() << "koff" << as.k_off;
   qDebug() << "units" << as.units;
   qDebug() << "component1" << as.component1;
   qDebug() << "component2" << as.component2;
   qDebug() << "component3" << as.component3;
   qDebug() << "stoichiometry1" << as.stoichiometry1;
   qDebug() << "stoichiometry2" << as.stoichiometry2;
   qDebug() << "stoichiometry3" << as.stoichiometry3;
   qDebug() << "comp list size " << as.comp.size();
   qDebug() << "comp list " << as.comp;
   qDebug() << "stoich list size " << as.stoich.size();
   qDebug() << "stoich list " << as.stoich;
   qDebug() << "react list size " << as.react.size();
   qDebug() << "react list " << as.react;
}

void US_Astfem_Sim::dump_simparms( void )
{
   qDebug() << "simparams";
   qDebug() << "mesh_radius list size " << simparams.mesh_radius.size();
   qDebug() << "mesh_radius list " << simparams.mesh_radius;;
   qDebug() << "speed profile list size " << simparams.speed_step.size();
   for ( int i = 0; i < simparams.speed_step.size(); i++ ) dump_ss( simparams.speed_step[ i ] );
   qDebug() << "simpoints " << simparams.simpoints;
   qDebug() << "mesh " << simparams.mesh;
   qDebug() << "moving_grid " << simparams.moving_grid;
   qDebug() << "radial_resolution " << simparams.radial_resolution;
   qDebug() << "meniscus " << simparams.meniscus;
   qDebug() << "bottom " << simparams.bottom;
   qDebug() << "rnoise " << simparams.rnoise;
   qDebug() << "tinoise " << simparams.tinoise;
   qDebug() << "rinoise " << simparams.rinoise;
   qDebug() << "rotor " << simparams.rotor;
   qDebug() << "band_forming " << simparams.band_forming;
   qDebug() << "band_volume " << simparams.band_volume;
   qDebug() << "band_firstScanIsConcentration " << simparams.band_firstScanIsConcentration;
}

void US_Astfem_Sim::dump_ss( struct SpeedProfile& sp )
{
   qDebug() << "speed profile";
   qDebug() << "duration_hours " << sp.duration_hours;
   qDebug() << "duration_minutes " << sp.duration_minutes;
   qDebug() << "delay_hours " << sp.delay_hours;
   qDebug() << "delay_minutes " << sp.delay_minutes;
   qDebug() << "scans " << sp.scans;
   qDebug() << "acceleration " << sp.acceleration;
   qDebug() << "rotorspeed " << sp.rotorspeed;
   qDebug() << "acceleration_flag " << sp.acceleration_flag;
}

void US_Astfem_Sim::dump_astfem_data( void )
{
   qDebug() << "astfem_data---- list size " << astfem_data.size();
   for ( int j = 0; j < astfem_data.size(); j++ ) 
   {
      qDebug() << "id " << astfem_data[ j ].id;
      qDebug() << "cell " << astfem_data[ j ].cell;
      qDebug() << "channel " << astfem_data[ j ].channel;
      qDebug() << "wavelength " << astfem_data[ j ].wavelength;
      qDebug() << "rpm " << astfem_data[ j ].rpm;
      qDebug() << "s20w_correction " << astfem_data[ j ].s20w_correction;
      qDebug() << "D20w_correction " << astfem_data[ j ].D20w_correction;
      qDebug() << "viscosity " << astfem_data[ j ].viscosity;
      qDebug() << "density " << astfem_data[ j ].density;
      qDebug() << "vbar " << astfem_data[ j ].vbar;
      qDebug() << "avg_temperature " << astfem_data[ j ].avg_temperature;
      qDebug() << "vbar20 " << astfem_data[ j ].vbar20;
      qDebug() << "meniscus " << astfem_data[ j ].meniscus;
      qDebug() << "bottom " << astfem_data[ j ].bottom;
      qDebug() << "radius list size " << astfem_data[ j ].radius.size();
      //qDebug() << "radius list " << astfem_data[ j ].radius;;
      qDebug() << "scan list size " << astfem_data[ j ].scan.size();
      for ( int i = 0; i < astfem_data[ j ].scan.size(); i++ ) 
         dump_mfem_scan( astfem_data[ j ].scan [ i ] );
   }
}

void US_Astfem_Sim::dump_mfem_scan( struct mfem_scan& ms )
{
   qDebug() << "mfem_scan----";
   qDebug() << "time " << ms.time;
   qDebug() << "omega_s_t " << ms.omega_s_t;
   qDebug() << "rpm " << ms.rpm;
   qDebug() << "temperature " << ms.temperature;
   qDebug() << "time " << ms.time;
   qDebug() << "conc list size " << ms.conc.size();
   //qDebug() << "conc " << ms.conc;
}
 
