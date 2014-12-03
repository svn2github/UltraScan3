#ifndef US_MODEL_H
#define US_MODEL_H

#include <QtCore>
#include "us_extern.h"
#include "us_db2.h"

//! A class to define a model and provide Disk/DB IO for models.
class US_UTIL_EXTERN US_Model
{
   public:
      //! \brief Constructor for the US_Model class
      US_Model();

      //! \brief Simulation component (solute/analyte) class
      class SimulationComponent;
      //! \brief Reversible association class
      class Association;

      //! Enumeration of the general shapes of molecules
      enum ShapeType { SPHERE, PROLATE, OBLATE, ROD };

      //! The type of optics used to acquire data for the current model
      enum OpticsType{ ABSORBANCE, INTERFERENCE, FLUORESCENCE };

      //! The type of analysis used with the model
      enum AnalysisType { MANUAL, TWODSA, TWODSA_MW, GA, GA_MW, PCSA,
                          COFS, FE, CUSTOMGRID, DMGA, DMGA_CONSTR };

      //! The type of global analysis used with the model
      enum GlobalType { NONE, MENISCUS, GLOBAL, SUPERGLOBAL };


      bool         monteCarlo;  //!< Model was generated by MC Analysis
      double       wavelength;  //!< Wavelength of the data acquisition
      double       variance;    //!< Variance of model fit
      double       meniscus;    //!< Meniscus value for model meniscus fit
      double       alphaRP;     //!< Alpha (Regularization Parameter) value
      QString      description; //!< Text description of the model
      QString      modelGUID;   //!< Identifier of the model
      QString      editGUID;    //!< Identifier of the edit data
      QString      requestGUID; //!< Identifier of the LIMS request
      QString      dataDescrip; //!< Raw data triple description string
      OpticsType   optics;      //!< The optics used for the data acquisition
      AnalysisType analysis;    //!< The analysis used with this model
      GlobalType   global;      //!< Global params used for model generation
      QStringList  mcixmls;     //!< Component MC iteration XML contents
      int          nmcixs;      //!< Number of MC component xmls (iterations)

		//! An integer to define the number of subgrids for a CUSTOMGRID needed
		//! for the 2DSA initialization or output components for DISCRETEGA
		int		    subGrids;

      //! An index into components (-1 means none).  Generally buffer data 
      //! in the form of a component that affects the data readings.
      int          coSedSolute;  

      //! The components being analyzed
      QVector< SimulationComponent > components;

      //! The association constants for interacting solutes
      QVector< Association >         associations;

      QString message;  //!< Used internally for communication

      //! \brief Read a model from the disk or database
      //! \param db_access - A flag to indicate if the DB (true) or disk (false)
      //!                    should be searched for the model
      //! \param guid      - The guid of the model to be loaded
      //! \param db        - For DB access, pointer to open database connection
      //! \returns         - The \ref US_DB2 return code for the operation
      int load( bool, const QString&, US_DB2* = 0 );

      //! \brief An overloaded function to read a model from a database
      //! \param id        - Database ModelID
      //! \param db        - For DB access, pointer to open database connection
      //! \returns         - The \ref US_DB2 return code for the operation
      int load( const QString&, US_DB2* ); 


      //! \brief An overloaded function to read a model from the disk
      //! \param filename  The name, including full path, of the analyte file
      //! \returns         - The \ref US_DB2 return code for the operation
      int load( const QString& );  
      
      //! A test for model equality
      bool operator== ( const US_Model& ) const;      

      //! A test for model inequality
      inline bool operator!= ( const US_Model& m )
         const { return ! operator==(m); }

      //! \brief Write a model to the disk or database
      //! \param db_access - A flag to indicate if the DB (true) or disk (false)
      //!                    should be used to save the model
      //! \param filename  - The filename (with path) where the xml file
      //!                    be written if disk access is specified
      //! \param db        - For DB access, pointer to open database connection
      //! \returns         - The \ref US_DB2 return code for the operation
      int write( bool, const QString&, US_DB2* = 0 );

      //! \brief An overloaded function to write a model to the DB
      //! \param db        - A pointer to an open database connection 
      //! \returns         - The \ref US_DB2 return code for the operation
      int write( US_DB2* );

      //! \brief An overloaded function to write a model to a file on disk
      //! \param filename  - The filename to write
      //! \returns         - The \ref US_DB2 return code for the operation
      int write( const QString& );

      //! \brief Update any missing analyte coefficient values
      //! \returns    - Success if values already existed or were successfully
      //!               filled in by calculations.
      bool update_coefficients( void );

      //! \brief Calculate any missing coefficient values in an analyte component
      //! \returns    - Success if values already existed or were successfully
      //!               filled in by calculations.
      static bool calc_coefficients( SimulationComponent& );

      //! \brief Model directory path existence test and creation
      //! \param path    - A reference to the directory path on the disk
      //!                  where model files are to be written
      //! \param is_perm - An optional flag if the files are non-temporary
      //! \returns    - Success if the path is found or created and failure
      //!               if the path cannot be created
      static bool model_path( QString&, bool = true );

