#include "mysql.h"

#include <cppconn/driver.h>

lock CMysqlConnection::m_SqlDriverLock;

CMysqlConnection::CMysqlConnection(
		const char *pDatabase,
		const char *pPrefix,
		const char *pUser,
		const char *pPass,
		const char *pIp,
		int Port,
		bool Setup) :
	IDbConnection(pPrefix),
	m_NewQuery(false),
	m_Port(Port),
	m_Setup(Setup),
	m_InUse(false)
{
	str_copy(m_aDatabase, pDatabase, sizeof(m_aDatabase));
	str_copy(m_aUser, pUser, sizeof(m_aUser));
	str_copy(m_aPass, pPass, sizeof(m_aPass));
	str_copy(m_aIp, pIp, sizeof(m_aIp));
}

CMysqlConnection::~CMysqlConnection()
{
}

CMysqlConnection *CMysqlConnection::Copy()
{
	return new CMysqlConnection(m_aDatabase, m_aPrefix, m_aUser, m_aPass, m_aIp, m_Port, m_Setup);
}

IDbConnection::Status CMysqlConnection::Connect()
{
	if(m_InUse.exchange(true))
		return Status::IN_USE;

	m_NewQuery = true;
	if(m_pConnection != nullptr)
	{
		try
		{
			// Connect to specific database
			m_pConnection->setSchema(m_aDatabase);
			return Status::SUCCESS;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL Error: %s", e.what());
		}
		catch (const std::exception& ex)
		{
			dbg_msg("sql", "MySQL Error: %s", ex.what());
		}
		catch (const std::string& ex)
		{
			dbg_msg("sql", "MySQL Error: %s", ex.c_str());
		}
		catch (...)
		{
			dbg_msg("sql", "Unknown Error cause by the MySQL/C++ Connector");
		}

		m_InUse = false;
		dbg_msg("sql", "ERROR: SQL connection failed");
		return Status::ERROR;
	}

	try
	{
		m_pConnection.release();
		m_pPreparedStmt.release();
		m_pResults.release();

		sql::ConnectOptionsMap connection_properties;
		connection_properties["hostName"]      = sql::SQLString(m_aIp);
		connection_properties["port"]          = m_Port;
		connection_properties["userName"]      = sql::SQLString(m_aUser);
		connection_properties["password"]      = sql::SQLString(m_aPass);
		connection_properties["OPT_CONNECT_TIMEOUT"] = 10;
		connection_properties["OPT_READ_TIMEOUT"] = 10;
		connection_properties["OPT_WRITE_TIMEOUT"] = 20;
		connection_properties["OPT_RECONNECT"] = true;
		connection_properties["OPT_CHARSET_NAME"] = sql::SQLString("utf8mb4");
		connection_properties["OPT_SET_CHARSET_NAME"] = sql::SQLString("utf8mb4");

		// Create connection
		{
			scope_lock GlobalLockScope(&m_SqlDriverLock);
			sql::Driver *pDriver = get_driver_instance();
			m_pConnection.reset(pDriver->connect(connection_properties));
		}

		// Apparently OPT_CHARSET_NAME and OPT_SET_CHARSET_NAME are not enough
		PrepareStatement("SET CHARACTER SET utf8mb4;");
		Step();

		if(m_Setup)
		{
			char aBuf[128];
			// create database
			str_format(aBuf, sizeof(aBuf), "CREATE DATABASE IF NOT EXISTS %s CHARACTER SET utf8mb4", m_aDatabase);
			PrepareStatement(aBuf);
			Step();
		}

		// Connect to specific database
		m_pConnection->setSchema(m_aDatabase);
		dbg_msg("sql", "sql connection established");
		return Status::SUCCESS;
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("sql", "MySQL Error: %s", e.what());
	}
	catch (const std::exception& ex)
	{
		dbg_msg("sql", "MySQL Error: %s", ex.what());
	}
	catch (const std::string& ex)
	{
		dbg_msg("sql", "MySQL Error: %s", ex.c_str());
	}
	catch (...)
	{
		dbg_msg("sql", "Unknown Error cause by the MySQL/C++ Connector");
	}

	dbg_msg("sql", "ERROR: sql connection failed");
	return Status::ERROR;
}

void CMysqlConnection::Disconnect()
{
	m_InUse.store(false);
}

void CMysqlConnection::Lock()
{
}

void CMysqlConnection::Unlock()
{
}

void CMysqlConnection::PrepareStatement(const char *pStmt)
{
	m_pPreparedStmt.reset(m_pConnection->prepareStatement(pStmt));
	m_NewQuery = true;
}

void CMysqlConnection::BindString(int Idx, const char *pString)
{
	m_pPreparedStmt->setString(Idx, pString);
	m_NewQuery = true;
}

void CMysqlConnection::BindInt(int Idx, int Value)
{
	m_pPreparedStmt->setInt(Idx, Value);
	m_NewQuery = true;
}

bool CMysqlConnection::Step()
{
	if(m_NewQuery)
	{
		m_NewQuery = false;
		m_pResults.reset(m_pPreparedStmt->executeQuery());
	}
	return m_pResults->next();
}

bool CMysqlConnection::IsNull(int Col) const
{
	return m_pResults->isNull(Col);
}

float CMysqlConnection::GetFloat(int Col) const
{
	return (float)m_pResults->getDouble(Col);
}

int CMysqlConnection::GetInt(int Col) const
{
	return m_pResults->getInt(Col);
}

void CMysqlConnection::GetString(int Col, char *pBuffer, int BufferSize) const
{
	auto String = m_pResults->getString(Col);
	str_copy(pBuffer, String.c_str(), BufferSize);
}

int CMysqlConnection::GetBlob(int Col, unsigned char *pBuffer, int BufferSize) const
{
	auto Blob = m_pResults->getBlob(Col);
	Blob->read((char *)pBuffer, BufferSize);
	return Blob->gcount();
}
