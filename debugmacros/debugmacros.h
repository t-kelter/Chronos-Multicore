/*

   This header file belongs to the

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


/* Enable the follwing for debuggin purposes */
//#define DEBUGDEBUG

#ifndef DEBUGMACROS_H
#define DEBUGMACROS_H

#ifdef __cplusplus
 #include <iostream>
 #include <string>
 #include <stdio.h>
#endif


// If debugmacros are not required, we can disable the complete DEBUGMACRO functionality
#ifndef DEBUGMACROS

#define DINITDEBUGMACROS(isfirstinit_var,x)    do{}while(0)
#define DINIT(...)                             do{}while(0)
#define DINITFILE(config,log)                  do{}while(0)
#define DASSERT(expr)                          do{}while(0)
#define DMASSERT(expr, info)                   do{}while(0)
#define THROW(args)                            do{}while(0)
#define DEMPTY()                               do{}while(0)
#define DSIZE()                                do{}while(0)
#define DSTART(name)                           do{}while(0)
#define DSTART_EXT(name1, name2)               do{}while(0)
#define DEND()                                 do{}while(0)
#define DRETURN(retval)                        do{ return retval; }while(0)
#define DRET()                                 do{ return; }while(0)
#define DTEST()                                do{ 0 }while(0)
#define DOUT(...)                              do{}while(0)
#define DOUTL(...)                             do{}while(0)
#define DPRINT(args)                           do{}while(0)
#define DPRINTL(args)                          do{}while(0)
#define DPRINT(args)                           do{}while(0)
#define DPRINTL(args)                          do{}while(0)
#define DACTION(action_part)                   do{}while(0)
#define DACTIONELSE(action_part, else_part)    do{ else_part; }while(0)
#define DACTIONL(action_part, arglevel)        do{}while(0)
#define DIFTHEN(if_part, then_part)            do{}while(0)
#define DIFTHENL(if_part, then_part, arglevel) do{}while(0)
#define WAITFORKEY(arglevel)                   do{}while(0)

#else

// Define output to go to the standard out by default - override this when needed
#define DEBUGMACRO_LOG "cout"

