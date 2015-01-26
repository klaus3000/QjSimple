Known Issues:
=============
- link between Buddy and QListWidgetItem is based on the AoR, thus if multiple buddies have the same URI this will cause conflicts!
- reported crash: two QjSimple instances started on the same PC, registered with different AoR. IM from client 1 to client 2. (I could not reproduce it)
- reported crash: QjSimple subscribe its own presence

Deployment:
===========
To distribute the Windows binaries you have to package:
  - QjSimple binary (e.g. QjSimple.exe)
  - Qt libraries (for release version use the DLLs without 'd')
      QtCored4.dll
      QtGuid4.dll
      QtNetworkd4.dll
      imageformats/qgifd4.dll
  - mingw DLL:
      mingwm10.dll
      libgcc_s_dw2-1.dll (some versions of mingw only)
  - openssl DLLs:
      ssleay32.dll
      libeay32.dll

Further, as OpenSSL libraries were built with Visual Studio 2008,
the PC running QjSimple needs to have the Microsoft Visual C++ 2008 Redistributable Package (x86) installed. You can download it from: http://www.microsoft.com/DOWNLOADS/details.aspx?FamilyID=9b2da534-3e03-4391-8a4d-074b9f2bc1bf&displaylang=en


If you have problems with library dependencies use the following tools to analyse the QjSimple binary:
 Windows: Dependency Walker
 Linux: ldd

Application Icon:
=================
refer to http://doc.trolltech.com/4.3/appicon.html
Windows: The RC file needs two icons:
1: 16x16 for Explorer List View and top-left corner of an application's top-level windows
2: 32x32 for Explorer Icon View


