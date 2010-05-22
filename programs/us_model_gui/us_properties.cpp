//! \file us_properties.cpp

#include "us_properties.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_buffer_gui.h"
#include "us_constants.h"
#include "us_femglobal_new.h"

#include "qwt_arrow_button.h"

#include <uuid/uuid.h>

US_Properties::US_Properties( 
      const US_Buffer&               buf, 
      US_FemGlobal_New::ModelSystem& mod,
      int                            invID,
      bool                           access )
   : US_WidgetsDialog( 0, 0 ), 
     buffer      ( buf ),
     model       ( mod ),
     investigator( invID ),
     db_access   ( access )
{
   setPalette( US_GuiSettings::frameColor() );
   setWindowTitle( tr( "Set Analyte Properties" ) );
   setAttribute( Qt::WA_DeleteOnClose );

   oldRow   = -1;
   inUpdate = false;

   normal = US_GuiSettings::editColor();

   // Very light gray
   gray = normal;
   gray.setColor( QPalette::Base, QColor( 0xe0, 0xe0, 0xe0 ) );
   
   // Initialize the check icon
   check = QIcon( US_Settings::usHomeDir() + "/etc/check.png" );

   // Grid
   QGridLayout* main = new QGridLayout( this );
   main->setSpacing( 2 );
   main->setContentsMargins( 2, 2, 2, 2 );

   int row = 0;

   QPushButton* pb_new = us_pushbutton( tr( "New Analyte" ) );
   connect( pb_new, SIGNAL( clicked() ), SLOT( newAnalyte() ) );
   main->addWidget( pb_new, row, 0 );

   QPushButton* pb_delete = us_pushbutton( tr( "Remove Current Analyte" ) );
   connect( pb_delete, SIGNAL( clicked() ), SLOT( del_component() ) );
   main->addWidget( pb_delete, row++, 1 );

   QLabel* lb_desc = us_label( tr( "Analyte Description:" ) );
   main->addWidget( lb_desc, row, 0 );

   le_description = us_lineedit( "" );
   connect( le_description, SIGNAL( editingFinished() ), 
                            SLOT  ( edit_component () ) );
   main->addWidget( le_description, row++, 1 );

   // Components List Box
   lw_components = new US_ListWidget;
   connect( lw_components, SIGNAL( currentRowChanged( int  ) ),
                           SLOT  ( update           ( int  ) ) );
   main->addWidget( lw_components, row, 0, 2, 4 );
   row += 4;

   // Row
   QLabel* lb_guid = us_label( tr( "Global Identifier:" ) );
   main->addWidget( lb_guid, row, 0 );

   le_guid = us_lineedit( "" );
   le_guid->setPalette ( gray );
   le_guid->setReadOnly( true );
   main->addWidget( le_guid, row++, 1 );

   if ( US_Settings::us_debug() == 0 )
   {
      lb_guid->setVisible( false );
      le_guid->setVisible( false );
   }

   // Row
   QLabel* lb_vbar = us_label( tr( "vbar at 20 " ) + DEGC + " (ml/g):" );
   main->addWidget( lb_vbar, row, 0 );

   le_vbar = us_lineedit( "" );
   connect( le_vbar, SIGNAL( editingFinished() ), 
                     SLOT  ( edit_vbar      () ) );
   main->addWidget( le_vbar, row++, 1 );

   // Row
   QLabel* lb_extinction =  us_label( tr( "Extinction (optical units):" ) );
   main->addWidget( lb_extinction, row, 0 );

   QHBoxLayout* extinction = new QHBoxLayout;
   extinction->setSpacing( 0 );

   le_extinction = us_lineedit( "" );
   connect( le_extinction, SIGNAL( editingFinished() ), SLOT( set_molar() ) );
   extinction->addWidget( le_extinction );

   QwtArrowButton* down = new QwtArrowButton( 1, Qt::DownArrow );
   down->setMinimumWidth( 16 );
   connect( down, SIGNAL( clicked() ), SLOT( lambda_down() ) );
   extinction->addWidget( down );

   le_wavelength = us_lineedit( "" );
   le_wavelength->setMinimumWidth( 80 );
   le_wavelength->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
   le_wavelength->setPalette ( gray );
   le_wavelength->setReadOnly( true );
   extinction->addWidget( le_wavelength );

   QwtArrowButton* up = new QwtArrowButton( 1, Qt::UpArrow );
   up->setMinimumWidth( 16 );
   connect( up, SIGNAL( clicked() ), SLOT( lambda_up() ) );
   extinction->addWidget( up );
   
   main->addLayout( extinction, row++, 1 );

   // Row
   QLabel* lb_molar = us_label( tr( "Molar Concentration:" ) );
   main->addWidget( lb_molar, row, 0 );

   le_molar = us_lineedit( "" );
   le_molar->setPalette ( gray );
   le_molar->setReadOnly( true );
   main->addWidget( le_molar, row++, 1 );

   // Row
   QLabel* lb_analyteConc = us_label( tr( "Analyte Signal Concentration:" ) );
   main->addWidget( lb_analyteConc, row, 0 );

   le_analyteConc = us_lineedit( "" );
   connect( le_analyteConc, SIGNAL( editingFinished() ), SLOT( set_molar() ) );
   main->addWidget( le_analyteConc, row++, 1 );

   // Row
   QPushButton* pb_sim = us_pushbutton( tr( "Simulate s and D" ) );
   connect( pb_sim, SIGNAL( clicked() ), SLOT( simulate() ) );
   main->addWidget( pb_sim, row, 0 );

   cmb_shape = us_comboBox();
   cmb_shape->addItem( tr( "Sphere"            ), US_FemGlobal_New::SPHERE  );
   cmb_shape->addItem( tr( "Prolate Ellipsoid" ), US_FemGlobal_New::PROLATE );
   cmb_shape->addItem( tr( "Oblate Ellipsoid"  ), US_FemGlobal_New::OBLATE  );
   cmb_shape->addItem( tr( "Rod"               ), US_FemGlobal_New::ROD     );
   connect( cmb_shape, SIGNAL( currentIndexChanged( int ) ),
                       SLOT  ( select_shape       ( int ) ) );
   main->addWidget( cmb_shape, row++, 1 );

   // Row
   QGridLayout* mw_layout = us_checkbox( 
         tr( "Molecular Wt/Stoichiometry" ), cb_mw, true );
   connect( cb_mw, SIGNAL( toggled( bool ) ), SLOT( calculate( bool ) ) );
   main->addLayout( mw_layout, row, 0 );

   QHBoxLayout* mw_layout2 = new QHBoxLayout;

   le_mw = us_lineedit( "" );
   connect( le_mw, SIGNAL( editingFinished() ), SLOT( calculate() ) );
   mw_layout2->addWidget( le_mw );

   ct_stoich = us_counter( 1, 1.0, 10.0, 1.0 );
   ct_stoich->setStep( 1.0 );
   connect( ct_stoich, SIGNAL( valueChanged ( double ) ), 
                       SLOT  ( set_stoich   ( double ) ) );
   mw_layout2->addWidget( ct_stoich );

   main->addLayout( mw_layout2, row++, 1 );

   // Row
   QGridLayout* f_f0_layout = 
      us_checkbox( tr( "Frictional Ratio (f/f0) (20W)" ), cb_f_f0, true );
   
   connect( cb_f_f0, SIGNAL( toggled( bool ) ), SLOT( calculate( bool ) ) );
   main->addLayout( f_f0_layout, row, 0 );

   le_f_f0 = us_lineedit( "1.25" );
   connect( le_f_f0, SIGNAL( editingFinished () ), SLOT( calculate() ) );
   main->addWidget( le_f_f0, row++, 1 );
   
   // Row 
   QGridLayout* s_layout = us_checkbox( tr( "Sedimentation Coeff. (s) (20W)" ), cb_s );
   cb_s->setEnabled( false);
   connect( cb_s, SIGNAL( toggled( bool ) ), SLOT( calculate( bool) ) );
   main->addLayout( s_layout, row, 0 );

   le_s = us_lineedit( "n/a" );
   le_s->setPalette ( gray );
   le_s->setReadOnly( true );
   connect( le_s, SIGNAL( editingFinished () ), SLOT( calculate() ) );
   main->addWidget( le_s, row++, 1 );

   // Row
   QGridLayout* D_layout = us_checkbox( tr( "Diffusion Coeff. (D) (20W)" ), cb_D );
   cb_D->setEnabled( false);
   connect( cb_D, SIGNAL( toggled( bool ) ), SLOT( calculate( bool ) ) );
   main->addLayout( D_layout, row, 0 );

   le_D = us_lineedit( "n/a" );
   le_D->setPalette ( gray );
   le_D->setReadOnly( true );
   connect( le_D, SIGNAL( editingFinished () ), SLOT( calculate() ) );
   main->addWidget( le_D, row++, 1 );

   // Row
   QGridLayout* f_layout = us_checkbox( tr( "Frictional Coeff. (f) (20W)" ), cb_f );
   cb_f->setEnabled( false);
   connect( cb_f, SIGNAL( toggled( bool ) ), SLOT( calculate( bool ) ) );
   main->addLayout( f_layout, row, 0 );

   le_f = us_lineedit( "n/a" );
   le_f->setPalette ( gray );
   le_f->setReadOnly( true );
   connect( le_f, SIGNAL( editingFinished () ), SLOT( calculate() ) );
   main->addWidget( le_f, row++, 1 );

   // Row
   QLabel* lb_sigma = us_label( tr( "Conc. Dependency of s (<span>&sigma;</span>):" ) );
   main->addWidget( lb_sigma, row, 0 );

   le_sigma = us_lineedit( "" );
   main->addWidget( le_sigma, row++, 1 );

   // Row
   QLabel* lb_delta = us_label( tr( "Conc. Dependency of D (<span>&delta;</span>):" ) );
   main->addWidget( lb_delta, row, 0 );

   le_delta = us_lineedit( "" );
   main->addWidget( le_delta, row++, 1 );

   // Row
   pb_load_c0 = us_pushbutton( tr( "Load C0 from File" ) );
   connect( pb_load_c0, SIGNAL( clicked() ), SLOT( load_c0() ) );
   main->addWidget( pb_load_c0, row, 0 );

   QGridLayout* co_sed_layout = us_checkbox( tr( "Co-sedimenting Solute" ), 
         cb_co_sed );
   connect( cb_co_sed, SIGNAL( stateChanged( int ) ), SLOT( co_sed( int ) ) );
   main->addLayout( co_sed_layout, row++, 1 );
   
   // Pushbuttons
   QPushButton* pb_close = us_pushbutton( tr( "Cancel") );
   main->addWidget( pb_close, row, 0 );
   connect( pb_close, SIGNAL( clicked() ), SLOT( close() ) );

   QPushButton* pb_accept = us_pushbutton( tr( "Accept") );
   main->addWidget( pb_accept, row++, 1 );
   connect( pb_accept, SIGNAL( clicked() ), SLOT( acceptProp() ) );

   clear_entries();
}

