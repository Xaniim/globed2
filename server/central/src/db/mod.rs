use globed_shared::{error, info};
use rocket::{
    fairing::{self, AdHoc},
    Build, Rocket,
};
use rocket_db_pools::Database;

mod dbimpl;

pub async fn run_migrations(rocket: Rocket<Build>) -> fairing::Result {
    match GlobedDb::fetch(&rocket) {
        Some(db) => match sqlx::migrate!().run(&**db).await {
            Ok(()) => Ok(rocket),
            Err(e) => {
                error!("Failed to intiialize sqlx database: {e}");
                Err(rocket)
            }
        },
        None => Err(rocket),
    }
}

pub fn migration_fairing() -> impl fairing::Fairing {
    AdHoc::try_on_ignite("Database migrations", |rocket| async {
        info!("Running migrations");
        run_migrations(rocket).await
    })
}

#[derive(Database)]
#[database("globed_db")]
pub struct GlobedDb(sqlx::SqlitePool);
