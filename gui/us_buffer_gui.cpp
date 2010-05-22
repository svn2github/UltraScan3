//! \file us_buffer_gui.cpp
#include "us_buffer_gui.h"
#include "us_gui_settings.h"
#include "us_settings.h"
#include "us_db2.h"
#include "us_passwd.h"
#include "us_constants.h"
#include "us_investigator.h"
#include "us_table.h"
#include "us_util.h"

US_BufferGui::US_BufferGui( 
      int              invID, 
      bool             signal_wanted,
      const US_Buffer& buf, 
      bool             disk 
   ) : US_WidgetsDialog( 0, 0 )
{
   signal        = signal_wanted;
   personID      = invID;
   bufferCurrent = false;
   manualUpdate  = false;
   buffer        = buf;

   US_BufferComponent::getAllFromHD( component_list );

   setWindowTitle( tr( "Buffer Management" ) );
   setPalette( US_GuiSettings::frameColor() );
   setAttribute( Qt::WA_DeleteOnClose );

   normal = US_GuiSettings::editColor();

   // Very light gray for read only line edit widgets
   gray = normal;
   gray.setColor( QPalette::Base, QColor( 0xe0, 0xe0, 0xe0 ) );
   
   int row = 0;
   
   QGridLayout* main = new QGridLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   QStringList DB = US_Settings::defaultDB();
   QLabel* lb_DB = us_banner( tr( "Database: " ) + DB.at( 0 ) );
   lb_DB->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_DB, row++, 0, 1, 3 );

   QPushButton* pb_investigator = us_pushbutton( tr( "Select Investigator" ) );
   connect( pb_investigator, SIGNAL( clicked() ), SLOT( sel_investigator() ) );
   main->addWidget( pb_investigator, row++, 0 );

   QBoxLayout* search = new QHBoxLayout;

   // Search
   QLabel* lb_search = us_label( tr( "Search:" ) );
   search->addWidget( lb_search );

   le_search = us_lineedit();
   le_search->setReadOnly( true );
   connect( le_search, SIGNAL( textChanged( const QString& ) ), 
                       SLOT  ( search     ( const QString& ) ) );
   search->addWidget( le_search );
   main->addLayout( search, row++, 0 );

   // Buffer descriptions from DB
   QLabel* lb_banner1 = us_banner( tr( "Doubleclick on buffer data to select" ), -2 );
   lb_banner1->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner1, row++, 0 );

   lw_buffer_db = us_listwidget();
   connect( lw_buffer_db, SIGNAL( itemDoubleClicked( QListWidgetItem* ) ), 
                          SLOT  ( select_buffer    ( QListWidgetItem* ) ) );

   main->addWidget( lw_buffer_db, row, 0, 6, 1 );
   row += 7;

   // Labels
   QLabel* lb_description = us_label( tr( "Buffer Description:" ) );
   main->addWidget( lb_description, row++, 0 );

   QLabel* lb_guid = us_label( tr( "Global Identifier:" ) );
   main->addWidget( lb_guid, row++, 0 );

   QLabel* lb_buffer1 = us_label( tr( "Please select a Buffer Component:" ) );
   lb_buffer1->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_buffer1, row++, 0 );

   lb_selected = us_label( "" );
   main->addWidget( lb_selected, row++, 0 );

   // Buffer Components
   QLabel* lb_banner2 = us_banner( tr( "Click on item to select" ), -2  );
   lb_banner2->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner2, row++, 0 );

   lw_ingredients = us_listwidget();

   for ( int i = 0; i < component_list.size(); i++ )
      lw_ingredients->addItem( component_list[ i ].name +  
                        " (" + component_list[ i ].range + ")" );

   connect( lw_ingredients, SIGNAL( itemSelectionChanged( void ) ), 
                            SLOT  ( list_component      ( void ) ) );
   
   main->addWidget( lw_ingredients, row, 0, 5, 1 );

   row += 5;

   QPushButton* pb_synch = us_pushbutton( tr( "Synch components with DB" ) );
   connect( pb_synch, SIGNAL( clicked() ), SLOT( synch_components() ) );
   main->addWidget( pb_synch, row++, 0 );

   row = 1;

   // Investigator
   le_investigator = us_lineedit( tr( "Not Selected" ) );
   le_investigator->setReadOnly( true );
   main->addWidget( le_investigator, row++, 1, 1, 2 );

   QGridLayout* db_layout = us_radiobutton( tr( "Use Database" ), rb_db );
   main->addLayout( db_layout, row, 1 );

   QGridLayout* disk_layout = us_radiobutton( tr( "Use Local Disk" ), rb_disk );
   disk ? rb_disk->setChecked( true ) : rb_db->setChecked( true );
   main->addLayout( disk_layout, row++, 2 );

   QLabel* lb_banner3 = us_banner( tr( "Database/Disk Functions" ), -2 );
   lb_banner3->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner3, row++, 1, 1, 2 );

   QPushButton* pb_query = us_pushbutton( tr( "Query Descriptions" ) );
   connect( pb_query, SIGNAL( clicked() ), SLOT( query() ) );
   main->addWidget( pb_query, row, 1 );

   pb_save = us_pushbutton( tr( "Save Buffer" ), false );
   connect( pb_save, SIGNAL( clicked() ), SLOT( save() ) );
   main->addWidget( pb_save, row++, 2 );

   pb_update = us_pushbutton( tr( "Update Buffer" ), false );
   connect( pb_update, SIGNAL( clicked() ), SLOT( update() ) );
   main->addWidget( pb_update, row, 1 );

   pb_del = us_pushbutton( tr( "Delete Buffer" ), false );
   connect( pb_del, SIGNAL( clicked() ), SLOT( delete_buffer() ) );
   main->addWidget( pb_del, row++, 2 );

   // Buffer parameters
   QLabel* lb_density = us_label( 
         tr( "Density (20" ) + DEGC + tr( ", g/cm<sup>3</sup>):" ) );
   main->addWidget( lb_density, row, 1 );

   le_density = us_lineedit();
   connect( le_density, SIGNAL( textEdited( const QString& ) ), 
                        SLOT  ( density   ( const QString& ) ) );
   main->addWidget( le_density, row++, 2 );

   QLabel* lb_viscosity = 
      us_label( tr( "Viscosity (20" ) + DEGC + tr( ", cp):" ) );
   main->addWidget( lb_viscosity, row, 1 );

   le_viscosity = us_lineedit();
   connect( le_density, SIGNAL( textEdited( const QString& ) ), 
                        SLOT  ( viscosity ( const QString& ) ) );
   main->addWidget( le_viscosity, row++, 2 );

   QLabel* lb_ph = us_label( tr( "pH:" ) );
   main->addWidget( lb_ph, row, 1 );

   le_ph = us_lineedit();
   main->addWidget( le_ph, row++, 2 );

   QLabel* lb_compressibility = us_label( tr( "Compressibility:" ) );
   main->addWidget( lb_compressibility, row, 1 );

   le_compressibility = us_lineedit();
   main->addWidget( le_compressibility, row++, 2 );

   QLabel* lb_optics = us_label( tr( "Optics:" ) );
   main->addWidget( lb_optics, row, 0 );

   cmb_optics = us_comboBox();
   cmb_optics->addItem( tr( "Absorbance"   ) );
   cmb_optics->addItem( tr( "Interference" ) );
   cmb_optics->addItem( tr( "Fluorescence" ) );
   main->addWidget( cmb_optics, row, 1 );

   QPushButton* pb_spectrum = us_pushbutton( tr( "Manage Spectrum" ) );
   connect( pb_spectrum, SIGNAL( clicked() ), SLOT( spectrum() ) );
   main->addWidget( pb_spectrum, row++, 2 );

   le_description = us_lineedit();
   main->addWidget( le_description, row++, 1, 1, 2 );

   le_guid = us_lineedit();
   le_guid->setReadOnly( true );
   le_guid->setPalette ( gray );
   main->addWidget( le_guid, row++, 1, 1, 2 );

   if ( US_Settings::us_debug() == 0 )
   {
      lb_guid->setVisible( false );
      le_guid->setVisible( false );
   }

   lb_units = us_label( "" );
   QPalette p = lb_units->palette();
   p.setColor( QPalette::WindowText, Qt::red );
   lb_units->setPalette( p );
   lb_units->setAlignment( Qt::AlignCenter );
   main->addWidget( lb_units, row++, 1, 1, 2 );

   le_concentration = us_lineedit();
   connect( le_concentration, SIGNAL( returnPressed() ), 
                              SLOT  ( add_component() ) );
   main->addWidget( le_concentration, row++, 1, 1, 2 );

   // Current buffer
   QLabel* lb_buffer = us_banner( tr( "Doubleclick on item to remove" ), -2 );
   lb_buffer->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_buffer, row++, 1, 1, 2 );

   lw_buffer = us_listwidget();
   connect( lw_buffer, SIGNAL( itemDoubleClicked( QListWidgetItem* ) ), 
                       SLOT  ( remove_component ( QListWidgetItem* ) ) );
   main->addWidget( lw_buffer, row, 1, 6, 2 );
   row += 6;

   // Standard buttons
   QBoxLayout* buttons = new QHBoxLayout;

   QPushButton* pb_reset = us_pushbutton( tr( "Reset" ) );
   connect( pb_reset, SIGNAL( clicked() ), SLOT( reset() ) );
   buttons->addWidget( pb_reset );

   QPushButton* pb_help = us_pushbutton( tr( "Help" ) );
   connect( pb_help, SIGNAL( clicked() ), SLOT( help() ) );
   buttons->addWidget( pb_help );

   QPushButton* pb_accept = us_pushbutton( tr( "Close" ) );;

   if ( signal )
   {
      QPushButton* pb_cancel = us_pushbutton( tr( "Cancel" ) );
      connect( pb_cancel, SIGNAL( clicked() ), SLOT( reject() ) );
      buttons->addWidget( pb_cancel );

      pb_accept->setText( tr( "Accept" ) );
   }

   connect( pb_accept, SIGNAL( clicked() ), SLOT( accept_buffer() ) );
   buttons->addWidget( pb_accept );

   main->addLayout( buttons, row, 0, 1, 3 );

   set_investigator();
   init_buffer();
}

