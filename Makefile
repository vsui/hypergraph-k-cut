.PHONY: test install build

build:
	mkdir -p build; cmake -H. -Bbuild

install: build
	cmake --build build --target install

test: build
	cmake --build build --target test
