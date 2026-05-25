{ pkgs ? import <nixpkgs> { } }:

pkgs.stdenv.mkDerivation {
  pname = "domacroc";
  version = "1.0";

  src = ./.;

  nativeBuildInputs = [ pkgs.gcc ];

  buildPhase = ''
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp domacroc $out/bin/
    cp domacro-send $out/bin/
  '';

  meta = with pkgs.lib; {
    description = "Simple macro keyboard program";
    license = licenses.mit;
    maintainers = with maintainers; [ "Squirrel Modeller" ];
    platforms = platforms.linux;
  };
}