void US_BufferGui::density( const QString& d )
{
   buffer.density = d.toDouble();

   if ( ! manualUpdate )
   {
      buffer.component    .clear();
      buffer.concentration.clear();
      buffer.componentIDs .clear();
      buffer.GUID         .clear();
      buffer.bufferID     .clear();
      le_guid            ->clear();
      lw_buffer          ->clear();
      lw_buffer          ->addItem( tr( "Manual Override" ) );

      lw_buffer_db->setCurrentRow( -1 );
      manualUpdate  = true;
      bufferCurrent = false;
   }
}

void US_BufferGui::viscosity( const QString& v )
{
   buffer.viscosity = v.toDouble();

   if ( ! manualUpdate )
   {
      buffer.component    .clear();
      buffer.concentration.clear();
      buffer.componentIDs .clear();
      buffer.GUID         .clear();
      buffer.bufferID     .clear();
      le_guid            ->clear();
      lw_buffer          ->clear();
      lw_buffer          ->addItem( tr( "Manual Override" ) );

      lw_buffer_db->setCurrentRow( -1 );
      manualUpdate  = true;
      bufferCurrent = false;
   }
}

void US_BufferGui::init_buffer( void )
{
   if ( ! buffer.GUID.isEmpty() )
   {
      query();

      if ( rb_disk->isChecked() ) // Disk access
      {
         // Search for GUID
         for ( int i = 0; i < GUIDs.size(); i++ )
         {
            if ( GUIDs[ i ] == buffer.GUID )
            {
               QListWidgetItem* item = lw_buffer_db->item( i );
               lw_buffer_db->setCurrentRow( i );
               select_buffer( item );
               manualUpdate = false;
               break;
            }
         }
      }

      else // DB access
      {
         // Search for bufferID
         for ( int i = 0; i < bufferIDs.size(); i++ )
         {
            if ( bufferIDs[ i ] == buffer.bufferID )
            {
               QListWidgetItem* item = lw_buffer_db->item( i );
               lw_buffer_db->setCurrentRow( i );
               select_buffer( item );
               manualUpdate = false;
               break;
            }
         }
      }
   }
   else
   {
      le_description->setText( buffer.description );
      le_density    ->setText( QString::number( buffer.density,   'f', 4 ) );
      le_viscosity  ->setText( QString::number( buffer.viscosity, 'f', 4 ) );
      le_ph         ->setText( QString::number( buffer.pH,        'f', 4 ) );
      le_compressibility->setText( 
                         QString::number( buffer.compressibility, 'f', 4 ) );
   }
}

