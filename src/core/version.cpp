/*******************************************************************************************

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

 [Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php

*******************************************************************************************/

#include <string>
#include "version.h"

/** Used for Visual Reference Only **/
const std::string CLIENT_NAME("Nexus");

/* The version number */
const std::string CLIENT_VERSION("2.5.5 [rc2]");

/* The interface used Qt, CLI, or Tritium) */
#if defined QT_GUI
    const std::string CLIENT_INTERFACE("Qt");
#elif defined TRITIUM_GUI
    const std::string CLIENT_INTERFACE("Tritium Beta");
#else
    const std::string CLIENT_INTERFACE("CLI");
#endif

/* The database type used (LevelDB, Berkeley DB, or Lower Level Database) */
#if defined USE_LLD
    const std::string CLIENT_DATABASE("[LLD]");
#elif defined USE_LEVELDB
    const std::string CLIENT_DATABASE("[LVD]");
#else
    const std::string CLIENT_DATABASE("[BDB]");
#endif

/* The Architecture (32-Bit or 64-Bit) */
#ifdef x64
    const std::string BUILD_ARCH = "[x64]";
#else
    const std::string BUILD_ARCH = "[x86]";
#endif

const std::string CLIENT_BUILD(CLIENT_VERSION + " " + CLIENT_INTERFACE + " " + CLIENT_DATABASE + BUILD_ARCH);
const std::string CLIENT_DATE(__DATE__ " " __TIME__);

/** Used to determine the current features available on the local database */
 const int DATABASE_VERSION =
                    1000000 * DATABASE_MAJOR
                  +   10000 * DATABASE_MINOR
                  +     100 * DATABASE_REVISION
                  +       1 * DATABASE_BUILD;

/** Used to determine the features available in the Nexus Network **/
const int PROTOCOL_VERSION =
                   1000000 * PROTOCOL_MAJOR
                 +   10000 * PROTOCOL_MINOR
                 +     100 * PROTOCOL_REVISION
                 +       1 * PROTOCOL_BUILD;

/** Used to Lock-Out Nodes that are running a protocol version that is too old,
    Or to allow certain new protocol changes without confusing Old Nodes. **/
const int MIN_PROTO_VERSION = 10000;
