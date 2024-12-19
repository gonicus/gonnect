The database migration files must follow these rules, or they might destroy something:

* Each file name must be "XXX.sql", with X being a single digit (0-9).
* The number must be incremented from the latest file.
* Each file must contain valid SQL syntax and nothing more.
* Each file may contain more than one SQL statement if they are terminated with a semicolon.
* Don't forget to include the file in migrationscripts.qrc.

