use ffi::{ResultMap, ScoreResult};
use std::sync::mpsc;
use worker::SqliteWorker;

mod mysql;
mod sqlite;
mod worker;

/// Setup instructions:
#[derive(Clone, Debug, PartialEq, Eq)]
struct MysqlConfig {
    /// Mysql database
    database: String,
    /// database table prefix. Not commonly used anymore. Sqlite uses `record`
    /// due to DDNet server using `record` by default. Use `record` if you
    /// don't have good reasons to choose a different prefix.
    prefix: String,
    /// username credentials for authenticating to database
    username: String,
    /// password credentials for authenticating to database
    password: String,
    ip: String,
    port: u16,
    /// MYSQL_OPT_BIND
    /// The network interface from which to connect to the server. This is used
    /// when the client host has multiple network interfaces. The argument is a
    /// host name or IP address (specified as a string).
    bind_address: String,
    /// If true tables are created on startup. Otherwise tables are expected
    /// to be setup prior.
    setup: bool,
}

#[derive(Clone)]
enum Request {
    SetSqlite(String),
    SetMysql(MysqlConfig),
    RemoveMysql,
    GetDatabases,

    // Testing messages
    #[cfg(test)]
    SqliteDump,
    #[cfg(test)]
    MysqlDump,
}
#[derive(Clone, Debug, PartialEq, Eq)]
enum Response {
    Databases(Option<String>, Option<MysqlConfig>),
    #[cfg(test)]
    SqliteDump,
    #[cfg(test)]
    MysqlDump,
}

impl Response {
    fn score_result(&self) -> ScoreResult {
        match &self {
            _ => ScoreResult::None,
        }
    }
}

pub struct DbPool {
    sender: Option<mpsc::Sender<Request>>,
    receiver: mpsc::Receiver<Response>,
    last_response: Option<Response>,
}

fn db_pool() -> Box<DbPool> {
    let (request_sender, request_receiver) = mpsc::channel();
    let (response_sender, response_receiver) = mpsc::channel();
    SqliteWorker::start(request_receiver, response_sender);
    Box::new(DbPool {
        sender: Some(request_sender),
        receiver: response_receiver,
        last_response: None,
    })
}

impl Drop for DbPool {
    fn drop(&mut self) {
        self.worker_shutdown();
    }
}

impl DbPool {
    /// Database management.
    fn worker_set_sqlite(&mut self, path: String) {
        if let Some(sender) = &self.sender {
            sender.send(Request::SetSqlite(path));
        }
    }
    fn worker_set_mysql(&mut self, config: MysqlConfig) {
        if let Some(sender) = &self.sender {
            sender.send(Request::SetMysql(config));
        }
    }
    fn worker_remove_mysql(&mut self) {
        if let Some(sender) = &self.sender {
            sender.send(Request::RemoveMysql);
        }
    }
    fn worker_print(&mut self) {
        if let Some(sender) = &self.sender {
            sender.send(Request::GetDatabases);
        }
    }
    fn worker_shutdown(&mut self) {
        self.sender = None
    }

    /// Check whether a result from a previous query arrived
    fn get_next_result(&mut self) -> ScoreResult {
        self.last_response = None;
        match self.receiver.try_recv() {
            Ok(response) => {
                let result = response.score_result();
                self.last_response = Some(response);
                result
            }
            Err(mpsc::TryRecvError::Disconnected) => ScoreResult::ShutdownDone,
            Err(mpsc::TryRecvError::Empty) => ScoreResult::None,
        }
    }

    #[cfg(test)]
    pub fn get_next_blocking(&mut self) -> Option<Response> {
        self.last_response = self.receiver.recv().ok();
        self.last_response.clone()
    }

    // DDNet database interactions
    // void LoadBestTime();
    // void MapInfo(int ClientId, const char *pMapName);
    // void MapVote(int ClientId, const char *pMapName);
    // void LoadPlayerData(int ClientId, const char *pName = "");
    // void LoadPlayerTimeCp(int ClientId, const char *pName = "");
    // void SaveScore(int ClientId, int TimeTicks, const char *pTimestamp, const float aTimeCp[NUM_CHECKPOINTS], bool NotEligible);

