#include "MySQLPreparedStatements.h"

#include "Implementation/AMSDatabase.h"
#include "Implementation/IMSDatabase.h"

namespace database
{

void RegisterPreparedStatements()
{
    Implementation::RegisterIMSPreparedStatements();
    Implementation::RegisterAMSPreparedStatements();
}

} // namespace database

