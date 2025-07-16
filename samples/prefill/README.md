# Sample Data Generator

The python script `generate.py` generates a sample call history database, which can be used for testing or demonstration purposes.

Call the script and substitute the call history database with the new generated file `callhistory.db` from this folder.

The file `sample-addressbook.csv` contains sample contacts. To use it, add this to your `sip.conf` and remove any ldap entries:

```ini
[csv0]
path="/path/to/sample-addressbook.csv"
```
