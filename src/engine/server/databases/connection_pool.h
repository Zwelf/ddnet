#ifndef ENGINE_SERVER_DATABASES_CONNECTION_POOL_H
#define ENGINE_SERVER_DATABASES_CONNECTION_POOL_H

#include "connection.h"

#include <base/tl/threading.h>
#include <memory>
#include <vector>

struct ISqlData
{
	virtual ~ISqlData() {};
};

class CDbConnectionPool
{
public:
	CDbConnectionPool();
	~CDbConnectionPool();

	enum Mode
	{
		READ,
		WRITE,
		WRITE_BACKUP,
		NUM_MODES,
	};

	void RegisterDatabase(std::unique_ptr<IDbConnection> pDatabase, Mode DatabaseMode);

	void Execute(
			bool (*pFuncPtr) (IDbConnection *, const ISqlData *),
			std::unique_ptr<ISqlData> pSqlRequestData,
			const char *pName);
	// writes to WRITE_BACKUP server in case of failure
	void ExecuteWrite(
			bool (*pFuncPtr) (IDbConnection *, const ISqlData *, bool),
			std::unique_ptr<ISqlData> pSqlRequestData,
			const char *pName);

	void Shutdown();

private:
	// each mode can have multiple database credentials, which can have multiple open connections
	// example
	// READ
	//  - MysqlServer1
	//     - open connection1
	//     - open connection2
	// WRITE
	//  - MysqlServer2
	//     - open connection1
	//     - open connection2
	//     - open connection3
	// WRITE_BACKUP
	//  - SqliteServer1
	//     - open connection1
	std::vector< std::vector< std::unique_ptr< IDbConnection >>> m_aaapDbConnections[NUM_MODES];
	lock m_ConnectionLookupLock[NUM_MODES];
};

#endif // ENGINE_SERVER_DATABASES_CONNECTION_POOL_H
