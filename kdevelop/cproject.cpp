/***************************************************************************
                 cproject.cpp - the projectproperties
                             -------------------                                         

    begin                : 28 Jul 1998                                        
    copyright            : (C) 1998 by Sandy Meier                         
    email                : smeier@rz.uni-potsdam.de                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#include <qdir.h>
#include "cproject.h"
#include <iostream.h>
#include <qregexp.h>
#include <kprocess.h>
#include "vc/versioncontrol.h"
#include "debug.h"

#define PROJECT_VERSION_STR "KDevelop Project File Version 0.3 #DO NOT EDIT#"

CProject::CProject(QString file)
  : config( file )
{
  valid = false;
  prjfile = file;
  vc = 0;

  ptStringMap = new QString[PT_END_POS];
  ptStringMap[ CORBA_SOURCE ] = "CORBA_SOURCE";
  ptStringMap[ CPP_SOURCE ] = "SOURCE";
  ptStringMap[ CPP_HEADER ] = "HEADER";
  ptStringMap[ SCRIPT ] = "SCRIPT";
  ptStringMap[ DATA ] = "DATA";
  ptStringMap[ PO ] = "PO";
  ptStringMap[ KDEV_DIALOG ] = "KDEV_DIALOG";
  ptStringMap[ LEXICAL ] = "SOURCE";
}

CProject::~CProject(){
  if(vc)
    delete vc;
  delete []ptStringMap;
}


bool CProject::readProject(){
  
  QFile qfile(prjfile);
  QTextStream stream(&qfile);
  qfile.open(IO_ReadOnly);
  QString str = stream.readLine();
  if(str.contains( PROJECT_VERSION_STR ) ){
    return false;
  }
  
  QFileInfo fileinfo(prjfile);
  dir = fileinfo.dirPath() + "/";
  setSourcesHeaders();
  vc = VersionControl::getVersionControl(getVCSystem());
  valid = true;
  return true;
}

void CProject::writeProject(){
  config.sync();
}

/*********************************************************************
 *                                                                   *
 *                 METHODS TO STORE CONFIG VALUES                    *
 *                                                                   *
 ********************************************************************/

void CProject::setVCSystem(const char *vcsystem)
{
    writeGroupEntry( "General", "version_control", vcsystem );
    if (vc)
        delete vc;
    vc = VersionControl::getVersionControl(vcsystem);
}


void CProject::setLFVOpenGroups(QStrList groups){
  config.setGroup( "General" );
  config.writeEntry( "lfv_open_groups", groups );
}

void CProject::setShortInfo(QStrList short_info){
  config.setGroup( "General" );
  config.writeEntry( "short_info", short_info );
}

void CProject::setLDFLAGS(QString flags){
  config.setDollarExpansion(false);
  writeGroupEntry( "Config for BinMakefileAm", "ldflags", flags );
  config.setDollarExpansion(true);
}

void CProject::setLDADD(QString libstring){
  config.setDollarExpansion(false);
  writeGroupEntry( "Config for BinMakefileAm", "ldadd", libstring );
  config.setDollarExpansion(true);
}

void CProject::setCXXFLAGS(QString flags){
  config.setDollarExpansion(false);
  writeGroupEntry( "Config for BinMakefileAm", "cxxflags",flags );
  config.setDollarExpansion(true);
}

void CProject::setModifyMakefiles(bool enable){
    config.setGroup("General");
    config.writeEntry("modifyMakefiles",enable);
    config.sync();
}

void CProject::setAdditCXXFLAGS(QString flags){
  config.setDollarExpansion(false);
  writeGroupEntry( "Config for BinMakefileAm", "addcxxflags", flags );
  config.setDollarExpansion(true);
}

void CProject::setFilters(QString group,QStrList& filters){
  config.setGroup("LFV Groups");
  config.writeEntry(group,filters);
}

void CProject::writeFileInfo(TFileInfo info){
  config.setGroup( info.rel_name );
  config.writeEntry("type", getTypeString( info.type ) );
  config.writeEntry("dist",info.dist);
  config.writeEntry("install",info.install);
  // save the $ because kconfig removes one
  info.install_location.replace("[\\$]","$$");
  config.writeEntry("install_location",info.install_location);
}

void CProject::writeDialogFileInfo(TDialogFileInfo info){
  config.setGroup( info.rel_name );
  config.writeEntry("type", "KDEV_DIALOG" );
  config.writeEntry("dist",info.dist);
  config.writeEntry("install",info.install);
  // save the $ because kconfig removes one
  info.install_location.replace("[\\$]","$$");
  config.writeEntry("install_location",info.install_location);

  config.writeEntry("baseclass",info.baseclass);
  config.writeEntry("widget_files",info.widget_files);
  config.writeEntry("is_toplevel_dialog",info.is_toplevel_dialog);
  config.writeEntry("header_file",info.header_file);
  config.writeEntry("cpp_file",info.header_file);
  config.writeEntry("data_file",info.data_file);
  config.writeEntry("classname",info.classname);
  
}
// void CProject::writeMakefileAmInfo(TMakefileAmInfo info){
//   QString rel_name = info.rel_name;
//   config.setGroup(rel_name);
//   config.writeEntry("type",info.type);
//   config.writeEntry("sub_dirs",info.sub_dirs);
// }

