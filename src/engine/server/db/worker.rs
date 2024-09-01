//! Worker handling SQL queries in separate threads
//!
//! There can either be only an `SqliteWorker` or additionally a `RemoteWorker`.
//! In case there exists only the SQLite worker
//!
//! Has two modes:
//!
//! 1. Sqlite only
//! 2. Mysql with Sqlite as backup server
//!
//! Takes care about making sure that write

use crate::{mysql::Mysql, sqlite::Sqlite, MysqlConfig, Request, Response};
use std::{sync::mpsc, thread};

pub struct SqliteWorker {
    receiver: mpsc::Receiver<Request>,
    sender: mpsc::Sender<Response>,
    sqlite: Option<(Sqlite, String)>,
    remote_worker: Option<SqliteWorkerRemoteInfo>,
}

/// Keep track of info from Remote worker within SqliteWorker
struct SqliteWorkerRemoteInfo {
    sender: mpsc::Sender<Request>,
    config: MysqlConfig,
}

impl SqliteWorker {
    /// Starts the database worker threads. Shutdown signal is when the Request sender
    /// channel gets closed.
    pub fn start(receiver: mpsc::Receiver<Request>, sender: mpsc::Sender<Response>) {
        thread::spawn(move || {
            let mut this = Self {
                receiver,
                sender,
                sqlite: None,
                remote_worker: None,
            };
            this.sqlite_worker();
        });
    }

    fn sqlite_worker(&mut self) {
        loop {
            match self.receiver.recv() {
                Ok(req) => {
                    // handle request in Sqlite worker writing to `_backup` tables if functional
                    // remote worker exists, otherwise use sqlite table as normal database.
                    self.handle_request(&req);
                    // handle request in remote_worker
                    if let Some(remote_worker) = &self.remote_worker {
                        if let Err(mpsc::SendError(_)) = remote_worker.sender.send(req.clone()) {
                            println!("error: remote worker thread paniced!");
                        }
                    }
                }
                // Got shutdown signal, terminate this thread.
                Err(mpsc::RecvError) => return,
            }
        }
    }

    fn handle_request(&mut self, req: &Request) {
        match req {
            Request::SetSqlite(path) => match Sqlite::open(path) {
                Ok(mut sqlite) => {
                    if self.remote_worker.is_some() {
                        sqlite.setup_backup();
                    }
                    self.sqlite = Some((sqlite, path.clone()));
                }
                Err(err) => println!("error adding sqlite {err}"),
            },
            Request::GetDatabases => {
                let sqlite = self.sqlite.as_ref().map(|s| s.1.clone());
                let mysql = self.remote_worker.as_ref().map(|r| r.config.clone());
                self.sender.send(Response::Databases(sqlite, mysql));
            }
            Request::SetMysql(config) => {
                // Close connection to previous worker by overwriting its
                // sender henceforth dropping it if it exists.
                // Start new thread. All following queries will go through
                // this worker.
                let (sender, receiver) = mpsc::channel();
                if let Some(sqlite) = self.sqlite.as_mut() {
                    sqlite.0.setup_backup();
                }
                RemoteWorker::start(config.clone(), receiver, self.sender.clone());
                self.remote_worker = Some(SqliteWorkerRemoteInfo {
                    config: config.clone(),
                    sender,
                });
            }
            Request::RemoveMysql => {
                self.remote_worker = None;
            }
            _ => todo!(),
        }
    }
}

struct RemoteWorker {
    config: MysqlConfig,
    mysql: Mysql,
    receiver: mpsc::Receiver<Request>,
    sender: mpsc::Sender<Response>,
}

impl RemoteWorker {
    pub fn start(
        config: MysqlConfig,
        receiver: mpsc::Receiver<Request>,
        sender: mpsc::Sender<Response>,
    ) {
        thread::spawn(move || {
            let mut this = Self {
                mysql: Mysql::connect(&config),
                config,
                sender,
                receiver,
            };
            this.mysql_worker();
        });
    }
    fn mysql_worker(&mut self) {
        loop {
            match self.receiver.recv() {
                Ok(req) => {
                    // handle request in Sqlite worker writing to `_backup` tables if functional
                    // remote worker exists, otherwise use sqlite table as normal database.
                    self.handle_request(&req);
                }
                // Got shutdown signal, terminate this thread.
                Err(mpsc::RecvError) => return,
            }
        }
    }
    fn handle_request(&mut self, req: &Request) {}
}
