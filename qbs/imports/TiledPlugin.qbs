import qbs 1.0

DynamicLibrary {
    Depends { name: "libtiled" }
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: "gui" }

    cpp.cxxLanguageVersion: "c++11"
    cpp.defines: [
        "QT_DEPRECATED_WARNINGS",
        "QT_DISABLE_DEPRECATED_BEFORE=0x050900",
        "QT_NO_FOREACH",
        "QT_NO_URL_CAST_FROM_STRING"
    ]
    Group {
        qbs.install: true
        qbs.installDir: {
            if (qbs.targetOS.contains("windows") || project.linuxArchive)
                return "plugins/tiled"
        }
        /*指定文件标记名,通过标记名来匹配文件*/
        fileTagsFilter: "dynamiclibrary"
    }
    //给指定模式文件添加标签者
    FileTagger {
        /*自定匹配的模式["1.cpp,2.cpp,*.cpp"]*/
        patterns: "plugin.json"
        /*文件标记名"cpp"*/
        fileTags: ["qt_plugin_metadata"]
    }
}