/*********************************************************************
 *                                                                   *
 *                 METHODS TO FETCH CONFIG VALUES                    *
 *                                                                   *
 ********************************************************************/

void CProject::getLFVOpenGroups(QStrList& groups){
  config.setGroup("General");
  config.readListEntry("lfv_open_groups",groups);  
}

void CProject::getLFVGroups(QStrList& groups){
  groups.clear();
  config.setGroup("LFV Groups");
  config.readListEntry("groups",groups);
}

QStrList CProject::getShortInfo(){
  QStrList list;
  config.setGroup("General");
  config.readListEntry("short_info",list);
  return list;
}

QString CProject::getLDFLAGS(){
  QString str;
  config.setDollarExpansion(false);
  str = readGroupEntry( "Config for BinMakefileAm", "ldflags" );
  config.setDollarExpansion(true);
  return str;
}

QString CProject::getLDADD(){
  QString str;
  config.setDollarExpansion(false);
  str = readGroupEntry( "Config for BinMakefileAm", "ldadd" );
  config.setDollarExpansion(true);
  return str;
}
bool CProject::getModifyMakefiles(){
    config.setGroup("General");
    return config.readBoolEntry("modifyMakefiles",true); 
}

QString CProject::getCXXFLAGS(){
  QString str;
  config.setDollarExpansion(false);
  str = readGroupEntry( "Config for BinMakefileAm", "cxxflags" );
  config.setDollarExpansion(true);
  return str;
}

QString CProject::getAdditCXXFLAGS(){
  QString str;
  config.setDollarExpansion(false);
  str = readGroupEntry( "Config for BinMakefileAm", "addcxxflags" );
  config.setDollarExpansion(true);
  return str;
}

void CProject::getFilters(QString group,QStrList& filters){
  filters.clear();
  config.setGroup("LFV Groups");
  config.readListEntry(group,filters);
}

TFileInfo CProject::getFileInfo(QString rel_filename){
  TFileInfo info;
  config.setGroup(rel_filename);
  info.rel_name = rel_filename;
  info.type = getTypeFromString( config.readEntry("type") );
  info.dist = config.readBoolEntry("dist");
  info.install = config.readBoolEntry("install");
  info.install_location = config.readEntry("install_location");
  return info;  
}

TMakefileAmInfo CProject::getMakefileAmInfo(QString rel_name){
  TMakefileAmInfo info;

  config.setGroup(rel_name);
  info.rel_name = rel_name;
  info.type = config.readEntry("type");
  config.readListEntry("sub_dirs",info.sub_dirs);

  return info;  
}

/*********************************************************************
 *                                                                   *
 *                         PUBLIC QUERIES                            *
 *                                                                   *
 ********************************************************************/

/*----------------------------------------------- CProject::getType()
 * getType()
 *   Return the type of file based on the extension.
 *
 * Parameters:
 *   aFile           The file to check.
 *
 * Returns:
 *   ProjectFileType The filetype.
 *   DATA            If unknown.
 *-----------------------------------------------------------------*/
ProjectFileType CProject::getType( const char *aFile )
{
  ProjectFileType retVal=DATA;

  QString ext(aFile);
  int pos = ext.findRev('.');

  if (pos == -1 ){ return retVal;} // not found, so DATA
  ext = ext.right(ext.length()-pos);
  
  //ext = rindex( aFile, '.' );
  if( !ext.isEmpty() )
    {
      // Convert to lowercase.
      ext = ext.lower();
      
      // Check for a known extension.
      if( ext == ".cpp" || ext == ".c" || ext == ".cc" ||
	  ext == ".ec" || ext == ".ecpp" || ext == ".C" || ext == ".cxx" )
	retVal = CPP_SOURCE;
      else if( ext == ".h" || ext == ".hxx" || ext == ".hpp" || ext == ".H" || ext == ".hh" )
	retVal = CPP_HEADER;
      else if( ext == ".l++" || ext == ".lxx" || ext == ".ll" || ext == ".l")
	retVal = LEXICAL;
      else if( ext == ".idl" )
	retVal = CORBA_SOURCE;
      else if( ext == ".kdevdlg" )
	retVal = KDEV_DIALOG;
      else if( ext == ".po" )
	retVal = PO;
    }
  
  return retVal;
}

/*------------------------------------- CProject::getTypeFromString()
 * getTypeFromString()
 *   Return the type matching a string
 *
 * Parameters:
 *   aStr            String representation of the type.
 *
 * Returns:
 *   ProjectFileType The filetype.
 *   DATA            If unknown.
 *-----------------------------------------------------------------*/
