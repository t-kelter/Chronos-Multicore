/*

   This source file belongs to the

            TU Dortmund University
              WCC Compiler framework

   and is property of the Technische Universität Dortmund. It must neither be
   used nor published even in parts without explicit written permission.

   Copyright 2007 - 2010

   TU Dortmund University
   Department of Computer Science 12
   44221 Dortmund
   Germany

   http://ls12-www.cs.tu-dortmund.de/research/activities/wcc

*/


#include "debugmacros.h"

// Include Standard headers
#include <iostream>
#include <fstream>
#include <map>
#include <stack>
#include <string>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Define for debugging these debug functions:
// #define DEBUGDEBUG

//Define for tabbing second row
// #define TABROW


using namespace std;


//! Map to store the ofstreams for writing debug output to
static map<string, ostream *> logFiles;

//! Map to store the functions with enabled debug output
static map<string, int> mapFunctions;

//! Map to store the name of the stream for each function to write to
static map<string, string> mapFiles;

static stack<string> stackFunctionCalls;
static stack<string> stackActivationNames;


// Flags for Command sets
bool bShowFuncStart = true;
bool bIgnoreFunctionName = false;
static bool bDoAutoDestruction = false;


#ifdef TABROW
// Initialized to 6 for standard output.
unsigned int iMaxFunctionNameLength = 6;
#endif


void UserInteraction()
{
  string event;

  dprint_prologue();
  cout << "Press 'x' to exit, any other key to continue  ";
  cin >> event;

  if ( event.compare( "x" ) == 0 )
    exit( -1 );
};


bool dempty()
{
  return (stackFunctionCalls.size() == 0);
}

unsigned int dsize()
{
  return stackFunctionCalls.size();
}


void dinitfile( const char *configfile, const char *logfilename )
{
  char *b = 0;
  char s[MAXDEBUGFILENAME];
  char *pszSecondString = 0;

  //  logfilename = substitute_environment_variables( logfilename );

  if ( logFiles.empty() )
    logFiles[ "cout" ] = &cout;

  // Read config file to select functions for en-/disabling debug output.
  FILE *datafile = fopen( configfile, "r" );

  // If no debug configuration file is found, simply disable debugging
  // instead of exiting.
  if ( datafile == 0 ) {
    #ifdef DEBUGDEBUG
    fprintf( stderr, "DEBUGMACROS::dinitfile: Can't open debug config file %s - %s",
             configfile, "debugging not possible for this module\n" );
    #endif
    return;
  }

  // Open desired output file for debug data if not stdout.
  if (logFiles[ string( logfilename ) ] == 0) {
    ofstream *logfile = new ofstream ( logfilename, ios::trunc );

    if ( !logfile ) {
      fprintf(
        stderr,
        "DEBUGMACROS::dinitfile: Can't open log file %s - %s",
        logfilename,
        "debugging not possible for this module\n" );
      return;
    }

    logFiles[ string( logfilename ) ] = logfile;

    #ifdef DEBUGDEBUG
    cout << "DEBUGMACROS: Called dinitfile() and opening a logfile called \""
         << logfilename << "\"." << endl;
    #endif
  }
  #ifdef DEBUGDEBUG
  else cout << "Logfile " << logfilename << " already opened" << endl;
  #endif

  // Read the function names to en-/disable debug output.
  while ( !feof( datafile ) ) {
    b = fgets( s, MAXDEBUGFILENAME - 1, datafile );
    if ( !b )
      break;

    if ( s[0] != '\n' ) {
      // Make sure, that the string is finite.
      s[MAXDEBUGFILENAME - 1] = '\0';

      if ( s[strlen( s ) - 1] == '\n' )
        s[strlen( s ) - 1] = '\0';


      // Only insert function names that are not commented out.
      if ( strncmp( s, "//", 2 ) != 0 ) {
        // Check, if line is a command.
        pszSecondString = strchr( s, '=' );
        if ( pszSecondString != 0 ) {
          for ( unsigned int i = 0; i < strlen( s ); i++ ) {
            if ( s[i] >= 'A' && s[i] <= 'Z' )
              s[i] += 32;
            else

            if ( s[i] == 'Ä' )
              s[i]='ä';
            else

            if ( s[i] == 'Ö' )
              s[i]='ö';
            else

            if ( s[i] == 'Ü' )
              s[i]='ü';
          }

          *pszSecondString++ = '\0';

          if ( strcmp( s, "showenterexit" ) == 0 ) {
            if ( strcmp( pszSecondString, "off" ) == 0 )
              bShowFuncStart = false;
            else

            if ( strcmp( pszSecondString, "on" ) == 0 )
              bShowFuncStart = true;
          } else

          if ( strcmp( s, "ignorefunctionname" ) == 0 ) {
            if ( strcmp( pszSecondString, "on" ) == 0 )
              bIgnoreFunctionName = true;
            else

            if ( strcmp( pszSecondString, "off" ) == 0 )
              bIgnoreFunctionName = false;
          }
        } else {
          mapFunctions[ string( s ) ] = 0;
          mapFiles[ string( s ) ] = string( logfilename );
        }

        #ifdef TABROW
        if ( strlen( s ) > iMaxFunctionNameLength )
          iMaxFunctionNameLength = strlen( s );
        #endif
      }
    }
  }

  if ( datafile )
    fclose( datafile );

};



