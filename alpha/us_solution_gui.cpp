//! \file us_solution_gui.cpp

#include "us_solution_gui.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_passwd.h"
#include "us_db2.h"
#include "us_investigator.h"
#include "us_buffer_gui.h"
#include "us_analyte_gui.h"
#include "us_solution.h"


US_SolutionMgrSelect::US_SolutionMgrSelect( int *invID, int *select_db_disk,
      US_Solution *tmp_solution ) : US_Widgets()
{
   solution    = tmp_solution;
   personID   = invID;
   db_or_disk = select_db_disk;
   from_db    = ( (*db_or_disk) == 1 );
   dbg_level  = US_Settings::us_debug();

   setPalette( US_GuiSettings::frameColor() );
   QGridLayout* main = new QGridLayout( this );
   main->setSpacing( 2 );
   main->setContentsMargins( 2, 2, 2, 2 );

   pb_cancel   = us_pushbutton( tr( "Cancel" ) );
   pb_accept   = us_pushbutton( tr( "Accept" ) );
   pb_spectrum = us_pushbutton( tr( "View Spectrum" ) );
   pb_delete   = us_pushbutton( tr( "Delete Solution" ) );
   pb_help     = us_pushbutton( tr( "Help" ) );

   QLabel* bn_select     = us_banner( tr( "Select a solution to use" ) );
   bn_select->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

   lw_solutions = us_listwidget();
   lw_solutions-> setSortingEnabled( true );
   

   pb_accept->setEnabled( false );
   pb_delete->setEnabled( false );
   //pb_info  ->setEnabled( false );
  
   QLabel* lb_search     = us_label( tr( "Search:" ) );
   le_search             = us_lineedit();

   //QLabel* lb_solutionDesc = us_label( tr( "Solution Name:" ) );
   //le_solutionDesc = us_lineedit( "", 1 );

   lb_amount = us_label( tr( "Analyte Molar Ratio:" ) );
   QLabel* lb_bufferInfo = us_label( tr( "Buffer Name:" ) );
   QLabel* lb_commonVbar20 = us_label( tr( "Common VBar (20C):" ) );
   QLabel* lb_density = us_label( tr( "Buffer density:" ) );
   QLabel* lb_viscosity = us_label( tr( "Buffer viscosity:" ) );
   QLabel* lb_storageTemp = us_label( tr( "Storage Temperature:" ) );
   

   le_amount = us_lineedit();
   us_setReadOnly( le_amount,   true );
   le_bufferInfo = us_lineedit();
   us_setReadOnly( le_bufferInfo,   true );
   le_commonVbar20 = us_lineedit();
   us_setReadOnly( le_commonVbar20,   true );
   le_density = us_lineedit();
   us_setReadOnly( le_density,     true );
   le_viscosity = us_lineedit();
   us_setReadOnly( le_viscosity,     true );
   le_storageTemp = us_lineedit();
   us_setReadOnly( le_storageTemp,     true );
   //connect( le_storageTemp, SIGNAL( textEdited      ( const QString&   ) ),
   //                         SLOT  ( saveTemperature ( const QString&   ) ) );

   te_notes = us_textedit();
   te_notes->setMaximumSize( 600, 100 );
   te_notes->setReadOnly( true );
   //connect( te_notes, SIGNAL( textChanged( void ) ),  SLOT  ( saveNotes  ( void ) ) );
   

   QLabel* lb_banner3 = us_banner( tr( "Current solution contents" )  );
   lb_banner3->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

   QLabel* lb_banner4 = us_banner( tr( "Solution notes" )  );
   lb_banner4->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   
   lw_analytes = us_listwidget();
   lw_analytes-> setSortingEnabled( true );
   connect( lw_analytes, SIGNAL( itemClicked  ( QListWidgetItem* ) ),   // To see Molar ratio for each analyte...
                         SLOT  ( selectAnalyte( QListWidgetItem* ) ) );
   //lw_analytes->setSelectionMode( QAbstractItemView::NoSelection );
   
   int row = 0;
   main->addWidget( bn_select,       row++, 0, 1, 12 );
   main->addWidget( lb_search,       row,   0, 1, 1 );
   main->addWidget( le_search,       row,   1, 1, 5 );
   main->addWidget( pb_help,         row,   6, 1,  2 );
   main->addWidget( pb_cancel,       row,   8, 1,  2 );
   main->addWidget( pb_accept,       row++, 10, 1,  2 );
   main->addWidget( lw_solutions,    row,   0, 8, 6 );
   main->addWidget( pb_spectrum,     row,   6, 1,  3 );
   main->addWidget( pb_delete,       row++, 9, 1,  3 );

   main->addWidget( lb_banner4,      row++,   6, 1, 6 );
   main->addWidget( te_notes,        row++,   6, 1, 6 );
   main->addWidget( lb_banner3,      row++,   6, 1, 6 );
   main->addWidget( lw_analytes,     row++,   6, 4, 6 );

   row += 5;

   main->addWidget( lb_amount,       row,   0, 1, 2 );
   main->addWidget( le_amount,       row,   2, 1, 4 );
   main->addWidget( lb_bufferInfo,   row,   6, 1, 2 );
   main->addWidget( le_bufferInfo,   row++, 8, 1, 4 );
   main->addWidget( lb_commonVbar20, row,   0, 1, 2 );
   main->addWidget( le_commonVbar20, row,   2, 1, 4 );
   main->addWidget( lb_density,      row,   6, 1, 2 );
   main->addWidget( le_density,      row++, 8, 1, 4 );
   main->addWidget( lb_viscosity,    row,   0, 1, 2 );
   main->addWidget( le_viscosity,    row,   2, 1, 4 );
   main->addWidget( lb_storageTemp,  row,   6, 1, 2);
   main->addWidget( le_storageTemp,  row,   8, 1, 4);
   
   //connect( lw_analytes, SIGNAL( itemClicked  ( QListWidgetItem* ) ),
   //                      SLOT  ( selectAnalyte( QListWidgetItem* ) ) );
   //connect( lw_analytes, SIGNAL( itemDoubleClicked  ( QListWidgetItem* ) ),
   //                      SLOT  ( changeAnalyte      ( QListWidgetItem* ) ) );

   connect( le_search,      SIGNAL( textChanged( const QString& ) ),
	    this,           SLOT  ( search     ( const QString& ) ) );
   connect( lw_solutions, SIGNAL  ( itemClicked    ( QListWidgetItem* ) ),
   	                    SLOT  ( selectSolution ( QListWidgetItem* ) ) );
   connect( pb_help,        SIGNAL( clicked() ),
	    this,           SLOT  ( help()    ) );
   connect( pb_cancel,      SIGNAL( clicked() ),
            this,           SLOT  ( reject()  ) );
   connect( pb_accept,      SIGNAL( clicked()       ),
	    this,           SLOT  ( accept_solution() ) );
   connect( pb_delete,      SIGNAL( clicked() ), 
	                    SLOT  ( delete_solution() ) );
   connect( pb_spectrum,    SIGNAL( clicked()  ),
            this,           SLOT  ( spectrum() ) );
       
   // Load the solution descriptions
   search("");
   qDebug() << "Before load: ";
   load();

   // Select the current one if we know what it is
   if ( solution->solutionID > 0 )
   {
      QList< QListWidgetItem* > items 
        = lw_solutions->findItems( solution->solutionDesc, Qt::MatchExactly );

      // should be exactly 1, but let's make sure
      if ( items.size() == 1 )
      {
         selectSolution( items[ 0 ] );
         lw_solutions->setCurrentItem( items[ 0 ] );
      }
   }
   

}