void US_BufferGui::set_investigator( void )
{
   if ( personID > 0 )
   {
      QString lname;
      QString fname;

      if ( US_Investigator::get_person_info( personID, lname, fname ) )
         assign_investigator( personID, lname, fname );
   }
}

//! \brief Get buffer components from the DB and  write to 
//  etc/bufferComponents.xml
void US_BufferGui::synch_components( void )
{
   US_Passwd pw;

   qApp->processEvents();

   component_list.clear();
   US_BufferComponent::getAllFromDB( pw.getPasswd(), component_list );
   US_BufferComponent::putAllToHD  ( component_list );

   // Update the list widget with components from DB
   lw_ingredients->clear();

   for ( int i = 0; i < component_list.size(); i++ )
      lw_ingredients->addItem( component_list[ i ].name +  
                        " (" + component_list[ i ].range + ")" );
}

void US_BufferGui::sel_investigator( void )
{
   US_Investigator* inv_dialog = new US_Investigator( true );

   connect( inv_dialog, 
      SIGNAL( investigator_accepted( int, const QString&, const QString& ) ),
      SLOT  ( assign_investigator  ( int, const QString&, const QString& ) ) );
   
   inv_dialog->exec();
}

void US_BufferGui::assign_investigator( int invID, 
      const QString& lname, const QString& fname)
{
   buffer.personID = invID;
   le_investigator->setText( "InvID (" + QString::number( invID ) + "): " +
         lname + ", " + fname );
}

// Get the path to the buffers.  Create it if necessary.
bool US_BufferGui::buffer_path( QString& path )
{
   QDir dir;
   path = US_Settings::dataDir() + "/buffers";

   if ( ! dir.exists( path ) )
   {
      if ( ! dir.mkpath( path ) )
      {
         QMessageBox::critical( this,
               tr( "Bad Buffer Path" ),
               tr( "Could not create default directory for buffers\n" )
               + path );
         return false;
      }
   }

   return true;
}

/*! Load buffer data and populate listbox. If an investigator is defined, only
    select the buffer files from the investigator. */
void US_BufferGui::query( void )
{
   if ( rb_disk->isChecked() ) read_buffer();
   else                        read_db(); 
}