ProjectFileType CProject::getTypeFromString( const char *aStr )
{
  int i;

  for(i=0; i<PT_END_POS && ptStringMap[ i ] != aStr; i++ )
    ;

  return ( i == PT_END_POS ? DATA : (ProjectFileType)i );
}

/*------------------------------------------ CProject::getTypeString()
 * getTypeString()
 *   Return the string matching a type.
 *
 * Parameters:
 *   aType           The type to get the string for.
 *
 * Returns:
 *   const char *    The type as a string.
 *-----------------------------------------------------------------*/
const char *CProject::getTypeString( ProjectFileType aType )
{
  return ptStringMap[ aType ];
}

void CProject::getAllFiles(QStrList& list){
  QStrList makefile_list;
  QStrList file_list;
  QString makefile_name;
  QString file_name;
  list.clear();
  config.setGroup("General");
  config.readListEntry("makefiles",makefile_list);
  
  for(makefile_name=makefile_list.first();makefile_name!=0;makefile_name=makefile_list.next()){
    config.setGroup(makefile_name);
    file_list.clear();
    config.readListEntry("files",file_list);
    for(file_name=file_list.first();file_name!=0;file_name=file_list.next()){
      list.append(file_name);
    }
  }  
}

bool CProject::isDirInProject(QString rel_name){

//  KDEBUG(KDEBUG_INFO,CPROJECT,"isDirInProject() Don't use this function,it's not implemented!");
  return true;
  int pos = rel_name.findRev('/');
  QString dir_name;
   
  
  if(pos == -1){ // not found
    dir_name = rel_name.copy();
    rel_name = "Makefile.am";
  }
  else{
    dir_name = rel_name.right(rel_name.length()-pos-1);
    rel_name.truncate(pos+1);
    rel_name.append("Makefile.am");
  }
  
  TMakefileAmInfo info = getMakefileAmInfo(rel_name);
  QString str;
  
  for(str=info.sub_dirs.first();str!=0;str=info.sub_dirs.next()){
    cerr << endl << str;
  }
}

bool CProject::addDialogFileToProject(QString rel_name,TDialogFileInfo info){
  config.setGroup(info.rel_name);
  config.writeEntry("baseclass",info.baseclass);
  config.writeEntry("widget_files",info.widget_files);
  config.writeEntry("is_toplevel_dialog",info.is_toplevel_dialog);
  config.writeEntry("header_file",info.header_file);
  config.writeEntry("cpp_file",info.header_file);
  config.writeEntry("data_file",info.data_file);
  config.writeEntry("classname",info.classname);
  
  TFileInfo file_info;
  file_info.rel_name = info.rel_name;
  file_info.type = KDEV_DIALOG;
  file_info.dist = info.dist;
  file_info.install = info.install;

  return addFileToProject(file_info.rel_name,file_info);
}

