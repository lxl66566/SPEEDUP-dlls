SPEEDUP = 2.0

target("mmdevapi_x64")
set_kind("shared")
set_arch("x64")
set_filename("mmdevapi")
add_files("MMDevAPI/x64/mmdevapi.cpp", "MMDevAPI/x64/mmdevapi.def", "MMDevAPI/x64/mmdevapi_asm.asm")
add_links("ole32", "user32")
add_defines("SPEEDUP=" .. SPEEDUP)

target("mmdevapi_x86")
set_kind("shared")
set_arch("x86")
set_filename("mmdevapi")
add_files("MMDevAPI/x86/mmdevapi.cpp", "MMDevAPI/x86/mmdevapi.def")
add_links("ole32", "user32")
add_defines("SPEEDUP=" .. SPEEDUP)
