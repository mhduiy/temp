#!/bin/bash

awk 'BEGIN{FS="\n";ORS="";str="const char *PASSKEY_SERVICE_DBUS_XML_DATA = \""} {str=str$1} END {str=str"\";";print str > "com.deepin.Passkey.c" }' $1