// View Spectrum in Analyte Select
US_SolutionViewSpectrum::US_SolutionViewSpectrum(QMap<double,double>& solution_temp) : US_Widgets()
{
  solution = solution_temp;
  
  data_plot = new QwtPlot();
  //changedCurve = NULL;
  plotLayout = new US_Plot(data_plot, tr(""), tr("Wavelength(nm)"), tr(""));
  data_plot->setCanvasBackground(Qt::black);
  data_plot->setTitle("Extinction Profile");
  data_plot->setMinimumSize(560, 240);
  //data_plot->enableAxis(1, true);
  data_plot->setAxisTitle(0, "Extinction OD/(mol*cm)");

  us_grid(data_plot);
   
  QGridLayout* main;
  main = new QGridLayout(this);
  main->setSpacing(2);
  //main->setContentsMargins(2,2,2,2);
  main->addLayout(plotLayout, 0, 1);

  plot_extinction();

}

void US_SolutionViewSpectrum::plot_extinction()
{ 
  QVector <double> x;
  QVector <double> y;
  
  QMap<double, double>::iterator it;
  
  for (it = solution.begin(); it != solution.end(); ++it) {
    x.push_back(it.key());
    y.push_back(it.value());
  }
  
  QwtSymbol* symbol = new QwtSymbol;
  symbol->setSize(10);
  symbol->setPen(QPen(Qt::blue));
  symbol->setBrush(Qt::yellow);
  symbol->setStyle(QwtSymbol::Ellipse);
  
  QwtPlotCurve *spectrum;
  spectrum = us_curve(data_plot, "Spectrum Data");
  spectrum->setSymbol(symbol);    
  spectrum->setSamples( x.data(), y.data(), (int) x.size() );
  data_plot->replot();
}

// Display a spectrum dialog for list/manage
void US_SolutionMgrSelect::spectrum( void )
{
  qDebug() << solution->extinction;
  
  if (solution->extinction.isEmpty())
    {
      QMessageBox::information( this,
      tr( "WARNING" ),
      tr( "Solution does not have spectrum data!" ) );
    }
  else
    {
      US_SolutionViewSpectrum *w = new US_SolutionViewSpectrum(solution->extinction);
      w->setParent(this, Qt::Window);
      w->show();
    }
}


// Function to delete a solution from disk, db, or in the current form
void US_SolutionMgrSelect::delete_solution( void )
{
   int status = US_DB2::OK;

   if ( from_db )
   {
      US_Passwd pw;
      QString masterPW = pw.getPasswd();
      US_DB2 db( masterPW );
   
      if ( db.lastErrno() != US_DB2::OK )
      {
         db_error( db.lastError() );
         return;
      }

      status = solution->deleteFromDB( &db );
   }

   else
      status = solution->deleteFromDisk();

   if ( status == US_DB2::SOLUT_IN_USE )
   {
      QMessageBox::warning( this,
         tr( "Delete aborted" ),
         tr( "Solution NOT Deleted, since it is in use\n"
             "by one or more experiments" ) );
      return;
   }

   solution->clear();
   analyteMap.clear();
   load();
   reset();

   QMessageBox::information( this,
         tr( "Delete results" ),
         tr( "Solution Deleted" ) );
   //changed = true;
}