//! For compatibility reasons.
void dinit( const char *datafilename )
{
  dinitfile( datafilename, "cout" );
};


/*!
   Closes all open file descriptors; is not neccessary,
   because every buffer is flushed after every function exit via DRETURN()
*/
void dclose()
{
  map<string, ostream *>::iterator iter;


  for ( iter = logFiles.begin(); iter != logFiles.end(); iter++ ) {
    #ifdef DEBUGDEBUG
    cout << "DEBUGMACROS::dclose: Closing debug output file " << (*iter).first
         << endl;
    #endif

    (*(*iter).second).flush();

    if ( (*iter).first != string( "cout" ) )
      delete (*iter).second;
  }

  for ( iter = logFiles.begin(); iter != logFiles.end(); iter=logFiles.begin() )
    logFiles.erase( iter );
};


/*!
   Should be called at the beginning of every function which produces debug
   output. Function calls are organized on a stack, so it is necessary to
   call dend() or dreturn() at the end of every function.
*/
void dstart( const char *szName, const char *activationName )
{
  if ( logFiles.empty() )
  {
    #ifdef DEBUGDEBUG
    cout << "DEBUGMACROS::dstart: (WARNING) No log files initialized, calling dinitfile" << endl;
    #endif
    dinitfile( "devnull", "devnull" );
  }

  string szNameString( szName );
  string activationNameString( activationName );

  #ifdef DEBUGDEBUG
  if ( mapFunctions.empty() != false )
    cout << "DEBUGMACROS::WARNING (Tool/dstart): No functions initialized\n";
  cout << "DEBUGMACROS: Entered dstart(" << szName << ", " << activationName
       << ") and writing to " << mapFiles[ activationNameString ] << endl;
  #endif

  // Add a new function call to the data structure.
  stackFunctionCalls.push( szNameString );
  stackActivationNames.push( activationNameString );

  // Test if this is a function not to debug
  if ( mapFunctions.find( activationNameString ) == mapFunctions.end() )
    return;

  ostream *out = logFiles[ mapFiles[ activationNameString ] ];
  
  if ( bShowFuncStart &&
       mapFunctions.find( activationNameString ) != mapFunctions.end() ) {
    dprint_prologue();

    string debugOutputFileName = mapFiles[activationName];
    
    if ( strcmp( debugOutputFileName.c_str(), "cout" ) == 0 )
      cout << "\33[32m"; /* Change text-color to green */
    *out << "--> enter ";
    if ( strcmp( debugOutputFileName.c_str(), "cout" ) == 0 )
      cout << "\33[0m"; /* Reset text-color */

    *out << szNameString;
    
    #ifdef TABROW
    for ( unsigned int i = 0;
            i < iMaxFunctionNameLength - szNameString.length() + 2;
            i++ )
      *out << "-";
    #endif
    
    *out << " (" << stackFunctionCalls.size() - 1 << ")\n";
  }
};


