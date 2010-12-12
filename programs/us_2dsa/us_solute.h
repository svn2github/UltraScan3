//! \file us_solute.h
#ifndef US_SOLUTE_H
#define US_SOLUTE_H

#include <QtCore>

#include "us_extern.h"
#include "us_dataIO2.h"
#include "us_simparms.h"
#include "us_model.h"
#include "us_noise.h"
#include "us_db2.h"

//! \brief Solute triples input/output for 2DSA analysis

/*! \class Solute
 *
    This class represents the s,k,c values of a solute that is
    input to or output from the 2DSA analysis.
*/
class Solute
{
   public:
      double s;     // sedimentation coefficent of solute
      double k;     // frictional ratio (f/f0) of solute
      double c;     // concentration of solute

      // Solute constructor:  set s,k,c
      Solute( double s0 = 0.0, double k0 = 0.0, double c0 = 0.0 )
      {
         s = s0;
         k = k0;
         c = c0;
      };

      bool operator== ( const Solute& solute )
      {
         return ( s == solute.s  &&  k == solute.k );
      };

      bool operator!= ( const Solute& solute )
      {
         return ( s != solute.s  ||  k != solute.k );
      };

      bool operator<  ( const Solute& solute ) const
      {
         return ( s < solute.s  ||  ( s == solute.s && k < solute.k ) );
      };

};

//! \brief Worker thread task packet
typedef struct work_packet_s
{
   int     thrn;       // thread number (1,...)
   int     taskx;      // task index (0,...)
   int     depth;      // depth index (0,...)
   int     iter;       // iteration index (0,...)
   int     menmcx;     // meniscus/monte-carlo index (0,...)
   int     typeref;    // refinement-type flag (0,... for UGRID,...)
   int     state;      // state flag (0-3 for READY,RUNNING,COMPLETE,ABORTED)
   int     noisf;      // noise flag (0-3 for NONE,TI,RI,BOTH)

   double  ll_s;       // subgrid lower-limit s
   double  ll_k;       // subgrid lower-limit k

   QVector< Solute > isolutes;  // input solutes
   QVector< Solute > csolutes;  // computed solutes
   QVector< double > ti_noise;  // computed ti noise
   QVector< double > ri_noise;  // computed ri noise

   US_DataIO2::EditedData*  edata;  // pointer to experiment data
   US_SimulationParameters* sparms; // pointer to simulation parameters
} WorkPacket;

#endif