// Function to load solutions into solutions list widget
void US_SolutionMgrSelect::load( void )
{
   from_db       = ( (*db_or_disk) == 1 );
   
   /*  In Buffers/Analytes - skipped here ... */
   QString sguid = solution->solutionGUID;
   int solid = solution->solutionID;
   //int idSol     = solid.toInt();

   if ( ( from_db  &&  solid < 0 )  ||
        ( !from_db  &&  sguid.isEmpty() ) )
   {
      lw_solutions->setCurrentRow( -1 );
   }
   /* */
   
   if ( from_db )
      loadDB();

   else
      loadDisk();

   if ( ! sguid.isEmpty() )
   {  // There is a selected analyte, select a list item
      if ( from_db ) // DB access
      {
         // Search for analyteID
         for ( int ii = 0; ii < info.size(); ii++ )
         {
            if ( solid == info[ ii ].solutionID )
            {
	      //DbgLv(1) << "agS-initb:  ii" << ii << "match anaID"<< analyte->analyteID;
              qDebug() <<  "agS-initb:  ii" << ii << "match SolID"<< solution->solutionID;
	      lw_solutions->setCurrentRow( ii );
               QListWidgetItem* item = lw_solutions->item( ii );
               //select_analyte( item );
	       selectSolution( item );
               break;
            }
         }
      }

      else            // Disk access
      {
         // Search for GUID
         for ( int ii = 0; ii < info.size(); ii++ )
         {
            if ( sguid == info[ ii ].GUID )
            {
               lw_solutions->setCurrentRow( ii );
               QListWidgetItem* item = lw_solutions->item( ii );
               //select_analyte( item );
               selectSolution( item );
	       break;
            }
         }
      }
   }
   else
   {  // There is no selected analyte, de-select a list item
      lw_solutions->setCurrentRow( -1 );

      reset();
   }   

}

// Function to load solutions from disk
void US_SolutionMgrSelect::loadDisk( void )
{
   QString path;
   if ( ! solution->diskPath( path ) ) return;

   IDs.clear();
   descriptions.clear();
   GUIDs.clear();
   filenames.clear();
   le_search->  clear();
   le_search->  setReadOnly( true );

   QDir dir( path );
   QStringList filter( "S*.xml" );
   QStringList names = dir.entryList( filter, QDir::Files, QDir::Name );

   QFile a_file;

   for ( int i = 0; i < names.size(); i++ )
   {
      a_file.setFileName( path + "/" + names[ i ] );

      if ( ! a_file.open( QIODevice::ReadOnly | QIODevice::Text) ) continue;

      QXmlStreamReader xml( &a_file );

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement() )
         {
            if ( xml.name() == "solution" )
            {
               QXmlStreamAttributes a = xml.attributes();

               IDs          << a.value( "id" ).toString();
               GUIDs        << a.value( "guid" ).toString();
               filenames    << path + "/" + names[ i ];

            }

            else if ( xml.name() == "description" )
            {
               xml.readNext();
               descriptions << xml.text().toString();
            }
         }
      }

      a_file.close();
   }

   // New from Analytes ///////////////
   lw_solutions->clear();
   if ( descriptions.size() == 0 )
     lw_solutions->addItem( "No analyte files found." );
   else
   {
      le_search->setReadOnly( false );
      search();
   }
   //////////////////////////////////////

   //loadSolutions();
}

// Function to load solutions from db
void US_SolutionMgrSelect::loadDB( void )
{
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() != US_DB2::OK )
   {
      db_error( db.lastError() );
      return;
   }

   if ( investigatorID < 1 ) investigatorID = US_Settings::us_inv_ID();

   QStringList q( "all_solutionIDs" );
   q << QString::number( investigatorID );
   db.query( q );

   if ( db.lastErrno() != US_DB2::OK ) return;

   IDs.clear();
   descriptions.clear();
   GUIDs.clear();
   filenames.clear();


   qDebug() << "load in DB:";

   le_search->  clear();
   le_search->  setText( "" );
   le_search->  setReadOnly( true );

   while ( db.next() )
   {
      QString newID = db.value( 0 ).toString();
      IDs          << newID;
      descriptions << db.value( 1 ).toString();
      GUIDs        << QString( "" );
      filenames    << QString( "" );
   }

   // NEW from Analyte ////
   lw_solutions->clear();
   if ( descriptions.size() == 0 )
   {
      lw_solutions->addItem( "No analyte files found." );
   }
   else
   {
      le_search->setReadOnly( false );
      qDebug() << "load in DB: before search";
      search();
   }
   // ///////////////////////////

   //loadSolutions();
}