bool CProject::addFileToProject(QString rel_name,TFileInfo info)
{  
  // normalize it a little bit
  rel_name.replace(QRegExp("///"),"/"); // remove ///
  rel_name.replace(QRegExp("//"),"/"); // remove //
    
  QStrList all_files;
  getAllFiles(all_files);

  if(all_files.contains(rel_name) > 0){ // file is already in Project?
    return false; // no new subdir;
  }
  
  QStrList list_files;
  QString makefile_name;
  QStrList sub_dirs;

  // find the correspond. Makefile.am, store it into makefile_name
  int slash_pos = rel_name.findRev('/');
  if(slash_pos == -1) { // not found
    makefile_name = "Makefile.am";
  }
  else {
    makefile_name = rel_name.copy();
    makefile_name.truncate(slash_pos+1);
    makefile_name += "Makefile.am";
  }

  //add the file into the filesentry in the Makefile.am group
  config.setGroup(makefile_name);
  config.readListEntry("files",list_files);
  list_files.append(rel_name);
  config.writeEntry("files",list_files);
  
  //++++++++++++++add Makefile.am and the toplevels makefile.ams to the project if needed (begin)
  QStrList makefile_list;
  QStrList check_makefile_list;
  // find the makefiles to check
  int slash2_pos;
  check_makefile_list.append(makefile_name);
  QString makefile_name_org = makefile_name.copy();

  //  cerr << endl << "*check:*" << makefile_name;
  
  while((slash_pos = makefile_name.findRev('/')) != -1){ // if found

    slash2_pos = makefile_name.findRev('/',slash_pos-1);
    if(slash2_pos != -1 && makefile_name != "/"){
      makefile_name.remove(slash2_pos,slash_pos-slash2_pos);
      check_makefile_list.append(makefile_name);
      cerr << "append to check_makefile_list: " << makefile_name << endl;
    } 
    else{
      makefile_name = "";
    }
  }

  config.setGroup("General");
  config.readListEntry("makefiles",makefile_list);

  TMakefileAmInfo makefileaminfo;
  
  for(makefile_name=check_makefile_list.first();
      makefile_name!=0;
      makefile_name=check_makefile_list.next())
  { 
    // check if current makefile exists and all makefile above,if not add it
    if(makefile_list.find(makefile_name) == -1){
      makefileaminfo.rel_name = makefile_name;
      if(makefile_name_org == makefile_name){ // the first makefileam
	if(info.type == CPP_SOURCE ) 
          makefileaminfo.type = "static_library"; // cool 
	else makefileaminfo.type = "normal";
	addMakefileAmToProject(makefile_name,makefileaminfo);
      }
      else{
	makefileaminfo.type = "normal";
	addMakefileAmToProject(makefile_name,makefileaminfo);
      }
    }
  }
  //++++++++++++++++add Makefile to the project if needed (end)


  makefile_name = check_makefile_list.first(); // get the complete makefilename


  // change the makefile type if needed
  if(info.type == CPP_SOURCE){ // a static library is needed?
    config.setGroup(makefile_name);

    if(config.readEntry("type") != "prog_main"){
      config.writeEntry("type","static_library");
    }
  }
  // and at last: modify the subdir entry in every Makefile.am if needed

  QString subdir;
  bool new_subdir=false;

  while((slash_pos = makefile_name.findRev('/')) != -1){ // if found
    slash2_pos = makefile_name.findRev('/',slash_pos-1);
    if(slash2_pos != -1){
      subdir = makefile_name.mid(slash2_pos+1,slash_pos-slash2_pos-1);
//      KDEBUG1(KDEBUG_INFO,DIALOG,"SUBDIR %s",subdir.data());
      makefile_name.remove(slash2_pos,slash_pos-slash2_pos);
      config.setGroup(makefile_name);
      sub_dirs.clear();
      config.readListEntry("sub_dirs",sub_dirs);
      
      if(sub_dirs.find(subdir) == -1){
	new_subdir = true;
	sub_dirs.append(subdir);
	config.writeEntry("sub_dirs",sub_dirs);
      }
    }
    else{
      // the subdirs of the topdir are special
      subdir = makefile_name.left(slash_pos);
      config.setGroup("Makefile.am");
      sub_dirs.clear();
      config.readListEntry("sub_dirs",sub_dirs);
      
      if(sub_dirs.find(subdir) == -1){
	new_subdir = true;
	sub_dirs.append(subdir);
	config.writeEntry("sub_dirs",sub_dirs);
      
      }
      makefile_name = "";
    }
  }

  if(new_subdir){
    updateConfigureIn();
  }

  //  createMakefilesAm(); // do some magic generation

  // writethe fileinfo
  writeFileInfo( info );

  config.sync();
  setSourcesHeaders();
  return new_subdir;
}

void CProject::removeFileFromProject(QString rel_name){
  QStrList list_files;
  QString makefile_name;
  int slash_pos = rel_name.findRev('/');
  if(slash_pos == -1) { // not found
    makefile_name = "Makefile.am";
  }
  else{
    makefile_name = rel_name.copy();
    makefile_name.truncate(slash_pos+1);
    makefile_name += "Makefile.am";
  }
  config.setGroup(makefile_name);
  config.readListEntry("files",list_files); 
  list_files.remove(rel_name);
  config.writeEntry("files",list_files);

  // remove the fileinfo
  config.deleteGroup(rel_name);
  setSourcesHeaders();
  updateMakefilesAm();
}

void CProject::updateMakefilesAm(){
  config.setGroup("General");
  
  bool update = getModifyMakefiles();
    
  if ( getProjectType() == "normal_empty" || update == false)
    return;
  
  QString makefile;
  QStrList makefile_list;
  
  config.readListEntry("makefiles",makefile_list);  
  for(makefile = makefile_list.first();makefile !=0;makefile =makefile_list.next()){ // every Makefile
    config.setGroup(makefile);
    updateMakefileAm(makefile); 
  }
}

