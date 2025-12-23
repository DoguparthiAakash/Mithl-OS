savedcmd_mithl_core.mod := printf '%s\n'   mithl_core.o | awk '!x[$$0]++ { print("./"$$0) }' > mithl_core.mod