//  will search solutions matching search string
 void US_SolutionMgrSelect::search( QString const& text )
 {
    QString sep = ";";
    QStringList sortdesc;
    lw_solutions  ->clear();
    info.clear();
    solutionMap.clear();
 
    sortdesc       .clear();
    int dsize   = descriptions.size();    

    for ( int ii = 0; ii < descriptions.size(); ii++ )
    {  // get list of filtered-description + index strings
       if ( descriptions[ ii ].contains(
          QRegExp( ".*" + text + ".*", Qt::CaseInsensitive ) )  &&
          ! descriptions[ ii].isEmpty() )
       {
          sortdesc << descriptions[ ii ] + sep + QString::number( ii );
       }
    }

    // sort the descriptions
    sortdesc.sort();

    qDebug() << "Sol-search:  descsize" << dsize  << "sortsize" << sortdesc.size();

 // DbgLv(1) << "BufS-search:  descsize" << dsize
 //  << "sortsize" << sortdesc.size();

    for ( int jj = 0; jj < sortdesc.size(); jj++ )
    {  // build list of sorted meta data and ListWidget entries
      
      int ii      = sortdesc[ jj ].section( sep, 1, 1 ).toInt();
      //int ii = jj;
      
      if ( ii < 0  ||  ii >= dsize )
       {
 DbgLv(1) << "BufS-search:  *ERROR* ii" << ii << "jj" << jj
  << "sdesc" << sortdesc[jj].section(sep,0,0);
          continue;
       }

       qDebug() << "search 11";
       SolutionInfo si;
       si.index       = ii;
       si.description = descriptions[ ii ];
       si.GUID        = GUIDs       [ ii ];
       si.filename    = filenames   [ ii ];
       qDebug() << "search 11a";
       si.solutionID    = IDs   [ ii ].toInt();
              
       qDebug() << "search 11aaa";
       
       info << si;

       qDebug() << "search 22";

       QListWidgetItem* item = new QListWidgetItem( descriptions[ ii ], lw_solutions );
       solutionMap[ item ] = ii;
       
       qDebug() << "Search ii(ndx): " << ii;
       lw_solutions->addItem( item );
       //lw_solutions->addItem( si.description );
    }
 }



/*
// Function to load the solutions list widget from the solutions data structure
void US_SolutionMgrSelect::loadSolutions( void )
{
   lw_solutions->clear();
   info.clear();
   solutionMap.clear();

   for ( int i = 0; i < descriptions.size(); i++ )
   {
      SolutionInfo si;
      si.solutionID  = IDs         [ i ].toInt();
      si.description = descriptions[ i ];
      si.GUID        = GUIDs       [ i ];
      si.filename    = filenames   [ i ];
      si.index       = i;
      info << si;

      // Create a map to account for automatic sorting of the list
      QListWidgetItem* item = new QListWidgetItem( descriptions[ i ], lw_solutions );
      solutionMap[ item ] = i;

      lw_solutions->addItem( item );
   }

}
*/

// Function to display an error returned from the database
void US_SolutionMgrSelect::db_error( const QString& error )
{
   QMessageBox::warning( this, tr( "Database Problem" ),
         tr( "Database returned the following error: \n" ) + error );
}



// Reject the selected solutiom and return to caller with no change
void US_SolutionMgrSelect::reject( void )
{
   emit selectionCanceled();
}
// Accept the currently selected solutiom
void US_SolutionMgrSelect::accept_solution( void )
{
   emit solutionAccepted();
}


// Function to handle when analyte listwidget item is selected
void US_SolutionMgrSelect::selectSolution( QListWidgetItem* item )
{
  int     solutionID = 0;
  QString solutionGUID = "";
  qDebug() << "selecSol item: ";
   // Account for the fact that the list has been sorted
   int     ndx          = solutionMap[ item ];
   qDebug() << "selecSol 2  ndx: " << ndx;
   
   // Iterate over 'info' and check for info[i].index - should be equal to ndx //
   for (int i = 0; i < info.size(); ++i)
     {
       qDebug() << "INFO_index: "<< info[i].index;
       if (ndx == info[i].index)
	 {
	   solutionID   = info[ i ].solutionID;
	   solutionGUID = info[ i ].GUID;
	   break;
	 }
     }

   //int     solutionID   = info[ ndx ].solutionID;
   //QString solutionGUID = info[ ndx ].GUID;
   
   solution->clear();

   qDebug() << "SolutionID: " << solutionID;
   
   int status = US_DB2::OK;

   if ( from_db )
   {
      US_Passwd pw;
      QString masterPW = pw.getPasswd();
      US_DB2 db( masterPW );
   
      if ( db.lastErrno() != US_DB2::OK )
      {
         db_error( db.lastError() );
         return;
      }

      status = solution->readFromDB  ( solutionID, &db );

      // Error reporting
      if ( status == US_DB2::NO_BUFFER )
      {
         QMessageBox::information( this,
               tr( "Attention" ),
               tr( "The buffer this solution refers to was not found.\n"
                   "Please restore and try again.\n" ) );
      }
      
      else if ( status == US_DB2::NO_ANALYTE )
      {
         QMessageBox::information( this,
               tr( "Attention" ),
               tr( "One of the analytes this solution refers to was not found.\n"
                   "Please restore and try again.\n" ) );
      }
      
      else if ( status != US_DB2::OK )
         db_error( db.lastError() );
      
   }

   else
   {
      status = solution->readFromDisk( solutionGUID );

      // Error reporting
      if ( status == US_DB2::NO_BUFFER )
      {
         QMessageBox::information( this,
               tr( "Attention" ),
               tr( "The buffer this solution refers to was not found.\n"
                   "Please restore and try again.\n" ) );
      }
      
      else if ( status == US_DB2::NO_ANALYTE )
      {
         QMessageBox::information( this,
               tr( "Attention" ),
               tr( "One of the analytes this solution refers to was not found.\n"
                   "Please restore and try again.\n" ) );
      }
      
      else if ( status != US_DB2::OK )
      {
         QMessageBox::information( this, 
               tr( "Disk Read Problem" ), 
               tr( "Could not read data from the disk.\n" 
                   "Disk status: " ) + QString::number( status ) ); 
      }
   }

   reset();
   changed = false;
}

