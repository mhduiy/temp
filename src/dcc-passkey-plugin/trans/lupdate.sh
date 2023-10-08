#!/bin/bash
cp ".transifexrc" ${HOME}/

lupdate ./ -ts -no-obsolete translations/dcc-passkey-plugin.ts

tx push -s -f --branch m20