    // void SaveTeamScore(int Team, int *pClientIds, unsigned int Size, int TimeTicks, const char *pTimestamp);

    // void ShowTop(int ClientId, int Offset = 1);
    // void ShowRank(int ClientId, const char *pName);

    // void ShowTeamTop5(int ClientId, int Offset = 1);
    // void ShowPlayerTeamTop5(int ClientId, const char *pName, int Offset = 1);
    // void ShowTeamRank(int ClientId, const char *pName);

    // void ShowTopPoints(int ClientId, int Offset = 1);
    // void ShowPoints(int ClientId, const char *pName);

    // void ShowTimes(int ClientId, const char *pName, int Offset = 1);
    // void ShowTimes(int ClientId, int Offset = 1);

    // void RandomMap(int ClientId, int Stars);
    // void RandomUnfinishedMap(int ClientId, int Stars);
    /// Returns either a ResultMap if possible or a `ResultDirectMessage`
    fn query_random_map(&mut self, player_uid: u64, category: String, stars: i32) {}
    fn query_random_unfinished_map(
        &mut self,
        player_uid: u64,
        player_name: String,
        category: String,
        stars: i32,
    ) {
    }

    // void SaveTeam(int ClientId, const char *pCode, const char *pServer);

    // void LoadTeam(const char *pCode, int ClientId);
    // void GetSaves(int ClientId);

    fn result_map(&self) -> ResultMap {
        todo!()
    }
    // Some results contain additional data
}

#[cxx::bridge]
mod ffi {
    /// Result from Database request. Not all request need to result in a response.
    /// Some results have more data that can get requested.
    #[derive(Copy, Clone, Debug, PartialEq, Eq)]
    enum ScoreResult {
        /// No result currently Exist
        None,
        ShutdownDone,
        RandomMap,
    }
    struct ResultMap {
        pub player_uid: u64,
        pub map_name: [u8; 64],
    }
    struct ResultDirectMessage {
        pub player_uid: u64,
        pub msg: String,
    }
    struct ResultTeamMessage {
        pub team: i32,
        pub msg: String,
    }
    struct ResultAllMessage {
        /// attach `player_uid` for rate limiting
        pub player_uid: i32,
        pub msg: String,
    }
    struct ResultBroacast {
        pub msg: String,
    }
    struct ResultPlayerInfo {
        pub player_uid: i32,
        /// score is attached to player name. Discard results if changed
        pub name: String,
        pub time_cp: [f32; 25],
        pub birthday: bool,
    }
    extern "Rust" {
        type DbPool;

        fn db_pool() -> Box<DbPool>;

        // Called to terminate threadpool safely
        // All write operations are persisted to disk
        fn worker_shutdown(&mut self);
        fn worker_set_sqlite(&mut self, path: String);

        // Interface for Database
        fn query_random_map(&mut self, player_uid: u64, category: String, stars: i32);

        fn get_next_result(&mut self) -> ScoreResult;

        fn result_map(&self) -> ResultMap;
    }
}

#[cfg(test)]
mod test {
    use super::*;
    const SQLITE_MEMORY_PATH: &'static str = "file::memory:?cache=shared";

    #[test]
    fn setup_shutdown() {
        let mut p = db_pool();
        // immidiate shutdown
        p.worker_shutdown();
        assert_eq!(p.get_next_blocking(), None);
        assert_eq!(p.get_next_result(), ScoreResult::ShutdownDone);
    }

    #[test]
    fn get_empty_databases() {
        let mut p = db_pool();
        assert_eq!(p.get_next_result(), ScoreResult::None);
        p.worker_print();
        assert_eq!(p.get_next_blocking(), Some(Response::Databases(None, None)));
    }

