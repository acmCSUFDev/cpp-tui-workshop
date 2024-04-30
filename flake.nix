{
	inputs = {
		nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
		flake-utils.url = "github:numtide/flake-utils";
	};

	outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system: let
		pkgs = nixpkgs.legacyPackages.${system};
		lib = nixpkgs.lib;

		buildInputs = with pkgs; [
			ftxui # tui
			libcpr # http
			nlohmann_json # json
		];

		CXXFLAGS = builtins.replaceStrings
			["\n"] [" "]
			(builtins.readFile ./compile_flags.txt);
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
					for bin in *.out; do
						dst="$out/bin/$(basename $bin .out)"
						mv $bin $dst
					done
				'';
			};

			apps = builtins.listToAttrs (map (exe: {
				name = exe;
				value = {
					type = "app";
					program = "${self.packages.${system}.default}/bin/${exe}";
				};
			}) ["weather-app" "basic-demo"]);
		}
	);
}
