#include "us_mpi_analysis.h"
#include "us_math2.h"
#include "us_tar.h"

#include <mpi.h>
#include <sys/user.h>

int main( int argc, char* argv[] )
{
   MPI_Init( &argc, &argv );
   QCoreApplication application( argc, argv );
   //US_MPI_Analysis* program = new US_MPI_Analysis( argv[ 1 ] );
   new US_MPI_Analysis( argv[ 1 ] );
   
   //QTimer::singleShot( 10, program, SLOT( start() ) );
   //return application.exec();
}

// Constructor
US_MPI_Analysis::US_MPI_Analysis( const QString& tarfile ) : QObject()
{
   MPI_Comm_size( MPI_COMM_WORLD2, &node_count );
   MPI_Comm_rank( MPI_COMM_WORLD2, &my_rank );

   if ( my_rank == 0 ) 
      socket = new QUdpSocket( this );

   QString output_dir = "output";
   QDir    d( "." );

   if ( my_rank == 0 )
   {
      // Unpack the input tarfile
      US_Tar tar;

      int result = tar.extract( tarfile );

      if ( result != TAR_OK ) abort( "Could not unpack " + tarfile );
      
      // Create a dedicated output directory and make sure it's empty
      // During testing, it may not always be empty
      QDir output( "." );
      output.mkdir  ( output_dir );
      output.setPath( output_dir );

      QStringList files = output.entryList( QStringList( "*" ), QDir::Files );
      QString     file;

      foreach( file, files ) output.remove( file );
   }
 
   MPI_Barrier( MPI_COMM_WORLD2 ); // Sync everybody up

   QStringList files = d.entryList( QStringList( "hpc*.xml" ) );
   if ( files.size() != 1 ) abort( "Could not find unique hpc input file." );
   
   QString xmlfile = files[ 0 ];

   maxrss         = 0;
   set_count      = 0;
   iterations     = 1;

   previous_values.variance = 1.0e99;  // A large number

   data_sets .clear();
   parameters.clear();
   analysisDate = QDateTime::currentDateTime().toString( "yyMMddhhmm" );

   // Clear old list of output files if it exists
   //QFile::remove( "analysis_files.txt" );

   US_Math2::randomize();   // Set system random sequence

   parse( xmlfile );
   send_udp( "Starting" );

   // Read data 
   for ( int i = 0; i < data_sets.size(); i++ )
   {
      DataSet* d = data_sets[ i ];

      try
      {
         int result = US_DataIO2::loadData( ".", d->edit_file, d->run_data );

         if ( result != US_DataIO2::OK ) throw result;
      }
      catch ( int error )
      {
         QString msg = "Abort.  Bad data file " + d->auc_file + " " + d->edit_file;
         abort( msg, error );
      }
      catch ( US_DataIO2::ioError error )
      {
         QString msg = "Abort.  Bad data file " + d->auc_file + " " + d->edit_file;
         abort( msg, error );
      }

      for ( int j = 0; j < d->noise_files.size(); j++ )
      {
          US_Noise noise;

          if ( noise.load( d->noise_files[ j ] ) != 0 )
          {
             QString msg = "Abort.  Bad noise file " + d->noise_files[ j ];
             abort( msg );
          }

          if ( noise.apply_to_data( d->run_data  ) != 0 )
          {
             QString msg = "Abort.  Bad noise file " + d->noise_files[ j ];
             abort( msg );
          }
      }
   }

   // After reading all input, set the working directory for file output.
   QDir::setCurrent( output_dir );

   // Set some minimums
   max_iterations  = parameters[ "max_iterations" ].toInt();
   max_iterations  = max( max_iterations, 1 );

   mc_iterations   = parameters[ "mc_iterations" ].toInt();
   mc_iterations   = max( mc_iterations, 1 );

   meniscus_range  = parameters[ "meniscus_range"  ].toDouble();
   meniscus_points = parameters[ "meniscus_points" ].toInt();
   meniscus_points = max( meniscus_points, 1 );

   // Do some parameter checking
   bool global_fit = data_sets.size() > 1;

   if ( global_fit  &&  meniscus_points > 1 )
   {
      abort( "Meniscus fit is not compatible with multiple data sets" );
   }

   if ( meniscus_points > 1  &&  mc_iterations > 1 )
   {
      abort( "Meniscus fit is not compatible with Monte Carlo analysis" );
   }

   bool noise = parameters[ "rinoise_option" ].toInt() > 0  ||
                parameters[ "rinoise_option" ].toInt() > 0;

   if ( mc_iterations > 1  &&  noise )
   {
      abort( "Monte Carlo iteration is not compatible with noise computation" );
   }

   if ( global_fit && noise )
   {
      abort( "Global fit is not compatible with noise computation" );
   }

   // Calculate meniscus values
   meniscus_values.resize( meniscus_points );

   double meniscus_start = data_sets[ 0 ]->run_data.meniscus 
                         - meniscus_range / 2.0;
   
   double dm = ( meniscus_points > 1 ) ? meniscus_range / ( meniscus_points - 1 ): 0.0;

   for ( int i = 0; i < meniscus_points; i++ )
   {
      meniscus_values[ i ] = meniscus_start + dm * i;
   }

   // Get lower limit of data and last (largest) meniscus value
   double start_range   = data_sets[ 0 ]->run_data.radius( 0 );
   double last_meniscus = meniscus_values[ meniscus_points - 1 ];

   if ( last_meniscus >= start_range )
   {
      abort( "Meniscus value extends into data" );
   }

   meniscus_run = 0;
   mc_iteration = 0;
   start();
}

// Main function
void US_MPI_Analysis::start( void )
{
   // Real processing goes here
   if ( analysis_type == "2DSA" )
   {
      iterations = parameters[ "montecarlo_value" ].toInt();
      if ( iterations < 1 ) iterations = 1;
      
      if ( my_rank == 0 ) 
          _2dsa_master();
      else
          _2dsa_worker();
   }
   /*
   else if ( analysis_type == "GA" )
   {

   }
   */
   
   if ( my_rank == 0 )
   {
      send_udp( "Finished: " + QString::number( maxrss) );
      qDebug() << "Finished: " + QString::number( maxrss );
   }

   // Pack results
   if ( my_rank == 0 )
   {
      QDir        d( "." );
      QStringList files = d.entryList( QStringList( "*" ), QDir::Files );

      US_Tar tar;
      tar.create( "analysis-results.tar", files );

      // Remove the files we just put into the tar archive
      QString file;
      foreach( file, files ) d.remove( file );
   }

   MPI_Finalize();
   exit ( 0 );
}

// Send udp
void US_MPI_Analysis::send_udp( const QString& message )
{
   if ( my_rank != 0 ) return;

   QString    jobid = db_name + "-" + requestID + ": ";
   QByteArray msg   = QString( jobid + message ).toAscii();
   socket->writeDatagram( msg.data(), msg.size(), server, port );
}

long int US_MPI_Analysis::max_rss( void )
{
   // Read /prod/$pid/stat
   QFile f( "/proc/" + QString::number( getpid() ) + "/stat" );
   f.open( QIODevice::ReadOnly );
   QByteArray ba = f.read( 1000 );
   f.close();

   //The 23rd entry is RSS in 4K pages.  Convert to kilobytes.

   const static int k = PAGE_SIZE / 1024;
   maxrss = max( maxrss, QString( ba ).section( " ", 23, 23 ).toLong() * k );
   return maxrss;
}

void US_MPI_Analysis::abort( const QString& message, int error )
{
    send_udp( message );
    qDebug() << message;
    MPI_Abort( MPI_COMM_WORLD2, error );
    exit( error );
}