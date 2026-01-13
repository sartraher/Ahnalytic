project "OpenSSL"
    kind "Makefile"

    local stamp_file = ".built"

    filter "system:linux or system:macosx"
        buildcommands {
            ('[ ! -f "%s" ] && { chmod +x ./build_unix.sh && ./build_unix.sh && echo ok > "%s"; } || true')
				:format(stamp_file, stamp_file)
        }
        buildoutputs { stamp_file }

    filter "system:windows"
        buildcommands {
            ('if not exist %s (build_window.bat && echo ok > %s)')
                :format(stamp_file, stamp_file)
        }
        buildoutputs { stamp_file }