void US_Properties::clear_entries( void )
{
   le_guid       ->clear();
   le_vbar       ->clear();
   le_extinction ->clear();
   le_wavelength ->clear();
   le_molar      ->clear();
   le_analyteConc->clear();
   le_mw         ->clear();
   le_s          ->clear();
   le_D          ->clear();
   le_f          ->clear();
   le_f_f0       ->clear();
   le_sigma      ->clear();
   le_delta      ->clear();

   cmb_shape ->setCurrentIndex( 0 );
   ct_stoich ->setValue( 1.0 );
   pb_load_c0->setIcon( QIcon() );
   cb_co_sed ->setChecked( false );
}

void US_Properties::newAnalyte( void )
{
   save_changes( lw_components->currentRow() );
   oldRow = -1;

   US_FemGlobal_New::SimulationComponent sc;
   model.components << sc;

   int last = model.components.size() - 1;
   update_lw();

   lw_components->setCurrentRow( last );  // Runs update() via signal
   calculate();
}

void US_Properties::del_component( void )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   QListWidgetItem* item = lw_components->item( row );
   int response = QMessageBox::question( this,
         tr( "Delete Analyte?" ),
         tr( "Delete the following analyte?\n" ) + item->text(),
         QMessageBox::Yes, QMessageBox::No );

   if ( response == QMessageBox::No ) return;

   if ( model.coSedSolute == row ) model.coSedSolute = -1;

   model.components.remove( row );
   lw_components->setCurrentRow( -1 );
   delete lw_components->takeItem( row );

   oldRow = -1;
   clear_entries();
   le_description->clear();
}

