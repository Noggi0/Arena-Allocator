set_project("arena")
set_languages("cxx20")

set_optimize("fastest")
set_arch("x64")

target("arena")
    add_files("./main.cpp")