      //! \brief Find a model file on the disk
      //! \param path  The full path of the directory to search
      //! \param guid  The GUID of the desired model
      //! \param newFile A reference to a boolean variable.  Sets false
      //!                if the model exist, true otherwise.
      //! \return The file name of the model. If newFile is true, the name
      //!         is the next in the M????????.xml numeric sequence
      //!         but does not yet exist.
      static QString get_filename( const QString&, const QString&, bool& );

      //! \brief Create or append to a composite MC model file
      //! \param mcfiles A list of MC iteration files to concatenate
      //! \param rmvi    Flag to remove iteration files
      //! \return The file name of the created/appended model.
      static QString composite_mc_file( QStringList&, const bool );

      //! \brief Model type text
      //! \param subtype Optional PCSA subtype (1,2,4...->SL,IS,DS...)
      //! \returns    - A short text string describing the type of model
      QString typeText( int = 0 );

      //! \brief Flag constant f/f0
      //! \returns - A boolean flag of whether component f/f0's are constant
      bool constant_ff0( void );

      //! \brief Flag constant vbar
      //! \returns - A boolean flag of whether component vbar's are constant
      bool constant_vbar( void );

      //! \brief Flag if a specified component is a reactant
      //! \param compx Index of component to interrogate
      //! \returns     A boolean flag whether specified component is a reactant
      bool is_reactant( const int compx );

      //! \brief Flag if a specified component is a product of a reaction
      //! \param compx Index of component to interrogate
      //! \returns     A boolean flag whether specified component is a product
      bool is_product ( const int compx );

      //! \brief Get count and list of MonteCarlo iteration xml contents
      //! \param mcixs Reference for return of MC xml contents
      //! \returns     Count of MC iteration xml content strings
      int  mc_iter_xmls( QStringList& );

      //! \brief Dump model data for debugging
      void debug( void );

      //! A class representing the initial concentration distribution of a
      //! solute in the buffer.
      class MfemInitial
      {
         public:
         QVector< double > radius;        //!< The radii of the distribution
         QVector< double > concentration; //!< The concentration values
      };

      //! \brief Simulation component (solute/analyte) class
      //! Each analyte in the model is a component.  A sedimenting solute
      //! can also be a component.
      class US_UTIL_EXTERN SimulationComponent
      {
         public:
         SimulationComponent();
         
         //! A test for identical components
         bool operator== ( const SimulationComponent& ) const;

         //! A test for unequal components
         inline bool operator!= ( const SimulationComponent& sc ) const 
         { return ! operator==(sc); }

         QString     analyteGUID;          //!< GUID for the analyte in the DB
         double      molar_concentration;  //!< Signal attenuation, molar basis
         double      signal_concentration; //!< Analyte concentration
         double      vbar20;               //!< Analyte specific volume
         double      mw;                   //!< Analyte molecular weight
         double      s;                    //!< Sedimentation coefficient
         double      D;                    //!< Diffusion coefficient
         double      f;                    //!< Frictional coefficient
         double      f_f0;                 //!< Frictional ratio
         double      extinction;           //!< Coefficient of light extinction
                                           //!<   at model wavelength
         double      axial_ratio;          //!< Ratio of major/minor shape axes
         double      sigma;         //!< Concentration dependency of s
         double      delta;         //!< Concentration dependency of D
         int         oligomer;      //!< Molecule count for this experiment
                                    //!<   (e.g. dimer = 2)
         ShapeType   shape;         //!< Classification of shape
         QString     name;          //!< Descriptive name
         int         analyte_type;  //!< Protein, RNA, DNA, Corbohydrate, etc
         MfemInitial c0;            //!< The radius/concentration points for a
                                    //!< user-defined initial concentration grid
      };

      //! \brief Reversible association class
      //! The chemical constants associated with a reaction.
      class US_UTIL_EXTERN Association
      {
         public:
         Association();
         double k_d;              //!< Dissociation Constant
         double k_off;            //!< K_Off Rate Constant 
         QVector< int > rcomps;   //!< List of all system components
                                  //!<  involved in this reaction
         QVector< int > stoichs;  //!< List of Stoichiometry values of
                                  //!<  components in chemical equation.
                                  //!<  Positive->reactant; negative->product

         //! A test for equal Associations
         bool operator== ( const Association& ) const;

         //! A test for unequal associations
         inline bool operator!= ( const Association& a ) const 
         { return ! operator==(a); }
      };

   private:

      //! \brief Load a model from the database
      int  load_db         ( const QString&, US_DB2* );
      //! \brief Load a model from a local disk file
      int  load_disk       ( const QString& );
      //! \brief Parse and load an initial concentration vector
      void mfem_scans      ( QXmlStreamReader&, SimulationComponent& );
      //! \brief Parse the associations part of a model XML
      void get_associations( QXmlStreamReader&, Association& );
                           
      //! \brief Load a model from an XML stream
      int  load_stream     ( QXmlStreamReader& );
      //! \brief Load a multi-iteration model from a text stream
      int  load_multi_model( QTextStream&      );
      //! \brief Write to an XML stream
      void write_stream    ( QXmlStreamWriter& );
      //! \brief Write to a multiple model text stream
      void write_mm_stream ( QTextStream&      );

};
#endif
