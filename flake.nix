{
  description = "Oh PSP - Modular Logic Puzzle Collection for PlayStation Portable";

  # Binary cache for pspdev toolchain (avoids rebuilding psp-gcc/pspsdk from source)
  nixConfig = {
    extra-substituters = [
      "https://pspdev.cachix.org"
      "https://cache.nixos.org"
    ];
    extra-trusted-public-keys = [
      "pspdev.cachix.org-1:lFw1M0EYJeN3Y2xHR7spiuPmThrNDXo8Z9I0Jgzig/0="
      "cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY="
    ];
  };

  inputs = {
    # 1. nixpkgs: Основной репозиторий пакетов NixOS (ветка unstable).
    # Используется для хостовых инструментов (компилятор GCC, CMake, SDL2/SDL3 для Linux, эмулятор PPSSPP).
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    # 2. flake-parts: Фреймворк для модульной структуры флейка.
    # Автоматизирует perSystem генерацию для нескольких платформ (x86_64, aarch64, darwin).
    flake-parts.url = "github:hercules-ci/flake-parts";

    # 3. pspdev: Nix-флейк официального PSPSDK (pspdev-nix).
    # Поставляет кросс-тулчейн allegrex MIPS: psp-gcc, psp-binutils, pspsdk, psp-cmake и библиотеки для PSP.
    pspdev.url = "github:pspdev/pspdev-nix";
  };

  outputs =
    {
      nixpkgs,
      flake-parts,
      pspdev,
      ...
    }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      perSystem =
        { system, pkgs, ... }:
        let
          # Use pspdev's pinned nixpkgs with pspdev overlay to ensure binary cache
          # compatibility for psp-gcc, pspsdk, and newlib
          pspPkgs = import pspdev.inputs.nixpkgs {
            inherit system;
            overlays = [ pspdev.overlays.default ];
          };
        in
        {
          packages = {
            # -------------------------------------------------------------
            # Target 1: PSP EBOOT.PBP (Кросс-компиляция под реальную PSP)
            # -------------------------------------------------------------
            default = pspdev.lib.pspMkDerivation { pkgs = pspPkgs; } {
              pname = "oh-psp";
              version = "0.1.0";
              src = ./.;
              buildSystem = "cmake";
              buildInputs = [
                # psp-sdl2: Порт SDL2 под архитектуру MIPS Allegrex (PSP).
                # Отвечает за окно, опрос физических кнопок PSP и цикл рендеринга.
                pspPkgs.psp-sdl2

                # psp-pspgl: Библиотека OpenGL для PSP.
                # Транслирует вызовы OpenGL/SDL2 в команды аппаратного видеочипа PSP (Graphics Engine).
                pspPkgs.psp-pspgl

                # psp-libpspvram: Менеджер сверхбыстрой встроенной видеопамяти PSP (2 МБ eDRAM).
                # Необходим для размещения кадрового буфера (framebuffer) и текстур в VRAM.
                pspPkgs.psp-libpspvram
              ];
              postInstall = ''
                if [ -d assets ]; then
                  cp -r assets "$out/assets"
                fi
              '';
            };

            # -------------------------------------------------------------
            # Target 2: Host Desktop Executable (Нативный бинарник для Linux/macOS)
            # -------------------------------------------------------------
            desktop = pkgs.stdenv.mkDerivation {
              pname = "oh-psp-desktop";
              version = "0.1.0";
              src = ./.;
              nativeBuildInputs = [
                # cmake: Система сборки проекта.
                pkgs.cmake

                # pkg-config: Инструмент для поиска путей и флагов заголовочных файлов SDL2.
                pkgs.pkg-config
              ];
              buildInputs = [
                # SDL2: В nixpkgs-unstable это sdl2-compat (официальная прослойка совместимости API SDL2).
                pkgs.SDL2

                # sdl3: Базовая библиотека SDL 3.x, которую sdl2-compat вызывает внутри себя для окон и ввода.
                pkgs.sdl3
              ];
              cmakeFlags = [
                "-DPLATFORM_PSP=OFF"
                "-DBUILD_TESTS=ON"
              ];
              doCheck = true;
              checkPhase = ''
                ctest --output-on-failure
              '';
              installPhase = ''
                mkdir -p $out/bin
                install -m755 oh_psp $out/bin/oh-psp
                if [ -d ../assets ]; then
                  cp -r ../assets $out/bin/assets
                elif [ -d assets ]; then
                  cp -r assets $out/bin/assets
                fi
              '';
            };
          };

          devShells = {
            # -----------------------------------------------------------
            # Development Shell для PSP (nix develop)
            # -----------------------------------------------------------
            default = pspdev.lib.pspMkShell { pkgs = pspPkgs; } {
              # includeTools = false отключает сборку устаревших утилит psplink,
              # которые падают с ошибками при компиляции через GCC 15.
              includeTools = false;
              psp-packages = [
                # psp-sdl2: Заголовочные файлы и статическая библиотека SDL2 для PSP
                pspPkgs.psp-sdl2

                # psp-pspgl: Заголовочные файлы GL и реализация OpenGL для PSP
                pspPkgs.psp-pspgl

                # psp-libpspvram: Заголовочные файлы и библиотека управления VRAM PSP
                pspPkgs.psp-libpspvram
              ];
              packages = with pkgs; [
                # cmake: Генератор файлов сборки под хост и под PSP (через psp-cmake)
                cmake

                # pkg-config: Поиск библиотек хоста
                pkg-config

                # ninja: Быстрый сборщик проектов
                ninja

                # SDL2: Библиотека SDL2 хоста для компиляции десктопной версии
                SDL2

                # sdl3: Библиотека SDL3 хоста, необходимая для работы sdl2-compat
                sdl3

                # ppsspp: Эмулятор PlayStation Portable для тестирования EBOOT.PBP
                ppsspp
              ] ++ [
                # psp-pkg-config: Специализированный pkg-config с путями к sysroot PSP
                pspPkgs.psp-pkg-config

                # ebootsigner: Утилита для подписи EBOOT.PBP для запуска на официальных прошивках (OFW)
                pspPkgs.ebootsigner
              ];

              shellHook = ''
                export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath [ pkgs.SDL2 pkgs.sdl3 ]}:$LD_LIBRARY_PATH"
              '';
            };

            # -----------------------------------------------------------
            # Development Shell для Host (nix develop .#host)
            # -----------------------------------------------------------
            host = pkgs.mkShell {
              packages = with pkgs; [
                # cmake: Генератор проектов
                cmake

                # pkg-config: Поиск библиотек
                pkg-config

                # ninja: Быстрая сборка
                ninja

                # SDL2: Заголовочные файлы и линковка SDL2
                SDL2

                # sdl3: Бэкенд SDL3 для запуска sdl2-compat
                sdl3

                # gcc: Нативный компилятор хоста (C++20)
                gcc

                # ppsspp: Эмулятор PSP для быстрого запуска
                ppsspp
              ];

              shellHook = ''
                export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath [ pkgs.SDL2 pkgs.sdl3 ]}:$LD_LIBRARY_PATH"
              '';
            };
          };
        };
    };
}