void CProject::updateMakefileAm(QString makefile){
  setKDevelopWriteArea(makefile);
  QString abs_filename = getProjectDir() + makefile;
  QFile file(abs_filename);
  QStrList list;
  QStrList files;
  QStrList subdirs;
  QString str;
  QString str2;
  QString dist_str;
  QTextStream stream(&file);
  bool found=false;

  QString libname;
  QStrList static_libs;
  getAllStaticLibraries(static_libs);
  
  QString sources;
  QStrList source_files;
  QStrList po_files;
  QString pos;
  config.setGroup(makefile);
  config.readListEntry("files",files);
  config.readListEntry("sub_dirs",subdirs);
  if(file.open(IO_ReadOnly)){ // read the makefileam
    while(!stream.eof()){
      list.append(stream.readLine());
    }
  }
  file.close();

  if(file.open(IO_WriteOnly)){
    for(str = list.first();str != 0;str = list.next()){ // every line
      if(str == "####### kdevelop will overwrite this part!!! (begin)##########"){
 	stream << str << "\n";
	
	//***************************generate needed things for the main makefile*********
	if (config.readEntry("type") == "prog_main"){ // the main makefile
	  stream << "bin_PROGRAMS = " << getBinPROGRAM() << "\n";
	  getSources(makefile,source_files);
	  for(str= source_files.first();str !=0;str = source_files.next()){
	    sources =  str + " " + sources ;
	  }
	  //	  stream << "CXXFLAGS = " << getCXXFLAGS()+" "+getAdditCXXFLAGS() << "\n";
	  //stream << "LDFLAGS = " << getLDFLAGS()  << "\n";
	  stream << getBinPROGRAM()  <<  "_SOURCES = " << sources << "\n";
	  if(static_libs.isEmpty()){
	    stream << getBinPROGRAM()  <<  "_LDADD   = " << getLDADD();
	  }
	  else{ // we must link some libs
	    stream << getBinPROGRAM()  <<  "_LDADD   = ";
	    for(libname = static_libs.first();libname != 0;libname = static_libs.next()){
	      stream << libname.replace(QRegExp("^"+getSubDir()),"./") << " "; // remove the subdirname
	    }
	    stream << getLDADD();
	  }
	  
	  if(getProjectType() != "normal_cpp" && getProjectType() != "normal_c") {
	    stream << " $(LIBSOCKET)" << "\n";
	  }
	  else{
	    stream << "\n";
	  }
	  
	}
	//***************************generate needed things for static_library*********
	config.setGroup(makefile);
	if(config.readEntry("type") == "static_library"){
	  getSources(makefile,source_files);
	  for(str= source_files.first();str !=0;str = source_files.next()){
	    sources =  str + " " + sources ;
	  }
	  QDir dir(getDir(makefile));
	  if (getProjectType() != "normal_cpp" && getProjectType() != "normal_c"){
	    stream << "\nINCLUDES = $(all_includes)\n\n";
	    stream << "lib" << dir.dirName() << "_a_METASOURCES = USE_AUTOMOC\n\n";
	  }
	  stream << "noinst_LIBRARIES = lib" << dir.dirName() << ".a\n\n";
	  stream << "lib" << dir.dirName() << "_a_SOURCES = " << sources << "\n";
	}

	//***************************generate needed things for a po makefile*********
	if (config.readEntry("type") == "po"){ // a po makefile
	  getPOFiles(makefile,po_files);
	  for(str= po_files.first();str !=0;str = po_files.next()){
	    pos =  str + " " + pos ;
	  }
	  
	  stream <<  "POFILES = " << pos << "\n";
	}
 
	// ********generate the dist-hook, to fix a automoc problem, hope "make dist" works now******
	if((getProjectType() != "normal_cpp")  && getProjectType() != "normal_c"&&
                   (makefile == QString("Makefile.am"))){
	  stream << "dist-hook:\n\t-perl automoc\n";
	}
	//************SUBDIRS***************
	if(!subdirs.isEmpty()){ // the SUBDIRS key
	  stream << "\nSUBDIRS = ";
	  for(str2 = subdirs.first();str2 !=0;str2 = subdirs.next()){
	    stream << str2 << " ";
	  }
	}
	stream << "\n";
	//************EXTRA_DIST************
	dist_str = "\nEXTRA_DIST = ";
	bool dist_write=false;
	for(str2 = files.first();str2 !=0;str2 = files.next()){
	  config.setGroup(str2);
	  if (config.readBoolEntry("dist")){
	    dist_str = dist_str + getName(str2) + " ";
	    dist_write = true;
	  }
	}
	if (dist_write){
	  stream << dist_str << "\n";
	}
	//**************install-data-local****************
	bool install_data=false;
	QString install_data_str = "\ninstall-data-local:\n";
	for(str2 = files.first();str2 !=0;str2 = files.next()){
	  config.setGroup(str2);
	  if (config.readBoolEntry("install") && config.readEntry("type") != "SCRIPT"){
	    install_data_str = install_data_str + "\t$(mkinstalldirs) " 
	      + getDir(config.readEntry("install_location")) + "\n";
	    install_data_str = install_data_str + "\t$(INSTALL_DATA) " +
	      getName(str2) + " " + config.readEntry("install_location") + "\n";
	    
	    install_data = true;
	  }
	}
	if(install_data){
	  stream << install_data_str;
	}
	
	//**************install-exec-local****************
	bool install_exec=false;
	QString install_exec_str = "\ninstall-exec-local:\n";
	for(str2 = files.first();str2 !=0;str2 = files.next()){
	  config.setGroup(str2);
	  if (config.readBoolEntry("install") && config.readEntry("type") == "SCRIPT"){
	    install_exec_str = install_exec_str + "\t$(mkinstalldirs) " 
	      + getDir(config.readEntry("install_location")) + "\n";
	    install_exec_str = install_exec_str + "\t$(INSTALL_SCRIPT) " +
	      getName(str2) + " " + config.readEntry("install_location") + "\n";
	    install_exec = true;
	  }
	}
	if(install_exec){
	  stream << install_exec_str;
	}
	
	//**************uninstall-local*******************
	bool uninstall_local=false;
	QString uninstall_local_str = "\nuninstall-local:\n";
	for(str2 = files.first();str2 !=0;str2 = files.next()){
	  config.setGroup(str2);
	  if (config.readBoolEntry("install")) {
	    uninstall_local_str = uninstall_local_str + "\t-rm -f " + 
	      config.readEntry("install_location") +"\n";
	    uninstall_local=true;
	  }
	}
	if(uninstall_local){
	  stream << uninstall_local_str;
	}
	stream << "\n";	
	found = true;
      }
      if(found == false){
	stream << str +"\n";
      }
    if(str =="####### kdevelop will overwrite this part!!! (end)############"){
      stream << str + "\n";
      found = false;
    } 
    } // end for
  }// end writeonly
  file.close();
  
}