void US_Properties::update_lw( void )
{
   lw_components->clear();

   for ( int i = 0; i < model.components.size(); i++ )
      lw_components->addItem( model.components[ i ].name );
}

void US_Properties::set_stoich( double stoich )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   inUpdate = true;

   sc->mw /= sc->stoichiometry;  // Get monomer mw
   sc->mw *= stoich;             // Now adjust for new stoichiometry
   le_mw->setText( QString::number( sc->mw, 'f', 1 ) );

   sc->extinction /= sc->stoichiometry;
   sc->extinction *= stoich;
   le_extinction->setText( QString::number( sc->extinction, 'f', 1 ) );

   sc->stoichiometry = (int) stoich;

   hydro_data.mw          = sc->mw;
   hydro_data.density     = buffer.density;
   hydro_data.viscosity   = buffer.viscosity;
   hydro_data.vbar        = sc->vbar20;
   hydro_data.axial_ratio = sc->axial_ratio;

   hydro_data.calculate( NORMAL_TEMP );

   set_molar();
   inUpdate = false;
   select_shape( sc->shape );
}

void US_Properties::edit_component( void )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   QString desc = le_description->text().trimmed();
   if ( desc.isEmpty() ) return;

   if ( desc == lw_components->item( row )->text() ) return;

   if ( keep_standard() )  // Do we want to change from the standard values?
   {
      // Restore the description
      le_description->setText( lw_components->item( row )->text() );
      return;
   }
      
   lw_components->disconnect( SIGNAL( currentRowChanged( int ) ) );
   delete lw_components->currentItem();
   
   lw_components->insertItem( row, new QListWidgetItem( desc ) );
   lw_components->setCurrentRow( row );

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];
   sc->name = desc;

   connect( lw_components, SIGNAL( currentRowChanged( int  ) ),
                           SLOT  ( update           ( int  ) ) );

   clear_guid();
   le_wavelength->clear();
}

void US_Properties::edit_vbar( void )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   if ( keep_standard() )  // Change from standard values?
   {
      le_vbar->setText( QString::number( sc->vbar20, 'f', 4 ) );
      return;
   }

   sc->vbar20 = le_vbar->text().toDouble();

   clear_guid();
   le_wavelength->clear();
   
   calculate();
}

