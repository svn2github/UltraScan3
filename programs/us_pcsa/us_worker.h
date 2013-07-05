//! \file us_worker.h
#ifndef US_THREAD_WORKER_H
#define US_THREAD_WORKER_H

#include <QtCore>

#include "us_extern.h"
#include "us_dataIO.h"
#include "us_simparms.h"
#include "us_model.h"
#include "us_noise.h"
#include "us_solute.h"
#include "us_solve_sim.h"

#ifndef DbgLv
#define DbgLv(a) if(dbg_level>=a)qDebug()
#endif

//! \brief Worker thread task packet
typedef struct work_packet_s
{
   int     thrn;       // thread number (1,...)
   int     taskx;      // task index (0,...)
   int     depth;      // depth (0->fit, 1->alpha-scan)
   int     state;      // state flag (0-3 for READY,RUNNING,COMPLETE,ABORTED)
   int     noisf;      // noise flag (0-3 for NONE,TI,RI,BOTH)

   double  str_k;      // model start k value
   double  end_k;      // model end   k value
   double  par1;       // sigmoid model par1 value
   double  par2;       // sigmoid model par2 value

   QVector< US_Solute >     isolutes;  // input solutes
   QVector< US_Solute >     csolutes;  // computed solutes
   QVector< double >        ti_noise;  // computed ti noise
   QVector< double >        ri_noise;  // computed ri noise

   QList< US_SolveSim::DataSet* > dsets;     // list of data set object pointers
   US_SolveSim::Simulation        sim_vals;  // simulation values


} WorkPacket;

//! \brief Worker thread to do actual work of 2DSA analysis

/*! \class WorkerThread
 *
    This class is for each of the individual worker threads that do the
    actual work of 2DSA analysis.
*/
class WorkerThread : public QThread
{
   Q_OBJECT

   public:
      WorkerThread( QObject* parent = 0 );
      ~WorkerThread();

   public slots:
      void define_work     ( WorkPacket& );
      void get_result      ( WorkPacket& );
      void run             ();
      void flag_abort      ();
      void forward_progress( int  );

   signals:
      void work_progress   ( int );
      void work_complete   ( WorkerThread* );

   private:

      void calc_residuals   ( void );
      void calc_resids_ratio( void );
      long int max_rss      ( void );
      void apply_alpha      ( const double,
            QVector< double >&, QVector< double >&,
            const int, const int, const int, double&, double& );

      long int maxrss;

      double  str_k;         // start model k value
      double  end_k;         // end   model k value
      double  par1;          // task par1 value
      double  par2;          // task par1 value

      int     thrn;          // thread number (1,...)
      int     taskx;         // grid refinement task index
      int     depth;         // depth index
      int     iter;          // iteration index
      int     menmcx;        // meniscus / monte carlo index
      int     typeref;       // type of refinement (0=UGRID, ...)
      int     nscans;        // number of scans in experiment
      int     npoints;       // number of radius points in experiment
      int     nsolutes;      // number of input solutes for this task
      int     noisflag;      // noise flag (0-3 for NONE|TI|RI|BOTH)
      int     dbg_level;     // debug flag

      bool    abort;         // should this thread be aborted?

      US_DataIO::EditedData*  edata;       // experiment data (pointer)
      US_DataIO::RawData      sdata;       // simulation data
      US_DataIO::RawData      rdata;       // residuals
      US_Model                model;       // output model
      US_Noise                ri_noise;    // computed radially-invariant noise
      US_Noise                ti_noise;    // computed time-invariant noise
      US_Noise                ra_noise;    // computed random noise
      US_SimulationParameters simparms;    // simulation parameters

      QList< US_SolveSim::DataSet* > dsets;     // list of data set obj. ptrs.
      US_SolveSim::Simulation        sim_vals;  // simulation values
      US_SolveSim*                   solvesim;  // object for calc_residuals()
      US_SolveSim::DataSet           dset_wk;   // local copy of data set

      QVector< US_Solute >    solutes_i;   // solutes input
      QVector< US_Solute >    solutes_c;   // solutes computed

      QVector< double >       sv_nnls_a;   // saved nnls_a (A matrix)
      QVector< double >       sv_nnls_b;   // saved nnls_b (B matrix)
};

#endif