//! Load buffer data from Hard Drive
void US_BufferGui::read_buffer( void )
{
   QString path;
   if ( ! buffer_path( path ) ) return;

   filenames   .clear();
   descriptions.clear();
   le_search->  clear();
   le_search->setPalette( gray );
   le_search->setReadOnly( true );

   pb_save  ->setEnabled( false );
   pb_update->setEnabled( false );
   pb_del   ->setEnabled( false );

   QDir f( path );
   QStringList filter( "B*.xml" );
   QStringList f_names = f.entryList( filter, QDir::Files, QDir::Name );

   for ( int i = 0; i < f_names.size(); i++ )
   {
      QFile b_file( path + "/" + f_names[ i ] );

      if ( ! b_file.open( QIODevice::ReadOnly | QIODevice::Text) ) continue;

      QXmlStreamReader xml( &b_file );

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement() )
         {
            if ( xml.name() == "buffer" )
            {
               QXmlStreamAttributes a = xml.attributes();
               descriptions << a.value( "description" ).toString();
               GUIDs        << a.value( "guid"        ).toString();
               filenames    << path + "/" + f_names[ i ];
               bufferIDs    << "";
               break;
            }
         }
      }
   }

   if ( descriptions.size() == 0 )
      lw_buffer_db->addItem( "No buffer files found." );
   else
   {
      le_search->setReadOnly( false );
      le_search->setPalette ( normal );
      search();
   }
}

void US_BufferGui::read_db( void )
{
   US_Passwd pw;
   US_DB2    db( pw.getPasswd() );
   
   if ( db.lastErrno() != US_DB2::OK )
   {
      connect_error( db.lastError() );
      return;
   }

   bufferIDs   .clear();
   descriptions.clear();
   le_search->  clear();
   le_search->setPalette( gray );
   le_search->setReadOnly( true );

   pb_save  ->setEnabled( false );
   pb_update->setEnabled( false );
   pb_del   ->setEnabled( false );

   le_search->setText( "" );
   le_search->setReadOnly( true );

   QStringList q( "get_buffer_desc" );
   q << QString::number( buffer.personID );

   db.query( q );

   while ( db.next() )
   {
      bufferIDs    << db.value( 0 ).toString();
      descriptions << db.value( 1 ).toString();
      GUIDs        << "";
   }

   if ( descriptions.size() < 1  &&  buffer.personID != -1 )
      QMessageBox::information( this,
            tr( "No data" ),
            tr( "No buffer file found for the selected investigator,\n"
                "You can click on \"Reset\", then query the DB to \n"
                "find buffers from all Investigators." ) );
   
   else if ( descriptions.size() < 1 )
      QMessageBox::information( this,
            tr( "No data" ),
            tr( "There are no buffer entries." ) );
   else
   {
      le_search->setReadOnly( false );
      le_search->setPalette ( normal );
      search();
   }
}

void US_BufferGui::search( const QString& text )
{
   lw_buffer_db  ->clear();
   buffer_metadata.clear();

   for ( int i = 0; i < descriptions.size(); i++ )
   {
      if ( descriptions[ i ].contains( 
               QRegExp( ".*" + text + ".*", Qt::CaseInsensitive ) ) )
      {
         BufferInfo info;
         info.index       = i;
         info.description = descriptions[ i ];
         info.guid        = GUIDs       [ i ];
         info.bufferID    = bufferIDs   [ i ];
        
         buffer_metadata << info;

         lw_buffer_db->addItem( info.description );
      }
   }
}

void US_BufferGui::connect_error( const QString& error )
{
   QMessageBox::warning( this, tr( "Connection Problem" ),
         tr( "Could not connect to databasee \n" ) + error );
}

void US_BufferGui::spectrum( void )
{
   QString   spectrum_type = cmb_optics->currentText();
   US_Table* dialog;
   QString   s = tr( "Extinction:" );

   if ( spectrum_type == tr( "Absorbance" ) )
     dialog = new US_Table( buffer.extinction, s, bufferCurrent );
   else if ( spectrum_type == tr( "Interference" ) )
     dialog = new US_Table( buffer.refraction, s, bufferCurrent );
   else
     dialog = new US_Table( buffer.fluorescence, s, bufferCurrent );

   dialog->setWindowTitle( tr( "Manage %1 Values" ).arg( spectrum_type ) );
   dialog->exec();
}

/*! \brief Display the appropriate data when the buffer name in the list widget
           is selected with a double click.
           \param item The description of the buffer selected.
*/
void US_BufferGui::select_buffer( QListWidgetItem* item )
{
   if ( rb_disk->isChecked() ) read_from_disk( item );
   else                        read_from_db  ( item ); 
   
   // Write values to screen
   le_description->setText( buffer.description );
   le_guid       ->setText( buffer.GUID );
   le_density    ->setText( QString::number( buffer.density,   'f', 4 ) );
   le_viscosity  ->setText( QString::number( buffer.viscosity, 'f', 4 ) );
   le_ph         ->setText( QString::number( buffer.pH,        'f', 4 ) );
   le_compressibility->setText( 
                      QString::number( buffer.compressibility, 'f', 4 ) );

   lw_buffer->clear();

   for ( int i = 0; i < buffer.componentIDs.size(); i ++ )
      update_lw_buf( buffer.componentIDs[ i ], buffer.concentration[ i ] );

   // Allow modification of the just selected buffer
   bufferCurrent = true;
   pb_save  ->setEnabled ( true ); 
   pb_update->setEnabled ( true ); 
   pb_del   ->setEnabled ( true ); 
}