void US_Properties::load_c0( void )
{
   int row = lw_components->currentRow();

   if ( row < 0 ) return;

   // See if the initialization vector is already loaded.
   if ( ! pb_load_c0->icon().isNull() )
   {
      int response = QMessageBox::question( this,
         tr( "Remove C0 Data?" ),
         tr( "The C0 infomation is loaded.\n"
             "Remove it?" ),
         QMessageBox::Yes, QMessageBox::No );

      if ( response == QMessageBox::Yes )
      {
         US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

         sc->c0.radius       .clear();
         sc->c0.concentration.clear();
         pb_load_c0->setIcon( QIcon() );
      }

      return;
   }
   
   QMessageBox::information( this,
      tr( "UltraScan Information" ),
      tr( "Please note:\n\n"
          "The initial concentration file should have\n"
          "the following format:\n\n"
          "radius_value1 concentration_value1\n"
          "radius_value2 concentration_value2\n"
          "radius_value3 concentration_value3\n"
          "etc...\n\n"
          "radius values smaller than the meniscus or\n"
          "larger than the bottom of the cell will be\n"
          "excluded from the concentration vector." ) );

   QString fn = QFileDialog::getOpenFileName(
         this, tr( "Load initial concentration" ), US_Settings::resultDir(), "*" );

   if ( ! fn.isEmpty() )
   {
      QFile f( fn );;

      if ( f.open( QIODevice::ReadOnly | QIODevice::Text ) )
      {
         QTextStream ts( &f );

         int row = lw_components->currentRow();

         US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

         sc->c0.radius       .clear();
         sc->c0.concentration.clear();

         // Sets concentration for this component to -1 to signal that we are
         // using a concentration vector

         double val1;
         double val2;

         while ( ! ts.atEnd() )
         {
            ts >> val1;
            ts >> val2;

            if ( val1 > 0.0 ) // ignore radius pairs that aren't positive
            {
               sc->c0.radius        .push_back( val1 );
               sc->c0.concentration .push_back( val2 );
            }
         }

         f.close();
         pb_load_c0->setIcon( check );
      }
      else
      {
         QMessageBox::warning( this,
               tr( "UltraScan Warning" ),
               tr( "UltraScan could not open the file specified\n" ) + fn );
      }
   }
}

void US_Properties::select_shape( int shape )
{
   if ( inUpdate ) return;

   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   switch ( shape )
   {
      case US_FemGlobal_New::PROLATE:
         le_f_f0->setText(  QString::number( hydro_data.prolate.f_f0, 'f', 3 ));
         le_s   ->setText(  QString::number( hydro_data.prolate.s,    'e', 4 ));
         le_D   ->setText(  QString::number( hydro_data.prolate.D,    'e', 4 ));
         le_f   ->setText(  QString::number( hydro_data.prolate.f,    'e', 4 ));
         break;

      case US_FemGlobal_New::OBLATE:
         le_f_f0->setText(  QString::number( hydro_data.oblate.f_f0, 'e', 3 ) );
         le_s   ->setText(  QString::number( hydro_data.oblate.s,    'e', 4 ) );
         le_D   ->setText(  QString::number( hydro_data.oblate.D,    'e', 4 ) );
         le_f   ->setText(  QString::number( hydro_data.oblate.f,    'e', 4 ) );
         break;

      case US_FemGlobal_New::ROD:
         le_f_f0->setText(  QString::number( hydro_data.rod.f_f0, 'e', 3 ) );
         le_s   ->setText(  QString::number( hydro_data.rod.s,    'e', 4 ) );
         le_D   ->setText(  QString::number( hydro_data.rod.D,    'e', 4 ) );
         le_f   ->setText(  QString::number( hydro_data.rod.f,    'e', 4 ) );
         break;
      
      default:  //SPHERE
         le_f_f0->setText(  QString::number( hydro_data.sphere.f_f0, 'e', 3 ) );
         le_s   ->setText(  QString::number( hydro_data.sphere.s,    'e', 4 ) );
         le_D   ->setText(  QString::number( hydro_data.sphere.D,    'e', 4 ) );
         le_f   ->setText(  QString::number( hydro_data.sphere.f,    'e', 4 ) );
         break;
   }

   sc->f_f0 = le_f_f0->text().toDouble();
   sc->f    = le_f   ->text().toDouble();
   sc->s    = le_s   ->text().toDouble();
   sc->D    = le_D   ->text().toDouble();
}

void US_Properties::lambda_down( void )
{
   lambda( true );
}

void US_Properties::lambda_up( void )
{
   lambda( false );
}

int US_Properties::next( QList< double > keys, double wavelength, bool down )
{
   if ( keys.isEmpty() )
   {
      le_wavelength->clear();
      return -1;
   }
   
   qSort( keys );

   int index = keys.indexOf( wavelength );
   
   if (   down && ( index < 1               ) ) return -1;
   if ( ! down && ( index > keys.size() - 2 ) ) return -1;
   
   ( down ) ? index-- : index++;
   return index;
}

void US_Properties::lambda( bool down )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   QList < double > keys;
   double           wavelength = le_wavelength->text().toDouble();
   double           extinction;
   int              index;
   
   switch ( model.optics )
   {
      case US_FemGlobal_New::ABSORBANCE:
         if ( analyte.extinction.size() > 0 )
         {
            keys = analyte.extinction.keys();

            index = next( keys, wavelength, down );
            if ( index < 0 ) return;

            wavelength = keys[ index ];
            le_wavelength->setText( QString::number( wavelength, 'f', 1 ) );
            
            extinction = analyte.extinction[ wavelength ];
            le_extinction->setText( QString::number( extinction, 'e', 4 ) );
         }
         break;
      
      case US_FemGlobal_New::INTERFERENCE:
         if ( analyte.refraction.size() > 0 )
         {
            keys = analyte.refraction.keys();

            index = next( keys, wavelength, down );
            if ( index < 0 ) return;

            wavelength = keys[ index ];
            le_wavelength->setText( QString::number( wavelength, 'f', 1 ) );
            
            extinction = analyte.refraction[ wavelength ];
            le_extinction->setText( QString::number( extinction, 'e', 4 ) );
         }
         break;
      
      case US_FemGlobal_New::FLUORESCENCE:
         if ( analyte.fluorescence.size() > 0 )
         {
            keys = analyte.fluorescence.keys();

            index = next( keys, wavelength, down );
            if ( index < 0 ) return;

            wavelength = keys[ index ];
            le_wavelength->setText( QString::number( wavelength, 'f', 1 ) );
            
            extinction = analyte.fluorescence[ wavelength ];
            le_extinction->setText( QString::number( extinction, 'e', 4 ) );
         }
         break;
   }

   inUpdate = true;
   set_molar();
   inUpdate = false;
}