void dend()
{
  if ( stackFunctionCalls.size() == 0 ) {
    cerr << "DEBUGMACROS::Error: Stack is empty, but you called dend()!"
         << endl;
    #ifdef DEBUGDEBUG
    exit(-1);
    #endif
    return;
  }

  #ifdef DEBUGDEBUG
  cout << "DEBUGMACROS: Entered dend(stack=" << stackFunctionCalls.size()
       << ")" << endl;
  #endif

  string stackTop = stackFunctionCalls.top();
  string activationName = stackActivationNames.top();

  // Test if this is a function not to debug
  if ( mapFunctions.find( activationName ) == mapFunctions.end() )
  {
    stackFunctionCalls.pop();
    stackActivationNames.pop();
    return;
  }

  ostream *out = logFiles[mapFiles[activationName]];

  if ( stackFunctionCalls.size() > 0 ) {
    if ( bShowFuncStart && dtest_fu_name( 1 ) ) {
      dprint_prologue();

      string debugOutputFileName = mapFiles[ activationName ];
      
      if ( strcmp( debugOutputFileName.c_str(), "cout") == 0 )
        cout << "\33[31m"; /* Change text-color to red */
      *out << "<--  exit ";
      if ( strcmp( debugOutputFileName.c_str(), "cout") == 0 )
        cout << "\33[0m"; /* Reset text-color */

      *out << stackTop;

      #ifdef TABROW
      for ( unsigned int i = 0;
            i < iMaxFunctionNameLength - stackFunctionCalls.top().length() + 2;
            i++)
        *out << "-";
      #endif

      *out << " (" << stackFunctionCalls.size() - 1 << ")\n";
    }

    stackFunctionCalls.pop();
    stackActivationNames.pop();

    #ifdef DEBUGDEBUG
    cout << "DEBUGMACROS: ...and removed function " << stackTop
         << " from stack (" << stackFunctionCalls.size() << ")" << endl;
    #endif
  } else {

    #ifdef DEBUGDEBUG
    cout << "DEBUGMACROS::WARNING (Debug/dend): Unexpected exit\n";
    #endif
  }

  // Flush buffer to ensure everything is written before exit
  if ( mapFiles[activationName] != "cout" )
    (*out).flush();

  if ( bDoAutoDestruction )
    ddestroy();
};


void ddestroy()
{
  for ( map<string, ostream *>::iterator it = logFiles.begin();
        it != logFiles.end(); it++ ) {
    string theFile = (*it).first;
    ostream *theStream = (*it).second;


    if ( theFile != "cout" ){
      theStream->flush();
      delete( theStream );
    }
  }

  logFiles.clear();

  bDoAutoDestruction = true;
};


int dbgprint( int iDLevel )
{
  #ifdef DEBUGDEBUG
  if ( mapFunctions.empty() != false )
    cout << "WARNING (Debug/dout): No functions initialized\n";
  if ( stackFunctionCalls.empty() != false )
    cout << "WARNING (Debug/dout): No function entered\n";
  #endif

  if ( dtest_fu_name( iDLevel ) ) {
    dprint_prologue();
    cout << stackFunctionCalls.top().c_str();

    #ifdef TABROW
    for ( unsigned int i = 0;
          i < iMaxFunctionNameLength - stackFunctionCalls.top().length() + 2;
          i++)
      cout << "-";
    #endif

    cout << " (" << stackFunctionCalls.size() - 1 << "): ";
    return( 1 );
  }

  return 0;
};


ostream *dout( int iDLevel )
{
  #ifdef DEBUGDEBUG
  if ( mapFunctions.empty() != false )
    cout << "DEBUGMACROS::WARNING (Debug/dout): No functions initialized\n";
  if ( stackFunctionCalls.empty() != false )
    cout << "DEBUGMACROS::WARNING (Debug/dout): No function entered\n";
  cout << "DEBUGMACROS::Writing debug info to ";
  #endif

  if (( stackActivationNames.size() == 0 ) ||
     ( mapFunctions.find( string( stackActivationNames.top() ) ) == mapFunctions.end() )) {
    
    cout << "DEBUGMACROS::dout: WARNING: you called dout() directly, better use DOUT()\n"
         << endl;
    return logFiles[ "cout" ];
  }
  #ifdef DEBUGDEBUG
  else
    cout << "DEBUGMACROS::Writing debug info to "
       << mapFiles[ stackActivationNames.top() ] << endl;
  #endif

  if( dtest_fu_name( iDLevel ) )
  {
    dprint_prologue();
    return( logFiles[ mapFiles[ stackActivationNames.top() ] ] );
  }

  #ifdef DEBUGDEBUG
  cout << "DEBUGMACROS::dout: WARNING: you called dout() directly, better use DOUT()\n"
       << endl;
  #endif

  return logFiles[ "cout" ];
};