void US_BufferGui::read_from_disk( QListWidgetItem* item )
{
   int row = item->listWidget()->currentRow();
   int buf = buffer_metadata[ row ].index;

   if ( ! buffer.readFromDisk( filenames[ buf ] ) )
      qDebug() << "read failed";

   buffer.component.clear();

   for ( int i = 0; i < buffer.componentIDs.size(); i++ )
   {
      int index = buffer.componentIDs[ i ].toInt();
      buffer.component << component_list[ index ];
   }
}

void US_BufferGui::read_from_db( QListWidgetItem* item )
{
   int row = item->listWidget()->currentRow();
   QString bufferID = buffer_metadata[ row ].bufferID;
   read_from_db( bufferID );
}

void US_BufferGui::read_from_db( const QString& bufferID )
{
   US_Passwd pw;
   US_DB2    db( pw.getPasswd() );
   
   // Get the buffer data from the database
   if ( db.lastErrno() != US_DB2::OK )
   {
      connect_error( db.lastError() );
      return;
   }
   
   QStringList q( "get_buffer_info" );
   q << bufferID;   // bufferID from list widget entry
      
   db.query( q );
   db.next(); 
   
   buffer.bufferID        = bufferID;
   buffer.GUID            = db.value( 0 ).toString();
   buffer.description     = db.value( 1 ).toString();
   buffer.compressibility = db.value( 2 ).toString().toDouble();
   buffer.pH              = db.value( 3 ).toString().toDouble();
   buffer.viscosity       = db.value( 4 ).toString().toDouble();
   buffer.density         = db.value( 5 ).toString().toDouble();

   QString personID       = db.value( 6 ).toString();
   buffer.personID        = personID.toInt();
   
   lw_buffer->clear();

   buffer.component    .clear();
   buffer.componentIDs .clear();
   buffer.concentration.clear();
   q                   .clear();

   q << "get_buffer_components" <<  bufferID;

   db.query( q );

   while ( db.next() )
   {
      QString index = db.value( 0 ).toString();
      buffer.componentIDs  << index;
      buffer.concentration << db.value( 4 ).toString().toDouble();
      buffer.component     << component_list[ index.toInt() ];
   }

   // Get the investigator's name and display it
   q.clear();
   q << "get_person_info" << personID;

   db.query( q );
   db.next(); 

   QString fname = db.value( 0 ).toString();
   QString lname = db.value( 1 ).toString();
   buffer.person =  fname + " " + lname;

   le_investigator->setText( "InvID (" + personID + ") " + buffer.person );

   // Get spectrum data
   buffer.getSpectrum( db, "Refraction" );
   buffer.getSpectrum( db, "Extinction" );
   buffer.getSpectrum( db, "Fluorescence" );
}

void US_BufferGui::update_lw_buf( const QString& componentID, double conc )
{
   for ( int i = 0; i < component_list.size(); i ++ )
   {
      if ( componentID == component_list[ i ].componentID )
      {
         QString name = component_list[ i ].name;
         QString unit = component_list[ i ].unit;
   
         QString s = QString::number( conc, 'f', 1 );
         
         lw_buffer->addItem( name + " (" + s + " " + unit + ")" );
         break;
      }
   }
}

void US_BufferGui::update_buffer( void )
{
   buffer.description     = le_description->text();
                          
   buffer.pH              = ( le_ph->text().isEmpty() ) 
                            ? 7.0 
                            : le_ph->text().toDouble();
   
   buffer.density         = le_density    ->text().toDouble();
   buffer.viscosity       = le_viscosity  ->text().toDouble();
   buffer.compressibility = le_compressibility->text().toDouble();

   // These are updated in other places
   //buffer.component
   //buffer.concentration
   //buffer.bufferID
   //buffer.personID
}

void US_BufferGui::delete_buffer( void )
{
   if ( buffer.GUID.size() == 0 || lw_buffer_db->currentRow() < 0 )
   {
      QMessageBox::information( this,
            tr( "Attention" ),
            tr( "First select the buffer which you "
                "want to delete." ) );
      return;
   }

   int response = QMessageBox::question( this, 
            tr( "Confirmation" ),
            tr( "Do you really want to delete this entry?\n"
                "Clicking 'OK' will delete the selected buffer data." ),
            QMessageBox::Ok, QMessageBox::Cancel );
   
   if ( response != QMessageBox::Ok ) return;
   
   if ( rb_disk->isChecked() ) delete_disk();
   else                        delete_db(); 

   reset();
   query();
   bufferCurrent = true;
}

void US_BufferGui::delete_disk( void )
{
   QString path;
   if ( ! buffer_path( path ) ) return;

   bool    newFile;
   QString filename = get_filename( path, le_guid->text(), newFile );

   if ( ! newFile )
   {
      QFile f( filename );
      f.remove();
   }
}

