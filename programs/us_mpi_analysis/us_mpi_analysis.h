#ifndef US_MPI_ANALYSIS_H
#define US_MPI_ANALYSIS_H

#include <QtCore>
#include <QtNetwork>
#include <mpi.h>

#include "us_model.h"
#include "us_dataIO2.h"
#include "us_noise.h"
#include "us_simparms.h"
#include "us_solve_sim.h"
#include "us_vector.h"

#define SIMULATION       US_SolveSim::Simulation
#define DATASET          US_SolveSim::DataSet

#define DbgLv(a) if(dbg_level>=a)qDebug() //!< debug-level-conditioned qDebug()
#define DbTiming if(dbg_timing)qDebug()   //!< debug-timing-conditioned qDebug()

class US_MPI_Analysis : public QObject
{
 Q_OBJECT

  public:
    US_MPI_Analysis( const QString& );

  public slots:
    void start( void );
     
  private:
    int                 proc_count;
    int                 my_rank;
    int                 iterations;           // Master only - Iterative
    int                 max_iterations;       // Master only - Iterative
    int                 mc_iterations;        // Monte Carlo
    int                 mc_iteration;         // Monte Carlo current iteration
    int                 max_experiment_size;
    int                 dbg_level;
    bool                dbg_timing;

    int                 current_dataset;      // For global fit
    int                 datasets_to_process;  // For global fit
    int                 count_calc_residuals; // Simple counter

    int                 population;
    int                 generations;
    int                 crossover;
    int                 mutation;
    int                 plague;
    int                 migrate_count;
    int                 elitism;

    double              mutate_sigma;
    double              p_mutate_s;
    double              p_mutate_k;
    double              p_mutate_sk;
    double              beta;

    long int            maxrss;
    static const int    min_experiment_size      = 100;
    static const double min_variance_improvement = 1.0e-100;
                        
    QVector< int >      worker_status;
    QVector< int >      worker_depth;
    int                 max_depth;
    int                 worknext;
    enum                WorkerStatus { INIT, READY, WORKING };
                        
    int                 meniscus_points;
    int                 meniscus_run;
    double              meniscus_range;   // Only used by master
    double              meniscus_value;   // Only used by worker
    QVector< double >   meniscus_values;

    QVector< double >   mc_data;
    QVector< double >   sigmas;
    QVector< long >     work_rss;

    US_DataIO2::RawData *res_data;       // Populated in calc_residuals
    US_DataIO2::RawData *sim_data;       // Populated in calc_residuals
    US_DataIO2::RawData sim_data1;       // Simulation for mc iteration 1
    US_DataIO2::RawData scaled_data;     // Populated after global fit

    QHostAddress        server;
    quint16             port;
    QUdpSocket*         socket;
                       
    QString             requestID;
    QString             directory;
    QString             analysis_type;
    QString             cluster;
    QString             db_name;
    QString             modelGUID;
    QString             requestGUID;
    QString             analysisDate;

    QMap< QString, QString > parameters;
  
    QDateTime           startTime;

    int                 set_count;
    QList< DATASET* >   data_sets;

    class MPI_Job
    {
        public:
            static const int MASTER = 0;
            static const int TAG3   = 3;

            enum Command { IDLE, PROCESS, WAIT, SHUTDOWN, NEWDATA };
            enum Status  { TAG0, READY, RESULTS };

            int     solution;
            int     length;
            Command command;
            int     depth;
            double  meniscus_value;
            int     dataset_offset;
            int     dataset_count;

            MPI_Job()
            {
                solution       = 0;
                length         = 0;
                command        = IDLE;
                depth          = 0;
                meniscus_value = 0.0;
                dataset_offset = 0;
                dataset_count  = 1;
            };
    };

    QList< QVector< US_Solute > > orig_solutes;

    class _2dsa_Job
    {
       public:
          MPI_Job              mpi_job;
          QVector< US_Solute > solutes;
    };

    QList< _2dsa_Job > job_queue;

    static const double LARGE          = 9.9e99;
    static const int    solute_doubles = sizeof( US_Solute ) / sizeof( double );
    QList< QVector< US_Solute > > calculated_solutes;

