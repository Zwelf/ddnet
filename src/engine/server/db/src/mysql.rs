use mysql::Pool;

use crate::MysqlConfig;

pub struct Mysql {
    pool: Pool,
}

impl Mysql {
    pub fn connect(config: &MysqlConfig) -> Self {
        todo!()
    }
}