    #[test]
    fn setup_sqlite_only() {
        let mut p = db_pool();
        assert_eq!(p.get_next_result(), ScoreResult::None);
        p.worker_set_sqlite(SQLITE_MEMORY_PATH.to_owned());
        p.worker_print();
        assert_eq!(
            p.get_next_blocking(),
            Some(Response::Databases(
                Some(SQLITE_MEMORY_PATH.to_owned()),
                None
            ))
        );
    }
    #[test]
    fn setup_mysql_only() {
        let mut p = db_pool();
        assert_eq!(p.get_next_result(), ScoreResult::None);
        let mysql_config = MysqlConfig {
            database: "ddnet_test".to_owned(), // randomize to parallize test
            prefix: "record".to_owned(),       // TODO: randomize for test?
            username: "root".to_owned(),
            password: "password".to_owned(),
            bind_address: "127.0.0.1".to_owned(),
            ip: "127.0.0.1".to_owned(),
            port: 13306,
            setup: true,
        };
        p.worker_set_mysql(mysql_config.clone());
        p.worker_print();
        assert_eq!(
            p.get_next_blocking(),
            Some(Response::Databases(None, Some(mysql_config)))
        );
    }
    #[test]
    fn setup_mysql_sqlite() {
        let mut p = db_pool();
        assert_eq!(p.get_next_result(), ScoreResult::None);
        let mysql_config = MysqlConfig {
            database: "ddnet_test".to_owned(),
            prefix: "record".to_owned(),
            username: "root".to_owned(),
            password: "password".to_owned(),
            bind_address: "127.0.0.1".to_owned(),
            ip: "127.0.0.1".to_owned(),
            port: 13306,
            setup: true,
        };
        p.worker_set_mysql(mysql_config.clone());
        p.worker_set_sqlite(SQLITE_MEMORY_PATH.to_owned());
        p.worker_print();
        assert_eq!(
            p.get_next_blocking(),
            Some(Response::Databases(
                Some(SQLITE_MEMORY_PATH.to_owned()),
                Some(mysql_config)
            ))
        );
    }
    #[test]
    fn setup_sqlite_mysql() {
        let mut p = db_pool();
        assert_eq!(p.get_next_result(), ScoreResult::None);
        let mysql_config = MysqlConfig {
            database: "ddnet_test".to_owned(), // randomize to parallize test
            prefix: "record".to_owned(),       // TODO: randomize for test?
            username: "root".to_owned(),
            password: "password".to_owned(),
            bind_address: "127.0.0.1".to_owned(),
            ip: "127.0.0.1".to_owned(),
            port: 13306,
            setup: true,
        };
        p.worker_set_mysql(mysql_config.clone());
        p.worker_set_sqlite(SQLITE_MEMORY_PATH.to_owned());
        p.worker_print();
        assert_eq!(
            p.get_next_blocking(),
            Some(Response::Databases(
                Some(SQLITE_MEMORY_PATH.to_owned()),
                Some(mysql_config)
            ))
        );
    }
    #[test]
    fn setup_mysql_sqlite_mysql() {
        let mut p = db_pool();
        assert_eq!(p.get_next_result(), ScoreResult::None);
        let mysql_config = MysqlConfig {
            database: "ddnet_test".to_owned(), // randomize to parallize test
            prefix: "record".to_owned(),       // TODO: randomize for test?
            username: "root".to_owned(),
            password: "password".to_owned(),
            bind_address: "127.0.0.1".to_owned(),
            ip: "127.0.0.1".to_owned(),
            port: 13306,
            setup: true,
        };
        p.worker_set_mysql(mysql_config.clone());
        p.worker_set_sqlite(SQLITE_MEMORY_PATH.to_owned());
        p.worker_print();
        assert_eq!(
            p.get_next_blocking(),
            Some(Response::Databases(
                Some(SQLITE_MEMORY_PATH.to_owned()),
                Some(mysql_config)
            ))
        );
    }
    // TODO: setup test framework to test in the following configurations:
    //  * Sqlite + Mysql
    //  * Mysql only
    //  * Sqlite only
    //  * Sqlite + Mysql (down)
    //    * Test that everything gets stored in SQLite
    //    * Test that everything gets transferred to Mysql when Mysql restarts
    //    * Test that everything gets transferred to Mysql when Mysql config changes to a valid one
    //    * Test that everything gets transferred to Mysql after Mysql only mode
    //  * None (extra):
    //    * Test that everything gets persistent after setting a valid Sqlite file
    //    * Test that everything gets persistent after setting a valid Mysql config
    //  * Being able to recover from panic in Sqlite and Mysql
}