void US_Properties::set_molar( void )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   double extinction       = le_extinction ->text().toDouble();
   double signalConc       = le_analyteConc->text().toDouble();
   
   if ( ! inUpdate )
   {
      if ( keep_standard() )
      {
         double wavelength = le_wavelength->text().toDouble();
         if ( wavelength < 200.0 ) return;

         switch ( model.optics )
         {
            case US_FemGlobal_New::ABSORBANCE:
               extinction = analyte.extinction[ wavelength ];
               break;
            
            case US_FemGlobal_New::INTERFERENCE:
               extinction = analyte.refraction[ wavelength ];
               break;
            
            case US_FemGlobal_New::FLUORESCENCE:
               extinction = analyte.fluorescence[ wavelength ];
               break;
         }

         le_extinction->setText( QString::number( extinction, 'f', 1 ) );         
         return;
      }
   }
   
   sc->molar_concentration = signalConc / extinction;
   sc->wavelength          = le_wavelength->text().toDouble();

   le_molar->setText( QString::number( sc->molar_concentration, 'e', 4 ) );
}

void US_Properties::clear_guid( void )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   uuid_clear( sc->analyteGUID );
   le_guid->clear();
}

void US_Properties::save_changes( int row )
{
   if ( row < 0 ) return;

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];
      
   // Description
   sc->name = lw_components->item( row )->text(); 
   
   // guid
   if ( le_guid->text().isEmpty() ) 
      uuid_clear( sc->analyteGUID );
   else
   {
      char uuid[ 37 ];
      uuid[ 36 ] = 0;
      strncpy( uuid, le_guid->text().toAscii().data(), 36 );
      uuid_parse( uuid, sc->analyteGUID );
   }

   // vbar
   sc->vbar20 = le_vbar->text().toDouble();

   // Extinction, Wavelength
   sc->extinction = le_extinction->text().toDouble();
   sc->wavelength = le_wavelength->text().toDouble();

   // Molar concentration
   sc->molar_concentration = le_molar->text().toDouble();
   
   //  Signal concentration
   sc->signal_concentration = le_analyteConc->text().toDouble();

   // Shape
   switch ( cmb_shape->currentIndex() )
   {
      case US_FemGlobal_New::SPHERE : 
         sc->shape = US_FemGlobal_New::SPHERE; break;
      
      case US_FemGlobal_New::PROLATE: 
         sc->shape = US_FemGlobal_New::PROLATE; break;
      
      case US_FemGlobal_New::OBLATE : 
         sc->shape = US_FemGlobal_New::OBLATE; break;
      
      default:                        
         sc->shape = US_FemGlobal_New::ROD; break;
   }
   
   int shape = cmb_shape->itemData( cmb_shape->currentIndex() ).toInt();
   sc->shape = ( US_FemGlobal_New::ShapeType ) shape;
   
   // Characteristics
   sc->mw    = le_mw   ->text().toDouble();
   sc->s     = le_s    ->text().toDouble();
   sc->D     = le_D    ->text().toDouble();
   sc->f     = le_f    ->text().toDouble();
   sc->f_f0  = le_f_f0 ->text().toDouble();
   sc->sigma = le_sigma->text().toDouble();
   sc->delta = le_delta->text().toDouble();

   // c0 values are already set
   // co-sed is already set
}

void US_Properties::update( int /* row */ )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   inUpdate = true;

   char uuid[ 37 ];
   uuid[ 36 ] = 0;

   // Save current data 
   save_changes( oldRow );

   oldRow = row;

   // Update to current data
   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   le_description->setText( sc->name );

   // Set guid
   
   if ( uuid_is_null( sc->analyteGUID ) )
      le_guid->clear(); 
   else
   {
      uuid_unparse( sc->analyteGUID, uuid );
      le_guid->setText( QString( uuid ) );
   }

   inUpdate = true;

   // Set vbar
   le_vbar->setText( QString::number( sc->vbar20, 'f', 4 ) );

   // Set extinction and concentration
   le_extinction ->setText( QString::number( sc->extinction,          'e', 4 ));

   if ( sc->wavelength <= 0.0 )
      le_wavelength->clear();
   else
      le_wavelength ->setText( QString::number( sc->wavelength,       'f', 1 ));

   le_molar      ->setText( QString::number( sc->molar_concentration, 'e', 4 ));
   le_analyteConc->setText( QString::number( sc->signal_concentration,'f', 1 ));
   
   // Update hydro data
   hydro_data.mw          = sc->mw;
   hydro_data.density     = buffer.density;
   hydro_data.viscosity   = buffer.viscosity;
   hydro_data.vbar        = sc->vbar20;
   hydro_data.axial_ratio = sc->axial_ratio;

   hydro_data.calculate( NORMAL_TEMP );

   // Set shape
   switch ( sc->shape )
   {
      case US_FemGlobal_New::SPHERE : cmb_shape->setCurrentIndex( 0 ); break;
      case US_FemGlobal_New::PROLATE: cmb_shape->setCurrentIndex( 1 ); break;
      case US_FemGlobal_New::OBLATE : cmb_shape->setCurrentIndex( 2 ); break;
      default:                        cmb_shape->setCurrentIndex( 3 ); break;
   }

   ct_stoich->setValue( sc->stoichiometry );

   // Set characteristics
   le_mw   ->setText( QString::number( sc->mw   , 'f', 1 ) );
   le_f_f0 ->setText( QString::number( sc->f_f0 , 'e', 3 ) );
   le_s    ->setText( QString::number( sc->s    , 'e', 4 ) );
   le_D    ->setText( QString::number( sc->D    , 'e', 4 ) );
   le_f    ->setText( QString::number( sc->f    , 'e', 4 ) );
   le_sigma->setText( QString::number( sc->sigma, 'e', 4 ) );
   le_delta->setText( QString::number( sc->delta, 'e', 4 ) );

   // Set load_co indication
   ( sc->c0.radius.size() > 0 ) ? pb_load_c0->setIcon( check )
                                : pb_load_c0->setIcon( QIcon() );
   // Set co-setimenting solute
   ( row == model.coSedSolute ) ? cb_co_sed->setChecked( true  )
                                : cb_co_sed->setChecked( false );
   inUpdate = false;
}

