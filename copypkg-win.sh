#!/usr/bin/bash
#   copypkg-win   - copy files for packaging - Windows

SRCDIR=/cygdrive/c/Users/Admin/Documents/ultrascan3
US2DIR=/cygdrive/c/Users/Admin/Documents/ultrascan2
QTBIN=/cygdrive/c/Qt/4.7.3/bin
DESTDIR=/cygdrive/c/dist
SOMOBINS="us3_somo.exe us3_config.exe rasmol.exe \
 rasmol.hlp us_admin.exe us_somo10.dll"
QTDLLS="Qt3Support4.dll QtCLucene4.dll QtCore4.dll \
 QtGui4.dll QtHelp4.dll QtNetwork4.dll QtOpenGL4.dll \
 QtSql4.dll QtSvg4.dll QtWebKit4.dll QtXml4.dll"


echo "Copy lib DLLs to bin"
cp -p ${SRCDIR}/lib/*.dll ${SRCDIR}/bin/

echo "Copy somo bins to bin"
for F in ${SOMOBINS}; do
  echo "  cp -p ${SRCDIR}/somo/bin/${F} ${SRCDIR}/bin/"
  cp -p ${SRCDIR}/somo/bin/${F} ${SRCDIR}/bin/
done

echo "Copy bin,etc directories"
cp -rp ${SRCDIR}/bin ${DESTDIR}/
cp -rp ${SRCDIR}/etc ${DESTDIR}/
chmod -R a+rw ${DESTDIR}/etc

if [ ! -d ${DESTDIR}/somo ]; then
  echo "Creating ${DESTDIR}/somo"
  mkdir ${DESTDIR}/somo
fi

echo "Copy somo doc,demo,test directories"
cp -rp ${SRCDIR}/somo/doc ${DESTDIR}/somo/
cp -rp ${US2DIR}/somo/demo ${DESTDIR}/somo/
cp -rp ${US2DIR}/somo/test ${DESTDIR}/somo/

echo "Copy Qt DLLs to bin"
for F in ${QTDLLS}; do
  echo "  cp -p ${QTBIN}/${F} ${DESTDIR}/bin/"
  cp -p ${QTBIN}/${F} ${DESTDIR}/bin/
done

