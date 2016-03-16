{
    "targets": [{
        "target_name": "imageprinter",
        "include_dirs": [
            "<!(node -e \"require('nan')\")"
        ],
        "sources": [
            "src/addon.cpp",
            "src/nodeimageprinter.cpp",
            "src/printers/fileprinter.cpp",
            "src/workerthread.cpp"
        ],
        "conditions": [
            [
                'OS=="linux"', {
                    "cflags_cc+": [
                        "-fexceptions",
                        "-std=c++14"
                    ]
                }
            ],
            [
                'OS=="mac"', {
                    "xcode_settings": {
                        "MACOSX_DEPLOYMENT_TARGET": "10.10",
                        "OTHER_CPLUSPLUSFLAGS": [
                            "-std=c++14",
                            "-stdlib=libc++"
                        ],
                        "OTHER_LDFLAGS": [
                            "-stdlib=libc++"
                        ]
                    }
                }
            ],
            [
                'OS=="win"', {
                    "sources": [
                        "src/printers/xps/printticket.cpp",
                        "src/printers/xps/xpsprinter.cpp"
                    ]
                }
            ],
        ]
    }]
}