// Function to refresh the display with values from the solution structure,
//  and to enable/disable features
void US_SolutionMgrSelect::reset( void )
{

   qDebug() << "We are at RESET: ";
   QList< US_Solution::AnalyteInfo >&   ai         = solution->analyteInfo;
   QString                              bufferDesc = solution->buffer.description;

   le_bufferInfo   -> setText( solution->buffer.description );
   //le_solutionDesc -> setText( solution->solutionDesc );
   le_commonVbar20 -> setText( QString::number( solution->commonVbar20 ) );
   le_density      -> setText( QString::number( solution->buffer.density ) );
   le_viscosity    -> setText( QString::number( solution->buffer.viscosity ) );
   le_storageTemp  -> setText( QString::number( solution->storageTemp  ) );
   te_notes        -> setText( solution->notes        );

   le_amount       -> setText( "1" );
   //le_guid         -> setText( solution->solutionGUID );

   //pb_buffer       -> setEnabled( true );

   //pb_addAnalyte   -> setEnabled( true );
   //pb_removeAnalyte-> setEnabled( false );

   // Let's calculate if we're eligible to save this solution
   //pb_save         -> setEnabled( false );

   qDebug() << "We are at RESET_1: ";

   if ( ! bufferDesc.isEmpty() ) //we can have a solution with buffer only
   {
     //pb_save      -> setEnabled( true );
   }

   // We can always delete something, even if it's just what's in the dialog
   pb_delete          -> setEnabled( false );
   if ( lw_solutions->currentRow() != -1 )
   {
      pb_delete       -> setEnabled( true );
   }

   // Display analytes that have been selected
   lw_analytes->clear();
   analyteMap.clear();
   for ( int i = 0; i < ai.size(); i++ )
   {
      // Create a map to account for sorting of the list
      QListWidgetItem* item = new QListWidgetItem( ai[ i ].analyte.description, lw_analytes );
      analyteMap[ item ] = i;

      lw_analytes->addItem( item );
   }

   // Turn the red label back
   //QPalette p = lb_amount->palette();
   //p.setColor( QPalette::WindowText, Qt::white );
   //lb_amount->setPalette( p );

   // Figure out if the accept button should be enabled
   if ( ! signal )      // Then it's just a close button
      pb_accept->setEnabled( true );

   else if ( solution->saveStatus == US_Solution::BOTH )
      pb_accept->setEnabled( true );

   // else if ( ( ! disk_controls->db() ) && solution->saveStatus == US_Solution::HD_ONLY )
   else if ( ( ! from_db ) && solution->saveStatus == US_Solution::HD_ONLY )
      pb_accept->setEnabled( true );

   //else if ( (   disk_controls->db() ) && solution->saveStatus == US_Solution::DB_ONLY )
   else if ( (  from_db ) && solution->saveStatus == US_Solution::DB_ONLY )
     pb_accept->setEnabled( true );

   else
      pb_accept->setEnabled( false );

   // // Display investigator
   // investigatorID = US_Settings::us_inv_ID();

   // QString number = ( investigatorID > 0 ) 
   //    ? QString::number( investigatorID ) + ": " 
   //    : "";

   // le_investigator->setText( number +  US_Settings::us_inv_name() );
}

// Function to handle when solution listwidget item is selected
void US_SolutionMgrSelect::selectAnalyte( QListWidgetItem* item )
{
   // Get the right index in the sorted list, and load the amount
   int ndx = analyteMap[ item ];
   le_amount ->setText( QString::number( solution->analyteInfo[ ndx ].amount ) );
   
   QPalette p = lb_amount->palette();
   p.setColor( QPalette::WindowText, Qt::red );
   lb_amount->setPalette( p );
}


 

