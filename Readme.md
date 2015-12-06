
# FreeDNSUpdate

## Summary

This project started as a simple way of updating a FreeDNS A record to reflect the changes to my servers' IPs on startup.

## Usage

You'll need an INI file named 'freedns.ini' in the same directory as the application file. The INI file should look similar to this

```
[FREEDNS]
UKEY=<unique-update-key>
```

The UKEY field is the unique key used by the FreeDNS update web page to change the record information. The application calls http://freedns.afraid.org/dynamic/update.php?<ukey> to make the update.

The program will create a log file named 'freedns.log' in the same directory as the application.