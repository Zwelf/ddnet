use rusqlite::Connection;

pub struct Sqlite {
    conn: Connection,
}

impl Sqlite {
    fn record_race(table_suffix: &str) -> String {
        // TODO: MAX_NAME_LENGTH_SQL for Name
        format!(
            "CREATE TABLE IF NOT EXISTS record_race{table_suffix} ( \
      		    Map VARCHAR(128) COLLATE BINARY NOT NULL, \
      		    Name VARCHAR(15) COLLATE BINARY NOT NULL, \
      		    Timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, \
      		    Time FLOAT DEFAULT 0, \
      		    Server CHAR(4), \
      		    cp1 FLOAT DEFAULT 0, cp2 FLOAT DEFAULT 0, cp3 FLOAT DEFAULT 0, \
      		    cp4 FLOAT DEFAULT 0, cp5 FLOAT DEFAULT 0, cp6 FLOAT DEFAULT 0, \
      		    cp7 FLOAT DEFAULT 0, cp8 FLOAT DEFAULT 0, cp9 FLOAT DEFAULT 0, \
      		    cp10 FLOAT DEFAULT 0, cp11 FLOAT DEFAULT 0, cp12 FLOAT DEFAULT 0, \
      		    cp13 FLOAT DEFAULT 0, cp14 FLOAT DEFAULT 0, cp15 FLOAT DEFAULT 0, \
      		    cp16 FLOAT DEFAULT 0, cp17 FLOAT DEFAULT 0, cp18 FLOAT DEFAULT 0, \
      		    cp19 FLOAT DEFAULT 0, cp20 FLOAT DEFAULT 0, cp21 FLOAT DEFAULT 0, \
      		    cp22 FLOAT DEFAULT 0, cp23 FLOAT DEFAULT 0, cp24 FLOAT DEFAULT 0, \
      		    cp25 FLOAT DEFAULT 0, \
      		    GameId VARCHAR(64), \
      		    DDNet7 BOOL DEFAULT FALSE, \
      		    PRIMARY KEY (Map, Name, Time, Timestamp, Server) \
            );"
        )
    }
    fn record_teamrace(table_suffix: &str) -> String {
        // TODO: Figure out whether BINARY type is equivalent to BLOB type
        // TODO: MAX_NAME_LENGTH_SQL for Name
        format!(
            "CREATE TABLE IF NOT EXISTS record_teamrace{table_suffix} ( \
                Map VARCHAR(128) COLLATE BINARY NOT NULL, \
                Name VARCHAR(16) COLLATE BINARY NOT NULL, \
                Timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, \
                Time FLOAT DEFAULT 0, \
                ID BINARY NOT NULL, \
                GameId VARCHAR(64), \
                DDNet7 BOOL DEFAULT FALSE, \
                PRIMARY KEY (Id, Name) \
            );"
        )
    }
    fn record_saves(table_suffix: &str) -> String {
        format!(
            "CREATE TABLE IF NOT EXISTS record_saves{table_suffix} ( \
                Savegame TEXT COLLATE BINARY NOT NULL, \
                Map VARCHAR(128) COLLATE BINARY NOT NULL, \
                Code VARCHAR(128) COLLATE BINARY NOT NULL, \
                Timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, \
                Server CHAR(4), \
                DDNet7 BOOL DEFAULT FALSE, \
                SaveId VARCHAR(36) DEFAULT NULL, \
                PRIMARY KEY (Map, Code) \
            )"
        )
    }

    pub fn setup_backup(&mut self) -> Result<(), rusqlite::Error> {
        self.conn.execute(&Sqlite::record_race("_backup"), [])?;
        self.conn.execute(&Sqlite::record_teamrace("_backup"), [])?;
        self.conn.execute(&Sqlite::record_saves("_backup"), [])?;
        Ok(())
    }
    fn setup_tables(&mut self) -> Result<(), rusqlite::Error> {
        self.conn.execute(&Sqlite::record_race(""), [])?;
        self.conn.execute(&Sqlite::record_teamrace(""), [])?;
        self.conn.execute(&Sqlite::record_saves(""), [])?;

        // TODO: also populate table with maps from map-directory
        //       allowing `/map`+random map (unfinished) to work from
        //       sqlite without populating the database manually.
        //       could also let these commands work when Mysql database
        //       is down.
        self.conn.execute(
            "CREATE TABLE IF NOT EXISTS record_maps ( \
                Map VARCHAR(128) COLLATE BINARY NOT NULL,  \
                Server VARCHAR(32) COLLATE BINARY NOT NULL,  \
                Mapper VARCHAR(128) COLLATE BINARY NOT NULL,  \
                Points INT DEFAULT 0, \
                Stars INT DEFAULT 0, \
                Timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,  \
                PRIMARY KEY (Map) \
    		)",
            [],
        )?;

        // TODO: MAX_NAME_LENGTH_SQL for Name
        self.conn.execute(
            "CREATE TABLE IF NOT EXISTS record_points ( \
          		Name VARCHAR(16) COLLATE BINARY NOT NULL, \
          		Points INT DEFAULT 0, \
          		PRIMARY KEY (Name) \
    		)",
            [],
        )?;
        Ok(())
    }
    pub fn open(path: &String) -> Result<Self, rusqlite::Error> {
        let mut this = Sqlite {
            conn: Connection::open(path)?,
        };
        this.setup_tables()?;
        Ok(this)
        // TODO: setup database schema
    }
}
