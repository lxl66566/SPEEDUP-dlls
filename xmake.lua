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

target("winmm")
set_kind("shared")
set_filename("winmm-" .. ARCH .. "-" .. SPEEDUP .. ".dll")
add_files("winmm/" .. ARCH .. "/winmm.cpp", "winmm/" .. ARCH .. "/winmm.def")
if is_arch("x64") then
	add_files("winmm/" .. ARCH .. "/winmm_asm.asm")
end
add_defines("SPEEDUP=" .. SPEEDUP)
