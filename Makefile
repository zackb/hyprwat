default: debug

debug:
	cmake --preset debug
	cmake --build --preset debug
	ln -sf build/debug/compile_commands.json

release:
	cmake --preset release
	cmake --build --preset release
	ln -sf build/release/compile_commands.json

install: release
	cmake --install build/release

run: debug
	./build/debug/hyprwat foo:FooZZZZZZZZZZZZZZZZZZZZZZZZZZZZ bar:Bar* zbz:'Zack Bartel'

run2: debug
	./build/debug/hyprwat foo:Foo bar:Bar zbz:'Baz'
run3: debug
	echo -e "foo:Foo\nbar:Bar\nbaz:Baz\nqux:Qux\nquux:Quux\ncorge:Corge\ngrault:Grault\ngarply:Garply*\nwaldo:Waldo\nfred:Fred\nplugh:Plugh\nxyzzy:Xyzzy\nthud:Thud" | ./build/debug/hyprwat


.PHONY: fmt
fmt:
	@echo "Formatting code with clang-format..."
	@find ./src ./include \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" -o -name "*.h" \) ! -name "json.hpp" -print0 | \
	xargs -0 -n 1 clang-format -i
	@echo "Done."

clean:
	rm -rf build

old: 
	gcc -c src/wayland/protocols/wlr-layer-shell-unstable-v1-client-protocol.c -o layer_shell.o
	gcc -c src/wayland/protocols/xdg-shell-client-protocol.c -o xdg_shell.o
	g++ -std=c++17 -o wayland_popup \
    src/main_old.cpp \
    ext/imgui/*.cpp \
    ext/imgui/backends/imgui_impl_opengl3.cpp \
	layer_shell.o xdg_shell.o \
    -Iext/imgui -Iext/imgui/backends \
    -I/usr/include \
    -lwayland-client -lwayland-egl -lEGL -lGL -ldl -lm -pthread