// Delete the buffer data from the database
void US_BufferGui::delete_db( void )
{
   US_Passwd pw;
   US_DB2    db( pw.getPasswd() );

   if ( db.lastErrno() != US_DB2::OK )
   {
      connect_error( db.lastError() );
      return;
   }
   
   QStringList q( "get_bufferID" );
   q << le_guid->text();

   db.query( q );

   int status = db.lastErrno();
   
   if (  status == US_DB2::OK )
   {
      db.next(); 
      QString bufferID = db.value( 0 ).toString();
  
      q[ 0 ] = "delete_buffer";
      q[ 1 ] = bufferID;
      status = db.statusQuery( q );
   }
  
   if ( status  != US_DB2::OK )
   {
      QMessageBox::warning( this,
         tr( "Attention" ),
         tr( "Delete failed.\n\n" ) + db.lastError() );
   }
}

QString US_BufferGui::get_filename( 
      const QString& path, const QString& guid, bool& newFile )
{
   QDir        f( path );
   QStringList filter( "B???????.xml" );
   QStringList f_names = f.entryList( filter, QDir::Files, QDir::Name );
   QString     filename;
   newFile = true;

   for ( int i = 0; i < f_names.size(); i++ )
   {
      QFile b_file( path + "/" + f_names[ i ] );

      if ( ! b_file.open( QIODevice::ReadOnly | QIODevice::Text) ) continue;

      QXmlStreamReader xml( &b_file );

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement() )
         {
            if ( xml.name() == "buffer" )
            {
               QXmlStreamAttributes a = xml.attributes();

               if ( a.value( "guid" ).toString() == guid )
               {
                  newFile  = false;
                  filename = path + "/" + f_names[ i ];
               }

               break;
            }
         }
      }

      b_file.close();
      if ( ! newFile ) return filename;
   }

   // If we get here, generate a new filename
   int number = ( f_names.size() > 0 ) ? f_names.last().mid( 1, 7 ).toInt() : 0;

   return path + "/B" + QString().sprintf( "%07i", number + 1 ) + ".xml";
}


void US_BufferGui::save( void )
{
   if ( le_description->text().isEmpty() )
   {
      QMessageBox::information( this,
            tr( "Attention" ), 
            tr( "Please enter a description for\n"
                "your buffer before saving it!" ) );
      return;
   }

   update_buffer();

   if ( le_guid->text().size() != 36 )
      le_guid->setText( US_Util::new_guid() );

   buffer.GUID = le_guid->text();

   if ( rb_disk->isChecked() ) save_disk();
   else                        save_db(); 
   
   bufferCurrent = true;
}

void US_BufferGui::save_disk( void )
{
   QString path;
   if ( ! buffer_path( path ) ) return;

   bool    newFile;
   QString filename = get_filename( path, le_guid->text(), newFile );
   buffer.writeToDisk( filename );

   QString s = ( newFile ) ? tr( "saved" ) : tr( "updated" );

   QMessageBox::information( this,
         tr( "Save results" ),
         tr( "Buffer " ) + s );

   read_buffer();
}

void US_BufferGui::save_db( void )
{
   if ( buffer.personID < 0 )
   {
      QMessageBox::information( this,
            tr( "Attention" ), 
            tr( "Please select an investigator first!" ) );
      return;
   }

   int response = QMessageBox::question( this, 
            tr( "UltraScan - Buffer Database" ),
            tr( "Click 'OK' to save the selected\n"
                "buffer file into the database" ) );
 
   if ( response == QMessageBox::Ok )
   {
      QStringList q( "new_buffer" );
      q << buffer.GUID
        << buffer.description
        << QString::number( buffer.compressibility, 'f', 4 )
        << QString::number( buffer.pH             , 'f', 4 )
        << QString::number( buffer.density        , 'f', 4 )
        << QString::number( buffer.viscosity      , 'f', 4 );

      US_Passwd pw;
      US_DB2    db( pw.getPasswd() );
            
      if ( db.lastErrno() != US_DB2::OK )
      {
         connect_error( db.lastError() );
         return;
      }

      db.statusQuery( q );

      if ( db.lastErrno() != US_DB2::OK )
      {
         QMessageBox::information( this,
               tr( "Attention" ), 
               tr( "Error saving buffer to the database:\n" )
               + db.lastError() );
         return;
      }

      int bufferID = db.lastInsertID();

      for ( int i = 0; i < buffer.component.size(); i++ )
      {
         q.clear();
         q << "add_buffer_component" 
           << QString::number ( bufferID ) 
           << buffer.component[ i ].componentID
           << QString::number( buffer.concentration[ i ], 'f', 5 );
      
         db.statusQuery( q );
      }

      buffer.putSpectrum( db, "Extinction" );
      buffer.putSpectrum( db, "Refraction" );
      buffer.putSpectrum( db, "Fluorescence" );

      reset();
      read_db();
   }
}

void US_BufferGui::update( void )
{
   update_buffer();
   buffer.GUID = le_guid->text();

   if ( rb_disk->isChecked() ) save_disk();
   else                        update_db(); 

   bufferCurrent = true;
}