QString CProject::getDir(QString rel_name){
  int pos = rel_name.findRev('/');
  return rel_name.left(pos+1);
}

QString CProject::getName(QString rel_name){
  int pos = rel_name.findRev('/');
  int len = rel_name.length() - pos - 1;
  return rel_name.right(len);
}

void CProject::getAllTopLevelDialogs(QStrList& list){
  list.clear();
  QStrList  all_files;
  TFileInfo info;
  getAllFiles(all_files);
  QString file;
  for(file = all_files.first();file != 0;file = all_files.next()){
    info = getFileInfo(file);
    if(info.type == KDEV_DIALOG){
      list.append(info.rel_name);
    }
  }
}

TDialogFileInfo CProject::getDialogFileInfo(QString rel_filename){
  TDialogFileInfo info;
  config.setGroup(rel_filename);
  info.rel_name = rel_filename;
  info.type = config.readEntry("type");
  info.dist = config.readBoolEntry("dist");
  info.install = config.readBoolEntry("install");
  info.install_location = config.readEntry("install_location");
  info.classname = config.readEntry("classname");
  info.baseclass = config.readEntry("baseclass");
  info.is_toplevel_dialog = config.readBoolEntry("is_toplevel_dialog");
  config.readListEntry("widget_files",info.widget_files);
  info.header_file = config.readEntry("header_file");
  info.source_file = config.readEntry("source_file");
  info.data_file = config.readEntry("data_file");
  return info;  
}

void CProject::getSources(QString rel_name_makefileam,QStrList& sources){
  sources.clear();
  QStrList files;
  QString file;
  TFileInfo info;
  config.setGroup(rel_name_makefileam);
  config.readListEntry("files",files);
  
  for(file = files.first();file != 0;file = files.next()){
    info = getFileInfo(file);
    if(info.type == CPP_SOURCE ){
      sources.append(getName(file));
    }
  }
  
  // for(file = files.first();file != 0;file = files.next()){
//     if( getType( file ) == CPP_SOURCE )
//       sources.append(getName(file));
//   }
}

void CProject::getPOFiles(QString rel_name_makefileam,QStrList& po_files){
  po_files.clear();
  QStrList files;
  char *file;
  TFileInfo info;
  config.setGroup(rel_name_makefileam);
  config.readListEntry("files",files);
  
  for(file = files.first();file != 0;file = files.next()){
    if( getType( file ) == PO )
    {
      info = getFileInfo(file);
      if(info.type == PO){
	po_files.append(getName(file));
      }
    }
  }
}

void CProject::setSourcesHeaders(){
  // clear the lists
  header_files.clear();
  cpp_files.clear();
  TFileInfo info;
  QStrList files;
  char *file;

  getAllFiles(files);

  for(file = files.first();file != 0;file = files.next()){
    info = getFileInfo(file);
    if(info.type == CPP_SOURCE){
      cpp_files.append(getProjectDir()+file);
    }
    if(info.type == CPP_HEADER){
      header_files.append(getProjectDir()+file);
    }
  }
    // switch( getType( file ) )
//     {
//       case CPP_SOURCE:
    // 	cpp_files.append(getProjectDir()+file);
//         break;
//       case CPP_HEADER:        
//         header_files.append(getProjectDir()+file);
//         break;
//       default:
//         break;
//     }
}

void CProject::setKDevelopWriteArea(QString makefile){
  QString abs_filename = getProjectDir() + makefile;
  QFile file(abs_filename);
  QStrList list;
  bool found = false;
  QTextStream stream(&file);
  QString str;
  
  if(file.open(IO_ReadOnly)){ // read the makefileam
    while(!stream.eof()){
      list.append(stream.readLine());
    }
  }
  file.close();
  for(str = list.first();str != 0;str = list.next()){
    if (str == "####### kdevelop will overwrite this part!!! (begin)##########"){
      found = true;
    }
  }
  if(!found){
    // create the writeable area
    file.open(IO_WriteOnly);
    stream << "####### kdevelop will overwrite this part!!! (begin)##########\n";
    stream << "####### kdevelop will overwrite this part!!! (end)############\n";
    for(str = list.first();str != 0;str = list.next()){
      stream << str + "\n";
    }
  }
}

