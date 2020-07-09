#ifndef ENGINE_SERVER_DATABASES_MYSQL_H
#define ENGINE_SERVER_DATABASES_MYSQL_H

#include "connection.h"
#include <base/tl/threading.h>

#include <atomic>
#include <memory>

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>

class CMysqlConnection : public IDbConnection
{
public:
	CMysqlConnection(
			const char *pDatabase,
			const char *pPrefix,
			const char *pUser,
			const char *pPass,
			const char *pIp,
			int Port,
			bool Setup);
	virtual ~CMysqlConnection();

	virtual CMysqlConnection *Copy();

	virtual Status Connect();
	virtual void Disconnect();

	virtual void Lock(const char *pTable);
	virtual void Unlock();

	virtual void PrepareStatement(const char *pStmt);

	virtual void BindString(int Idx, const char *pString);
	virtual void BindInt(int Idx, int Value);

	virtual bool Step();

	virtual bool IsNull(int Col) const;
	virtual float GetFloat(int Col) const;
	virtual int GetInt(int Col) const;
	virtual void GetString(int Col, char *pBuffer, int BufferSize) const;
	virtual int GetBlob(int Col, unsigned char *pBuffer, int BufferSize) const;

private:
	std::unique_ptr<sql::Connection> m_pConnection;
	std::unique_ptr<sql::PreparedStatement> m_pPreparedStmt;
	std::unique_ptr<sql::Statement> m_pStmt;
	std::unique_ptr<sql::ResultSet> m_pResults;
	bool m_NewQuery;

	// copy of config vars
	char m_aDatabase[64];
	char m_aUser[64];
	char m_aPass[64];
	char m_aIp[64];
	int m_Port;
	bool m_Setup;

	std::atomic_bool m_InUse;
	bool m_Locked;

	static lock m_SqlDriverLock;

};

#endif // ENGINE_SERVER_DATABASES_MYSQL_H