US_SolutionMgrNew::US_SolutionMgrNew( int *invID, int *select_db_disk,
      US_Solution *tmp_solution ) : US_Widgets()
{
   solution     = tmp_solution;
   personID   = invID;
   db_or_disk = select_db_disk;
   from_db    = ( (*db_or_disk) == 1 );
   dbg_level  = US_Settings::us_debug();

   setPalette( US_GuiSettings::frameColor() );
   QGridLayout* main = new QGridLayout( this );
   main->setSpacing( 2 );
   main->setContentsMargins( 2, 2, 2, 2 );

   QLabel* bn_newsolution    = us_banner( tr( "Create New Solution" ) );
   bn_newsolution  ->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

   lb_descrip   = us_label( tr( "Solution Name:" ) );
   le_descrip   = us_lineedit( "New Solution", 0, false );
   //connect( le_descrip, SIGNAL( editingFinished   () ), 
   //                         SLOT  ( new_description() ) );

   lb_bufferInfo   = us_label( tr( "Buffer Name:" ) );
   le_bufferInfo = us_lineedit();
   us_setReadOnly( le_bufferInfo,   true );
      
   pb_analyte = us_pushbutton( tr( "Add Analyte" ) );
   pb_buffer  = us_pushbutton( tr( "Select Buffer" ) );

   pb_removeAnalyte = us_pushbutton( tr( "Remove Analyte" ), false );
   //connect( pb_removeAnalyte, SIGNAL( clicked() ), SLOT( removeAnalyte() ) );

   pb_spectrum = us_pushbutton( tr( "Manage Spectrum" ) );

   pb_cancel   = us_pushbutton( tr( "Cancel" ) );
   // connect( pb_cancel,   SIGNAL( clicked()     ),
   //          this,        SLOT  ( newCanceled() ) );

   pb_reset   = us_pushbutton( tr( "Reset" ) );
   // connect( pb_reset,   SIGNAL( clicked()     ),
   //          this,        SLOT  ( reset() ) );
   
   //pb_save     = us_pushbutton( tr( "Accept" ), false);
   //connect( pb_save, SIGNAL( clicked() ), SLOT( save() ) );
   
   pb_accept     = us_pushbutton( tr( "Accept" ), false);
   // connect( pb_accept,   SIGNAL( clicked()     ),
   //          this,        SLOT  ( newAccepted() ) );


   QHBoxLayout* lo_amount = new QHBoxLayout();

   lb_amount = us_label( tr( "Analyte Molar Ratio:" ) );
   lo_amount->addWidget( lb_amount );

   ct_amount = us_counter ( 2, 0, 100, 1 ); // #buttons, low, high, start_value
   ct_amount->setSingleStep( 1 );
   ct_amount->setFont( QFont( US_GuiSettings::fontFamily(),
                              US_GuiSettings::fontSize() ) );
   lo_amount->addWidget( ct_amount );
   

   QLabel* lb_banner3 = us_banner( tr( "Current solution contents" )  );
   lb_banner3->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   lw_analytes = us_listwidget();
   lw_analytes-> setSortingEnabled( true );
   //connect( lw_analytes, SIGNAL( itemClicked  ( QListWidgetItem* ) ),
   //                      SLOT  ( selectAnalyte( QListWidgetItem* ) ) );
   //connect( lw_analytes, SIGNAL( itemDoubleClicked  ( QListWidgetItem* ) ),
   //                      SLOT  ( changeAnalyte      ( QListWidgetItem* ) ) );


   QLabel* lb_commonVbar20 = us_label( tr( "Common VBar (20C):" ) );
   QLabel* lb_density = us_label( tr( "Buffer density:" ) );
   QLabel* lb_viscosity = us_label( tr( "Buffer viscosity:" ) );
   QLabel* lb_storageTemp = us_label( tr( "Storage Temperature:" ) );
   

   le_commonVbar20 = us_lineedit();
   us_setReadOnly( le_commonVbar20,   true );
   le_density = us_lineedit();
   us_setReadOnly( le_density,     true );
   le_viscosity = us_lineedit();
   us_setReadOnly( le_viscosity,     true );
   le_storageTemp = us_lineedit();
   //connect( le_storageTemp, SIGNAL( textEdited      ( const QString&   ) ),
   //                         SLOT  ( saveTemperature ( const QString&   ) ) );
 
   te_notes = us_textedit();
   //te_notes->setMaximumSize( 600, 100 );
   te_notes->setReadOnly( false );
   //connect( te_notes, SIGNAL( textChanged( void ) ),  SLOT  ( saveNotes  ( void ) ) );
   
   QLabel* lb_banner1 = us_banner( tr( "Solution notes" )  );
   lb_banner1->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

   int row = 0;
   main->addWidget( bn_newsolution,  row++,  0, 1, 12 );
   main->addWidget( lb_descrip,      row,    0, 1,  4 );
   main->addWidget( le_descrip,      row++,  4, 1,  8 );
   
   main->addWidget( lb_bufferInfo,   row,    0, 1,  2 );
   main->addWidget( le_bufferInfo,   row,    2, 1,  4 );
      
   main->addWidget( pb_analyte,      row,    6, 1,  3 );
   main->addWidget( pb_buffer,       row++,  9, 1,  3 );
   main->addLayout( lo_amount,       row,    0, 1,  6 );

   main->addWidget( pb_removeAnalyte,  row,    6, 1,  3 );
   main->addWidget( pb_spectrum,       row++,  9, 1,  3 );

   main->addWidget( lb_banner3,      row,    0, 1, 6 );

   main->addWidget( pb_cancel,       row,    6, 1,  2 );
   main->addWidget( pb_reset,        row,    8, 1,  2 );
   main->addWidget( pb_accept,       row++, 10, 1,  2 );
   
   main->addWidget( lw_analytes,     row,     0, 4, 6 );
   main->addWidget( lb_banner1,      row++,   6, 1, 6 );
   main->addWidget( te_notes,        row++,   6, 3, 6 );

   
   row += 7;
   
   main->addWidget( lb_commonVbar20, row,   0, 1, 2 );
   main->addWidget( le_commonVbar20, row,   2, 1, 4 );
   main->addWidget( lb_density,      row,   6, 1, 2 );
   main->addWidget( le_density,      row++, 8, 1, 4 );
   main->addWidget( lb_viscosity,    row,   0, 1, 2 );
   main->addWidget( le_viscosity,    row,   2, 1, 4 );
   main->addWidget( lb_storageTemp,  row,   6, 1, 2);
   main->addWidget( le_storageTemp,  row,   8, 1, 4);  

   //main->setRowStretch( 5, 5 );

 }

