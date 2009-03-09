//! \file us_astfem_math.h
#ifndef US_ASTFEM_MATH_H
#define US_ASTFEM_MATH_H

#include "us_femglobal.h"
#include "us_extern.h"

//! \brief Reaction Group
struct ReactionGroup
{
   QList< unsigned int > association;
   QList< unsigned int > GroupComponent;
};

//! \brief Component Role
struct ComponentRole
{
   unsigned int          comp_index; // index of this component
   QList< unsigned int > assoc;      // assoc vector index where this component occurs
   QList< int >          react;      // role of component in each association, 
                                     // = 1: if as reactant; =-1, if as product
   QList< unsigned int > st;         // stoichiometry of each component in 
                                     // each assoc., index is linked to assoc.
};

//! \brief Parameters for finite element solution
struct AstFemParameters
{
   unsigned int simpoints;

   QList< double > s;       //!< sedimentation coefficient
   QList< double > D;       //!< Diffusion coefficient
   QList< double > kext;    //!< extinctiom coefficient
   QList< struct ComponentRole > role; //!< role of each component in various reactions

   double pathlength;       //!< path length of centerpiece;
   double dt;               //!< time step size;
   uint   time_steps;       //!< number of time steps for simulation
   double omega_s;          //!< omega^2
   double start_time;       //!< start time in seconds of simulation at constant speed
   double current_meniscus; //!< actual meniscus for current speed
   double current_bottom;   //!< actual bottom for current speed
   uint   first_speed;      //!< constant speed at first speed step
   uint   rg_index;         //!< reaction group index

   //! Local index of each GroupComponent involved in a reaction group
   QList< unsigned int > local_index;  
   
   //! All association rules in a reaction group, with comp expressed in local index
   QList< struct Association > association; 
};

//! \brief A group of static mathematical functions to support finite element 
//!        calculations
class US_EXTERN US_AstfemMath
{ 
   public:
   //! Interpolate first onto second
   static void interpolate_C0( struct mfem_initial&, struct mfem_initial& );

   //! Interpolate starting concentration vector mfem_initial onto C0
   static void interpolate_C0( struct mfem_initial&, double*, QList< double >& );

   static void initialize_2d( uint, uint, double*** );
   static void clear_2d     ( uint, double** );

   static double maxval( const QList< double >& );
   static double minval( const QList< double >& );
   static double maxval( const QList< struct SimulationComponent >& );
   static double minval( const QList< struct SimulationComponent >& );
   
   static void   initialize_3d( uint, uint, uint, double**** );
   static void   clear_3d     ( uint, uint, double*** );
   
   static void   tridiag      ( double*, double*, double*, 
                                double*, double*, uint );

   static double cube_root    ( double, double, double );
   static int    GaussElim    ( int, double**, double* );

   static double find_C1_mono_Nmer( int, double, double );

   static int    interpolate  ( struct mfem_data&, struct mfem_data&, bool );  
   static void   QuadSolver   ( double*, double*, double*, double*, 
                                double*, double*, uint);
   
   static void   IntQT1       ( QList< double >, double, double, double**, double );
   static void   IntQTm       ( QList< double >, double, double, double**, double );
   static void   IntQTn2      ( QList< double >, double, double, double**, double );
   static void   IntQTn1      ( QList< double >, double, double, double**, double );
   static void   DefineFkp    ( uint, double** );
   static double AreaT        ( QList< double >&, QList< double >& );

   static void   BasisTS      ( double, double, double*, double*, double*);
   static void   BasisQS      ( double, double, double*, double*, double*);
   
   static void   BasisTR      ( QList< double >, QList< double >, double, double, 
                               double*, double*, double* );
   
   static void   BasisQR      ( QList< double >, double, double, double*, double*, 
                                double*, double );

   static double Integrand    ( double, double, double, double, double, double, 
                                double, double);

   static void   DefineGaussian( uint, double** );
};
#endif
