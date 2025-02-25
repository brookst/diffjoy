{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.poetry2nix.url = "github:nix-community/poetry2nix";

  outputs = { self, nixpkgs, poetry2nix }:
    let
      supportedSystems = [ "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgs = forAllSystems (system: nixpkgs.legacyPackages.${system});
    in
    {
      nixosModules.default = { config, pkgs, lib, ... }:
        {
        options.services.pedal_controller = {
          enable = lib.mkEnableOption "enable the pedal_controller service";
        };

        config = lib.mkIf config.services.pedal_controller.enable {
          systemd.services.pedal_controller = {
            description = "map pedal controller position to keypresses";
            wantedBy = [ "multi-user.target" ];
            after = [ "udev.service" ];
            serviceConfig = {
              Type = "simple";
              ExecStart = "${self.packages.${pkgs.system}.default}/bin/pedal-controller";
              Restart = "on-failure";
              ProtectHome = "read-only";
            };
          };
        };
      };
      packages = forAllSystems (system: let
        inherit (poetry2nix.lib.mkPoetry2Nix { pkgs = pkgs.${system}; }) mkPoetryApplication;
      in {
        default = mkPoetryApplication {
          projectDir = self;
          propagatedBuildInputs = [ pkgs.${system}.kbd ];
        };
      });

      devShells = forAllSystems (system: let
        inherit (poetry2nix.lib.mkPoetry2Nix { pkgs = pkgs.${system}; }) mkPoetryEnv;
      in {
        default = pkgs.${system}.mkShellNoCC {
          packages = with pkgs.${system}; [
            (mkPoetryEnv { projectDir = self; })
            poetry
            python312Packages.jedi-language-server
            python312Packages.black
          ];
        };
      });
    };
}