void US_Properties::simulate( void )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;
   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];
   
   working_data     = hydro_data; // working_data will be updated
   working_data.mw /= sc->stoichiometry;

   US_Predict1* dialog = new US_Predict1( 
         working_data, investigator, analyte, true, true );

   connect( dialog, SIGNAL( changed  ( US_Analyte ) ), 
                    SLOT  ( new_hydro( US_Analyte ) ) );
   dialog->exec();
}

void US_Properties::new_hydro( US_Analyte ad )
{
   int row = lw_components->currentRow();
   if ( row < 0 ) return;  // Should never happen

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   hydro_data       = working_data;
   working_data.mw *= sc->stoichiometry;
   analyte          = ad;

   hydro_data.calculate( NORMAL_TEMP );

   // Set the name of the component
   if ( ! ad.description.isEmpty() )
   {
      lw_components->disconnect( SIGNAL( currentRowChanged( int ) ) );
      delete lw_components->currentItem();
      lw_components->insertItem( row, new QListWidgetItem( ad.description ) );
      lw_components->setCurrentRow( row );

      connect( lw_components, SIGNAL( currentRowChanged( int  ) ),
                              SLOT  ( update           ( int  ) ) );

      sc->name = ad.description;
      le_description->setText( ad.description );
   }

   // Set guid
   le_guid->setText( ad.guid );
   uuid_parse( ad.guid.toAscii().data(), sc->analyteGUID );

   // Set extinction
   if ( ad.extinction.size() > 0 )
   {
      QList< double > keys = ad.extinction.keys();
      le_wavelength->setText( QString::number( keys[ 0 ], 'f', 1 ) );

      double value = ad.extinction[ keys[ 0 ] ] * sc->stoichiometry;
      le_extinction->setText( QString::number( value,     'f', 1 ) );
   }
   else
   {
      le_wavelength->clear();
      le_extinction->setText( "0.0000" );
   }

   sc->extinction = le_extinction->text().toDouble();

   // Set vbar(20), mw
   le_mw  ->setText( QString::number( hydro_data.mw,   'f', 1 ) );
   le_vbar->setText( QString::number( hydro_data.vbar, 'f', 4 ) );

   sc->mw          = hydro_data.mw;
   sc->vbar20      = hydro_data.vbar;
   sc->axial_ratio = hydro_data.axial_ratio;
   
   // Set f/f0, s, D, and f for shape
   cmb_shape->setEnabled( true );
   int shape = cmb_shape->itemData( cmb_shape->currentIndex() ).toInt();
   select_shape( shape );

   inUpdate = true;
   set_molar();
   inUpdate = false;
}

void US_Properties::acceptProp( void ) 
{
   update( 0 );  // Parameter is ignored 
   emit done();
   accept();
}

void US_Properties::checkbox( void )
{
   if ( countChecks() != 2 )
   {
      enable( le_mw  , false, gray );
      enable( le_s   , false, gray );
      enable( le_D   , false, gray );
      enable( le_f   , false, gray );
      enable( le_f_f0, false, gray );
      return;
   }

   ( cb_mw  ->isChecked() ) ? enable( le_mw,   false, normal ) 
                            : enable( le_mw,   true , gray   );
   
   ( cb_s   ->isChecked() ) ? enable( le_s,    false, normal ) 
                            : enable( le_s,    true , gray   );
   
   ( cb_D   ->isChecked() ) ? enable( le_D,    false, normal ) 
                            : enable( le_D,    true , gray   );
   
   ( cb_f   ->isChecked() ) ? enable( le_f,    false, normal ) 
                            : enable( le_f,    true , gray   );
   
   ( cb_f_f0->isChecked() ) ? enable( le_f_f0, false, normal ) 
                            : enable( le_f_f0, true , gray   );
}

void US_Properties::enable( QLineEdit* le, bool status, const QPalette& p )
{
   le->setReadOnly( status );
   le->setPalette ( p );
}