void dbgprintf( const char *format, ... )
{
  if ( !dtest_fu_name( 1 ) )
    return;

  va_list argument_list;
  va_start( argument_list, format );

  // Print desired output to a string
  char temp[512];
  vsnprintf( temp, 512, format, argument_list );
  va_end( argument_list );

  (*dout( 1 )) << temp;
};


int dtest_fu_name( int iDLevel )
{
  #ifdef DEBUGDEBUG
  if ( iDLevel < 0 )
    cout << "DEBUGMACROS::WARNING (Debug/dtest_fu_name):"
         << "DebugLevel in macro parameter <= 0" << endl;
  #endif

  // If function name is ignored, then iDLevel=0 produces output.
  if ( ( iDLevel == 0 ) && ( bIgnoreFunctionName != false ) )
    return( 1 );

  // Any DSTART called and function is declared in config file?
  if ( ( stackFunctionCalls.size() > 0 ) &&
       ( mapFunctions.find( stackActivationNames.top() ) !=
         mapFunctions.end() ) ) {
    // Debug level correct?
    if ( ( iDLevel >= 0 ) &&
         ( ( mapFunctions[stackActivationNames.top() ] == 0 ) ||
           ( iDLevel <= mapFunctions[ stackActivationNames.top() ] ) ) )
      return( 1 );
  }

  return( 0 );
};


void dthrow_assert( int nLineNo, const char *pcszFileName, const char *pcszDate,
                    const char *pcszExpression, const char *pcszFunName,
                    const char *pcszInfo )
{
  printf( "\nDebug Assertion:\n" );
  printf( "File:\t %s\n", pcszFileName );
  printf( "Func:\t %s\n", pcszFunName );
  printf( "Line:\t %d\n", nLineNo );
  printf( "Date:\t %s\n", pcszDate );
  printf( "Expr:\t %s\n", pcszExpression );

  if ( pcszInfo != 0 )
    printf( "Info:\t %s\n\n", pcszInfo );
  else
    printf( "\n" );

  abort();
};


void dprint_prologue()
{
  if ( !dtest_fu_name( 1 ) )
    return;

  string spaces = "";
  for ( unsigned int i = 1; i < stackFunctionCalls.size(); i++ )
    spaces += "  ";
  (*logFiles[ mapFiles[ stackActivationNames.top() ] ]) << spaces;
};


const char *substitute_environment_variables( string filename )
{
  bool done_subst = false;
  unsigned int pos_dollar;
  unsigned int pos_slash;
  char *env_charp;
  string subst_value;
  string env_var;


  while ( !done_subst ) {
    // Look for a $ character in the filename.
    pos_dollar = filename.find_first_of( "$" );

    if ( pos_dollar == string::npos )
      // No $ character found -> done!
      done_subst = true;
    else {
      // $ character found -> perform substitution.

      // Look for next slash.
      pos_slash = filename.find_first_of( "/", pos_dollar );

      // env_var contains the NAME of the environment variable,
      // without the leading $ character and without the trailing slash
      env_var = filename.substr( pos_dollar + 1, pos_slash - 1 );

      // Look for this variable in the environment.
      env_charp = getenv( env_var.c_str() );
      if ( env_charp != 0 ) {
        // The variable was found in the environment.
        subst_value = env_charp;

        // Add one to size due to missing $ character.
        filename =
          filename.replace( pos_dollar, env_var.size() + 1, subst_value );

        #ifdef DEBUGDEBUG
        cout << "substitution: " << subst_value << endl;
        cout << "new filename: " << filename << endl;
        #endif
      }
    }
  }

  return( filename.c_str() );
};