/*!  Update changed buffer data to DB table  */
void US_BufferGui::update_db( void )
{
   if ( buffer.bufferID.toInt() <= 0 )
   {
      QMessageBox::information( this,
            tr( "Attention" ), 
            tr( "Please select an existing Buffer first!" ) );
      return;
   }
   
   if ( buffer.personID <= 0 )
   {
      QMessageBox::information( this,
            tr( "Attention" ), 
            tr( "Please select an investigator first!" ) );
      return;
   }
   
   if ( le_description->text().isEmpty() )
   {
      QMessageBox::information( this,
            tr( "Attention" ), 
            tr( "Please enter a buffer description first!" ) );
      return;
   }

   int response = QMessageBox::question( this, 
            tr( "Confirmation" ),
            tr( "Do you really want to update this entry in the database?\n" ),
            QMessageBox::Ok, QMessageBox::Cancel );

   if ( response == QMessageBox::Ok )
   {
      US_Passwd pw;
      US_DB2    db( pw.getPasswd() );

      // Delete the buffer data from the database
      if ( db.lastErrno() != US_DB2::OK )
      {
         connect_error( db.lastError() );
         return;
      }
         
      QStringList q( "update_buffer" );
      q << buffer.bufferID
        << buffer.description
        << QString::number( buffer.compressibility, 'f', 4 )
        << QString::number( buffer.pH             , 'f', 4 )
        << QString::number( buffer.density        , 'f', 4 )
        << QString::number( buffer.viscosity      , 'f', 4 );

      db.statusQuery( q );

      if ( db.lastErrno() != US_DB2::OK )
      {
         QMessageBox::information( this,
               tr( "Attention" ), 
               tr( "Error updating buffer in the database:\n" )
               + db.lastError() );
         return;
      }

      q.clear();
      q << "delete_buffer_components" << buffer.bufferID;
      db.statusQuery( q );

      for ( int i = 0; i < buffer.component.size(); i++ )
      {
         q.clear();
         q << "add_buffer_component" 
           << buffer.bufferID
           << buffer.component[ i ].componentID
           << QString::number( buffer.concentration[ i ], 'f', 5 );

         db.statusQuery( q );
      }

      // Update spectrum by deletion and re-insertion
      q.clear();
      q << "delete_spectrum" << buffer.bufferID << "Buffer" << "Extinction";
      db.statusQuery( q );
      buffer.putSpectrum( db, "Extinction" );
      
      q[ 3 ] = "Refraction";
      db.statusQuery( q );
      buffer.putSpectrum( db, "Refraction" );
      
      q[ 3 ] = "Fluorescence";
      db.statusQuery( q );
      buffer.putSpectrum( db, "Fluorescence" );

      QMessageBox::information( this, 
           tr( "Success" ),
           tr( "The database has been updated\n" ) );
   }
}

/*!  Input the value of component  selected in lw_ingredients.  After 'Return'
 *   key was pressed, this function will dispaly the selected component value in
 *   lw_buffer and recalculate the density and viscosity.  */
void US_BufferGui::add_component( void )
{
   // We are modifying the buffer, nothing should be selected in the DB list
   lw_buffer_db->clearSelection();
   
   if ( lw_ingredients->currentItem() < 0 )
   {
      QMessageBox::information( this, 
            tr( "Attention" ),
            tr( "First select a buffer component!\n" ) );
      return;
   }

   double partial_concentration = le_concentration->text().toDouble();
   if ( partial_concentration <= 0.0 ) return; 

   if ( manualUpdate ) lw_buffer->clear();

   QString s;
   bool    newItem = true;
   int     current = lw_ingredients->currentRow();

   US_BufferComponent std_bc = component_list[ current ];
   
   // Find out if this inredient already exists, otherwise add a new component
   for ( int i = 0; i < buffer.component.size(); i++ ) 
   {
      US_BufferComponent* bc = &buffer.component[ i ];

      if ( std_bc.name == bc->name )
      {
         // Simply update the partial concentration of the existing ingredient
         buffer.concentration[ i ] = partial_concentration;

         s.sprintf( " (%.1f ", partial_concentration );

         lw_buffer->item( i )->setText( std_bc.name + s + std_bc.unit + ")" );
         newItem = false;
         break;
      }
   }

   // Add a new ingredient to this buffer
   if ( newItem ) 
   {
   //   US_BufferComponent component;
      
      buffer.concentration << partial_concentration;
      buffer.component     << std_bc;
      buffer.componentIDs  << std_bc.componentID;
      
      s.sprintf( " (%.1f ", partial_concentration );
      lw_buffer->addItem( std_bc.name + s + std_bc.unit + ")" );
   }

   recalc_density();
   recalc_viscosity();

   le_density      ->setText( QString::number( buffer.density,   'f', 4 ) );
   le_viscosity    ->setText( QString::number( buffer.viscosity, 'f', 4 ) );
   le_concentration->setText( "" );
   
   pb_save->setEnabled( true );
   bufferCurrent = false;
   manualUpdate  = false;
}

/*!  After selection of the buffer component in lw_ingredients, this method will
     display in lb_selected and wait for a partial concentartion input value. */
void US_BufferGui::list_component( void )
{
   int row = lw_ingredients->currentRow();

   lb_selected->setText( lw_ingredients->currentItem()->text() );
   lb_units->setText( tr( "Please enter with units in: " ) + 
         component_list[ row ].unit );
   
   le_concentration->setFocus();
}

