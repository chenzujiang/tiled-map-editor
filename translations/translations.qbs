import qbs 1.0

Product {
    name: "translations"
    type: "qm"
    files: "*.ts"

    // 禁用语言，因为它们太过时了
    excludeFiles: [
        "tiled_lv.ts",
    ]

    Depends { name: "Qt.core" }

    Group {
        /*指定文件标记名,通过标记名来匹配文件*/
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: {
            if (qbs.targetOS.contains("windows") || project.linuxArchive)
                return "translations"
            else if (qbs.targetOS.contains("macos"))
                return "Tiled.app/Contents/Translations"
            else
                return "share/tiled/translations"
        }
    }
}
