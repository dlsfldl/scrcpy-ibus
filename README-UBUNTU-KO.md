# Ubuntu + IBus용 scrcpy 수정본

scrcpy 4.1, 원본 커밋 `19c1261d2e2cbf2b5e6a71a8b64cc1dd3ede06ac` 기준입니다.

`-K`로 실행하면 키를 휴대폰의 물리 키보드 입력으로 전달하고,
한글 조합은 휴대폰 입력기가 처리합니다. 이 수정본은 해당 모드에서
PC의 문자 입력기를 활성화하지 않아 IBus가 키 이벤트를 소비하는 경로를 피합니다.
기본 SDK 입력 모드에는 이 수정이 적용되지 않으므로 **`-K`를 붙여 실행하세요.**

## Ubuntu에서 빌드

비공개 저장소에 접근 가능한 GitHub 계정으로 소스를 받습니다.

```bash
git clone https://github.com/dlsfldl/scrcpy-ibus.git
cd scrcpy-ibus
```

또는 배포한 소스 압축을 풀고 `scrcpy-4.1-ibus` 폴더 안에서 실행합니다.
개발 도구가 없는 경우 먼저 설치합니다.

```bash
sudo apt update
sudo apt install build-essential meson ninja-build pkg-config adb curl \
  libavcodec-dev libavformat-dev libavutil-dev libswresample-dev \
  libavdevice-dev libv4l-dev libusb-1.0-0-dev
```

SDL3 개발 패키지도 필요합니다. 이미 `pkg-config --modversion sdl3`가
3.2.0 이상을 출력한다면 다음 설치는 생략합니다.

```bash
sudo apt install libsdl3-dev
```

사용 중인 Ubuntu에 `libsdl3-dev` 패키지가 없으면 아래의 **SDL3 직접 빌드**를
먼저 진행합니다. SDL2 개발 패키지로는 이 버전을 빌드할 수 없습니다.

압축에는 공식 Android 서버 파일 `scrcpy-server`가 포함되어 있습니다.
소스 저장소에서 직접 작업해서 이 파일이 없다면 다음 주소로 받습니다.

```bash
curl -fL https://github.com/Genymobile/scrcpy/releases/download/v4.1/scrcpy-server-v4.1 \
  -o scrcpy-server
echo 'deacb991ed2509715160ffdc7907e47b4160eb30d1566217e9047fd5b8850cae  scrcpy-server' | sha256sum -c -
```

빌드하고 자동 검사를 실행합니다. 기존 설치본을 덮어쓰지 않습니다.

```bash
meson setup build-ubuntu --buildtype=debug -Dprebuilt_server="$PWD/scrcpy-server"
meson compile -C build-ubuntu
meson test -C build-ubuntu --print-errorlogs
```

## 실행과 확인

휴대폰의 USB 디버깅을 허용한 뒤 실행합니다.

```bash
bash ./run build-ubuntu -K
```

처음에는 scrcpy 창에서 `Alt+k`를 눌러 휴대폰의 물리 키보드 설정을 열고
한국어 배열을 설정합니다. 한/영 전환은 휴대폰 입력기의 설정을 따릅니다.

1. IBus를 실행해 둔 채 휴대폰의 메모 앱에서 영문, 숫자, Backspace를 확인합니다.
2. Ubuntu에서 IBus 한글 모드를 선택한 상태로 scrcpy에 돌아와 입력이 계속 전달되는지 확인합니다.
3. 휴대폰 입력기를 한국어로 전환해 `안녕하세요`를 입력하고 받침 수정, Enter를 확인합니다.
4. 다른 Ubuntu 앱으로 돌아가 IBus 한글 입력이 정상인지 확인합니다.

자동 검사는 실제 scrcpy 창 초기화 후 SDL의 문자 입력 상태를 확인합니다.
수정 전에는 UHID 검사에 실패하고, 수정 후에는 SDK 두 모드·UHID·키보드 비활성화
검사가 통과했습니다. Windows에서 클라이언트 빌드와 전체 13개 검사가 통과했습니다
(`usb=false`, SDL 3.4.12). **Ubuntu/IBus에서의 실제 타이핑은 아직 확인하지 않았습니다.**

계속 문제가 있으면 `ibus version`, `ibus engine`, `echo "$XDG_SESSION_TYPE"`,
`./build-ubuntu/app/scrcpy --version` 결과로 환경을 확인할 수 있습니다.

## SDL3 직접 빌드 — 배포판에 개발 패키지가 없을 때만

아래는 Ubuntu 22.04 이상을 대상으로 한 준비 명령입니다.
SDL을 이 소스 폴더 안에 설치합니다. 같은 터미널에서 이후 scrcpy 빌드를 진행하세요.

```bash
sudo apt install cmake libasound2-dev libpulse-dev libx11-dev libxext-dev \
  libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev \
  libxkbcommon-dev libwayland-dev libdecor-0-dev wayland-protocols \
  libegl1-mesa-dev libgl1-mesa-dev libgbm-dev libdrm-dev \
  libdbus-1-dev libibus-1.0-dev libudev-dev
mkdir -p build-deps
curl -fL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.12.tar.gz \
  -o build-deps/SDL.tar.gz
echo 'b68381f06a7580e63400b3b6eb547ec57d8c3ebde70f9f40e0aba530ba05da27  build-deps/SDL.tar.gz' | sha256sum -c -
tar -xf build-deps/SDL.tar.gz -C build-deps
cmake -S build-deps/SDL-release-3.4.12 -B build-deps/sdl-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/build-deps/sdl" \
  -DCMAKE_INSTALL_LIBDIR=lib -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF \
  -DSDL_X11=ON -DSDL_WAYLAND=ON
cmake --build build-deps/sdl-build --parallel
cmake --install build-deps/sdl-build
export PKG_CONFIG_PATH="$PWD/build-deps/sdl/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$PWD/build-deps/sdl/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```

새 터미널에서 실행할 때는 소스 폴더에서 마지막 `LD_LIBRARY_PATH` 설정을 다시 적용합니다.

참고: [scrcpy 빌드 문서](doc/build.md), [키보드 설정](doc/keyboard.md),
[SDL Linux 빌드 문서](https://github.com/libsdl-org/SDL/blob/release-3.4.12/docs/README-linux.md).