    SIMULATION simulation_values;
    SIMULATION previous_values;

    // GA class variables and classes

    class Bucket
    {
      public:
         double s_min;
         double s_max;
         double ff0_min;
         double ff0_max;
         double ds;
         double dk;
    };

    int                       generation;
    int                       s_grid;
    int                       k_grid;
    int                       fitness_count;

    typedef QVector< US_Solute > Gene;

    double                    regularization;
    double                    concentration_threshold;
    QList< Bucket >           buckets;
    QList< Gene >             genes;
    QList< Gene >             best_genes;   // Size is number of processors
    QList< SIMULATION >       sim_values;
    QMap < QString, double >  fitness_map;
    int                       fitness_hits;

    class Fitness
    {
      public:
       int    index;      // Index into genes and sim_values lists
       double fitness;

       // Set the operators so we can sort a fitness list
       bool operator== ( const Fitness& f )
       {
          return f.fitness == fitness;
       }
       
       bool operator!= ( const Fitness& f )
       {
          return f.fitness != fitness;
       }
       
       // Sort smallest to largest
       bool operator< ( const Fitness& f ) const
       {
          return ( fitness < f.fitness );
       }
    };

    QList< Fitness > fitness;
    QList< Fitness > best_fitness; // Size is number of processors

    class MPI_GA_MSG
    {
      public:
       int    generation;  // From worker: -1 = final; From master: 1 = done
       int    size;        // Number of solutes in the following vector or
                           // or genes requested for emmigration
       double fitness;     // Fitness of best result
    };

    enum { GENERATION, GENE, IMMIGRATE, EMMIGRATE, UPDATE, FINISHED };

    // Methods

    void     parse         ( const QString& );
    void     parse_job     ( QXmlStreamReader& );
    void     parse_dataset ( QXmlStreamReader&, DATASET* );
    void     parse_files   ( QXmlStreamReader&, DATASET* );
    void     parse_solution( QXmlStreamReader&, DATASET* );
    void     send_udp      ( const QString& );
    void     abort         ( const QString&, int=-1 );
    long int max_rss       ( void );

    Gene     create_solutes( double, double, double,
                             double, double, double );
    void     init_solutes  ( void );
    void     fill_queue    ( void );

    // Master
    void     _2dsa_master      ( void );
    void     submit            ( _2dsa_Job&, int );
    void     process_results   ( int, const int* );
    void     shutdown_all      ( void );
    void     write_noise       ( US_Noise::NoiseType, const QVector< double>& );
    void     iterate           ( void );
    void     set_meniscus      ( void );
    void     set_monteCarlo    ( void );
    void     write_output      ( void );
    void     set_gaussians     ( void );
    void     global_fit        ( void );
    void     write_model       ( const SIMULATION&, US_Model::AnalysisType );

    // Worker
    void     _2dsa_worker      ( void );

    void     calc_residuals    ( int, int, SIMULATION& );

    // GA Master
    void ga_master       ( void );
    void ga_master_loop  ( void );
    void ga_global_fit   ( void );
    void set_gaMonteCarlo( void );

    // GA Worker
    void   ga_worker     ( void );
    void   ga_worker_loop( void );
    Gene   new_gene      ( void );
    void   init_fitness  ( void );
    void   mutate_s      ( US_Solute&, int );
    void   mutate_k      ( US_Solute&, int );
    void   mutate_gene   ( Gene& );
    void   cross_gene    ( Gene&, QList< Gene > );
    int    migrate_genes ( void );
    double random_01     ( void );
    int    u_random      ( int = 100 );
    int    e_random      ( void );
    double minimize      ( Gene&, double );
    double get_fitness   ( const Gene& );
    double get_fitness_v ( const US_Vector& );
    void   lamm_gsm_df   ( const US_Vector&, US_Vector& );
    void   align_gene    ( Gene& );

    // Debug
    void dump_buckets( void );
    void dump_genes  ( int );
    void dump_fitness( const QList< Fitness >& );
};
#endif