/*! When double clicked, the selected item in lw_buffer
    will be removed  and the density and viscosity will be recalculated.  */
void US_BufferGui::remove_component( QListWidgetItem* item )
{
   if ( manualUpdate ) return;
   int row = lw_buffer->row( item );

   buffer.component    .removeAt( row );
   buffer.concentration.removeAt( row );
   buffer.componentIDs .removeAt( row );
   
   recalc_viscosity();
   recalc_density();
   
   le_density  ->setText( QString::number( buffer.density,   'f', 4 ) );
   le_viscosity->setText( QString::number( buffer.viscosity, 'f', 4 ) );
   
   QListWidgetItem* oldItem = lw_buffer->takeItem( row );
   delete oldItem;

   bufferCurrent = false;
}

bool US_BufferGui::up_to_date( void )
{
   if ( ! bufferCurrent                                                 ||
        le_description    ->text()            != buffer.description     ||
        le_compressibility->text().toDouble() != buffer.compressibility ||
        le_ph             ->text().toDouble() != buffer.pH )
      return false;

   return true;
}

void US_BufferGui::accept_buffer( void )
{
   if ( ! up_to_date()  &&  signal )
   {
      int response = QMessageBox::question( this,
            tr( "Buffer changed" ),
            tr( "Changes have not been saved.\n\nContinue?" ),
            QMessageBox::Yes, QMessageBox::Cancel );
      
      if ( response != QMessageBox::Yes ) return;
   }

   if ( signal ) 
   {
      update_buffer();
      buffer.GUID = le_guid->text();
      emit valueChanged ( buffer.density, buffer.viscosity );
      emit valueChanged ( buffer );
      emit valueBufferID( buffer.bufferID );
   }

   accept();
}

/*! Reset some variables to initialization. */
void US_BufferGui::reset( void )
{
   buffer = US_Buffer();

   le_search         ->clear();;
   le_search         ->setReadOnly( false );

   le_guid           ->clear();
                    
   lw_buffer_db      ->clear();
   lw_buffer         ->clear();
                    
   lb_selected       ->setText( "" );
                    
   le_density        ->setText( "0.0" );
                     
   le_viscosity      ->setText( "0.0" );
                     
   le_description    ->clear();
   le_compressibility->clear();
   
   le_ph             ->setText( "7.0" );
                   
   pb_save           ->setEnabled( false );
   pb_update         ->setEnabled( false );
   pb_del            ->setEnabled( false );
                     
   lb_units          ->setText( "" );
   le_concentration  ->clear();

   le_investigator   ->setText( "Not Selected" );

   if ( personID > 0 )
   {
      QString lname;
      QString fname;

      if ( US_Investigator::get_person_info( personID, lname, fname ) )
         assign_investigator( personID, lname, fname );
   }
}

/*!  Recalculate the density of the buffer based on the information in the
     template file */
void US_BufferGui::recalc_density( void )
{
   buffer.density = DENS_20W;

   // Iterate over all components in this buffer
   for ( int i = 0; i < buffer.component.size(); i++ ) 
   {
      US_BufferComponent* bc = &buffer.component[ i ];

      double c1 = buffer.concentration[ i ];
      if ( bc->unit == "mM" ) c1 /= 1000;

      double c2 = c1 * c1; // c1^2
      double c3 = c2 * c1; // c1^3
      double c4 = c3 * c1; // c1^4 

      if ( c1 > 0.0 )
      {
         buffer.density += 
           bc->dens_coeff[ 0 ] +
           bc->dens_coeff[ 1 ] * 1.0e-3 * sqrt( c1 )
         + bc->dens_coeff[ 2 ] * 1.0e-2 * c1
         + bc->dens_coeff[ 3 ] * 1.0e-3 * c2
         + bc->dens_coeff[ 4 ] * 1.0e-4 * c3
         + bc->dens_coeff[ 5 ] * 1.0e-6 * c4
         - DENS_20W;
      }
   }
}

/*!  Recalculate the viscosity of the buffer based on the information in the
     template file */
void US_BufferGui::recalc_viscosity( void )
{
   buffer.viscosity = 100.0 * VISC_20W;

   // Iterate over all components in this buffer
   for ( int i = 0; i < buffer.component.size(); i++) 
   {
      US_BufferComponent* bc = &buffer.component[ i ];

      double c1 = buffer.concentration[ i ];
      if ( bc->unit == "mM" ) c1 /= 1000;

      double c2 = c1 * c1; // c1^2
      double c3 = c2 * c1; // c1^3
      double c4 = c3 * c1; // c1^4 
      
      if ( c1 > 0.0 )
      {
         buffer.viscosity += 
           bc->visc_coeff[ 0 ]  
         + bc->visc_coeff[ 1 ] * 1.0e-3 * sqrt( c1 )
         + bc->visc_coeff[ 2 ] * 1.0e-2 * c1
         + bc->visc_coeff[ 3 ] * 1.0e-3 * c2 
         + bc->visc_coeff[ 4 ] * 1.0e-4 * c3
         + bc->visc_coeff[ 5 ] * 1.0e-6 * c4
         - 100.0 * VISC_20W;
      }
   }
}