#define DASSERT(expr) do {if (! (expr)) {                               \
                         dthrow_assert(__LINE__, __FILE__, __DATE__, #expr, __func__, NULL);}}while(0)

#define DMASSERT(expr, info) do {if (!(expr)) {\
            dthrow_assert(__LINE__, __FILE__, __DATE__, #expr, __func__, info);}}while(0)

#define THROW(args) do{ cerr << endl << endl << "ERROR in file     " << __FILE__; \
                      cerr << endl <<         "         function " << __func__; \
                      cerr << endl <<         "         line     " << __LINE__; \
                      throw args; exit(1); }while(0)

/* Using C++ you can call DINIT with one or two paramaters.
   In C you have to call DINIT if you use one parameter or
   DINITFILE if you want to log to a file
*/
#ifdef __cplusplus
  #define DINIT(...) dinitfile(__VA_ARGS__)

  #define DINITDEBUGMACROS( isfirstinit_var, conf_file )\
  if ( isfirstinit_var ) {\
    std::string conffile = DEBUGMACRO_CONF;\
    conffile += conf_file;\
    dinitfile( conffile.c_str(), DEBUGMACRO_LOG );\
    isfirstinit_var = 0; }
#else
  #define DINITFILE(config,log) dinitfile(config,log)
  #define DINIT(args) dinit(args)

  #define DINITDEBUGMACROS( isfirstinit_var, conf_file )\
  if ( isfirstinit_var ) {\
    char conffile[512];\
    strcpy( conffile, DEBUGMACRO_CONF );\
    strcat( conffile, conf_file );\
    dinitfile( conffile, DEBUGMACRO_LOG );\
    isfirstinit_var = 0; }
#endif

#define DEMPTY() (dempty())
#define DSIZE() (dsize())
#define DSTART(name) (dstart(name, name))
#define DSTART_EXT(name1, name2) (dstart(name1, name2))
#define DEND() (dend())

#define DRETURN(retval) do{dend(); return retval;}while(0)
#define DRET() do{dend(); return ;}while(0)

#define DTEST() dtest_fu_name(1)

#ifndef __cplusplus
#define DOUT(format, ...)  do{if (dtest_fu_name(1)) dbgprintf(format, ## __VA_ARGS__);}while(0)
#else
  #define DOUT(...) do{if (dtest_fu_name(1)) *(dout(1)) << __VA_ARGS__;}while(0)
#endif

#ifndef __cplusplus
  #define DOUTL(args, arglevel, ...) do{if (dtest_fu_name(arglevel)) dbgprintf(format, ## __VA_ARGS__);}while(0)
#else
#define DOUTL(args, arglevel) do{*(dout(arglevel)) << args;}while(0)
#endif

/* Some older projects use debug.h which already defines DPRINT.
   */
#ifndef DPRINT
#ifndef __cplusplus
#define DPRINT(args) do {if (dprint(1)) printf args }while(0)
#else
#define DPRINT(args) do { if (dprint(1)) { cout << args; } } while(0)
#endif
#endif

#ifndef __cplusplus
#define DPRINTL(args,arglevel) do {if (dprint(arglevel)) printf args }while(0)
#else
#define DPRINTL(args,arglevel)  do{*(dout(arglevel)) << args}while(0)
#endif

#define DACTION(action_part) do{if (dtest_fu_name(1)) { action_part; }}while(0)


// Caveat! The DACTIONELSE rule actually leaves some stuff in the code
// even when no DEBUGMACROS is being defined! However, it is probable
// that this is the desired behavior, since the user wants to perform
// an action when debugging is activated, and a different action when
// debugging is disabled. Not doing anything is probably not the
// user's intention, since then he would be using DACTION

#define DACTIONELSE(action_part, else_part) do {if (dtest_fu_name(1)) { action_part; } else { else_part; };}while(0)
#define DACTIONL(action_part, arglevel) do {if (dtest_fu_name(arglevel)) { action_part; }}while(0)
#define DIFTHEN(if_part, then_part) do {if (dtest_fu_name(1)) { if(if_part) { then_part; } }}while(0)
#define DIFTHENL(if_part, then_part, arglevel) do {if (dtest_fu_name(arglevel)) { if(if_part) { then_part; } }}while(0)

#define WAITFORKEY(arglevel) do{if(dtest_fu_name(arglevel)) { UserInteraction(); }}while(0)

#endif



#ifdef __cplusplus
  extern "C" bool dempty();
  extern "C" unsigned int dsize();
  extern "C" void dinitfile( const char *configfile,
                             const char *logfilename = "cout" );
  extern "C" void dinit( const char *datafile );
  extern "C" void dclose();
  extern "C" void dstart( const char *, const char * );
  extern "C" void dend();
  extern "C" void ddestroy();
  extern std::ostream *dout( int iDLevel );
  extern "C" void dbgprintf( const char *format, ... );
  extern "C" int  dbgprint( int iDLevel );
  extern "C" int  dtest_fu_name( int iDLevel );
  extern "C" void dprint_prologue();
  extern "C" void UserInteraction();
  extern "C" void dthrow_assert( int nLineNo,
                             const char *pcszFileName,
                             const char *pcszDate,
                             const char *pcszExpression,
                             const char *pcszFunName,
                             const char *pcszInfo );
//const char *substitute_environment_variables( std::string filename );
#else
  _Bool dempty();
  unsigned int dsize();
  void dinitfile( const char *configfile, const char *logfilename );
  void dinit( const char *datafile );
  void dclose();
  void dstart( const char *, const char * );
  void dend();
  void ddestroy();
  void dbgprintf( const char *format, ... );
  int  dbgprint( int iDLevel );
  int  dtest_fu_name( int iDLevel );
  void dprint_prologue();
  void UserInteraction();
  void dthrow_assert(int nLineNo,
                            const char* pcszFileName,
                            const char* pcszDate,
                            const char* pcszExpression,
                            const char* pcszFunName,
                            const char* pcszInfo);
//const char *substitute_environment_variables( std::string filename );
#endif //__cplusplus


// Setting AUTODEBUG makes DEND obsolete. As soon as the scope is left
// dend is invoked.
#ifdef AUTODEBUG
 #ifdef __cplusplus
  #undef DSTART
  #undef DRETURN
  #undef DRET
  #undef DEND
  #define DSTART(N) const Autodend __autodebug(N)
    #define DRETURN(retval) return retval
  #define DRET() DRETURN()
    #define DEND() do{}while(0)
    struct Autodend {
        Autodend(char* name)
      {
        dstart(name, name);
      }
        ~Autodend()
      {
        dend();
      }
    };
 #endif
#endif

#define MAXDEBUGFILENAME 256


#endif //DEBUGMACROS_H
