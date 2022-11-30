-- global config

set_toolchains("gcc")

set_languages("c++17")
set_warnings("all")

add_rules("mode.debug", "mode.release")
add_requires("fmt")


if is_plat("linux") then
    add_syslinks("pthread")
end

--

function use_asan() 
    set_config("cxflags", "-fPIE -fsanitize=address")
    set_config("ldflags", "-fsanitize=address")
end

function build_magio_promise()
    target("magio-promise")
        set_kind("static")
        add_files("src/magio/**.cpp")
        add_includedirs("src", {public = true})
        add_packages("fmt")
end

function build_dev()
    for _, val in ipairs(os.files("dev/**.cpp")) do 
        target(path.basename(val))
            set_kind("binary")
            add_files(val)
            add_deps("magio-promise")
            add_packages("fmt")
    end
end

function build_examples() 
    for _, val in ipairs(os.files("examples/**.cpp")) do 
        target(path.basename(val))
            set_kind("binary")
            add_files(val)
            add_deps("magio-promise")
            add_packages("fmt")
    end
end

use_asan()
--build_dev()
build_magio_promise()
build_examples()