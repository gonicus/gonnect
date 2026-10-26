option(ENABLE_HARDENING "Enable compiler and linker hardening" ON)

if(ENABLE_HARDENING)
    include(CheckPIESupported)
    check_pie_supported()

    if(LINUX)
        add_compile_options(  
            -fstack-protector-strong
            -fstack-clash-protection
            -ftrivial-auto-var-init=zero
            -D_GLIBCXX_ASSERTIONS
        )   
        if(NOT FLATPAK)
            add_compile_options($<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=3>)
        endif()
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            add_compile_options(-fcf-protection=full)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            add_compile_options(-mbranch-protection=standard)
        endif()
        add_link_options(-Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)
    elseif(MSVC)
        add_compile_options(/guard:cf /sdl)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS) # logfault headers...
        add_link_options(/guard:cf /CETCOMPAT)
    endif()
endif() 
