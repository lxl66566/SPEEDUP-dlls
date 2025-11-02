SPEEDUP = 2.0
if is_arch("x64") then
	ARCH = "x64"
else
	ARCH = "x86"
end

set_encodings("utf-8")
set_policy("build.warning", true)
set_languages("c++20")
set_optimize("fastest")
if is_arch("x64") then
	add_requires("soundtouch", { arch = ARCH })
else
	add_requires("soundtouch", { arch = ARCH })
end
-- add_requires("soundtouch", { configs = { shared = true } })

target("mmdevapi")
set_kind("shared")
set_filename("MMDevAPI-" .. ARCH .. "-" .. SPEEDUP .. ".dll")
add_files("MMDevAPI/" .. ARCH .. "/mmdevapi.cpp", "MMDevAPI/" .. ARCH .. "/mmdevapi.def")
if is_arch("x64") then
	add_files("MMDevAPI/" .. ARCH .. "/mmdevapi_asm.asm")
end
add_links("ole32", "user32")
add_defines("SPEEDUP=" .. SPEEDUP)

target("dsound_x64")
set_kind("shared")
set_arch("x64")
set_filename("dsound-win64-" .. SPEEDUP .. ".dll")
add_files("dsound/x64/dsound.cpp", "dsound/x64/dsound.def", "dsound/x64/dsound_asm.asm")
add_links("ole32", "user32", "dsound", "dxguid")
add_defines("SPEEDUP=" .. SPEEDUP)

target("dsound_x86")
set_kind("shared")
set_arch("x86")
set_filename("dsound-win32-" .. SPEEDUP .. ".dll")
add_files("dsound/x86/dsound.cpp", "dsound/x86/dsound.def")
add_links("ole32", "user32", "dsound", "dxguid")
add_defines("SPEEDUP=" .. SPEEDUP)
