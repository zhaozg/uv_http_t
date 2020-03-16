all:	build
	ninja -C build

test: dist
	./out/Release/uv_http_t-test

example: dist
	./out/Release/uv_http_t-example

build:
	mkdir build
	meson build
clean:
	rf -rf build

.PHONY: test example dist build
