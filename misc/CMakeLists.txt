file(GLOB_RECURSE DBUS_CONF
    "dbus/system.d/*.conf"
)
install(FILES ${DBUS_CONF} DESTINATION share/dbus-1/system.d/)

file(GLOB_RECURSE DBUS_SYSTEM_SERVICE
    "dbus/system-services/*.service"
)
install(FILES ${DBUS_SYSTEM_SERVICE} DESTINATION share/dbus-1/system-services/)

file(GLOB_RECURSE SYSTEMD_SERVICE
    "systemd/system/*.service"
)
install(FILES ${SYSTEMD_SERVICE} DESTINATION lib/systemd/system/)