void CProject::addLFVGroup(QString name,QString ace_group){
  QStrList groups;
  config.setGroup("LFV Groups");
  config.readListEntry("groups",groups);
  if(ace_group.isEmpty()){
    groups.insert(0,name);
    config.writeEntry("groups",groups);
    return;
  }
  int pos = groups.find(ace_group);
  groups.insert(pos+1,name);
  config.writeEntry("groups",groups);
}

void CProject::removeLFVGroup(QString name){
  QStrList groups;
  config.setGroup("LFV Groups");
  config.readListEntry("groups",groups);
  groups.remove(name);
  config.deleteEntry(name,false);
  config.writeEntry("groups",groups);
}

void CProject::addMakefileAmToProject(QString rel_name,TMakefileAmInfo info){
  
  config.setGroup(rel_name);
  config.writeEntry("type",info.type);
  config.writeEntry("sub_dirs",info.sub_dirs);

  QStrList makefile_list;
  config.setGroup("General");
  config.readListEntry("makefiles",makefile_list); 
  makefile_list.append(rel_name);
  config.writeEntry("makefiles",makefile_list);
}

void CProject::updateConfigureIn(){

  if( isCustomProject()) return; // do nothing

  QString abs_filename = getProjectDir() + "/configure.in";
  QFile file(abs_filename);
  QStrList list;
  QTextStream stream(&file);
  QString str;
  QStrList makefile_list;
  QString makefile;
    

  if(file.open(IO_ReadOnly)){ // read the configure.in
    while(!stream.eof()){
      list.append(stream.readLine());
    }
  }
  file.close();

  file.open(IO_WriteOnly);
  
  for(str = list.first();str != 0;str = list.next()){
    if(str.find("AC_OUTPUT(") != -1){ // if found
      stream << "AC_OUTPUT(";
      config.setGroup("General");
      config.readListEntry("makefiles",makefile_list);  
      for(makefile = makefile_list.first();makefile !=0;makefile =makefile_list.next()){
	stream << makefile.remove(makefile.length()-3,3) << " ";
      }
      stream << ")\n";
      
    }
   //  else if(str.find("AC_PROG_LEX") != -1) {
//       stream << "AC_PROG_LEX" << endl;
//     }
//     else if(str.find("AC_DECL_YYTEXT") != -1) {
//       stream << "AC_DECL_YYTEXT" << endl;
//     }
    else if(str.find("KDE_DO_IT_ALL(") != -1){
      stream << "KDE_DO_IT_ALL(";
      stream << getProjectName().lower() << "," << getVersion();
      stream << ")\n";
    }
    else if(str.find("AM_INIT_AUTOMAKE(") != -1){
      stream << "AM_INIT_AUTOMAKE(";
      stream << getProjectName().lower() << "," << getVersion();
      stream << ")\n";
    }
    else{
      stream << str + "\n";
    }
  }
  
 
}
void  CProject::writeWorkspace(TWorkspace ws){
  switch(ws.id){
  case 1:
    config.setGroup("Workspace_1");
    break;
  case 2:
    config.setGroup("Workspace_2");
    break;
  case 3:
    config.setGroup("Workspace_3");
    break;
  default:
    config.setGroup("Workspace_1");
  }
  config.writeEntry("openfiles",ws.openfiles);
  config.writeEntry("header_file",ws.header_file);
  config.writeEntry("cpp_file",ws.cpp_file);
  config.writeEntry("browser_file",ws.browser_file);
  config.writeEntry("show_treeview",ws.show_treeview);
  config.writeEntry("show_outputview",ws.show_output_view);
  config.sync();
}
void CProject::setCurrentWorkspaceNumber(int id){
   config.setGroup("General");
   config.writeEntry("workspace",id);
   config.sync();
}
int CProject::getCurrentWorkspaceNumber(){
  config.setGroup("General");
  int i = config.readNumEntry("workspace",1);
  return i;
}
TWorkspace CProject::getWorkspace(int id){
  TWorkspace ws;

  switch(id){
  case 1:
    config.setGroup("Workspace_1");
    break;
  case 2:
    config.setGroup("Workspace_2");
    break;
  case 3:
    config.setGroup("Workspace_3");
    break;
  default:
    config.setGroup("Workspace_1");
  }

  config.readListEntry("openfiles",ws.openfiles);
  ws.header_file = config.readEntry("header_file");
  ws.cpp_file = config.readEntry("cpp_file");
  ws.browser_file = config.readEntry("browser_file");
  ws.show_treeview = config.readBoolEntry("show_treeview",true);
  ws.show_output_view =config.readBoolEntry("show_outputview",true);
  return ws;
}

