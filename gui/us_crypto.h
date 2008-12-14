//! \file us_crypto.h
#ifndef US_CRYPTO_H
#define US_CRYPTO_H

#include <QtCore>
/*! \brief A class to encrypt and decrypt the user's database passwords.
    
    The class uses the master password and the AES-128 encryption
    algorithm.  The password is automatically retrieved from 
    global memory vi \ref US_Global.  If the master password is not 
    yet in global memory, US_Global prompts the user for it.
*/
class US_Crypto
{
  public:
    //! A null constructor.
    US_Crypto() {};

    //! A null destructor.
    ~US_Crypto(){};

    /*! A static function to encrypt a password string.
        \param plaintext The string to be encrypted.
        \retval encryptionData A list of two strings containing the ciphertext
                               of the password and the initialization vector
                               used during encryption.
    */
    static QStringList encrypt( const QString& );

    /*! A static function to decrypt a string.
        \param ciphertext The string to be encrypted.
        \param initVector An initialization string used when the string was encrypted.
        \retval plaintext The plaintext password.
    */
    static QString     decrypt( const QString&, const QString& );
};

#endif