// Edit Existing Solution panel
US_SolutionMgrEdit::US_SolutionMgrEdit( int *invID, int *select_db_disk,
      US_Solution *tmp_solution ) : US_Widgets()
{
  
   solution      = tmp_solution;
   orig_solution = *solution;
   personID    = invID;
   db_or_disk  = select_db_disk;
   from_db     = ( (*db_or_disk) == 1 );
   dbg_level   = US_Settings::us_debug();

   QGridLayout* main = new QGridLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   QPushButton* pb_cancel   = us_pushbutton( tr( "Cancel" ) );
   pb_accept                = us_pushbutton( tr( "Accept" ) );
   QPushButton* pb_spectrum = us_pushbutton( tr( "Manage Spectrum" ) );
   QPushButton* pb_help     = us_pushbutton( tr( "Help" ) );
   QLabel* bn_modana        = us_banner( tr( "Edit an existing analyte" ) );
   QLabel* lb_descrip       = us_label( tr( "Description:" ) );
   le_descrip    = us_lineedit( solution-> solutionDesc );
   us_setReadOnly( le_descrip, true );

   QLabel* lb_storageTemp = us_label( tr( "Storage Temperature:" ) );
   le_storageTemp = us_lineedit();

   te_notes = us_textedit();
   //te_notes->setMaximumSize( 600, 100 );
   te_notes->setReadOnly( false );
   //connect( te_notes, SIGNAL( textChanged( void ) ),  SLOT  ( saveNotes  ( void ) ) );
   QLabel* lb_banner4 = us_banner( tr( "Solution comments" )  );
   lb_banner4->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

   int row = 0;
   main->addWidget( bn_modana,       row++, 0, 1, 12 );
   main->addWidget( lb_descrip,      row,   0, 1, 4 );
   main->addWidget( le_descrip,      row++, 4, 1, 8 );

   main->addWidget( lb_banner4,      row,   0, 1, 6 );
   main->addWidget( lb_storageTemp,  row,   6, 1, 2 );
   main->addWidget( le_storageTemp,  row++, 8, 1, 4 );

   main->addWidget( te_notes,        row,   0, 2, 6 );

   main->addWidget( pb_cancel,       row,   6, 1, 3 );
   main->addWidget( pb_accept,       row++, 9, 1, 3 );
   
   main->addWidget( pb_spectrum,     row,   6, 1, 3 );
   main->addWidget( pb_help,         row,   9, 1, 3 );
   
   row += 8;

   QLabel *empty = us_banner ("");
   main->addWidget( empty,           row,   0, 9, 12);
   

}

// Settings panel
US_SolutionMgrSettings::US_SolutionMgrSettings( int *invID, int *select_db_disk )
   : US_Widgets()
{
   personID   = invID;
   db_or_disk = select_db_disk;
   from_db    = ( (*db_or_disk) == 1 );
   dbg_level  = US_Settings::us_debug();

   QGridLayout* main = new QGridLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

      QStringList DB = US_Settings::defaultDB();
   if ( DB.isEmpty() ) DB << "Undefined";

   QLabel* lb_DB  = us_banner( tr( "Database: " ) + DB.at( 0 ) );
   QPushButton* pb_investigator = us_pushbutton( tr( "Select Investigator" ) );
   QPushButton* pb_help         = us_pushbutton( tr( "Help" ) );

   if ( US_Settings::us_inv_level() < 3 )
      pb_investigator->setEnabled( false );

   QString number  = ( (*personID) > 0 )
      ? QString::number( US_Settings::us_inv_ID() ) + ": "
      : "";

   le_investigator = us_lineedit( number + US_Settings::us_inv_name() );
   int idb_or_disk = from_db ? US_Disk_DB_Controls::DB
                             : US_Disk_DB_Controls::Disk;
   disk_controls   = new US_Disk_DB_Controls( idb_or_disk );
   QLabel *empty   = us_banner ("");

   lb_DB      ->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   us_setReadOnly( le_investigator, true );


   int row = 0;
   main->addWidget( lb_DB,           row++, 0, 1, 4 );
   main->addWidget( pb_investigator, row,   0, 1, 1 );
   main->addWidget( le_investigator, row++, 1, 1, 3 );
   main->addLayout( disk_controls,   row,   0, 1, 3 );
   main->addWidget( pb_help,         row++, 3, 1, 1 );
   main->addWidget( empty,           row,   0, 6, 4 );

   connect( disk_controls,   SIGNAL( changed   ( bool ) ),
            this,            SLOT  ( db_changed( bool ) ) );
   connect( pb_investigator, SIGNAL( clicked()          ),
            this,            SLOT(   sel_investigator() ) );
   connect( pb_help,         SIGNAL( clicked() ),
            this,            SLOT  ( help()    ) );

}
// Select a new investigator
void US_SolutionMgrSettings::sel_investigator( void )
{
   US_Investigator* inv_dialog = new US_Investigator( true, (*personID) );

   connect( inv_dialog,
            SIGNAL( investigator_accepted( int ) ),
            SLOT  ( assign_investigator  ( int ) ) );

   inv_dialog->exec();
}

// Note and notify of change in db/disk source
void US_SolutionMgrSettings::db_changed( bool db )
{
   emit use_db( db );
   // calling class needs to query DB/disk when signal is emitted
   qApp->processEvents();
}

// Assign an investigator after a change
void US_SolutionMgrSettings::assign_investigator( int invID )
{
   (*personID) = invID;

   QString number = ( (*personID) > 0 )
   ? QString::number( invID ) + ": "
   : "";

   le_investigator->setText( number + US_Settings::us_inv_name() );
   emit investigator_changed( invID );
}



// Main Solution window with panels
US_SolutionGui::US_SolutionGui( 
      int   expID,
      int   chID,
      bool  signal_wanted,
      int   select_db_disk,
      const US_Solution& dataIn,
      bool  auto_save
      ) : US_WidgetsDialog( 0, 0 ), experimentID( expID ), channelID( chID ),
        signal( signal_wanted ), solution( dataIn ), autosave( auto_save )