int US_Properties::countChecks( void )
{
   int checked = 0;
   if ( cb_mw  ->isChecked() ) checked++;
   if ( cb_s   ->isChecked() ) checked++;
   if ( cb_D   ->isChecked() ) checked++;
   if ( cb_f   ->isChecked() ) checked++;
   if ( cb_f_f0->isChecked() ) checked++;
   return checked;
}

void US_Properties::setInvalid( void )
{
   if ( ! cb_mw  ->isChecked() ) le_mw  ->setText( tr( "n/a" ) );
   if ( ! cb_s   ->isChecked() ) le_s   ->setText( tr( "n/a" ) );
   if ( ! cb_D   ->isChecked() ) le_D   ->setText( tr( "n/a" ) );
   if ( ! cb_f   ->isChecked() ) le_f   ->setText( tr( "n/a" ) );
   if ( ! cb_f_f0->isChecked() ) le_f_f0->setText( tr( "n/a" ) );
   checkbox();
}

void US_Properties::co_sed( int new_state )
{
   if ( inUpdate ) return;

   if ( new_state == Qt::Checked )
   {
      int row = lw_components->currentRow();

      if ( model.coSedSolute != -1 )
      {
         int response = QMessageBox::question( this,
            tr( "Change co-sedimenting solute?" ),
            tr( "Another component is marked as the co-sedimenting solute.\n"
                "Change it to the current analyte?" ),
            QMessageBox::Yes, QMessageBox::No );

         if ( response == QMessageBox::No )
         {
             cb_co_sed->disconnect();
             cb_co_sed->setChecked( false );
             connect( cb_co_sed, SIGNAL( stateChanged( int ) ), 
                                 SLOT  ( co_sed      ( int ) ) );
             return;
         }
      }
      model.coSedSolute = row;
   }
   else
      model.coSedSolute = -1;
}

bool US_Properties::keep_standard( void )
{
   if ( le_guid->text().isEmpty() ) return false;

   int response = QMessageBox::question( this,
         tr( "Changing Standard Value" ),
         tr( "You are changing a value that does not correspond\n" 
             "with a saved analyte.\n\n"
             "Continue?" ),
         QMessageBox::Yes, QMessageBox::No );

   if ( response == QMessageBox::Yes )
   {
      clear_guid();
      le_wavelength->clear();
      analyte.extinction  .clear();
      analyte.refraction  .clear();
      analyte.fluorescence.clear();
      cmb_shape->setEnabled( false );
      return false;
   }

   return true;
}

