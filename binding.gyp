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
                }
            ],
            [
                'OS=="win"', {
                    "sources": [
                        "src/printers/xpsprinter.cpp",
                    ]
                }
            ],
        ]
    }]
}