{
   personID     = US_Settings::us_inv_ID();
   solution      = dataIn;
   orig_solution = dataIn;
   disk_or_db  = ( select_db_disk == US_Disk_DB_Controls::Default )
                 ?  US_Settings::default_data_location()
                 : select_db_disk;
   dbg_level    = US_Settings::us_debug();

   
   setWindowTitle( tr( "Solution Management" ) );
   setPalette( US_GuiSettings::frameColor() );
   this->setMinimumSize( 640, 480 );

   QGridLayout* main = new QGridLayout(this);
   main->setSpacing(2);
   main->setContentsMargins(2, 2, 2, 2);

   tabWidget   = us_tabwidget();
   selectTab   = new US_SolutionMgrSelect  ( &personID, &disk_or_db, &solution );
   newTab      = new US_SolutionMgrNew     ( &personID, &disk_or_db, &solution );
   editTab     = new US_SolutionMgrEdit    ( &personID, &disk_or_db, &solution );
   settingsTab = new US_SolutionMgrSettings( &personID, &disk_or_db );
   tabWidget -> addTab( selectTab,   tr( "Select Solution" ) );
   tabWidget -> addTab( newTab,      tr( "Enter New Solution" ) );
   tabWidget -> addTab( editTab,     tr( "Edit Existing Solution" ) );
   tabWidget -> addTab( settingsTab, tr( "Settings" ) );

   main->addWidget( tabWidget );

   connect( tabWidget,   SIGNAL( currentChanged(       int  ) ),
            this,        SLOT (  checkTab(             int  ) ) );
   connect( selectTab,   SIGNAL( solutionAccepted(      void ) ),
            this,        SLOT (  solutionAccepted(      void ) ) );
   connect( selectTab,   SIGNAL( selectionCanceled(    void ) ),
            this,        SLOT (  solutionRejected(      void ) ) );
   // connect( newTab,      SIGNAL( newAnaAccepted(       void ) ),
   //          this,        SLOT (  newAnaAccepted(       void ) ) );
   // connect( newTab,      SIGNAL( newAnaCanceled(       void ) ),
   //          this,        SLOT (  newAnaCanceled(       void ) ) );
   // connect( editTab,     SIGNAL( editAnaAccepted(      void ) ),
   //          this,        SLOT (  editAnaAccepted(      void ) ) );
   // connect( editTab,     SIGNAL( editAnaCanceled(      void ) ),
   //          this,        SLOT (  editAnaCanceled(      void ) ) );
   // connect( settingsTab, SIGNAL( use_db(               bool ) ),
   //          this,        SLOT (  update_disk_or_db(    bool ) ) );
   // connect( settingsTab, SIGNAL( investigator_changed( int  ) ),
   //          this,        SLOT (  update_personID(      int  ) ) );
}

void US_SolutionGui::value_changed( const QString& )
{
   // This only is activated by changes to vbar20
   // (either protein or dna/rna) but vbar is not saved.
#if 0
   temp_changed( le_protein_temp->text() );
#endif
}

// React to a change in panel
void US_SolutionGui::checkTab( int currentTab )
{
DbgLv(1) << "ckTab: currTab" << currentTab;
   // Need to re-read the database or disk or to make other adjustments
   // in case relevant changes were made elsewhere
   if ( currentTab == 0 )
   {
DbgLv(1) << "ckTab:   selectTab  init_solution";
//selectTab  ->init_solution();
   }
   else if ( currentTab == 1 )
   {
DbgLv(1) << "ckTab:   newTab     init_solution";
//newTab     ->init_solution();
   }
   else if ( currentTab == 2 )
   {
DbgLv(1) << "ckTab:   editTab    init_solution";
//editTab    ->init_solution();
   }
}

// Make global setting for change in db/disk in Settings panel
void US_SolutionGui::update_disk_or_db( bool choice )
{
   (choice) ? (disk_or_db = 1 ) : (disk_or_db = 0 );
}

// Global person ID after Settings panel change
void US_SolutionGui::update_personID( int ID )
{
   personID = ID;
}

// Slot for Edit panel solution-accepted
void US_SolutionGui::editAnaAccepted( void )
{
DbgLv(1) << "main: editAnaAccepted";
   tabWidget->setCurrentIndex( 0 );
}

// Slot for Edit panel solution-changes-rejected
void US_SolutionGui::editAnaCanceled( void )
{
DbgLv(1) << "main: editAnaCanceled";
   tabWidget->setCurrentIndex( 0 );
}

// Slot for New panel solution accepted
void US_SolutionGui::newAnaAccepted( void )
{
DbgLv(1) << "main: newAnaAccepted";
   tabWidget->setCurrentIndex( 0 );
}

// Slot for New panel solution add rejected
void US_SolutionGui::newAnaCanceled( void )
{
DbgLv(1) << "main: newAnaCanceled";
   tabWidget->setCurrentIndex( 0 );
}

// Exit and signal caller that changes and selected were accepted
void US_SolutionGui::solutionAccepted( void )
{
#if 0
   valueChanged      ( solution.density, solution.viscosity );
#endif
   emit valueChanged  ( solution );
   emit valueSolutionID( solution.solutionID );

   accept();
}

// Exit and signal caller that solution selection/changes were rejected
void US_SolutionGui::solutionRejected( void )
{
   solution   = orig_solution;

   reject();
}

