#!/usr/bin/env python3

import sys
import os
import os.path
import time
import sqlite3

DB_FILE = 'callhistory.db'
MIGRATE_DIR = '../migrate/callhistory'

def init_database():
    if os.path.isfile(DB_FILE):
        os.remove(DB_FILE)

    file_names = []

    for entry in os.scandir(MIGRATE_DIR):
        if entry.name.endswith('.sql'):
            file_names.append(entry.name)

    file_names.sort()

    con = sqlite3.connect(DB_FILE)
    cur = con.cursor()

    for file_name in file_names:
        with open(os.path.join(MIGRATE_DIR, file_name)) as f:
            cur.executescript(f.read())

    version = file_names[-1].split('.')[0]

    cur.execute(f'UPDATE appinfo SET value = "{version}" WHERE key = "db_scheme_version"')
    con.commit()
    print(f'Initialized database at version {version}')

    return con


def fill_database(con):
    cur = con.cursor()
    cur.execute("""
        INSERT INTO contactflags VALUES
            ('+496990009872', 5, 1, 0),
            ('+4915228817386', 17, 0, 0),
            ('+496990009876', 42, 1, 0),
            ('+496990009871', 3, 0, 0)
    """)
    con.commit()

    timestamp = int(time.time())
    cur.execute(f"""
        INSERT INTO history VALUES
            (1, {timestamp - 400 - (24 * 60 * 60)}, '+4915228817386', 'account0', 1, 589, '', NULL),
            (2, {timestamp - 300}, '"Juan Pérez" <sip:+496990009872@foo.org>', 'account0', 1, 129, '', NULL),
            (3, {timestamp - 60}, '"Juan Pérez" <sip:+496990009872@foo.org>', 'account0', 1, 58, '', NULL)
    """)
    con.commit()

def main():
    
    if os.getcwd().split('/')[-1] != 'samples':
        print('Please call the script with the working directory being the one it is place in.')
        sys.exit(1) 

    con = init_database()
    fill_database(con)

    print(f'Database generated in file {DB_FILE}')


if __name__ == '__main__':
    main()
