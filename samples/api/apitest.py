#!/usr/bin/env python3
from json import loads, dumps
from gi.repository import GLib
from pydbus import SessionBus

api = None

def onRinging(id, data):
    print(f'Incoming call ID {id}')
    d = loads(data)
    print(dumps(d))

    api.AddMetaData(id, "{}")

def onCallEnded(id):
    print(f'Call ID {id} terminated')

def onCallAnswered(id):
    print(f'Call ID {id} answered')

if __name__ == '__main__':
    bus = SessionBus()
    try:
        api = bus.get('de.gonicus.gonnect', 'api')
    except:
        print("Error: failed to initialize DBus connection")
        exit(1)

    print(f'GOnnect API version {api.version} - listening for events...')

    loop = GLib.MainLoop()

    api.onRinging = onRinging
    api.onCallEnded = onCallEnded
    api.onCallAnswered = onCallAnswered

    loop.run()
