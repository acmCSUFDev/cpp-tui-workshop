{
	inputs = {
		nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
		flake-utils.url = "github:numtide/flake-utils";
	};

	outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system: let
		lib = nixpkgs.lib;

		pkgs = nixpkgs.legacyPackages.${system}.extend (self: super: {
			ftxui = super.ftxui.overrideAttrs (old: {
				cmakeFlags = [
					"-DFTXUI_ENABLE_INSTALL=ON"
					"-DFTXUI_BUILD_EXAMPLES=OFF"
					"-DFTXUI_BUILD_TESTS=OFF"
					"-DFTXUI_BUILD_DOCS=OFF"
					"-DCMAKE_BUILD_TYPE=Release"
				];
			});
		});

		buildInputs = with pkgs; [
			ftxui # tui
			libcpr # http
			nlohmann_json # json
		];

		CXXFLAGS = lib.concatStringsSep " " [
			"-std=c++20"
			"-Wall"
			"-Wextra"
			"-Werror"
			"-pedantic"
		];
	in
		{
			devShells.default = pkgs.clangStdenv.mkDerivation rec {
				name = "cpp-tui-workshop-shell";
				inherit CXXFLAGS buildInputs;
				nativeBuildInputs = with pkgs; [ clang-tools cmake ];
			};

			packages.default = pkgs.clangStdenv.mkDerivation rec {
				name = "cpp-tui-workshop";
				src = self;

				inherit CXXFLAGS buildInputs;
				nativeBuildInputs = with pkgs; [ cmake ];

				installPhase = ''
					mkdir -p $out/bin
					mv cpp-tui-workshop $out/bin
				'';

				meta = {
					mainProgram = "cpp-tui-workshop";
				};
			};
		}
	);
}