void CProject::getAllStaticLibraries(QStrList& libs){
  QDir dir;
  QStrList makefiles;
  QString makefile;

  libs.clear();
  config.setGroup("General");
  
  config.readListEntry("makefiles",makefiles);
  
  for(makefile=makefiles.first();makefile != 0;makefile=makefiles.next()){
    config.setGroup(makefile);
    if(config.readEntry("type") == "static_library"){
      
      dir.setPath(getDir(makefile));
      libs.append(getDir(makefile) + "lib" + dir.dirName() + ".a");
    }
  }
}

/*-------------------------------------- CProject::setInfosInString()
 * setInfosInString()
 *    changes the string to the real values of the project         *
 * Parameters:
 *    strtemplate   template string to change
 *    basics        if true change only basic informations
 *                  (optimizing parameter)
 *
 * Returns:
 *   reference to the stringlist
 *-----------------------------------------------------------------*/
QString& CProject::setInfosInString(QString& strtemplate, bool basics)
{
   QString date=QDate::currentDate().toString();
   QString year;
   year.setNum(QDate::currentDate().year());
   QString projectName=getProjectName();

   strtemplate.replace(QRegExp("\\|PRJNAME\\|"), projectName);
   strtemplate.replace(QRegExp("\\|NAME\\|"), projectName);
   strtemplate.replace(QRegExp("\\|NAMELITTLE\\|"), projectName.lower());
   strtemplate.replace(QRegExp("\\|NAMEBIG\\|"), projectName.upper());

   strtemplate.replace(QRegExp("\\|PRJDIR\\|"), getProjectDir());
   strtemplate.replace(QRegExp("\\|PRJFILE\\|"), getProjectFile());
   strtemplate.replace(QRegExp("\\|SUBDIR\\|"), getSubDir());
   strtemplate.replace(QRegExp("\\|AUTHOR\\|"), getAuthor());
   strtemplate.replace(QRegExp("\\|EMAIL\\|"), getEmail());
   strtemplate.replace(QRegExp("\\|VERSION\\|"), getVersion());
   strtemplate.replace(QRegExp("\\|BINPROGRAM\\|"), getBinPROGRAM());
   strtemplate.replace(QRegExp("\\|DATE\\|"), date);
   strtemplate.replace(QRegExp("\\|YEAR\\|"), year);

   if (!basics)
   {
     strtemplate.replace(QRegExp("\\|KDEVPRJVER\\|"), getKDevPrjVersion());
     strtemplate.replace(QRegExp("\\|PRJTYPE\\|"), getProjectType());
     strtemplate.replace(QRegExp("\\|CLASSVIEWTREE\\|"), getClassViewTree());
     strtemplate.replace(QRegExp("\\|SGMLFILE\\|"), getSGMLFile());
     strtemplate.replace(QRegExp("\\|EXEARGS\\|"), getExecuteArgs());
     strtemplate.replace(QRegExp("\\|MAKEOPTIONS\\|"), getMakeOptions());
     strtemplate.replace(QRegExp("\\|LDFLAGS\\|"), getLDFLAGS());
     strtemplate.replace(QRegExp("\\|LDADD\\|"), getLDADD());
     strtemplate.replace(QRegExp("\\|CXXFLAGS\\|"), getCXXFLAGS());
     strtemplate.replace(QRegExp("\\|ADDITCXX\\|"), getAdditCXXFLAGS());
   }

   return strtemplate;
}

/*********************************************************************
 *                                                                   *
 *                          PRIVATE METHODS                          *
 *                                                                   *
 ********************************************************************/

/*-------------------------------------- CProject::writeGroupEntry()
 * writeGroupEntry()
 *   Write an entry to the project file. 
 *
 * Parameters:
 *   group          Name of the group.
 *   tag            The value-tag e.g version.
 *   entry          The string to store.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CProject::writeGroupEntry( const char *group, const char *tag, const char *entry )
{
  config.setGroup( group );
  config.writeEntry( tag, entry );
}

/*-------------------------------------- CProject::readGroupEntry()
 * writeGroupEntry()
 *   Read an entry from the project file and return it.
 *
 * Parameters:
 *   group          Name of the group.
 *   tag            The value-tag e.g version.
 *
 * Returns:
 *   QString        The value.
 *-----------------------------------------------------------------*/
QString CProject::readGroupEntry( const char *group, const char *tag )
{
  config.setGroup( group );
  return config.readEntry( tag,"" );
}
bool CProject::isKDEProject(){
  if (getProjectType() == "normal_kde" || getProjectType() == "mini_kde") return true;
  return false;
}
bool CProject::isQtProject(){
  if (getProjectType() == "normal_qt") return true;
  return false;
}
bool CProject::isCustomProject(){
  if(getProjectType() == "normal_empty") return true;
  return false;
}
