//! \file us_license_t.h
#ifndef US_LICENSE_T_H
#define US_LICENSE_T_H

#include <QtCore>

//! \brief A text only class to provide a check to ensure a valid license

class US_License_t
{
  public:
    //! A null constructor. 
    US_License_t()  {};
    //! A null destructor. 
    ~US_License_t() {};

    enum { OK, Expired, Invalid, Missing, BadPlatform, BadOS };

    /*! \brief A static function that retrieves the current UltraScan
               license from \ref US_Settings and checks it for validity.
               The return value is one of the enum values in this class.
    */
    static int isValid( QString&, const QStringList& = QStringList() );

  private:

    static QString encode( const QString&, const QString& );
};

#endif
