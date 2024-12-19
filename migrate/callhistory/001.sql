CREATE TABLE "appinfo" (
	"id"	INTEGER,
	"key"	INTEGER,
	"value"	TEXT,
	PRIMARY KEY("id" AUTOINCREMENT)
);

INSERT INTO appinfo (key, value) VALUES('db_scheme_version', '001');

CREATE TABLE "history" (
	"id"	INTEGER,
	"time"	INTEGER,
	"remoteUrl"	TEXT,
	"account"	TEXT,
	"type"	INTEGER,
	"durationSeconds"	INTEGER,
	PRIMARY KEY("id" AUTOINCREMENT)
);

