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

file(GLOB DCONFIG_FILES
    "dsg-configs/org.deepin.dde.passkey.dcc-plugin.json"
)
install(FILES ${DCONFIG_FILES} DESTINATION share/dsg/configs/org.deepin.dde.passkey/)

file(GLOB POLKIT_FILES
    "polkit/com.deepin.dde.passkey.dcc-plugin.policy"
)
install(FILES ${POLKIT_FILES} DESTINATION share/polkit-1/actions/)