void US_Properties::calculate( void )
{
   if ( inUpdate ) return;

   checkbox();
   // First do some sanity checking
   double vbar = le_vbar->text().toDouble();
   if ( vbar <= 0.0 ) return;

   // Exatly two checkboxes must be set
   if ( countChecks() < 2 )
   {
      cb_mw  ->setEnabled( true );
      cb_f_f0->setEnabled( true );
      cb_f   ->setEnabled( true );
      cb_s   ->setEnabled( true );
      cb_D   ->setEnabled( true );
      return;
   }
   
   // Two checkboxes set -- Disable others
   if ( ! cb_mw  ->isChecked() ) cb_mw  ->setEnabled( false );
   if ( ! cb_f_f0->isChecked() ) cb_f_f0->setEnabled( false );
   if ( ! cb_f   ->isChecked() ) cb_f   ->setEnabled( false );
   if ( ! cb_s   ->isChecked() ) cb_s   ->setEnabled( false );
   if ( ! cb_D   ->isChecked() ) cb_D   ->setEnabled( false );

   US_Math::SolutionData d;

// Are these water by default?
   d.vbar      = vbar;
   d.density   = DENS_20W; //buffer.density;
   d.viscosity = VISC_20W; //buffer.viscosity;

   US_Math::data_correction( 20.0, d );  // Always use 20C

   double t = K0 + 20.0;  // Calculatons below need Kelvin

   double mw;
   double s;
   double D;
   double f;
   double f_f0;
   double vol_per_molec;
   double radius_sphere;

   // Molecular weight
   if ( cb_mw->isChecked() )
   {
      mw = le_mw->text().toDouble();
      
      if ( mw <= 0.0 ) 
      {
         setInvalid();
         return;
      }

      vol_per_molec = vbar * mw / AVOGADRO;
      radius_sphere = pow( vol_per_molec * 0.75 / M_PI, 1.0 / 3.0 );
      double f0     = radius_sphere * 6.0 * M_PI * VISC_20W * 0.01;

      // mw and s
      if ( cb_s->isChecked() )
      {
         s = le_s->text().toDouble();
         
         if ( s <= 0.0 ) 
         {
            setInvalid();
            return;
         }

         D    = s * R * t / ( d.buoyancyb * mw );
         f    = mw * d.buoyancyb / ( s * AVOGADRO );
         f_f0 = f / f0;
      }

      // mw and D
      else if ( cb_D->isChecked() )
      {
         D = le_D->text().toDouble();
         
         if ( D <= 0.0 ) 
         {
            setInvalid();
            return;
         }

         s    = D * d.buoyancyb * mw / ( R * t );
         f    = mw * d.buoyancyb / ( s * AVOGADRO );
         f_f0 = f / f0;
      }

      // mw and f
      else if ( cb_f->isChecked() )
      {
         f = le_f->text().toDouble();
         
         if ( f <= 0.0 ) 
         {
            setInvalid();
            return;
         }

         f_f0 = f / f0;
         s    = mw * ( 1.0 - vbar * DENS_20W ) / ( AVOGADRO * f );
         D    = s * R * t / ( d.buoyancyb * mw );
      }

      else // mw and f_f0
      {
         f_f0 = le_f_f0->text().toDouble();
         
         if ( f_f0 < 1.0 ) 
         {
            setInvalid();
            return;
         }

         f = f_f0 * f0;
         s = mw * ( 1.0 - vbar * DENS_20W ) / ( AVOGADRO * f );
         D = s * R * t / ( d.buoyancyb * mw );
      }
   }

   else if ( cb_s->isChecked() )  // mw is NOT checked
   {
      s = le_s->text().toDouble();
      
      if ( s <= 0.0 ) 
      {
         setInvalid();
         return;
      }

      if ( cb_D->isChecked() )   // s and D
      {
         D = le_D->text().toDouble();

         if ( D <= 0.0 )
         {
            setInvalid();
            return;
         }

         mw            = s * R * t / ( D * ( 1.0 - vbar * DENS_20W ) );
         vol_per_molec = vbar * mw / AVOGADRO;
         radius_sphere = pow( vol_per_molec * 0.75 / M_PI, 1.0 / 3.0 );
         double f0     = radius_sphere * 6.0 * M_PI * VISC_20W * 0.01;
         f             = mw * d.buoyancyb / ( s * AVOGADRO );
         f_f0          = f / f0;
      }

      else if (  cb_f->isChecked() ) // s and f
      {
         f = le_f->text().toDouble();
         
         if ( f <= 0.0 ) 
         {
            setInvalid();
            return;
         }

         D             = R * t / ( AVOGADRO * f );
         mw            = s * R * t / ( D * ( 1.0 - vbar * DENS_20W ) );
         vol_per_molec = vbar * mw / AVOGADRO;
         radius_sphere = pow( vol_per_molec * 0.75 / M_PI, 1.0 / 3.0 );
         double f0     = radius_sphere * 6.0 * M_PI * VISC_20W * 0.01;
         f_f0          = f / f0;
      }

      else  // s and f_f0
      {
         f_f0 = le_f_f0->text().toDouble();
         
         if ( f_f0 <= 1.0 ) 
         {
            setInvalid();
            return;
         }

         double n  = 2.0 * s * f_f0 * vbar * VISC_20W; // numerator
         double d  = 1.0 - vbar * DENS_20W;              // denominator
         double f0 = 9.0 * VISC_20W * M_PI * sqrt( n / d );
         f         = f_f0 * f0;
         D         = R * t / ( AVOGADRO * f ); 
         mw        = s * R * t / ( D * ( 1.0 - vbar * DENS_20W ) );
      }
   }

   else if ( cb_D->isChecked() ) // mw and s are NOT checked
   {
      D = le_D->text().toDouble();
      
      if ( D <= 0.0 ) 
      {
         setInvalid();
         return;
      }

      if ( cb_f->isChecked() ) // D and f  -  The is an invalid combination
      {
         setInvalid();
         return;
      }

      else // D and f/f0
      {
         f_f0 = le_f_f0->text().toDouble();
         
         if ( f_f0 <= 1.0 ) 
         {
            setInvalid();
            return;
         }

         f             = R * t / ( AVOGADRO * D );
         double f0     = f / f_f0;
         radius_sphere = f0 / ( 6.0 * M_PI * VISC_20W );
         double volume = ( 4.0 / 3.0 ) * M_PI * pow( radius_sphere, 3.0 );
         mw            = volume * AVOGADRO / vbar;
         s             = mw * (1.0 - vbar * DENS_20W ) / ( AVOGADRO * f );
      }
   }

   else  // only f and f_f0 are checked
   {
      f    = le_f   ->text().toDouble();
      f_f0 = le_f_f0->text().toDouble();
      
      if ( f <= 0.0  ||  f_f0 < 1.0 ) 
      {
         setInvalid();
         return;
      }

      double f0     = f / f_f0;
      D             = R * t / ( AVOGADRO * f );
      radius_sphere = f0 / ( 6.0 * M_PI * VISC_20W );
      double volume = ( 4.0 / 3.0 ) * M_PI * pow( radius_sphere, 3.0 );
      mw            = volume * AVOGADRO / vbar;
      s             = mw * (1.0 - vbar * DENS_20W ) / ( AVOGADRO * f );
   }

   int row = lw_components->currentRow();
   if ( row < 0 ) return;

   US_FemGlobal_New::SimulationComponent* sc = &model.components[ row ];

   if ( mw != sc->mw )
   {
      if ( keep_standard() )  // Reset
      {
         mw   = sc->mw;
         f_f0 = sc->f_f0;
         f    = sc->f; 
         s    = sc->s;    
         D    = sc->D;    
      }
   }

   le_mw  ->setText( QString::number( mw  , 'f', 1 ) );
   le_f_f0->setText( QString::number( f_f0, 'e', 3 ) );
   le_s   ->setText( QString::number( s   , 'e', 4 ) );
   le_D   ->setText( QString::number( D   , 'e', 4 ) );
   le_f   ->setText( QString::number( f   , 'e', 4 ) );

   sc->mw   = mw;
   sc->f_f0 = f_f0;
   sc->f    = f;
   sc->s    = s;
   sc->D    = D;
}

