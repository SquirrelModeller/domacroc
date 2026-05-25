{
  description = "Simple macro keyboard program (domacroc)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    systems = ["x86_64-linux" "aarch64-linux"];
  in {
    packages = nixpkgs.lib.genAttrs systems (
      system: let
        pkgs = import nixpkgs {
          inherit system;
        };
      in {
        default = pkgs.callPackage ./default.nix {};
      }
    );

    devShells = nixpkgs.lib.genAttrs systems (
      system: let
        pkgs = import nixpkgs {
          inherit system;
        };
      in {
        default = pkgs.mkShell {
          packages = with pkgs; [
            gcc
            gnumake
          ];
        };
      }
    );

    nixosModules.default = {
      config,
      lib,
      pkgs,
      ...
    }: let
      cfg = config.services.domacroc;
      package = self.packages.${pkgs.stdenv.hostPlatform.system}.default;
    in {
      options.services.domacroc = {
        enable = lib.mkEnableOption "domacroc macro keyboard daemon";
      };

      config = lib.mkIf cfg.enable {
        # allow the input group to open /dev/uinput
        services.udev.extraRules = ''
          KERNEL=="uinput", GROUP="input", MODE="0660"
        '';

        systemd.user.services.domacroc = {
          description = "domacroc macro keyboard daemon";
          wantedBy = ["default.target"];
          serviceConfig = {
            ExecStart = "${package}/bin/domacroc";
            Restart = "on-failure";
          };
        };
      };
    };
  };
}
